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

#include "common/world/gen/feature/FeatureSorter.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/Assert.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/placement/PlacedFeature.hpp"
#include "common/world/gen/placement/PlacedFeatureRegistry.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 测试用 StubFeature
// ============================================================================

/**
 * @brief 最小化的 ConfiguredFeatureBase 实现，仅用于测试
 *
 * FeatureSorter 只使用特征对象的指针身份和 ResourceLocation id，
 * 不调用 place() 方法。
 */
class StubFeature : public ConfiguredFeatureBase {
public:
    StubFeature(const char* n, DecorationStage s)
        : m_name(n)
        , m_stage(s)
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

// ============================================================================
// 测试辅助工具
// ============================================================================

/**
 * @brief 注册一个 stub 放置特征到 PlacedFeatureRegistry
 *
 * 构造一个 StubFeature（ConfiguredFeatureBase）并用一个最小放置链
 * （SquarePlacement + EmptyPlacementConfig，行为无关紧要——测试从不调用 place()）
 * 包装成 PlacedFeature，以 ResourceLocation("test", name) 为 id 注册。
 *
 * @param registry 放置特征注册表
 * @param name 特征名称（同时作为 ResourceLocation 的 path）
 * @param stage 装饰阶段
 * @return 注册后的 PlacedFeature 指针（由 registry 拥有所有权）
 */
static const PlacedFeature* registerPlacedStub(PlacedFeatureRegistry& registry, const char* name, DecorationStage stage)
{
    auto configuredFeature = std::make_unique<StubFeature>(name, stage);
    const ConfiguredFeatureBase* configuredPtr = configuredFeature.get();

    // PlacedFeature 不拥有 ConfiguredFeatureBase 所有权，需要先保活。
    // 这里把 configuredFeature 的所有权借给一个 static 容器，避免泄漏。
    // （测试场景下 registry 在 TearDown 清空，stub 配置由下方 s_stubConfigs 持有。）
    static std::vector<std::unique_ptr<ConfiguredFeatureBase>> s_stubConfigs;
    s_stubConfigs.push_back(std::move(configuredFeature));

    // PlacedFeature 构造要求 placement 非空（与 MC 语义一致：PlacedFeature 必须含 placement 链）。
    // FeatureSorter 仅使用指针身份与 id，不调用 place()，故放置链行为无关紧要，
    // 这里用最小的 SquarePlacement + EmptyPlacementConfig 占位。
    auto placement = std::make_unique<ConfiguredPlacement>(
        std::make_unique<SquarePlacement>(), std::make_unique<EmptyPlacementConfig>());

    auto placedFeature =
        std::make_unique<PlacedFeature>(configuredPtr, std::move(placement), ResourceLocation("test", name));
    const PlacedFeature* placedPtr = placedFeature.get();
    registry.registerPlacedFeature(std::move(placedFeature));
    return placedPtr;
}

/**
 * @brief 测试夹具：自动清除 PlacedFeatureRegistry，并安装抛异常的断言处理器
 *
 * FeatureSorter 在检测到 feature 依赖环时会触发 MC_ASSERT_RELEASE_MSG(false, ...)。
 * 默认的断言处理器会终止进程，导致测试无法验证成环行为。这里在 SetUp 中安装
 * 抛出 AssertException 的处理器，使测试可以用 EXPECT_THROW 捕获断言失败。
 */
class FeatureSorterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        PlacedFeatureRegistry::instance().clear();
        originalAssertConfig_ = mc::assert::AssertManager::instance().config();
        mc::assert::AssertConfig config;
        config.handler = [](const mc::assert::AssertFailure& failure) { throw mc::assert::AssertException(failure); };
        mc::assert::AssertManager::instance().setConfig(config);
    }

    void TearDown() override
    {
        mc::assert::AssertManager::instance().setConfig(originalAssertConfig_);
        PlacedFeatureRegistry::instance().clear();
    }

    /**
     * @brief 构建 getFeatures 回调函数
     *
     * 使用 map<(BiomeId, stageIndex), vector<ResourceLocation>> 存储测试数据，
     * 返回的 lambda 返回对应 vector 的 const 引用。
     */
    static std::function<const std::vector<ResourceLocation>&(BiomeId, DecorationStage)> makeGetFeatures(
        std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>>& data)
    {
        return [&data](BiomeId biomeId, DecorationStage stage) -> const std::vector<ResourceLocation>& {
            static const std::vector<ResourceLocation> empty;
            auto it = data.find({biomeId, static_cast<int>(stage)});
            return it != data.end() ? it->second : empty;
        };
    }

private:
    mc::assert::AssertConfig originalAssertConfig_;
};

// ============================================================================
// StepFeatureData 单元测试
// ============================================================================

TEST_F(FeatureSorterTest, StepFeatureDataEmpty)
{
    FeatureSorter::StepFeatureData data;

    EXPECT_TRUE(data.empty());
    EXPECT_TRUE(data.features.empty());
    EXPECT_TRUE(data.indexMapping.empty());
}

TEST_F(FeatureSorterTest, StepFeatureDataConstructorAndIndexMapping)
{
    // 创建 3 个 stub 放置特征（不需要注册到 registry，仅测试 StepFeatureData 构造）
    auto& registry = PlacedFeatureRegistry::instance();
    const PlacedFeature* f1 = registerPlacedStub(registry, "feature_a", DecorationStage::UndergroundOres);
    const PlacedFeature* f2 = registerPlacedStub(registry, "feature_b", DecorationStage::UndergroundOres);
    const PlacedFeature* f3 = registerPlacedStub(registry, "feature_c", DecorationStage::UndergroundOres);

    std::vector<const PlacedFeature*> features = {f1, f2, f3};
    std::vector<ResourceLocation> featureIds = {
        ResourceLocation("test", "feature_a"),
        ResourceLocation("test", "feature_b"),
        ResourceLocation("test", "feature_c"),
    };

    FeatureSorter::StepFeatureData data(features, featureIds);

    EXPECT_FALSE(data.empty());
    EXPECT_EQ(data.features.size(), 3u);
    EXPECT_EQ(data.features[0], f1);
    EXPECT_EQ(data.features[1], f2);
    EXPECT_EQ(data.features[2], f3);

    // indexMapping: placed_feature id → sortedIndex
    EXPECT_EQ(data.getIndex(ResourceLocation("test", "feature_a")), 0);
    EXPECT_EQ(data.getIndex(ResourceLocation("test", "feature_b")), 1);
    EXPECT_EQ(data.getIndex(ResourceLocation("test", "feature_c")), 2);
}

TEST_F(FeatureSorterTest, StepFeatureDataGetIndexNotFound)
{
    auto& registry = PlacedFeatureRegistry::instance();
    const PlacedFeature* f1 = registerPlacedStub(registry, "feature_a", DecorationStage::UndergroundOres);

    std::vector<const PlacedFeature*> features = {f1};
    std::vector<ResourceLocation> featureIds = {ResourceLocation("test", "feature_a")};

    FeatureSorter::StepFeatureData data(features, featureIds);

    EXPECT_EQ(data.getIndex(ResourceLocation("test", "feature_a")), 0);
    EXPECT_EQ(data.getIndex(ResourceLocation("test", "nonexistent")), -1); // 不存在的 id
    EXPECT_EQ(data.getIndex(ResourceLocation("other", "feature_a")), -1);  // 命名空间不同
}

// ============================================================================
// 基础排序测试
// ============================================================================

TEST_F(FeatureSorterTest, EmptyBiomesList)
{
    auto& registry = PlacedFeatureRegistry::instance();
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    auto getFeatures = makeGetFeatures(biomeData);

    std::vector<BiomeId> biomes; // 空
    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 空生物群系列表时 maxStep 为 0，result.resize(1) 产生一个空的 StepFeatureData
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(result[0].empty());
}

TEST_F(FeatureSorterTest, SingleBiomeSingleStageSingleFeature)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // 注册 1 个放置特征到 UndergroundOres 阶段
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);

    // 生物群系 0 在 UndergroundOres 有 1 个特征
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {ResourceLocation("test", "coal_ore")};

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // result 大小 = maxStep + 1 = 6 + 1 = 7 (UndergroundOres 是 stage 6)
    ASSERT_EQ(result.size(), 7u);

    // UndergroundOres 步骤应有 1 个特征
    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    ASSERT_EQ(oresStep.features.size(), 1u);
    EXPECT_EQ(oresStep.features[0], coalOre);
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "coal_ore")), 0);

    // 其他步骤应为空（result 大小为 maxStep+1=7，索引 0-6）
    EXPECT_TRUE(result[static_cast<int>(DecorationStage::Lakes)].empty());
    // VegetalDecoration (stage=9) 超出 result 大小范围，不检查
}

TEST_F(FeatureSorterTest, SingleBiomeMultipleFeaturesSameStage)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // 注册 3 个放置特征到 UndergroundOres 阶段
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* ironOre = registerPlacedStub(registry, "iron_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* goldOre = registerPlacedStub(registry, "gold_ore", DecorationStage::UndergroundOres);

    // 生物群系 0 在 UndergroundOres 有 3 个特征
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "iron_ore"),
        ResourceLocation("test", "gold_ore"),
    };

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    ASSERT_EQ(oresStep.features.size(), 3u);

    // 同一阶段内的拓扑排序应保持原始顺序（因为只有顺序依赖边 coal→iron→gold）
    EXPECT_EQ(oresStep.features[0], coalOre);
    EXPECT_EQ(oresStep.features[1], ironOre);
    EXPECT_EQ(oresStep.features[2], goldOre);

    // indexMapping 验证
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "coal_ore")), 0);
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "iron_ore")), 1);
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "gold_ore")), 2);
}

TEST_F(FeatureSorterTest, SingleBiomeMultipleStages)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // Lakes 阶段: WaterLake, LavaLake
    const PlacedFeature* waterLake = registerPlacedStub(registry, "water_lake", DecorationStage::Lakes);
    const PlacedFeature* lavaLake = registerPlacedStub(registry, "lava_lake", DecorationStage::Lakes);

    // UndergroundOres 阶段: CoalOre, IronOre
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* ironOre = registerPlacedStub(registry, "iron_ore", DecorationStage::UndergroundOres);

    // VegetalDecoration 阶段: OakTree
    const PlacedFeature* oakTree = registerPlacedStub(registry, "oak_tree", DecorationStage::VegetalDecoration);

    // 生物群系 0: Lakes + UndergroundOres + VegetalDecoration
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {
        ResourceLocation("test", "water_lake"),
        ResourceLocation("test", "lava_lake"),
    };
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "iron_ore"),
    };
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {
        ResourceLocation("test", "oak_tree"),
    };

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 验证各阶段特征数
    // Lakes(stage=1)应有 2 个, UndergroundOres(stage=6)应有 2 个, VegetalDecoration(stage=9)应有 1 个
    ASSERT_GE(result.size(), 10u);

    auto& lakesStep = result[static_cast<int>(DecorationStage::Lakes)];
    ASSERT_EQ(lakesStep.features.size(), 2u);
    EXPECT_EQ(lakesStep.features[0], waterLake);
    EXPECT_EQ(lakesStep.features[1], lavaLake);

    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    ASSERT_EQ(oresStep.features.size(), 2u);
    EXPECT_EQ(oresStep.features[0], coalOre);
    EXPECT_EQ(oresStep.features[1], ironOre);

    auto& vegetalStep = result[static_cast<int>(DecorationStage::VegetalDecoration)];
    ASSERT_EQ(vegetalStep.features.size(), 1u);
    EXPECT_EQ(vegetalStep.features[0], oakTree);

    // 空阶段验证
    EXPECT_TRUE(result[static_cast<int>(DecorationStage::RawGeneration)].empty());
    EXPECT_TRUE(result[static_cast<int>(DecorationStage::SurfaceStructures)].empty());
}

TEST_F(FeatureSorterTest, SingleBiomeFeaturesInCorrectStep)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // 注册特征到不同阶段
    const PlacedFeature* waterLake = registerPlacedStub(registry, "water_lake", DecorationStage::Lakes); // stage 1
    const PlacedFeature* coalOre =
        registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres); // stage 6
    const PlacedFeature* oakTree =
        registerPlacedStub(registry, "oak_tree", DecorationStage::VegetalDecoration); // stage 9
    const PlacedFeature* freeze =
        registerPlacedStub(registry, "freeze", DecorationStage::TopLayerModification); // stage 10

    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {ResourceLocation("test", "water_lake")};
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {ResourceLocation("test", "coal_ore")};
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {ResourceLocation("test", "oak_tree")};
    biomeData[{0, static_cast<int>(DecorationStage::TopLayerModification)}] = {ResourceLocation("test", "freeze")};

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // result 大小应为 maxStep + 1 = 10 + 1 = 11
    ASSERT_EQ(result.size(), 11u);

    // 每个非空 step 包含正确的特征
    EXPECT_EQ(result[1].features[0], waterLake);
    EXPECT_EQ(result[6].features[0], coalOre);
    EXPECT_EQ(result[9].features[0], oakTree);
    EXPECT_EQ(result[10].features[0], freeze);

    // 验证 indexMapping
    EXPECT_EQ(result[1].getIndex(ResourceLocation("test", "water_lake")), 0);
    EXPECT_EQ(result[6].getIndex(ResourceLocation("test", "coal_ore")), 0);
    EXPECT_EQ(result[9].getIndex(ResourceLocation("test", "oak_tree")), 0);
    EXPECT_EQ(result[10].getIndex(ResourceLocation("test", "freeze")), 0);
}

// ============================================================================
// 跨生物群系测试
// ============================================================================

TEST_F(FeatureSorterTest, TwoBiomesSharedFeatures)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // 注册 3 个矿石放置特征
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* ironOre = registerPlacedStub(registry, "iron_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* goldOre = registerPlacedStub(registry, "gold_ore", DecorationStage::UndergroundOres);

    // 生物群系 0 和 1 共享 coalOre 和 ironOre
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "iron_ore"),
    };
    biomeData[{1, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "iron_ore"),
        ResourceLocation("test", "gold_ore"),
    };

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    ASSERT_EQ(oresStep.features.size(), 3u);

    // 所有 3 个特征都应存在
    // 拓扑排序应保持: coal→iron (来自两个生物群系), iron→gold (来自生物群系 1)
    // 所以顺序应为 coal, iron, gold
    EXPECT_EQ(oresStep.features[0], coalOre);
    EXPECT_EQ(oresStep.features[1], ironOre);
    EXPECT_EQ(oresStep.features[2], goldOre);

    // indexMapping 验证
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "coal_ore")), 0);
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "iron_ore")), 1);
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "gold_ore")), 2);
}

TEST_F(FeatureSorterTest, TwoBiomesSharedFeaturesInSameStage)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // Vegetal Decoration 阶段特征
    const PlacedFeature* oakTree = registerPlacedStub(registry, "oak_tree", DecorationStage::VegetalDecoration);
    const PlacedFeature* birchTree = registerPlacedStub(registry, "birch_tree", DecorationStage::VegetalDecoration);
    const PlacedFeature* spruceTree = registerPlacedStub(registry, "spruce_tree", DecorationStage::VegetalDecoration);
    const PlacedFeature* plainsGrass = registerPlacedStub(registry, "plains_grass", DecorationStage::VegetalDecoration);

    // 生物群系 0 (Plains): oak + grass
    // 生物群系 1 (Forest): oak + birch + spruce
    // oakTree 被两个生物群系共享
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {
        ResourceLocation("test", "oak_tree"),
        ResourceLocation("test", "plains_grass"),
    };
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {
        ResourceLocation("test", "oak_tree"),
        ResourceLocation("test", "birch_tree"),
        ResourceLocation("test", "spruce_tree"),
    };

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    auto& vegetalStep = result[static_cast<int>(DecorationStage::VegetalDecoration)];
    ASSERT_EQ(vegetalStep.features.size(), 4u);

    // oakTree 应该排在 birchTree/spruceTree 之前（来自 Forest 的边: oak→birch→spruce）
    // oakTree 也应该在 plainsGrass 之前（来自 Plains 的边: oak→grass）
    // 验证 oakTree 的索引小于 birchTree 和 plainsGrass
    i32 oakIdx = vegetalStep.getIndex(ResourceLocation("test", "oak_tree"));
    i32 birchIdx = vegetalStep.getIndex(ResourceLocation("test", "birch_tree"));
    i32 spruceIdx = vegetalStep.getIndex(ResourceLocation("test", "spruce_tree"));
    i32 grassIdx = vegetalStep.getIndex(ResourceLocation("test", "plains_grass"));

    EXPECT_GE(oakIdx, 0);
    EXPECT_GE(birchIdx, 0);
    EXPECT_GE(spruceIdx, 0);
    EXPECT_GE(grassIdx, 0);

    // oakTree 应排在 birchTree 和 grass 之前
    EXPECT_LT(oakIdx, birchIdx);
    EXPECT_LT(oakIdx, grassIdx);
    // birchTree 应排在 spruceTree 之前
    EXPECT_LT(birchIdx, spruceIdx);
}

TEST_F(FeatureSorterTest, CrossStageGlobalIndexNoCollision)
{
    // 回归测试：不同 stage 的同名 fid 不应碰撞
    // 新标识体系下 placed_feature id 是 ResourceLocation，跨 stage 不会重名；
    // 这里沿用原测试意图：同一生物群系在多个 stage 引用不同 placed_feature，每个 stage 独立排序。
    auto& registry = PlacedFeatureRegistry::instance();

    // Lakes 阶段: WaterLake
    const PlacedFeature* waterLake = registerPlacedStub(registry, "water_lake", DecorationStage::Lakes);

    // UndergroundOres 阶段: CoalOre（与 WaterLake 是不同的 placed_feature）
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);

    // TopLayerModification 阶段: FreezeTopLayer
    const PlacedFeature* freezeTop = registerPlacedStub(registry, "freeze_top", DecorationStage::TopLayerModification);

    // 生物群系 0 拥有所有三个阶段的特征
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {ResourceLocation("test", "water_lake")};
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {ResourceLocation("test", "coal_ore")};
    biomeData[{0, static_cast<int>(DecorationStage::TopLayerModification)}] = {ResourceLocation("test", "freeze_top")};

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 关键断言：每个阶段的特征应该是不同的对象
    auto& lakesStep = result[static_cast<int>(DecorationStage::Lakes)];
    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    auto& freezeStep = result[static_cast<int>(DecorationStage::TopLayerModification)];

    ASSERT_EQ(lakesStep.features.size(), 1u);
    ASSERT_EQ(oresStep.features.size(), 1u);
    ASSERT_EQ(freezeStep.features.size(), 1u);

    // 每个阶段应包含正确的特征（不是其他阶段的特征）
    EXPECT_EQ(lakesStep.features[0], waterLake);
    EXPECT_NE(lakesStep.features[0], coalOre);
    EXPECT_NE(lakesStep.features[0], freezeTop);

    EXPECT_EQ(oresStep.features[0], coalOre);
    EXPECT_NE(oresStep.features[0], waterLake);
    EXPECT_NE(oresStep.features[0], freezeTop);

    EXPECT_EQ(freezeStep.features[0], freezeTop);
    EXPECT_NE(freezeStep.features[0], waterLake);
    EXPECT_NE(freezeStep.features[0], coalOre);

    // indexMapping 应正确：每个阶段的 id 映射到索引 0
    EXPECT_EQ(lakesStep.getIndex(ResourceLocation("test", "water_lake")), 0);
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "coal_ore")), 0);
    EXPECT_EQ(freezeStep.getIndex(ResourceLocation("test", "freeze_top")), 0);
}

// ============================================================================
// 环检测与特殊场景
// ============================================================================

TEST_F(FeatureSorterTest, CycleDetectionThrowsAssert)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // 构造人工环：两个特征在两个生物群系中出现顺序相反
    // 生物群系 A: featureA → featureB  (A 在 B 前)
    // 生物群系 B: featureB → featureA  (B 在 A 前) → 环！
    const PlacedFeature* featureA = registerPlacedStub(registry, "feature_a", DecorationStage::VegetalDecoration);
    const PlacedFeature* featureB = registerPlacedStub(registry, "feature_b", DecorationStage::VegetalDecoration);

    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {
        ResourceLocation("test", "feature_a"),
        ResourceLocation("test", "feature_b"),
    };
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {
        ResourceLocation("test", "feature_b"),
        ResourceLocation("test", "feature_a"),
    };

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    // 成环时应抛出 AssertException（MC_ASSERT_RELEASE_MSG(false, ...) 触发）
    EXPECT_THROW(FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry), mc::assert::AssertException);
}

TEST_F(FeatureSorterTest, CycleExceptionMessageContainsDiagnosticInfo)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // 构造人工环：A→B (生物群系 0) 和 B→A (生物群系 1)
    const PlacedFeature* featureA = registerPlacedStub(registry, "feature_a", DecorationStage::VegetalDecoration);
    const PlacedFeature* featureB = registerPlacedStub(registry, "feature_b", DecorationStage::VegetalDecoration);

    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {
        ResourceLocation("test", "feature_a"),
        ResourceLocation("test", "feature_b"),
    };
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {
        ResourceLocation("test", "feature_b"),
        ResourceLocation("test", "feature_a"),
    };

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    try {
        FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);
        FAIL() << "Expected AssertException to be thrown";
    }
    catch (const mc::assert::AssertException& ex) {
        const std::string msg = ex.what();
        // 断言消息应包含环节点链和生物群系来源的诊断信息
        EXPECT_NE(msg.find("cycle"), std::string::npos);
        EXPECT_NE(msg.find("feature_a"), std::string::npos);
        EXPECT_NE(msg.find("feature_b"), std::string::npos);
    }
}

TEST_F(FeatureSorterTest, UnregisteredFeatureIdSkipped)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // 注册一个有效特征
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);

    // 在 biomeData 中引用一个未注册的 placed_feature id
    // buildFeaturesPerStep 经 registry.get() 解析得到 nullptr 时应跳过该特征
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "nonexistent"),
    };

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    // 只有 coalOre 被处理，nonexistent 被跳过
    ASSERT_EQ(oresStep.features.size(), 1u);
    EXPECT_EQ(oresStep.features[0], coalOre);
}

TEST_F(FeatureSorterTest, EmptyStagesSkipped)
{
    auto& registry = PlacedFeatureRegistry::instance();

    // 只在 VegetalDecoration (stage=9) 注册特征，跳过中间所有空阶段
    const PlacedFeature* oakTree = registerPlacedStub(registry, "oak_tree", DecorationStage::VegetalDecoration);

    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {ResourceLocation("test", "oak_tree")};

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // result 大小 = maxStep + 1 = 9 + 1 = 10
    ASSERT_EQ(result.size(), 10u);

    // 只有 VegetalDecoration 有特征
    for (int i = 0; i < 9; ++i) {
        if (i != static_cast<int>(DecorationStage::VegetalDecoration)) {
            EXPECT_TRUE(result[i].empty()) << "Stage " << i << " should be empty";
        }
    }

    auto& vegetalStep = result[static_cast<int>(DecorationStage::VegetalDecoration)];
    ASSERT_EQ(vegetalStep.features.size(), 1u);
    EXPECT_EQ(vegetalStep.features[0], oakTree);
}

// ============================================================================
// 跨阶段依赖边测试
// ============================================================================

TEST_F(FeatureSorterTest, CrossStageDependencyEdges)
{
    // 模拟真实的 Overworld 生物群系场景：
    // Lakes → UndergroundOres → VegetalDecoration → TopLayerModification
    // 验证跨阶段的依赖边确保了正确的排序
    auto& registry = PlacedFeatureRegistry::instance();

    const PlacedFeature* waterLake = registerPlacedStub(registry, "water_lake", DecorationStage::Lakes);
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* oakTree = registerPlacedStub(registry, "oak_tree", DecorationStage::VegetalDecoration);
    const PlacedFeature* freezeTop = registerPlacedStub(registry, "freeze_top", DecorationStage::TopLayerModification);

    // 单个生物群系，跨阶段依赖链: WaterLake→CoalOre→OakTree→FreezeTop
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {ResourceLocation("test", "water_lake")};
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {ResourceLocation("test", "coal_ore")};
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {ResourceLocation("test", "oak_tree")};
    biomeData[{0, static_cast<int>(DecorationStage::TopLayerModification)}] = {ResourceLocation("test", "freeze_top")};

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 验证每个阶段的特征存在且正确
    ASSERT_GE(result.size(), 11u);

    EXPECT_EQ(result[1].features.size(), 1u);  // Lakes
    EXPECT_EQ(result[6].features.size(), 1u);  // UndergroundOres
    EXPECT_EQ(result[9].features.size(), 1u);  // VegetalDecoration
    EXPECT_EQ(result[10].features.size(), 1u); // TopLayerModification

    EXPECT_EQ(result[1].features[0], waterLake);
    EXPECT_EQ(result[6].features[0], coalOre);
    EXPECT_EQ(result[9].features[0], oakTree);
    EXPECT_EQ(result[10].features[0], freezeTop);
}

TEST_F(FeatureSorterTest, MultiBiomeCrossStageNoFalseCycle)
{
    // 回归测试：模拟之前 bug 导致的虚假环
    // 两个生物群系，一个有 Lakes+Ores，另一个只有 Ores
    auto& registry = PlacedFeatureRegistry::instance();

    const PlacedFeature* waterLake = registerPlacedStub(registry, "water_lake", DecorationStage::Lakes);
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* ironOre = registerPlacedStub(registry, "iron_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* oakTree = registerPlacedStub(registry, "oak_tree", DecorationStage::VegetalDecoration);

    // 生物群系 0 (Plains): Lakes + Ores + Vegetal
    // 生物群系 1 (River): Ores + Vegetal (无 Lakes)
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {ResourceLocation("test", "water_lake")};
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "iron_ore"),
    };
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {ResourceLocation("test", "oak_tree")};

    biomeData[{1, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "iron_ore"),
    };
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {ResourceLocation("test", "oak_tree")};

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    auto& lakesStep = result[static_cast<int>(DecorationStage::Lakes)];
    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    auto& vegetalStep = result[static_cast<int>(DecorationStage::VegetalDecoration)];

    ASSERT_EQ(lakesStep.features.size(), 1u);
    ASSERT_EQ(oresStep.features.size(), 2u);
    ASSERT_EQ(vegetalStep.features.size(), 1u);

    // Lakes 阶段只有 WaterLake
    EXPECT_EQ(lakesStep.features[0], waterLake);

    // Ores 阶段保持顺序: Coal → Iron
    EXPECT_EQ(oresStep.features[0], coalOre);
    EXPECT_EQ(oresStep.features[1], ironOre);

    // Vegetal 阶段只有 Oak
    EXPECT_EQ(vegetalStep.features[0], oakTree);
}

TEST_F(FeatureSorterTest, RealWorldOverworldScenario)
{
    // 模拟简化的真实主世界场景：
    // Plains (有 Lakes+Ores+Vegetal+FreezeTop) 和
    // Ocean (无 Lakes+Ores+Vegetal，无 FreezeTop)
    // 关键：Ocean 没有 Lakes 和 TopLayerModification，
    // 但与 Plains 共享 Ores 特征
    auto& registry = PlacedFeatureRegistry::instance();

    // Lakes
    const PlacedFeature* waterLake = registerPlacedStub(registry, "water_lake", DecorationStage::Lakes);
    const PlacedFeature* lavaLake = registerPlacedStub(registry, "lava_lake", DecorationStage::Lakes);

    // UndergroundOres
    const PlacedFeature* coalOre = registerPlacedStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* ironOre = registerPlacedStub(registry, "iron_ore", DecorationStage::UndergroundOres);
    const PlacedFeature* goldOre = registerPlacedStub(registry, "gold_ore", DecorationStage::UndergroundOres);

    // VegetalDecoration
    const PlacedFeature* oakTree = registerPlacedStub(registry, "oak_tree", DecorationStage::VegetalDecoration);
    const PlacedFeature* seagrass = registerPlacedStub(registry, "seagrass", DecorationStage::VegetalDecoration);

    // TopLayerModification
    const PlacedFeature* freezeTop = registerPlacedStub(registry, "freeze_top", DecorationStage::TopLayerModification);

    // Plains (biome 0): Lakes + Ores + Vegetal + Freeze
    // Ocean  (biome 1): Ores + Vegetal (no Lakes, no Freeze)
    std::map<std::pair<BiomeId, int>, std::vector<ResourceLocation>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {
        ResourceLocation("test", "water_lake"),
        ResourceLocation("test", "lava_lake"),
    };
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "iron_ore"),
        ResourceLocation("test", "gold_ore"),
    };
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {ResourceLocation("test", "oak_tree")};
    biomeData[{0, static_cast<int>(DecorationStage::TopLayerModification)}] = {ResourceLocation("test", "freeze_top")};

    biomeData[{1, static_cast<int>(DecorationStage::UndergroundOres)}] = {
        ResourceLocation("test", "coal_ore"),
        ResourceLocation("test", "iron_ore"),
        ResourceLocation("test", "gold_ore"),
    };
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {ResourceLocation("test", "seagrass")};

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 验证无虚假环——所有特征都在正确的阶段
    auto& lakesStep = result[static_cast<int>(DecorationStage::Lakes)];
    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    auto& vegetalStep = result[static_cast<int>(DecorationStage::VegetalDecoration)];
    auto& freezeStep = result[static_cast<int>(DecorationStage::TopLayerModification)];

    ASSERT_EQ(lakesStep.features.size(), 2u);
    ASSERT_EQ(oresStep.features.size(), 3u);
    ASSERT_EQ(vegetalStep.features.size(), 2u);
    ASSERT_EQ(freezeStep.features.size(), 1u);

    // Lakes 阶段只有 water/lake
    EXPECT_EQ(lakesStep.features[0], waterLake);
    EXPECT_EQ(lakesStep.features[1], lavaLake);

    // Ores 保持顺序
    EXPECT_EQ(oresStep.features[0], coalOre);
    EXPECT_EQ(oresStep.features[1], ironOre);
    EXPECT_EQ(oresStep.features[2], goldOre);

    // Vegetal 阶段包含 Plains 的 Oak 和 Ocean 的 Seagrass
    EXPECT_EQ(vegetalStep.features.size(), 2u);

    // Freeze 只有 Plains 的
    EXPECT_EQ(freezeStep.features[0], freezeTop);

    // 每个 Ores 特征的 indexMapping 正确
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "coal_ore")), 0);
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "iron_ore")), 1);
    EXPECT_EQ(oresStep.getIndex(ResourceLocation("test", "gold_ore")), 2);
}
