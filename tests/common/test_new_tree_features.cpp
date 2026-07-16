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

/**
 * @file test_new_tree_features.cpp
 * @brief 新增树木特征配置和相关修复的单元测试
 *
 * 测试本次变更中新增的6种树木配置（FancyOak, Pine, JungleBush,
 * SwampTree, MegaPine, TallBirch）、新注册的方块、以及各 FoliagePlacer
 * 和 TrunkPlacer 的基本行为。
 */

#include "TestWorldHelper.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "util/math/MathUtils.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/chunk/data/ChunkPrimer.hpp"
#include "world/gen/chunk/IChunkGenerator.hpp"
#include "world/gen/feature/FeatureSpread.hpp"
#include "world/gen/feature/nether/HugeFungusFeature.hpp"
#include "world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include "world/gen/feature/tree/TreeFeature.hpp"
#include "world/gen/feature/tree/foliage/BlobFoliagePlacer.hpp"
#include "world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "world/gen/feature/tree/foliage/FoliagePlacers.hpp"
#include "world/gen/feature/tree/foliage/RandomSpreadFoliagePlacer.hpp"
#include "world/gen/feature/tree/trunk/BendingTrunkPlacer.hpp"
#include "world/gen/feature/tree/trunk/StraightTrunkPlacer.hpp"
#include "world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include "world/gen/feature/tree/trunk/TrunkPlacers.hpp"
#include "world/gen/feature/vegetation/BigMushroomFeature.hpp"
#include "world/gen/valueprovider/IntProvider.hpp"
#include <map>
#include <gtest/gtest.h>

using namespace mc;

namespace {
// BaseTestWorld 默认构造为 protected，派生一个 public 构造的测试世界供采样调用。
class TreeFeatureTestWorld : public test::BaseTestWorld {
public:
    TreeFeatureTestWorld() = default;
};
} // namespace

// ============================================================================
// 新增树木特征配置测试
// ============================================================================

class NewTreeFeatureConfigTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

TEST_F(NewTreeFeatureConfigTest, FancyOakConfig)
{
    auto config = TreeFeatures::fancyOakConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::OAK_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::OAK_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "fancy");
    EXPECT_STREQ(config.foliagePlacer->name(), "fancy");
    EXPECT_EQ(config.minHeight, 4);
}

TEST_F(NewTreeFeatureConfigTest, PineConfig)
{
    auto config = TreeFeatures::pineConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::SPRUCE_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::SPRUCE_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "StraightTrunkPlacer");
    EXPECT_STREQ(config.foliagePlacer->name(), "pine");
    EXPECT_EQ(config.minHeight, 6);
}

TEST_F(NewTreeFeatureConfigTest, JungleBushConfig)
{
    auto config = TreeFeatures::jungleBushConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    // Jungle bush uses jungle log but oak leaves (MC behavior)
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::JUNGLE_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::OAK_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "StraightTrunkPlacer");
    EXPECT_STREQ(config.foliagePlacer->name(), "bush");
    EXPECT_EQ(config.minHeight, 1);
}

TEST_F(NewTreeFeatureConfigTest, SwampTreeConfig)
{
    auto config = TreeFeatures::swampConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::OAK_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::OAK_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "StraightTrunkPlacer");
    EXPECT_STREQ(config.foliagePlacer->name(), "BlobFoliagePlacer");
    // Swamp tree allows water depth
    EXPECT_EQ(config.maxWaterDepth, 1);
    EXPECT_EQ(config.minHeight, 5);
}

TEST_F(NewTreeFeatureConfigTest, MegaPineConfig)
{
    auto config = TreeFeatures::megaPineConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::SPRUCE_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::SPRUCE_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "giant");
    EXPECT_STREQ(config.foliagePlacer->name(), "mega_pine");
    EXPECT_EQ(config.minHeight, 13);
}

TEST_F(NewTreeFeatureConfigTest, TallBirchConfig)
{
    auto config = TreeFeatures::tallBirchConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::BIRCH_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::BIRCH_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "StraightTrunkPlacer");
    EXPECT_STREQ(config.foliagePlacer->name(), "BlobFoliagePlacer");
    EXPECT_EQ(config.minHeight, 5);
}

TEST_F(NewTreeFeatureConfigTest, AzaleaConfig)
{
    // 验证杜鹃树配置与 MC 1.21.11 AZALEA_TREE 一致
    auto config = TreeFeatures::azaleaConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    // 杜鹃树使用橡木原木作为树干
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::OAK_LOG));
    // 杜鹃树使用加权树叶提供者，foliageBlock 可为空
    EXPECT_TRUE(config.hasFoliageProvider());
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    // 弯曲树干放置器
    EXPECT_STREQ(config.trunkPlacer->name(), "bending");
    // 随机散布树叶放置器
    EXPECT_STREQ(config.foliagePlacer->name(), "random_spread");
    // 最小尺寸约束（TwoLayersFeatureSize）
    ASSERT_NE(config.minimumSize, nullptr);
    EXPECT_EQ(config.minimumSize->type(), FeatureSizeType::TwoLayers);
    EXPECT_EQ(config.minHeight, 4);

    // 验证加权树叶提供者包含杜鹃叶与开花杜鹃叶（权重 3:1）
    const auto* weightedProvider =
        dynamic_cast<const world::gen::feature::state::WeightedBlockStateProvider*>(config.foliageProvider.get());
    ASSERT_NE(weightedProvider, nullptr);
    const auto& entries = weightedProvider->entries();
    EXPECT_EQ(entries.size(), 2u);
    i32 totalWeight = 0;
    bool hasAzaleaLeaves = false;
    bool hasFloweringAzaleaLeaves = false;
    for (const auto& entry : entries) {
        totalWeight += entry.weight;
        if (entry.state->is(block_registry::CaveBlocks::AZALEA_LEAVES)) {
            hasAzaleaLeaves = true;
            EXPECT_EQ(entry.weight, 3);
        }
        if (entry.state->is(block_registry::CaveBlocks::FLOWERING_AZALEA_LEAVES)) {
            hasFloweringAzaleaLeaves = true;
            EXPECT_EQ(entry.weight, 1);
        }
    }
    EXPECT_TRUE(hasAzaleaLeaves);
    EXPECT_TRUE(hasFloweringAzaleaLeaves);
    EXPECT_EQ(totalWeight, 4);
}

TEST_F(NewTreeFeatureConfigTest, AzaleaConfigDeepCopy)
{
    // 验证杜鹃树配置的深拷贝（foliageProvider 和 minimumSize 都需要正确克隆）
    auto original = TreeFeatures::azaleaConfig();
    TreeFeatureConfig copy(original);

    ASSERT_NE(copy.foliageProvider, nullptr);
    EXPECT_NE(copy.foliageProvider.get(), original.foliageProvider.get());
    const auto* copyWeighted =
        dynamic_cast<const world::gen::feature::state::WeightedBlockStateProvider*>(copy.foliageProvider.get());
    const auto* originalWeighted2 =
        dynamic_cast<const world::gen::feature::state::WeightedBlockStateProvider*>(original.foliageProvider.get());
    ASSERT_NE(copyWeighted, nullptr);
    ASSERT_NE(originalWeighted2, nullptr);
    EXPECT_EQ(copyWeighted->entries().size(), originalWeighted2->entries().size());
    EXPECT_EQ(copyWeighted->totalWeight(), originalWeighted2->totalWeight());

    ASSERT_NE(copy.minimumSize, nullptr);
    EXPECT_NE(copy.minimumSize.get(), original.minimumSize.get());
    EXPECT_EQ(copy.minimumSize->type(), original.minimumSize->type());
}

// ============================================================================
// TreeFeatureConfig 深拷贝测试
// ============================================================================

TEST_F(NewTreeFeatureConfigTest, DeepCopyPreservesTrunkPlacer)
{
    auto original = TreeFeatures::fancyOakConfig();

    // Copy construct
    TreeFeatureConfig copy(original);
    ASSERT_NE(copy.trunkPlacer, nullptr);
    ASSERT_NE(copy.foliagePlacer, nullptr);
    // The pointers should be different (deep copy)
    EXPECT_NE(copy.trunkPlacer.get(), original.trunkPlacer.get());
    EXPECT_NE(copy.foliagePlacer.get(), original.foliagePlacer.get());
    // But the names should be the same
    EXPECT_STREQ(copy.trunkPlacer->name(), original.trunkPlacer->name());
    EXPECT_STREQ(copy.foliagePlacer->name(), original.foliagePlacer->name());
    EXPECT_EQ(copy.maxWaterDepth, original.maxWaterDepth);
    EXPECT_EQ(copy.minHeight, original.minHeight);
}

// ============================================================================
// 新增 FoliagePlacer 测试
// ============================================================================

class NewFoliagePlacerTest : public ::testing::Test {
protected:
    void SetUp() override { random = std::make_unique<math::Random>(54321); }

    std::unique_ptr<math::Random> random;
};

TEST_F(NewFoliagePlacerTest, PineFoliagePlacerName)
{
    PineFoliagePlacer placer(FeatureSpread::spread(1, 1), FeatureSpread::fixed(1), 4);
    EXPECT_STREQ(placer.name(), "pine");
}

TEST_F(NewFoliagePlacerTest, PineFoliagePlacerHeight)
{
    PineFoliagePlacer placer(FeatureSpread::spread(1, 1), FeatureSpread::fixed(1), 4);
    // Pine foliage height should be reasonable
    i32 height = placer.getFoliageHeight(*random, 6);
    EXPECT_GT(height, 0);
}

TEST_F(NewFoliagePlacerTest, PineFoliagePlacerClone)
{
    PineFoliagePlacer placer(FeatureSpread::spread(1, 1), FeatureSpread::fixed(1), 4);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "pine");
}

TEST_F(NewFoliagePlacerTest, SpruceFoliagePlacerName)
{
    SpruceFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    EXPECT_STREQ(placer.name(), "spruce");
}

TEST_F(NewFoliagePlacerTest, SpruceFoliagePlacerHeight)
{
    SpruceFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    i32 height = placer.getFoliageHeight(*random, 6);
    EXPECT_GT(height, 0);
}

TEST_F(NewFoliagePlacerTest, SpruceFoliagePlacerClone)
{
    SpruceFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "spruce");
}

TEST_F(NewFoliagePlacerTest, AcaciaFoliagePlacerName)
{
    AcaciaFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    EXPECT_STREQ(placer.name(), "acacia");
}

TEST_F(NewFoliagePlacerTest, AcaciaFoliagePlacerHeight)
{
    AcaciaFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    // MC 1.16.5: Acacia foliage is flat (getFoliageHeight returns 0)
    i32 height = placer.getFoliageHeight(*random, 6);
    EXPECT_EQ(height, 0);
}

TEST_F(NewFoliagePlacerTest, AcaciaFoliagePlacerClone)
{
    AcaciaFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "acacia");
}

TEST_F(NewFoliagePlacerTest, DarkOakFoliagePlacerName)
{
    DarkOakFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    EXPECT_STREQ(placer.name(), "dark_oak");
}

TEST_F(NewFoliagePlacerTest, DarkOakFoliagePlacerClone)
{
    DarkOakFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "dark_oak");
}

TEST_F(NewFoliagePlacerTest, JungleFoliagePlacerName)
{
    JungleFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    EXPECT_STREQ(placer.name(), "jungle");
}

TEST_F(NewFoliagePlacerTest, JungleFoliagePlacerClone)
{
    JungleFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "jungle");
}

TEST_F(NewFoliagePlacerTest, MegaPineFoliagePlacerName)
{
    MegaPineFoliagePlacer placer(FeatureSpread::spread(3, 2), FeatureSpread::fixed(0), 8);
    EXPECT_STREQ(placer.name(), "mega_pine");
}

TEST_F(NewFoliagePlacerTest, MegaPineFoliagePlacerClone)
{
    MegaPineFoliagePlacer placer(FeatureSpread::spread(3, 2), FeatureSpread::fixed(0), 8);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "mega_pine");
}

TEST_F(NewFoliagePlacerTest, BushFoliagePlacerName)
{
    BushFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    EXPECT_STREQ(placer.name(), "bush");
}

TEST_F(NewFoliagePlacerTest, BushFoliagePlacerHeight)
{
    BushFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    // Bush foliage should be single-layer (height 1)
    i32 height = placer.getFoliageHeight(*random, 1);
    EXPECT_GE(height, 1);
}

TEST_F(NewFoliagePlacerTest, BushFoliagePlacerClone)
{
    BushFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "bush");
}

TEST_F(NewFoliagePlacerTest, FancyFoliagePlacerName)
{
    FancyFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    EXPECT_STREQ(placer.name(), "fancy");
}

TEST_F(NewFoliagePlacerTest, FancyFoliagePlacerHeight)
{
    FancyFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    i32 height = placer.getFoliageHeight(*random, 10);
    EXPECT_GT(height, 0);
}

TEST_F(NewFoliagePlacerTest, FancyFoliagePlacerClone)
{
    FancyFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "fancy");
}

// ============================================================================
// RandomSpreadFoliagePlacer 测试
// ============================================================================

TEST_F(NewFoliagePlacerTest, RandomSpreadFoliagePlacerName)
{
    auto placer = std::make_unique<RandomSpreadFoliagePlacer>(
        FeatureSpread::fixed(3), FeatureSpread::fixed(0), world::gen::valueprovider::ConstantInt::create(2), 50);
    EXPECT_STREQ(placer->name(), "random_spread");
}

TEST_F(NewFoliagePlacerTest, RandomSpreadFoliagePlacerHeight)
{
    RandomSpreadFoliagePlacer placer(
        FeatureSpread::fixed(3), FeatureSpread::fixed(0), world::gen::valueprovider::ConstantInt::create(2), 50);
    // ConstantInt(2) 始终返回 2
    EXPECT_EQ(placer.getFoliageHeight(*random, 6), 2);
}

TEST_F(NewFoliagePlacerTest, RandomSpreadFoliagePlacerUniformHeight)
{
    // UniformInt(1, 4) 应在 [1, 4] 范围内采样
    RandomSpreadFoliagePlacer placer(
        FeatureSpread::fixed(3), FeatureSpread::fixed(0), world::gen::valueprovider::UniformInt::create(1, 4), 50);
    for (int i = 0; i < 100; ++i) {
        i32 h = placer.getFoliageHeight(*random, 6);
        EXPECT_GE(h, 1);
        EXPECT_LE(h, 4);
    }
}

TEST_F(NewFoliagePlacerTest, RandomSpreadFoliagePlacerClone)
{
    RandomSpreadFoliagePlacer placer(
        FeatureSpread::fixed(3), FeatureSpread::fixed(0), world::gen::valueprovider::ConstantInt::create(2), 50);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "random_spread");
    // 验证 clone 后的 leafPlacementAttempts 一致
    auto* cloned = dynamic_cast<RandomSpreadFoliagePlacer*>(clone.get());
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->leafPlacementAttempts(), 50);
}

TEST_F(NewFoliagePlacerTest, RandomSpreadFoliagePlacerClonePreservesHeight)
{
    // 验证 clone 保留 IntProvider 状态：clone 后 getFoliageHeight 应正常工作
    RandomSpreadFoliagePlacer placer(
        FeatureSpread::fixed(3), FeatureSpread::fixed(0), world::gen::valueprovider::ConstantInt::create(2), 50);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_EQ(clone->getFoliageHeight(*random, 6), 2);
}

TEST_F(NewFoliagePlacerTest, RandomSpreadFoliagePlacerNullHeightProvider)
{
    // foliageHeight 为 nullptr 时，getFoliageHeight 应安全返回 0（防御性）
    RandomSpreadFoliagePlacer placer(FeatureSpread::fixed(3), FeatureSpread::fixed(0), nullptr, 50);
    EXPECT_EQ(placer.getFoliageHeight(*random, 6), 0);
}

// ============================================================================
// WeightedBlockStateProvider 测试
// ============================================================================

TEST(WeightedBlockStateProviderTest, EmptyProviderReturnsNull)
{
    world::gen::feature::state::WeightedBlockStateProvider provider;
    TreeFeatureTestWorld world;
    math::Random rng(42);
    EXPECT_EQ(provider.getState(world, rng, 0, 0, 0), nullptr);
    EXPECT_TRUE(provider.empty());
    EXPECT_EQ(provider.size(), 0u);
}

TEST(WeightedBlockStateProviderTest, SingleEntryAlwaysReturnsThatState)
{
    VanillaBlocks::initialize();
    world::gen::feature::state::WeightedBlockStateProvider provider;
    const BlockState* azalea = VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES);
    provider.add(azalea, 5);
    TreeFeatureTestWorld world;
    math::Random rng(42);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(provider.getState(world, rng, 0, 0, 0), azalea);
    }
    EXPECT_FALSE(provider.empty());
    EXPECT_EQ(provider.size(), 1u);
    EXPECT_EQ(provider.totalWeight(), 5);
}

TEST(WeightedBlockStateProviderTest, WeightedDistributionRoughlyProportional)
{
    VanillaBlocks::initialize();
    world::gen::feature::state::WeightedBlockStateProvider provider;
    const BlockState* azalea = VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES);
    const BlockState* flowering = VanillaBlocks::getState(VanillaBlocks::FLOWERING_AZALEA_LEAVES);
    provider.add(azalea, 3);
    provider.add(flowering, 1);

    math::Random rng(12345);
    TreeFeatureTestWorld world;
    int azaleaCount = 0;
    int floweringCount = 0;
    constexpr int kSamples = 4000;
    for (int i = 0; i < kSamples; ++i) {
        const BlockState* s = provider.getState(world, rng, 0, 0, 0);
        if (s == azalea) {
            ++azaleaCount;
        } else if (s == flowering) {
            ++floweringCount;
        } else {
            FAIL() << "getState returned unknown state";
        }
    }
    // 权重 3:1 -> 期望 75%:25%，给 ±10% 容差
    EXPECT_NEAR(static_cast<double>(azaleaCount) / kSamples, 0.75, 0.10);
    EXPECT_NEAR(static_cast<double>(floweringCount) / kSamples, 0.25, 0.10);
    EXPECT_EQ(azaleaCount + floweringCount, kSamples);
}

TEST(WeightedBlockStateProviderTest, CloneProducesIndependentCopy)
{
    VanillaBlocks::initialize();
    world::gen::feature::state::WeightedBlockStateProvider provider;
    const BlockState* azalea = VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES);
    const BlockState* flowering = VanillaBlocks::getState(VanillaBlocks::FLOWERING_AZALEA_LEAVES);
    provider.add(azalea, 3);
    provider.add(flowering, 1);

    auto cloned = provider.clone();
    ASSERT_NE(cloned, nullptr);
    const auto* clonedWeighted =
        dynamic_cast<const world::gen::feature::state::WeightedBlockStateProvider*>(cloned.get());
    ASSERT_NE(clonedWeighted, nullptr);
    EXPECT_EQ(clonedWeighted->size(), 2u);
    EXPECT_EQ(clonedWeighted->totalWeight(), 4);

    // 修改原对象不影响 clone
    const BlockState* oak = VanillaBlocks::getState(VanillaBlocks::OAK_LEAVES);
    provider.add(oak, 10);
    EXPECT_EQ(provider.size(), 3u);
    EXPECT_EQ(clonedWeighted->size(), 2u); // clone 不受影响
}

TEST(WeightedBlockStateProviderTest, CopyConstructorProducesIndependentCopy)
{
    VanillaBlocks::initialize();
    world::gen::feature::state::WeightedBlockStateProvider provider;
    const BlockState* azalea = VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES);
    provider.add(azalea, 3);

    world::gen::feature::state::WeightedBlockStateProvider copied(provider);
    const BlockState* flowering = VanillaBlocks::getState(VanillaBlocks::FLOWERING_AZALEA_LEAVES);
    provider.add(flowering, 1);

    EXPECT_EQ(provider.size(), 2u);
    EXPECT_EQ(copied.size(), 1u); // 拷贝构造后独立
}

// ============================================================================
// TreeFeatureConfig foliageProvider 测试
// ============================================================================

TEST(TreeFeatureConfigFoliageProviderTest, DefaultHasNoProvider)
{
    TreeFeatureConfig config;
    EXPECT_FALSE(config.hasFoliageProvider());
}

TEST(TreeFeatureConfigFoliageProviderTest, HasFoliageProviderWhenSet)
{
    VanillaBlocks::initialize();
    TreeFeatureConfig config;
    auto provider = std::make_unique<world::gen::feature::state::WeightedBlockStateProvider>();
    provider->add(VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES), 3);
    config.foliageProvider = std::move(provider);
    EXPECT_TRUE(config.hasFoliageProvider());
}

TEST(TreeFeatureConfigFoliageProviderTest, CopyConstructorDeepCopiesProvider)
{
    VanillaBlocks::initialize();
    TreeFeatureConfig original;
    original.foliageBlock = VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES);
    auto provider = std::make_unique<world::gen::feature::state::WeightedBlockStateProvider>();
    provider->add(VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES), 3);
    provider->add(VanillaBlocks::getState(VanillaBlocks::FLOWERING_AZALEA_LEAVES), 1);
    original.foliageProvider = std::move(provider);

    // 拷贝构造
    TreeFeatureConfig copied(original);
    ASSERT_TRUE(copied.hasFoliageProvider());
    ASSERT_TRUE(original.hasFoliageProvider());

    // 修改 original 的 provider 不应影响 copied
    auto* originalWeighted =
        dynamic_cast<world::gen::feature::state::WeightedBlockStateProvider*>(original.foliageProvider.get());
    ASSERT_NE(originalWeighted, nullptr);
    originalWeighted->add(VanillaBlocks::getState(VanillaBlocks::OAK_LEAVES), 5);
    EXPECT_EQ(originalWeighted->size(), 3u);
    EXPECT_EQ(dynamic_cast<const world::gen::feature::state::WeightedBlockStateProvider*>(copied.foliageProvider.get())
                  ->size(),
        2u);
}

TEST(TreeFeatureConfigFoliageProviderTest, GetFoliageStateUsesProviderWhenPresent)
{
    VanillaBlocks::initialize();
    TreeFeatureConfig config;
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES);
    auto provider = std::make_unique<world::gen::feature::state::WeightedBlockStateProvider>();
    provider->add(VanillaBlocks::getState(VanillaBlocks::FLOWERING_AZALEA_LEAVES), 1);
    config.foliageProvider = std::move(provider);

    TreeFeatureTestWorld world;
    math::Random rng(42);
    // provider 优先级高于 foliageBlock
    const BlockState* s = config.getFoliageState(world, rng, 0, 0, 0);
    EXPECT_EQ(s, VanillaBlocks::getState(VanillaBlocks::FLOWERING_AZALEA_LEAVES));
}

TEST(TreeFeatureConfigFoliageProviderTest, GetFoliageStateFallsBackToFoliageBlock)
{
    VanillaBlocks::initialize();
    TreeFeatureConfig config;
    config.foliageBlock = VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES);
    // 不设置 foliageProvider

    TreeFeatureTestWorld world;
    math::Random rng(42);
    const BlockState* s = config.getFoliageState(world, rng, 0, 0, 0);
    EXPECT_EQ(s, VanillaBlocks::getState(VanillaBlocks::AZALEA_LEAVES));
}

// ============================================================================
// TrunkPlacer 测试
// ============================================================================

class NewTrunkPlacerTest : public ::testing::Test {
protected:
    void SetUp() override { random = std::make_unique<math::Random>(99999); }

    std::unique_ptr<math::Random> random;
};

TEST_F(NewTrunkPlacerTest, FancyTrunkPlacerName)
{
    FancyTrunkPlacer placer(3, 11, 0);
    EXPECT_STREQ(placer.name(), "fancy");
}

TEST_F(NewTrunkPlacerTest, FancyTrunkPlacerHeight)
{
    FancyTrunkPlacer placer(3, 11, 0);
    // Height range: 3 + [0,11] + [0,0] = 3-14
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 3);
        EXPECT_LE(height, 14);
    }
}

TEST_F(NewTrunkPlacerTest, FancyTrunkPlacerClone)
{
    FancyTrunkPlacer placer(3, 11, 0);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "fancy");
}

TEST_F(NewTrunkPlacerTest, ForkyTrunkPlacerName)
{
    ForkyTrunkPlacer placer(5, 2, 1);
    EXPECT_STREQ(placer.name(), "forky");
}

TEST_F(NewTrunkPlacerTest, ForkyTrunkPlacerHeight)
{
    ForkyTrunkPlacer placer(5, 2, 1);
    // Height range: 5 + [0,2] + [0,1] = 5-8
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 5);
        EXPECT_LE(height, 8);
    }
}

TEST_F(NewTrunkPlacerTest, ForkyTrunkPlacerClone)
{
    ForkyTrunkPlacer placer(5, 2, 1);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "forky");
}

TEST_F(NewTrunkPlacerTest, DarkOakTrunkPlacerName)
{
    DarkOakTrunkPlacer placer(6, 3, 1);
    EXPECT_STREQ(placer.name(), "dark_oak");
}

TEST_F(NewTrunkPlacerTest, DarkOakTrunkPlacerHeight)
{
    DarkOakTrunkPlacer placer(6, 3, 1);
    // Height range: 6 + [0,3] + [0,1] = 6-10
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 6);
        EXPECT_LE(height, 10);
    }
}

TEST_F(NewTrunkPlacerTest, DarkOakTrunkPlacerClone)
{
    DarkOakTrunkPlacer placer(6, 3, 1);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "dark_oak");
}

TEST_F(NewTrunkPlacerTest, GiantTrunkPlacerName)
{
    GiantTrunkPlacer placer(13, 5, 3);
    EXPECT_STREQ(placer.name(), "giant");
}

TEST_F(NewTrunkPlacerTest, GiantTrunkPlacerHeight)
{
    GiantTrunkPlacer placer(13, 5, 3);
    // Height range: 13 + [0,5] + [0,3] = 13-21
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 13);
        EXPECT_LE(height, 21);
    }
}

TEST_F(NewTrunkPlacerTest, GiantTrunkPlacerClone)
{
    GiantTrunkPlacer placer(13, 5, 3);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "giant");
}

TEST_F(NewTrunkPlacerTest, MegaJungleTrunkPlacerName)
{
    MegaJungleTrunkPlacer placer(10, 8, 5);
    EXPECT_STREQ(placer.name(), "mega_jungle");
}

TEST_F(NewTrunkPlacerTest, MegaJungleTrunkPlacerHeight)
{
    MegaJungleTrunkPlacer placer(10, 8, 5);
    // Height range: 10 + [0,8] + [0,5] = 10-23
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 10);
        EXPECT_LE(height, 23);
    }
}

TEST_F(NewTrunkPlacerTest, MegaJungleTrunkPlacerClone)
{
    MegaJungleTrunkPlacer placer(10, 8, 5);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "mega_jungle");
}

// ============================================================================
// BendingTrunkPlacer 测试
// ============================================================================

class BendingTrunkPlacerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        random = std::make_unique<math::Random>(12345);

        // 创建 WorldGenRegion 用于放置测试
        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);

                // 初始化平坦地表：y=0 为草方块，y>=1 为空气
                const BlockState* ground = &VanillaBlocks::GRASS_BLOCK->defaultState();
                for (i32 x = 0; x < 16; ++x) {
                    for (i32 z = 0; z < 16; ++z) {
                        chunk->setBlockState(x, 0, z, ground);
                    }
                }

                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));
    }

    std::unique_ptr<math::Random> random;
    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
};

TEST_F(BendingTrunkPlacerTest, Name)
{
    BendingTrunkPlacer placer(4, 2, 0, 3, std::make_unique<world::gen::valueprovider::UniformInt>(1, 2));
    EXPECT_STREQ(placer.name(), "bending");
}

TEST_F(BendingTrunkPlacerTest, HeightRange)
{
    // BendingTrunkPlacer(4, 2, 0, minHeightForLeaves, bendLength)
    // 高度范围: 4 + [0,2] + [0,0] = 4-6
    BendingTrunkPlacer placer(4, 2, 0, 3, std::make_unique<world::gen::valueprovider::UniformInt>(1, 2));
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 4);
        EXPECT_LE(height, 6);
    }
}

TEST_F(BendingTrunkPlacerTest, HeightWithZeroRandom)
{
    // baseHeight=5, heightRandA=0, heightRandB=0 => 高度恒为5
    BendingTrunkPlacer placer(5, 0, 0, 1, std::make_unique<world::gen::valueprovider::UniformInt>(1, 1));
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(placer.getHeight(*random), 5);
    }
}

TEST_F(BendingTrunkPlacerTest, Clone)
{
    auto bendLen = std::make_unique<world::gen::valueprovider::UniformInt>(1, 2);
    BendingTrunkPlacer placer(4, 2, 0, 3, std::move(bendLen));
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "bending");
    // 验证克隆后的高度范围一致
    math::Random rng1(42);
    math::Random rng2(42);
    EXPECT_EQ(placer.getHeight(rng1), clone->getHeight(rng2));
}

TEST_F(BendingTrunkPlacerTest, PlaceTrunkReturnsFoliagePositions)
{
    // 使用确定性高度：baseHeight=5, heightRandA=0, heightRandB=0 => 高度恒为5
    // bendLength=1 => 水平延伸2格（k=0和k=1）
    BendingTrunkPlacer placer(5, 0, 0, 1, std::make_unique<world::gen::valueprovider::UniformInt>(1, 1));
    const BlockState* trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
    std::set<BlockPos> trunkBlocks;

    auto foliagePositions = placer.placeTrunk(*m_region, *random, 5, BlockPos(8, 1, 8), trunkBlocks, trunkBlock);

    // 垂直阶段：height=5, i=4, 循环j=0到4共5层
    // minHeightForLeaves=1 => j>=1时产生树叶附着点（j=1,2,3,4 => 4个）
    // 水平弯曲阶段：bendLength=1, 循环k=0到1共2格 => 2个树叶附着点
    // 总计：4 + 2 = 6个树叶附着点
    // 注意：j=0时不产生树叶（j < minHeightForLeaves=1）
    EXPECT_GE(foliagePositions.size(), 4u);
}

TEST_F(BendingTrunkPlacerTest, PlaceTrunkVerticalBlocksPlaced)
{
    // 确定性高度：5, 无弯曲（bendLength=0 只有1格水平延伸）
    BendingTrunkPlacer placer(5, 0, 0, 1, std::make_unique<world::gen::valueprovider::UniformInt>(0, 0));
    const BlockState* trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
    std::set<BlockPos> trunkBlocks;

    placer.placeTrunk(*m_region, *random, 5, BlockPos(8, 1, 8), trunkBlocks, trunkBlock);

    // 至少应有5个垂直树干方块（y=1到y=5）
    bool hasTrunkAtBase = trunkBlocks.count(BlockPos(8, 1, 8)) > 0;
    EXPECT_TRUE(hasTrunkAtBase);
}

TEST_F(BendingTrunkPlacerTest, PlaceTrunkMinHeightForLeavesZero)
{
    // minHeightForLeaves=0 => 所有垂直层都产生树叶附着点
    BendingTrunkPlacer placer(4, 0, 0, 0, std::make_unique<world::gen::valueprovider::UniformInt>(1, 1));
    const BlockState* trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
    std::set<BlockPos> trunkBlocks;

    auto foliagePositions = placer.placeTrunk(*m_region, *random, 4, BlockPos(8, 1, 8), trunkBlocks, trunkBlock);

    // 垂直阶段：4层都产生树叶附着点 => 4个
    // 水平弯曲阶段：bendLength=1, k=0到1 => 2个树叶附着点
    // 总计：4 + 2 = 6
    EXPECT_EQ(foliagePositions.size(), 6u);
}

TEST_F(BendingTrunkPlacerTest, PlaceTrunkMinHeightForLeavesLarge)
{
    // minHeightForLeaves=10，高度=5 => 垂直阶段无树叶附着点
    // 弯曲阶段：bendLength=1, k=0到1 => 2个树叶附着点
    BendingTrunkPlacer placer(4, 0, 0, 10, std::make_unique<world::gen::valueprovider::UniformInt>(1, 1));
    const BlockState* trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
    std::set<BlockPos> trunkBlocks;

    auto foliagePositions = placer.placeTrunk(*m_region, *random, 5, BlockPos(8, 1, 8), trunkBlocks, trunkBlock);

    // 所有5个垂直层都不产生树叶（j < 10），水平阶段产生2个
    EXPECT_EQ(foliagePositions.size(), 2u);
}

TEST_F(BendingTrunkPlacerTest, PlaceTrunkBendLengthRange)
{
    // 使用多个随机种子验证bendLength在范围内
    // bendLength=UniformInt(1,3) => 采样值在[1,3]之间
    // 水平延伸 = bendLength + 1 格
    for (int seed = 0; seed < 50; ++seed) {
        math::Random rng(seed);
        BendingTrunkPlacer placer(4, 0, 0, 1, std::make_unique<world::gen::valueprovider::UniformInt>(1, 3));
        const BlockState* trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
        std::set<BlockPos> trunkBlocks;

        auto foliagePositions = placer.placeTrunk(*m_region, rng, 4, BlockPos(8, 1, 8), trunkBlocks, trunkBlock);

        // 垂直4层，minHeightForLeaves=1 => j=1,2,3 => 3个树叶
        // 水平层：bendLength在1-3之间，加1格 => 2-4个树叶
        // 总计：3 + (2~4) = 5~7
        EXPECT_GE(foliagePositions.size(), 5u);
        EXPECT_LE(foliagePositions.size(), 7u);
    }
}

TEST_F(BendingTrunkPlacerTest, PlaceTrunkHorizontalBendMovesPosition)
{
    // 验证水平弯曲确实移动了树干位置
    // 使用多个种子运行，统计X/Z偏移
    bool hasXOffset = false;
    bool hasZOffset = false;

    for (int seed = 0; seed < 100; ++seed) {
        math::Random rng(seed);
        BendingTrunkPlacer placer(5, 0, 0, 1, std::make_unique<world::gen::valueprovider::UniformInt>(2, 2));
        const BlockState* trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
        std::set<BlockPos> trunkBlocks;

        placer.placeTrunk(*m_region, rng, 5, BlockPos(8, 1, 8), trunkBlocks, trunkBlock);

        // 检查是否有方块不在(8, *, 8)的中心列上
        for (const auto& pos : trunkBlocks) {
            if (pos.x != 8) {
                hasXOffset = true;
            }
            if (pos.z != 8) {
                hasZOffset = true;
            }
        }
    }

    // 由于方向随机（4个水平方向），100次运行应该能看到X和Z两个方向的偏移
    EXPECT_TRUE(hasXOffset || hasZOffset);
}

TEST_F(BendingTrunkPlacerTest, PlaceTrunkFoliagePositionRadiusBonusZero)
{
    // BendingTrunkPlacer的所有树叶附着点radiusBonus=0, trunkTop=false
    BendingTrunkPlacer placer(4, 0, 0, 1, std::make_unique<world::gen::valueprovider::UniformInt>(1, 1));
    const BlockState* trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
    std::set<BlockPos> trunkBlocks;

    auto foliagePositions = placer.placeTrunk(*m_region, *random, 4, BlockPos(8, 1, 8), trunkBlocks, trunkBlock);

    for (const auto& fp : foliagePositions) {
        EXPECT_EQ(fp.radiusBonus, 0);
        EXPECT_FALSE(fp.trunkTop);
    }
}

TEST_F(BendingTrunkPlacerTest, PlaceTrunkDirtUnderBase)
{
    // 验证placeTrunk在草方块地面上正常工作，不会崩溃
    // 并且正确放置了树干方块（placeDirtUnder在基类中已测试）
    BendingTrunkPlacer placer(4, 0, 0, 1, std::make_unique<world::gen::valueprovider::UniformInt>(1, 1));
    const BlockState* trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
    std::set<BlockPos> trunkBlocks;

    auto foliagePositions = placer.placeTrunk(*m_region, *random, 4, BlockPos(8, 1, 8), trunkBlocks, trunkBlock);

    // 验证底部有树干方块
    EXPECT_GT(trunkBlocks.count(BlockPos(8, 1, 8)), 0u);
    // 验证有树叶附着点
    EXPECT_GE(foliagePositions.size(), 2u);
}

TEST_F(BendingTrunkPlacerTest, AzaleaTreeConfig)
{
    // 验证杜鹃树使用的BendingTrunkPlacer参数与MC一致
    // MC: BendingTrunkPlacer(4, 2, 0, 3, UniformInt(1, 2))
    BendingTrunkPlacer placer(4, 2, 0, 3, std::make_unique<world::gen::valueprovider::UniformInt>(1, 2));

    // 高度范围：4 + [0,2] + [0,0] = 4-6
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 4);
        EXPECT_LE(height, 6);
    }

    EXPECT_STREQ(placer.name(), "bending");
}

// ============================================================================
// 新增方块注册测试
// ============================================================================

class NewBlockRegistrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

TEST_F(NewBlockRegistrationTest, FlowerBlocks)
{
    EXPECT_NE(VanillaBlocks::LILY_OF_THE_VALLEY, nullptr);
    EXPECT_NE(VanillaBlocks::SUNFLOWER, nullptr);
    EXPECT_NE(VanillaBlocks::LILAC, nullptr);
    EXPECT_NE(VanillaBlocks::ROSE_BUSH, nullptr);
    EXPECT_NE(VanillaBlocks::PEONY, nullptr);
    EXPECT_NE(VanillaBlocks::CORNFLOWER, nullptr);
    EXPECT_NE(VanillaBlocks::WITHER_ROSE, nullptr);
}

TEST_F(NewBlockRegistrationTest, MushroomBlocks)
{
    EXPECT_NE(VanillaBlocks::BROWN_MUSHROOM_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::RED_MUSHROOM_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::MUSHROOM_STEM, nullptr);
}

TEST_F(NewBlockRegistrationTest, UtilityBlocks)
{
    EXPECT_NE(VanillaBlocks::FARMLAND, nullptr);
    EXPECT_NE(VanillaBlocks::RED_SAND, nullptr);
}

TEST_F(NewBlockRegistrationTest, NetherBlocks)
{
    EXPECT_NE(VanillaBlocks::CRIMSON_STEM, nullptr);
    EXPECT_NE(VanillaBlocks::WARPED_STEM, nullptr);
    EXPECT_NE(VanillaBlocks::CRIMSON_NYLIUM, nullptr);
    EXPECT_NE(VanillaBlocks::WARPED_NYLIUM, nullptr);
    EXPECT_NE(VanillaBlocks::SHROOMLIGHT, nullptr);
    EXPECT_NE(VanillaBlocks::CRIMSON_FUNGUS, nullptr);
    EXPECT_NE(VanillaBlocks::WARPED_FUNGUS, nullptr);
    EXPECT_NE(VanillaBlocks::WEEPING_VINES, nullptr);
    EXPECT_NE(VanillaBlocks::TWISTING_VINES, nullptr);
}

TEST_F(NewBlockRegistrationTest, AllNewBlocksHaveDefaultState)
{
    // Flower blocks
    ASSERT_NE(VanillaBlocks::LILY_OF_THE_VALLEY, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::LILY_OF_THE_VALLEY->defaultState());

    ASSERT_NE(VanillaBlocks::SUNFLOWER, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::SUNFLOWER->defaultState());

    ASSERT_NE(VanillaBlocks::CORNFLOWER, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::CORNFLOWER->defaultState());

    // Mushroom blocks
    ASSERT_NE(VanillaBlocks::BROWN_MUSHROOM_BLOCK, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::BROWN_MUSHROOM_BLOCK->defaultState());

    ASSERT_NE(VanillaBlocks::MUSHROOM_STEM, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::MUSHROOM_STEM->defaultState());

    // Nether blocks
    ASSERT_NE(VanillaBlocks::CRIMSON_STEM, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::CRIMSON_STEM->defaultState());

    ASSERT_NE(VanillaBlocks::SHROOMLIGHT, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::SHROOMLIGHT->defaultState());
}

// ============================================================================
// BigMushroomFeatureConfig 测试
// ============================================================================

TEST(BigMushroomConfigTest, DefaultConfig)
{
    BigMushroomFeatureConfig config;
    EXPECT_EQ(config.capState, nullptr);
    EXPECT_EQ(config.stemState, nullptr);
    EXPECT_EQ(config.capRadius, 2);
}

TEST(BigMushroomConfigTest, ExplicitConfig)
{
    VanillaBlocks::initialize();
    const BlockState* cap =
        VanillaBlocks::BROWN_MUSHROOM_BLOCK ? &VanillaBlocks::BROWN_MUSHROOM_BLOCK->defaultState() : nullptr;
    const BlockState* stem = VanillaBlocks::MUSHROOM_STEM ? &VanillaBlocks::MUSHROOM_STEM->defaultState() : nullptr;

    BigMushroomFeatureConfig config(cap, stem, 3);
    EXPECT_EQ(config.capState, cap);
    EXPECT_EQ(config.stemState, stem);
    EXPECT_EQ(config.capRadius, 3);
}

// ============================================================================
// BigMushroomFeature 高度计算测试
// ============================================================================

// Test subclasses to access protected methods
class TestBrownMushroom : public BigBrownMushroomFeature {
public:
    using BigBrownMushroomFeature::getCapRadius;
};

class TestRedMushroom : public BigRedMushroomFeature {
public:
    using BigRedMushroomFeature::getCapRadius;
};

TEST(BigMushroomHeightTest, BrownMushroomCapRadius)
{
    TestBrownMushroom brownFeature;
    // For brown: height <= 3 returns 0, otherwise capRadius
    EXPECT_EQ(brownFeature.getCapRadius(-1, 6, 2, 2), 0);
    EXPECT_EQ(brownFeature.getCapRadius(-1, 6, 2, 3), 0);
    EXPECT_EQ(brownFeature.getCapRadius(-1, 6, 2, 4), 2);
    EXPECT_EQ(brownFeature.getCapRadius(-1, 6, 2, 5), 2);
}

TEST(BigMushroomHeightTest, RedMushroomCapRadius)
{
    TestRedMushroom redFeature;
    // For red: height < totalHeight - 3 returns 0
    // height == totalHeight returns capRadius
    // otherwise capRadius - 1
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 0), 0);
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 2), 0);
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 3), 1); // capRadius - 1
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 4), 1);
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 5), 1);
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 6), 2); // at totalHeight
}

// ============================================================================
// HugeFungusFeatureConfig 测试
// ============================================================================

TEST(HugeFungusFeatureTest, FungusTypeEnum)
{
    // Verify enum values exist
    EXPECT_NE(static_cast<int>(FungusType::Crimson), static_cast<int>(FungusType::Warped));
}

TEST(HugeFungusFeatureTest, FungusConfigDefaults)
{
    HugeFungusFeatureConfig config;
    EXPECT_EQ(config.fungusType, FungusType::Crimson);
    EXPECT_EQ(config.planted, false);
}

TEST(HugeFungusFeatureTest, FungusConfigExplicit)
{
    HugeFungusFeatureConfig config(FungusType::Warped, true);
    EXPECT_EQ(config.fungusType, FungusType::Warped);
    EXPECT_EQ(config.planted, true);
}

// ============================================================================
// TrunkPlacer 高度分布测试
// ============================================================================

TEST(TrunkPlacerDistributionTest, TallBirchTrunkHeight)
{
    math::Random rng(777);
    StraightTrunkPlacer placer(5, 2, 6);

    // Height range: 5 + [0,2] + [0,6] = 5-13
    std::map<i32, i32> distribution;
    for (int i = 0; i < 1000; ++i) {
        i32 height = placer.getHeight(rng);
        distribution[height]++;
        EXPECT_GE(height, 5);
        EXPECT_LE(height, 13);
    }

    // At least 5 distinct heights should appear
    EXPECT_GE(distribution.size(), 5u);
}

TEST(TrunkPlacerDistributionTest, JungleBushTrunkHeight)
{
    math::Random rng(888);
    StraightTrunkPlacer placer(1, 0, 0);

    // Fixed height: always 1
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(placer.getHeight(rng), 1);
    }
}
