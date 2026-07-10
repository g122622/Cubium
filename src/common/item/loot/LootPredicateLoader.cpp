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
 * The above copyright notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "LootPredicateLoader.hpp"
#include "LootSerializers.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace mc::trace;

namespace mc {
namespace loot {

namespace fs = std::filesystem;

LootPredicateLoader::LootPredicateLoader(LootPredicateManager& manager)
    : m_manager(manager)
{}

void LootPredicateLoader::_clearIfNeeded()
{
    if (m_clearBeforeLoad) {
        m_manager.clear();
    }
}

Result<LootPredicateLoader::LoadResult> LootPredicateLoader::loadFromResourcePacks(
    const PackRepository& packs, ProgressCallback callback)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "LootPredicateLoader::loadFromResourcePacks");

    m_lastResult = LoadResult{};
    _clearIfNeeded();

    auto listResult = packs.listResources(resource::PackType::ServerData, "", ".json");
    if (!listResult.success()) {
        return listResult.error();
    }

    std::vector<std::string> predicateResources;
    for (const auto& path : listResult.value()) {
        if (path.find("/predicates/") == std::string::npos && path.find("predicates/") == std::string::npos &&
            path.find("/predicate/") == std::string::npos && path.find("predicate/") == std::string::npos) {
            continue;
        }
        predicateResources.push_back(path);
    }

    Size current = 0;
    const Size total = predicateResources.size();
    for (const auto& resourcePath : predicateResources) {
        const std::string id = pathToPredicateId(resourcePath);
        if (callback) {
            callback(current, total, id);
        }

        auto readResult = packs.readTextResource(resource::PackType::ServerData, resourcePath);
        if (!readResult.success()) {
            ++m_lastResult.failedCount;
            m_lastResult.errors.push_back(resourcePath + ": " + readResult.error().toString());
            ++current;
            continue;
        }

        auto loadResult = loadJson(id, readResult.value());
        if (loadResult.success()) {
            ++m_lastResult.successCount;
        } else {
            ++m_lastResult.failedCount;
            m_lastResult.errors.push_back(resourcePath + ": " + loadResult.error().toString());
        }
        ++current;
    }

    if (callback) {
        callback(total, total, "");
    }

    return m_lastResult;
}

Result<LootPredicateLoader::LoadResult> LootPredicateLoader::loadFromDataPackRepository(
    const mc::resource::DataPackRepository& dataPacks, ProgressCallback callback)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "LootPredicateLoader::loadFromDataPackRepository");

    m_lastResult = LoadResult{};
    _clearIfNeeded();

    auto listResult = dataPacks.listResources("", ".json");
    if (!listResult.success()) {
        return listResult.error();
    }

    std::vector<std::string> predicateResources;
    for (const auto& path : listResult.value()) {
        if (path.find("/predicates/") == std::string::npos && path.find("predicates/") == std::string::npos &&
            path.find("/predicate/") == std::string::npos && path.find("predicate/") == std::string::npos) {
            continue;
        }
        predicateResources.push_back(path);
    }

    Size current = 0;
    const Size total = predicateResources.size();
    for (const auto& resourcePath : predicateResources) {
        const std::string id = pathToPredicateId(resourcePath);
        if (callback) {
            callback(current, total, id);
        }

        auto readResult = dataPacks.readTextResource(resourcePath);
        if (!readResult.success()) {
            ++m_lastResult.failedCount;
            m_lastResult.errors.push_back(resourcePath + ": " + readResult.error().toString());
            ++current;
            continue;
        }

        auto loadResult = loadJson(id, readResult.value());
        if (loadResult.success()) {
            ++m_lastResult.successCount;
        } else {
            ++m_lastResult.failedCount;
            m_lastResult.errors.push_back(resourcePath + ": " + loadResult.error().toString());
        }
        ++current;
    }

    if (callback) {
        callback(total, total, "");
    }

    return m_lastResult;
}

Result<LootPredicateLoader::LoadResult> LootPredicateLoader::loadFromDirectory(
    const std::string& directoryPath, ProgressCallback callback)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.IO.Resource, "LootPredicateLoader::loadFromDirectory", "directory", directoryPath);

    if (!fs::exists(directoryPath)) {
        return Error(ErrorCode::FileNotFound, "Directory not found: " + directoryPath);
    }

    m_lastResult = LoadResult{};
    _clearIfNeeded();

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

    Size total = jsonFiles.size();
    Size current = 0;

    for (const auto& filePath : jsonFiles) {
        std::string id = pathToPredicateId(filePath.generic_string());

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

Result<std::string> LootPredicateLoader::loadFile(const std::string& filePath)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "LootPredicateLoader::loadFile", "filePath", filePath);

    // 从文件路径推导谓词ID
    std::string id = pathToPredicateId(filePath);

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

Result<std::string> LootPredicateLoader::loadJson(const std::string& id, const std::string& jsonString)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "LootPredicateLoader::loadJson", "id", id);

    // 解析 JSON
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonString);
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::ResourceParseError, "JSON parse error for predicate '" + id + "': " + e.what());
    }

    // 使用 LootSerializers 解析谓词条件
    auto conditionResult = LootSerializers::parseCondition(json);
    if (!conditionResult.success()) {
        return Error(ErrorCode::ResourceParseError,
            "Failed to parse predicate '" + id + "': " + conditionResult.error().toString());
    }

    // 注册到管理器
    m_manager.registerPredicate(id, conditionResult.value());

    return id;
}

std::string LootPredicateLoader::pathToPredicateId(const std::string& filePath) const
{
    fs::path path(filePath);

    std::string namespaceStr = "minecraft";
    std::string relativePath;

    // 将路径转为通用格式（正斜杠）
    std::string genericPath = path.generic_string();

    // 查找 "predicates" 或 "predicate" 目录的位置（MC 1.21+ 使用单数形式）
    Size predicatesPos = genericPath.find("/predicates/");
    Size predicatesTokenSize = std::string("/predicates/").size();
    if (predicatesPos == std::string::npos) {
        predicatesPos = genericPath.find("predicates/");
        predicatesTokenSize = std::string("predicates/").size();
    }
    if (predicatesPos == std::string::npos) {
        predicatesPos = genericPath.find("/predicate/");
        predicatesTokenSize = std::string("/predicate/").size();
    }
    if (predicatesPos == std::string::npos) {
        predicatesPos = genericPath.find("predicate/");
        predicatesTokenSize = std::string("predicate/").size();
    }

    if (predicatesPos != std::string::npos) {
        // 从 predicates 前面提取 namespace
        auto dataPos = genericPath.rfind("/data/", predicatesPos);
        if (dataPos == std::string::npos && genericPath.rfind("data/", 0) == 0) {
            dataPos = 0;
        }
        if (dataPos != std::string::npos) {
            Size namespaceStart = dataPos == 0 ? 5 : dataPos + 6;
            Size namespaceEnd = predicatesPos;
            namespaceStr = genericPath.substr(namespaceStart, namespaceEnd - namespaceStart);
        } else if (predicatesPos > 0 && genericPath[predicatesPos] == '/') {
            namespaceStr = genericPath.substr(0, predicatesPos);
        }

        // 从 predicates/ 后面提取路径（不含 .json 扩展名）
        Size pathStart = predicatesPos + predicatesTokenSize;
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

void LootPredicateLoader::resetResult()
{
    m_lastResult = LoadResult{};
}

} // namespace loot
} // namespace mc
