/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SaveFormat.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/CompressionUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc::world::storage {

Result<SaveFormatInfo> SaveFormatDetector::detect(const std::filesystem::path& worldDir)
{
    std::error_code ec;

    // 目录必须存在
    if (!std::filesystem::exists(worldDir, ec)) {
        return Error(ErrorCode::WorldNotFound, fmt::format("World directory does not exist: {}", worldDir.string()));
    }

    // 优先级 1：检查是否存在 region/ 目录及 .mca 文件 → Java Anvil
    if (_hasAnvilRegion(worldDir)) {
        spdlog::info("SaveFormatDetector: Detected Java Anvil format at {}", worldDir.string());
        auto versionResult = _detectJavaVersion(worldDir);
        if (versionResult.success()) {
            return versionResult;
        }
        // 即使无法读取版本信息，也返回 Java Anvil 格式
        SaveFormatInfo info;
        info.format = SaveFormat::JavaAnvil;
        info.formatName = "Java Anvil";
        info.dataVersion = 0;
        info.readonly = true;
        return info;
    }

    // 检查 db/ 目录
    std::filesystem::path dbDir = worldDir / "db";

    if (std::filesystem::exists(dbDir, ec)) {
        // 优先级 2：检查是否包含 Bedrock LevelDB 特征文件
        if (_hasBedrockDb(dbDir)) {
            spdlog::info("SaveFormatDetector: Detected Bedrock LevelDB format at {}", worldDir.string());
            auto versionResult = _detectBedrockVersion(worldDir);
            if (versionResult.success()) {
                return versionResult;
            }
            SaveFormatInfo info;
            info.format = SaveFormat::BedrockLDB;
            info.formatName = "Bedrock LevelDB";
            info.dataVersion = 0;
            info.readonly = true;
            return info;
        }

        // 优先级 3：检查是否包含 RocksDB 特征文件 → Native
        if (_hasRocksDb(dbDir)) {
            spdlog::info("SaveFormatDetector: Detected Native RocksDB format at {}", worldDir.string());
            SaveFormatInfo info;
            info.format = SaveFormat::Native;
            info.formatName = "Native";
            info.dataVersion = 1;
            info.readonly = false;
            return info;
        }
    }

    // 无法从目录结构判断，尝试读取 level.dat
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    if (std::filesystem::exists(levelDatPath, ec)) {
        // 尝试读取为 Java 格式（gzip + 大端序 NBT）
        std::ifstream file(levelDatPath, std::ios::binary);
        if (file.is_open()) {
            std::vector<u8> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            if (!compressed.empty()) {
                // Java 版 level.dat 以 gzip 压缩开头（0x1F 0x8B）
                if (compressed.size() >= 2 && compressed[0] == 0x1F && compressed[1] == 0x8B) {
                    auto decompressed = mc::util::decompressGzip(compressed);
                    if (!decompressed.empty()) {
                        std::istringstream stream(std::string(decompressed.begin(), decompressed.end()));
                        stream >> mc::nbt::contexts::java;
                        auto root = mc::nbt::tags::compound_tag::read(stream);
                        if (root) {
                            // Java 版 level.dat 成功解析
                            auto versionResult = _detectJavaVersion(worldDir);
                            if (versionResult.success()) {
                                return versionResult;
                            }
                            SaveFormatInfo info;
                            info.format = SaveFormat::JavaAnvil;
                            info.formatName = "Java Anvil";
                            info.readonly = true;
                            return info;
                        }
                    }
                }

                // 基岩版 level.dat 有 8 字节头（4字节版本 + 4字节长度）+ 小端序 NBT
                if (compressed.size() >= 8) {
                    // 检查是否能按 Bedrock 格式解析
                    std::istringstream stream(std::string(compressed.begin() + 8, compressed.end()));
                    stream >> mc::nbt::contexts::bedrock_disk;
                    auto root = mc::nbt::tags::compound_tag::read(stream);
                    if (root) {
                        auto versionResult = _detectBedrockVersion(worldDir);
                        if (versionResult.success()) {
                            return versionResult;
                        }
                        SaveFormatInfo info;
                        info.format = SaveFormat::BedrockLDB;
                        info.formatName = "Bedrock LevelDB";
                        info.readonly = true;
                        return info;
                    }
                }
            }
        }
    }

    // 默认假定为 Native 格式（可能是全新的存档目录）
    spdlog::info("SaveFormatDetector: Defaulting to Native format for {}", worldDir.string());
    SaveFormatInfo info;
    info.format = SaveFormat::Native;
    info.formatName = "Native";
    info.dataVersion = 0;
    info.readonly = false;
    return info;
}

bool SaveFormatDetector::_hasRocksDb(const std::filesystem::path& dbDir)
{
    std::error_code ec;

    // RocksDB 特征文件：OPTIONS- 开头的文件、CURRENT 文件、.sst 文件
    bool hasCurrent = std::filesystem::exists(dbDir / "CURRENT", ec);
    bool hasOptions = false;

    for (const auto& entry : std::filesystem::directory_iterator(dbDir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::string name = entry.path().filename().string();
        if (name.starts_with("OPTIONS-")) {
            hasOptions = true;
        }
    }

    // RocksDB 至少有 CURRENT 文件，且通常有 OPTIONS- 文件
    return hasCurrent && hasOptions;
}

bool SaveFormatDetector::_hasAnvilRegion(const std::filesystem::path& worldDir)
{
    std::error_code ec;

    // Java 版存档有 region/ 目录，包含 .mca 文件
    std::filesystem::path regionDir = worldDir / "region";
    if (!std::filesystem::exists(regionDir, ec)) {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(regionDir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mca") {
            return true;
        }
    }

    return false;
}

bool SaveFormatDetector::_hasBedrockDb(const std::filesystem::path& dbDir)
{
    std::error_code ec;

    // LevelDB 特征文件：.ldb 文件，且没有 OPTIONS- 开头的文件
    bool hasLdb = false;
    bool hasOptions = false;

    for (const auto& entry : std::filesystem::directory_iterator(dbDir, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::string name = entry.path().filename().string();
        if (name.ends_with(".ldb") || name.ends_with(".log")) {
            hasLdb = true;
        }
        if (name.starts_with("OPTIONS-")) {
            hasOptions = true;
        }
    }

    // LevelDB 有 .ldb 或 .log 文件，且没有 RocksDB 特有的 OPTIONS- 文件
    // LevelDB 的 MANIFEST 文件没有 OPTIONS- 前缀
    return hasLdb && !hasOptions;
}

Result<SaveFormatInfo> SaveFormatDetector::_detectJavaVersion(const std::filesystem::path& worldDir)
{
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    std::error_code ec;

    if (!std::filesystem::exists(levelDatPath, ec)) {
        return Error(ErrorCode::FileNotFound, "level.dat not found");
    }

    try {
        std::ifstream file(levelDatPath, std::ios::binary);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed, "Cannot open level.dat");
        }

        std::vector<u8> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        if (compressed.empty()) {
            return Error(ErrorCode::FileCorrupted, "level.dat is empty");
        }

        auto decompressed = mc::util::decompressGzip(compressed);
        if (decompressed.empty()) {
            return Error(ErrorCode::DecompressionFailed, "Failed to decompress level.dat");
        }

        std::istringstream stream(std::string(decompressed.begin(), decompressed.end()));
        stream >> mc::nbt::contexts::java;
        auto root = mc::nbt::tags::compound_tag::read(stream);
        if (!root) {
            return Error(ErrorCode::FileCorrupted, "Failed to parse level.dat NBT");
        }

        SaveFormatInfo info;
        info.format = SaveFormat::JavaAnvil;
        info.readonly = true;

        // 读取 DataVersion
        if (root->value.count("Data") != 0) {
            auto* data = dynamic_cast<mc::nbt::tags::compound_tag*>(root->value.at("Data").get());
            if (data) {
                if (data->value.count("DataVersion") != 0) {
                    info.dataVersion = static_cast<i32>(data->get<mc::nbt::tags::int_tag>("DataVersion"));
                }
                // 读取版本名称
                if (data->value.count("Version") != 0) {
                    auto* version = dynamic_cast<mc::nbt::tags::compound_tag*>(data->value.at("Version").get());
                    if (version && version->value.count("Name") != 0) {
                        info.formatName = "Java " + version->get<mc::nbt::tags::string_tag>("Name");
                    }
                }
            }
        }

        if (info.formatName.empty()) {
            info.formatName = "Java Anvil";
        }

        spdlog::info("SaveFormatDetector: Java version: {} (DataVersion: {})", info.formatName, info.dataVersion);
        return info;
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Failed to read Java level.dat: {}", e.what()));
    }
}

Result<SaveFormatInfo> SaveFormatDetector::_detectBedrockVersion(const std::filesystem::path& worldDir)
{
    std::filesystem::path levelDatPath = worldDir / "level.dat";
    std::error_code ec;

    if (!std::filesystem::exists(levelDatPath, ec)) {
        return Error(ErrorCode::FileNotFound, "level.dat not found");
    }

    try {
        std::ifstream file(levelDatPath, std::ios::binary);
        if (!file.is_open()) {
            return Error(ErrorCode::FileOpenFailed, "Cannot open level.dat");
        }

        // 基岩版 level.dat 有 8 字节头
        u8 header[8] = {};
        if (!file.read(reinterpret_cast<char*>(header), 8)) {
            return Error(ErrorCode::FileCorrupted, "Failed to read Bedrock level.dat header");
        }

        // 跳过 8 字节头后读取 NBT
        std::vector<u8> nbtData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        std::istringstream stream(std::string(nbtData.begin(), nbtData.end()));
        stream >> mc::nbt::contexts::bedrock_disk;
        auto root = mc::nbt::tags::compound_tag::read(stream);
        if (!root) {
            return Error(ErrorCode::FileCorrupted, "Failed to parse Bedrock level.dat NBT");
        }

        SaveFormatInfo info;
        info.format = SaveFormat::BedrockLDB;
        info.readonly = true;

        // 读取版本信息
        if (root->value.count("lastOpenedWithVersion") != 0) {
            // lastOpenedWithVersion 是 IntList，格式 [major, minor, patch, revision, ...]
            auto* versionList = dynamic_cast<mc::nbt::tags::int_list_tag*>(root->value["lastOpenedWithVersion"].get());
            if (versionList && versionList->size() >= 2) {
                i32 major = 0;
                i32 minor = 0;
                i32 patch = 0;
                const auto& values = versionList->value;
                if (values.size() >= 1) {
                    major = static_cast<i32>(values[0]);
                }
                if (values.size() >= 2) {
                    minor = static_cast<i32>(values[1]);
                }
                if (values.size() >= 3) {
                    patch = static_cast<i32>(values[2]);
                }
                info.dataVersion = major; // 用协议版本作为 dataVersion
                info.formatName = fmt::format("Bedrock {}.{}.{}", major, minor, patch);
            }
        }

        if (info.formatName.empty()) {
            info.formatName = "Bedrock LevelDB";
        }

        spdlog::info("SaveFormatDetector: Bedrock version: {} (protocol: {})", info.formatName, info.dataVersion);
        return info;
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::FileCorrupted, fmt::format("Failed to read Bedrock level.dat: {}", e.what()));
    }
}

} // namespace mc::world::storage
