#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace meb {
inline constexpr std::size_t kMaxSelectionBytes = 64 * 1024;
struct Position { double x; double y; double z; };
struct Selection {
    std::string minecraft_id;
    std::string dimension;
    Position position;
};
struct SelectionResult {
    std::optional<Selection> selection;
    std::string error;
};
SelectionResult ParseSelection(std::string_view json_text);
SelectionResult ReadSelection(const std::filesystem::path& path);
}
