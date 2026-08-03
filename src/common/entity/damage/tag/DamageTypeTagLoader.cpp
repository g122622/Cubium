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

#include "DamageTypeTagLoader.hpp"
#include "DamageTypeTag.hpp"
#include "DamageTypeTags.hpp"
#include "common/core/Result.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 标签条目的原始数据（未解析引用）
 */
struct DamageTypeRawTagEntry {
    std::string id;       ///< 条目标识符（伤害类型名或 # 标签引用）
    bool required = true; ///< 是否必须存在
};

/**
 * @brief 单个标签文件的原始解析数据（未解析引用）
 */
struct DamageTypeRawTagData {
    bool replace = false;                       ///< 数据包 replace 语义标志
    std::vector<DamageTypeRawTagEntry> entries; ///< 原始条目列表
};

/**
 * @brief 多数据包合并后的标签数据（未解析引用）
 */
struct DamageTypeMergedTagData {
    bool replace = false;                       ///< 合并后的 replace 标志
    std::vector<DamageTypeRawTagEntry> entries; ///< 合并后的条目列表
};

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 解析标签值列表中的单个条目（解析引用阶段）
 *
 * 将 DamageTypeRawTagEntry 解析为实际的 DamageType。
 * 标签引用 (#namespace:path) 会从 DamageTypeTags 注册表中查找已注册的标签并展开其成员。
 */
static void resolveDamageTypeTagEntry(const DamageTypeRawTagEntry& entry,
    std::vector<DamageType>& damageTypes,
    std::unordered_set<ResourceLocation>& visitedTags,
    const ResourceLocation& tagLocation)
{
    if (entry.id.empty()) {
        return;
    }

    if (entry.id[0] == '#') {
        // 标签引用: #namespace:path
        std::string tagRef = entry.id.substr(1);
        ResourceLocation tagRefLocation = ResourceLocation::parse(tagRef);

        // 防止循环引用
        if (visitedTags.count(tagRefLocation) > 0) {
            if (entry.required) {
                spdlog::warn("DamageTypeTagLoader: circular tag reference '{}' (required), skipped (tag: {})",
                    entry.id,
                    tagLocation.toString());
            }
            return;
        }
        visitedTags.insert(tagRefLocation);

        // 查找被引用的标签
        auto* referencedTag = DamageTypeTags::getTag(tagRefLocation);
        if (referencedTag != nullptr) {
            for (DamageType type : referencedTag->getDamageTypes()) {
                damageTypes.push_back(type);
            }
        } else {
            if (entry.required) {
                spdlog::warn("DamageTypeTagLoader: referenced tag '{}' not found (required), skipped (tag: {})",
                    entry.id,
                    tagLocation.toString());
            }
        }
    } else {
        // 直接伤害类型名称
        ResourceLocation typeLocation = ResourceLocation::parse(entry.id);
        auto type = DamageTypeNames::fromResourceLocation(typeLocation);
        if (type.has_value()) {
            damageTypes.push_back(*type);
        } else {
            if (entry.required) {
                spdlog::warn("DamageTypeTagLoader: unknown damage type '{}', skipped (tag: {})",
                    entry.id,
                    tagLocation.toString());
            }
        }
    }
}

/**
 * @brief 解析 JSON 字符串为原始标签数据（第一阶段：不解析引用）
 */
static Result<DamageTypeRawTagData> parseDamageTypeJsonRaw(const std::string& json, const ResourceLocation& location)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);

        DamageTypeRawTagData rawData;

        // 解析 replace 字段（可选，默认 false）
        if (jsonObj.contains("replace") && jsonObj["replace"].is_boolean()) {
            rawData.replace = jsonObj["replace"].get<bool>();
        }

        // 解析 values 数组
        if (!jsonObj.contains("values") || !jsonObj["values"].is_array()) {
            return Error(ErrorCode::InvalidData, "damage type tag missing 'values' array");
        }

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                rawData.entries.push_back({value.get<std::string>(), true});
            } else if (value.is_object()) {
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::warn("DamageTypeTagLoader: object entry in tag '{}' missing 'id' field, skipped",
                        location.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::warn(
                        "DamageTypeTagLoader: object entry 'id' in tag '{}' is empty, skipped", location.toString());
                    continue;
                }

                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                rawData.entries.push_back({id, required});
            } else {
                spdlog::warn(
                    "DamageTypeTagLoader: value in tag '{}' is not a string or object, skipped", location.toString());
            }
        }

        return rawData;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("failed to parse JSON: ") + e.what());
    }
}

/**
 * @brief 将合并后的标签数据解析并填充到 DamageTypeTag
 */
static void resolveAndFillDamageTypeTag(
    const ResourceLocation& location, const DamageTypeMergedTagData& data, DamageTypeTag& tag)
{
    if (data.replace) {
        tag.clear();
    }

    if (data.entries.empty()) {
        return;
    }

    std::vector<DamageType> damageTypes;
    std::unordered_set<ResourceLocation> visitedTags;
    visitedTags.insert(location); // 防止自引用

    for (const auto& entry : data.entries) {
        resolveDamageTypeTagEntry(entry, damageTypes, visitedTags, location);
    }

    tag.addAll(damageTypes);

    if (damageTypes.empty()) {
        spdlog::info("DamageTypeTagLoader: tag '{}' resolved no valid damage types", location.toString());
    }
}

/**
 * @brief 递归解析标签及其依赖（第二阶段：带依赖顺序的解析）
 */
static void resolveDamageTypeTagWithDependencies(const ResourceLocation& location,
    std::unordered_map<ResourceLocation, DamageTypeMergedTagData>& mergedTags,
    std::unordered_set<ResourceLocation>& resolved,
    std::unordered_set<ResourceLocation>& resolving)
{
    // 已经解析过，无需重复处理
    if (resolved.count(location) > 0) {
        return;
    }

    // 不在本次数据包加载范围内
    auto it = mergedTags.find(location);
    if (it == mergedTags.end()) {
        return;
    }

    // 检测循环依赖
    if (resolving.count(location) > 0) {
        spdlog::warn("DamageTypeTagLoader: circular tag dependency detected '{}', skipped", location.toString());
        return;
    }

    resolving.insert(location);

    const auto& data = it->second;

    // 先递归解析所有 # 标签引用的依赖
    for (const auto& entry : data.entries) {
        if (!entry.id.empty() && entry.id[0] == '#') {
            std::string tagRef = entry.id.substr(1);
            ResourceLocation tagRefLocation = ResourceLocation::parse(tagRef);
            resolveDamageTypeTagWithDependencies(tagRefLocation, mergedTags, resolved, resolving);
        }
    }

    // 所有依赖已解析，现在解析当前标签
    auto* tag = DamageTypeTags::getTag(location);
    if (tag != nullptr) {
        resolveAndFillDamageTypeTag(location, data, *tag);
    }

    resolving.erase(location);
    resolved.insert(location);
}

/**
 * @brief 从路径提取标签名称
 *
 * 路径格式: namespace/tags/damage_type/subdir/xxx.json -> namespace:subdir/xxx
 */
static ResourceLocation extractDamageTypeTagLocation(
    const std::string& namespace_, const std::string& directory, const std::string& resourcePath)
{
    std::string tagName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
    // 移除 .json 扩展名
    if (tagName.size() >= 5 && tagName.substr(tagName.size() - 5) == ".json") {
        tagName = tagName.substr(0, tagName.size() - 5);
    }
    return ResourceLocation(tagName);
}

// ============================================================================
// DamageTypeTagLoader 实现
// ============================================================================

Result<size_t> DamageTypeTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    size_t loadedCount = 0;

    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    // ========================================
    // 第一阶段：解析所有 JSON 文件，收集原始条目数据
    // ========================================
    std::unordered_map<ResourceLocation, DamageTypeMergedTagData> mergedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/tags/damage_type";
        auto stacksResult = dataPackList.listResourceStacks(directory, ".json");

        if (!stacksResult.success()) {
            continue;
        }

        for (auto& [resourcePath, versions] : stacksResult.value()) {
            ResourceLocation location = extractDamageTypeTagLocation(namespace_, directory, resourcePath);

            // 遍历同一资源路径在所有数据包中的版本（按优先级从低到高）
            for (auto it = versions.rbegin(); it != versions.rend(); ++it) {
                auto& version = *it;

                auto parseResult = parseDamageTypeJsonRaw(version.content, location);
                if (!parseResult.success()) {
                    spdlog::warn("DamageTypeTagLoader: failed to parse tag {} (from data pack {}): {}",
                        location.toString(),
                        version.packName,
                        parseResult.error().message());
                    continue;
                }

                auto& rawData = parseResult.value();
                auto& existing = mergedTags[location];
                if (rawData.replace) {
                    existing.replace = true;
                    existing.entries.clear();
                    for (const auto& entry : rawData.entries) {
                        existing.entries.push_back(entry);
                    }
                } else {
                    for (const auto& entry : rawData.entries) {
                        existing.entries.push_back(entry);
                    }
                }
            }
        }
    }

    // ========================================
    // 第二阶段：注册空标签，然后按依赖顺序解析引用并填充内容
    // ========================================

    // 2a. 先注册所有尚不存在的标签（空标签）
    for (auto& [location, data] : mergedTags) {
        if (DamageTypeTags::getTag(location) == nullptr) {
            DamageTypeTags::registerTag(location);
        }
    }

    // 2b. 按依赖顺序解析所有标签的引用并填充内容
    std::unordered_set<ResourceLocation> resolved;
    std::unordered_set<ResourceLocation> resolving;

    for (auto& [location, data] : mergedTags) {
        resolveDamageTypeTagWithDependencies(location, mergedTags, resolved, resolving);
        ++loadedCount;
    }

    if (loadedCount > 0) {
        spdlog::info("DamageTypeTagLoader: loaded {} damage type tags from data pack", loadedCount);
    }

    return loadedCount;
}

Result<size_t> DamageTypeTagLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    // ========================================
    // 第一阶段：解析所有 JSON 文件，收集原始条目数据
    // ========================================
    std::unordered_map<ResourceLocation, DamageTypeMergedTagData> mergedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/tags/damage_type";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = extractDamageTypeTagLocation(namespace_, directory, resourcePath);

            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("DamageTypeTagLoader: failed to read tag file: {}", resourcePath);
                continue;
            }

            auto parseResult = parseDamageTypeJsonRaw(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("DamageTypeTagLoader: failed to parse tag {}: {}",
                    location.toString(),
                    parseResult.error().message());
                continue;
            }

            auto& rawData = parseResult.value();
            auto& existing = mergedTags[location];
            if (rawData.replace) {
                existing.replace = true;
                existing.entries.clear();
                for (const auto& entry : rawData.entries) {
                    existing.entries.push_back(entry);
                }
            } else {
                for (const auto& entry : rawData.entries) {
                    existing.entries.push_back(entry);
                }
            }
        }
    }

    // ========================================
    // 第二阶段：注册空标签，然后按依赖顺序解析引用并填充内容
    // ========================================

    for (auto& [location, data] : mergedTags) {
        if (DamageTypeTags::getTag(location) == nullptr) {
            DamageTypeTags::registerTag(location);
        }
    }

    std::unordered_set<ResourceLocation> resolved;
    std::unordered_set<ResourceLocation> resolving;

    for (auto& [location, data] : mergedTags) {
        resolveDamageTypeTagWithDependencies(location, mergedTags, resolved, resolving);
        ++loadedCount;
    }

    return loadedCount;
}

Result<std::unique_ptr<DamageTypeTag>> DamageTypeTagLoader::loadFromJson(
    const std::string& json, const ResourceLocation& location)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);

        // 解析 replace 字段（可选，默认 false）
        bool replace = false;
        if (jsonObj.contains("replace") && jsonObj["replace"].is_boolean()) {
            replace = jsonObj["replace"].get<bool>();
        }
        (void)replace; // 新标签总是空的，replace 在此场景无实际效果

        auto tag = std::make_unique<DamageTypeTag>(location);

        // 解析 values 数组
        if (!jsonObj.contains("values") || !jsonObj["values"].is_array()) {
            return Error(ErrorCode::InvalidData, "damage type tag missing 'values' array");
        }

        std::vector<DamageType> damageTypes;
        std::unordered_set<ResourceLocation> visitedTags;
        visitedTags.insert(location); // 防止自引用

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                std::string entry = value.get<std::string>();
                DamageTypeRawTagEntry rawEntry{entry, true};
                resolveDamageTypeTagEntry(rawEntry, damageTypes, visitedTags, location);
            } else if (value.is_object()) {
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::warn("DamageTypeTagLoader: object entry in tag '{}' missing 'id' field, skipped",
                        location.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::warn(
                        "DamageTypeTagLoader: object entry 'id' in tag '{}' is empty, skipped", location.toString());
                    continue;
                }

                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                DamageTypeRawTagEntry rawEntry{id, required};
                resolveDamageTypeTagEntry(rawEntry, damageTypes, visitedTags, location);
            } else {
                spdlog::warn(
                    "DamageTypeTagLoader: value in tag '{}' is not a string or object, skipped", location.toString());
            }
        }

        tag->addAll(damageTypes);

        if (damageTypes.empty()) {
            spdlog::info("DamageTypeTagLoader: tag '{}' resolved no valid damage types", location.toString());
        }

        return tag;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("failed to parse JSON: ") + e.what());
    }
}

} // namespace mc
