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

// ============================================================================
// 花卉 placed_feature 列表填充与解析测试
//
// 覆盖三条链路：
// 1. BiomeGenerationSettings::addFlowerFeature 仅追加到独立花卉列表，
//    不再重复登记到阶段通用列表（避免世界生成时重复放置）。
// 2. BiomeLoader::loadFromJson 解析 biome JSON 的 features 二维数组时，
//    对底层 configured_feature 为 ConfiguredFlowerFeature 的 placed_feature，
//    同时调用 addPlacedFeature 与 addFlowerFeature，使花卉既参与正常装饰生成，
//    也出现在花卉列表中供 GrassBlock::grow 骨粉催花使用。
// 3. GrassBlock::grow 骨粉催花的解析路径：从花卉 placed_feature id 经
//    PlacedFeatureRegistry 解析为 PlacedFeature，再取 feature() 拿到
//    ConfiguredFlowerFeature，从其配置中随机选择花朵方块状态。
// ============================================================================

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeGenerationSettings.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeLoader.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/feature/vegetation/FlowerFeature.hpp"
#include "common/world/gen/placement/PlacedFeature.hpp"
#include "common/world/gen/placement/PlacedFeatureRegistry.hpp"
#include "common/world/gen/placement/Placement.hpp"

#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::biome;

namespace {

// ============================================================================
// 测试辅助：构建一个 ConfiguredFlowerFeature（底层 configured_feature）
// ============================================================================

/// 持有 ConfiguredFlowerFeature 所有权，避免在测试用例间泄漏或提前释放。
/// PlacedFeature 不拥有 configured_feature 的所有权，必须由外部保活。
std::vector<std::unique_ptr<ConfiguredFeatureBase>>& stubFeatureStorage()
{
    static std::vector<std::unique_ptr<ConfiguredFeatureBase>> storage;
    return storage;
}

/// 构建一个含单一花朵方块状态的 ConfiguredFlowerFeature
ConfiguredFlowerFeature* makeFlowerFeature(const char* name, const BlockState* flowerState)
{
    auto config = std::make_unique<FlowerFeatureConfig>(flowerState);
    auto feature = std::make_unique<ConfiguredFlowerFeature>(std::move(config), name);
    auto* raw = feature.get();
    stubFeatureStorage().push_back(std::move(feature));
    return raw;
}

/// 构建一个最简单的 PlacedFeature 并注册到 PlacedFeatureRegistry
const PlacedFeature* registerPlacedFlower(const ResourceLocation& id, ConfiguredFlowerFeature* configured)
{
    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<SquarePlacement>(), std::make_unique<EmptyPlacementConfig>());
    auto placed = std::make_unique<PlacedFeature>(configured, std::move(placement), id);
    const PlacedFeature* raw = placed.get();
    PlacedFeatureRegistry::instance().registerPlacedFeature(std::move(placed));
    return raw;
}

/// 构建一个非花卉的 ConfiguredFeatureBase 存根（用于验证非花卉条目不会被误加入花卉列表）
class NonFlowerStubFeature : public ConfiguredFeatureBase {
public:
    explicit NonFlowerStubFeature(const char* name, DecorationStage stage)
        : m_name(name)
        , m_stage(stage)
    {}

    bool place(WorldGenRegion&, ChunkPrimer&, IChunkGenerator&, math::Random&, const BlockPos&) const override
    {
        return false;
    }
    const char* name() const override { return m_name; }
    DecorationStage stage() const override { return m_stage; }

private:
    const char* m_name;
    DecorationStage m_stage;
};

NonFlowerStubFeature* makeNonFlowerStub(const char* name, DecorationStage stage)
{
    auto stub = std::make_unique<NonFlowerStubFeature>(name, stage);
    auto* raw = stub.get();
    stubFeatureStorage().push_back(std::move(stub));
    return raw;
}

const PlacedFeature* registerPlacedStub(const ResourceLocation& id, NonFlowerStubFeature* configured)
{
    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<SquarePlacement>(), std::make_unique<EmptyPlacementConfig>());
    auto placed = std::make_unique<PlacedFeature>(configured, std::move(placement), id);
    const PlacedFeature* raw = placed.get();
    PlacedFeatureRegistry::instance().registerPlacedFeature(std::move(placed));
    return raw;
}

/// 测试夹具：每个用例前后清空 PlacedFeatureRegistry 与 stub 存储
class BiomeFlowerFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        PlacedFeatureRegistry::instance().clear();
        stubFeatureStorage().clear();
    }

    void TearDown() override
    {
        PlacedFeatureRegistry::instance().clear();
        stubFeatureStorage().clear();
    }
};

} // namespace

// ============================================================================
// BiomeGenerationSettings::addFlowerFeature 单元测试
// ============================================================================

TEST_F(BiomeFlowerFeatureTest, AddFlowerFeatureOnlyAppendsToFlowerList)
{
    // 重构后：addFlowerFeature 仅追加到 m_flowerFeatureIds，
    // 不再调用 addPlacedFeature，避免同一 placed_feature 在阶段列表中被登记两次。
    BiomeGenerationSettings settings;
    const ResourceLocation flowerId("minecraft", "flower_default");

    settings.addFlowerFeature(flowerId);

    const auto& flowerIds = settings.getFlowerFeatureIds();
    ASSERT_EQ(flowerIds.size(), 1u);
    EXPECT_EQ(flowerIds[0], flowerId);

    // 阶段通用列表不应被 addFlowerFeature 改动
    const auto& vegetalFeatures = settings.getFeatures(DecorationStage::VegetalDecoration);
    EXPECT_TRUE(vegetalFeatures.empty());
}

TEST_F(BiomeFlowerFeatureTest, AddPlacedFeatureAndFlowerFeatureIndependent)
{
    // 调用方应先 addPlacedFeature 登记到阶段列表，再 addFlowerFeature 登记到花卉列表。
    // 两个列表互不干扰，花卉 placed_feature 在阶段列表中仅出现一次。
    BiomeGenerationSettings settings;
    const ResourceLocation flowerId("minecraft", "flower_plains");
    const ResourceLocation otherId("minecraft", "patch_tall_grass_2");

    settings.addPlacedFeature(DecorationStage::VegetalDecoration, flowerId);
    settings.addPlacedFeature(DecorationStage::VegetalDecoration, otherId);
    settings.addFlowerFeature(flowerId);

    const auto& vegetalFeatures = settings.getFeatures(DecorationStage::VegetalDecoration);
    ASSERT_EQ(vegetalFeatures.size(), 2u);
    EXPECT_EQ(vegetalFeatures[0], flowerId);
    EXPECT_EQ(vegetalFeatures[1], otherId);

    const auto& flowerIds = settings.getFlowerFeatureIds();
    ASSERT_EQ(flowerIds.size(), 1u);
    EXPECT_EQ(flowerIds[0], flowerId);
}

TEST_F(BiomeFlowerFeatureTest, ClearResetsFlowerList)
{
    BiomeGenerationSettings settings;
    settings.addFlowerFeature(ResourceLocation("minecraft", "flower_default"));
    settings.addPlacedFeature(DecorationStage::VegetalDecoration, ResourceLocation("minecraft", "patch_grass"));

    settings.clear();

    EXPECT_TRUE(settings.getFlowerFeatureIds().empty());
    EXPECT_TRUE(settings.getFeatures(DecorationStage::VegetalDecoration).empty());
}

TEST_F(BiomeFlowerFeatureTest, HasPlacedFeatureStillWorksAcrossStages)
{
    // 回归测试：hasPlacedFeature 不受花卉列表重构影响
    BiomeGenerationSettings settings;
    const ResourceLocation flowerId("minecraft", "flower_default");

    settings.addPlacedFeature(DecorationStage::VegetalDecoration, flowerId);
    settings.addFlowerFeature(flowerId);

    EXPECT_TRUE(settings.hasPlacedFeature(flowerId));
    EXPECT_FALSE(settings.hasPlacedFeature(ResourceLocation("minecraft", "nonexistent")));
}

// ============================================================================
// BiomeLoader::loadFromJson 花卉列表填充集成测试
// ============================================================================

TEST_F(BiomeFlowerFeatureTest, LoadFromJsonPopulatesFlowerListForFlowerPlacedFeatures)
{
    // 构造一个最小 biome JSON：features 二维数组，第 9 阶段（VegetalDecoration）
    // 包含一个花卉 placed_feature 和一个非花卉 placed_feature。
    // 期望 BiomeLoader 将花卉条目同时登记到阶段列表和花卉列表，非花卉条目只进阶段列表。
    BiomeRegistry::instance().initialize();
    auto& biome = BiomeRegistry::instance().getMutable(Biomes::Plains);

    // 先注册一个花卉 placed_feature 和一个非花卉 placed_feature
    const BlockState* dandelion = &VanillaBlocks::DANDELION->defaultState();
    ConfiguredFlowerFeature* flowerCfg = makeFlowerFeature("flower", dandelion);
    const ResourceLocation flowerPlacedId("minecraft", "flower_plains_test");
    registerPlacedFlower(flowerPlacedId, flowerCfg);

    NonFlowerStubFeature* nonFlowerCfg = makeNonFlowerStub("patch_grass", DecorationStage::VegetalDecoration);
    const ResourceLocation nonFlowerPlacedId("minecraft", "patch_tall_grass_test");
    registerPlacedStub(nonFlowerPlacedId, nonFlowerCfg);

    // 构造 features JSON：11 个子数组（对应 11 个 DecorationStage），仅第 9 个非空
    nlohmann::json featuresJson = nlohmann::json::array();
    for (size_t i = 0; i < 11; ++i) {
        featuresJson.push_back(nlohmann::json::array());
    }
    featuresJson[9] = nlohmann::json::array({flowerPlacedId.toString(), nonFlowerPlacedId.toString()});

    nlohmann::json biomeJson = {
        {"temperature", 0.8},
        {"downfall", 0.4},
        {"has_precipitation", true},
        {"effects", nlohmann::json::object({{"sky_color", "#78a7ff"}, {"water_color", "#3f76e4"}})},
        {"features", featuresJson},
    };

    const ResourceLocation biomeId("minecraft", "plains");
    auto result = BiomeLoader::loadFromJson(biomeJson, biomeId);
    ASSERT_TRUE(result.success()) << "BiomeLoader::loadFromJson 应成功";

    const auto& genSettings = biome.generationSettings();

    // 花卉列表应只包含花卉 placed_feature
    const auto& flowerIds = genSettings.getFlowerFeatureIds();
    ASSERT_EQ(flowerIds.size(), 1u);
    EXPECT_EQ(flowerIds[0], flowerPlacedId);

    // VegetalDecoration 阶段列表应包含两个 placed_feature（花卉和非花卉）
    const auto& vegetalFeatures = genSettings.getFeatures(DecorationStage::VegetalDecoration);
    ASSERT_EQ(vegetalFeatures.size(), 2u);
    EXPECT_EQ(vegetalFeatures[0], flowerPlacedId);
    EXPECT_EQ(vegetalFeatures[1], nonFlowerPlacedId);
}

TEST_F(BiomeFlowerFeatureTest, LoadFromJsonNoFlowerPlacedFeaturesYieldsEmptyFlowerList)
{
    // 当 features 中没有花卉 placed_feature 时，花卉列表应为空
    BiomeRegistry::instance().initialize();
    auto& biome = BiomeRegistry::instance().getMutable(Biomes::Desert);

    NonFlowerStubFeature* nonFlowerCfg = makeNonFlowerStub("patch_grass", DecorationStage::VegetalDecoration);
    const ResourceLocation nonFlowerPlacedId("minecraft", "patch_dead_bush_test");
    registerPlacedStub(nonFlowerPlacedId, nonFlowerCfg);

    nlohmann::json featuresJson = nlohmann::json::array();
    for (size_t i = 0; i < 11; ++i) {
        featuresJson.push_back(nlohmann::json::array());
    }
    featuresJson[9] = nlohmann::json::array({nonFlowerPlacedId.toString()});

    nlohmann::json biomeJson = {
        {"temperature", 2.0},
        {"downfall", 0.0},
        {"has_precipitation", false},
        {"effects", nlohmann::json::object({{"sky_color", "#fffbdb"}, {"water_color", "#3f76e4"}})},
        {"features", featuresJson},
    };

    const ResourceLocation biomeId("minecraft", "desert");
    auto result = BiomeLoader::loadFromJson(biomeJson, biomeId);
    ASSERT_TRUE(result.success());

    const auto& genSettings = biome.generationSettings();
    EXPECT_TRUE(genSettings.getFlowerFeatureIds().empty());
    EXPECT_EQ(genSettings.getFeatures(DecorationStage::VegetalDecoration).size(), 1u);
}

TEST_F(BiomeFlowerFeatureTest, LoadFromJsonSkipsUnregisteredPlacedFeatures)
{
    // 未注册的 placed_feature id 应被 warn + skip，不影响其他条目解析
    BiomeRegistry::instance().initialize();
    auto& biome = BiomeRegistry::instance().getMutable(Biomes::Plains);

    const BlockState* dandelion = &VanillaBlocks::DANDELION->defaultState();
    ConfiguredFlowerFeature* flowerCfg = makeFlowerFeature("flower", dandelion);
    const ResourceLocation flowerPlacedId("minecraft", "flower_plains_test");
    registerPlacedFlower(flowerPlacedId, flowerCfg);

    nlohmann::json featuresJson = nlohmann::json::array();
    for (size_t i = 0; i < 11; ++i) {
        featuresJson.push_back(nlohmann::json::array());
    }
    featuresJson[9] = nlohmann::json::array(
        {flowerPlacedId.toString(), "minecraft:unregistered_placed_feature", "minecraft:another_unregistered"});

    nlohmann::json biomeJson = {
        {"temperature", 0.8},
        {"downfall", 0.4},
        {"has_precipitation", true},
        {"effects", nlohmann::json::object({{"sky_color", "#78a7ff"}, {"water_color", "#3f76e4"}})},
        {"features", featuresJson},
    };

    const ResourceLocation biomeId("minecraft", "plains");
    auto result = BiomeLoader::loadFromJson(biomeJson, biomeId);
    ASSERT_TRUE(result.success());

    const auto& genSettings = biome.generationSettings();
    // 只有已注册的花卉条目进入花卉列表和阶段列表
    EXPECT_EQ(genSettings.getFlowerFeatureIds().size(), 1u);
    EXPECT_EQ(genSettings.getFeatures(DecorationStage::VegetalDecoration).size(), 1u);
}

// ============================================================================
// PlacedFeature -> ConfiguredFlowerFeature 解析路径测试
// （GrassBlock::grow 骨粉催花依赖此路径）
// ============================================================================

TEST_F(BiomeFlowerFeatureTest, PlacedFeatureResolvesToConfiguredFlowerFeature)
{
    // 验证 GrassBlock::grow 的解析路径：placed_feature id → PlacedFeatureRegistry::get
    // → PlacedFeature::feature() → dynamic_cast<ConfiguredFlowerFeature*>
    const BlockState* dandelion = &VanillaBlocks::DANDELION->defaultState();
    ConfiguredFlowerFeature* flowerCfg = makeFlowerFeature("flower", dandelion);
    const ResourceLocation placedId("minecraft", "flower_default_test");
    registerPlacedFlower(placedId, flowerCfg);

    const PlacedFeature* placed = PlacedFeatureRegistry::instance().get(placedId);
    ASSERT_NE(placed, nullptr);
    const ConfiguredFeatureBase* configured = placed->feature();
    ASSERT_NE(configured, nullptr);

    auto* flower = dynamic_cast<const ConfiguredFlowerFeature*>(configured);
    ASSERT_NE(flower, nullptr);

    // 验证从配置中能取到花朵方块状态
    mc::math::Random rng(42);
    const BlockState* chosen = flower->getConfig().getRandomFlower(rng);
    ASSERT_NE(chosen, nullptr);
    EXPECT_EQ(chosen->blockId(), dandelion->blockId());
}

TEST_F(BiomeFlowerFeatureTest, PlacedFeatureNonFlowerDynamicCastReturnsNull)
{
    // 验证非花卉 placed_feature 的 dynamic_cast 返回 nullptr（GrassBlock::grow 据此跳过）
    NonFlowerStubFeature* nonFlowerCfg = makeNonFlowerStub("patch_grass", DecorationStage::VegetalDecoration);
    const ResourceLocation placedId("minecraft", "patch_grass_test");
    registerPlacedStub(placedId, nonFlowerCfg);

    const PlacedFeature* placed = PlacedFeatureRegistry::instance().get(placedId);
    ASSERT_NE(placed, nullptr);
    auto* flower = dynamic_cast<const ConfiguredFlowerFeature*>(placed->feature());
    EXPECT_EQ(flower, nullptr);
}

TEST_F(BiomeFlowerFeatureTest, PlacedFeatureRegistryReturnsNullForUnknownId)
{
    // 未注册的 placed_feature id 应返回 nullptr（GrassBlock::grow 据此回退到蒲公英）
    const PlacedFeature* placed = PlacedFeatureRegistry::instance().get(ResourceLocation("minecraft", "nonexistent"));
    EXPECT_EQ(placed, nullptr);
}
