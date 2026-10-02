#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

namespace meb {
class Logger final {
public:
    bool Open(const std::filesystem::path& primary,
              const std::filesystem::path& fallback) noexcept;
    bool Info(std::string_view message) noexcept;
    bool Warn(std::string_view message) noexcept;
    bool Error(std::string_view message) noexcept;

private:
    bool Write(std::string_view level, std::string_view message) noexcept;
    std::mutex mutex_;
    std::ofstream file_;
};
}
