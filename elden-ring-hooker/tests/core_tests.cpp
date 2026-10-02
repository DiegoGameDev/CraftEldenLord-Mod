#include "logger.h"
#include "selection_reader.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
const std::string valid = R"({"schema_version":1,"minecraft_id":"minecraft:stone","dimension":"minecraft:overworld","position":{"x":128,"y":64,"z":-32}})";
void Write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << text;
    file.flush();
    Check(file.good(), "Fixture write failed.");
}
std::string Read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    Check(file.good(), "Fixture read failed.");
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
void Reject(const std::string& value) {
    const auto result = meb::ParseSelection(value);
    Check(!result.selection && !result.error.empty(), "Invalid JSON was accepted.");
}
}

int wmain(int argc, wchar_t** argv) {
    try {
        Check(argc == 2, "Usage: bridge_core_tests OUTPUT_DIR");
        const std::filesystem::path directory(argv[1]);
        std::filesystem::create_directories(directory);
        const auto result = meb::ParseSelection(valid);
        Check(result.selection && result.selection->minecraft_id == "minecraft:stone" &&
              result.selection->dimension == "minecraft:overworld" &&
              result.selection->position.x == 128 && result.selection->position.y == 64 &&
              result.selection->position.z == -32, "Valid selection mismatch.");
        const auto escaped = meb::ParseSelection(R"({"position":{"z":-3.2e1,"x":1.28e2,"y":64.0},"dimension":"minecraft:overworld","minecraft_id":"minecraft:\u0073tone","schema_version":1})");
        Check(escaped.selection && escaped.selection->minecraft_id == "minecraft:stone" &&
              escaped.selection->position.z == -32, "Escapes or exponents failed.");
        Reject("");
        Reject("{");
        Reject(valid + " trailing");
        Reject("[]");
        Reject("{\"wrapper\":" + valid + "}");
        Reject(valid.substr(0, valid.size() - 1) + ",\"schema_version\":1}");
        Reject(valid.substr(0, valid.size() - 1) + ",\"unknown\":true}");
        for (const std::string replacement : {"null", "true", "\"64\"", "1e100", "1e9999"}) {
            auto invalid = valid;
            invalid.replace(invalid.find("64"), 2, replacement);
            Reject(invalid);
        }
        auto wrong_version = valid;
        wrong_version.replace(wrong_version.find(":1,"), 3, ":2,");
        Reject(wrong_version);
        auto bad_id = valid;
        bad_id.replace(bad_id.find("minecraft:stone"), 15, "Minecraft:stone");
        Reject(bad_id);
        Reject(std::string(40, '[') + "0" + std::string(40, ']'));
        Reject(std::string(meb::kMaxSelectionBytes + 1, ' '));
        const auto fixture = directory / L"selection-\u00e7.json";
        Write(fixture, valid);
        Check(meb::ReadSelection(fixture).selection.has_value(), "Unicode path read failed.");
        Write(fixture, std::string(meb::kMaxSelectionBytes + 1, ' '));
        Check(!meb::ReadSelection(fixture).selection, "Oversized file accepted.");
        Check(!meb::ReadSelection(directory / L"nonexistent.json").selection, "Missing file accepted.");
        Check(!meb::ReadSelection(directory).selection, "Directory accepted.");

        const auto log_path = directory / L"concurrent.log";
        Write(log_path, "existing-line\n");
        meb::Logger logger;
        Check(logger.Open(log_path, {}), "Cannot open logger.");
        std::atomic<bool> writes_ok{true};
        std::vector<std::thread> threads;
        for (int worker = 0; worker < 4; ++worker) {
            threads.emplace_back([&logger, &writes_ok, worker] {
                for (int entry = 0; entry < 100; ++entry) {
                    if (!logger.Info("record=" + std::to_string(worker) + ":" + std::to_string(entry))) {
                        writes_ok = false;
                    }
                }
            });
        }
        for (auto& thread : threads) thread.join();
        Check(writes_ok, "Concurrent log write failed.");
        Check(logger.Info("line\nforged\r\t"), "Escaped log write failed.");
        const auto content = Read(log_path);
        Check(content.find("existing-line\n") == 0, "Logger did not append.");
        Check(content.find("line\\x0aforged\\x0d\\x09") != std::string::npos, "Log control escaping failed.");
        for (int worker = 0; worker < 4; ++worker) {
            for (int entry = 0; entry < 100; ++entry) {
                const auto record = "record=" + std::to_string(worker) + ":" + std::to_string(entry) + '\n';
                const auto index = content.find(record);
                Check(index != std::string::npos && content.find(record, index + 1) == std::string::npos,
                      "Log record missing, duplicated or interleaved.");
            }
        }
        meb::Logger fallback;
        const auto fallback_path = directory / L"fallback.log";
        Write(fallback_path, "");
        Check(fallback.Open(directory / L"concurrent.log" / L"impossible.log", fallback_path),
              "Logger fallback failed.");
        Check(fallback.Info("fallback works"), "Fallback write failed.");
        Check(Read(fallback_path).find("using fallback") != std::string::npos, "Fallback warning missing.");
        meb::Logger failed;
        Check(!failed.Open(directory, directory), "Invalid log destination accepted.");
        Check(!failed.Error("closed logger"), "Closed logger reported success.");
        std::cout << "Reader validation, bounded file input, Unicode paths, 400 concurrent log records and fallback passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
