#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc != 5) throw std::runtime_error("Usage: bridge_smoke DLL SELECTION_JSON LOG_DIR EXPECTED_CODE");
        const auto dll = std::filesystem::absolute(argv[1]);
        const auto selection = std::filesystem::absolute(argv[2]);
        const auto directory = std::filesystem::absolute(argv[3]);
        const auto expected = static_cast<DWORD>(std::stoul(argv[4]));
        std::filesystem::create_directories(directory);
        const auto log_path = directory / L"MinecraftEldenBridge.log";
        {
            std::ofstream fresh(log_path, std::ios::trunc);
            if (!fresh) throw std::runtime_error("Cannot prepare smoke-test log.");
        }
        if (!SetEnvironmentVariableW(L"MEB_SELECTION_JSON", selection.c_str()) ||
            !SetEnvironmentVariableW(L"MEB_LOG_DIRECTORY", directory.c_str())) {
            throw std::runtime_error("Cannot set smoke-test environment.");
        }
        const HMODULE module = LoadLibraryW(dll.c_str());
        if (!module) throw std::runtime_error("LoadLibrary failed: " + std::to_string(GetLastError()));
        using WaitFunction = DWORD(WINAPI*)(DWORD);
        const auto wait = reinterpret_cast<WaitFunction>(GetProcAddress(module, "MEB_WaitForInitialization"));
        if (!wait) throw std::runtime_error("Test export missing.");
        // Exercise process-lifetime pinning immediately after LoadLibrary, even with a queued worker.
        FreeLibrary(module);
        const DWORD code = wait(10000);
        if (code != expected) throw std::runtime_error("Unexpected worker status: " + std::to_string(code));
        std::ifstream input(log_path, std::ios::binary);
        const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        if (text.find("MinecraftEldenBridge hook loaded.") == std::string::npos ||
            text.find("no hooks activated") == std::string::npos) {
            throw std::runtime_error("Required startup log missing.");
        }
        if (expected == 0 && (text.find("minecraft_id=minecraft:stone dimension=minecraft:overworld position=(x=128, y=64, z=-32)") == std::string::npos ||
                              text.find("Initialization complete.") == std::string::npos)) {
            throw std::runtime_error("Selection output mismatch.");
        }
        if (expected == 1 && text.find("Selection unavailable:") == std::string::npos) {
            throw std::runtime_error("Missing selection warning not logged.");
        }
        std::cout << "DLL loaded, worker finished with status " << code << ". Log: " << log_path.u8string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
