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
#include "common/item/core/ItemRegistry.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <unordered_map>
#include <unordered_set>

namespace mc::item::tag {

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 根据 ResourceLocation 解析物品指针
 *
 * 在 ItemRegistry 中查找匹配的已注册物品。
 *
 * @param itemLocation 物品的资源位置
 * @return 对应的 Item 指针，未找到则返回 nullptr
 */
static const Item* resolveItem(const ResourceLocation& itemLocation)
{
    return ItemRegistry::instance().getItem(itemLocation);
}

/**
 * @brief 解析物品名称为 Item 指针
 *
 * 支持带命名空间前缀（"minecraft:diamond"）和不带前缀（"diamond"）的格式。
 *
 * @param itemName 物品名称
 * @return 对应的 Item 指针，未找到则返回 nullptr
 */
static const Item* resolveItemByName(const std::string& itemName)
{
    ResourceLocation location = ResourceLocation::parse(itemName);
    return resolveItem(location);
}

/**
 * @brief 解析标签值列表中的单个条目
 *
 * 支持三种格式：
 * - 直接物品名称: "minecraft:diamond"
 * - 标签引用: "#minecraft:arrows"（解析引用标签中的所有物品）
 * - 对象格式: {"id":"minecraft:diamond","required":false}
 *
 * @param entry 值条目字符串（如 "minecraft:diamond" 或 "#minecraft:arrows"）
 * @param required 是否必须存在（required=true 时缺失条目会输出警告，required=false 时静默跳过）
 * @param items 输出参数：收集的 Item 指针
 * @param visitedTags 已访问的标签集合（防止循环引用）
 */
static void resolveTagEntry(const std::string& entry,
    bool required,
    std::vector<const Item*>& items,
    std::unordered_set<ResourceLocation>& visitedTags)
{
    if (entry.empty()) {
        return;
    }

    if (entry[0] == '#') {
        // 标签引用: #namespace:path
        std::string tagRef = entry.substr(1);
        ResourceLocation tagLocation = ResourceLocation::parse(tagRef);

        // 防止循环引用
        if (visitedTags.count(tagLocation) > 0) {
            if (required) {
                spdlog::warn("ItemTagLoader: 循环标签引用 '{}' (required), 跳过", entry);
            }
            return;
        }
        visitedTags.insert(tagLocation);

        // 查找被引用的标签
        auto* referencedTag = ItemTags::getTag(tagLocation);
        if (referencedTag != nullptr) {
            for (const auto* item : referencedTag->getItems()) {
                items.push_back(item);
            }
        } else {
            if (required) {
                spdlog::warn("ItemTagLoader: 引用的标签 '{}' 未找到 (required), 跳过", entry);
            }
            // required=false 时静默跳过
        }
    } else {
        // 直接物品名称
        const Item* item = resolveItemByName(entry);
        if (item != nullptr) {
            items.push_back(item);
        } else {
            if (required) {
                spdlog::warn("ItemTagLoader: 未知的物品 '{}' (required), 跳过", entry);
            }
            // required=false 时静默跳过
        }
    }
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
    // MC Java 的标签加载语义：按数据包优先级从低到高遍历同名标签文件，
    // 默认追加，replace=true 时清空已有条目后追加。
    // listResourceStacks 返回的每个路径对应的 ResourceVersion 向量按数据包优先级从高到低排序，
    // 因此需要逆序遍历以匹配 MC Java 的从低到高遍历顺序。

    // 用于存储已解析的标签数据
    // key: 标签 ResourceLocation, value: (replace标志, item指针列表)
    struct TagParseData {
        bool replace = false;
        std::vector<const Item*> items;
    };
    std::unordered_map<ResourceLocation, TagParseData> parsedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 tags/item/ 目录下所有数据包中的 JSON 文件及其内容栈
        std::string directory = namespace_ + "/tags/item";
        auto stacksResult = dataPackList.listResourceStacks(directory, ".json");

        if (!stacksResult.success()) {
            continue;
        }

        for (auto& [resourcePath, versions] : stacksResult.value()) {
            // 从路径提取标签名称
            // 路径格式: namespace/tags/item/subdir/xxx.json
            std::string tagName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            // 移除 .json 扩展名
            if (tagName.size() >= 5 && tagName.substr(tagName.size() - 5) == ".json") {
                tagName = tagName.substr(0, tagName.size() - 5);
            }

            ResourceLocation location(tagName);

            // 遍历同一资源路径在所有数据包中的版本（按优先级从低到高）
            // listResourceStacks 返回的版本按优先级从高到低排序，
            // 但 MC Java 的 TagLoader.load() 语义是先处理低优先级数据包，后处理高优先级数据包，
            // replace=true 时清空已有条目后追加，默认追加。
            // 因此需要逆序遍历，以匹配 MC Java 的行为。
            for (auto it = versions.rbegin(); it != versions.rend(); ++it) {
                auto& version = *it;
                // 解析 JSON
                auto parseResult = loadFromJson(version.content, location);
                if (!parseResult.success()) {
                    spdlog::warn("ItemTagLoader: 无法解析标签 {} (来自数据包 {}): {}",
                        tagName,
                        version.packName,
                        parseResult.error().message());
                    continue;
                }

                std::unique_ptr<ItemTag> parsedTag = parseResult.value();
                auto& existing = parsedTags[location];
                if (parsedTag->isReplace()) {
                    // replace=true：清空已有条目，使用当前数据包的内容
                    existing.replace = true;
                    existing.items.clear();
                    for (const auto* item : parsedTag->getItems()) {
                        existing.items.push_back(item);
                    }
                } else {
                    // 默认追加模式
                    for (const auto* item : parsedTag->getItems()) {
                        existing.items.push_back(item);
                    }
                }
            }
        }
    }

    // 将解析结果注册到 ItemTags
    for (auto& [location, tagData] : parsedTags) {
        // 尝试获取已有标签（可能由 ItemTags::initialize() 注册的内置标签）
        auto* existingTag = ItemTags::getTag(location);
        if (existingTag != nullptr) {
            // 已有标签：根据 replace 语义合并
            if (tagData.replace) {
                // replace=true：清空现有标签内容，使用数据包的内容
                existingTag->clear();
            }
            for (const auto* item : tagData.items) {
                existingTag->add(item);
            }
        } else {
            // 全新标签：注册到 ItemTags
            auto& newTag = ItemTags::registerTag(location);
            if (tagData.replace) {
                // replace 对新标签无意义，但保持语义一致
                newTag.setReplace(true);
            }
            for (const auto* item : tagData.items) {
                newTag.add(item);
            }
        }

        ++loadedCount;
    }

    if (loadedCount > 0) {
        spdlog::info("ItemTagLoader: 从数据包加载了 {} 个物品标签", loadedCount);
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

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 tags/item/ 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/tags/item";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取标签名称
            std::string tagName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            if (tagName.size() >= 5 && tagName.substr(tagName.size() - 5) == ".json") {
                tagName = tagName.substr(0, tagName.size() - 5);
            }

            ResourceLocation location(tagName);

            // 读取 JSON 内容
            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("ItemTagLoader: 无法读取标签文件: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("ItemTagLoader: 无法解析标签 {}: {}", tagName, parseResult.error().message());
                continue;
            }

            // 将解析结果合并到 ItemTags 注册表
            std::unique_ptr<ItemTag> parsedTag = parseResult.value();
            auto* existingTag = ItemTags::getTag(parsedTag->getId());
            if (existingTag != nullptr) {
                if (parsedTag->isReplace()) {
                    // replace=true：清空现有标签内容
                    existingTag->clear();
                }
                for (const auto* item : parsedTag->getItems()) {
                    existingTag->add(item);
                }
            } else {
                // 全新标签：注册到 ItemTags
                auto& newTag = ItemTags::registerTag(parsedTag->getId());
                if (parsedTag->isReplace()) {
                    newTag.setReplace(true);
                }
                for (const auto* item : parsedTag->getItems()) {
                    newTag.add(item);
                }
            }

            ++loadedCount;
        }
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
            return Error(ErrorCode::InvalidData, "物品标签缺少 'values' 数组");
        }

        std::vector<const Item*> items;
        std::unordered_set<ResourceLocation> visitedTags;
        visitedTags.insert(location); // 防止自引用

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                // 字符串格式: "minecraft:diamond" 或 "#minecraft:arrows"
                // 字符串格式默认 required=true
                std::string entry = value.get<std::string>();
                resolveTagEntry(entry, true, items, visitedTags);
            } else if (value.is_object()) {
                // 对象格式: {"id":"minecraft:diamond","required":false}
                // 对应 MC Java 的 TagEntry 对象格式，支持 required 语义
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::warn("ItemTagLoader: 标签 '{}' 中的对象格式条目缺少 'id' 字段, 跳过", location.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::warn("ItemTagLoader: 标签 '{}' 中的对象格式条目 'id' 为空, 跳过", location.toString());
                    continue;
                }

                // 解析 required 字段（可选，默认 true）
                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                resolveTagEntry(id, required, items, visitedTags);
            } else {
                spdlog::warn("ItemTagLoader: 标签 '{}' 中的值不是字符串或对象, 跳过", location.toString());
            }
        }

        // 将解析到的物品添加到标签中
        tag->addAll(items);

        if (items.empty()) {
            spdlog::info("ItemTagLoader: 标签 '{}' 没有解析到有效的物品", location.toString());
        }

        return tag;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON 解析错误: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("解析 JSON 失败: ") + e.what());
    }
}

} // namespace mc::item::tag
