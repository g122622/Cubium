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
#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/Direction.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include "common/world/gen/feature/parser/BlockPredicateParser.hpp"
#include "common/world/gen/valueprovider/HeightProviderParser.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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
 * MC 与项目的两处不一致：
 * - MC "in_square"  → 项目 "square"
 * - MC "biome"      → 项目 "biome_filter"（MC 的 biome placement 实为 BiomeFilter，
 *                     项目的 "biome" 是白名单 BiomePlacement，二者语义不同）
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
    } else {
        // TODO: 其余 placement type（chance/rarity_filter/heightmap/surface 等）的 JSON config
        // 解析尚未实现。这些 type 在 MC 中各有自己的 config（如 chance 需 IntProvider、
        // heightmap 需高度图类型枚举、surface 需水深阈值），当前一律回退到 EmptyPlacementConfig，
        // 导致其修饰行为静默丢失（仅靠 Placement 默认实现兜底）。待对应 feature type 落地时
        // 在此分支逐 type 补全 config 解析。
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
