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
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::item::tag {

namespace {

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
ResourceLocation extractTagLocation(
    const std::string& namespace_, const std::string& directory, const std::string& resourcePath)
{
    std::string tagName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
    // 移除 .json 扩展名
    if (tagName.size() >= 5 && tagName.substr(tagName.size() - 5) == ".json") {
        tagName = tagName.substr(0, tagName.size() - 5);
    }
    return ResourceLocation(tagName);
}

/// 全部标签编译完成后再统一应用：先把引用目标注册成空标签，再逐条落内容。
void applyAll(const std::unordered_map<ResourceLocation, nlohmann::json>& compiled)
{
    for (const auto& [location, unused] : compiled) {
        if (ItemTags::getTag(location) == nullptr) {
            ItemTags::registerTag(location);
        }
    }
    for (const auto& [location, contents] : compiled) {
        static_cast<void>(ItemTags::applyTagJson(contents, location));
    }
    if (auto linked = ItemTags::linkAndExpand(); linked.failed()) {
        spdlog::error("ItemTagLoader: {}", linked.error().message());
    }
}

} // namespace

Result<size_t> ItemTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    // 获取所有命名空间
    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return static_cast<size_t>(0);
    }

    // 多数据包标签合并：使用 listResourceStacks 获取同一资源路径在所有数据包中的内容。
    // `listResourceStacks` 的版本按数据包优先级从高到低排列。
    std::unordered_map<ResourceLocation, nlohmann::json> compiled;
    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/tags/item";
        auto stacksResult = dataPackList.listResourceStacks(directory, ".json");
        if (!stacksResult.success()) {
            continue;
        }

        for (const auto& [resourcePath, versions] : stacksResult.value()) {
            const ResourceLocation location = extractTagLocation(namespace_, directory, resourcePath);

            std::vector<std::string> contentsList;
            contentsList.reserve(versions.size());
            for (const auto& version : versions) {
                contentsList.push_back(version.content);
            }

            auto merged = ItemTags::mergeTagVersions(contentsList, location);
            if (merged.failed()) {
                spdlog::warn("ItemTagLoader: {}", merged.error().message());
                continue;
            }
            compiled[location] = std::move(merged).value();
        }
    }

    const size_t loadedCount = compiled.size();
    applyAll(compiled);
    if (loadedCount > 0) {
        spdlog::info("ItemTagLoader: loaded {} item tags from datapacks", loadedCount);
    }
    return loadedCount;
}

Result<size_t> ItemTagLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    // 获取所有命名空间
    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return static_cast<size_t>(0);
    }

    std::unordered_map<ResourceLocation, nlohmann::json> compiled;
    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/tags/item";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation location = extractTagLocation(namespace_, directory, resourcePath);

            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("ItemTagLoader: failed to read tag file: {}", resourcePath);
                continue;
            }

            auto compiledTag = ItemTags::compileTagJson(readResult.value(), location);
            if (compiledTag.failed()) {
                spdlog::warn("ItemTagLoader: {}", compiledTag.error().message());
                continue;
            }
            compiled[location] = std::move(compiledTag).value();
        }
    }

    const size_t loadedCount = compiled.size();
    applyAll(compiled);
    return loadedCount;
}

Result<ItemTag> ItemTagLoader::loadFromJson(
    const std::string& json, const ResourceLocation& location, std::optional<ItemTag> base)
{
    auto compiledTag = ItemTags::compileTagJson(json, location);
    if (compiledTag.failed()) {
        return compiledTag.error();
    }
    ItemTag tag = base.has_value() ? std::move(*base) : ItemTag(location, false);
    if (auto applied = ItemTags::applyTagJson(compiledTag.value(), tag); applied.failed()) {
        return applied.error();
    }
    // 独立解析的标签不进标签表，直接在结果上展开引用（引用的标签必须已注册）。
    if (!tag.getNestedTags().empty()) {
        for (const auto& nested : tag.getNestedTags()) {
            const ItemTag* source = ItemTags::getTag(nested);
            if (source == nullptr) {
                return Error(ErrorCode::NotFound,
                    "Item tag '" + location.toString() + "' references unknown tag '" + nested.toString() + "'");
            }
            source->forEachItem([&tag](const Item* item) { tag.add(item); });
        }
    }
    return tag;
}

} // namespace mc::item::tag