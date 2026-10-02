#include "logger.h"

#include <Windows.h>
#include <iomanip>
#include <sstream>
#include <string>

namespace meb {
namespace {
std::string Escape(std::string_view text) {
    std::ostringstream out;
    for (const unsigned char ch : text) {
        if (ch < 32 || ch == 127) {
            out << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<unsigned int>(ch);
        } else {
            out << static_cast<char>(ch);
        }
    }
    return out.str();
}
}

bool Logger::Open(const std::filesystem::path& primary,
                  const std::filesystem::path& fallback) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) return false;
        bool using_fallback = false;
        for (const auto& path : {primary, fallback}) {
            if (!path.empty()) {
                std::error_code error;
                if (!path.parent_path().empty()) {
                    std::filesystem::create_directories(path.parent_path(), error);
                }
                file_.clear();
                file_.open(path, std::ios::out | std::ios::app | std::ios::binary);
                if (file_.is_open()) {
                    if (using_fallback) {
                        file_ << "[WARN] Primary log unavailable; using fallback.\n";
                    }
                    file_ << "[INFO] Log file: " << Escape(path.u8string()) << '\n';
                    file_.flush();
                    if (file_.good()) return true;
                    file_.close();
                }
            }
            using_fallback = true;
        }
    } catch (...) {
        // Diagnostics must never propagate exceptions into the game.
    }
    OutputDebugStringA("MinecraftEldenBridge: unable to open log file.\n");
    return false;
}

bool Logger::Write(std::string_view level, std::string_view message) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        SYSTEMTIME now{};
        GetSystemTime(&now);
        std::ostringstream line;
        line << std::setfill('0') << std::setw(4) << now.wYear << '-'
             << std::setw(2) << now.wMonth << '-' << std::setw(2) << now.wDay << 'T'
             << std::setw(2) << now.wHour << ':' << std::setw(2) << now.wMinute << ':'
             << std::setw(2) << now.wSecond << '.' << std::setw(3) << now.wMilliseconds
             << "Z [" << level << "] [pid=" << GetCurrentProcessId()
             << " tid=" << GetCurrentThreadId() << "] " << Escape(message) << '\n';
        const auto text = line.str();
        OutputDebugStringA(text.c_str());
        if (!file_.is_open()) return false;
        file_ << text;
        file_.flush();
        return file_.good();
    } catch (...) {
        OutputDebugStringA("MinecraftEldenBridge: log write failed.\n");
        return false;
    }
}

bool Logger::Info(std::string_view message) noexcept { return Write("INFO", message); }
bool Logger::Warn(std::string_view message) noexcept { return Write("WARN", message); }
bool Logger::Error(std::string_view message) noexcept { return Write("ERROR", message); }
}
