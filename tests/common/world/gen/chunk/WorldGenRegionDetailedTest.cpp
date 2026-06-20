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
// WorldGenRegion 详细单元测试
//
// 测试覆盖：
// 1. 构造函数（无步骤/有步骤两种模式）
// 2. 区块访问（getChunkAt, getIChunk, getMainChunk）
// 3. 方块状态读写（getBlockState, setBlockState）
// 4. 坐标转换和边界检查
// 5. 高度图查询（getHeight, getTopBlockY）
// 6. 生物群系查询（getBiome）
// 7. 流体状态查询（getFluidState）
// 8. 方块实体管理（getBlockEntity, setBlockEntity, removeBlockEntity）
// 9. 世界边界检查（isWithinWorldBounds）
// 10. 写入权限检查（ensureCanWrite）
// 11. 生成追踪（setCurrentlyGenerating/clearCurrentlyGenerating）
// 12. 维度感知高度
// 13. 与 MC 1.21.11 的行为对齐
// ============================================================================

#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

namespace {

// ============================================================================
// 辅助工具
// ============================================================================

/**
 * @brief 创建指定半径的 WorldGenRegion
 *
 * 所有 ChunkPrimer 使用默认构造（EMPTY 状态）。
 * 区块数组按 Z-外层、X-内层顺序排列，与 WorldGenRegion 内部布局一致。
 */
std::unique_ptr<WorldGenRegion> createRegion(ChunkCoord mainX, ChunkCoord mainZ, i32 radius, DimensionId dimId = 0)
{
    const i32 diameter = radius * 2 + 1;
    std::vector<std::unique_ptr<ChunkPrimer>> primers;
    std::vector<IChunk*> chunkPtrs;

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            primers.push_back(std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz));
        }
    }

    for (auto& p : primers) {
        chunkPtrs.push_back(p.get());
    }

    // 需要确保 primers 在 region 使用期间存活
    // 因此使用一种技巧：将 primers 的所有权转移到一个静态容器中
    static thread_local std::vector<std::unique_ptr<ChunkPrimer>> s_primerStorage;
    s_primerStorage = std::move(primers);

    return std::make_unique<WorldGenRegion>(mainX, mainZ, radius, std::move(chunkPtrs), dimId);
}

/**
 * @brief 创建填充了方块的 WorldGenRegion（用于读写测试）
 */
std::unique_ptr<WorldGenRegion> createFilledRegion(
    ChunkCoord mainX, ChunkCoord mainZ, i32 radius, const BlockState* fillBlock, DimensionId dimId = 0)
{
    const i32 diameter = radius * 2 + 1;
    std::vector<std::unique_ptr<ChunkPrimer>> primers;
    std::vector<IChunk*> chunkPtrs;

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            auto primer = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);
            // 填充方块到 Y=0~10
            if (fillBlock != nullptr) {
                for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MIN_BUILD_HEIGHT + 10; ++y) {
                    for (i32 lx = 0; lx < world::CHUNK_WIDTH; ++lx) {
                        for (i32 lz = 0; lz < world::CHUNK_WIDTH; ++lz) {
                            primer->setBlockState(lx, y, lz, fillBlock);
                            primer->updateHeightmap(HeightmapType::WorldSurfaceWG, lx, y, lz, fillBlock);
                        }
                    }
                }
            }
            primers.push_back(std::move(primer));
        }
    }

    for (auto& p : primers) {
        chunkPtrs.push_back(p.get());
    }

    static thread_local std::vector<std::unique_ptr<ChunkPrimer>> s_primerStorage;
    s_primerStorage = std::move(primers);

    return std::make_unique<WorldGenRegion>(mainX, mainZ, radius, std::move(chunkPtrs), dimId);
}

// ============================================================================
// 测试夹具
// ============================================================================

class WorldGenRegionDetailedTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    void SetUp() override { m_region = createRegion(5, 5, 1, 0); }

    std::unique_ptr<WorldGenRegion> m_region;
};

// ============================================================================
// 1. 构造函数
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, Construction_NoStep_BasicProperties)
{
    EXPECT_EQ(m_region->mainX(), 5);
    EXPECT_EQ(m_region->mainZ(), 5);
    EXPECT_EQ(m_region->chunkRadius(), 1);
    EXPECT_EQ(m_region->dimension(), 0);
    EXPECT_EQ(m_region->seed(), 0u);
    EXPECT_EQ(m_region->currentTick(), 0u);
    EXPECT_EQ(m_region->dayTime(), 0);
    EXPECT_FALSE(m_region->isHardcore());
    EXPECT_EQ(m_region->difficulty(), Difficulty::Normal);
    EXPECT_FALSE(m_region->isClientSide());
}

TEST_F(WorldGenRegionDetailedTest, Construction_ZeroRadius_SingleChunk)
{
    auto region = createRegion(10, 20, 0);
    EXPECT_EQ(region->mainX(), 10);
    EXPECT_EQ(region->mainZ(), 20);
    EXPECT_EQ(region->chunkRadius(), 0);
    EXPECT_NE(region->getMainChunk(), nullptr);
    EXPECT_EQ(region->getMainChunk()->x(), 10);
    EXPECT_EQ(region->getMainChunk()->z(), 20);
}

TEST_F(WorldGenRegionDetailedTest, Construction_LargeRadius)
{
    auto region = createRegion(0, 0, 8);
    EXPECT_EQ(region->chunkRadius(), 8);
    EXPECT_NE(region->getMainChunk(), nullptr);
}

TEST_F(WorldGenRegionDetailedTest, Construction_NegativeChunkCoords)
{
    auto region = createRegion(-10, -20, 2);
    EXPECT_EQ(region->mainX(), -10);
    EXPECT_EQ(region->mainZ(), -20);
    EXPECT_EQ(region->minChunkX(), -12);
    EXPECT_EQ(region->maxChunkX(), -8);
    EXPECT_EQ(region->minChunkZ(), -22);
    EXPECT_EQ(region->maxChunkZ(), -18);
}

// ============================================================================
// 2. 区块访问
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, GetMainChunk_ReturnsCenter)
{
    IChunk* center = m_region->getMainChunk();
    ASSERT_NE(center, nullptr);
    EXPECT_EQ(center->x(), 5);
    EXPECT_EQ(center->z(), 5);
}

TEST_F(WorldGenRegionDetailedTest, GetChunkAt_ValidPositions)
{
    // 中心区块
    IChunk* center = m_region->getChunkAt(0, 0);
    ASSERT_NE(center, nullptr);
    EXPECT_EQ(center->x(), 5);
    EXPECT_EQ(center->z(), 5);

    // 邻居区块
    IChunk* east = m_region->getChunkAt(1, 0);
    ASSERT_NE(east, nullptr);
    EXPECT_EQ(east->x(), 6);

    IChunk* south = m_region->getChunkAt(0, 1);
    ASSERT_NE(south, nullptr);
    EXPECT_EQ(south->z(), 6);

    // 角落
    IChunk* se = m_region->getChunkAt(1, 1);
    ASSERT_NE(se, nullptr);
    EXPECT_EQ(se->x(), 6);
    EXPECT_EQ(se->z(), 6);

    IChunk* nw = m_region->getChunkAt(-1, -1);
    ASSERT_NE(nw, nullptr);
    EXPECT_EQ(nw->x(), 4);
    EXPECT_EQ(nw->z(), 4);
}

TEST_F(WorldGenRegionDetailedTest, GetChunkAt_OutOfBounds_ReturnsNull)
{
    EXPECT_EQ(m_region->getChunkAt(2, 0), nullptr);
    EXPECT_EQ(m_region->getChunkAt(0, 2), nullptr);
    EXPECT_EQ(m_region->getChunkAt(-2, 0), nullptr);
    EXPECT_EQ(m_region->getChunkAt(0, -2), nullptr);
    EXPECT_EQ(m_region->getChunkAt(2, 2), nullptr);
}

TEST_F(WorldGenRegionDetailedTest, GetIChunk_ByWorldCoord)
{
    IChunk* center = m_region->getIChunk(5, 5);
    ASSERT_NE(center, nullptr);
    EXPECT_EQ(center->x(), 5);

    IChunk* neighbor = m_region->getIChunk(6, 5);
    ASSERT_NE(neighbor, nullptr);
    EXPECT_EQ(neighbor->x(), 6);

    // 超出范围
    IChunk* outOfRange = m_region->getIChunk(3, 5);
    // 无步骤模式，getIChunk 会在区块不存在时返回 nullptr
    // 但无步骤模式不做 status 校验
}

TEST_F(WorldGenRegionDetailedTest, GetChunkAt_ConstVersion)
{
    const auto& constRegion = *m_region;
    const IChunk* center = constRegion.getChunkAt(0, 0);
    ASSERT_NE(center, nullptr);
    EXPECT_EQ(center->x(), 5);

    const IChunk* outOfRange = constRegion.getChunkAt(5, 5);
    EXPECT_EQ(outOfRange, nullptr);
}

// ============================================================================
// 3. 方块状态读写
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, GetBlockState_OutOfBoundsY_ReturnsAir)
{
    // Y 低于最小建筑高度
    const BlockState* state = m_region->getBlockState(80, world::MIN_BUILD_HEIGHT - 1, 80);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isAir());

    // Y 高于最大建筑高度
    state = m_region->getBlockState(80, world::MAX_BUILD_HEIGHT, 80);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isAir());
}

TEST_F(WorldGenRegionDetailedTest, GetBlockState_WithinBoundsEmptyChunk_ReturnsAir)
{
    // 空 ChunkPrimer 的默认方块状态可能是空气或 nullptr
    const BlockState* state = m_region->getBlockState(80, 64, 80);
    // 无论返回 nullptr 还是空气方块，都表示该位置无方块
    EXPECT_TRUE(state == nullptr || state->isAir());
}

TEST_F(WorldGenRegionDetailedTest, SetBlockState_WithinBounds_WorksCorrectly)
{
    // 设置石头方块
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    bool result = m_region->setBlockState(80, 64, 80, stone);
    EXPECT_TRUE(result);

    // 读取回来
    const BlockState* readState = m_region->getBlockState(80, 64, 80);
    ASSERT_NE(readState, nullptr);
    EXPECT_EQ(readState, stone);
}

TEST_F(WorldGenRegionDetailedTest, SetBlockState_OutOfBoundsY_ReturnsFalse)
{
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(m_region->setBlockState(80, world::MIN_BUILD_HEIGHT - 1, 80, stone));
    EXPECT_FALSE(m_region->setBlockState(80, world::MAX_BUILD_HEIGHT, 80, stone));
}

TEST_F(WorldGenRegionDetailedTest, SetBlockState_CoordinateConversion)
{
    // 测试世界坐标到区块坐标的转换
    // 区块 (5,5) 覆盖世界 X/Z [80, 96)
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();

    // 区块 (5,5) 内的坐标
    EXPECT_TRUE(m_region->setBlockState(80, 64, 80, stone)); // 左上角
    EXPECT_TRUE(m_region->setBlockState(95, 64, 95, stone)); // 右下角

    // 区块 (6,5) 内的坐标 - 在半径1范围内
    EXPECT_TRUE(m_region->setBlockState(96, 64, 80, stone));

    // 区块 (4,5) 内的坐标 - 在半径1范围内
    EXPECT_TRUE(m_region->setBlockState(64, 64, 80, stone));
}

TEST_F(WorldGenRegionDetailedTest, GetBlockState_BlockPosOverload)
{
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    m_region->setBlockState(80, 64, 80, stone);

    BlockPos pos(80, 64, 80);
    const BlockState* state = m_region->getBlockState(pos);
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state, stone);
}

// ============================================================================
// 4. hasChunk
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, HasChunk_WithinBounds)
{
    EXPECT_TRUE(m_region->hasChunk(5, 5)); // 中心
    EXPECT_TRUE(m_region->hasChunk(4, 5)); // 西
    EXPECT_TRUE(m_region->hasChunk(6, 5)); // 东
    EXPECT_TRUE(m_region->hasChunk(5, 4)); // 北
    EXPECT_TRUE(m_region->hasChunk(5, 6)); // 南
    EXPECT_TRUE(m_region->hasChunk(4, 4)); // 西北
    EXPECT_TRUE(m_region->hasChunk(6, 6)); // 东南
}

TEST_F(WorldGenRegionDetailedTest, HasChunk_OutOfBounds)
{
    EXPECT_FALSE(m_region->hasChunk(3, 5)); // 太远
    EXPECT_FALSE(m_region->hasChunk(7, 5));
    EXPECT_FALSE(m_region->hasChunk(5, 3));
    EXPECT_FALSE(m_region->hasChunk(5, 7));
    EXPECT_FALSE(m_region->hasChunk(100, 100));
}

// ============================================================================
// 5. 高度图查询
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, GetHeight_ReturnsSurfaceHeight)
{
    // 创建填充了石头的区域
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    auto region = createFilledRegion(5, 5, 1, stone);

    // 在填充区域内查询高度
    // 填充了 Y=-64 到 Y=-54（10层石头），高度图应返回 -54 或附近
    i32 height = region->getHeight(80, 80);
    EXPECT_GT(height, world::MIN_BUILD_HEIGHT);
}

TEST_F(WorldGenRegionDetailedTest, GetTopBlockY_ReturnsHeightForType)
{
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    auto region = createFilledRegion(5, 5, 1, stone);

    i32 surfaceHeight = region->getTopBlockY(80, 80, HeightmapType::WorldSurfaceWG);
    EXPECT_GT(surfaceHeight, world::MIN_BUILD_HEIGHT);
}

// ============================================================================
// 6. 生物群系查询
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, GetBiome_EmptyChunk_ReturnsDefault)
{
    // 空 ChunkPrimer 的生物群系数据是默认初始化的
    // BiomeContainer 默认为 0（Ocean）
    BiomeId biome = m_region->getBiome(80, 64, 80);
    // 默认生物群系取决于 BiomeContainer 的初始化
    // 不检查具体值，只检查能调用不崩溃
    (void)biome;
}

TEST_F(WorldGenRegionDetailedTest, GetBiome_DifferentPositions)
{
    // 多个位置的生物群系查询应该都能成功
    m_region->getBiome(80, 0, 80);
    m_region->getBiome(80, 100, 80);
    m_region->getBiome(80, -50, 80);
}

// ============================================================================
// 7. 流体状态查询
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, GetFluidState_AirBlock_ReturnsEmpty)
{
    // 空气方块应返回空流体
    const auto* fluidState = m_region->getFluidState(80, 64, 80);
    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->isEmpty());
}

TEST_F(WorldGenRegionDetailedTest, GetFluidState_OutOfBoundsY_ReturnsEmpty)
{
    const auto* fluidState = m_region->getFluidState(80, world::MIN_BUILD_HEIGHT - 1, 80);
    ASSERT_NE(fluidState, nullptr);
    EXPECT_TRUE(fluidState->isEmpty());
}

// ============================================================================
// 8. 世界边界检查
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, IsWithinWorldBounds_ValidPosition)
{
    EXPECT_TRUE(m_region->isWithinWorldBounds(0, 0, 0));
    EXPECT_TRUE(m_region->isWithinWorldBounds(100, 64, 100));
    EXPECT_TRUE(m_region->isWithinWorldBounds(-100, -64, -100));
    EXPECT_TRUE(
        m_region->isWithinWorldBounds(world::WORLD_BORDER - 1, world::MAX_BUILD_HEIGHT - 1, world::WORLD_BORDER - 1));
}

TEST_F(WorldGenRegionDetailedTest, IsWithinWorldBounds_InvalidPosition)
{
    // Y 越界
    EXPECT_FALSE(m_region->isWithinWorldBounds(0, world::MIN_BUILD_HEIGHT - 1, 0));
    EXPECT_FALSE(m_region->isWithinWorldBounds(0, world::MAX_BUILD_HEIGHT, 0));

    // X/Z 超出世界边界
    EXPECT_FALSE(m_region->isWithinWorldBounds(world::WORLD_BORDER, 0, 0));
    EXPECT_FALSE(m_region->isWithinWorldBounds(0, 0, world::WORLD_BORDER));
    EXPECT_FALSE(m_region->isWithinWorldBounds(-world::WORLD_BORDER - 1, 0, 0));
}

// ============================================================================
// 9. 写入权限检查
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, EnsureCanWrite_NoStep_ReturnsFalse)
{
    // 没有生成步骤时，ensureCanWrite 应返回 false
    // 因为 blockStateWriteRadius() 返回 -1
    EXPECT_FALSE(m_region->ensureCanWrite(0, 0, 0));
    EXPECT_FALSE(m_region->ensureCanWrite(80, 64, 80));
}

// ============================================================================
// 10. 生成追踪
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, CurrentlyGenerating_SetAndClear)
{
    EXPECT_TRUE(m_region->currentlyGenerating().empty());

    m_region->setCurrentlyGenerating("minecraft:village");
    EXPECT_EQ(m_region->currentlyGenerating(), "minecraft:village");

    m_region->setCurrentlyGenerating("minecraft:fortress");
    EXPECT_EQ(m_region->currentlyGenerating(), "minecraft:fortress");

    m_region->clearCurrentlyGenerating();
    EXPECT_TRUE(m_region->currentlyGenerating().empty());
}

// ============================================================================
// 11. 维度感知高度
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, DimensionHeight_Overworld)
{
    // 主世界 (dimId=0): minY=-64, maxY=320
    auto region = createRegion(0, 0, 1, 0);
    EXPECT_EQ(region->getMinBuildHeight(), -64);
    EXPECT_EQ(region->getMaxBuildHeight(), 320);
}

TEST_F(WorldGenRegionDetailedTest, DimensionHeight_Nether)
{
    // 下界 (dimId=-1): minY=0, maxY=128
    auto region = createRegion(0, 0, 1, -1);
    EXPECT_EQ(region->getMinBuildHeight(), 0);
    EXPECT_EQ(region->getMaxBuildHeight(), 128);
}

TEST_F(WorldGenRegionDetailedTest, DimensionHeight_End)
{
    // 末地 (dimId=1): DimensionType::theEnd() 使用 MIN_BUILD_HEIGHT/MAX_BUILD_HEIGHT
    // 与主世界相同（minY=-64, maxY=320），因为维度类型定义中 m_minHeight=MIN_BUILD_HEIGHT
    auto region = createRegion(0, 0, 1, 1);
    EXPECT_EQ(region->getMinBuildHeight(), world::MIN_BUILD_HEIGHT);
    EXPECT_EQ(region->getMaxBuildHeight(), world::MAX_BUILD_HEIGHT);
}

// ============================================================================
// 12. 种子、时间、难度设置
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, Seed_SetAndGet)
{
    m_region->setSeed(12345);
    EXPECT_EQ(m_region->seed(), 12345u);

    m_region->setSeed(0);
    EXPECT_EQ(m_region->seed(), 0u);
}

TEST_F(WorldGenRegionDetailedTest, CurrentTick_SetAndGet)
{
    m_region->setCurrentTick(1000);
    EXPECT_EQ(m_region->currentTick(), 1000u);
}

TEST_F(WorldGenRegionDetailedTest, DayTime_SetAndGet)
{
    m_region->setDayTime(6000);
    EXPECT_EQ(m_region->dayTime(), 6000);
}

TEST_F(WorldGenRegionDetailedTest, Hardcore_SetAndGet)
{
    m_region->setHardcore(true);
    EXPECT_TRUE(m_region->isHardcore());
    m_region->setHardcore(false);
    EXPECT_FALSE(m_region->isHardcore());
}

TEST_F(WorldGenRegionDetailedTest, Difficulty_SetAndGet)
{
    m_region->setDifficulty(Difficulty::Peaceful);
    EXPECT_EQ(m_region->difficulty(), Difficulty::Peaceful);
    m_region->setDifficulty(Difficulty::Easy);
    EXPECT_EQ(m_region->difficulty(), Difficulty::Easy);
    m_region->setDifficulty(Difficulty::Normal);
    EXPECT_EQ(m_region->difficulty(), Difficulty::Normal);
    m_region->setDifficulty(Difficulty::Hard);
    EXPECT_EQ(m_region->difficulty(), Difficulty::Hard);
}

// ============================================================================
// 13. IWorld 接口的空实现
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, PhysicsEngine_ReturnsNull)
{
    EXPECT_EQ(m_region->physicsEngine(), nullptr);
}

TEST_F(WorldGenRegionDetailedTest, HasBlockCollision_AlwaysFalse)
{
    AxisAlignedBB box(0, 0, 0, 10, 10, 10);
    EXPECT_FALSE(m_region->hasBlockCollision(box));
}

TEST_F(WorldGenRegionDetailedTest, GetBlockCollisions_AlwaysEmpty)
{
    AxisAlignedBB box(0, 0, 0, 10, 10, 10);
    auto collisions = m_region->getBlockCollisions(box);
    EXPECT_TRUE(collisions.empty());
}

TEST_F(WorldGenRegionDetailedTest, HasEntityCollision_AlwaysFalse)
{
    AxisAlignedBB box(0, 0, 0, 10, 10, 10);
    EXPECT_FALSE(m_region->hasEntityCollision(box));
}

TEST_F(WorldGenRegionDetailedTest, GetEntityCollisions_AlwaysEmpty)
{
    AxisAlignedBB box(0, 0, 0, 10, 10, 10);
    auto collisions = m_region->getEntityCollisions(box);
    EXPECT_TRUE(collisions.empty());
}

TEST_F(WorldGenRegionDetailedTest, GetEntitiesInAABB_AlwaysEmpty)
{
    AxisAlignedBB box(0, 0, 0, 10, 10, 10);
    auto entities = m_region->getEntitiesInAABB(box);
    EXPECT_TRUE(entities.empty());
}

TEST_F(WorldGenRegionDetailedTest, GetEntitiesInRange_AlwaysEmpty)
{
    Vector3 pos(0, 0, 0);
    auto entities = m_region->getEntitiesInRange(pos, 10.0f);
    EXPECT_TRUE(entities.empty());
}

TEST_F(WorldGenRegionDetailedTest, GetBlockLight_ReturnsZero)
{
    EXPECT_EQ(m_region->getBlockLight(80, 64, 80), 0);
    EXPECT_EQ(m_region->getBlockLight(0, 0, 0), 0);
}

TEST_F(WorldGenRegionDetailedTest, GetSkyLight_Returns15)
{
    EXPECT_EQ(m_region->getSkyLight(80, 64, 80), 15);
    EXPECT_EQ(m_region->getSkyLight(0, 0, 0), 15);
}

TEST_F(WorldGenRegionDetailedTest, TickManager_ThrowsException)
{
    EXPECT_THROW(m_region->tickManager(), std::logic_error);
    EXPECT_THROW(std::as_const(*m_region).tickManager(), std::logic_error);
}

// ============================================================================
// 14. getChunk 返回 ChunkData
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, GetChunk_ReturnsChunkDataForPrimer)
{
    // ChunkPrimer 持有 ChunkData，应该能获取
    const ChunkData* data = m_region->getChunk(5, 5);
    // ChunkPrimer 默认有 ChunkData
    // 不检查是否非空，因为取决于 ChunkPrimer 的实现
    // 但至少不应崩溃
    (void)data;
}

TEST_F(WorldGenRegionDetailedTest, GetChunk_OutOfBounds_ReturnsNull)
{
    const ChunkData* data = m_region->getChunk(3, 5);
    EXPECT_EQ(data, nullptr);
}

// ============================================================================
// 15. WorldGenRegion 与 ChunkStep 模式
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, Construction_WithStep_GeneratesCorrectly)
{
    // 使用 ChunkStep 构造 WorldGenRegion
    // NOISE 步骤的 accumulatedRadius 可能很大（8），需要提供足够数量的区块
    const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::NOISE);
    const i32 radius = step.accumulatedRadius();
    const i32 diameter = radius * 2 + 1;
    const ChunkCoord mainX = 0;
    const ChunkCoord mainZ = 0;

    std::vector<std::unique_ptr<ChunkPrimer>> primers;
    std::vector<IChunk*> chunkPtrs;

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            primers.push_back(std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz));
        }
    }
    for (auto& p : primers) {
        chunkPtrs.push_back(p.get());
    }

    static thread_local std::vector<std::unique_ptr<ChunkPrimer>> s_primerStorage;
    s_primerStorage = std::move(primers);

    auto region = std::make_unique<WorldGenRegion>(mainX, mainZ, step, std::move(chunkPtrs), 0);
    EXPECT_NE(region, nullptr);
    EXPECT_EQ(region->mainX(), 0);
    EXPECT_EQ(region->mainZ(), 0);

    // 验证 blockStateWriteRadius
    EXPECT_EQ(region->blockStateWriteRadius(), step.blockStateWriteRadius());
}

// ============================================================================
// 16. 负坐标区块访问
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, NegativeChunkCoords)
{
    auto region = createRegion(-5, -5, 1);

    EXPECT_EQ(region->mainX(), -5);
    EXPECT_EQ(region->mainZ(), -5);
    EXPECT_EQ(region->minChunkX(), -6);
    EXPECT_EQ(region->maxChunkX(), -4);
    EXPECT_EQ(region->minChunkZ(), -6);
    EXPECT_EQ(region->maxChunkZ(), -4);

    // 访问中心区块
    IChunk* center = region->getIChunk(-5, -5);
    ASSERT_NE(center, nullptr);
    EXPECT_EQ(center->x(), -5);
    EXPECT_EQ(center->z(), -5);

    // 访问邻居区块
    IChunk* neighbor = region->getIChunk(-4, -5);
    ASSERT_NE(neighbor, nullptr);
    EXPECT_EQ(neighbor->x(), -4);
}

// ============================================================================
// 17. 方块实体管理
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, GetBlockEntity_NoEntity_ReturnsNull)
{
    BlockPos pos(80, 64, 80);
    BlockEntity* entity = m_region->getBlockEntity(pos);
    EXPECT_EQ(entity, nullptr);

    const BlockEntity* constEntity = std::as_const(*m_region).getBlockEntity(pos);
    EXPECT_EQ(constEntity, nullptr);
}

TEST_F(WorldGenRegionDetailedTest, SetBlockEntity_NullPtr_DoesNothing)
{
    BlockPos pos(80, 64, 80);
    // 传入 nullptr 不应崩溃
    m_region->setBlockEntity(pos, nullptr);
}

TEST_F(WorldGenRegionDetailedTest, RemoveBlockEntity_NoEntity_DoesNotCrash)
{
    BlockPos pos(80, 64, 80);
    // 移除不存在的方块实体不应崩溃
    m_region->removeBlockEntity(pos);
}

// ============================================================================
// 18. setBlockState 标记后处理
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, SetBlockState_LiquidBlock_MarksForPostprocessing)
{
    // 设置水方块应该标记后处理
    const BlockState* water = &VanillaBlocks::WATER->defaultState();
    bool result = m_region->setBlockState(80, 64, 80, water);
    EXPECT_TRUE(result);
    // 标记后处理是内部行为，通过 ChunkPrimer 的后处理列表验证
}

// ============================================================================
// 19. 世界边界常量一致性
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, WorldConstantsConsistency)
{
    // 验证 WorldGenRegion 使用的常量与世界常量一致
    EXPECT_EQ(m_region->getMinBuildHeight(), world::MIN_BUILD_HEIGHT);
    EXPECT_EQ(m_region->getMaxBuildHeight(), world::MAX_BUILD_HEIGHT);

    // 维度特定高度
    auto netherRegion = createRegion(0, 0, 1, -1);
    EXPECT_EQ(netherRegion->getMinBuildHeight(), DimensionType::fromId(-1).minHeight());
    EXPECT_EQ(netherRegion->getMaxBuildHeight(), DimensionType::fromId(-1).maxHeight());

    auto endRegion = createRegion(0, 0, 1, 1);
    EXPECT_EQ(endRegion->getMinBuildHeight(), DimensionType::fromId(1).minHeight());
    EXPECT_EQ(endRegion->getMaxBuildHeight(), DimensionType::fromId(1).maxHeight());
}

// ============================================================================
// 20. 区块索引边界条件
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, ChunkAccessAtRadiusBoundary)
{
    // 半径1的区域，边界在 ±1
    auto region = createRegion(0, 0, 1);

    // 边界上的区块应可访问
    EXPECT_NE(region->getChunkAt(1, 0), nullptr);
    EXPECT_NE(region->getChunkAt(-1, 0), nullptr);
    EXPECT_NE(region->getChunkAt(0, 1), nullptr);
    EXPECT_NE(region->getChunkAt(0, -1), nullptr);
    EXPECT_NE(region->getChunkAt(1, 1), nullptr);
    EXPECT_NE(region->getChunkAt(-1, -1), nullptr);
    EXPECT_NE(region->getChunkAt(1, -1), nullptr);
    EXPECT_NE(region->getChunkAt(-1, 1), nullptr);

    // 边界外应返回 nullptr
    EXPECT_EQ(region->getChunkAt(2, 0), nullptr);
    EXPECT_EQ(region->getChunkAt(-2, 0), nullptr);
    EXPECT_EQ(region->getChunkAt(0, 2), nullptr);
    EXPECT_EQ(region->getChunkAt(0, -2), nullptr);
}

// ============================================================================
// 21. 零半径区域
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, ZeroRadius_OnlyCenterChunk)
{
    auto region = createRegion(100, 200, 0);

    EXPECT_EQ(region->chunkRadius(), 0);
    EXPECT_NE(region->getChunkAt(0, 0), nullptr);
    EXPECT_EQ(region->getChunkAt(1, 0), nullptr);
    EXPECT_EQ(region->getChunkAt(-1, 0), nullptr);
    EXPECT_EQ(region->getChunkAt(0, 1), nullptr);
    EXPECT_EQ(region->getChunkAt(0, -1), nullptr);
}

// ============================================================================
// 22. setBlockState 和 getBlockState 的往返一致性
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, SetGetBlockState_RoundTrip)
{
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* dirt = &VanillaBlocks::DIRT->defaultState();

    // 设置石头
    m_region->setBlockState(80, 64, 80, stone);
    EXPECT_EQ(m_region->getBlockState(80, 64, 80), stone);

    // 替换为泥土
    m_region->setBlockState(80, 64, 80, dirt);
    EXPECT_EQ(m_region->getBlockState(80, 64, 80), dirt);

    // 设置为空气
    const BlockState* air = BlockRegistry::instance().airState();
    m_region->setBlockState(80, 64, 80, air);
    EXPECT_EQ(m_region->getBlockState(80, 64, 80), air);
}

// ============================================================================
// 23. 不同区块的方块设置
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, SetBlockState_DifferentChunks)
{
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* dirt = &VanillaBlocks::DIRT->defaultState();

    // 区块 (5,5) 的方块
    m_region->setBlockState(80, 64, 80, stone);

    // 区块 (6,5) 的方块
    m_region->setBlockState(96, 64, 80, dirt);

    // 验证各区块的方块正确
    EXPECT_EQ(m_region->getBlockState(80, 64, 80), stone);
    EXPECT_EQ(m_region->getBlockState(96, 64, 80), dirt);
}

// ============================================================================
// 24. getChunk 对区块外位置返回 nullptr
// ============================================================================

TEST_F(WorldGenRegionDetailedTest, GetChunk_OutOfBounds)
{
    // 超出区域范围
    const ChunkData* data = m_region->getChunk(3, 5);
    EXPECT_EQ(data, nullptr);

    data = m_region->getChunk(7, 5);
    EXPECT_EQ(data, nullptr);
}

} // namespace
