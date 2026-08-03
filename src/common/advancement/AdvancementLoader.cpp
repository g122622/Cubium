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

#include "AdvancementLoader.hpp"
#include "AdvancementManager.hpp"
#include "common/advancement/Advancement.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::advancement {

Result<AdvancementLoader::LoadResult> AdvancementLoader::loadFromDataPackRepository(
    const mc::resource::DataPackRepository& dataPacks, ProgressCallback callback)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "AdvancementLoader::loadFromDataPackRepository");

    m_lastResult = LoadResult{};
    _clearIfNeeded();

    auto listResult = dataPacks.listResources("", ".json");
    if (!listResult.success()) {
        return listResult.error();
    }

    // 筛选成就资源路径：data/<namespace>/advancement/<path>.json 或 data/<namespace>/advancements/<path>.json
    // MC 1.21+ 使用单数形式 "advancement"，MC 1.16.5 使用复数形式 "advancements"，两种格式都需要支持
    std::vector<std::string> advancementResources;
    for (const auto& path : listResult.value()) {
        if (path.find("/advancement/") == std::string::npos && path.find("advancement/") == std::string::npos &&
            path.find("/advancements/") == std::string::npos && path.find("advancements/") == std::string::npos) {
            continue;
        }
        advancementResources.push_back(path);
    }

    Size current = 0;
    const Size total = advancementResources.size();
    for (const auto& resourcePath : advancementResources) {
        const ResourceLocation id = pathToAdvancementId(resourcePath);
        const std::string idStr = id.toString();
        if (callback) {
            callback(current, total, idStr);
        }

        auto readResult = dataPacks.readTextResource(resourcePath);
        if (!readResult.success()) {
            ++m_lastResult.failedCount;
            m_lastResult.errors.push_back(resourcePath + ": " + readResult.error().toString());
            ++current;
            continue;
        }

        auto loadResult = loadJson(id, readResult.value());
        if (loadResult.failed()) {
            ++m_lastResult.failedCount;
            m_lastResult.errors.push_back(resourcePath + ": " + loadResult.error().toString());
            ++current;
            continue;
        }

        // 注册到管理器
        auto advancement = std::make_shared<Advancement>(std::move(loadResult.value()));
        if (!AdvancementManager::instance().registerAdvancement(advancement)) {
            ++m_lastResult.failedCount;
            m_lastResult.errors.push_back(resourcePath + ": Duplicate advancement ID: " + idStr);
        } else {
            ++m_lastResult.successCount;
        }
        ++current;
    }

    if (callback) {
        callback(total, total, "");
    }

    return m_lastResult;
}

void AdvancementLoader::_clearIfNeeded()
{
    if (m_clearBeforeLoad) {
        AdvancementManager::instance().clear();
    }
}

Result<AdvancementLoader::LoadResult> AdvancementLoader::loadFromDirectory(
    const std::string& directoryPath, ProgressCallback callback)
{

    LoadResult result;

    // 检查目录是否存在
    if (!std::filesystem::exists(directoryPath)) {
        return Error(ErrorCode::FileNotFound, "Directory not found: " + directoryPath);
    }

    // 如果设置，清空管理器
    _clearIfNeeded();

    // 查找所有JSON文件
    auto jsonFiles = _findJsonFiles(directoryPath);
    result.errors.reserve(jsonFiles.size());

    spdlog::info("Loading advancements from: {} ({} files)", directoryPath, jsonFiles.size());

    Size index = 0;
    for (const auto& filePath : jsonFiles) {
        if (callback) {
            callback(index, static_cast<Size>(jsonFiles.size()), filePath.filename().string());
        }
        ++index;

        auto loadResult = loadFile(filePath.string());
        if (loadResult.success()) {
            ++result.successCount;
        } else {
            ++result.failedCount;
            result.errors.push_back(filePath.string() + ": " + loadResult.error().message());
            spdlog::warn("Failed to load advancement {}: {}", filePath.string(), loadResult.error().message());
        }
    }

    m_lastResult = result;
    spdlog::info("Loaded {} advancements, {} failed", result.successCount, result.failedCount);

    return result;
}

Result<ResourceLocation> AdvancementLoader::loadFile(const std::string& filePath)
{
    // 读取文件
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Cannot open file: " + filePath);
    }

    // 解析JSON
    nlohmann::json json;
    try {
        file >> json;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::ResourceParseError, "JSON parse error: " + std::string(e.what()));
    }

    // 推导成就ID
    ResourceLocation id = pathToAdvancementId(filePath);

    // 解析成就
    auto result = loadJson(id, json);
    if (result.failed()) {
        return result.error();
    }

    // 注册到管理器
    auto advancement = std::make_shared<Advancement>(std::move(result.value()));
    if (!AdvancementManager::instance().registerAdvancement(advancement)) {
        return Error(ErrorCode::AlreadyExists, "Advancement already registered: " + id.toString());
    }

    return id;
}

Result<Advancement> AdvancementLoader::loadJson(const ResourceLocation& id, const std::string& jsonString)
{
    // 解析JSON
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonString);
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::ResourceParseError, "JSON parse error: " + std::string(e.what()));
    }

    return loadJson(id, json);
}

Result<Advancement> AdvancementLoader::loadJson(const ResourceLocation& id, const nlohmann::json& json)
{
    return Advancement::fromJson(id, json);
}

ResourceLocation AdvancementLoader::pathToAdvancementId(const std::string& filePath) const
{
    // 路径格式支持两种：
    // 1. 旧格式（含 data/ 前缀）："data/minecraft/advancements/story/mine_stone.json"
    // 2. 新格式（相对于 data/ 根）："minecraft/advancements/story/mine_stone.json"
    // ID格式: "minecraft:story/mine_stone"

    std::filesystem::path path(filePath);

    std::string namespaceName;
    std::string advancementPath;

    auto it = path.begin();
    bool foundAdvancements = false;
    bool foundDataPrefix = false;

    // 第一步：提取命名空间
    // 扫描查找 "data" 段（支持路径带任意前缀，如临时目录前缀）：
    //   "<prefix>/data/minecraft/advancements/story/mine_stone.json" → namespace=minecraft
    //   "data/minecraft/advancements/story/mine_stone.json"           → namespace=minecraft
    // 若路径中无 "data" 段（DataPackRepository::listResources 返回相对 data/ 根的路径），
    // 则首段直接作为命名空间：
    //   "minecraft/advancements/story/mine_stone.json"                 → namespace=minecraft
    for (auto scan = path.begin(); scan != path.end(); ++scan) {
        if (scan->string() == "data") {
            foundDataPrefix = true;
            it = scan;
            ++it; // 跳过 "data"
            if (it != path.end()) {
                namespaceName = it->string();
                ++it; // 跳过命名空间段
            }
            break;
        }
    }
    if (!foundDataPrefix) {
        // 无 "data" 前缀：首段即命名空间
        it = path.begin();
        if (it != path.end()) {
            namespaceName = it->string();
            ++it;
        }
    }

    // 第二步：查找 "advancements" 或 "advancement" 目录（MC 1.21+ 使用单数，MC 1.16.5 使用复数）
    for (; it != path.end(); ++it) {
        const std::string segment = it->string();
        if (segment == "advancements" || segment == "advancement") {
            foundAdvancements = true;
            ++it;
            break;
        }
    }

    // 收集剩余路径作为成就路径
    std::vector<std::string> pathParts;
    for (; it != path.end(); ++it) {
        std::string part = it->string();
        // 移除 .json 扩展名
        if (part.ends_with(".json")) {
            part = part.substr(0, part.length() - 5);
        }
        if (!part.empty()) {
            pathParts.push_back(part);
        }
    }

    if (!foundAdvancements || namespaceName.empty()) {
        // 如果找不到标准路径结构，使用文件名作为ID
        std::string filename = path.stem().string();
        return ResourceLocation(filename);
    }

    // 构建成就ID
    std::string advancementId;
    for (size_t i = 0; i < pathParts.size(); ++i) {
        if (i > 0) advancementId += "/";
        advancementId += pathParts[i];
    }

    return ResourceLocation(namespaceName, advancementId);
}

std::vector<std::filesystem::path> AdvancementLoader::_findJsonFiles(const std::filesystem::path& directory) const
{
    std::vector<std::filesystem::path> files;

    if (!std::filesystem::exists(directory)) {
        return files;
    }

    // 递归遍历目录
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }

    // 排序以保证加载顺序一致
    std::sort(files.begin(), files.end());

    return files;
}

} // namespace mc::advancement
