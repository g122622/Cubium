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

#include "common/resource/DataPackList.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/resource/PackType.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/gen/surface/SurfaceRules.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// 静态成员初始化
std::vector<std::unique_ptr<StructureDefinition>> StructureDefinitionLoader::s_definitions;
std::unordered_map<ResourceLocation, StructureDefinition*> StructureDefinitionLoader::s_byId;

Result<size_t> StructureDefinitionLoader::loadFromDataPackList(const resource::DataPackList& dataPackList)
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

Result<size_t> StructureDefinitionLoader::loadFromResourcePack(const resource::IResourcePack& pack)
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
            def->startHeight = _parseHeightProvider(jsonObj["start_height"]);
        }

        if (jsonObj.contains("project_start_to_heightmap") && jsonObj["project_start_to_heightmap"].is_string()) {
            def->projectStartToHeightmap = true;
            def->heightmapName = jsonObj["project_start_to_heightmap"].get<std::string>();
        }

        if (jsonObj.contains("max_distance_from_center") && jsonObj["max_distance_from_center"].is_number_integer()) {
            def->maxDistanceFromCenter = JigsawStructure::MaxDistance(jsonObj["max_distance_from_center"].get<i32>());
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

        // 解析池别名
        if (jsonObj.contains("pool_aliases") && jsonObj["pool_aliases"].is_array()) {
            // 池别名解析后续实现，目前仅记录日志
            spdlog::info("Structure '{}': pool_aliases parsing deferred", location.toString());
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

std::unique_ptr<valueprovider::HeightProvider> StructureDefinitionLoader::_parseHeightProvider(
    const nlohmann::json& jsonObj)
{
    if (!jsonObj.is_object()) {
        return nullptr;
    }

    // 绝对高度: { "absolute": N }
    if (jsonObj.contains("absolute") && jsonObj["absolute"].is_number_integer()) {
        i32 absoluteValue = jsonObj["absolute"].get<i32>();
        return std::make_unique<valueprovider::ConstantHeight>(surface::VerticalAnchor::absolute(absoluteValue));
    }

    // 底部偏移: { "above_bottom": N }
    if (jsonObj.contains("above_bottom") && jsonObj["above_bottom"].is_number_integer()) {
        i32 offset = jsonObj["above_bottom"].get<i32>();
        return std::make_unique<valueprovider::ConstantHeight>(surface::VerticalAnchor::aboveBottom(offset));
    }

    // 顶部偏移: { "below_top": N }
    if (jsonObj.contains("below_top") && jsonObj["below_top"].is_number_integer()) {
        i32 offset = jsonObj["below_top"].get<i32>();
        return std::make_unique<valueprovider::ConstantHeight>(surface::VerticalAnchor::belowTop(offset));
    }

    // 后续可扩展支持其他高度提供者类型：
    // - { "uniform": { "min_inclusive": {...}, "max_inclusive": {...} } }
    // - { "biased_to_bottom": { "min_inclusive": {...}, "max_inclusive": {...} } }
    // - { "very_biased_to_bottom": { "min_inclusive": {...}, "max_inclusive": {...} } }
    // - { "trapezoid": { "min_inclusive": {...}, "max_inclusive": {...}, "plateau": ... } }

    spdlog::warn("Unsupported height provider format, defaulting to absolute 0");
    return std::make_unique<valueprovider::ConstantHeight>(surface::VerticalAnchor::absolute(0));
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
