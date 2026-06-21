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
 */

#include "../src/common/world/gen/feature/FeatureSorter.hpp"
#include "../src/common/core/Types.hpp"
#include "../src/common/world/gen/feature/ConfiguredFeature.hpp"
#include "../src/common/world/gen/feature/DecorationStage.hpp"
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
 * FeatureSorter 只使用特征对象的指针身份和 featureId，
 * 不调用 place() 方法。
 */
class StubFeature : public ConfiguredFeatureBase {
public:
    StubFeature(const char* n, DecorationStage s)
        : m_name(n)
        , m_stage(s)
    {}

    bool place(WorldGenRegion&, ChunkPrimer&, IChunkGenerator&, math::Random&, const BlockPos&) override
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
 * @brief 注册一个 stub 特征到 FeatureRegistry
 * @param registry 特征注册表
 * @param name 特征名称
 * @param stage 装饰阶段
 * @return 注册后的特征指针（由 registry 拥有所有权）
 */
static ConfiguredFeatureBase* registerStub(FeatureRegistry& registry, const char* name, DecorationStage stage)
{
    auto feature = std::make_unique<StubFeature>(name, stage);
    ConfiguredFeatureBase* ptr = feature.get();
    registry.registerFeature(std::move(feature), stage);
    return ptr;
}

/**
 * @brief 测试夹具：自动清除 FeatureRegistry
 */
class FeatureSorterTest : public ::testing::Test {
protected:
    void SetUp() override { FeatureRegistry::instance().clear(); }

    void TearDown() override { FeatureRegistry::instance().clear(); }

    /**
     * @brief 构建 getFeatures 回调函数
     *
     * 使用 map<(BiomeId, stageIndex), vector<fid>> 存储测试数据，
     * 返回的 lambda 返回对应 vector 的 const 引用。
     */
    static std::function<const std::vector<u32>&(BiomeId, DecorationStage)> makeGetFeatures(
        std::map<std::pair<BiomeId, int>, std::vector<u32>>& data)
    {
        return [&data](BiomeId biomeId, DecorationStage stage) -> const std::vector<u32>& {
            static const std::vector<u32> empty;
            auto it = data.find({biomeId, static_cast<int>(stage)});
            return it != data.end() ? it->second : empty;
        };
    }
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
    // 创建 3 个 stub 特征（不需要注册到 registry，仅测试 StepFeatureData 构造）
    auto f1 = std::make_unique<StubFeature>("feature_a", DecorationStage::UndergroundOres);
    auto f2 = std::make_unique<StubFeature>("feature_b", DecorationStage::UndergroundOres);
    auto f3 = std::make_unique<StubFeature>("feature_c", DecorationStage::UndergroundOres);

    std::vector<ConfiguredFeatureBase*> features = {f1.get(), f2.get(), f3.get()};
    std::vector<u32> featureIds = {10, 20, 30};

    FeatureSorter::StepFeatureData data(features, featureIds);

    EXPECT_FALSE(data.empty());
    EXPECT_EQ(data.features.size(), 3u);
    EXPECT_EQ(data.features[0], f1.get());
    EXPECT_EQ(data.features[1], f2.get());
    EXPECT_EQ(data.features[2], f3.get());

    // indexMapping: featureId -> sortedIndex
    EXPECT_EQ(data.getIndex(10), 0);
    EXPECT_EQ(data.getIndex(20), 1);
    EXPECT_EQ(data.getIndex(30), 2);
}

TEST_F(FeatureSorterTest, StepFeatureDataGetIndexNotFound)
{
    auto f1 = std::make_unique<StubFeature>("feature_a", DecorationStage::UndergroundOres);
    std::vector<ConfiguredFeatureBase*> features = {f1.get()};
    std::vector<u32> featureIds = {5};

    FeatureSorter::StepFeatureData data(features, featureIds);

    EXPECT_EQ(data.getIndex(5), 0);
    EXPECT_EQ(data.getIndex(999), -1); // 不存在的 featureId
    EXPECT_EQ(data.getIndex(0), -1);   // 不存在的 featureId
}

// ============================================================================
// 基础排序测试
// ============================================================================

TEST_F(FeatureSorterTest, EmptyBiomesList)
{
    auto& registry = FeatureRegistry::instance();
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    auto getFeatures = makeGetFeatures(biomeData);

    std::vector<BiomeId> biomes; // 空
    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 空生物群系列表时 maxStep 为 0，result.resize(1) 产生一个空的 StepFeatureData
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(result[0].empty());
}

TEST_F(FeatureSorterTest, SingleBiomeSingleStageSingleFeature)
{
    auto& registry = FeatureRegistry::instance();

    // 注册 1 个特征到 UndergroundOres 阶段
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);

    // 生物群系 0 在 UndergroundOres 有 1 个特征
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0}; // fid=0

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // result 大小 = maxStep + 1 = 6 + 1 = 7 (UndergroundOres 是 stage 6)
    ASSERT_EQ(result.size(), 7u);

    // UndergroundOres 步骤应有 1 个特征
    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    ASSERT_EQ(oresStep.features.size(), 1u);
    EXPECT_EQ(oresStep.features[0], coalOre);
    EXPECT_EQ(oresStep.getIndex(0), 0); // fid=0 在排序后索引为 0

    // 其他步骤应为空（result 大小为 maxStep+1=7，索引 0-6）
    EXPECT_TRUE(result[static_cast<int>(DecorationStage::Lakes)].empty());
    // VegetalDecoration (stage=9) 超出 result 大小范围，不检查
}

TEST_F(FeatureSorterTest, SingleBiomeMultipleFeaturesSameStage)
{
    auto& registry = FeatureRegistry::instance();

    // 注册 3 个特征到 UndergroundOres 阶段
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    auto* ironOre = registerStub(registry, "iron_ore", DecorationStage::UndergroundOres);
    auto* goldOre = registerStub(registry, "gold_ore", DecorationStage::UndergroundOres);

    // 生物群系 0 在 UndergroundOres 有 3 个特征
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 1, 2};

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
    EXPECT_EQ(oresStep.getIndex(0), 0); // coalOre fid=0
    EXPECT_EQ(oresStep.getIndex(1), 1); // ironOre fid=1
    EXPECT_EQ(oresStep.getIndex(2), 2); // goldOre fid=2
}

TEST_F(FeatureSorterTest, SingleBiomeMultipleStages)
{
    auto& registry = FeatureRegistry::instance();

    // Lakes 阶段: WaterLake(fid=0), LavaLake(fid=1)
    auto* waterLake = registerStub(registry, "water_lake", DecorationStage::Lakes);
    auto* lavaLake = registerStub(registry, "lava_lake", DecorationStage::Lakes);

    // UndergroundOres 阶段: CoalOre(fid=0), IronOre(fid=1)
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    auto* ironOre = registerStub(registry, "iron_ore", DecorationStage::UndergroundOres);

    // VegetalDecoration 阶段: OakTree(fid=0)
    auto* oakTree = registerStub(registry, "oak_tree", DecorationStage::VegetalDecoration);

    // 生物群系 0: Lakes + UndergroundOres + VegetalDecoration
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {0, 1};
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 1};
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0};

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
    auto& registry = FeatureRegistry::instance();

    // 注册特征到不同阶段
    auto* waterLake = registerStub(registry, "water_lake", DecorationStage::Lakes);         // stage 1
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);   // stage 6
    auto* oakTree = registerStub(registry, "oak_tree", DecorationStage::VegetalDecoration); // stage 9
    auto* freeze = registerStub(registry, "freeze", DecorationStage::TopLayerModification); // stage 10

    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {0};
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0};
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0};
    biomeData[{0, static_cast<int>(DecorationStage::TopLayerModification)}] = {0};

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
    EXPECT_EQ(result[1].getIndex(0), 0);
    EXPECT_EQ(result[6].getIndex(0), 0);
    EXPECT_EQ(result[9].getIndex(0), 0);
    EXPECT_EQ(result[10].getIndex(0), 0);
}

// ============================================================================
// 跨生物群系测试
// ============================================================================

TEST_F(FeatureSorterTest, TwoBiomesSharedFeatures)
{
    auto& registry = FeatureRegistry::instance();

    // 注册 3 个矿石特征
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    auto* ironOre = registerStub(registry, "iron_ore", DecorationStage::UndergroundOres);
    auto* goldOre = registerStub(registry, "gold_ore", DecorationStage::UndergroundOres);

    // 生物群系 0 和 1 共享 coalOre 和 ironOre
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 1};    // coal, iron
    biomeData[{1, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 1, 2}; // coal, iron, gold

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
    EXPECT_EQ(oresStep.getIndex(0), 0); // coalOre
    EXPECT_EQ(oresStep.getIndex(1), 1); // ironOre
    EXPECT_EQ(oresStep.getIndex(2), 2); // goldOre
}

TEST_F(FeatureSorterTest, TwoBiomesSharedFeaturesInSameStage)
{
    auto& registry = FeatureRegistry::instance();

    // VegetalDecoration 阶段特征
    auto* oakTree = registerStub(registry, "oak_tree", DecorationStage::VegetalDecoration);
    auto* birchTree = registerStub(registry, "birch_tree", DecorationStage::VegetalDecoration);
    auto* spruceTree = registerStub(registry, "spruce_tree", DecorationStage::VegetalDecoration);
    auto* plainsGrass = registerStub(registry, "plains_grass", DecorationStage::VegetalDecoration);

    // 生物群系 0 (Plains): oak + grass
    // 生物群系 1 (Forest): oak + birch + spruce
    // oakTree 被两个生物群系共享
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0, 3};    // oak, grass
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0, 1, 2}; // oak, birch, spruce

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    auto& vegetalStep = result[static_cast<int>(DecorationStage::VegetalDecoration)];
    ASSERT_EQ(vegetalStep.features.size(), 4u);

    // oakTree 应该排在 birchTree/spruceTree 之前（来自 Forest 的边: oak→birch→spruce）
    // oakTree 也应该在 plainsGrass 之前（来自 Plains 的边: oak→grass）
    // 验证 oakTree 的索引小于 birchTree 和 plainsGrass
    i32 oakIdx = vegetalStep.getIndex(0);
    i32 birchIdx = vegetalStep.getIndex(1);
    i32 spruceIdx = vegetalStep.getIndex(2);
    i32 grassIdx = vegetalStep.getIndex(3);

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
    // 这是修复 featureIdToGlobalIndex 使用 fid 作为 key 的 bug 的核心测试
    auto& registry = FeatureRegistry::instance();

    // Lakes 阶段: fid=0 是 WaterLake
    auto* waterLake = registerStub(registry, "water_lake", DecorationStage::Lakes);

    // UndergroundOres 阶段: fid=0 是 CoalOre（与 Lakes 的 fid=0 相同！）
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);

    // TopLayerModification 阶段: fid=0 是 FreezeTopLayer
    auto* freezeTop = registerStub(registry, "freeze_top", DecorationStage::TopLayerModification);

    // 生物群系 0 拥有所有三个阶段的特征
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {0};
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0};
    biomeData[{0, static_cast<int>(DecorationStage::TopLayerModification)}] = {0};

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 关键断言：每个阶段的特征应该是不同的对象
    // 之前 bug: fid=0 在三个阶段都被映射到同一个 globalIndex，导致它们被合并为一个节点
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

    // indexMapping 应正确：每个阶段的 fid=0 映射到索引 0
    EXPECT_EQ(lakesStep.getIndex(0), 0);
    EXPECT_EQ(oresStep.getIndex(0), 0);
    EXPECT_EQ(freezeStep.getIndex(0), 0);
}

// ============================================================================
// 环检测与特殊场景
// ============================================================================

TEST_F(FeatureSorterTest, CycleDetectionDoesNotCrash)
{
    auto& registry = FeatureRegistry::instance();

    // 构造人工环：两个特征在两个生物群系中出现顺序相反
    // 生物群系 A: featureA → featureB  (A 在 B 前)
    // 生物群系 B: featureB → featureA  (B 在 A 前) → 环！
    auto* featureA = registerStub(registry, "feature_a", DecorationStage::VegetalDecoration);
    auto* featureB = registerStub(registry, "feature_b", DecorationStage::VegetalDecoration);

    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0, 1}; // A → B
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {1, 0}; // B → A (环!)

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    // 不应崩溃
    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 应该返回结果（可能不完整，但不崩溃）
    auto& vegetalStep = result[static_cast<int>(DecorationStage::VegetalDecoration)];
    // 有环时，DFS 会跳过循环节点但继续处理
    // 至少应该有一些结果
    EXPECT_GE(vegetalStep.features.size(), 1u);
}

TEST_F(FeatureSorterTest, CycleResultStillContainsFeatures)
{
    auto& registry = FeatureRegistry::instance();

    // 构造一个场景：A→B (生物群系 0) 和 B→A (生物群系 1) 形成环
    // 加上 C 只在生物群系 0 中，不参与环
    auto* featureA = registerStub(registry, "feature_a", DecorationStage::VegetalDecoration);
    auto* featureB = registerStub(registry, "feature_b", DecorationStage::VegetalDecoration);
    auto* featureC = registerStub(registry, "feature_c", DecorationStage::VegetalDecoration);

    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0, 1, 2}; // A → B → C
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {1, 0};    // B → A (环)

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    auto& vegetalStep = result[static_cast<int>(DecorationStage::VegetalDecoration)];
    // 即使有环，结果不应为空，非环节点仍然应出现在排序结果中
    EXPECT_GE(vegetalStep.features.size(), 1u);

    // featureC 不参与环，应该能正常排序
    // 它可能出现在结果中（取决于 DFS 处理顺序）
    bool hasFeatureC = false;
    for (auto* f : vegetalStep.features) {
        if (f == featureC) {
            hasFeatureC = true;
            break;
        }
    }
    EXPECT_TRUE(hasFeatureC);
}

TEST_F(FeatureSorterTest, NullFeatureSkipped)
{
    auto& registry = FeatureRegistry::instance();

    // 注册一个有效特征
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);

    // 手动在 registry 的 UndergroundOres 阶段添加一个 null 槽位
    // 注意: registerFeature 不会添加 null，所以我们通过 biomeData 引用越界 fid 来测试
    // 当 fid >= allFeatures.size() 时，buildFeaturesPerStep 会跳过该特征
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 99}; // fid=99 不存在

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    auto& oresStep = result[static_cast<int>(DecorationStage::UndergroundOres)];
    // 只有 fid=0 (coalOre) 被处理，fid=99 被跳过
    ASSERT_EQ(oresStep.features.size(), 1u);
    EXPECT_EQ(oresStep.features[0], coalOre);
}

TEST_F(FeatureSorterTest, EmptyStagesSkipped)
{
    auto& registry = FeatureRegistry::instance();

    // 只在 VegetalDecoration (stage=9) 注册特征，跳过中间所有空阶段
    auto* oakTree = registerStub(registry, "oak_tree", DecorationStage::VegetalDecoration);

    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0};

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
    auto& registry = FeatureRegistry::instance();

    auto* waterLake = registerStub(registry, "water_lake", DecorationStage::Lakes);
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    auto* oakTree = registerStub(registry, "oak_tree", DecorationStage::VegetalDecoration);
    auto* freezeTop = registerStub(registry, "freeze_top", DecorationStage::TopLayerModification);

    // 单个生物群系，跨阶段依赖链: WaterLake→CoalOre→OakTree→FreezeTop
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {0};
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0};
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0};
    biomeData[{0, static_cast<int>(DecorationStage::TopLayerModification)}] = {0};

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
    // 之前 fid 碰撞导致 WaterLake 和 CoalOre 共享 globalIndex，产生虚假环
    auto& registry = FeatureRegistry::instance();

    auto* waterLake = registerStub(registry, "water_lake", DecorationStage::Lakes);
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    auto* ironOre = registerStub(registry, "iron_ore", DecorationStage::UndergroundOres);
    auto* oakTree = registerStub(registry, "oak_tree", DecorationStage::VegetalDecoration);

    // 生物群系 0 (Plains): Lakes + Ores + Vegetal
    // 生物群系 1 (River): Ores + Vegetal (无 Lakes)
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {0};              // WaterLake
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 1}; // Coal, Iron
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0};  // Oak

    biomeData[{1, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 1}; // Coal, Iron
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0};  // Oak

    auto getFeatures = makeGetFeatures(biomeData);
    std::vector<BiomeId> biomes = {0, 1};

    auto result = FeatureSorter::buildFeaturesPerStep(biomes, getFeatures, registry);

    // 之前 bug: WaterLake (Lakes fid=0) 和 CoalOre (Ores fid=0) 共享 globalIndex
    // 导致 WaterLake→LavaLake 和 CoalOre→IronOre 边被合并，产生虚假环
    // 修复后不应有环，所有特征应正确排序

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
    auto& registry = FeatureRegistry::instance();

    // Lakes
    auto* waterLake = registerStub(registry, "water_lake", DecorationStage::Lakes);
    auto* lavaLake = registerStub(registry, "lava_lake", DecorationStage::Lakes);

    // UndergroundOres
    auto* coalOre = registerStub(registry, "coal_ore", DecorationStage::UndergroundOres);
    auto* ironOre = registerStub(registry, "iron_ore", DecorationStage::UndergroundOres);
    auto* goldOre = registerStub(registry, "gold_ore", DecorationStage::UndergroundOres);

    // VegetalDecoration
    auto* oakTree = registerStub(registry, "oak_tree", DecorationStage::VegetalDecoration);
    auto* seagrass = registerStub(registry, "seagrass", DecorationStage::VegetalDecoration);

    // TopLayerModification
    auto* freezeTop = registerStub(registry, "freeze_top", DecorationStage::TopLayerModification);

    // Plains (biome 0): Lakes + Ores + Vegetal + Freeze
    // Ocean  (biome 1): Ores + Vegetal (no Lakes, no Freeze)
    std::map<std::pair<BiomeId, int>, std::vector<u32>> biomeData;
    biomeData[{0, static_cast<int>(DecorationStage::Lakes)}] = {0, 1};              // Water, Lava
    biomeData[{0, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 1, 2}; // Coal, Iron, Gold
    biomeData[{0, static_cast<int>(DecorationStage::VegetalDecoration)}] = {0};     // Oak
    biomeData[{0, static_cast<int>(DecorationStage::TopLayerModification)}] = {0};  // Freeze

    biomeData[{1, static_cast<int>(DecorationStage::UndergroundOres)}] = {0, 1, 2}; // Coal, Iron, Gold
    biomeData[{1, static_cast<int>(DecorationStage::VegetalDecoration)}] = {1};     // Seagrass

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
    // 拓扑排序: Oak 在 Plains 的链中位于 Ores 之后
    // Seagrass 在 Ocean 的链中也位于 Ores 之后
    // 两者都满足约束，具体顺序取决于 DFS 遍历顺序
    EXPECT_EQ(vegetalStep.features.size(), 2u);

    // Freeze 只有 Plains 的
    EXPECT_EQ(freezeStep.features[0], freezeTop);

    // 每个 Ores 特征的 indexMapping 正确
    EXPECT_EQ(oresStep.getIndex(0), 0); // CoalOre
    EXPECT_EQ(oresStep.getIndex(1), 1); // IronOre
    EXPECT_EQ(oresStep.getIndex(2), 2); // GoldOre
}
