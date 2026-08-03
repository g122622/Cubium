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

#include "ItemTagLoader.hpp"
#include "ItemTags.hpp"
#include "common/core/Result.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/tag/ItemTag.hpp"
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

namespace mc::item::tag {

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 标签条目的原始数据（未解析引用）
 *
 * 在两阶段加载中，第一阶段仅解析 JSON 结构并收集原始条目，
 * 第二阶段再解析 # 标签引用并填充物品。
 * 这样可以确保标签引用在被解析时，被引用的标签已经注册到 ItemTags 中。
 */
struct RawTagEntry {
    std::string id;       ///< 条目标识符（物品名或 # 标签引用）
    bool required = true; ///< 是否必须存在
};

/**
 * @brief 单个标签文件的原始解析数据（未解析引用）
 */
struct RawTagData {
    bool replace = false;             ///< 数据包 replace 语义标志
    std::vector<RawTagEntry> entries; ///< 原始条目列表
};

/**
 * @brief 多数据包合并后的标签数据（未解析引用）
 */
struct MergedTagData {
    bool replace = false;             ///< 合并后的 replace 标志
    std::vector<RawTagEntry> entries; ///< 合并后的条目列表
};

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 根据 ResourceLocation 解析物品指针
 */
static const Item* resolveItem(const ResourceLocation& itemLocation)
{
    return ItemRegistry::instance().getItem(itemLocation);
}

/**
 * @brief 解析物品名称为 Item 指针
 */
static const Item* resolveItemByName(const std::string& itemName)
{
    ResourceLocation location = ResourceLocation::parse(itemName);
    return resolveItem(location);
}

/**
 * @brief 解析标签值列表中的单个条目（解析引用阶段）
 *
 * 将 RawTagEntry 解析为实际的 Item 指针。
 * 标签引用 (#namespace:path) 会从 ItemTags 注册表中查找已注册的标签并展开其物品。
 *
 * @param entry 原始条目
 * @param items 输出参数：收集的 Item 指针
 * @param visitedTags 已访问的标签集合（防止循环引用）
 * @param tagLocation 当前正在解析的标签位置（用于日志输出）
 */
static void resolveTagEntry(const RawTagEntry& entry,
    std::vector<const Item*>& items,
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
                spdlog::warn("ItemTagLoader: circular tag reference '{}' (required), skipping (tag: {})",
                    entry.id,
                    tagLocation.toString());
            }
            return;
        }
        visitedTags.insert(tagRefLocation);

        // 查找被引用的标签
        auto* referencedTag = ItemTags::getTag(tagRefLocation);
        if (referencedTag != nullptr) {
            for (const auto* item : referencedTag->getItems()) {
                items.push_back(item);
            }
        } else {
            if (entry.required) {
                spdlog::warn("ItemTagLoader: referenced tag '{}' not found (required), skipping (tag: {})",
                    entry.id,
                    tagLocation.toString());
            }
            // required=false 时静默跳过
        }
    } else {
        // 直接物品名称
        const Item* item = resolveItemByName(entry.id);
        if (item != nullptr) {
            items.push_back(item);
        } else {
            if (entry.required) {
                spdlog::warn("ItemTagLoader: unknown item '{}' (required), skipping (tag: {})",
                    entry.id,
                    tagLocation.toString());
            }
            // required=false 时静默跳过
        }
    }
}

/**
 * @brief 解析 JSON 字符串为原始标签数据（第一阶段：不解析引用）
 *
 * 仅解析 JSON 结构，收集原始条目（包括 # 标签引用），
 * 不尝试解析引用目标。这在两阶段加载的第一阶段使用，
 * 确保所有标签都已注册后再解析引用。
 *
 * @param json JSON 内容
 * @param location 标签资源位置
 * @return 原始标签数据，或错误信息
 */
static Result<RawTagData> parseJsonRaw(const std::string& json, const ResourceLocation& location)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);

        RawTagData rawData;

        // 解析 replace 字段（可选，默认 false）
        if (jsonObj.contains("replace") && jsonObj["replace"].is_boolean()) {
            rawData.replace = jsonObj["replace"].get<bool>();
        }

        // 解析 values 数组
        if (!jsonObj.contains("values") || !jsonObj["values"].is_array()) {
            return Error(ErrorCode::InvalidData, "Item tag missing 'values' array");
        }

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                // 字符串格式: "minecraft:diamond" 或 "#minecraft:arrows"
                // 字符串格式默认 required=true
                rawData.entries.push_back({value.get<std::string>(), true});
            } else if (value.is_object()) {
                // 对象格式: {"id":"minecraft:diamond","required":false}
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::warn(
                        "ItemTagLoader: object entry in tag '{}' missing 'id' field, skipping", location.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::warn(
                        "ItemTagLoader: object entry 'id' in tag '{}' is empty, skipping", location.toString());
                    continue;
                }

                // 解析 required 字段（可选，默认 true）
                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                rawData.entries.push_back({id, required});
            } else {
                spdlog::warn(
                    "ItemTagLoader: value in tag '{}' is not a string or object, skipping", location.toString());
            }
        }

        return rawData;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse JSON: ") + e.what());
    }
}

/**
 * @brief 将合并后的标签数据解析并填充到 ItemTag
 *
 * 解析 # 标签引用并将物品添加到目标标签中。
 *
 * @param location 标签资源位置
 * @param data 合并后的标签数据
 * @param tag 目标 ItemTag（可以是已有标签或新注册的标签）
 */
static void resolveAndFillTag(const ResourceLocation& location, const MergedTagData& data, ItemTag& tag)
{
    if (data.replace) {
        tag.clear();
    }

    if (data.entries.empty()) {
        return;
    }

    std::vector<const Item*> items;
    std::unordered_set<ResourceLocation> visitedTags;
    visitedTags.insert(location); // 防止自引用

    for (const auto& entry : data.entries) {
        resolveTagEntry(entry, items, visitedTags, location);
    }

    tag.addAll(items);

    if (items.empty()) {
        spdlog::info("ItemTagLoader: tag '{}' resolved no valid items", location.toString());
    }
}

/**
 * @brief 递归解析标签及其依赖（第二阶段：带依赖顺序的解析）
 *
 * 当标签 A 引用 #B 时，需要先确保 B 的内容已被填充，
 * 否则 A 从 B 引用到的物品列表为空。
 * 此函数通过递归确保被引用的标签先于引用者被解析。
 *
 * @param location 要解析的标签位置
 * @param mergedTags 所有标签的合并数据
 * @param resolved 已完成解析的标签集合
 * @param resolving 正在解析中的标签集合（用于检测循环依赖）
 */
static void resolveTagWithDependencies(const ResourceLocation& location,
    std::unordered_map<ResourceLocation, MergedTagData>& mergedTags,
    std::unordered_set<ResourceLocation>& resolved,
    std::unordered_set<ResourceLocation>& resolving)
{
    // 已经解析过，无需重复处理
    if (resolved.count(location) > 0) {
        return;
    }

    // 不在本次数据包加载范围内，可能是内置标签或不存在
    auto it = mergedTags.find(location);
    if (it == mergedTags.end()) {
        return;
    }

    // 检测循环依赖：如果标签在 resolving 集合中，说明存在循环引用
    if (resolving.count(location) > 0) {
        spdlog::warn("ItemTagLoader: circular tag dependency detected '{}', skipping", location.toString());
        return;
    }

    resolving.insert(location);

    const auto& data = it->second;

    // 先递归解析所有 # 标签引用的依赖
    for (const auto& entry : data.entries) {
        if (!entry.id.empty() && entry.id[0] == '#') {
            std::string tagRef = entry.id.substr(1);
            ResourceLocation tagRefLocation = ResourceLocation::parse(tagRef);

            // 递归解析依赖的标签（确保它在被引用前已填充内容）
            resolveTagWithDependencies(tagRefLocation, mergedTags, resolved, resolving);
        }
    }

    // 所有依赖已解析，现在解析当前标签
    auto* tag = ItemTags::getTag(location);
    if (tag != nullptr) {
        resolveAndFillTag(location, data, *tag);
    }

    resolving.erase(location);
    resolved.insert(location);
}

/**
 * @brief 从路径提取标签名称
 *
 * 路径格式: namespace/tags/item/subdir/xxx.json -> namespace:subdir/xxx
 *
 * @param namespace_ 命名空间
 * @param directory 目录前缀（如 "minecraft/tags/item"）
 * @param resourcePath 资源路径
 * @return 标签资源位置
 */
static ResourceLocation extractTagLocation(
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
// ItemTagLoader 实现
// ============================================================================

Result<size_t> ItemTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    // 多数据包标签合并：使用 listResourceStacks 获取同一资源路径在所有数据包中的内容。
    // 标签加载语义：按数据包优先级从低到高遍历同名标签文件，
    // 默认追加，replace=true 时清空已有条目后追加。
    // listResourceStacks 返回的每个路径对应的 ResourceVersion 向量按数据包优先级从高到低排序，
    // 因此需要逆序遍历以匹配从低到高遍历顺序。

    // ========================================
    // 第一阶段：解析所有 JSON 文件，收集原始条目数据
    // （不解析 # 标签引用，仅记录原始条目）
    // ========================================
    std::unordered_map<ResourceLocation, MergedTagData> mergedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/tags/item";
        auto stacksResult = dataPackList.listResourceStacks(directory, ".json");

        if (!stacksResult.success()) {
            continue;
        }

        for (auto& [resourcePath, versions] : stacksResult.value()) {
            ResourceLocation location = extractTagLocation(namespace_, directory, resourcePath);

            // 遍历同一资源路径在所有数据包中的版本（按优先级从低到高）
            // listResourceStacks 返回的版本按优先级从高到低排序，
            // 但标签加载语义是先处理低优先级数据包，后处理高优先级数据包，
            // replace=true 时清空已有条目后追加，默认追加。
            // 因此需要逆序遍历。
            for (auto it = versions.rbegin(); it != versions.rend(); ++it) {
                auto& version = *it;

                // 第一阶段：仅解析 JSON 结构，不解析引用
                auto parseResult = parseJsonRaw(version.content, location);
                if (!parseResult.success()) {
                    spdlog::warn("ItemTagLoader: failed to parse tag {} (from datapack {}): {}",
                        location.toString(),
                        version.packName,
                        parseResult.error().message());
                    continue;
                }

                auto& rawData = parseResult.value();
                auto& existing = mergedTags[location];
                if (rawData.replace) {
                    // replace=true：清空已有条目，使用当前数据包的内容
                    existing.replace = true;
                    existing.entries.clear();
                    for (const auto& entry : rawData.entries) {
                        existing.entries.push_back(entry);
                    }
                } else {
                    // 默认追加模式
                    for (const auto& entry : rawData.entries) {
                        existing.entries.push_back(entry);
                    }
                }
            }
        }
    }

    // ========================================
    // 第二阶段：注册空标签（确保引用可解析），然后按依赖顺序解析引用并填充内容
    // ========================================

    // 2a. 先注册所有尚不存在的标签（空标签），确保标签引用可以找到目标
    for (auto& [location, data] : mergedTags) {
        if (ItemTags::getTag(location) == nullptr) {
            // 注册空标签占位符，后续会填充内容
            ItemTags::registerTag(location);
        }
    }

    // 2b. 按依赖顺序解析所有标签的引用并填充内容
    // 使用递归依赖解析确保被引用的标签先于引用者被解析，
    // 避免因 unordered_map 的遍历顺序不确定导致标签引用到空内容。
    std::unordered_set<ResourceLocation> resolved;  // 已完成解析的标签
    std::unordered_set<ResourceLocation> resolving; // 正在解析中的标签（检测循环依赖）

    for (auto& [location, data] : mergedTags) {
        resolveTagWithDependencies(location, mergedTags, resolved, resolving);
        ++loadedCount;
    }

    if (loadedCount > 0) {
        spdlog::info("ItemTagLoader: loaded {} item tags from datapacks", loadedCount);
    }

    return loadedCount;
}

Result<size_t> ItemTagLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    // ========================================
    // 第一阶段：解析所有 JSON 文件，收集原始条目数据
    // （不解析 # 标签引用，仅记录原始条目）
    // ========================================
    std::unordered_map<ResourceLocation, MergedTagData> mergedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/tags/item";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = extractTagLocation(namespace_, directory, resourcePath);

            // 读取 JSON 内容
            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("ItemTagLoader: failed to read tag file: {}", resourcePath);
                continue;
            }

            // 第一阶段：仅解析 JSON 结构，不解析引用
            auto parseResult = parseJsonRaw(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn(
                    "ItemTagLoader: failed to parse tag {}: {}", location.toString(), parseResult.error().message());
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
    // 第二阶段：注册空标签（确保引用可解析），然后按依赖顺序解析引用并填充内容
    // ========================================

    // 2a. 先注册所有尚不存在的标签（空标签），确保标签引用可以找到目标
    for (auto& [location, data] : mergedTags) {
        if (ItemTags::getTag(location) == nullptr) {
            ItemTags::registerTag(location);
        }
    }

    // 2b. 按依赖顺序解析所有标签的引用并填充内容
    std::unordered_set<ResourceLocation> resolved;
    std::unordered_set<ResourceLocation> resolving;

    for (auto& [location, data] : mergedTags) {
        resolveTagWithDependencies(location, mergedTags, resolved, resolving);
        ++loadedCount;
    }

    return loadedCount;
}

Result<std::unique_ptr<ItemTag>> ItemTagLoader::loadFromJson(const std::string& json, const ResourceLocation& location)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);

        // 解析 replace 字段（可选，默认 false）
        bool replace = false;
        if (jsonObj.contains("replace") && jsonObj["replace"].is_boolean()) {
            replace = jsonObj["replace"].get<bool>();
        }

        // 创建标签，传入 replace 标志
        auto tag = std::make_unique<ItemTag>(location, replace);

        // 解析 values 数组
        if (!jsonObj.contains("values") || !jsonObj["values"].is_array()) {
            return Error(ErrorCode::InvalidData, "Item tag missing 'values' array");
        }

        std::vector<const Item*> items;
        std::unordered_set<ResourceLocation> visitedTags;
        visitedTags.insert(location); // 防止自引用

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                // 字符串格式: "minecraft:diamond" 或 "#minecraft:arrows"
                // 字符串格式默认 required=true
                std::string entry = value.get<std::string>();
                RawTagEntry rawEntry{entry, true};
                resolveTagEntry(rawEntry, items, visitedTags, location);
            } else if (value.is_object()) {
                // 对象格式: {"id":"minecraft:diamond","required":false}
                // 支持 required 语义
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::warn(
                        "ItemTagLoader: object entry in tag '{}' missing 'id' field, skipping", location.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::warn(
                        "ItemTagLoader: object entry 'id' in tag '{}' is empty, skipping", location.toString());
                    continue;
                }

                // 解析 required 字段（可选，默认 true）
                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                RawTagEntry rawEntry{id, required};
                resolveTagEntry(rawEntry, items, visitedTags, location);
            } else {
                spdlog::warn(
                    "ItemTagLoader: value in tag '{}' is not a string or object, skipping", location.toString());
            }
        }

        // 将解析到的物品添加到标签中
        tag->addAll(items);

        if (items.empty()) {
            spdlog::info("ItemTagLoader: tag '{}' resolved no valid items", location.toString());
        }

        return tag;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse JSON: ") + e.what());
    }
}

} // namespace mc::item::tag
