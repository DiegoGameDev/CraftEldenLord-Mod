#include "selection_reader.h"

#include <nlohmann/json.hpp>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <vector>

namespace meb {
namespace {
using Json = nlohmann::json;

bool ResourceId(const std::string& value) {
    if (value.empty() || value.size() > 256) return false;
    const auto colon = value.find(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 == value.size()) return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == colon) continue;
        const char c = value[i];
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || (i > colon && c == '/')) continue;
        return false;
    }
    return true;
}

void ExactKeys(const Json& object, std::initializer_list<const char*> keys) {
    if (!object.is_object() || object.size() != keys.size()) {
        throw std::runtime_error("Object has missing or unknown fields.");
    }
    for (const auto* key : keys) {
        if (!object.contains(key)) throw std::runtime_error(std::string("Missing field: ") + key);
    }
}
}

SelectionResult ParseSelection(std::string_view json_text) {
    try {
        if (json_text.empty() || json_text.size() > kMaxSelectionBytes) {
            throw std::runtime_error("Selection must contain 1..65536 bytes.");
        }
        // Bound nesting and reject duplicate keys before accepting a parsed document.
        std::vector<std::set<std::string>> object_keys;
        const auto callback = [&object_keys](int depth, Json::parse_event_t event, Json& value) {
            if (depth > 32) throw std::runtime_error("JSON nesting exceeds 32 levels.");
            if (event == Json::parse_event_t::object_start) object_keys.emplace_back();
            if (event == Json::parse_event_t::key &&
                !object_keys.back().insert(value.get<std::string>()).second) {
                throw std::runtime_error("Duplicate JSON field.");
            }
            if (event == Json::parse_event_t::object_end) object_keys.pop_back();
            return true;
        };
        const Json root = Json::parse(json_text.begin(), json_text.end(), callback);
        ExactKeys(root, {"schema_version", "minecraft_id", "dimension", "position"});
        if (!root.at("schema_version").is_number() || root.at("schema_version") != 1) {
            throw std::runtime_error("Unsupported schema_version; expected 1.");
        }
        Selection result;
        result.minecraft_id = root.at("minecraft_id").get<std::string>();
        result.dimension = root.at("dimension").get<std::string>();
        if (!ResourceId(result.minecraft_id) || !ResourceId(result.dimension)) {
            throw std::runtime_error("minecraft_id and dimension must be namespaced resource IDs.");
        }
        const auto& position = root.at("position");
        ExactKeys(position, {"x", "y", "z"});
        const auto coordinate = [&position](const char* key) {
            if (!position.at(key).is_number()) throw std::runtime_error("Position must be numeric.");
            const double value = position.at(key).get<double>();
            if (!std::isfinite(value) || std::abs(value) > 1.0e9) {
                throw std::runtime_error("Position must be finite and within +/-1e9.");
            }
            return value;
        };
        result.position = {coordinate("x"), coordinate("y"), coordinate("z")};
        return {std::move(result), {}};
    } catch (const std::exception& error) {
        return {std::nullopt, error.what()};
    }
}

SelectionResult ReadSelection(const std::filesystem::path& path) {
    try {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error)) {
            return {std::nullopt, "Selection is missing, inaccessible or not a regular file."};
        }
        std::ifstream input(path, std::ios::binary);
        if (!input) return {std::nullopt, "Could not open selection file."};
        // Read one extra byte to detect oversized files even if a writer changes the size.
        std::string bytes(kMaxSelectionBytes + 1, '\0');
        input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (input.bad() || (input.fail() && !input.eof())) {
            return {std::nullopt, "Selection file read failed."};
        }
        bytes.resize(static_cast<std::size_t>(input.gcount()));
        return ParseSelection(bytes);
    } catch (const std::exception& error) {
        return {std::nullopt, error.what()};
    }
}
}
