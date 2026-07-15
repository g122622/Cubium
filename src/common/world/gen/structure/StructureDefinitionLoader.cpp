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

#include "StructureDefinitionLoader.hpp"

#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"
#include "common/world/gen/valueprovider/HeightProviderParser.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// 静态成员初始化
std::vector<std::unique_ptr<StructureDefinition>> StructureDefinitionLoader::s_definitions;
std::unordered_map<ResourceLocation, StructureDefinition*> StructureDefinitionLoader::s_byId;

Result<size_t> StructureDefinitionLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 structure 目录下的所有 JSON 文件
        // 注意：数据包路径为 worldgen/structure/，不是 worldgen/structure_set/
        std::string directory = namespace_ + "/worldgen/structure";
        auto listResult = dataPackList.listResources(directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取结构定义名称
            // 路径格式: namespace/worldgen/structure/path/to/struct.json
            std::string defName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            // 移除 .json 扩展名
            if (defName.size() >= 5 && defName.substr(defName.size() - 5) == ".json") {
                defName = defName.substr(0, defName.size() - 5);
            }

            ResourceLocation location(defName);

            // 读取 JSON 内容
            auto readResult = dataPackList.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read structure definition: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse structure definition {}: {}", defName, parseResult.error().message());
                continue;
            }

            ++loadedCount;
        }
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} structure definitions from datapacks", loadedCount);
    }

    return loadedCount;
}

Result<size_t> StructureDefinitionLoader::loadFromResourcePack(const IResourcePack& pack)
{
    size_t loadedCount = 0;

    // 获取所有命名空间
    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        // 列出 structure 目录下的所有 JSON 文件
        std::string directory = namespace_ + "/worldgen/structure";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");

        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            // 从路径提取结构定义名称
            std::string defName = namespace_ + ":" + resourcePath.substr(directory.length() + 1);
            if (defName.size() >= 5 && defName.substr(defName.size() - 5) == ".json") {
                defName = defName.substr(0, defName.size() - 5);
            }

            ResourceLocation location(defName);

            // 读取 JSON 内容
            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read structure definition: {}", resourcePath);
                continue;
            }

            // 解析 JSON
            auto parseResult = loadFromJson(readResult.value(), location);
            if (!parseResult.success()) {
                spdlog::warn("Failed to parse structure definition {}: {}", defName, parseResult.error().message());
                continue;
            }

            ++loadedCount;
        }
    }

    return loadedCount;
}

Result<void> StructureDefinitionLoader::loadFromJson(const std::string& json, const ResourceLocation& location)
{
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(json);

        auto def = std::make_unique<StructureDefinition>();
        def->id = location;

        // 解析结构类型
        if (jsonObj.contains("type") && jsonObj["type"].is_string()) {
            def->type = jsonObj["type"].get<std::string>();
        } else {
            return Error(ErrorCode::InvalidData, "Structure definition missing 'type' field");
        }

        // 解析生物群系列表/标签
        if (jsonObj.contains("biomes") && jsonObj["biomes"].is_string()) {
            def->biomes = ResourceLocation(jsonObj["biomes"].get<std::string>());
        }

        // 解析生成阶段
        if (jsonObj.contains("step") && jsonObj["step"].is_string()) {
            def->step = _parseDecorationStage(jsonObj["step"].get<std::string>());
        }

        // 解析地形适配模式
        if (jsonObj.contains("terrain_adaptation") && jsonObj["terrain_adaptation"].is_string()) {
            def->terrainAdaptation = _parseTerrainAdaptation(jsonObj["terrain_adaptation"].get<std::string>());
        }

        // 解析 Jigsaw 专用参数
        if (jsonObj.contains("start_pool") && jsonObj["start_pool"].is_string()) {
            def->startPool = ResourceLocation(jsonObj["start_pool"].get<std::string>());
        }

        if (jsonObj.contains("size") && jsonObj["size"].is_number_integer()) {
            def->size = jsonObj["size"].get<i32>();
        }

        if (jsonObj.contains("start_height")) {
            // start_height 两种形式：
            //   简写（单键锚点）：{ "absolute": 0 } / { "above_bottom": N } / { "below_top": N }
            //     —— MC HeightProvider.CODEC 将单键锚点视作 constant，本项目同此处理。
            //   分派（带 type）：{ "type": "minecraft:uniform", "min_inclusive": {...}, ... }
            //     —— 委托 HeightProviderParser::parse 解析。
            const auto& heightJson = jsonObj["start_height"];
            if (heightJson.is_object() && heightJson.contains("type") && heightJson["type"].is_string()) {
                auto heightResult = valueprovider::HeightProviderParser::parse(heightJson);
                if (heightResult.success()) {
                    def->startHeight = heightResult.value();
                } else {
                    spdlog::warn("Structure '{}': failed to parse start_height ({}), defaulting to absolute 0",
                        location.toString(),
                        heightResult.error().message());
                }
            } else {
                auto anchorResult = valueprovider::HeightProviderParser::parseAnchor(heightJson);
                if (anchorResult.success()) {
                    def->startHeight = valueprovider::ConstantHeight::create(anchorResult.value());
                } else {
                    spdlog::warn("Structure '{}': failed to parse start_height anchor ({}), defaulting to absolute 0",
                        location.toString(),
                        anchorResult.error().message());
                }
            }
        }

        if (jsonObj.contains("project_start_to_heightmap") && jsonObj["project_start_to_heightmap"].is_string()) {
            def->projectStartToHeightmap = true;
            def->heightmapName = jsonObj["project_start_to_heightmap"].get<std::string>();
        }

        if (jsonObj.contains("max_distance_from_center") && jsonObj["max_distance_from_center"].is_number_integer()) {
            def->maxDistanceFromCenter = MaxDistance(jsonObj["max_distance_from_center"].get<i32>());
        }

        if (jsonObj.contains("start_jigsaw_name") && jsonObj["start_jigsaw_name"].is_string()) {
            def->startJigsawName = ResourceLocation(jsonObj["start_jigsaw_name"].get<std::string>());
        }

        if (jsonObj.contains("use_expansion_hack") && jsonObj["use_expansion_hack"].is_boolean()) {
            def->useExpansionHack = jsonObj["use_expansion_hack"].get<bool>();
        }

        // 解析维度填充
        if (jsonObj.contains("dimension_padding")) {
            const auto& padding = jsonObj["dimension_padding"];
            if (padding.is_object()) {
                if (padding.contains("bottom") && padding["bottom"].is_number_integer()) {
                    def->dimensionPadding.bottom = padding["bottom"].get<i32>();
                }
                if (padding.contains("top") && padding["top"].is_number_integer()) {
                    def->dimensionPadding.top = padding["top"].get<i32>();
                }
            } else if (padding.is_number_integer()) {
                // 简写形式：单一数字同时应用于 top 和 bottom
                i32 value = padding.get<i32>();
                def->dimensionPadding.bottom = value;
                def->dimensionPadding.top = value;
            }
        }

        // 解析液体设置
        if (jsonObj.contains("liquid_settings") && jsonObj["liquid_settings"].is_string()) {
            def->liquidSettings = _parseLiquidSettings(jsonObj["liquid_settings"].get<std::string>());
        }

        // 解析生物生成覆盖（按类别分键）
        if (jsonObj.contains("spawn_overrides") && jsonObj["spawn_overrides"].is_object()) {
            _parseSpawnOverrides(jsonObj["spawn_overrides"], def->spawnOverrides);
        }

        // 解析池别名
        if (jsonObj.contains("pool_aliases") && jsonObj["pool_aliases"].is_array()) {
            _parsePoolAliasBindings(jsonObj["pool_aliases"], def->poolAliases);
        }

        // 存储定义
        spdlog::info("Loaded structure definition '{}' (type={})", location.toString(), def->type);
        s_byId[location] = def.get();
        s_definitions.push_back(std::move(def));

        return Result<void>::ok();
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Failed to parse structure definition: ") + e.what());
    }
}

const StructureDefinition* StructureDefinitionLoader::getDefinition(const ResourceLocation& id)
{
    auto it = s_byId.find(id);
    if (it != s_byId.end()) {
        return it->second;
    }
    return nullptr;
}

const std::vector<std::unique_ptr<StructureDefinition>>& StructureDefinitionLoader::getAllDefinitions()
{
    return s_definitions;
}

void StructureDefinitionLoader::clear()
{
    s_byId.clear();
    s_definitions.clear();
}

TerrainAdaptation StructureDefinitionLoader::_parseTerrainAdaptation(const std::string& str)
{
    std::string s = str;
    if (s.size() > 10 && s.substr(0, 10) == "minecraft:") {
        s = s.substr(10);
    }

    if (s == "bury") {
        return TerrainAdaptation::Bury;
    } else if (s == "beard_thin") {
        return TerrainAdaptation::BeardThin;
    } else if (s == "beard_box") {
        return TerrainAdaptation::BeardBox;
    } else if (s == "encapsulate") {
        return TerrainAdaptation::Encapsulate;
    } else {
        return TerrainAdaptation::None;
    }
}

DecorationStage StructureDefinitionLoader::_parseDecorationStage(const std::string& str)
{
    // 移除命名空间前缀
    std::string s = str;
    if (s.size() > 10 && s.substr(0, 10) == "minecraft:") {
        s = s.substr(10);
    }

    return DecorationStages::fromName(s);
}

LiquidSettings StructureDefinitionLoader::_parseLiquidSettings(const std::string& str)
{
    std::string s = str;
    if (s.size() > 10 && s.substr(0, 10) == "minecraft:") {
        s = s.substr(10);
    }

    if (s == "ignore_waterlogging") {
        return LiquidSettings::IgnoreWaterlogging;
    } else {
        return LiquidSettings::ApplyWaterlogging;
    }
}

namespace {
/// 解析 bounding_box 字符串为 SpawnOverrideType（"piece"→Piece, "full"→Full, 默认 Full）
SpawnOverrideType parseBoundingBoxType(const std::string& str)
{
    if (str == "piece") {
        return SpawnOverrideType::Piece;
    }
    return SpawnOverrideType::Full; // "full" 或未知
}

/// 去除 "minecraft:" 命名空间前缀
std::string stripMinecraftPrefix(const std::string& str)
{
    constexpr size_t kPrefixLen = 10; // "minecraft:"
    if (str.size() > kPrefixLen && str.substr(0, kPrefixLen) == "minecraft:") {
        return str.substr(kPrefixLen);
    }
    return str;
}
} // namespace

void StructureDefinitionLoader::_parseSpawnOverrides(
    const nlohmann::json& jsonObj, StructureSpawnOverrideMap& outOverrides)
{
    // jsonObj 形如 { "monster": { "bounding_box": "piece", "spawns": [...] }, ... }
    for (auto it = jsonObj.begin(); it != jsonObj.end(); ++it) {
        if (!it.value().is_object()) {
            continue;
        }
        const auto& entry = it.value();

        StructureSpawnOverride override;
        if (entry.contains("bounding_box") && entry["bounding_box"].is_string()) {
            override.boundingBoxType = parseBoundingBoxType(entry["bounding_box"].get<std::string>());
        }

        // spawns 列表：[{ "type": <entity>, "weight": N, "minCount": N, "maxCount": N }, ...]
        // 当前仅解析 minCount/maxCount，实体类型字段（type）后续接入生物生成链路时补全。
        if (entry.contains("spawns") && entry["spawns"].is_array()) {
            for (const auto& spawn : entry["spawns"]) {
                if (!spawn.is_object()) {
                    continue;
                }
                SpawnOverrideEntry spawnEntry;
                spawnEntry.minCount = spawn.value("minCount", 1);
                spawnEntry.maxCount = spawn.value("maxCount", 1);
                override.entries.push_back(std::move(spawnEntry));
            }
        }

        outOverrides[it.key()] = std::move(override);
    }
}

void StructureDefinitionLoader::_parsePoolAliasBindings(
    const nlohmann::json& jsonObj, jigsaw::PoolAliasBindings& outBindings)
{
    // jsonObj 为 pool_aliases 数组，每项形如 { "type": "minecraft:direct", "alias":..., "target":... }
    for (const auto& item : jsonObj) {
        if (!item.is_object() || !item.contains("type")) {
            continue;
        }
        const std::string type = stripMinecraftPrefix(item["type"].get<std::string>());

        if (type == "direct") {
            // { "alias": <id>, "target": <id> }
            if (!item.contains("alias") || !item.contains("target")) {
                spdlog::warn("pool_aliases: direct binding missing alias/target, skipped");
                continue;
            }
            outBindings.addBinding(
                std::make_unique<jigsaw::DirectPoolAliasBinding>(ResourceLocation(item["alias"].get<std::string>()),
                    ResourceLocation(item["target"].get<std::string>())));
        } else if (type == "random") {
            // { "alias": <id>, "targets": [ { "data": <id>, "weight": N }, ... ] }
            if (!item.contains("alias") || !item.contains("targets") || !item["targets"].is_array()) {
                spdlog::warn("pool_aliases: random binding missing alias/targets, skipped");
                continue;
            }
            std::vector<jigsaw::RandomPoolAliasBinding::WeightedTarget> targets;
            for (const auto& target : item["targets"]) {
                if (!target.is_object() || !target.contains("data")) {
                    continue;
                }
                targets.push_back({ResourceLocation(target["data"].get<std::string>()), target.value("weight", 1)});
            }
            outBindings.addBinding(std::make_unique<jigsaw::RandomPoolAliasBinding>(
                ResourceLocation(item["alias"].get<std::string>()), std::move(targets)));
        } else if (type == "random_group") {
            // { "groups": [ { "data": [ <direct bindings> ], "weight": N }, ... ] }
            // random_group 无顶层 alias，组标识名仅用于调试，此处用占位 id。
            if (!item.contains("groups") || !item["groups"].is_array()) {
                spdlog::warn("pool_aliases: random_group binding missing groups, skipped");
                continue;
            }
            std::vector<jigsaw::RandomGroupPoolAliasBinding::AliasGroup> groups;
            for (const auto& group : item["groups"]) {
                if (!group.is_object() || !group.contains("data") || !group["data"].is_array()) {
                    continue;
                }
                jigsaw::RandomGroupPoolAliasBinding::AliasGroup aliasGroup;
                for (const auto& binding : group["data"]) {
                    if (!binding.is_object() || !binding.contains("type")) {
                        continue;
                    }
                    const std::string bType = stripMinecraftPrefix(binding["type"].get<std::string>());
                    if (bType != "direct" || !binding.contains("alias") || !binding.contains("target")) {
                        spdlog::warn("pool_aliases: random_group non-direct binding skipped");
                        continue;
                    }
                    aliasGroup.bindings.push_back(std::make_unique<jigsaw::DirectPoolAliasBinding>(
                        ResourceLocation(binding["alias"].get<std::string>()),
                        ResourceLocation(binding["target"].get<std::string>())));
                }
                aliasGroup.weight = group.value("weight", 1);
                groups.push_back(std::move(aliasGroup));
            }
            outBindings.addBinding(std::make_unique<jigsaw::RandomGroupPoolAliasBinding>(
                ResourceLocation("minecraft:random_group"), std::move(groups)));
        } else {
            spdlog::warn("pool_aliases: unknown binding type '{}', skipped", type);
        }
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
