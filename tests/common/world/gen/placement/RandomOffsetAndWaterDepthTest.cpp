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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"
#include "common/world/gen/placement/Placements.hpp"
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

class RandomOffsetAndWaterDepthTest : public ::testing::Test {
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
// random_offset placement
// ============================================================================

TEST_F(RandomOffsetAndWaterDepthTest, RandomOffsetZeroSpreadKeepsPosition)
{
    // xz_spread=0, y_spread=0 → 偏移恒为 0，位置不变。
    RandomOffsetConfig config(std::make_unique<vp::ConstantInt>(0), std::make_unique<vp::ConstantInt>(0));

    RandomOffsetPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(8, 64, 8));

    ASSERT_EQ(positions.size(), 1u);
    EXPECT_EQ(positions[0].x, 8);
    EXPECT_EQ(positions[0].y, 64);
    EXPECT_EQ(positions[0].z, 8);
}

TEST_F(RandomOffsetAndWaterDepthTest, RandomOffsetConstantSpreadShiftsByConstant)
{
    // xz_spread=ConstantInt(3), y_spread=ConstantInt(-2)。
    // dx=3, dy=-2, dz=3（dx/dz 各自从同一 ConstantInt 独立采样，结果相同）。
    RandomOffsetConfig config(std::make_unique<vp::ConstantInt>(3), std::make_unique<vp::ConstantInt>(-2));

    RandomOffsetPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(8, 64, 8));

    ASSERT_EQ(positions.size(), 1u);
    EXPECT_EQ(positions[0].x, 11);
    EXPECT_EQ(positions[0].y, 62);
    EXPECT_EQ(positions[0].z, 11);
}

TEST_F(RandomOffsetAndWaterDepthTest, RandomOffsetUniformSpreadStaysInRange)
{
    // xz_spread=uniform(-16,16), y_spread=uniform(-16,16)。多次采样，偏移量恒落在 [-16,16]。
    RandomOffsetConfig config(std::make_unique<vp::UniformInt>(-16, 16), std::make_unique<vp::UniformInt>(-16, 16));

    RandomOffsetPlacement placement;
    math::Random random(7);
    for (i32 i = 0; i < 64; ++i) {
        const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(0, 0, 0));
        ASSERT_EQ(positions.size(), 1u);
        EXPECT_GE(positions[0].x, -16);
        EXPECT_LE(positions[0].x, 16);
        EXPECT_GE(positions[0].y, -16);
        EXPECT_LE(positions[0].y, 16);
        EXPECT_GE(positions[0].z, -16);
        EXPECT_LE(positions[0].z, 16);
    }
}

// ============================================================================
// surface_water_depth_filter placement（项目注册名 water_depth_threshold）
// ============================================================================

TEST_F(RandomOffsetAndWaterDepthTest, WaterDepthShallowWaterPasses)
{
    // basePos(8,64,8)；其下方 2 格水(63,62)，再下方石头(61)。水深=2 <= max_water_depth=2 → 保留。
    const i32 bx = 8, by = 64, bz = 8;
    placeBlock(bx, by - 1, bz, &VanillaBlocks::WATER->defaultState()); // 63
    placeBlock(bx, by - 2, bz, &VanillaBlocks::WATER->defaultState()); // 62
    placeBlock(bx, by - 3, bz, &VanillaBlocks::STONE->defaultState()); // 61（固体，终止计数）

    WaterDepthThresholdConfig config(2);
    WaterDepthThresholdPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(bx, by, bz));

    ASSERT_EQ(positions.size(), 1u) << "水深 2 <= max 2 应保留";
    EXPECT_EQ(positions[0].y, by);
}

TEST_F(RandomOffsetAndWaterDepthTest, WaterDepthDeepWaterFiltered)
{
    // basePos(8,64,8)；其下方 5 格水，再下方石头。水深=5 > max_water_depth=2 → 过滤掉。
    const i32 bx = 8, by = 64, bz = 8;
    for (i32 dy = 1; dy <= 5; ++dy) {
        placeBlock(bx, by - dy, bz, &VanillaBlocks::WATER->defaultState());
    }
    placeBlock(bx, by - 6, bz, &VanillaBlocks::STONE->defaultState());

    WaterDepthThresholdConfig config(2);
    WaterDepthThresholdPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(bx, by, bz));

    EXPECT_TRUE(positions.empty()) << "水深 5 > max 2 应过滤掉";
}

TEST_F(RandomOffsetAndWaterDepthTest, WaterDepthNoWaterPasses)
{
    // basePos 下方直接石头（无水）→ 水深 0 <= max → 保留。
    const i32 bx = 8, by = 64, bz = 8;
    placeBlock(bx, by - 1, bz, &VanillaBlocks::STONE->defaultState());

    WaterDepthThresholdConfig config(0);
    WaterDepthThresholdPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(bx, by, bz));

    ASSERT_EQ(positions.size(), 1u) << "水深 0 <= max 0 应保留";
}

// ============================================================================
// rarity_filter placement
// ============================================================================

TEST_F(RandomOffsetAndWaterDepthTest, RarityFilterChanceOneAlwaysPasses)
{
    // chance=1 → 1/1=1.0，nextFloat() < 1.0f 恒真（nextFloat 返回 [0,1)），始终保留。
    RarityFilterConfig config(1);
    RarityFilterPlacement placement;
    math::Random random(1);
    for (i32 i = 0; i < 16; ++i) {
        const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(8, 64, 8));
        ASSERT_EQ(positions.size(), 1u) << "chance=1 应恒通过";
    }
}

TEST_F(RandomOffsetAndWaterDepthTest, RarityFilterLargeChanceRarelyPasses)
{
    // chance=1000 → 1/1000 概率，64 次采样中通过次数应远小于 64（概率上约 0~2 次）。
    // 不固定断言通过次数（随机），仅验证不崩溃且返回 0 或 1 个位置。
    RarityFilterConfig config(1000);
    RarityFilterPlacement placement;
    math::Random random(1);
    i32 passCount = 0;
    for (i32 i = 0; i < 64; ++i) {
        const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(8, 64, 8));
        EXPECT_LE(positions.size(), 1u);
        if (!positions.empty()) {
            ++passCount;
        }
    }
    EXPECT_LT(passCount, 64) << "chance=1000 不应每次都通过";
}

// ============================================================================
// heightmap placement
// ============================================================================

TEST_F(RandomOffsetAndWaterDepthTest, HeightmapReturnsColumnTopY)
{
    // 在 (8,*,8) 列放一个 stone@70。HeightmapPlacement(WORLD_SURFACE) 应返回 (8, 71, 8)：
    // 生成期间 ctx.getHeight 走 WorldGenRegion.getHeight = Heightmap.getFirstAvailable
    // = 最高方块 Y+1（即上方一格空气的 Y），对齐 MC 1.21.11 HeightmapPlacement。
    // fixture persistedStatus=FEATURES，维护 POST_FEATURES 高度图
    // （WORLD_SURFACE/OCEAN_FLOOR/MOTION_BLOCKING/...），故用 WORLD_SURFACE 而非 WG 变体。
    const i32 bx = 8, bz = 8;
    placeBlock(bx, 70, bz, &VanillaBlocks::STONE->defaultState());

    HeightmapPlacementConfig config(HeightmapType::WorldSurface);
    HeightmapPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(bx, 64, bz));

    ASSERT_EQ(positions.size(), 1u);
    EXPECT_EQ(positions[0].x, bx);
    EXPECT_EQ(positions[0].z, bz);
    EXPECT_EQ(positions[0].y, 71) << "应返回最高方块上方一格空气 Y（blockY+1，对齐 MC）";
}

TEST_F(RandomOffsetAndWaterDepthTest, HeightmapEmptyColumnReturnsEmpty)
{
    // 空列（无方块）→ Heightmap.getFirstAvailable 返回 NO_BLOCK_SENTINEL（= minY-1），
    // k = minY-1 <= minY → 返回空（对齐 MC HeightmapPlacement 的 k > minY 判据）。
    HeightmapPlacementConfig config(HeightmapType::WorldSurface);
    HeightmapPlacement placement;
    math::Random random(1);
    const std::vector<BlockPos> positions = placement.getPositions(*m_region, random, config, BlockPos(8, 64, 8));

    EXPECT_TRUE(positions.empty()) << "空列应返回空（k = NO_BLOCK_SENTINEL <= minY）";
}

// ============================================================================
// heightmapTypeFromString
// ============================================================================

TEST_F(RandomOffsetAndWaterDepthTest, HeightmapTypeFromStringParsesAllVanillaNames)
{
    // MC 6 个全大写序列化名应全部解析成功。
    EXPECT_EQ(heightmapTypeFromString("WORLD_SURFACE_WG"), HeightmapType::WorldSurfaceWG);
    EXPECT_EQ(heightmapTypeFromString("WORLD_SURFACE"), HeightmapType::WorldSurface);
    EXPECT_EQ(heightmapTypeFromString("OCEAN_FLOOR_WG"), HeightmapType::OceanFloorWG);
    EXPECT_EQ(heightmapTypeFromString("OCEAN_FLOOR"), HeightmapType::OceanFloor);
    EXPECT_EQ(heightmapTypeFromString("MOTION_BLOCKING"), HeightmapType::MotionBlocking);
    EXPECT_EQ(heightmapTypeFromString("MOTION_BLOCKING_NO_LEAVES"), HeightmapType::MotionBlockingNoLeaves);
}

TEST_F(RandomOffsetAndWaterDepthTest, HeightmapTypeFromStringRejectsUnknown)
{
    EXPECT_FALSE(heightmapTypeFromString("world_surface_wg").has_value()) << "小写应被拒绝（MC 用全大写）";
    EXPECT_FALSE(heightmapTypeFromString("UNKNOWN").has_value());
    EXPECT_FALSE(heightmapTypeFromString("").has_value());
}
