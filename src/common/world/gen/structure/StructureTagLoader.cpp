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

#include "StructureTagLoader.hpp"
#include "StructureTags.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <unordered_map>

namespace mc::world::gen::structure {

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 解析标签值列表中的单个条目
 *
 * 支持三种格式：
 * - 直接结构名称: "minecraft:shipwreck"
 * - 标签引用: "#minecraft:ocean_ruin"（解析引用标签中的所有结构）
 * - 对象格式: {"id":"minecraft:shipwreck","required":false}（由上层解析后调用此方法）
 *
 * 循环检测采用"按解析路径"策略：visitedTags 按值传递，每个标签引用分支获得
 * 独立的访问集合副本。这样同一标签在不同分支中都能被正确展开，避免跨分支
 * 错误跳过。最终去重由 StructureTag 内部的 std::unordered_set 保证。
 *
 * 注意：当前实现不递归解析标签引用——StructureTags::getTag() 返回的是已构建
 * 完成的标签，直接拷贝其成员即可。真正的循环引用（如 A→B→A）不会导致
 * 无限递归，因为不存在的数据包标签 getTag() 返回 nullptr。
 * 此处的循环检测主要用于防止自引用（loadFromJson 已预插入 location）。
 *
 * @param entry 值条目字符串（如 "minecraft:shipwreck" 或 "#minecraft:ocean_ruin"）
 * @param required 是否必须存在（required=true 时缺失条目会输出警告，required=false 时静默跳过）
 * @param structureIds 输出参数：收集的结构 ID 集合
 * @param visitedTags 已访问的标签集合（按值传递，实现路径级循环检测）
 */
static void resolveTagEntry(const std::string& entry,
    bool required,
    std::vector<ResourceLocation>& structureIds,
    std::unordered_set<ResourceLocation> visitedTags)
{
    if (entry.empty()) {
        return;
    }

    if (entry[0] == '#') {
        // 标签引用: #namespace:path
        std::string tagRef = entry.substr(1);
        ResourceLocation tagLocation = ResourceLocation::parse(tagRef);

        // 路径级循环检测：若当前解析路径已包含此标签，跳过
        // 由于 visitedTags 按值传递，不同分支的解析路径互不影响
        if (visitedTags.count(tagLocation) > 0) {
            if (required) {
                spdlog::warn("StructureTagLoader: circular tag reference '{}' (required), skipped", entry);
            }
            return;
        }
        // 注意：此处修改的是局部副本，不影响调用方的 visitedTags
        visitedTags.insert(tagLocation);

        // 查找被引用的标签（已构建完成的标签）
        auto* referencedTag = StructureTags::getTag(tagLocation);
        if (referencedTag != nullptr) {
            for (const auto& id : referencedTag->getStructureIds()) {
                structureIds.push_back(id);
            }
        } else {
            if (required) {
                spdlog::warn("StructureTagLoader: referenced tag '{}' not found (required), skipped", entry);
            }
            // required=false 时静默跳过
        }
    } else {
        // 直接结构 ID
        ResourceLocation structureId = ResourceLocation::parse(entry);
        structureIds.push_back(structureId);
    }
}

// ============================================================================
// StructureTagLoader 实现
// ============================================================================

Result<size_t> StructureTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackRepository)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = dataPackRepository.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    // 多数据包标签合并：使用 listResourceStacks 获取同一资源路径在所有数据包中的内容。
    // MC Java 的标签加载语义：按数据包优先级从低到高遍历同名标签文件，
    // 默认追加，replace=true 时清空已有条目后追加。
    // listResourceStacks 返回的每个路径对应的 ResourceVersion 向量按数据包优先级从高到低排序，
    // 因此需要逆序遍历以匹配 MC Java 的从低到高遍历顺序。

    // 用于存储已解析的标签数据
    // key: 标签 ResourceLocation, value: (replace标志, structureId列表)
    struct TagParseData {
        bool replace = false;
        std::vector<ResourceLocation> structureIds;
    };
    std::unordered_map<ResourceLocation, TagParseData> parsedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 tags/worldgen/structure/ 目录下所有数据包中的 JSON 文件及其内容栈
        std::string directory = namespace_ + "/tags/worldgen/structure";
        auto stacksResult = dataPackRepository.listResourceStacks(directory, ".json");

        if (!stacksResult.success()) {
            continue;
        }

        for (auto& [resourcePath, versions] : stacksResult.value()) {
            // 从路径提取标签名称
            // 路径格式: namespace/tags/worldgen/structure/xxx.json
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
                    spdlog::warn("StructureTagLoader: failed to parse tag {} (from data pack {}): {}",
                        tagName,
                        version.packName,
                        parseResult.error().message());
                    continue;
                }

                std::unique_ptr<StructureTag> parsedTag = parseResult.value();
                auto& existing = parsedTags[location];
                if (parsedTag->isReplace()) {
                    // replace=true：清空已有条目，使用当前数据包的内容
                    existing.replace = true;
                    existing.structureIds.clear();
                    for (const auto& id : parsedTag->getStructureIds()) {
                        existing.structureIds.push_back(id);
                    }
                } else {
                    // 默认追加模式
                    for (const auto& id : parsedTag->getStructureIds()) {
                        existing.structureIds.push_back(id);
                    }
                }
            }
        }
    }

    // 将解析结果注册到 StructureTags
    for (auto& [location, tagData] : parsedTags) {
        auto* existingTag = StructureTags::getTag(location);
        if (existingTag != nullptr) {
            if (tagData.replace) {
                // replace=true：清空现有标签内容，使用数据包的内容
                existingTag->clear();
            }
            for (const auto& id : tagData.structureIds) {
                existingTag->add(id);
            }
        }
        // 注意：如果标签在 StructureTags::initialize() 中尚未注册，
        // 数据包中的标签会被丢弃，因为 StructureTags 使用静态注册表。
        // 数据包加载应在 initialize() 之后执行。

        ++loadedCount;
    }

    if (loadedCount > 0) {
        spdlog::info("StructureTagLoader: loaded {} structure tags from data pack", loadedCount);
    }

    return loadedCount;
}

Result<size_t> StructureTagLoader::loadFromResourcePack(const IResourcePack& pack)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 tags/worldgen/structure/ 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/tags/worldgen/structure";
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
                spdlog::warn("StructureTagLoader: failed to read tag file: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("StructureTagLoader: failed to parse tag {}: {}", tagName, parseResult.error().message());
                continue;
            }

            // 将解析结果合并到 StructureTags 注册表
            std::unique_ptr<StructureTag> parsedTag = parseResult.value();
            auto* existingTag = StructureTags::getTag(parsedTag->getId());
            if (existingTag != nullptr) {
                if (parsedTag->isReplace()) {
                    // replace=true：清空现有标签内容
                    existingTag->clear();
                }
                for (const auto& id : parsedTag->getStructureIds()) {
                    existingTag->add(id);
                }
            }

            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<std::unique_ptr<StructureTag>> StructureTagLoader::loadFromJson(
    const std::string& json, const ResourceLocation& location)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);

        // 解析 replace 字段（可选，默认 false）
        bool replace = false;
        if (jsonObj.contains("replace") && jsonObj["replace"].is_boolean()) {
            replace = jsonObj["replace"].get<bool>();
        }

        // 创建标签，传入 replace 标志
        auto tag = std::make_unique<StructureTag>(location, replace);

        // 解析 values 数组
        if (!jsonObj.contains("values") || !jsonObj["values"].is_array()) {
            return Error(ErrorCode::InvalidData, "structure tag missing 'values' array");
        }

        std::vector<ResourceLocation> structureIds;
        std::unordered_set<ResourceLocation> visitedTags;
        visitedTags.insert(location); // 防止自引用

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                // 字符串格式: "minecraft:shipwreck" 或 "#minecraft:ocean_ruin"
                // 字符串格式默认 required=true
                std::string entry = value.get<std::string>();
                resolveTagEntry(entry, true, structureIds, visitedTags);
            } else if (value.is_object()) {
                // 对象格式: {"id":"minecraft:shipwreck","required":false}
                // 对应 MC Java 的 TagEntry 对象格式，支持 required 语义
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::warn(
                        "StructureTagLoader: object entry in tag '{}' missing 'id' field, skipped", location.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::warn("StructureTagLoader: object entry 'id' in tag '{}' is empty, skipped", location.toString());
                    continue;
                }

                // 解析 required 字段（可选，默认 true）
                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                resolveTagEntry(id, required, structureIds, visitedTags);
            } else {
                spdlog::warn("StructureTagLoader: value in tag '{}' is not a string or object, skipped", location.toString());
            }
        }

        // 将解析到的结构 ID 添加到标签中
        tag->addAll(structureIds);

        if (structureIds.empty()) {
            spdlog::info("StructureTagLoader: tag '{}' resolved no valid structure IDs", location.toString());
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

} // namespace mc::world::gen::structure
