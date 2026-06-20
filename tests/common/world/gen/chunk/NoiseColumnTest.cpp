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
// NoiseColumn 单元测试
//
// 测试覆盖：
// 1. 构造函数（默认构造、指定 minY 和 height）
// 2. getBlock/setBlock 的索引计算
// 3. 边界条件（越界访问）
// 4. 与 MC 1.21.11 NoiseColumn 的行为对齐
// ============================================================================

#include "common/world/gen/chunk/NoiseColumn.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;

namespace {

class NoiseColumnTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

// ============================================================================
// 1. 默认构造
// ============================================================================

TEST_F(NoiseColumnTest, DefaultConstructor)
{
    NoiseColumn column;
    EXPECT_EQ(column.minY(), 0);
    EXPECT_EQ(column.height(), 0);
}

// ============================================================================
// 2. 指定 minY 和 height 的构造
// ============================================================================

TEST_F(NoiseColumnTest, ConstructorWithMinYAndHeight)
{
    // MC 1.21: 主世界 minY=-64, height=384
    NoiseColumn column(-64, 384);
    EXPECT_EQ(column.minY(), -64);
    EXPECT_EQ(column.height(), 384);
}

TEST_F(NoiseColumnTest, ConstructorWithZeroMinY)
{
    // MC 1.21: 末地/下界 minY=0
    NoiseColumn column(0, 128);
    EXPECT_EQ(column.minY(), 0);
    EXPECT_EQ(column.height(), 128);
}

// ============================================================================
// 3. getBlock/setBlock 索引计算
// ============================================================================

TEST_F(NoiseColumnTest, SetAndGetBlock_BasicRoundTrip)
{
    NoiseColumn column(-64, 384);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();

    // 设置 Y=0 的方块
    column.setBlock(0, stone);
    EXPECT_EQ(column.getBlock(0), stone);

    // 设置 Y=-64 的方块（最低点）
    column.setBlock(-64, stone);
    EXPECT_EQ(column.getBlock(-64), stone);

    // 设置 Y=319 的方块（最高点）
    column.setBlock(319, stone);
    EXPECT_EQ(column.getBlock(319), stone);
}

TEST_F(NoiseColumnTest, SetAndGetBlock_MultipleYLevels)
{
    NoiseColumn column(-64, 384);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* dirt = &VanillaBlocks::DIRT->defaultState();
    const BlockState* grass = &VanillaBlocks::GRASS_BLOCK->defaultState();

    column.setBlock(-64, stone);
    column.setBlock(0, dirt);
    column.setBlock(63, grass);

    EXPECT_EQ(column.getBlock(-64), stone);
    EXPECT_EQ(column.getBlock(0), dirt);
    EXPECT_EQ(column.getBlock(63), grass);
}

TEST_F(NoiseColumnTest, GetBlock_UnsetReturnsNull)
{
    // 未设置的方块应返回 nullptr（空气）
    NoiseColumn column(-64, 384);
    EXPECT_EQ(column.getBlock(-64), nullptr);
    EXPECT_EQ(column.getBlock(0), nullptr);
    EXPECT_EQ(column.getBlock(319), nullptr);
}

TEST_F(NoiseColumnTest, SetBlock_OverwritePreviousValue)
{
    NoiseColumn column(-64, 384);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* dirt = &VanillaBlocks::DIRT->defaultState();

    column.setBlock(0, stone);
    EXPECT_EQ(column.getBlock(0), stone);

    // 覆盖
    column.setBlock(0, dirt);
    EXPECT_EQ(column.getBlock(0), dirt);
}

// ============================================================================
// 4. 边界条件
// ============================================================================

TEST_F(NoiseColumnTest, GetBlock_BelowMinY_ReturnsNull)
{
    // MC 1.21: y < minY 返回 null（空气）
    NoiseColumn column(-64, 384);
    EXPECT_EQ(column.getBlock(-65), nullptr);
    EXPECT_EQ(column.getBlock(-100), nullptr);
}

TEST_F(NoiseColumnTest, GetBlock_AboveMaxY_ReturnsNull)
{
    // MC 1.21: y >= minY + height 返回 null（空气）
    NoiseColumn column(-64, 384);
    EXPECT_EQ(column.getBlock(320), nullptr);
    EXPECT_EQ(column.getBlock(500), nullptr);
}

TEST_F(NoiseColumnTest, SetBlock_BelowMinY_SilentlyIgnored)
{
    // MC 1.21: y < minY 时 setBlock 静默忽略
    // 注意：MC 会抛 IllegalArgumentException，Cubium 当前静默忽略
    // 测试不会崩溃即可
    NoiseColumn column(-64, 384);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    EXPECT_NO_THROW(column.setBlock(-65, stone));
    EXPECT_NO_THROW(column.setBlock(-100, stone));

    // 设置不生效
    EXPECT_EQ(column.getBlock(-65), nullptr);
}

TEST_F(NoiseColumnTest, SetBlock_AboveMaxY_SilentlyIgnored)
{
    NoiseColumn column(-64, 384);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    EXPECT_NO_THROW(column.setBlock(320, stone));
    EXPECT_NO_THROW(column.setBlock(500, stone));

    EXPECT_EQ(column.getBlock(320), nullptr);
}

// ============================================================================
// 5. 空列行为
// ============================================================================

TEST_F(NoiseColumnTest, EmptyColumn_GetBlockAlwaysReturnsNull)
{
    NoiseColumn column;
    EXPECT_EQ(column.getBlock(0), nullptr);
    EXPECT_EQ(column.getBlock(-64), nullptr);
    EXPECT_EQ(column.getBlock(319), nullptr);
}

TEST_F(NoiseColumnTest, ZeroHeightColumn)
{
    NoiseColumn column(0, 0);
    EXPECT_EQ(column.minY(), 0);
    EXPECT_EQ(column.height(), 0);
    EXPECT_EQ(column.getBlock(0), nullptr);
}

// ============================================================================
// 6. 与 MC 1.21 对齐：NoiseColumn 索引验证
// ============================================================================

TEST_F(NoiseColumnTest, IndexCalculation_MC21_Alignment)
{
    // MC 1.21: index = y - minY
    // 对于 minY=-64 的主世界列：
    //   y=-64 → index=0
    //   y=0   → index=64
    //   y=319 → index=383
    NoiseColumn column(-64, 384);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();

    column.setBlock(-64, stone);
    column.setBlock(0, stone);
    column.setBlock(319, stone);

    // 所有三个位置应该都能正确读回
    EXPECT_EQ(column.getBlock(-64), stone);
    EXPECT_EQ(column.getBlock(0), stone);
    EXPECT_EQ(column.getBlock(319), stone);

    // 边界外返回 nullptr
    EXPECT_EQ(column.getBlock(-65), nullptr); // y < minY
    EXPECT_EQ(column.getBlock(320), nullptr); // y >= minY + height
}

TEST_F(NoiseColumnTest, FlatWorldColumn_MinY0_Height384)
{
    // FlatChunkGenerator 使用 minY=0, height=384
    NoiseColumn column(0, 384);
    const BlockState* bedrock = &VanillaBlocks::BEDROCK->defaultState();
    const BlockState* dirt = &VanillaBlocks::DIRT->defaultState();

    // 模拟平坦世界层：Y=0 基岩, Y=1-2 泥土
    column.setBlock(0, bedrock);
    column.setBlock(1, dirt);
    column.setBlock(2, dirt);

    EXPECT_EQ(column.getBlock(0), bedrock);
    EXPECT_EQ(column.getBlock(1), dirt);
    EXPECT_EQ(column.getBlock(2), dirt);
    EXPECT_EQ(column.getBlock(3), nullptr);   // 空气
    EXPECT_EQ(column.getBlock(383), nullptr); // 空气
}

// ============================================================================
// 7. setBlock 后 getBlock 的一致性检查
// ============================================================================

TEST_F(NoiseColumnTest, SetBlock_Nullptr_ReturnsNullptr)
{
    // 设置为 nullptr 应覆盖之前的方块
    NoiseColumn column(-64, 384);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();

    column.setBlock(0, stone);
    EXPECT_EQ(column.getBlock(0), stone);

    column.setBlock(0, nullptr);
    EXPECT_EQ(column.getBlock(0), nullptr);
}

// ============================================================================
// 8. 大量 setBlock 性能正确性
// ============================================================================

TEST_F(NoiseColumnTest, SetBlock_AllPositions)
{
    NoiseColumn column(0, 128);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();

    // 设置所有位置
    for (i32 y = 0; y < 128; ++y) {
        column.setBlock(y, stone);
    }

    // 验证所有位置
    for (i32 y = 0; y < 128; ++y) {
        EXPECT_EQ(column.getBlock(y), stone) << "Failed at y=" << y;
    }

    // 边界外
    EXPECT_EQ(column.getBlock(-1), nullptr);
    EXPECT_EQ(column.getBlock(128), nullptr);
}

} // namespace
