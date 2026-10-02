#include <Windows.h>

#include "logger.h"
#include "selection_reader.h"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
HANDLE worker_thread = nullptr;

std::filesystem::path EnvironmentPath(const wchar_t* name) {
    const DWORD length = GetEnvironmentVariableW(name, nullptr, 0);
    if (length == 0) return {};
    std::vector<wchar_t> buffer(length);
    const DWORD copied = GetEnvironmentVariableW(name, buffer.data(), length);
    if (copied == 0 || copied >= length) throw std::runtime_error("Environment variable changed during read.");
    std::filesystem::path path(buffer.data());
    if (!path.is_absolute()) throw std::runtime_error("MEB environment paths must be absolute.");
    return path;
}

DWORD EnvironmentDword(const wchar_t* name, DWORD fallback) {
    const DWORD length = GetEnvironmentVariableW(name, nullptr, 0);
    if (length == 0) return fallback;
    std::vector<wchar_t> buffer(length);
    const DWORD copied = GetEnvironmentVariableW(name, buffer.data(), length);
    if (copied == 0 || copied >= length) return fallback;
    wchar_t* end = nullptr;
    const unsigned long value = wcstoul(buffer.data(), &end, 10);
    if (end == buffer.data() || *end != L'\0' || value > 60000UL) return fallback;
    return static_cast<DWORD>(value);
}

std::filesystem::path ModuleDirectory(HMODULE module) {
    std::vector<wchar_t> buffer(32768);
    const DWORD size = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0 || size >= buffer.size()) throw std::runtime_error("Cannot resolve DLL path.");
    return std::filesystem::path(std::wstring(buffer.data(), size)).parent_path();
}

std::filesystem::path FallbackLog() {
    std::vector<wchar_t> buffer(32768);
    const DWORD size = GetTempPathW(static_cast<DWORD>(buffer.size()), buffer.data());
    if (size == 0 || size >= buffer.size()) return {};
    return std::filesystem::path(buffer.data()) / L"MinecraftEldenBridge" / L"MinecraftEldenBridge.log";
}

void FutureMinHookInitialization(meb::Logger& log) {
    // TODO/research: verify the game build, targets, ABI and thread before adding MinHook.
    // No addresses, signatures, internal calls or MH_* calls exist in milestone 1.
    log.Info("MinHook integration: TODO/research; no hooks activated.");
}

std::string SelectionMessage(const meb::Selection& selection) {
    std::ostringstream message;
    message << std::setprecision(17) << "minecraft_id=" << selection.minecraft_id
            << " dimension=" << selection.dimension << " position=(x=" << selection.position.x
            << ", y=" << selection.position.y << ", z=" << selection.position.z << ')';
    return message.str();
}

bool LogSelectionRead(meb::Logger& log, const std::filesystem::path& selection_path,
                      std::string& last_message, std::string& last_error) {
    const auto result = meb::ReadSelection(selection_path);
    if (!result.selection) {
        if (result.error != last_error) {
            last_error = result.error;
            last_message.clear();
            return log.Warn("Selection unavailable: " + result.error);
        }
        return true;
    }
    last_error.clear();
    const auto message = SelectionMessage(*result.selection);
    if (message == last_message) return true;
    last_message = message;
    return log.Info(message);
}

DWORD WINAPI InitializeBridge(void* context) noexcept {
    meb::Logger log;
    try {
        const auto directory = ModuleDirectory(static_cast<HMODULE>(context));
        auto log_directory = EnvironmentPath(L"MEB_LOG_DIRECTORY");
        if (log_directory.empty()) log_directory = directory;
        if (!log.Open(log_directory / L"MinecraftEldenBridge.log", FallbackLog())) return 2;
        if (!log.Info("MinecraftEldenBridge hook loaded.")) return 2;
        FutureMinHookInitialization(log);
        auto selection_path = EnvironmentPath(L"MEB_SELECTION_JSON");
        if (selection_path.empty()) selection_path = directory / L"minecraft_selection.json";
        log.Info("Selection file: " + selection_path.u8string());
        const DWORD poll_ms = EnvironmentDword(L"MEB_SELECTION_POLL_MS", 0);
        std::string last_message;
        std::string last_error;
        if (!LogSelectionRead(log, selection_path, last_message, last_error)) return 2;
        if (poll_ms == 0) {
            if (!last_error.empty()) return 1;
            return log.Info("Initialization complete.") ? 0 : 2;
        }
        log.Info("Selection polling active. interval_ms=" + std::to_string(poll_ms) +
                 " note=20_frames_at_60fps_is_about_333ms");
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
            if (!LogSelectionRead(log, selection_path, last_message, last_error)) return 2;
        }
    } catch (const std::exception& error) {
        log.Error(error.what());
        return 3;
    } catch (...) {
        log.Error("Unexpected initialization failure.");
        return 3;
    }
}
}

// Test-host API. Never call it from DllMain: waiting there would block the worker.
extern "C" __declspec(dllexport) DWORD WINAPI MEB_WaitForInitialization(DWORD timeout_ms) noexcept {
    if (!worker_thread) return ERROR_INVALID_HANDLE;
    const DWORD wait = WaitForSingleObject(worker_thread, timeout_ms);
    if (wait == WAIT_TIMEOUT) return ERROR_TIMEOUT;
    if (wait != WAIT_OBJECT_0) return GetLastError();
    DWORD code = 3;
    return GetExitCodeThread(worker_thread, &code) ? code : GetLastError();
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Pin before dispatch so FreeLibrary cannot unmap queued/running worker code.
        // This milestone deliberately has process lifetime, without hot unloading.
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                               reinterpret_cast<LPCWSTR>(module), &pinned)) return FALSE;
        worker_thread = CreateThread(nullptr, 0, InitializeBridge, module, 0, nullptr);
        if (!worker_thread) {
            OutputDebugStringA("MinecraftEldenBridge: worker creation failed.\n");
            return FALSE;
        }
        // Static CRT requires thread notifications; do not DisableThreadLibraryCalls.
    }
    // No I/O, locks or waits during detach. The OS reclaims the one retained thread handle.
    return TRUE;
}
