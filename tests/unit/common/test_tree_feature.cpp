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

#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/block/blocks/RotatedPillarBlock.hpp"
#include "common/world/block/registry/GardenBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/FeatureSpread.hpp"
#include "server/world/gen/feature/state/BlockStateProvider.hpp"
#include "server/world/gen/feature/tree/TreeFeature.hpp"
#include "server/world/gen/feature/tree/decorator/PlaceOnGroundDecorator.hpp"
#include "server/world/gen/feature/tree/decorator/TreeDecorator.hpp"
#include "server/world/gen/feature/tree/foliage/BlobFoliagePlacer.hpp"
#include "server/world/gen/feature/tree/foliage/FancyFoliagePlacer.hpp"
#include "server/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "server/world/gen/feature/tree/trunk/FancyTrunkPlacer.hpp"
#include "server/world/gen/feature/tree/trunk/StraightTrunkPlacer.hpp"
#include "server/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include "server/world/gen/valueprovider/IntProvider.hpp"
#include "util/math/MathUtils.hpp"
#include "world/block/BlockRegistry.hpp"
#include <array>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// FeatureSpread 测试
// ============================================================================

class FeatureSpreadTest : public ::testing::Test {
protected:
    void SetUp() override { random = std::make_unique<math::Random>(12345); }

    std::unique_ptr<math::Random> random;
};

TEST_F(FeatureSpreadTest, FixedValue)
{
    FeatureSpread spread = FeatureSpread::fixed(5);

    EXPECT_EQ(spread.base(), 5);
    EXPECT_EQ(spread.spread(), 0);

    // 固定值应该总是返回相同的结果
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(spread.get(*random), 5);
    }
}

TEST_F(FeatureSpreadTest, SpreadValue)
{
    FeatureSpread spread = FeatureSpread::spread(10, 5);

    EXPECT_EQ(spread.base(), 10);
    EXPECT_EQ(spread.spread(), 5);

    // 检查值在范围内
    for (int i = 0; i < 100; ++i) {
        i32 value = spread.get(*random);
        EXPECT_GE(value, 10);
        EXPECT_LE(value, 15);
    }
}

TEST_F(FeatureSpreadTest, ZeroSpread)
{
    FeatureSpread spread(5, 0);

    EXPECT_EQ(spread.get(*random), 5);
}

// ============================================================================
// TrunkPlacer 测试
// ============================================================================

class TrunkPlacerTest : public ::testing::Test {
protected:
    void SetUp() override { random = std::make_unique<math::Random>(12345); }

    std::unique_ptr<math::Random> random;
};

TEST_F(TrunkPlacerTest, StraightTrunkHeight)
{
    StraightTrunkPlacer placer(4, 2, 1);

    // 高度范围: 4 + [0,2] + [0,1] = 4-7
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 4);
        EXPECT_LE(height, 7);
    }
}

TEST_F(TrunkPlacerTest, StraightTrunkZeroRandom)
{
    StraightTrunkPlacer placer(5, 0, 0);

    // 没有随机性的高度
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(placer.getHeight(*random), 5);
    }
}

TEST_F(TrunkPlacerTest, Name)
{
    StraightTrunkPlacer placer(4, 2, 1);
    EXPECT_STREQ(placer.name(), "StraightTrunkPlacer");
}

// ============================================================================
// FoliagePlacer 测试
// ============================================================================

class FoliagePlacerTest : public ::testing::Test {
protected:
    void SetUp() override { random = std::make_unique<math::Random>(12345); }

    std::unique_ptr<math::Random> random;
};

TEST_F(FoliagePlacerTest, BlobFoliageHeight)
{
    BlobFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 3);

    // BlobFoliagePlacer 应该返回固定高度
    EXPECT_EQ(placer.getFoliageHeight(*random, 6), 3);
}

TEST_F(FoliagePlacerTest, BlobFoliageName)
{
    BlobFoliagePlacer placer(FeatureSpread::fixed(2), FeatureSpread::fixed(0), 3);

    EXPECT_STREQ(placer.name(), "BlobFoliagePlacer");
}

// ============================================================================
// TreeFeatureConfig 测试
// ============================================================================

TEST(TreeFeatureConfigTest, OakConfig)
{
    VanillaBlocks::initialize();
    const TreeFeatureConfig config = TreeFeatures::oakConfig();

    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::OAK_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::OAK_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_EQ(config.minHeight, 4);
}

TEST(TreeFeatureConfigTest, BirchConfig)
{
    VanillaBlocks::initialize();
    const TreeFeatureConfig config = TreeFeatures::birchConfig();

    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::BIRCH_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::BIRCH_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_GE(config.minHeight, 5);
}

TEST(TreeFeatureConfigTest, SpruceConfig)
{
    VanillaBlocks::initialize();
    const TreeFeatureConfig config = TreeFeatures::spruceConfig();

    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::SPRUCE_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::SPRUCE_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
}

TEST(TreeFeatureConfigTest, JungleConfig)
{
    VanillaBlocks::initialize();
    const TreeFeatureConfig config = TreeFeatures::jungleConfig();

    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::JUNGLE_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::JUNGLE_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
}

// ============================================================================
// TreeFeature 静态方法测试
// ============================================================================

class TreeFeatureTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// 注意：完整的放置测试需要模拟 WorldGenRegion，这里只测试静态方法

TEST_F(TreeFeatureTest, FeatureSpreadIntegration)
{
    // 测试 FeatureSpread 与 Random 的集成
    math::Random rng(42);

    FeatureSpread spread = FeatureSpread::spread(3, 4);

    // 统计分布
    std::map<i32, i32> distribution;
    for (int i = 0; i < 1000; ++i) {
        i32 value = spread.get(rng);
        distribution[value]++;
    }

    // 检查值都在范围内
    for (const auto& [value, count] : distribution) {
        EXPECT_GE(value, 3);
        EXPECT_LE(value, 7);
    }

    // 检查分布合理（每个值至少有一些出现）
    EXPECT_GE(distribution.size(), 4);
}

TEST_F(TreeFeatureTest, TrunkPlacerHeightDistribution)
{
    // 测试树干高度分布
    math::Random rng(123);

    StraightTrunkPlacer placer(5, 3, 2);

    // 高度范围: 5 + [0,3] + [0,2] = 5-10
    std::map<i32, i32> distribution;
    for (int i = 0; i < 1000; ++i) {
        i32 height = placer.getHeight(rng);
        distribution[height]++;
    }

    // 检查所有高度都有可能
    for (i32 h = 5; h <= 10; ++h) {
        EXPECT_GT(distribution[h], 0) << "Height " << h << " never appeared";
    }
}

TEST_F(TreeFeatureTest, ConfigCreation)
{
    // 测试手动创建配置
    auto trunkPlacer = std::make_unique<StraightTrunkPlacer>(4, 2, 0);
    auto foliagePlacer = std::make_unique<BlobFoliagePlacer>(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 3);

    TreeFeatureConfig config(&VanillaBlocks::OAK_LOG->defaultState(),
        &VanillaBlocks::OAK_LEAVES->defaultState(),
        std::move(trunkPlacer),
        std::move(foliagePlacer));

    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::OAK_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::OAK_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
}

// ============================================================================
// TreeFeature 放置行为测试（含 WorldGenRegion）
// ============================================================================

class TreeFeaturePlacementWorldTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);

                // 初始化平坦地表：y=0 为草方块，y>=1 为空气。
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

    [[nodiscard]] TreeFeatureConfig makeFixedOakConfig() const
    {
        TreeFeatureConfig config;
        config.trunkBlock = &VanillaBlocks::OAK_LOG->defaultState();
        config.foliageBlock = &VanillaBlocks::OAK_LEAVES->defaultState();
        config.trunkPlacer = std::make_unique<StraightTrunkPlacer>(5, 0, 0);
        config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(FeatureSpread::fixed(2), FeatureSpread::fixed(0), 3);
        config.minHeight = 5;
        return config;
    }

    void setWorldBlock(i32 x, i32 y, i32 z, const BlockState* state)
    {
        ASSERT_TRUE(m_region->setBlockState(x, y, z, state));
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
};

TEST_F(TreeFeaturePlacementWorldTest, PlaceTreeSucceedsWhenVolumeClear)
{
    TreeFeature feature;
    TreeFeatureConfig config = makeFixedOakConfig();
    math::Random rng(12345);

    EXPECT_TRUE(feature.place(*m_region, rng, BlockPos(8, 1, 8), config));
    EXPECT_TRUE(m_region->getBlockState(8, 0, 8)->is(VanillaBlocks::DIRT));
}

TEST_F(TreeFeaturePlacementWorldTest, TreeAndTrunkReplaceTaggedGroundDecorations)
{
    TreeFeature feature;
    auto config = makeFixedOakConfig();
    setWorldBlock(8, 1, 8, &block_registry::GardenBlocks::LEAF_LITTER->defaultState());
    setWorldBlock(8, 2, 8, &block_registry::GardenBlocks::BUSH->defaultState());
    math::JavaLegacyRandom rng(12345);

    ASSERT_TRUE(feature.place(*m_region, rng, BlockPos(8, 1, 8), config));
    for (i32 y = 1; y <= 5; ++y) {
        ASSERT_NE(m_region->getBlockState(8, y, 8), nullptr);
        EXPECT_TRUE(m_region->getBlockState(8, y, 8)->is(VanillaBlocks::OAK_LOG));
    }
}

TEST_F(TreeFeaturePlacementWorldTest, ExistingLogsAllowSpaceProbeButRemainUnchanged)
{
    TreeFeature feature;
    auto config = makeFixedOakConfig();
    setWorldBlock(8, 3, 8, &VanillaBlocks::SPRUCE_LOG->defaultState());
    math::JavaLegacyRandom rng(12345);

    ASSERT_TRUE(feature.place(*m_region, rng, BlockPos(8, 1, 8), config));
    EXPECT_TRUE(m_region->getBlockState(8, 3, 8)->is(VanillaBlocks::SPRUCE_LOG));
    EXPECT_TRUE(m_region->getBlockState(8, 4, 8)->is(VanillaBlocks::OAK_LOG));
}

TEST_F(TreeFeaturePlacementWorldTest, IgnoreVinesControlsSpaceProbe)
{
    TreeFeature feature;
    auto config = makeFixedOakConfig();
    setWorldBlock(8, 2, 8, &VanillaBlocks::VINE->defaultState());
    math::JavaLegacyRandom blockedRng(12345);
    EXPECT_FALSE(feature.place(*m_region, blockedRng, BlockPos(8, 1, 8), config));
    EXPECT_TRUE(m_region->getBlockState(8, 2, 8)->is(VanillaBlocks::VINE));

    config.ignoreVines = true;
    math::JavaLegacyRandom allowedRng(12345);
    ASSERT_TRUE(feature.place(*m_region, allowedRng, BlockPos(8, 1, 8), config));
    EXPECT_TRUE(m_region->getBlockState(8, 2, 8)->is(VanillaBlocks::OAK_LOG));
}

TEST_F(TreeFeaturePlacementWorldTest, GroundDecorationsCannotStackOnPreviouslyPlacedPlants)
{
    using namespace world::gen::feature::tree::decorator;
    const BlockPos plantPos(8, 1, 8);
    math::JavaLegacyRandom random(42);
    TreeDecoratorContext context(*m_region,
        nullptr,
        nullptr,
        [this](const BlockPos& pos, const BlockState* state) { m_region->setBlockState(pos, state); },
        random,
        {plantPos},
        {},
        {});
    context.setBlock(plantPos, &VanillaBlocks::SHORT_GRASS->defaultState());
    ASSERT_FALSE(m_region->getBlockState(plantPos)->isSolid());
    PlaceOnGroundDecorator decorator(128,
        0,
        1,
        std::make_unique<world::gen::feature::state::SimpleBlockStateProvider>(
            &VanillaBlocks::SHORT_GRASS->defaultState()));
    decorator.place(context);
    EXPECT_TRUE(m_region->getBlockState(plantPos.up())->isAir());
    EXPECT_TRUE(m_region->getBlockState(plantPos.up(2))->isAir());
    // 写入后再被替换为空气或石头时，条件同样须反映实际状态。
    context.setBlock(plantPos, &VanillaBlocks::AIR->defaultState());
    EXPECT_FALSE(context.checkBlock(plantPos, [](const BlockState& state) { return state.isSolid(); }));
    context.setBlock(plantPos, &VanillaBlocks::STONE->defaultState());
    EXPECT_TRUE(context.checkBlock(plantPos, [](const BlockState& state) { return state.isSolid(); }));
}

TEST_F(TreeFeaturePlacementWorldTest, PlaceTreeSucceedsOnFarmland)
{
    TreeFeature feature;
    TreeFeatureConfig config = makeFixedOakConfig();
    math::Random rng(12345);

    setWorldBlock(8, 0, 8, &VanillaBlocks::FARMLAND->defaultState());

    EXPECT_TRUE(feature.place(*m_region, rng, BlockPos(8, 1, 8), config));
}

TEST_F(TreeFeaturePlacementWorldTest, PlaceTreeFailsWhenCanopySideBlocked)
{
    TreeFeature feature;
    TreeFeatureConfig config = makeFixedOakConfig();
    math::Random rng(12345);

    // 在树干侧面放置不可替换方块，覆盖中层空间检查半径。
    setWorldBlock(9, 4, 8, &VanillaBlocks::STONE->defaultState());

    EXPECT_FALSE(feature.place(*m_region, rng, BlockPos(8, 1, 8), config));
}

TEST_F(TreeFeaturePlacementWorldTest, ForcePlacementIgnoresVolumeCheck)
{
    TreeFeature feature;
    TreeFeatureConfig config = makeFixedOakConfig();
    config.forcePlacement = true;
    math::Random rng(12345);

    // 与上一用例相同障碍物，forcePlacement 时应跳过空间检查。
    setWorldBlock(9, 4, 8, &VanillaBlocks::STONE->defaultState());

    EXPECT_TRUE(feature.place(*m_region, rng, BlockPos(8, 1, 8), config));
}

TEST_F(TreeFeaturePlacementWorldTest, BlobRowsMatchJavaCornersAndRandomConsumption)
{
    using world::gen::valueprovider::ConstantInt;
    BlobFoliagePlacer placer(FeatureSpread::of(ConstantInt::create(2)), FeatureSpread::of(ConstantInt::create(0)), 3);
    math::JavaLegacyRandom rng(42);
    const auto shape = placer.sampleShape(rng, 5);
    std::set<BlockPos> leaves;
    placer.placeFoliage(*m_region,
        rng,
        5,
        {FoliagePosition(BlockPos(8, 5, 8), 0, false)},
        shape,
        &VanillaBlocks::OAK_LEAVES->defaultState(),
        nullptr,
        leaves);
    // 独立 Java Random(42) 按从上至下、X/Z 升序生成得到的角落与总量。
    EXPECT_EQ(leaves.size(), 57u);
    EXPECT_FALSE(leaves.contains(BlockPos(7, 5, 7)));
    EXPECT_TRUE(leaves.contains(BlockPos(7, 4, 9)));
    EXPECT_TRUE(leaves.contains(BlockPos(10, 3, 6)));
    EXPECT_EQ(rng.nextInt(), 1991376913);
}

TEST_F(TreeFeaturePlacementWorldTest, FancyRowsUseFixedHeightAndSquaredCircularBounds)
{
    using world::gen::valueprovider::ConstantInt;
    FancyFoliagePlacer placer(FeatureSpread::of(ConstantInt::create(2)), FeatureSpread::of(ConstantInt::create(0)), 4);
    math::JavaLegacyRandom rng(42);
    math::JavaLegacyRandom expected(42);
    const auto shape = placer.sampleShape(rng, 8);
    EXPECT_EQ(shape.height, 4);
    std::set<BlockPos> leaves;
    placer.placeFoliage(*m_region,
        rng,
        8,
        {FoliagePosition(BlockPos(8, 6, 8), 0, false)},
        shape,
        &VanillaBlocks::OAK_LEAVES->defaultState(),
        nullptr,
        leaves);
    EXPECT_EQ(leaves.size(), 73u);
    EXPECT_FALSE(leaves.contains(BlockPos(10, 6, 8)));
    EXPECT_TRUE(leaves.contains(BlockPos(10, 5, 9)));
    EXPECT_FALSE(leaves.contains(BlockPos(10, 5, 10)));
    EXPECT_EQ(rng.nextInt(), expected.nextInt());
}

TEST_F(TreeFeaturePlacementWorldTest, FancyCanopySupportsProviderWithoutFallbackBlock)
{
    using world::gen::valueprovider::ConstantInt;
    FancyFoliagePlacer placer(FeatureSpread::of(ConstantInt::create(2)), FeatureSpread::of(ConstantInt::create(0)), 4);
    world::gen::feature::state::SimpleBlockStateProvider provider(&VanillaBlocks::BIRCH_LEAVES->defaultState());
    math::JavaLegacyRandom rng(42);
    const auto shape = placer.sampleShape(rng, 8);
    std::set<BlockPos> leaves;
    placer.placeFoliage(
        *m_region, rng, 8, {FoliagePosition(BlockPos(8, 6, 8), 0, false)}, shape, nullptr, &provider, leaves);
    ASSERT_EQ(leaves.size(), 73u);
    for (const auto& pos : leaves) {
        EXPECT_TRUE(m_region->getBlockState(pos)->is(VanillaBlocks::BIRCH_LEAVES));
    }
}

TEST_F(TreeFeaturePlacementWorldTest, RejectedTreeStillSamplesFoliageRadius)
{
    using world::gen::valueprovider::ConstantInt;
    using world::gen::valueprovider::UniformInt;
    TreeFeature feature;
    auto config = makeFixedOakConfig();
    config.foliagePlacer = std::make_unique<BlobFoliagePlacer>(
        FeatureSpread::of(UniformInt::create(2, 3)), FeatureSpread::of(ConstantInt::create(0)), 3);
    setWorldBlock(9, 4, 8, &VanillaBlocks::STONE->defaultState());
    math::JavaLegacyRandom rng(42);
    math::JavaLegacyRandom expected(42);
    static_cast<void>(expected.nextInt(1));
    static_cast<void>(expected.nextInt(1));
    static_cast<void>(expected.nextInt(2));
    EXPECT_FALSE(feature.place(*m_region, rng, BlockPos(8, 1, 8), config));
    EXPECT_EQ(rng.nextInt(), expected.nextInt());
}

TEST_F(TreeFeaturePlacementWorldTest, FancyTrunkMatchesJavaOnBothSidesOfWorldOrigin)
{
    // 真值由 testdata/worldgen/tree/FancyTrunkOracle.java 独立生成。
    for (const i32 horizontalOrigin : {8, -8}) {
        FancyTrunkPlacer placer(3, 11, 0);
        math::JavaLegacyRandom rng(42);
        std::set<BlockPos> logs;
        const auto attachments = placer.placeTrunk(*m_region,
            rng,
            8,
            BlockPos(horizontalOrigin, 1, horizontalOrigin),
            logs,
            &VanillaBlocks::OAK_LOG->defaultState());
        std::set<BlockPos> expectedLogs;
        for (i32 y = 1; y <= 7; ++y) {
            expectedLogs.emplace(horizontalOrigin, y, horizontalOrigin);
        }
        for (const i32 y : {4, 5}) {
            for (const i32 z : {1, 2}) {
                expectedLogs.emplace(horizontalOrigin + 1, y, horizontalOrigin + z);
            }
        }
        EXPECT_EQ(logs, expectedLogs);
        ASSERT_EQ(attachments.size(), 3u);
        EXPECT_EQ(attachments[0].pos, BlockPos(horizontalOrigin, 6, horizontalOrigin));
        EXPECT_EQ(attachments[1].pos, BlockPos(horizontalOrigin + 1, 5, horizontalOrigin + 2));
        EXPECT_EQ(attachments[2].pos, BlockPos(horizontalOrigin + 1, 4, horizontalOrigin + 2));
        for (const auto& attachment : attachments) {
            EXPECT_FALSE(attachment.trunkTop);
        }
        const auto* branch = m_region->getBlockState(horizontalOrigin + 1, 5, horizontalOrigin + 2);
        ASSERT_NE(branch, nullptr);
        EXPECT_EQ(branch->get(RotatedPillarBlock::AXIS()), Axis::Z);
        EXPECT_EQ(rng.nextInt(), 1190043011);
    }
}
