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

#include <gtest/gtest.h>

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/noise/PerlinSimplexNoise.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

using namespace mc;
namespace vp = mc::world::gen::valueprovider;
using mc::world::chunk::heightmapTypeFromString;

namespace {

/// 最小 IChunkGenerator 存根
class StubGenerator : public IChunkGenerator {
public:
    void generateStructureStarts(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateStructureReferences(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateBiomes(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateNoise(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void buildSurface(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void applyCarvers(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void placeFeatures(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    i32 spawnInitialMobs(
        WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/, std::vector<SpawnedEntityData>& /*outEntities*/) override
    {
        return 0;
    }
    [[nodiscard]] BiomeId getBiome(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override { return 0; }
    [[nodiscard]] BiomeId getNoiseBiome(i32 /*noiseX*/, i32 /*noiseY*/, i32 /*noiseZ*/) const override { return 0; }
    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/, HeightmapType /*type*/) const override { return 64; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] const DimensionSettings& settings() const override { return DimensionSettings::overworld(); }
    [[nodiscard]] i32 seaLevel() const override { return 63; }
};

} // namespace

class CountOnEveryLayerAndNoisePlacementTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        PlacementRegistry::instance().initialize();

        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                chunk->setPersistedStatus(ChunkStatuses::FEATURES);
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }
        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));
    }

    void TearDown() override
    {
        m_region.reset();
        m_ownedChunks.clear();
    }

    ChunkPrimer& centerChunk() { return *m_ownedChunks[4]; }

    void placeBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_region->setBlockState(x, y, z, state); }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    StubGenerator m_stub;
};

// ============================================================================
// fixed_placement
// MC 1.21.11 FixedPlacement: 仅当 basePos 所在区块包含 positions 中某些坐标时，
// 返回那些坐标；否则返回空。
// ============================================================================

TEST_F(CountOnEveryLayerAndNoisePlacementTest, FixedPlacementReturnsPositionsInSameChunk)
{
    // basePos 在区块 (0,0)。positions 含两个坐标：一个在区块(0,0)，一个在区块(1,0)。
    // 应只返回区块(0,0)内的那个。
    std::vector<BlockPos> positions = {BlockPos(5, 70, 6), BlockPos(20, 80, 5)};
    FixedPlacementConfig config(positions);

    FixedPlacement placement;
    math::Random random(1);
    // basePos 用区块原点 (0,0,0)
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(0, 0, 0));

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].x, 5);
    EXPECT_EQ(result[0].y, 70);
    EXPECT_EQ(result[0].z, 6);
}

TEST_F(CountOnEveryLayerAndNoisePlacementTest, FixedPlacementEmptyWhenNoPositionInChunk)
{
    // positions 全在区块(1,0)，basePos 在区块(0,0) → 空。
    std::vector<BlockPos> positions = {BlockPos(20, 80, 5), BlockPos(31, 90, 10)};
    FixedPlacementConfig config(positions);

    FixedPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(0, 0, 0));

    EXPECT_TRUE(result.empty());
}

TEST_F(CountOnEveryLayerAndNoisePlacementTest, FixedPlacementMultiplePositionsInSameChunk)
{
    // 两个坐标都在区块(0,0)，应都返回。
    std::vector<BlockPos> positions = {BlockPos(1, 70, 1), BlockPos(15, 80, 15)};
    FixedPlacementConfig config(positions);

    FixedPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(8, 0, 8));

    EXPECT_EQ(result.size(), 2u);
}

// ============================================================================
// count_on_every_layer
// MC 1.21.11 CountOnEveryLayerPlacement (@Deprecated):
//   i=0; do { flag=false; for j in 0..count.sample(): k=rand(16)+x, l=rand(16)+z,
//     i1=getHeight(MOTION_BLOCKING,k,l), j1=findOnGroundYPosition(k,i1,l,i);
//     if j1!=MAX: emit (k,j1,l); flag=true; i++; } while(flag);
//   findOnGroundYPosition: 从 i1 向下扫到 minY+1，找"下方非空且当前空且下方非基岩"的层，
//     返回第 i 个这样的层的 (下方方块 Y + 1)。找不到返回 MAX。
// isEmpty = air/water/lava。
// ============================================================================

TEST_F(CountOnEveryLayerAndNoisePlacementTest, CountOnEveryLayerFindsGroundSurface)
{
    // 在区块 (0,0) 的每一列都放 stone@70。MOTION_BLOCKING 高度图 → topY=70，getHeight 原=71。
    // findOnGroundYPosition 从 i1=71 向下扫：j=71 时 below=@70=stone(非空) 且 current=@71=air 且非基岩
    // → layer 0 命中，返回 70+1=71。任意 (k,l) 采样都命中 Y=71。
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            placeBlock(x, 70, z, &VanillaBlocks::STONE->defaultState());
        }
    }

    CountOnEveryLayerConfig config(std::make_unique<vp::ConstantInt>(8));

    CountOnEveryLayerPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(0, 0, 0));

    // 第 0 层：8 次采样全部命中 Y=71。后续层因每列只有一个地面、findOnGround 返回 MAX → flag=false 退出。
    ASSERT_EQ(result.size(), 8u);
    for (const auto& pos : result) {
        EXPECT_EQ(pos.y, 71) << "首层地面应在 Y=71（stone@70 上方）";
        EXPECT_GE(pos.x, 0);
        EXPECT_LT(pos.x, 16);
        EXPECT_GE(pos.z, 0);
        EXPECT_LT(pos.z, 16);
    }
}

TEST_F(CountOnEveryLayerAndNoisePlacementTest, CountOnEveryLayerSkipsBedrockAsGround)
{
    // (8,*,8) 列：stone@70, bedrock@60。bedrock 不算地面（MC: !blockstate1.is(BEDROCK)）。
    // 故只有 stone@70 一层地面。
    placeBlock(8, 70, 8, &VanillaBlocks::STONE->defaultState());
    placeBlock(8, 60, 8, &VanillaBlocks::BEDROCK->defaultState());

    CountOnEveryLayerConfig config(std::make_unique<vp::ConstantInt>(4));

    CountOnEveryLayerPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(0, 0, 0));

    // bedrock 层不应出现；stone 层 Y=71 可能出现（取决于随机采样的 k,l 是否落在 (8,8)）。
    for (const auto& pos : result) {
        EXPECT_NE(pos.y, 61) << "基岩上方不应作为地面层";
    }
}

TEST_F(CountOnEveryLayerAndNoisePlacementTest, CountOnEveryLayerEmptyColumnProducesNothing)
{
    // 空列：findOnGroundYPosition 始终返回 MAX → flag 恒 false → 循环不执行 → 空。
    // （do-while 第一轮 count.sample() 次采样全 MAX，flag=false，退出。）
    CountOnEveryLayerConfig config(std::make_unique<vp::ConstantInt>(2));

    CountOnEveryLayerPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(0, 0, 0));

    // 空列不应产生任何位置。
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// noise_threshold_count
// MC 1.21.11 NoiseThresholdCountPlacement (RepeatingPlacement):
//   d0 = BIOME_INFO_NOISE.getValue(x/200.0, z/200.0, false);
//   count = d0 < noiseLevel ? belowNoise : aboveNoise;
//   返回 count 个 basePos。
// ============================================================================

TEST_F(CountOnEveryLayerAndNoisePlacementTest, NoiseThresholdCountReturnsBelowOrAboveByNoise)
{
    // 用同一 (x,z) 采样 biomeInfoNoise，确定噪声值方向后构造两个 config 验证 below/above 分支。
    constexpr i32 px = 100, pz = 100;
    const f64 noise = world::biome::biomeInfoNoise().getValue(px / 200.0, pz / 200.0, false);

    NoiseThresholdCountConfig configLow(noise - 0.01, 3, 7);  // noise < (noise-0.01) 恒假 → aboveNoise=7
    NoiseThresholdCountConfig configHigh(noise + 0.01, 3, 7); // noise < (noise+0.01) 恒真 → belowNoise=3

    NoiseThresholdCountPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> above = placement.getPositions(*m_region, random, configLow, BlockPos(px, 64, pz));
    const std::vector<BlockPos> below = placement.getPositions(*m_region, random, configHigh, BlockPos(px, 64, pz));

    EXPECT_EQ(above.size(), 7u);
    EXPECT_EQ(below.size(), 3u);
    // RepeatingPlacement 返回 count 个 basePos（位置相同）
    for (const auto& pos : below) {
        EXPECT_EQ(pos.x, px);
        EXPECT_EQ(pos.z, pz);
    }
}

// ============================================================================
// noise_based_count
// MC 1.21.11 NoiseBasedCountPlacement (RepeatingPlacement):
//   d0 = BIOME_INFO_NOISE.getValue(x/noiseFactor, z/noiseFactor, false);
//   count = ceil((d0 + noiseOffset) * noiseToCountRatio);
// ============================================================================

TEST_F(CountOnEveryLayerAndNoisePlacementTest, NoiseBasedCountMatchesCeilFormula)
{
    constexpr i32 px = 64, pz = 64;
    constexpr f64 factor = 200.0;
    constexpr f64 offset = 0.5;
    constexpr i32 ratio = 4;
    const f64 d0 = world::biome::biomeInfoNoise().getValue(px / factor, pz / factor, false);
    const i32 expected = static_cast<i32>(std::ceil((d0 + offset) * static_cast<f64>(ratio)));

    NoiseBasedCountConfig config(ratio, factor, offset);

    NoiseBasedCountPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(px, 64, pz));

    EXPECT_EQ(static_cast<i32>(result.size()), expected);
}

TEST_F(CountOnEveryLayerAndNoisePlacementTest, NoiseBasedCountNegativeCountClampsToZero)
{
    // 极大负 offset 使 (d0+offset)*ratio 为大负数 → ceil 后仍负 → clamp 到 0。
    NoiseBasedCountConfig config(4, 200.0, -1000.0);

    NoiseBasedCountPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(64, 64, 64));

    EXPECT_TRUE(result.empty());
}

// ============================================================================
// surface_relative_threshold_filter
// MC 1.21.11 SurfaceRelativeThresholdFilter (PlacementFilter):
//   i = getHeight(heightmap, x, z);   // Y+1 语义
//   j = i + minInclusive; k = i + maxInclusive;
//   return j <= y && y <= k;
// 注意：项目 getTopBlockY 返回最高方块 Y（非 Y+1）。MC getHeight 返回 Y+1。
// 这里 config 的 min/max 是相对高度图原值的偏移，placement 实现内部须用与 MC 一致的
// "高度图原值"（即 getTopBlockY+1）做基准，确保与原版数值语义一致。
// ============================================================================

TEST_F(CountOnEveryLayerAndNoisePlacementTest, SurfaceRelativeThresholdFilterPassesWithinRange)
{
    // (8,*,8) 列 stone@70 → MOTION_BLOCKING 高度图原值 = 71（Y+1）。
    // min=-2, max=2 → 允许 y ∈ [71-2, 71+2] = [69, 73]。basePos.y=70 在范围内 → 保留。
    placeBlock(8, 70, 8, &VanillaBlocks::STONE->defaultState());

    SurfaceRelativeThresholdFilterConfig config(HeightmapType::MotionBlocking, -2, 2);

    SurfaceRelativeThresholdFilterPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(8, 70, 8));

    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].y, 70);
}

TEST_F(CountOnEveryLayerAndNoisePlacementTest, SurfaceRelativeThresholdFilterRejectsOutOfRange)
{
    // 同列 stone@70，高度图原值=71。min=-2,max=2 → [69,73]。basePos.y=75 超出 → 过滤。
    placeBlock(8, 70, 8, &VanillaBlocks::STONE->defaultState());

    SurfaceRelativeThresholdFilterConfig config(HeightmapType::MotionBlocking, -2, 2);

    SurfaceRelativeThresholdFilterPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(8, 75, 8));

    EXPECT_TRUE(result.empty());
}

TEST_F(CountOnEveryLayerAndNoisePlacementTest, SurfaceRelativeThresholdFilterDefaultBoundsAlwaysPass)
{
    // min/max 缺省 = INT_MIN/INT_MAX → 任意 y 都通过。
    placeBlock(8, 70, 8, &VanillaBlocks::STONE->defaultState());

    SurfaceRelativeThresholdFilterConfig config(HeightmapType::MotionBlocking); // 用缺省

    SurfaceRelativeThresholdFilterPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> result = placement.getPositions(*m_region, random, config, BlockPos(8, 200, 8));

    ASSERT_EQ(result.size(), 1u);
}
