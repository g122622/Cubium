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

#include "LootTableLoader.hpp"
#include "LootTable.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace loot {

namespace fs = std::filesystem;

LootTableLoader::LootTableLoader(LootTableManager& manager)
    : m_manager(manager)
{}

Result<LootTableLoader::LoadResult> LootTableLoader::loadFromDirectory(
    const std::string& directoryPath, ProgressCallback callback)
{
    if (!fs::exists(directoryPath)) {
        return Error(ErrorCode::FileNotFound, "Directory not found: " + directoryPath);
    }

    if (m_clearBeforeLoad) {
        m_manager.clear();
    }

    m_lastResult = LoadResult{};

    // 收集所有 .json 文件
    std::vector<fs::path> jsonFiles;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                jsonFiles.push_back(entry.path());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        return Error(ErrorCode::FileOpenFailed, "Failed to iterate directory: " + std::string(e.what()));
    }

    size_t total = jsonFiles.size();
    size_t current = 0;

    for (const auto& filePath : jsonFiles) {
        std::string id = pathToLootTableId(filePath.generic_string());

        if (callback) {
            callback(current, total, id);
        }

        auto result = loadFile(filePath.generic_string());
        if (result.success()) {
            m_lastResult.successCount++;
        } else {
            m_lastResult.failedCount++;
            m_lastResult.errors.push_back(filePath.filename().string() + ": " + result.error().toString());
        }
        current++;
    }

    if (callback) {
        callback(total, total, "");
    }

    return m_lastResult;
}

Result<std::string> LootTableLoader::loadFile(const std::string& filePath)
{
    // 从文件路径推导掉落表ID
    std::string id = pathToLootTableId(filePath);

    // 读取文件内容
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Failed to open file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonString = buffer.str();

    return loadJson(id, jsonString);
}

Result<std::string> LootTableLoader::loadJson(const std::string& id, const std::string& jsonString)
{
    // 解析 JSON
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonString);
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::ResourceParseError, "JSON parse error for loot table '" + id + "': " + e.what());
    }

    // 使用 LootSerializers 解析掉落表
    auto tableResult = LootTable::fromJson(jsonString);
    if (!tableResult.success()) {
        return Error(ErrorCode::ResourceParseError,
            "Failed to parse loot table '" + id + "': " + tableResult.error().toString());
    }

    // 注册到管理器
    auto table = tableResult.value();
    m_manager.registerTable(id, std::move(table));

    return id;
}

std::string LootTableLoader::pathToLootTableId(const std::string& filePath) const
{
    fs::path path(filePath);

    // 从文件路径中提取相对路径部分：
    // 查找 "loot_tables" 目录，然后向上找到 namespace
    // 格式：data/<namespace>/loot_tables/<category>/<name>.json
    std::string namespaceStr = "minecraft";
    std::string relativePath;

    // 将路径转为通用格式（正斜杠）
    std::string genericPath = path.generic_string();

    // 查找 "loot_tables" 的位置
    auto lootTablesPos = genericPath.find("/loot_tables/");
    if (lootTablesPos != std::string::npos) {
        // 从 loot_tables 前面提取 namespace
        auto dataPos = genericPath.rfind("/data/", lootTablesPos);
        if (dataPos != std::string::npos) {
            // data/<namespace>/loot_tables/...
            size_t namespaceStart = dataPos + 6; // 跳过 "/data/"
            size_t namespaceEnd = lootTablesPos;
            namespaceStr = genericPath.substr(namespaceStart, namespaceEnd - namespaceStart);
        }

        // 从 loot_tables/ 后面提取路径（不含 .json 扩展名）
        size_t pathStart = lootTablesPos + 14; // 跳过 "/loot_tables/"
        relativePath = genericPath.substr(pathStart);

        // 去掉 .json 扩展名
        const std::string ext = ".json";
        if (relativePath.size() > ext.size() && relativePath.substr(relativePath.size() - ext.size()) == ext) {
            relativePath = relativePath.substr(0, relativePath.size() - ext.size());
        }
    } else {
        // 没有找到标准路径结构，使用文件名（去掉 .json）作为路径
        relativePath = path.stem().string();
    }

    // 组合为 ResourceLocation 格式的字符串
    ResourceLocation location(namespaceStr, relativePath);
    return location.toString();
}

void LootTableLoader::resetResult()
{
    m_lastResult = LoadResult{};
}

} // namespace loot
} // namespace mc
