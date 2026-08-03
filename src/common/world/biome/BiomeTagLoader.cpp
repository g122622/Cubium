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

#include "BiomeTagLoader.hpp"
#include "BiomeLoader.hpp"
#include "BiomeTags.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/BiomeTag.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::world::biome {

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 根据生物群系名称解析 BiomeId
 *
 * 复用 BiomeLoader 内置的 65 项 biome 名→BiomeId 映射表（含 11 个 1.18+ 重命名 biome 别名），
 * 绕开 Biome 内部 m_name——部分老 biome 的 m_name 仍是 1.16.5 旧名（如 stone_shore），
 * 而数据包 tag JSON 用的是 1.18+ 新名（如 stony_shore），直接比 m_name 会失配。
 *
 * @param biomeName 生物群系名称（如 "minecraft:desert" 或 "desert"）
 * @return 对应的 BiomeId，未找到则返回空
 */
static std::optional<BiomeId> resolveBiomeId(const std::string& biomeName)
{
    return BiomeLoader::biomeIdByName(ResourceLocation(biomeName));
}

/**
 * @brief 解析标签值列表中的单个条目
 *
 * 支持三种格式：
 * - 直接生物群系名称: "minecraft:desert"
 * - 标签引用: "#minecraft:is_jungle"（解析引用标签中的所有生物群系）
 * - 对象格式: {"id":"minecraft:desert","required":false}（由上层解析后调用此方法）
 *
 * @param entry 值条目字符串（如 "minecraft:desert" 或 "#minecraft:is_jungle"）
 * @param required 是否必须存在（required=true 时缺失条目会输出警告，required=false 时静默跳过）
 * @param biomeIds 输出参数：收集的 BiomeId 集合
 * @param visitedTags 已访问的标签集合（防止循环引用）
 */
static void resolveTagEntry(const std::string& entry,
    bool required,
    std::vector<BiomeId>& biomeIds,
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
                spdlog::warn("BiomeTagLoader: circular tag reference '{}' (required), skipped", entry);
            }
            return;
        }
        visitedTags.insert(tagLocation);

        // 查找被引用的标签
        auto* referencedTag = BiomeTags::getTag(tagLocation);
        if (referencedTag != nullptr) {
            for (BiomeId id : referencedTag->getBiomeIds()) {
                biomeIds.push_back(id);
            }
        } else {
            if (required) {
                spdlog::warn("BiomeTagLoader: referenced tag '{}' not found (required), skipped", entry);
            }
            // required=false 时静默跳过
        }
    } else {
        // 直接生物群系名称
        auto biomeId = resolveBiomeId(entry);
        if (biomeId.has_value()) {
            biomeIds.push_back(biomeId.value());
        } else {
            if (required) {
                spdlog::warn("BiomeTagLoader: unknown biome '{}' (required), skipped", entry);
            }
            // required=false 时静默跳过
        }
    }
}

// ============================================================================
// BiomeTagLoader 实现
// ============================================================================

Result<size_t> BiomeTagLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
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
    // key: 标签 ResourceLocation, value: (replace标志, biomeId列表)
    struct TagParseData {
        bool replace = false;
        std::vector<BiomeId> biomeIds;
    };
    std::unordered_map<ResourceLocation, TagParseData> parsedTags;

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 tags/worldgen/biome/ 目录下所有数据包中的 JSON 文件及其内容栈
        std::string directory = namespace_ + "/tags/worldgen/biome";
        auto stacksResult = dataPackList.listResourceStacks(directory, ".json");

        if (!stacksResult.success()) {
            continue;
        }

        for (auto& [resourcePath, versions] : stacksResult.value()) {
            // 从路径提取标签名称
            // 路径格式: namespace/tags/worldgen/biome/has_structure/xxx.json
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
                    spdlog::warn("BiomeTagLoader: failed to parse tag {} (from data pack {}): {}",
                        tagName,
                        version.packName,
                        parseResult.error().message());
                    continue;
                }

                std::unique_ptr<BiomeTag> parsedTag = parseResult.value();
                auto& existing = parsedTags[location];
                if (parsedTag->isReplace()) {
                    // replace=true：清空已有条目，使用当前数据包的内容
                    existing.replace = true;
                    existing.biomeIds.clear();
                    for (BiomeId id : parsedTag->getBiomeIds()) {
                        existing.biomeIds.push_back(id);
                    }
                } else {
                    // 默认追加模式
                    for (BiomeId id : parsedTag->getBiomeIds()) {
                        existing.biomeIds.push_back(id);
                    }
                }
            }
        }
    }

    // 将解析结果注册到 BiomeTags
    for (auto& [location, tagData] : parsedTags) {
        auto* existingTag = BiomeTags::getTag(location);
        if (existingTag != nullptr) {
            if (tagData.replace) {
                // replace=true：清空现有标签内容，使用数据包的内容
                existingTag->clear();
            }
            for (BiomeId id : tagData.biomeIds) {
                existingTag->add(id);
            }
        }
        // 注意：如果标签在 BiomeTags::initialize() 中尚未注册，
        // 数据包中的标签会被丢弃，因为 BiomeTags 使用静态注册表。
        // 数据包加载应在 initialize() 之后执行。

        ++loadedCount;
    }

    if (loadedCount > 0) {
        spdlog::info("BiomeTagLoader: loaded {} biome tags from data pack", loadedCount);
    }

    return loadedCount;
}

Result<size_t> BiomeTagLoader::loadFromResourcePack(const IResourcePack& pack)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 tags/worldgen/biome/ 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/tags/worldgen/biome";
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
                spdlog::warn("BiomeTagLoader: failed to read tag file: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("BiomeTagLoader: failed to parse tag {}: {}", tagName, parseResult.error().message());
                continue;
            }

            // 将解析结果合并到 BiomeTags 注册表
            std::unique_ptr<BiomeTag> parsedTag = parseResult.value();
            auto* existingTag = BiomeTags::getTag(parsedTag->getId());
            if (existingTag != nullptr) {
                if (parsedTag->isReplace()) {
                    // replace=true：清空现有标签内容
                    existingTag->clear();
                }
                for (BiomeId id : parsedTag->getBiomeIds()) {
                    existingTag->add(id);
                }
            }

            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<std::unique_ptr<BiomeTag>> BiomeTagLoader::loadFromJson(
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
        auto tag = std::make_unique<BiomeTag>(location, replace);

        // 解析 values 数组
        if (!jsonObj.contains("values") || !jsonObj["values"].is_array()) {
            return Error(ErrorCode::InvalidData, "biome tag missing 'values' array");
        }

        std::vector<BiomeId> biomeIds;
        std::unordered_set<ResourceLocation> visitedTags;
        visitedTags.insert(location); // 防止自引用

        for (const auto& value : jsonObj["values"]) {
            if (value.is_string()) {
                // 字符串格式: "minecraft:desert" 或 "#minecraft:is_jungle"
                // 字符串格式默认 required=true
                std::string entry = value.get<std::string>();
                resolveTagEntry(entry, true, biomeIds, visitedTags);
            } else if (value.is_object()) {
                // 对象格式: {"id":"minecraft:desert","required":false}
                // 对应 MC Java 的 TagEntry 对象格式，支持 required 语义
                if (!value.contains("id") || !value["id"].is_string()) {
                    spdlog::warn(
                        "BiomeTagLoader: object entry in tag '{}' missing 'id' field, skipped", location.toString());
                    continue;
                }

                std::string id = value["id"].get<std::string>();
                if (id.empty()) {
                    spdlog::warn(
                        "BiomeTagLoader: object entry 'id' in tag '{}' is empty, skipped", location.toString());
                    continue;
                }

                // 解析 required 字段（可选，默认 true）
                bool required = true;
                if (value.contains("required") && value["required"].is_boolean()) {
                    required = value["required"].get<bool>();
                }

                resolveTagEntry(id, required, biomeIds, visitedTags);
            } else {
                spdlog::warn(
                    "BiomeTagLoader: value in tag '{}' is not a string or object, skipped", location.toString());
            }
        }

        // 将解析到的生物群系 ID 添加到标签中
        tag->addAll(biomeIds);

        if (biomeIds.empty()) {
            spdlog::info("BiomeTagLoader: tag '{}' resolved no valid biome IDs", location.toString());
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

} // namespace mc::world::biome
