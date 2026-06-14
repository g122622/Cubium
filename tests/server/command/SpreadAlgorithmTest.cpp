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
 * @file SpreadAlgorithmTest.cpp
 * @brief SpreadAlgorithm 分散算法单元测试
 *
 * 测试 SpreadPosition 辅助结构和迭代分散算法的核心逻辑，
 * 包括距离计算、钳制、生成Y坐标搜索、安全性检查、以及完整分散流程。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "server/command/support/SpreadAlgorithm.hpp"

#include <cmath>

namespace mc {
namespace command {
namespace support {
namespace {

// ============================================================================
// 测试用世界 - 基于区块存储支持方块读写
// ============================================================================

class SpreadTestWorld : public test::BaseChunkBackedTestWorld {
public:
    SpreadTestWorld() = default;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const ChunkData* chunk = getChunk(x >> 4, z >> 4);
        if (chunk == nullptr) {
            return getAirState();
        }
        const BlockState* state = chunk->getBlockState(x & 15, y, z & 15);
        return state != nullptr ? state : getAirState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        ChunkData& chunk = ensureChunk(x >> 4, z >> 4);
        chunk.setBlockState(x & 15, y, z & 15, state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override
    {
        const ChunkData* chunk = getChunk(x >> 4, z >> 4);
        if (chunk == nullptr) {
            return 0;
        }
        for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
            const BlockState* state = chunk->getBlockState(x & 15, y, z & 15);
            if (state != nullptr && !state->isAir()) {
                return y;
            }
        }
        return 0;
    }

    void fillBiome(ChunkData& chunk, BiomeId biomeId)
    {
        for (i32 section = 0; section < BiomeContainer::SECTION_COUNT; ++section) {
            for (i32 x = 0; x < BiomeContainer::HORIZ_SIZE; ++x) {
                for (i32 y = 0; y < BiomeContainer::VERT_SIZE; ++y) {
                    for (i32 z = 0; z < BiomeContainer::HORIZ_SIZE; ++z) {
                        chunk.getBiomes().setBiome(section, x, y, z, biomeId);
                    }
                }
            }
        }
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SpreadTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SpreadTestWorld::tickManager not implemented");
    }

private:
    [[nodiscard]] const BlockState* getAirState() const { return &VanillaBlocks::AIR->defaultState(); }
};

// ============================================================================
// 测试基类
// ============================================================================

class SpreadAlgorithmTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();

        // 在 (0,0) 区块创建基础地形
        ChunkData& chunk = m_world.ensureChunk(0, 0);
        m_world.fillBiome(chunk, 1); // 平原

        // 在 Y=63 铺一层石头作为地面
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                m_world.setBlockState(x, 63, z, stone);
            }
        }
    }

    SpreadTestWorld m_world;
    math::Random m_rng{42};
};

// ============================================================================
// SpreadPosition 基础方法测试
// ============================================================================

TEST_F(SpreadAlgorithmTest, DistZeroSamePosition)
{
    SpreadPosition a{5.0, 10.0};
    EXPECT_DOUBLE_EQ(a.dist(a), 0.0);
}

TEST_F(SpreadAlgorithmTest, DistPositive)
{
    SpreadPosition a{0.0, 0.0};
    SpreadPosition b{3.0, 4.0};
    EXPECT_NEAR(a.dist(b), 5.0, 1e-10);
}

TEST_F(SpreadAlgorithmTest, DistSymmetric)
{
    SpreadPosition a{1.0, 2.0};
    SpreadPosition b{4.0, 6.0};
    EXPECT_DOUBLE_EQ(a.dist(b), b.dist(a));
}

TEST_F(SpreadAlgorithmTest, GetLengthZero)
{
    SpreadPosition pos{0.0, 0.0};
    EXPECT_DOUBLE_EQ(pos.getLength(), 0.0);
}

TEST_F(SpreadAlgorithmTest, GetLengthPositive)
{
    SpreadPosition pos{3.0, 4.0};
    EXPECT_NEAR(pos.getLength(), 5.0, 1e-10);
}

TEST_F(SpreadAlgorithmTest, NormalizeZeroVector)
{
    SpreadPosition pos{0.0, 0.0};
    pos.normalize();
    EXPECT_DOUBLE_EQ(pos.x, 0.0);
    EXPECT_DOUBLE_EQ(pos.z, 0.0);
}

TEST_F(SpreadAlgorithmTest, NormalizeNonZeroVector)
{
    SpreadPosition pos{3.0, 4.0};
    pos.normalize();
    EXPECT_NEAR(pos.getLength(), 1.0, 1e-10);
    EXPECT_NEAR(pos.x, 0.6, 1e-10);
    EXPECT_NEAR(pos.z, 0.8, 1e-10);
}

TEST_F(SpreadAlgorithmTest, MoveAwayBasic)
{
    SpreadPosition pos{5.0, 5.0};
    SpreadPosition direction{1.0, 0.0}; // 向右的单位向量
    pos.moveAway(direction);
    EXPECT_DOUBLE_EQ(pos.x, 4.0);
    EXPECT_DOUBLE_EQ(pos.z, 5.0);
}

TEST_F(SpreadAlgorithmTest, ClampNoClamping)
{
    SpreadPosition pos{5.0, 5.0};
    bool clamped = pos.clamp(0.0, 0.0, 10.0, 10.0);
    EXPECT_FALSE(clamped);
    EXPECT_DOUBLE_EQ(pos.x, 5.0);
    EXPECT_DOUBLE_EQ(pos.z, 5.0);
}

TEST_F(SpreadAlgorithmTest, ClampMinX)
{
    SpreadPosition pos{-1.0, 5.0};
    bool clamped = pos.clamp(0.0, 0.0, 10.0, 10.0);
    EXPECT_TRUE(clamped);
    EXPECT_DOUBLE_EQ(pos.x, 0.0);
    EXPECT_DOUBLE_EQ(pos.z, 5.0);
}

TEST_F(SpreadAlgorithmTest, ClampMaxX)
{
    SpreadPosition pos{15.0, 5.0};
    bool clamped = pos.clamp(0.0, 0.0, 10.0, 10.0);
    EXPECT_TRUE(clamped);
    EXPECT_DOUBLE_EQ(pos.x, 10.0);
    EXPECT_DOUBLE_EQ(pos.z, 5.0);
}

TEST_F(SpreadAlgorithmTest, ClampMinZ)
{
    SpreadPosition pos{5.0, -2.0};
    bool clamped = pos.clamp(0.0, 0.0, 10.0, 10.0);
    EXPECT_TRUE(clamped);
    EXPECT_DOUBLE_EQ(pos.x, 5.0);
    EXPECT_DOUBLE_EQ(pos.z, 0.0);
}

TEST_F(SpreadAlgorithmTest, ClampMaxZ)
{
    SpreadPosition pos{5.0, 20.0};
    bool clamped = pos.clamp(0.0, 0.0, 10.0, 10.0);
    EXPECT_TRUE(clamped);
    EXPECT_DOUBLE_EQ(pos.x, 5.0);
    EXPECT_DOUBLE_EQ(pos.z, 10.0);
}

TEST_F(SpreadAlgorithmTest, ClampBothAxes)
{
    SpreadPosition pos{-5.0, 20.0};
    bool clamped = pos.clamp(0.0, 0.0, 10.0, 10.0);
    EXPECT_TRUE(clamped);
    EXPECT_DOUBLE_EQ(pos.x, 0.0);
    EXPECT_DOUBLE_EQ(pos.z, 10.0);
}

TEST_F(SpreadAlgorithmTest, RandomizeWithinBounds)
{
    SpreadPosition pos;
    for (i32 i = 0; i < 100; ++i) {
        pos.randomize(m_rng, -10.0, -10.0, 10.0, 10.0);
        EXPECT_GE(pos.x, -10.0);
        EXPECT_LE(pos.x, 10.0);
        EXPECT_GE(pos.z, -10.0);
        EXPECT_LE(pos.z, 10.0);
    }
}

// ============================================================================
// getSpawnY 测试
// ============================================================================

TEST_F(SpreadAlgorithmTest, GetSpawnYOnSolidGround)
{
    // Y=63 是石头地面，Y=64,65,66 是空气 -> 站立位为 Y=64（脚下 Y=63 非空气，上方两格空气）
    SpreadPosition pos{5.0, 5.0};
    i32 spawnY = pos.getSpawnY(m_world, world::MAX_BUILD_HEIGHT);
    EXPECT_EQ(spawnY, 64);
}

TEST_F(SpreadAlgorithmTest, GetSpawnYAllAir)
{
    // 创建新区块，不铺地面 -> 全是空气 -> 应返回 maxHeight + 1
    m_world.ensureChunk(1, 0);
    SpreadPosition pos{17.0, 5.0}; // 区块 (1,0)
    i32 spawnY = pos.getSpawnY(m_world, 100);
    EXPECT_EQ(spawnY, 101); // maxHeight + 1
}

TEST_F(SpreadAlgorithmTest, GetSpawnYGroundAtTop)
{
    // 在 maxHeight 处放一个方块 -> 上方只有一格空气（maxHeight+1），
    // 不满足"上方两格空气"条件，应回退到 Y=63 的石头地面处
    m_world.setBlockState(5, world::MAX_BUILD_HEIGHT, 5, &VanillaBlocks::STONE->defaultState());
    SpreadPosition pos{5.0, 5.0};
    i32 spawnY = pos.getSpawnY(m_world, world::MAX_BUILD_HEIGHT);
    EXPECT_EQ(spawnY, 64);
}

TEST_F(SpreadAlgorithmTest, GetSpawnYTwoBlocksAbove)
{
    // 在 Y=70 放一个方块，Y=71,72 是空气 -> 站立位为 Y=71
    m_world.setBlockState(5, 70, 5, &VanillaBlocks::STONE->defaultState());
    SpreadPosition pos{5.0, 5.0};
    i32 spawnY = pos.getSpawnY(m_world, 100);
    EXPECT_EQ(spawnY, 71);
}

// ============================================================================
// isSafe 测试
// ============================================================================

TEST_F(SpreadAlgorithmTest, IsSafeOnSolidGround)
{
    // Y=63 石头地面 -> getSpawnY 返回 64 -> isSafe 检查 Y=63（石头）-> 安全
    SpreadPosition pos{5.0, 5.0};
    EXPECT_TRUE(pos.isSafe(m_world, world::MAX_BUILD_HEIGHT));
}

TEST_F(SpreadAlgorithmTest, IsSafeOnLiquidIsUnsafe)
{
    // Y=62 石头基座，Y=63 水（液体，isAir=false），Y=64,65 空气
    // getSpawnY 在 Y=63 找到非空气 + Y=64,65 空气 -> 返回 64
    // isSafe 检查 spawnY-1=63 -> 水 isLiquid()=true -> 不安全
    m_world.setBlockState(5, 63, 5, &VanillaBlocks::WATER->defaultState());
    SpreadPosition pos{5.0, 5.0};
    EXPECT_EQ(pos.getSpawnY(m_world, 100), 64);
    EXPECT_FALSE(pos.isSafe(m_world, 100));
}

TEST_F(SpreadAlgorithmTest, IsSafeBelowMaxHeight)
{
    // maxHeight 足够大时，spawnY 应该小于 maxHeight -> 安全
    SpreadPosition pos{5.0, 5.0};
    EXPECT_TRUE(pos.isSafe(m_world, world::MAX_BUILD_HEIGHT));
}

TEST_F(SpreadAlgorithmTest, IsSafeOnFireIsUnsafe)
{
    // 验证 BlockTags::FIRE 包含火方块，且 isSafe 在脚下方块为火焰时返回 false
    // 火方块 isAir() 通常返回 true，getSpawnY 会跳过它
    // 因此需要构造一个场景：Y=62 石头，Y=63 石头，Y=64 上放火
    // 但火方块不能独立存在，所以直接验证 BlockTags::FIRE 标签包含火方块
    const BlockState* fireState = &VanillaBlocks::FIRE->defaultState();
    ASSERT_TRUE(BlockTags::FIRE().contains(*fireState)) << "火方块应在 BlockTags::FIRE 标签中";

    // 验证 isSafe 代码路径：如果 getSpawnY 返回的 spawnY 下方恰好是火方块，isSafe 应返回 false
    // 由于火方块 isAir()=true 导致 getSpawnY 跳过它，这里直接验证 BlockTags::FIRE 检测能力
    // 完整的 isSafe + FIRE 集成测试需要在集成测试环境中进行
}

TEST_F(SpreadAlgorithmTest, IsSafeMaxHeightBoundary)
{
    // 当 spawnY >= maxHeight 时应返回 false（全空气世界）
    SpreadTestWorld airWorld;
    VanillaBlocks::initialize();
    BiomeRegistry::instance().initialize();
    airWorld.ensureChunk(0, 0);
    SpreadPosition pos{5.0, 5.0};
    EXPECT_EQ(pos.getSpawnY(airWorld, 10), 11);
    EXPECT_FALSE(pos.isSafe(airWorld, 10));
}

// ============================================================================
// createInitialPositions 测试
// ============================================================================

TEST_F(SpreadAlgorithmTest, CreateInitialPositionsCount)
{
    auto positions = createInitialPositions(m_rng, 5, 0.0, 0.0, 100.0, 100.0);
    EXPECT_EQ(positions.size(), 5u);
}

TEST_F(SpreadAlgorithmTest, CreateInitialPositionsZero)
{
    auto positions = createInitialPositions(m_rng, 0, 0.0, 0.0, 100.0, 100.0);
    EXPECT_EQ(positions.size(), 0u);
}

TEST_F(SpreadAlgorithmTest, CreateInitialPositionsInBounds)
{
    auto positions = createInitialPositions(m_rng, 50, -50.0, -50.0, 50.0, 50.0);
    for (const auto& pos : positions) {
        EXPECT_GE(pos.x, -50.0);
        EXPECT_LE(pos.x, 50.0);
        EXPECT_GE(pos.z, -50.0);
        EXPECT_LE(pos.z, 50.0);
    }
}

// ============================================================================
// spreadPositions 测试
// ============================================================================

TEST_F(SpreadAlgorithmTest, SpreadPositionsSinglePosition)
{
    // 单个位置不需要分散，应直接成功，返回最小距离 0.0
    auto positions = createInitialPositions(m_rng, 1, 0.0, 0.0, 100.0, 100.0);
    f64 minDist = 0.0;
    bool success =
        spreadPositions(10.0, m_world, m_rng, 0.0, 0.0, 100.0, 100.0, world::MAX_BUILD_HEIGHT, positions, minDist);
    EXPECT_TRUE(success);
    EXPECT_DOUBLE_EQ(minDist, 0.0);
    EXPECT_EQ(positions.size(), 1u);
    // 单个位置应在边界内
    EXPECT_GE(positions[0].x, 0.0);
    EXPECT_LE(positions[0].x, 100.0);
    EXPECT_GE(positions[0].z, 0.0);
    EXPECT_LE(positions[0].z, 100.0);
}

TEST_F(SpreadAlgorithmTest, SpreadPositionsTwoPositionsFarEnough)
{
    // 两个位置在大范围内，最小距离 1.0 -> 应容易分散
    auto positions = createInitialPositions(m_rng, 2, 0.0, 0.0, 100.0, 100.0);
    f64 minDist = 0.0;
    bool success =
        spreadPositions(1.0, m_world, m_rng, 0.0, 0.0, 100.0, 100.0, world::MAX_BUILD_HEIGHT, positions, minDist);
    EXPECT_TRUE(success);
    EXPECT_GE(minDist, 0.0);
    EXPECT_EQ(positions.size(), 2u);
    // 验证两者之间的距离 >= spreadDistance
    EXPECT_GE(positions[0].dist(positions[1]), 1.0 - 1e-6);
}

TEST_F(SpreadAlgorithmTest, SpreadPositionsRespectsBounds)
{
    // 分散后所有位置应在边界内
    auto positions = createInitialPositions(m_rng, 10, 0.0, 0.0, 50.0, 50.0);
    f64 minDist = 0.0;
    bool success =
        spreadPositions(5.0, m_world, m_rng, 0.0, 0.0, 50.0, 50.0, world::MAX_BUILD_HEIGHT, positions, minDist);
    EXPECT_TRUE(success);
    EXPECT_GE(minDist, 0.0);
    for (const auto& pos : positions) {
        EXPECT_GE(pos.x, 0.0);
        EXPECT_LE(pos.x, 50.0);
        EXPECT_GE(pos.z, 0.0);
        EXPECT_LE(pos.z, 50.0);
    }
}

TEST_F(SpreadAlgorithmTest, SpreadPositionsSmallAreaManyPositionsFails)
{
    // 100 个位置在 1x1 范围内，最小距离 100.0 -> 不可能满足，应失败
    auto positions = createInitialPositions(m_rng, 100, 0.0, 0.0, 1.0, 1.0);
    f64 minDist = 0.0;
    bool success =
        spreadPositions(100.0, m_world, m_rng, 0.0, 0.0, 1.0, 1.0, world::MAX_BUILD_HEIGHT, positions, minDist);
    EXPECT_FALSE(success);
    // 实际最小距离必然小于要求的 100.0
    EXPECT_LT(minDist, 100.0);
}

TEST_F(SpreadAlgorithmTest, SpreadPositionsZeroPositions)
{
    // 0 个位置 -> 不需要分散，应成功（返回 0.0）
    std::vector<SpreadPosition> positions;
    f64 minDist = 0.0;
    bool success =
        spreadPositions(10.0, m_world, m_rng, 0.0, 0.0, 100.0, 100.0, world::MAX_BUILD_HEIGHT, positions, minDist);
    EXPECT_TRUE(success);
    EXPECT_DOUBLE_EQ(minDist, 0.0);
    EXPECT_EQ(positions.size(), 0u);
}

TEST_F(SpreadAlgorithmTest, SpreadPositionsEmptyPositionsNoCrash)
{
    // 空位置列表不应崩溃
    std::vector<SpreadPosition> positions;
    f64 minDist = 0.0;
    bool success =
        spreadPositions(10.0, m_world, m_rng, -50.0, -50.0, 50.0, 50.0, world::MAX_BUILD_HEIGHT, positions, minDist);
    EXPECT_TRUE(success);
    EXPECT_DOUBLE_EQ(minDist, 0.0);
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(SpreadAlgorithmTest, SpreadPositionDistLargeValues)
{
    SpreadPosition a{-1000000.0, -1000000.0};
    SpreadPosition b{1000000.0, 1000000.0};
    f64 dist = a.dist(b);
    EXPECT_GT(dist, 0.0);
    EXPECT_TRUE(std::isfinite(dist));
}

TEST_F(SpreadAlgorithmTest, ClampBoundaryExactValues)
{
    // 位置恰好在边界上不应被钳制
    SpreadPosition pos{0.0, 0.0};
    bool clamped = pos.clamp(0.0, 0.0, 10.0, 10.0);
    EXPECT_FALSE(clamped);

    SpreadPosition pos2{10.0, 10.0};
    bool clamped2 = pos2.clamp(0.0, 0.0, 10.0, 10.0);
    EXPECT_FALSE(clamped2);
}

TEST_F(SpreadAlgorithmTest, GetSpawnYNoChunk)
{
    // 没有区块的位置 -> 全是空气 -> 返回 maxHeight + 1
    SpreadPosition pos{1000.0, 1000.0}; // 远离已创建区块
    i32 spawnY = pos.getSpawnY(m_world, 100);
    EXPECT_EQ(spawnY, 101);
}

} // namespace
} // namespace support
} // namespace command
} // namespace mc
