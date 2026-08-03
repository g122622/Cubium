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

#include "PlacedFeatureLoader.hpp"

#include "BiomeFilterPlacement.hpp"
#include "BlockPredicateFilterPlacement.hpp"
#include "EnvironmentScanPlacement.hpp"
#include "PlacedFeature.hpp"
#include "PlacedFeatureRegistry.hpp"
#include "Placement.hpp"
#include "PlacementRegistry.hpp"
#include "Placements.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include "common/world/gen/feature/parser/BlockPredicateParser.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/valueprovider/HeightProviderParser.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world::gen::placement {

namespace {

/**
 * @brief 剥离 "minecraft:" 命名空间前缀
 */
std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/**
 * @brief 把 MC JSON 的 placement type 名映射到项目 PlacementRegistry 注册名
 *
 * MC 与项目的三处不一致：
 * - MC "in_square"                   → 项目 "square"
 * - MC "biome"                       → 项目 "biome_filter"（MC 的 biome placement 实为 BiomeFilter，
 *                                       项目的 "biome" 是白名单 BiomePlacement，二者语义不同）
 * - MC "surface_water_depth_filter"  → 项目 "water_depth_threshold"（同一算法，注册名不同）
 */
std::string mapPlacementName(const std::string& mcType)
{
    const std::string t = stripNamespace(mcType);
    if (t == "in_square") {
        return "square";
    }
    if (t == "biome") {
        return "biome_filter";
    }
    if (t == "surface_water_depth_filter") {
        return "water_depth_threshold";
    }
    return t;
}

/**
 * @brief 按项目 placement 名构造一个新的 Placement 实例
 *
 * ConfiguredPlacement 拥有 Placement 的所有权，因此不能复用 PlacementRegistry 中
 * 的单例——每条放置链都需各自构造一份 Placement。
 */
std::unique_ptr<Placement> createPlacement(const std::string& name)
{
    if (name == "count") {
        return std::make_unique<CountPlacement>();
    }
    if (name == "height_range") {
        return std::make_unique<HeightRangePlacement>();
    }
    if (name == "square") {
        return std::make_unique<SquarePlacement>();
    }
    if (name == "biome_filter") {
        return std::make_unique<BiomeFilterPlacement>();
    }
    if (name == "biome") {
        return std::make_unique<BiomePlacement>();
    }
    if (name == "chance") {
        return std::make_unique<ChancePlacement>();
    }
    if (name == "surface") {
        return std::make_unique<SurfacePlacement>();
    }
    if (name == "heightmap") {
        return std::make_unique<HeightmapPlacement>();
    }
    if (name == "rarity_filter") {
        return std::make_unique<RarityFilterPlacement>();
    }
    if (name == "block_predicate_filter") {
        return std::make_unique<BlockPredicateFilterPlacement>();
    }
    if (name == "environment_scan") {
        return std::make_unique<EnvironmentScanPlacement>();
    }
    if (name == "random_offset") {
        return std::make_unique<RandomOffsetPlacement>();
    }
    if (name == "water_depth_threshold") {
        return std::make_unique<WaterDepthThresholdPlacement>();
    }
    if (name == "fixed_placement") {
        return std::make_unique<FixedPlacement>();
    }
    if (name == "count_on_every_layer") {
        return std::make_unique<CountOnEveryLayerPlacement>();
    }
    if (name == "noise_threshold_count") {
        return std::make_unique<NoiseThresholdCountPlacement>();
    }
    if (name == "noise_based_count") {
        return std::make_unique<NoiseBasedCountPlacement>();
    }
    if (name == "surface_relative_threshold_filter") {
        return std::make_unique<SurfaceRelativeThresholdFilterPlacement>();
    }
    return nullptr;
}

/**
 * @brief 解析单个 placement 节点为 ConfiguredPlacement
 *
 * 按 type 分派读取对应 config JSON，构造新的 Placement 实例与 config，
 * 组装 ConfiguredPlacement。未知 type 严格报错。
 *
 * @param type 已映射的项目 placement 名（如 "count"/"square"/"height_range"/"biome_filter"）
 * @param node 该 placement 节点的完整 JSON（含 "type"）
 * @param placedFeatureId 当前 placed_feature 的 id（回填给 BiomeFilterConfig）
 */
Result<std::unique_ptr<ConfiguredPlacement>> parsePlacementNode(
    const std::string& type, const nlohmann::json& node, const ResourceLocation& placedFeatureId)
{
    // 严格校验：项目是否注册了该 placement（即是否有 C++ 实现）
    if (!PlacementRegistry::instance().get(type)) {
        return Error(ErrorCode::NotFound,
            "Unregistered placement type: 'minecraft:" + type +
                "'. This placement type has no C++ implementation yet.");
    }

    auto placement = createPlacement(type);
    if (placement == nullptr) {
        return Error(ErrorCode::NotFound,
            "Placement type 'minecraft:" + type + "' registered but has no factory in PlacedFeatureLoader");
    }

    std::unique_ptr<IPlacementConfig> config;

    if (type == "count") {
        // count: 裸整数或 IntProvider
        if (!node.contains("count")) {
            return Error(ErrorCode::InvalidData, "count placement missing 'count' field");
        }
        auto providerResult = valueprovider::IntProviderParser::parse(node["count"]);
        if (!providerResult.success()) {
            return Error(providerResult.error().code(), "count placement: " + providerResult.error().message());
        }
        config = std::make_unique<CountWithProviderConfig>(providerResult.value());
    } else if (type == "square") {
        // in_square: 无 config
        config = std::make_unique<EmptyPlacementConfig>();
    } else if (type == "height_range") {
        // height_range: HeightProvider（在 "height" 字段）
        if (!node.contains("height") || !node["height"].is_object()) {
            return Error(ErrorCode::InvalidData, "height_range placement missing 'height' object");
        }
        auto heightResult = valueprovider::HeightProviderParser::parse(node["height"]);
        if (!heightResult.success()) {
            return Error(heightResult.error().code(), "height_range placement: " + heightResult.error().message());
        }
        config = std::make_unique<HeightProviderPlacementConfig>(heightResult.value());
    } else if (type == "biome_filter") {
        // biome: 无 config。运行时由 BiomeFilterPlacement 反查生物群系是否包含
        // 当前 placed_feature（通过 placedFeatureId）。
        config = std::make_unique<BiomeFilterConfig>(placedFeatureId);
    } else if (type == "block_predicate_filter") {
        // block_predicate_filter: {predicate: <BlockPredicate>}。仅当 basePos 满足谓词才保留该位置。
        if (!node.contains("predicate") || !node["predicate"].is_object()) {
            return Error(ErrorCode::InvalidData, "block_predicate_filter placement missing 'predicate' object");
        }
        auto predResult = feature::parser::BlockPredicateParser::parse(node["predicate"]);
        if (!predResult.success()) {
            return Error(
                predResult.error().code(), "block_predicate_filter placement: " + predResult.error().message());
        }
        config = std::make_unique<BlockPredicateFilterConfig>(predResult.value());
    } else if (type == "environment_scan") {
        // environment_scan: {direction_of_search, target_condition, [allowed_search_condition], max_steps}。
        // 沿 direction_of_search 逐步扫描，找到满足 target_condition 的位置；扫描路径上每格须满足
        // allowed_search_condition（缺省视为 always_true，项目用 nullptr 表示无约束）。
        if (!node.contains("direction_of_search") || !node["direction_of_search"].is_string()) {
            return Error(ErrorCode::InvalidData, "environment_scan placement missing 'direction_of_search' string");
        }
        auto dirOpt = Directions::fromName(node["direction_of_search"].get<std::string>());
        if (!dirOpt.has_value()) {
            return Error(ErrorCode::InvalidData, "environment_scan placement: unknown direction_of_search");
        }
        if (!node.contains("target_condition") || !node["target_condition"].is_object()) {
            return Error(ErrorCode::InvalidData, "environment_scan placement missing 'target_condition' object");
        }
        auto targetResult = feature::parser::BlockPredicateParser::parse(node["target_condition"]);
        if (!targetResult.success()) {
            return Error(targetResult.error().code(),
                "environment_scan placement target_condition: " + targetResult.error().message());
        }
        std::unique_ptr<feature::predicate::BlockPredicate> allowedPred;
        if (node.contains("allowed_search_condition") && node["allowed_search_condition"].is_object()) {
            auto allowedResult = feature::parser::BlockPredicateParser::parse(node["allowed_search_condition"]);
            if (!allowedResult.success()) {
                return Error(allowedResult.error().code(),
                    "environment_scan placement allowed_search_condition: " + allowedResult.error().message());
            }
            allowedPred = allowedResult.value();
        }
        if (!node.contains("max_steps") || !node["max_steps"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "environment_scan placement missing 'max_steps' integer");
        }
        config = std::make_unique<EnvironmentScanConfig>(
            dirOpt.value(), targetResult.value(), std::move(allowedPred), node["max_steps"].get<i32>());
    } else if (type == "random_offset") {
        // random_offset: {xz_spread, y_spread}，二者均为 IntProvider（CODEC 范围 [-16,16]），
        // vanilla JSON 常用裸整数（如 0/-1）。xz_spread 给 dx 与 dz 各独立采样一次，y_spread 给 dy。
        if (!node.contains("xz_spread")) {
            return Error(ErrorCode::InvalidData, "random_offset placement missing 'xz_spread' field");
        }
        if (!node.contains("y_spread")) {
            return Error(ErrorCode::InvalidData, "random_offset placement missing 'y_spread' field");
        }
        auto xzResult = valueprovider::IntProviderParser::parse(node["xz_spread"], -16, 16);
        if (!xzResult.success()) {
            return Error(xzResult.error().code(), "random_offset placement xz_spread: " + xzResult.error().message());
        }
        auto yResult = valueprovider::IntProviderParser::parse(node["y_spread"], -16, 16);
        if (!yResult.success()) {
            return Error(yResult.error().code(), "random_offset placement y_spread: " + yResult.error().message());
        }
        config = std::make_unique<RandomOffsetConfig>(xzResult.value(), yResult.value());
    } else if (type == "water_depth_threshold") {
        // surface_water_depth_filter: {max_water_depth}（裸整数）。
        // 仅当当前位置列的水柱深度 <= max_water_depth 才保留该位置。
        if (!node.contains("max_water_depth") || !node["max_water_depth"].is_number_integer()) {
            return Error(
                ErrorCode::InvalidData, "surface_water_depth_filter placement missing 'max_water_depth' integer");
        }
        config = std::make_unique<WaterDepthThresholdConfig>(node["max_water_depth"].get<i32>());
    } else if (type == "rarity_filter") {
        // rarity_filter: {chance}（裸整数，POSITIVE_INT 即 >=1）。以 1/chance 概率通过。
        if (!node.contains("chance") || !node["chance"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "rarity_filter placement missing 'chance' integer");
        }
        const i32 chance = node["chance"].get<i32>();
        if (chance < 1) {
            return Error(ErrorCode::InvalidData,
                "rarity_filter placement 'chance' must be >= 1 (POSITIVE_INT), got " + std::to_string(chance));
        }
        config = std::make_unique<RarityFilterConfig>(chance);
    } else if (type == "heightmap") {
        // heightmap: {heightmap}（全大写枚举字符串）。用配置的高度图类型查 (x,z) 列最高方块 Y。
        if (!node.contains("heightmap") || !node["heightmap"].is_string()) {
            return Error(ErrorCode::InvalidData, "heightmap placement missing 'heightmap' string");
        }
        const auto heightmapOpt = world::chunk::heightmapTypeFromString(node["heightmap"].get<std::string>());
        if (!heightmapOpt.has_value()) {
            return Error(ErrorCode::InvalidData,
                "heightmap placement: unknown heightmap type '" + node["heightmap"].get<std::string>() + "'");
        }
        config = std::make_unique<HeightmapPlacementConfig>(heightmapOpt.value());
    } else if (type == "fixed_placement") {
        // fixed_placement: {positions: [[x,y,z], ...]}。仅当 basePos 所在区块含这些坐标时返回它们。
        if (!node.contains("positions") || !node["positions"].is_array()) {
            return Error(ErrorCode::InvalidData, "fixed_placement placement missing 'positions' array");
        }
        std::vector<BlockPos> positions;
        positions.reserve(node["positions"].size());
        for (const auto& entry : node["positions"]) {
            if (!entry.is_array() || entry.size() != 3 || !entry[0].is_number_integer() ||
                !entry[1].is_number_integer() || !entry[2].is_number_integer()) {
                return Error(
                    ErrorCode::InvalidData, "fixed_placement placement: each position must be [x,y,z] integers");
            }
            positions.emplace_back(entry[0].get<i32>(), entry[1].get<i32>(), entry[2].get<i32>());
        }
        config = std::make_unique<FixedPlacementConfig>(std::move(positions));
    } else if (type == "count_on_every_layer") {
        // count_on_every_layer: {count: IntProvider(0,256) 或裸整数}。
        if (!node.contains("count")) {
            return Error(ErrorCode::InvalidData, "count_on_every_layer placement missing 'count' field");
        }
        auto providerResult = valueprovider::IntProviderParser::parse(node["count"], 0, 256);
        if (!providerResult.success()) {
            return Error(
                providerResult.error().code(), "count_on_every_layer placement: " + providerResult.error().message());
        }
        config = std::make_unique<CountOnEveryLayerConfig>(providerResult.value());
    } else if (type == "noise_threshold_count") {
        // noise_threshold_count: {noise_level: double, below_noise: int, above_noise: int}。
        if (!node.contains("noise_level") || !node["noise_level"].is_number()) {
            return Error(ErrorCode::InvalidData, "noise_threshold_count placement missing 'noise_level' number");
        }
        if (!node.contains("below_noise") || !node["below_noise"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "noise_threshold_count placement missing 'below_noise' integer");
        }
        if (!node.contains("above_noise") || !node["above_noise"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "noise_threshold_count placement missing 'above_noise' integer");
        }
        config = std::make_unique<NoiseThresholdCountConfig>(
            node["noise_level"].get<f64>(), node["below_noise"].get<i32>(), node["above_noise"].get<i32>());
    } else if (type == "noise_based_count") {
        // noise_based_count: {noise_to_count_ratio: int, noise_factor: double, noise_offset: double (缺省 0)}。
        if (!node.contains("noise_to_count_ratio") || !node["noise_to_count_ratio"].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "noise_based_count placement missing 'noise_to_count_ratio' integer");
        }
        if (!node.contains("noise_factor") || !node["noise_factor"].is_number()) {
            return Error(ErrorCode::InvalidData, "noise_based_count placement missing 'noise_factor' number");
        }
        const f64 noiseOffset = node.value("noise_offset", 0.0);
        config = std::make_unique<NoiseBasedCountConfig>(
            node["noise_to_count_ratio"].get<i32>(), node["noise_factor"].get<f64>(), noiseOffset);
    } else if (type == "surface_relative_threshold_filter") {
        // surface_relative_threshold_filter: {heightmap, min_inclusive? int, max_inclusive? int}。
        if (!node.contains("heightmap") || !node["heightmap"].is_string()) {
            return Error(
                ErrorCode::InvalidData, "surface_relative_threshold_filter placement missing 'heightmap' string");
        }
        const auto heightmapOpt = world::chunk::heightmapTypeFromString(node["heightmap"].get<std::string>());
        if (!heightmapOpt.has_value()) {
            return Error(ErrorCode::InvalidData,
                "surface_relative_threshold_filter placement: unknown heightmap type '" +
                    node["heightmap"].get<std::string>() + "'");
        }
        const i32 minInclusive = node.value("min_inclusive", std::numeric_limits<i32>::min());
        const i32 maxInclusive = node.value("max_inclusive", std::numeric_limits<i32>::max());
        config =
            std::make_unique<SurfaceRelativeThresholdFilterConfig>(heightmapOpt.value(), minInclusive, maxInclusive);
    } else {
        // chance/surface 是项目自造的遗留 placement（MC 1.21.11 无此 type，原版数据包不会触发），
        // 保留其工厂以兼容潜在第三方数据包，config 用空实现兜底。其余未注册 type 已由
        // PlacementRegistry::get 在入口拦截，不会走到这里。
        config = std::make_unique<EmptyPlacementConfig>();
    }

    return std::make_unique<ConfiguredPlacement>(std::move(placement), std::move(config));
}

} // namespace

Result<std::unique_ptr<ConfiguredPlacement>> PlacedFeatureLoader::parsePlacementChain(
    const nlohmann::json& placementArr, const ResourceLocation& placedFeatureId)
{
    if (!placementArr.is_array()) {
        return Error(ErrorCode::InvalidData, "placed_feature 'placement' must be an array");
    }
    // 空 placement 链：返回恒等放置器（在 origin 处放置），对齐 MC 空 placement 修饰符列表语义。
    // 用于内联 PlacedFeature（random_patch/simple_random_selector 的 features[] 项）placement 为空的情形。
    if (placementArr.empty()) {
        return std::make_unique<ConfiguredPlacement>(
            std::make_unique<IdentityPlacement>(), std::make_unique<EmptyPlacementConfig>());
    }

    std::unique_ptr<ConfiguredPlacement> head;
    ConfiguredPlacement* tail = nullptr;

    for (const auto& node : placementArr) {
        if (!node.is_object() || !node.contains("type") || !node["type"].is_string()) {
            return Error(ErrorCode::InvalidData, "placement entry missing 'type' string");
        }
        const std::string mapped = mapPlacementName(node["type"].get<std::string>());

        auto nodeResult = parsePlacementNode(mapped, node, placedFeatureId);
        if (!nodeResult.success()) {
            return Error(nodeResult.error().code(), nodeResult.error().message());
        }

        auto configured = nodeResult.value();
        ConfiguredPlacement* raw = configured.get();
        if (head == nullptr) {
            head = std::move(configured);
            tail = head.get();
        } else {
            tail->setNext(std::move(configured));
            tail = raw;
        }
    }

    return head;
}

Result<std::unique_ptr<PlacedFeature>> PlacedFeatureLoader::loadFromJson(
    const nlohmann::json& jsonObj, const ResourceLocation& id)
{
    // feature 字段：configured_feature 的 ResourceLocation 字符串
    if (!jsonObj.contains("feature") || !jsonObj["feature"].is_string()) {
        return Error(ErrorCode::InvalidData, "placed_feature '" + id.toString() + "' missing 'feature' string");
    }
    const ResourceLocation featureId = ResourceLocation::parse(jsonObj["feature"].get<std::string>());
    const ConfiguredFeatureBase* feature = ConfiguredFeatureRegistry::instance().get(featureId);
    if (feature == nullptr) {
        return Error(ErrorCode::NotFound,
            "placed_feature '" + id.toString() + "' references unregistered configured_feature '" +
                featureId.toString() + "'");
    }

    // placement 数组
    if (!jsonObj.contains("placement") || !jsonObj["placement"].is_array()) {
        return Error(ErrorCode::InvalidData, "placed_feature '" + id.toString() + "' missing 'placement' array");
    }
    auto chainResult = parsePlacementChain(jsonObj["placement"], id);
    if (!chainResult.success()) {
        return Error(
            chainResult.error().code(), "placed_feature '" + id.toString() + "': " + chainResult.error().message());
    }

    return std::make_unique<PlacedFeature>(feature, chainResult.value(), id);
}

namespace {

/**
 * @brief 从资源路径推导 ResourceLocation
 *
 * 路径格式: <namespace>/worldgen/placed_feature/<path>.json
 * ResourceLocation = <namespace>:<path>（去 .json）
 */
ResourceLocation locationFromResourcePath(const std::string& ns, const std::string& directory, const std::string& path)
{
    std::string name = path.substr(directory.length() + 1); // +1 跳过 '/'
    if (name.size() >= 5 && name.substr(name.size() - 5) == ".json") {
        name = name.substr(0, name.size() - 5);
    }
    return ResourceLocation(ns, name);
}

} // namespace

Result<size_t> PlacedFeatureLoader::loadFromDataPackRepository(const resource::DataPackRepository& repo)
{
    size_t loadedCount = 0;
    // 清空旧数据，保证重复加载（重启世界/测试 fixture 复用）不累积重复条目
    PlacedFeatureRegistry::instance().clear();

    auto namespacesResult = repo.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/worldgen/placed_feature";
        auto listResult = repo.listResources(directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation id = locationFromResourcePath(ns, directory, resourcePath);

            auto readResult = repo.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read placed_feature: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse placed_feature {}: {}", id.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, id);
            if (!parseResult.success()) {
                spdlog::warn("Failed to load placed_feature {}: {}", id.toString(), parseResult.error().message());
                continue;
            }

            PlacedFeatureRegistry::instance().registerPlacedFeature(parseResult.value());
            ++loadedCount;
        }
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} placed_features from datapacks", loadedCount);
    }
    return loadedCount;
}

Result<size_t> PlacedFeatureLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;
    // 清空旧数据，保证重复加载不累积重复条目
    PlacedFeatureRegistry::instance().clear();

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/worldgen/placed_feature";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation id = locationFromResourcePath(ns, directory, resourcePath);

            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read placed_feature: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse placed_feature {}: {}", id.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, id);
            if (!parseResult.success()) {
                spdlog::warn("Failed to load placed_feature {}: {}", id.toString(), parseResult.error().message());
                continue;
            }

            PlacedFeatureRegistry::instance().registerPlacedFeature(parseResult.value());
            ++loadedCount;
        }
    }

    return loadedCount;
}

} // namespace world::gen::placement
} // namespace mc
