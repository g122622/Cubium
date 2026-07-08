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

#include "FeatureTypeRegistry.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/BasaltPillarFeature.hpp"
#include "common/world/gen/feature/BlockBlobFeature.hpp"
#include "common/world/gen/feature/BlockColumnFeature.hpp"
#include "common/world/gen/feature/BonusChestFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/EndPlatformFeature.hpp"
#include "common/world/gen/feature/LakeFeature.hpp"
#include "common/world/gen/feature/MonsterRoomFeature.hpp"
#include "common/world/gen/feature/RandomBooleanSelectorFeature.hpp"
#include "common/world/gen/feature/RandomPatchFeature.hpp"
#include "common/world/gen/feature/RandomSelectorFeature.hpp"
#include "common/world/gen/feature/SimpleBlockFeature.hpp"
#include "common/world/gen/feature/SimpleRandomSelectorFeature.hpp"
#include "common/world/gen/feature/SnowAndFreezeFeature.hpp"
#include "common/world/gen/feature/VoidStartPlatformFeature.hpp"
#include "common/world/gen/feature/cave/CaveSurface.hpp"
#include "common/world/gen/feature/cave/RootSystemFeature.hpp"
#include "common/world/gen/feature/cave/VegetationPatchFeature.hpp"
#include "common/world/gen/feature/end/ChorusPlantFeature.hpp"
#include "common/world/gen/feature/end/EndGatewayFeature.hpp"
#include "common/world/gen/feature/end/EndIslandFeature.hpp"
#include "common/world/gen/feature/end/EndSpikeFeature.hpp"
#include "common/world/gen/feature/end/IceSpikeFeature.hpp"
#include "common/world/gen/feature/nether/BasaltFeature.hpp"
#include "common/world/gen/feature/nether/DeltaFeature.hpp"
#include "common/world/gen/feature/nether/GlowstoneFeature.hpp"
#include "common/world/gen/feature/nether/HugeFungusFeature.hpp"
#include "common/world/gen/feature/nether/UnderwaterMagmaFeature.hpp"
#include "common/world/gen/feature/ocean/BlueIceFeature.hpp"
#include "common/world/gen/feature/ocean/CoralFeature.hpp"
#include "common/world/gen/feature/ocean/KelpFeature.hpp"
#include "common/world/gen/feature/ocean/SeaPickleFeature.hpp"
#include "common/world/gen/feature/ocean/SeagrassFeature.hpp"
#include "common/world/gen/feature/ore/OreFeature.hpp"
#include "common/world/gen/feature/parser/BlockPredicateParser.hpp"
#include "common/world/gen/feature/parser/BlockStateParser.hpp"
#include "common/world/gen/feature/parser/BlockStateProviderParser.hpp"
#include "common/world/gen/feature/parser/FoliagePlacerParser.hpp"
#include "common/world/gen/feature/parser/RuleTestParser.hpp"
#include "common/world/gen/feature/parser/TrunkPlacerParser.hpp"
#include "common/world/gen/feature/tree/TreeFeature.hpp"
#include "common/world/gen/feature/vegetation/BambooFeature.hpp"
#include "common/world/gen/feature/vegetation/BigMushroomFeature.hpp"
#include "common/world/gen/feature/vegetation/FlowerFeature.hpp"
#include "common/world/gen/placement/PlacedFeature.hpp"
#include "common/world/gen/placement/PlacedFeatureLoader.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <nlohmann/json.hpp>

namespace mc {
namespace world::gen::feature {

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
 * @brief 把派生 ConfiguredFeature unique_ptr 提升为基类 unique_ptr
 *
 * Result<unique_ptr<ConfiguredFeatureBase>> 的构造函数要求精确的
 * unique_ptr<ConfiguredFeatureBase>，无法从 unique_ptr<Derived> 隐式推导，
 * 故工厂统一经此 helper 做一次派生→基类转换。
 */
std::unique_ptr<ConfiguredFeatureBase> toBase(std::unique_ptr<ConfiguredFeatureBase> feature)
{
    return feature;
}

/**
 * @brief monster_room 工厂：config 为空，构造 ConfiguredMonsterRoomFeature
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createMonsterRoom(const nlohmann::json& /*configJson*/)
{
    std::unique_ptr<ConfiguredFeatureBase> feature = std::make_unique<ConfiguredMonsterRoomFeature>();
    return feature;
}

/**
 * @brief freeze_top_layer 工厂：config 为空，构造 ConfiguredSnowAndFreezeFeature
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createFreezeTopLayer(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredSnowAndFreezeFeature>("freeze_top_layer"));
}

/**
 * @brief end_island 工厂：config 为空，构造 ConfiguredEndIslandFeature
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createEndIsland(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredEndIslandFeature>("end_island"));
}

/**
 * @brief chorus_plant 工厂：config 为空，构造 ConfiguredChorusPlantFeature
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createChorusPlant(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredChorusPlantFeature>("chorus_plant"));
}

/**
 * @brief ice_spike 工厂：config 为空，用默认值（尖塔型）构造
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createIceSpike(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredIceSpikeFeature>(std::make_unique<IceSpikeFeatureConfig>(), "ice_spike"));
}

/**
 * @brief glowstone_blob 工厂：config 为空结构体
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createGlowstoneBlob(const nlohmann::json& /*configJson*/)
{
    return toBase(
        std::make_unique<ConfiguredGlowstoneFeature>(std::make_unique<GlowstoneFeatureConfig>(), "glowstone_blob"));
}

/**
 * @brief kelp 工厂：config 为空，海带方块状态硬编码（KELP/KELP_PLANT）
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createKelp(const nlohmann::json& /*configJson*/)
{
    auto config = std::make_unique<KelpFeatureConfig>(
        VanillaBlocks::getState(VanillaBlocks::KELP), VanillaBlocks::getState(VanillaBlocks::KELP_PLANT), 16, 6);
    return toBase(std::make_unique<ConfiguredKelpFeature>(std::move(config), "kelp"));
}

/**
 * @brief blue_ice 工厂：config 为空，蓝冰/浮冰方块状态硬编码
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createBlueIce(const nlohmann::json& /*configJson*/)
{
    auto config = std::make_unique<BlueIceFeatureConfig>();
    config->blueIceState = VanillaBlocks::getState(VanillaBlocks::BLUE_ICE);
    config->packedIceState = VanillaBlocks::getState(VanillaBlocks::PACKED_ICE);
    return toBase(std::make_unique<ConfiguredBlueIceFeature>(std::move(config), "blue_ice"));
}

/**
 * @brief coral_tree/coral_mushroom/coral_claw 工厂：config 为空。
 *
 * MC 三个 type 各自只生成对应形状；项目 CoralFeature 内部用 random 在三种形状间分支
 * （CoralFeature.cpp），故三个 type 共用 ConfiguredCoralFeature，行为为项目现状。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createCoral(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredCoralFeature>(std::make_unique<CoralFeatureConfig>(), "coral"));
}

// ----------------------------------------------------------------------------
// 中档工厂：config 字段较少，从 JSON 读取后构造
// ----------------------------------------------------------------------------

namespace {

f32 getFloat(const nlohmann::json& obj, const char* key, f32 fallback)
{
    return (obj.contains(key) && obj[key].is_number()) ? obj[key].get<f32>() : fallback;
}

i32 getInt(const nlohmann::json& obj, const char* key, i32 fallback)
{
    return (obj.contains(key) && obj[key].is_number_integer()) ? obj[key].get<i32>() : fallback;
}

bool getBool(const nlohmann::json& obj, const char* key, bool fallback)
{
    return (obj.contains(key) && obj[key].is_boolean()) ? obj[key].get<bool>() : fallback;
}

} // namespace

/**
 * @brief bamboo 工厂：probability → podzolProbability；4 个竹子方块状态硬编码。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createBamboo(const nlohmann::json& configJson)
{
    const f32 podzolProbability = getFloat(configJson, "probability", 0.0f);
    auto config = std::make_unique<BambooFeatureConfig>();
    config->podzolProbability = podzolProbability;
    // 粗竹竿：AGE=1, STAGE=0, LEAVES=None
    config->bambooState =
        &VanillaBlocks::BAMBOO->defaultState()
             .with(BlockStateProperties::AGE_0_1(), 1)
             .with(BlockStateProperties::STAGE_0_1(), 0)
             .with(BlockStateProperties::BAMBOO_LEAVES_PROP(), BlockStateProperties::BambooLeaves::None);
    // 顶部停止生长：LEAVES=Large, STAGE=1
    config->topFinalState =
        &VanillaBlocks::BAMBOO->defaultState()
             .with(BlockStateProperties::BAMBOO_LEAVES_PROP(), BlockStateProperties::BambooLeaves::Large)
             .with(BlockStateProperties::STAGE_0_1(), 1);
    // 顶部下方第1格：LEAVES=Large, STAGE=0
    config->topLargeState =
        &VanillaBlocks::BAMBOO->defaultState()
             .with(BlockStateProperties::BAMBOO_LEAVES_PROP(), BlockStateProperties::BambooLeaves::Large)
             .with(BlockStateProperties::STAGE_0_1(), 0);
    // 顶部下方第2格：LEAVES=Small, STAGE=0
    config->topSmallState =
        &VanillaBlocks::BAMBOO->defaultState()
             .with(BlockStateProperties::BAMBOO_LEAVES_PROP(), BlockStateProperties::BambooLeaves::Small)
             .with(BlockStateProperties::STAGE_0_1(), 0);
    return toBase(std::make_unique<ConfiguredBambooFeature>(std::move(config), "bamboo"));
}

/**
 * @brief seagrass 工厂：probability → tallSeagrassChance；海草/高海草状态硬编码。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createSeagrass(const nlohmann::json& configJson)
{
    const f32 tallChance = getFloat(configJson, "probability", 0.0f);
    auto config = std::make_unique<SeagrassFeatureConfig>();
    config->seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    config->tallSeagrassLowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    config->tallSeagrassUpperState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    config->tallSeagrassChance = tallChance;
    return toBase(std::make_unique<ConfiguredSeagrassFeature>(std::move(config), "seagrass"));
}

/**
 * @brief sea_pickle 工厂：count → tries（放置尝试次数）；海泡菜状态硬编码（PICKLES=1）。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createSeaPickle(const nlohmann::json& configJson)
{
    const i32 count = getInt(configJson, "count", 10);
    auto config = std::make_unique<SeaPickleFeatureConfig>();
    config->seaPickleState = &VanillaBlocks::SEA_PICKLE->defaultState().with(BlockStateProperties::PICKLES_1_4(), 1);
    config->tries = count;
    return toBase(std::make_unique<ConfiguredSeaPickleFeature>(std::move(config), "sea_pickle"));
}

/**
 * @brief huge_brown_mushroom/huge_red_mushroom 工厂：cap_provider+stem_provider → capState/stemState，
 *        foliage_radius → capRadius；第三个 isBrown 参数区分棕/红。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createBigMushroom(const nlohmann::json& configJson, bool isBrown)
{
    if (!configJson.contains("cap_provider") || !configJson.contains("stem_provider")) {
        return Error(ErrorCode::InvalidData, "huge_mushroom config missing 'cap_provider'/'stem_provider'");
    }
    auto capResult = parser::BlockStateProviderParser::parse(configJson["cap_provider"]);
    if (!capResult.success()) {
        return capResult.error();
    }
    const BlockState* capState = capResult.value().asSingle();
    if (capState == nullptr) {
        return Error(ErrorCode::InvalidData, "huge_mushroom cap_provider must be simple_state_provider");
    }
    auto stemResult = parser::BlockStateProviderParser::parse(configJson["stem_provider"]);
    if (!stemResult.success()) {
        return stemResult.error();
    }
    const BlockState* stemState = stemResult.value().asSingle();
    if (stemState == nullptr) {
        return Error(ErrorCode::InvalidData, "huge_mushroom stem_provider must be simple_state_provider");
    }
    auto config =
        std::make_unique<BigMushroomFeatureConfig>(capState, stemState, getInt(configJson, "foliage_radius", 2));
    return toBase(std::make_unique<ConfiguredBigMushroomFeature>(
        std::move(config), isBrown ? "huge_brown_mushroom" : "huge_red_mushroom", isBrown));
}

Result<std::unique_ptr<ConfiguredFeatureBase>> createHugeBrownMushroom(const nlohmann::json& configJson)
{
    return createBigMushroom(configJson, true);
}

Result<std::unique_ptr<ConfiguredFeatureBase>> createHugeRedMushroom(const nlohmann::json& configJson)
{
    return createBigMushroom(configJson, false);
}

/**
 * @brief end_spike 工厂：spikes 为空时 place() 会用 generator.seed() 自动生成；
 *        crystal_invulnerable 当前未映射（项目 EndSpike 无该字段）。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createEndSpike(const nlohmann::json& /*configJson*/)
{
    auto config = std::make_unique<EndSpikeFeatureConfig>();
    // spikes 留空 → place() 内自动 generateSpikes(generator.seed())
    return toBase(std::make_unique<ConfiguredEndSpikeFeature>(std::move(config), "end_spike"));
}

/**
 * @brief end_gateway 工厂：MC 的 exit(可选 BlockPos 数组 [x,y,z]) → exactPosition；
 *        exact(bool) → 是否使用精确出口位置。项目 EndGatewayFeature::place 当前不读 config
 *        （结构固定），故仅做正确解析备未来使用。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createEndGateway(const nlohmann::json& configJson)
{
    auto config = std::make_unique<EndGatewayFeatureConfig>();
    if (configJson.contains("exit") && configJson["exit"].is_array() && configJson["exit"].size() == 3) {
        const i32 x = configJson["exit"][0].get<i32>();
        const i32 y = configJson["exit"][1].get<i32>();
        const i32 z = configJson["exit"][2].get<i32>();
        config->exactPosition = BlockPos(x, y, z);
        config->isExit = true;
    } else {
        config->isExit = false;
    }
    return toBase(std::make_unique<ConfiguredEndGatewayFeature>(std::move(config), "end_gateway"));
}

/**
 * @brief huge_fungus 工厂：从 stem_state.Name 推断 FungusType（crimson_stem→Crimson,
 *        warped_stem→Warped）；planted → planted。hat_state/decor_state/replaceable_blocks
 *        当前未映射（项目内部按 fungusType 硬编码全部状态）。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createHugeFungus(const nlohmann::json& configJson)
{
    FungusType fungusType = FungusType::Crimson;
    if (configJson.contains("stem_state") && configJson["stem_state"].is_object() &&
        configJson["stem_state"].contains("Name") && configJson["stem_state"]["Name"].is_string()) {
        const std::string& name = configJson["stem_state"]["Name"].get<std::string>();
        if (name.find("warped_stem") != std::string::npos) {
            fungusType = FungusType::Warped;
        }
    }
    auto config = std::make_unique<HugeFungusFeatureConfig>(fungusType, getBool(configJson, "planted", false));
    return toBase(std::make_unique<ConfiguredHugeFungusFeature>(std::move(config), "huge_fungus"));
}

/**
 * @brief simple_block 工厂：to_place（BlockStateProvider）。
 * simple_state_provider → 取单一状态；weighted_state_provider → 持有提供者按权重采样。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createSimpleBlock(const nlohmann::json& configJson)
{
    if (!configJson.contains("to_place")) {
        return Error(ErrorCode::InvalidData, "simple_block config missing 'to_place' block state provider");
    }
    auto providerResult = parser::BlockStateProviderParser::parse(configJson["to_place"]);
    if (!providerResult.success()) {
        return providerResult.error();
    }
    auto& handle = providerResult.value();
    auto config = std::make_unique<cave::SimpleBlockConfig>();
    if (handle.kind == parser::BlockStateProviderHandle::Kind::Simple) {
        config->toPlace = handle.simple;
        if (config->toPlace == nullptr) {
            return Error(ErrorCode::InvalidData, "simple_block to_place simple_state_provider has null state");
        }
    } else if (handle.kind == parser::BlockStateProviderHandle::Kind::Weighted) {
        config->weightedProvider = std::move(handle.weighted);
        if (config->weightedProvider == nullptr || config->weightedProvider->empty()) {
            return Error(ErrorCode::InvalidData, "simple_block to_place weighted_state_provider has no entries");
        }
    } else {
        return Error(
            ErrorCode::InvalidData, "simple_block to_place must be simple_state_provider or weighted_state_provider");
    }
    return toBase(std::make_unique<cave::ConfiguredSimpleBlockFeature>(std::move(config), "simple_block"));
}

/**
 * @brief random_boolean_selector 工厂：feature_true/feature_false（嵌套 {feature:"id",placement:[]}，
 *        取 "feature" 字段）→ 两个 ResourceLocation 子特征 id。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createRandomBooleanSelector(const nlohmann::json& configJson)
{
    if (!configJson.contains("feature_true") || !configJson.contains("feature_false")) {
        return Error(ErrorCode::InvalidData, "random_boolean_selector config missing 'feature_true'/'feature_false'");
    }
    auto config = std::make_unique<cave::RandomBooleanFeatureConfig>();
    config->featureTrueId = ResourceLocation(configJson["feature_true"]["feature"].get<std::string>());
    config->featureFalseId = ResourceLocation(configJson["feature_false"]["feature"].get<std::string>());
    return toBase(
        std::make_unique<cave::ConfiguredRandomBooleanSelectorFeature>(std::move(config), "random_boolean_selector"));
}

/**
 * @brief simple_random_selector 工厂：features 数组（每项为内联 PlacedFeature，对齐 MC
 *        SimpleRandomFeatureConfiguration{features: HolderSet<PlacedFeature>}）。
 *
 * 每项形式：{feature:{type,config}, placement:[...]}（内联 configured_feature + placement 链）。
 * 原版 4 个 simple_random_selector 文件（dripleaf/forest_flowers/pointed_dripstone/
 * warm_ocean_vegetation）均用此内联对象形式。
 *
 * 内联 configured_feature 所有权由 SimpleRandomFeatureConfig::inlineFeatures 托管；
 * PlacedFeature 持其裸指针 + placement 链（经 PlacedFeatureLoader::parsePlacementChain 构造）。
 * 内联项无独立 id，placement 链回填用占位 ResourceLocation。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createSimpleRandomSelector(const nlohmann::json& configJson)
{
    if (!configJson.contains("features") || !configJson["features"].is_array()) {
        return Error(ErrorCode::InvalidData, "simple_random_selector config missing 'features' array");
    }
    auto config = std::make_unique<cave::SimpleRandomFeatureConfig>();

    for (size_t i = 0; i < configJson["features"].size(); ++i) {
        const auto& entry = configJson["features"][i];
        const ResourceLocation placeholderId("minecraft", "simple_random_selector_inline_" + std::to_string(i));

        // 每项必须是内联 PlacedFeature 对象：{feature:{type,config}, placement:[...]}。
        // 对齐 MC SimpleRandomFeatureConfiguration.features=HolderSet<PlacedFeature>；
        // 原版 4 个 simple_random_selector 文件均用此内联对象形式。
        if (!entry.is_object() || !entry.contains("feature") || !entry["feature"].is_object()) {
            return Error(ErrorCode::InvalidData,
                "simple_random_selector features[] entry must be an inline PlacedFeature object "
                "{feature:{type,config}, placement:[...]}");
        }
        const auto& featureField = entry["feature"];

        if (!entry.contains("placement") || !entry["placement"].is_array()) {
            return Error(ErrorCode::InvalidData, "simple_random_selector features[] entry missing 'placement' array");
        }
        auto chainResult = placement::PlacedFeatureLoader::parsePlacementChain(entry["placement"], placeholderId);
        if (!chainResult.success()) {
            return Error(chainResult.error().code(),
                "simple_random_selector features[] placement: " + chainResult.error().message());
        }

        if (!featureField.contains("type") || !featureField["type"].is_string()) {
            return Error(ErrorCode::InvalidData, "simple_random_selector inline feature object missing 'type' string");
        }
        const std::string innerType = featureField["type"].get<std::string>();
        const nlohmann::json innerConfig =
            featureField.contains("config") ? featureField["config"] : nlohmann::json::object();
        auto innerResult = FeatureTypeRegistry::instance().create(innerType, innerConfig);
        if (!innerResult.success()) {
            return Error(innerResult.error().code(),
                "simple_random_selector inline configured_feature '" + innerType +
                    "': " + innerResult.error().message());
        }
        auto inlineFeature = innerResult.value();
        if (inlineFeature == nullptr) {
            return Error(
                ErrorCode::InvalidData, "simple_random_selector inline configured_feature constructed as null");
        }
        config->inlineFeatures.push_back(std::move(inlineFeature));
        const auto* cfPtr = config->inlineFeatures.back().get();
        config->features.push_back(std::make_unique<PlacedFeature>(cfPtr, chainResult.value(), placeholderId));
    }

    return toBase(
        std::make_unique<cave::ConfiguredSimpleRandomSelectorFeature>(std::move(config), "simple_random_selector"));
}

/**
 * @brief 从内联 PlacedFeature 字段提取 configured_feature 的 ResourceLocation id
 *
 * MC JSON 中 PlacedFeature 内联对象形如 {"feature": "minecraft:xxx", "placement": [...]}，
 * 但部分条目直接用字符串 "minecraft:xxx"（如 random_selector features[] 中混用两种形式）。
 * 本函数统一处理：
 * - 字符串：直接作为 id
 * - 对象：取其 "feature" 子字段（必须是字符串）
 * - 其他：返回 Error
 *
 * placement 数组在数据驱动简化模式下被丢弃（项目 placement 链由父 PlacedFeature 走完）。
 */
Result<ResourceLocation> extractInlineFeatureId(const nlohmann::json& featureField, const char* contextForError)
{
    if (featureField.is_string()) {
        return ResourceLocation(featureField.get<std::string>());
    }
    if (featureField.is_object() && featureField.contains("feature") && featureField["feature"].is_string()) {
        return ResourceLocation(featureField["feature"].get<std::string>());
    }
    return Error(ErrorCode::InvalidData,
        std::string(contextForError) + " feature field must be string id or {feature:\"id\",...} object");
}

/**
 * @brief random_selector 工厂：features[]（每项 {chance, feature: <inline placed_feature|"id">}）+
 *        default（inline placed_feature）。顺序概率检查 + default 兜底。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createRandomSelector(const nlohmann::json& configJson)
{
    if (!configJson.contains("default")) {
        return Error(ErrorCode::InvalidData, "random_selector config missing 'default'");
    }
    auto defaultResult = extractInlineFeatureId(configJson["default"], "random_selector default");
    if (!defaultResult.success()) {
        return defaultResult.error();
    }

    auto config = std::make_unique<RandomSelectorFeatureConfig>();
    config->defaultFeatureId = defaultResult.value();

    if (configJson.contains("features") && configJson["features"].is_array()) {
        for (const auto& entry : configJson["features"]) {
            if (!entry.is_object() || !entry.contains("feature") || !entry.contains("chance")) {
                return Error(ErrorCode::InvalidData, "random_selector features[] entry missing 'feature'/'chance'");
            }
            auto idResult = extractInlineFeatureId(entry["feature"], "random_selector features[]");
            if (!idResult.success()) {
                return idResult.error();
            }
            WeightedFeatureEntry weighted;
            weighted.featureId = idResult.value();
            weighted.chance = getFloat(entry, "chance", 0.0f);
            config->features.push_back(std::move(weighted));
        }
    }

    return toBase(std::make_unique<ConfiguredRandomSelectorFeature>(std::move(config), "random_selector"));
}

// ----------------------------------------------------------------------------
// 简单档工厂：NoneConfig 或单一 BlockState 配置
// ----------------------------------------------------------------------------

/**
 * @brief end_platform / void_start_platform / bonus_chest / basalt_pillar 工厂：
 *        均 NoneFeatureConfiguration，直接构造对应 ConfiguredXxxFeature。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createEndPlatform(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredEndPlatformFeature>());
}

Result<std::unique_ptr<ConfiguredFeatureBase>> createVoidStartPlatform(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredVoidStartPlatformFeature>());
}

Result<std::unique_ptr<ConfiguredFeatureBase>> createBonusChest(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredBonusChestFeature>());
}

Result<std::unique_ptr<ConfiguredFeatureBase>> createBasaltPillar(const nlohmann::json& /*configJson*/)
{
    return toBase(std::make_unique<ConfiguredBasaltPillarFeature>());
}

/**
 * @brief forest_rock 工厂：state（block state 对象）→ BlockStateConfig。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createForestRock(const nlohmann::json& configJson)
{
    if (!configJson.contains("state")) {
        return Error(ErrorCode::InvalidData, "forest_rock config missing 'state'");
    }
    auto stateResult = parser::BlockStateParser::parse(configJson["state"]);
    if (!stateResult.success()) {
        return stateResult.error();
    }
    auto config = std::make_unique<BlockStateConfig>(stateResult.value());
    return toBase(std::make_unique<ConfiguredBlockBlobFeature>(std::move(config), "forest_rock"));
}

// ----------------------------------------------------------------------------
// 复杂档工厂：config 字段多，依赖 RuleTest/IntProvider/TrunkPlacer/FoliagePlacer/BlockPredicate 解析器
// ----------------------------------------------------------------------------

/**
 * @brief ore 工厂：size + discard_chance_on_air_exposure + targets[]（每项 state + target RuleTest）。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createOre(const nlohmann::json& configJson)
{
    if (!configJson.contains("targets") || !configJson["targets"].is_array()) {
        return Error(ErrorCode::InvalidData, "ore config missing 'targets' array");
    }
    std::vector<OreTarget> targets;
    for (const auto& entry : configJson["targets"]) {
        if (!entry.contains("state") || !entry.contains("target")) {
            return Error(ErrorCode::InvalidData, "ore target missing 'state'/'target'");
        }
        auto stateResult = parser::BlockStateParser::parse(entry["state"]);
        if (!stateResult.success()) {
            return stateResult.error();
        }
        auto targetResult = parser::RuleTestParser::parse(entry["target"]);
        if (!targetResult.success()) {
            return targetResult.error();
        }
        targets.emplace_back(targetResult.value(), stateResult.value());
    }
    const i32 size = getInt(configJson, "size", 0);
    const f32 discardChance = getFloat(configJson, "discard_chance_on_air_exposure", 0.0f);
    auto config = std::make_unique<OreFeatureConfig>(std::move(targets), size, discardChance);
    return toBase(std::make_unique<ConfiguredOreFeature>(std::move(config), "ore"));
}

/**
 * @brief tree 工厂：trunk_provider/foliage_provider → trunkBlock/foliageBlock(或 foliageProvider)，
 *        trunk_placer/foliage_placer → 放置器；ignore_vines/force_dirt/max_water_depth/minimum_size。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createTree(const nlohmann::json& configJson)
{
    auto config = std::make_unique<TreeFeatureConfig>();

    if (configJson.contains("trunk_provider")) {
        auto providerResult = parser::BlockStateProviderParser::parse(configJson["trunk_provider"]);
        if (!providerResult.success()) {
            return providerResult.error();
        }
        config->trunkBlock = providerResult.value().asSingle();
        if (config->trunkBlock == nullptr) {
            return Error(ErrorCode::InvalidData, "tree trunk_provider must be simple_state_provider");
        }
    }
    if (configJson.contains("foliage_provider")) {
        auto providerResult = parser::BlockStateProviderParser::parse(configJson["foliage_provider"]);
        if (!providerResult.success()) {
            return providerResult.error();
        }
        auto& handle = providerResult.value();
        if (handle.kind == parser::BlockStateProviderHandle::Kind::Weighted) {
            config->foliageProvider = std::move(handle.weighted);
        } else {
            config->foliageBlock = handle.simple;
        }
    }
    if (configJson.contains("trunk_placer")) {
        auto trunkResult = parser::TrunkPlacerParser::parse(configJson["trunk_placer"]);
        if (!trunkResult.success()) {
            return trunkResult.error();
        }
        config->trunkPlacer = trunkResult.value();
    }
    if (configJson.contains("foliage_placer")) {
        auto foliageResult = parser::FoliagePlacerParser::parse(configJson["foliage_placer"]);
        if (!foliageResult.success()) {
            return foliageResult.error();
        }
        config->foliagePlacer = foliageResult.value();
    }
    config->ignoreVines = getBool(configJson, "ignore_vines", false);
    // MC 的 force_dirt 对应项目 forcePlacement（跳过高度检查的强制放置语义最近）
    config->forcePlacement = getBool(configJson, "force_dirt", false);
    // minimum_size 的 limit/lower_size/upper_size 项目 TreeFeatureConfig 未建模，暂忽略
    // TODO: 对齐 MC minimum_size（TwoLayersFeatureSize）字段需扩展 TreeFeatureConfig。
    return toBase(std::make_unique<ConfiguredTreeFeature>(std::move(config), "tree"));
}

/**
 * @brief block_column 工厂：layers[{height(IntProvider), provider}] + direction + allowed_placement +
 *        prioritize_tip。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createBlockColumn(const nlohmann::json& configJson)
{
    auto config = std::make_unique<cave::BlockColumnConfig>();
    if (configJson.contains("layers") && configJson["layers"].is_array()) {
        for (const auto& layerObj : configJson["layers"]) {
            cave::BlockColumnLayer layer;
            if (!layerObj.contains("height")) {
                return Error(ErrorCode::InvalidData, "block_column layer missing 'height' IntProvider");
            }
            auto heightResult = valueprovider::IntProviderParser::parse(layerObj["height"]);
            if (!heightResult.success()) {
                return heightResult.error();
            }
            layer.height = heightResult.value();
            if (layerObj.contains("provider")) {
                auto providerResult = parser::BlockStateProviderParser::parse(layerObj["provider"]);
                if (!providerResult.success()) {
                    return providerResult.error();
                }
                auto& handle = providerResult.value();
                if (handle.kind == parser::BlockStateProviderHandle::Kind::Weighted) {
                    layer.stateProvider = std::move(handle.weighted);
                } else {
                    layer.state = handle.simple;
                }
            }
            config->layers.push_back(std::move(layer));
        }
    }
    if (configJson.contains("direction") && configJson["direction"].is_string()) {
        auto dir = mc::Directions::fromName(configJson["direction"].get<std::string>());
        if (dir.has_value()) {
            config->direction = *dir;
        }
    }
    if (configJson.contains("allowed_placement")) {
        auto predResult = parser::BlockPredicateParser::parse(configJson["allowed_placement"]);
        if (!predResult.success()) {
            return predResult.error();
        }
        config->allowedPlacement = predResult.value();
    }
    config->prioritizeTip = getBool(configJson, "prioritize_tip", false);
    return toBase(std::make_unique<cave::ConfiguredBlockColumnFeature>(std::move(config), "block_column"));
}

namespace {

cave::CaveSurface parseCaveSurface(const nlohmann::json& obj, const char* key, cave::CaveSurface fallback)
{
    if (!obj.contains(key) || !obj[key].is_string()) {
        return fallback;
    }
    const std::string s = obj[key].get<std::string>();
    return s == "ceiling" ? cave::CaveSurface::Ceiling : cave::CaveSurface::Floor;
}

} // namespace

/**
 * @brief vegetation_patch / waterlogged_vegetation_patch 工厂：
 *        depth/extra_bottom_block_chance/extra_edge_column_chance/ground_state/replaceable/surface/
 *        vegetation_chance/vegetation_feature/vertical_range/xz_radius。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createVegetationPatch(const nlohmann::json& configJson, bool waterlogged)
{
    auto config = std::make_unique<cave::VegetationPatchConfig>();
    if (configJson.contains("replaceable") && configJson["replaceable"].is_string()) {
        config->replaceableTag = configJson["replaceable"].get<std::string>();
    }
    if (configJson.contains("ground_state")) {
        // ground_state 是 BlockStateProvider（如 simple_state_provider），不是裸 BlockState；
        // 配置存单一状态，故取 asSingle()。非 simple provider（如 weighted）无法降级为单一状态。
        auto groundResult = parser::BlockStateProviderParser::parse(configJson["ground_state"]);
        if (!groundResult.success()) {
            return groundResult.error();
        }
        const BlockState* state = groundResult.value().asSingle();
        if (state == nullptr) {
            return Error(ErrorCode::InvalidData,
                "vegetation_patch ground_state must be simple_state_provider (non-single provider unsupported)");
        }
        config->groundState = state;
    }
    if (configJson.contains("vegetation_feature") && configJson["vegetation_feature"].is_object() &&
        configJson["vegetation_feature"].contains("feature") &&
        configJson["vegetation_feature"]["feature"].is_string()) {
        config->vegetationFeatureId = ResourceLocation(configJson["vegetation_feature"]["feature"].get<std::string>());
    }
    config->surface = parseCaveSurface(configJson, "surface", cave::CaveSurface::Floor);
    if (configJson.contains("depth")) {
        auto depthResult = valueprovider::IntProviderParser::parse(configJson["depth"]);
        if (!depthResult.success()) {
            return depthResult.error();
        }
        config->depth = depthResult.value();
    }
    config->extraBottomBlockChance = getFloat(configJson, "extra_bottom_block_chance", 0.0f);
    config->verticalRange = getInt(configJson, "vertical_range", 5);
    config->vegetationChance = getFloat(configJson, "vegetation_chance", 0.8f);
    if (configJson.contains("xz_radius")) {
        auto radiusResult = valueprovider::IntProviderParser::parse(configJson["xz_radius"]);
        if (!radiusResult.success()) {
            return radiusResult.error();
        }
        config->xzRadius = radiusResult.value();
    }
    config->extraEdgeColumnChance = getFloat(configJson, "extra_edge_column_chance", 0.3f);

    if (waterlogged) {
        return toBase(std::make_unique<cave::ConfiguredWaterloggedPatchFeature>(
            std::move(config), "waterlogged_vegetation_patch"));
    }
    return toBase(std::make_unique<cave::ConfiguredVegetationPatchFeature>(std::move(config), "vegetation_patch"));
}

Result<std::unique_ptr<ConfiguredFeatureBase>> createWaterloggedVegetationPatch(const nlohmann::json& configJson)
{
    return createVegetationPatch(configJson, true);
}

/**
 * @brief root_system 工厂：feature(树 id) + 各 root/hanging 参数 + root_state_provider/
 *        hanging_root_state_provider(取单一状态) + root_replaceable(标签)。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createRootSystem(const nlohmann::json& configJson)
{
    auto config = std::make_unique<cave::RootSystemConfig>();
    if (configJson.contains("feature") && configJson["feature"].is_object() &&
        configJson["feature"].contains("feature") && configJson["feature"]["feature"].is_string()) {
        config->treeFeatureId = ResourceLocation(configJson["feature"]["feature"].get<std::string>());
    }
    config->requiredVerticalSpaceForTree = getInt(configJson, "required_vertical_space_for_tree", 3);
    config->rootRadius = getInt(configJson, "root_radius", 3);
    if (configJson.contains("root_replaceable") && configJson["root_replaceable"].is_string()) {
        config->rootReplaceableTag = configJson["root_replaceable"].get<std::string>();
    }
    if (configJson.contains("root_state_provider")) {
        auto providerResult = parser::BlockStateProviderParser::parse(configJson["root_state_provider"]);
        if (!providerResult.success()) {
            return providerResult.error();
        }
        config->rootState = providerResult.value().asSingle();
    }
    config->rootPlacementAttempts = getInt(configJson, "root_placement_attempts", 20);
    config->rootColumnMaxHeight = getInt(configJson, "root_column_max_height", 100);
    config->hangingRootRadius = getInt(configJson, "hanging_root_radius", 20);
    config->hangingRootsVerticalSpan = getInt(configJson, "hanging_roots_vertical_span", 2);
    if (configJson.contains("hanging_root_state_provider")) {
        auto providerResult = parser::BlockStateProviderParser::parse(configJson["hanging_root_state_provider"]);
        if (!providerResult.success()) {
            return providerResult.error();
        }
        config->hangingRootState = providerResult.value().asSingle();
    }
    config->hangingRootPlacementAttempts = getInt(configJson, "hanging_root_placement_attempts", 3);
    config->allowedVerticalWaterForTree = getInt(configJson, "allowed_vertical_water_for_tree", 2);
    return toBase(std::make_unique<cave::ConfiguredRootSystemFeature>(std::move(config), "root_system"));
}

/**
 * @brief flower 工厂：tries/xz_spread/y_spread + feature（内联 simple_block + weighted_state_provider，
 *        提取全部加权条目状态为 flowers 列表）。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createFlower(const nlohmann::json& configJson)
{
    auto config = std::make_unique<FlowerFeatureConfig>();
    config->tries = getInt(configJson, "tries", 64);
    config->xzSpread = getInt(configJson, "xz_spread", 7);
    config->ySpread = getInt(configJson, "y_spread", 3);

    // 内联特征结构：config.feature.feature.config.to_place (BlockStateProvider)
    if (configJson.contains("feature") && configJson["feature"].is_object() &&
        configJson["feature"].contains("feature") && configJson["feature"]["feature"].is_object() &&
        configJson["feature"]["feature"].contains("config") &&
        configJson["feature"]["feature"]["config"].contains("to_place")) {
        const auto& toPlace = configJson["feature"]["feature"]["config"]["to_place"];
        auto providerResult = parser::BlockStateProviderParser::parse(toPlace);
        if (!providerResult.success()) {
            return providerResult.error();
        }
        auto& handle = providerResult.value();
        if (handle.kind == parser::BlockStateProviderHandle::Kind::Weighted && handle.weighted != nullptr) {
            // 加权提供者：把所有条目状态平铺为可选花卉列表（MC random_patch 的 flower 集合语义）
            for (const auto& entry : handle.weighted->entries()) {
                if (entry.state != nullptr) {
                    config->flowers.push_back(entry.state);
                }
            }
        } else if (handle.kind == parser::BlockStateProviderHandle::Kind::Simple) {
            config->flowers.push_back(handle.simple);
        }
    }
    return toBase(std::make_unique<ConfiguredFlowerFeature>(std::move(config), "flower"));
}

/**
 * @brief random_patch 工厂：tries/xz_spread/y_spread + feature（内联 PlacedFeature，
 *        含内联 configured_feature 对象 + placement 链，通常含 block_predicate_filter）。
 *
 * 内联 configured_feature 经 FeatureTypeRegistry::create 递归构造，所有权由
 * RandomPatchFeatureConfig::inlineFeature 托管；PlacedFeature 持有其裸指针 +
 * placement 链（经 PlacedFeatureLoader::parsePlacementChain 构造）。
 * 因内联 feature 无独立 id，placement 链回填用一个占位 ResourceLocation。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createRandomPatch(const nlohmann::json& configJson)
{
    if (!configJson.contains("feature") || !configJson["feature"].is_object()) {
        return Error(ErrorCode::InvalidData, "random_patch config missing inline 'feature' object");
    }
    const auto& inlinePlacedFeature = configJson["feature"];

    // 内联 configured_feature：{type, config}
    if (!inlinePlacedFeature.contains("feature") || !inlinePlacedFeature["feature"].is_object() ||
        !inlinePlacedFeature["feature"].contains("type")) {
        return Error(ErrorCode::InvalidData,
            "random_patch inline feature.feature must be a configured_feature object with 'type'");
    }
    const auto& inlineConfiguredFeature = inlinePlacedFeature["feature"];
    const std::string innerType = inlineConfiguredFeature["type"].get<std::string>();
    const nlohmann::json innerConfig =
        inlineConfiguredFeature.contains("config") ? inlineConfiguredFeature["config"] : nlohmann::json::object();
    auto innerResult = FeatureTypeRegistry::instance().create(innerType, innerConfig);
    if (!innerResult.success()) {
        return Error(innerResult.error().code(),
            "random_patch inline configured_feature '" + innerType + "': " + innerResult.error().message());
    }
    auto inlineFeature = innerResult.value();
    if (inlineFeature == nullptr) {
        return Error(ErrorCode::InvalidData, "random_patch inline configured_feature constructed as null");
    }

    // placement 链：内联 feature 无独立 id，用一个占位 ResourceLocation 回填 BiomeFilterConfig
    if (!inlinePlacedFeature.contains("placement") || !inlinePlacedFeature["placement"].is_array()) {
        return Error(ErrorCode::InvalidData, "random_patch inline feature missing 'placement' array");
    }
    const ResourceLocation placeholderId("minecraft", "random_patch_inline");
    auto chainResult =
        placement::PlacedFeatureLoader::parsePlacementChain(inlinePlacedFeature["placement"], placeholderId);
    if (!chainResult.success()) {
        return Error(chainResult.error().code(), "random_patch inline placement: " + chainResult.error().message());
    }

    auto config = std::make_unique<RandomPatchFeatureConfig>();
    config->tries = getInt(configJson, "tries", 128);
    config->xzSpread = getInt(configJson, "xz_spread", 7);
    config->ySpread = getInt(configJson, "y_spread", 3);
    config->inlineFeature = std::move(inlineFeature);
    // PlacedFeature 持有 inlineFeature 裸指针（由 config 持有所有权，生命周期长于 PlacedFeature）
    config->feature = std::make_unique<PlacedFeature>(config->inlineFeature.get(), chainResult.value(), placeholderId);

    return toBase(std::make_unique<ConfiguredRandomPatchFeature>(std::move(config), "random_patch"));
}

// ----------------------------------------------------------------------------
// 算法对齐档：lake / basalt_columns / delta_feature / underwater_magma
// 各自忠实复刻 MC 1.21.11 算法，config 从 JSON 读取。
// ----------------------------------------------------------------------------

/**
 * @brief lake 工厂：fluid + barrier（均为 BlockStateProvider）。
 * simple 提供者取 asSingle() 填 fluidState/barrierState；weighted 填 provider。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createLake(const nlohmann::json& configJson)
{
    lake::LakeFeatureConfig config;
    if (configJson.contains("fluid")) {
        auto result = parser::BlockStateProviderParser::parse(configJson["fluid"]);
        if (!result.success()) {
            return result.error();
        }
        auto& handle = result.value();
        if (handle.kind == parser::BlockStateProviderHandle::Kind::Weighted) {
            config.fluidProvider = std::move(handle.weighted);
        } else {
            config.fluidState = handle.simple;
        }
    }
    if (configJson.contains("barrier")) {
        auto result = parser::BlockStateProviderParser::parse(configJson["barrier"]);
        if (!result.success()) {
            return result.error();
        }
        auto& handle = result.value();
        if (handle.kind == parser::BlockStateProviderHandle::Kind::Weighted) {
            config.barrierProvider = std::move(handle.weighted);
        } else {
            config.barrierState = handle.simple;
        }
    }
    if (config.fluidState == nullptr && config.fluidProvider == nullptr) {
        return Error(ErrorCode::InvalidData, "lake config missing 'fluid' BlockStateProvider");
    }
    return toBase(std::make_unique<ConfiguredLakeFeature>(std::move(config), "lake"));
}

/**
 * @brief basalt_columns 工厂：reach + height（均 IntProvider）。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createBasaltColumns(const nlohmann::json& configJson)
{
    auto config = std::make_unique<BasaltColumnFeatureConfig>();
    if (!configJson.contains("reach") || !configJson.contains("height")) {
        return Error(ErrorCode::InvalidData, "basalt_columns config missing 'reach'/'height' IntProvider");
    }
    auto reachResult = valueprovider::IntProviderParser::parse(configJson["reach"]);
    if (!reachResult.success()) {
        return reachResult.error();
    }
    auto heightResult = valueprovider::IntProviderParser::parse(configJson["height"]);
    if (!heightResult.success()) {
        return heightResult.error();
    }
    config->reach = reachResult.value();
    config->height = heightResult.value();
    return toBase(std::make_unique<ConfiguredBasaltColumnFeature>(std::move(config), "basalt_columns"));
}

/**
 * @brief delta_feature 工厂：contents/rim（BlockState）+ size/rim_size（IntProvider）。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createDelta(const nlohmann::json& configJson)
{
    auto config = std::make_unique<DeltaFeatureConfig>();
    if (configJson.contains("contents")) {
        auto result = parser::BlockStateParser::parse(configJson["contents"]);
        if (!result.success()) {
            return result.error();
        }
        config->contents = result.value();
    }
    if (configJson.contains("rim")) {
        auto result = parser::BlockStateParser::parse(configJson["rim"]);
        if (!result.success()) {
            return result.error();
        }
        config->rim = result.value();
    }
    if (configJson.contains("size")) {
        auto result = valueprovider::IntProviderParser::parse(configJson["size"]);
        if (!result.success()) {
            return result.error();
        }
        config->size = result.value();
    }
    if (configJson.contains("rim_size")) {
        auto result = valueprovider::IntProviderParser::parse(configJson["rim_size"]);
        if (!result.success()) {
            return result.error();
        }
        config->rimSize = result.value();
    }
    if (config->contents == nullptr || config->rim == nullptr) {
        return Error(ErrorCode::InvalidData, "delta_feature config missing 'contents'/'rim' BlockState");
    }
    return toBase(std::make_unique<ConfiguredDeltaFeature>(std::move(config), "delta_feature"));
}

/**
 * @brief underwater_magma 工厂：floor_search_range(int) +
 *        placement_radius_around_floor(int) + placement_probability_per_valid_position(float)。
 */
Result<std::unique_ptr<ConfiguredFeatureBase>> createUnderwaterMagma(const nlohmann::json& configJson)
{
    const i32 floorSearchRange = getInt(configJson, "floor_search_range", 0);
    const i32 placementRadius = getInt(configJson, "placement_radius_around_floor", 0);
    const f32 probability = getFloat(configJson, "placement_probability_per_valid_position", 0.0f);
    UnderwaterMagmaConfig config(floorSearchRange, placementRadius, probability);
    return toBase(std::make_unique<ConfiguredUnderwaterMagmaFeature>(config, "underwater_magma"));
}

} // namespace

FeatureTypeRegistry& FeatureTypeRegistry::instance()
{
    static FeatureTypeRegistry s_instance;
    return s_instance;
}

void FeatureTypeRegistry::registerType(const std::string& type, Factory factory)
{
    m_factories[stripNamespace(type)] = std::move(factory);
}

Result<std::unique_ptr<ConfiguredFeatureBase>> FeatureTypeRegistry::create(
    const std::string& type, const nlohmann::json& configJson) const
{
    const std::string key = stripNamespace(type);
    const auto it = m_factories.find(key);
    if (it == m_factories.end()) {
        return Error(ErrorCode::NotFound,
            "Unregistered configured_feature type: '" + type +
                "'. This feature type has no C++ implementation yet; "
                "implement it and register in FeatureTypeRegistry.");
    }
    return it->second(configJson);
}

bool FeatureTypeRegistry::has(const std::string& type) const noexcept
{
    return m_factories.find(stripNamespace(type)) != m_factories.end();
}

void FeatureTypeRegistry::clear() noexcept
{
    m_factories.clear();
}

/**
 * @brief 初始化内置特征类型工厂
 *
 * 注册当前已实现的 feature type。未注册的 type 在加载时严格报错（见 create()）。
 */
void initializeBuiltinFeatureTypes()
{
    auto& reg = FeatureTypeRegistry::instance();
    reg.registerType("monster_room", createMonsterRoom);
    // 极易档：config 为空或几乎为空，直接构造
    reg.registerType("freeze_top_layer", createFreezeTopLayer);
    reg.registerType("end_island", createEndIsland);
    reg.registerType("chorus_plant", createChorusPlant);
    reg.registerType("ice_spike", createIceSpike);
    reg.registerType("glowstone_blob", createGlowstoneBlob);
    reg.registerType("kelp", createKelp);
    reg.registerType("blue_ice", createBlueIce);
    // coral 三个 type 共用 ConfiguredCoralFeature（项目内部随机分支三种形状）
    reg.registerType("coral_tree", createCoral);
    reg.registerType("coral_mushroom", createCoral);
    reg.registerType("coral_claw", createCoral);
    // 中档：config 字段较少，从 JSON 读取后构造
    reg.registerType("bamboo", createBamboo);
    reg.registerType("seagrass", createSeagrass);
    reg.registerType("sea_pickle", createSeaPickle);
    reg.registerType("huge_brown_mushroom", createHugeBrownMushroom);
    reg.registerType("huge_red_mushroom", createHugeRedMushroom);
    reg.registerType("end_spike", createEndSpike);
    reg.registerType("end_gateway", createEndGateway);
    reg.registerType("huge_fungus", createHugeFungus);
    reg.registerType("simple_block", createSimpleBlock);
    reg.registerType("random_boolean_selector", createRandomBooleanSelector);
    reg.registerType("simple_random_selector", createSimpleRandomSelector);
    reg.registerType("random_selector", createRandomSelector);
    // 复杂档：config 依赖 RuleTest/IntProvider/TrunkPlacer/FoliagePlacer/BlockPredicate 解析器
    reg.registerType("ore", createOre);
    reg.registerType("tree", createTree);
    reg.registerType("block_column", createBlockColumn);
    reg.registerType("vegetation_patch", [](const nlohmann::json& j) { return createVegetationPatch(j, false); });
    reg.registerType("waterlogged_vegetation_patch", createWaterloggedVegetationPatch);
    reg.registerType("root_system", createRootSystem);
    reg.registerType("flower", createFlower);
    reg.registerType("random_patch", createRandomPatch);
    // 算法对齐档：忠实复刻 MC 1.21.11 的 lake/basalt_columns/delta_feature/underwater_magma
    reg.registerType("lake", createLake);
    reg.registerType("basalt_columns", createBasaltColumns);
    reg.registerType("delta_feature", createDelta);
    reg.registerType("underwater_magma", createUnderwaterMagma);
    // 简单档：NoneConfig 或单一 BlockState 配置
    reg.registerType("end_platform", createEndPlatform);
    reg.registerType("void_start_platform", createVoidStartPlatform);
    reg.registerType("bonus_chest", createBonusChest);
    reg.registerType("basalt_pillar", createBasaltPillar);
    reg.registerType("forest_rock", createForestRock);
    // TODO: 数据包共 54 种 configured_feature type，当前注册 42 种。
    // 未注册的 type（geode/sculk_patch/large_dripstone/disk/spring_feature/fossil/
    // desert_well/scattered_ore/netherrack_replace_blobs/block_pile/twisting_vines/
    // weeping_vines/nether_forest_vegetation/fallen_tree/iceberg 等）
    // 加载对应 JSON 时会严格报错中断。按报错逐个补实现并在此 registerType。
}

} // namespace world::gen::feature
} // namespace mc
