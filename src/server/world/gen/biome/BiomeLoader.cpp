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

#include "BiomeLoader.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/attribute/EnvironmentAttributeMap.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/biome/BiomeGenerationSettings.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include "common/world/spawn/MobSpawnInfoJson.hpp"
#include "server/world/gen/carver/ConfiguredCarverRegistry.hpp"
#include "server/world/gen/feature/vegetation/FlowerFeature.hpp"
#include "server/world/gen/placement/PlacedFeature.hpp"
#include "server/world/gen/placement/PlacedFeatureRegistry.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world::biome {

namespace {

// ============================================================================
// 数据包 biome 名 → BiomeId 映射表
// ============================================================================
//
// 数据包 biome JSON 文件名（如 "plains"、"stony_shore"）需映射到项目内部 BiomeId。
// 大部分名字与 BiomeIds.hpp 的常量名（snake_case）一一对应。
// 11 个 1.18+ 重命名的 biome 在 BiomeIds.hpp 中已有别名常量（如 StonyShore = StoneShore），
// 这里直接使用别名指向的真实 BiomeId，避免依赖 Biome 内部 m_name（部分老 biome 的
// m_name 仍是 1.16.5 旧名，如 "stone_shore"）。
//
// 若数据包中存在映射表里没有的 biome 名，loadFromJson 会 warn + skip。

const std::unordered_map<std::string, BiomeId>& biomeNameToIdMap()
{
    static const std::unordered_map<std::string, BiomeId> map = {
        // 基础生物群系 (0-13)
        {"ocean", Biomes::Ocean},
        {"plains", Biomes::Plains},
        {"desert", Biomes::Desert},
        {"forest", Biomes::Forest},
        {"taiga", Biomes::Taiga},
        {"swamp", Biomes::Swamp},
        {"river", Biomes::River},
        {"nether_wastes", Biomes::NetherWastes},
        {"the_end", Biomes::TheEnd},
        {"frozen_ocean", Biomes::FrozenOcean},
        {"frozen_river", Biomes::FrozenRiver},
        {"snowy_plains", Biomes::SnowyPlains},
        // 蘑菇岛
        {"mushroom_fields", Biomes::MushroomFields},
        // 海滩
        {"beach", Biomes::Beach},
        {"snowy_beach", Biomes::SnowyBeach},
        // 桦木森林
        {"birch_forest", Biomes::BirchForest},
        {"dark_forest", Biomes::DarkForest},
        // 雪地针叶林
        {"snowy_taiga", Biomes::SnowyTaiga},
        // 热带草原
        {"savanna", Biomes::Savanna},
        {"savanna_plateau", Biomes::SavannaPlateau},
        // 恶地
        {"badlands", Biomes::Badlands},
        {"eroded_badlands", Biomes::ErodedBadlands},
        // 末地生物群系
        {"small_end_islands", Biomes::SmallEndIslands},
        {"end_midlands", Biomes::EndMidlands},
        {"end_highlands", Biomes::EndHighlands},
        {"end_barrens", Biomes::EndBarrens},
        // 海洋温度变体
        {"warm_ocean", Biomes::WarmOcean},
        {"lukewarm_ocean", Biomes::LukewarmOcean},
        {"cold_ocean", Biomes::ColdOcean},
        {"deep_warm_ocean", Biomes::DeepWarmOcean},
        {"deep_lukewarm_ocean", Biomes::DeepLukewarmOcean},
        {"deep_cold_ocean", Biomes::DeepColdOcean},
        {"deep_frozen_ocean", Biomes::DeepFrozenOcean},
        {"deep_ocean", Biomes::DeepOcean},
        // 虚空
        {"the_void", Biomes::TheVoid},
        // 变体生物群系
        {"sunflower_plains", Biomes::SunflowerPlains},
        {"flower_forest", Biomes::FlowerForest},
        {"ice_spikes", Biomes::IceSpikes},
        {"tall_birch_forest", Biomes::TallBirchForest},
        // 下界生物群系
        {"soul_sand_valley", Biomes::SoulSandValley},
        {"crimson_forest", Biomes::CrimsonForest},
        {"warped_forest", Biomes::WarpedForest},
        {"basalt_deltas", Biomes::BasaltDeltas},
        // 新增生物群系
        {"meadow", Biomes::Meadow},
        {"grove", Biomes::Grove},
        {"snowy_slopes", Biomes::SnowySlopes},
        {"jagged_peaks", Biomes::JaggedPeaks},
        {"frozen_peaks", Biomes::FrozenPeaks},
        {"stony_peaks", Biomes::StonyPeaks},
        {"dripstone_caves", Biomes::DripstoneCaves},
        {"lush_caves", Biomes::LushCaves},
        {"mangrove_swamp", Biomes::MangroveSwamp},
        {"cherry_grove", Biomes::CherryGrove},
        {"pale_garden", Biomes::PaleGarden},
        // 丛林
        {"jungle", Biomes::Jungle},
        {"bamboo_jungle", Biomes::BambooJungle},
        // 1.18+ 重命名 biome（11 项，使用 BiomeIds.hpp 的别名常量）
        {"stony_shore", Biomes::StonyShore},                          // = StoneShore (25)
        {"old_growth_pine_taiga", Biomes::OldGrowthPineTaiga},        // = GiantTreeTaiga (32)
        {"old_growth_spruce_taiga", Biomes::OldGrowthSpruceTaiga},    // = GiantSpruceTaiga (160)
        {"old_growth_birch_forest", Biomes::OldGrowthBirchForest},    // = TallBirchForest (155)
        {"sparse_jungle", Biomes::SparseJungle},                      // = JungleEdge (23)
        {"windswept_hills", Biomes::WindsweptHills},                  // = Mountains (3)
        {"windswept_forest", Biomes::WindsweptForest},                // = WoodedMountains (34)
        {"windswept_gravelly_hills", Biomes::WindsweptGravellyHills}, // = GravellyMountains (131)
        {"windswept_savanna", Biomes::WindsweptSavanna},              // = ShatteredSavanna (163)
        {"wooded_badlands", Biomes::WoodedBadlands},                  // = WoodedBadlandsPlateau (38)
        {"deep_dark", Biomes::DeepDark},                              // = 182
    };
    return map;
}

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 从资源路径推导 ResourceLocation
 *
 * 路径格式: <namespace>/worldgen/biome/<path>.json
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

/**
 * @brief 解析 "#RRGGBB" 十六进制颜色字符串为 u32
 *
 * biome JSON 的颜色字段（sky_color/water_color/fog_color 等）使用 "#RRGGBB" 格式。
 * 也兼容无 # 前缀的纯十六进制。
 */
Result<u32> parseColor(const std::string& colorStr)
{
    std::string hex = colorStr;
    if (!hex.empty() && hex[0] == '#') {
        hex = hex.substr(1);
    }
    if (hex.size() != 6) {
        return Error(ErrorCode::InvalidData, "invalid color string: " + colorStr);
    }
    try {
        return static_cast<u32>(std::stoul(hex, nullptr, 16));
    }
    catch (...) {
        return Error(ErrorCode::InvalidData, "invalid color string: " + colorStr);
    }
}

/**
 * @brief 安全读取 JSON 字符串字段
 */
std::optional<std::string> readString(const nlohmann::json& obj, const std::string& key)
{
    if (obj.contains(key) && obj[key].is_string()) {
        return obj[key].get<std::string>();
    }
    return std::nullopt;
}

/**
 * @brief 安全读取 JSON 浮点字段
 */
std::optional<f32> readF32(const nlohmann::json& obj, const std::string& key)
{
    if (obj.contains(key) && obj[key].is_number()) {
        return obj[key].get<f32>();
    }
    return std::nullopt;
}

/**
 * @brief 安全读取 JSON 布尔字段
 */
std::optional<bool> readBool(const nlohmann::json& obj, const std::string& key)
{
    if (obj.contains(key) && obj[key].is_boolean()) {
        return obj[key].get<bool>();
    }
    return std::nullopt;
}

// ============================================================================
// climate 解析
// ============================================================================

/**
 * @brief 解析 temperature_modifier 字符串为枚举
 */
BiomeClimate::TemperatureModifier parseTemperatureModifier(const std::string& modifierStr)
{
    if (modifierStr == "frozen") {
        return BiomeClimate::TemperatureModifier::Frozen;
    }
    return BiomeClimate::TemperatureModifier::None;
}

void applyClimate(Biome& biome, const nlohmann::json& jsonObj)
{
    // temperature / downfall / has_precipitation / temperature_modifier
    // 直接通过 Biome 的 setter 叠加（不重置 humidity/continentalness/erosion，
    // 这些字段不在 biome JSON 中，由 BiomeFactory 已经设置好）
    if (auto temp = readF32(jsonObj, "temperature")) {
        biome.setTemperature(*temp);
    }
    if (auto downfall = readF32(jsonObj, "downfall")) {
        biome.setDownfall(*downfall);
    }
    if (auto hasPrecip = readBool(jsonObj, "has_precipitation")) {
        biome.setHasPrecipitation(*hasPrecip);
    }
    if (auto modifierStr = readString(jsonObj, "temperature_modifier")) {
        biome.setTemperatureModifier(parseTemperatureModifier(*modifierStr));
    }
}

// ============================================================================
// attributes 解析（vanilla 1.21.11 群系 JSON 顶层 attributes 字段）
// ============================================================================
//
// 对齐 vanilla EnvironmentAttributes：群系 JSON 的 attributes 对象以 "minecraft:<路径>" 为键
// 存储环境属性（visual/*、audio/*、gameplay/* 等）。每个条目的写法为：
//   1. 直接给出参数（等价于 override），或
//   2. {"modifier": "<操作名>", "argument": <参数>}，按属性类型允许的修改器应用。
// 属性名解析、参数解析与修改器应用统一由 EnvironmentAttributeMap::Builder::setFromJson 完成，
// 解析出的属性表挂到 Biome::attributes()，再由维度级环境属性系统的群系位置层按位置消费。
// 未注册的属性、不允许的修改器、无法解析的参数均告警并跳过该条目（与 vanilla 数据包容错一致）。

void applyAttributes(Biome& biome, const nlohmann::json& jsonObj)
{
    if (!jsonObj.contains("attributes") || !jsonObj["attributes"].is_object()) {
        return; // attributes 可选
    }

    attribute::EnvironmentAttributeMap::Builder builder;
    for (const auto& [name, argument] : jsonObj["attributes"].items()) {
        if (Result<void> result = builder.setFromJson(name, argument); !result.success()) {
            spdlog::warn(
                "biome '{}': skipping environment attribute '{}': {}", biome.name(), name, result.error().message());
        }
    }
    attribute::EnvironmentAttributeMap attributes = builder.build();

    // 兼容旧字段：火焰加速熄灭标志曾以独立的 Biome 字段承载并直接被 FireBlock 消费，
    // 现由环境属性表驱动；这里同步该字段，保证无维度上下文的查询路径行为不变。
    bool increasedFireBurnout = false;
    if (attributes.contains(&attribute::EnvironmentAttributes::INCREASED_FIRE_BURNOUT())) {
        attributes.applyModifier(&attribute::EnvironmentAttributes::INCREASED_FIRE_BURNOUT(), &increasedFireBurnout);
    }
    biome.setIncreasedFireBurnout(increasedFireBurnout);

    biome.setAttributes(std::move(attributes));
}

// ============================================================================
// effects 解析
// ============================================================================

void applyEffects(Biome& biome, const nlohmann::json& jsonObj)
{
    if (!jsonObj.contains("effects") || !jsonObj["effects"].is_object()) {
        return; // effects 可选
    }
    const auto& effectsJson = jsonObj["effects"];
    BiomeEffects::Builder builder;

    // sky_color / fog_color / water_color / water_fog_color
    // foliage_color / grass_color / grass_color_modifier / dry_foliage_color
    if (auto skyColorStr = readString(effectsJson, "sky_color")) {
        if (auto color = parseColor(*skyColorStr); color.success()) {
            builder.skyColor(color.value());
        }
    }
    if (auto fogColorStr = readString(effectsJson, "fog_color")) {
        if (auto color = parseColor(*fogColorStr); color.success()) {
            builder.fogColor(color.value());
        }
    }
    if (auto waterColorStr = readString(effectsJson, "water_color")) {
        if (auto color = parseColor(*waterColorStr); color.success()) {
            builder.waterColor(color.value());
        }
    }
    if (auto waterFogColorStr = readString(effectsJson, "water_fog_color")) {
        if (auto color = parseColor(*waterFogColorStr); color.success()) {
            builder.waterFogColor(color.value());
        }
    }
    if (auto foliageColorStr = readString(effectsJson, "foliage_color")) {
        if (auto color = parseColor(*foliageColorStr); color.success()) {
            builder.foliageColor(color.value());
        }
    }
    if (auto grassColorStr = readString(effectsJson, "grass_color")) {
        if (auto color = parseColor(*grassColorStr); color.success()) {
            builder.grassColor(color.value());
        }
    }
    if (auto dryFoliageColorStr = readString(effectsJson, "dry_foliage_color")) {
        if (auto color = parseColor(*dryFoliageColorStr); color.success()) {
            builder.dryFoliageColor(color.value());
        }
    }
    if (auto modifierStr = readString(effectsJson, "grass_color_modifier")) {
        if (*modifierStr == "swamp") {
            builder.grassColorModifier(GrassColorModifier::Swamp);
        } else if (*modifierStr == "dark_forest") {
            builder.grassColorModifier(GrassColorModifier::DarkForest);
        }
        // "none" 或其他 → 保持默认 None
    }
    biome.setEffects(builder.build());
}

// ============================================================================
// spawners 解析
// ============================================================================


// ============================================================================
// spawn_costs 解析
// ============================================================================

/**
 * @brief 用数据包的 spawners / creature_spawn_probability 重建生成信息
 *
 * 不做部分合并：`spawners` 出现时整体替换（`BiomeFactory` 的默认生成表随之作废）。
 * 动物生成概率缺省时保留工厂方法的设定（雪原等少数群系不是默认的 0.1）。
 * 解码失败（未知分类 / 未注册实体 / 非法数量）与原版一样视为**该群系加载失败**：整份
 * JSON 都会回滚（`loadFromJson` 在调用前保存了原始 `MobSpawnInfo`），不保留半套数据。
 */
Result<void> applySpawners(Biome& biome, const nlohmann::json& jsonObj)
{
    if (!jsonObj.contains("spawners") && !jsonObj.contains("creature_spawn_probability")) {
        return {}; // 两个字段都可选
    }
    std::optional<nlohmann::json> probability;
    if (jsonObj.contains("creature_spawn_probability")) {
        probability = jsonObj["creature_spawn_probability"];
    }
    const nlohmann::json spawners = jsonObj.contains("spawners") ? jsonObj["spawners"] : nlohmann::json();
    auto result = world::spawn::mobSpawnInfoFromJson(
        spawners, probability, biome.spawnInfo().getCreatureSpawnProbability());
    if (result.failed()) {
        return Error(ErrorCode::InvalidData, "biome '" + biome.name() + "': " + result.error().message());
    }
    biome.setSpawnInfo(std::move(result).value());
    return {};
}

/**
 * @brief 把 `spawn_costs` 并入群系的生成信息
 *
 * 与原版一样是同一个 `MobSpawnSettings` 的一部分：解码失败同样视为该群系加载失败。
 */
Result<void> applySpawnCosts(Biome& biome, const nlohmann::json& jsonObj)
{
    if (!jsonObj.contains("spawn_costs")) {
        return {}; // spawn_costs 可选（多数 biome 为空对象 {}）
    }
    auto result = world::spawn::applySpawnCosts(biome.spawnInfo(), jsonObj["spawn_costs"]);
    if (result.failed()) {
        return Error(ErrorCode::InvalidData, "biome '" + biome.name() + "': " + result.error().message());
    }
    return {};
}

// ============================================================================
// features 解析
// ============================================================================

/**
 * @brief 解析 features 二维数组并叠加到 BiomeGenerationSettings
 *
 * features 是一个数组，每个元素是一个 placed_feature id 字符串数组。
 * 数组索引对应 DecorationStage 枚举值（0=RawGeneration, 1=Lakes, ..., 10=TopLayerModification）。
 * 缺失的 placed_feature id 在 PlacedFeatureRegistry 中查不到时 warn + skip。
 *
 * 花卉列表填充：对每个 placed_feature，若其底层 configured_feature 为
 * ConfiguredFlowerFeature（对应 MC 的 Feature.FLOWER），则同时通过 addFlowerFeature
 * 登记到 BiomeGenerationSettings 的独立花卉列表。GrassBlock::grow 骨粉催花时从该列表
 * 随机选取 placed_feature，通过 PlacedFeatureRegistry 解析后取 feature() 拿到
 * ConfiguredFlowerFeature，再从其配置中随机选择花朵方块状态放置。
 */
void applyFeatures(Biome& biome, const nlohmann::json& jsonObj)
{
    if (!jsonObj.contains("features") || !jsonObj["features"].is_array()) {
        return; // features 可选
    }
    const auto& featuresJson = jsonObj["features"];
    auto& genSettings = biome.generationSettings();
    const auto& placedRegistry = PlacedFeatureRegistry::instance();

    for (size_t stageIdx = 0; stageIdx < featuresJson.size(); ++stageIdx) {
        if (stageIdx >= static_cast<size_t>(DecorationStage::Count)) {
            spdlog::warn("features stage index {} out of range (max {}), ignoring",
                stageIdx,
                static_cast<i32>(DecorationStage::Count) - 1);
            break;
        }
        const auto& stageArr = featuresJson[stageIdx];
        if (!stageArr.is_array()) {
            continue;
        }
        const DecorationStage stage = static_cast<DecorationStage>(stageIdx);
        for (const auto& featureIdJson : stageArr) {
            if (!featureIdJson.is_string()) {
                continue;
            }
            const std::string featureIdStr = featureIdJson.get<std::string>();
            const ResourceLocation featureId = ResourceLocation::parse(featureIdStr);

            // 校验 placed_feature 是否已注册；未注册则 warn + skip（世界仍可生成）
            const PlacedFeature* placedFeature = placedRegistry.get(featureId);
            if (placedFeature == nullptr) {
                spdlog::warn(
                    "biome '{}' references unregistered placed_feature '{}', skipping", biome.name(), featureIdStr);
                continue;
            }
            genSettings.addPlacedFeature(stage, featureId);

            // 花卉 placed_feature 同时登记到独立花卉列表，供 GrassBlock::grow 骨粉催花使用
            if (dynamic_cast<const ConfiguredFlowerFeature*>(placedFeature->feature()) != nullptr) {
                genSettings.addFlowerFeature(featureId);
            }
        }
    }
}

// ============================================================================
// carvers 解析
// ============================================================================

/**
 * @brief 解析 carvers 字段并叠加到 BiomeGenerationSettings
 *
 * carvers 字段有两种形态：
 * 1. 字符串: "minecraft:nether_cave"（单个 carver）
 * 2. 字符串数组: ["minecraft:cave", "minecraft:canyon"]
 *
 * 所有形态的 carver id 都通过 addCarver 添加（项目当前不区分 air/liquid）。
 * 未在 ConfiguredCarverRegistry 注册的 carver id → warn + skip。
 */
void applyCarvers(Biome& biome, const nlohmann::json& jsonObj)
{
    if (!jsonObj.contains("carvers")) {
        return; // carvers 可选
    }
    const auto& carversJson = jsonObj["carvers"];
    auto& genSettings = biome.generationSettings();

    // 收集所有 carver id 字符串
    std::vector<std::string> carverIds;
    if (carversJson.is_string()) {
        carverIds.push_back(carversJson.get<std::string>());
    } else if (carversJson.is_array()) {
        for (const auto& c : carversJson) {
            if (c.is_string()) {
                carverIds.push_back(c.get<std::string>());
            }
        }
    }

    for (const auto& carverIdStr : carverIds) {
        const ResourceLocation carverId = ResourceLocation::parse(carverIdStr);
        // 校验 configured_carver 是否已注册；未注册则 warn + skip
        if (ConfiguredCarverRegistry::instance().get(carverId) == nullptr) {
            spdlog::warn(
                "biome '{}' references unregistered configured_carver '{}', skipping", biome.name(), carverIdStr);
            continue;
        }
        genSettings.addCarver(carverId);
    }
}

} // namespace

// ============================================================================
// 公共 API
// ============================================================================

Result<size_t> BiomeLoader::loadFromDataPackRepository(const resource::DataPackRepository& repo)
{
    size_t loadedCount = 0;

    auto namespacesResult = repo.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/worldgen/biome";
        auto listResult = repo.listResources(directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation id = locationFromResourcePath(ns, directory, resourcePath);

            auto readResult = repo.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read biome: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse biome {}: {}", id.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, id);
            if (parseResult.success()) {
                ++loadedCount;
            }
            // loadFromJson 内部已对可恢复错误 warn + skip，不在此重复日志
        }
    }

    if (loadedCount > 0) {
        spdlog::info("Loaded {} biomes from datapacks", loadedCount);
    }
    return loadedCount;
}

Result<size_t> BiomeLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    size_t loadedCount = 0;

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        return loadedCount;
    }

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/worldgen/biome";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }

        for (const auto& resourcePath : listResult.value()) {
            const ResourceLocation id = locationFromResourcePath(ns, directory, resourcePath);

            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read biome: {}", resourcePath);
                continue;
            }

            nlohmann::json jsonObj;
            try {
                jsonObj = nlohmann::json::parse(readResult.value());
            }
            catch (const nlohmann::json::parse_error& e) {
                spdlog::warn("Failed to parse biome {}: {}", id.toString(), e.what());
                continue;
            }

            auto parseResult = loadFromJson(jsonObj, id);
            if (parseResult.success()) {
                ++loadedCount;
            }
        }
    }

    return loadedCount;
}

Result<void> BiomeLoader::loadFromJson(const nlohmann::json& jsonObj, const ResourceLocation& id)
{
    if (!jsonObj.is_object()) {
        return Error(ErrorCode::InvalidData, "biome '" + id.toString() + "' JSON is not an object");
    }

    // 通过 datapack biome path（如 "plains"、"stony_shore"）查找 BiomeId
    const std::string& biomeName = id.path();
    const auto& nameMap = biomeNameToIdMap();
    auto it = nameMap.find(biomeName);
    if (it == nameMap.end()) {
        // 数据包中存在映射表里没有的 biome 名 → warn + skip（项目可能未注册该 biome）
        spdlog::warn("biome '{}' has no BiomeId mapping, skipping", id.toString());
        return Result<void>::ok();
    }

    const BiomeId biomeId = it->second;
    auto& registry = BiomeRegistry::instance();
    if (!registry.hasBiome(biomeId)) {
        // BiomeId 存在但 BiomeRegistry 未注册该 biome → warn + skip
        spdlog::warn(
            "biome '{}' (id={}) not registered in BiomeRegistry, skipping", id.toString(), static_cast<i32>(biomeId));
        return Result<void>::ok();
    }

    Biome& biome = registry.getMutable(biomeId);

    // 叠加 JSON 字段到已有的 Biome 对象（保留 BiomeFactory 设置的 depth/scale/surface blocks 等）
    applyClimate(biome, jsonObj);
    applyEffects(biome, jsonObj);
    applyAttributes(biome, jsonObj);
    // 生成表是整体替换：解码失败时整份群系加载失败，不保留「部分字段已覆盖」的半套数据。
    auto spawnerResult = applySpawners(biome, jsonObj);
    if (spawnerResult.failed()) {
        return spawnerResult.error();
    }
    auto costResult = applySpawnCosts(biome, jsonObj);
    if (costResult.failed()) {
        return costResult.error();
    }

    // 生成设置覆盖：先清空 BiomeFactory 可能设置的旧数据，再从 JSON 填充
    biome.generationSettings().clear();
    applyCarvers(biome, jsonObj);
    applyFeatures(biome, jsonObj);

    return Result<void>::ok();
}

std::optional<BiomeId> BiomeLoader::biomeIdByName(const ResourceLocation& name)
{
    const auto& nameMap = biomeNameToIdMap();
    const auto it = nameMap.find(name.path());
    if (it == nameMap.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace world::biome
} // namespace mc
