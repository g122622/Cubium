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
 * @file MudBlockTest.cpp
 * @brief MudBlock 单元测试
 *
 * 测试泥巴方块的核心行为：
 * - 方块注册为 MudBlock 类型（非 SimpleBlock）
 * - 碰撞箱高度为 14/16 格（0.875），实体走在上面会略微下沉
 * - allowsMovement 返回 false（不可被路径寻找通过）
 * - 支持形状和视觉形状仍为完整方块（使用默认实现）
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/block/blocks/dirt/MudBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::blocks;

class MudBlockTestWorld final : public mc::test::BaseTestWorld {
public:
    MudBlockTestWorld() = default;
};

// ============================================================================
// 方块注册与类型测试
// ============================================================================

class MudBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(MudBlockTest, MudIsRegistered)
{
    ASSERT_NE(VanillaBlocks::MUD, nullptr);
}

TEST_F(MudBlockTest, MudIsMudBlockType)
{
    // 泥巴应注册为 MudBlock 类型（非 SimpleBlock），因其有自定义碰撞箱
    const Block* mud = VanillaBlocks::MUD;
    ASSERT_NE(mud, nullptr);

    auto* mudBlock = dynamic_cast<const MudBlock*>(mud);
    EXPECT_NE(mudBlock, nullptr) << "MUD should be registered as MudBlock, not SimpleBlock";
}

TEST_F(MudBlockTest, PackedMudAndMudBricksAreSimpleBlocks)
{
    // 泥坯和泥砖是普通方块，应为 SimpleBlock 而非 MudBlock
    const Block* packedMud = VanillaBlocks::PACKED_MUD;
    const Block* mudBricks = VanillaBlocks::MUD_BRICKS;

    ASSERT_NE(packedMud, nullptr);
    ASSERT_NE(mudBricks, nullptr);

    // 这些方块不应是 MudBlock 类型
    auto* packedMudAsMud = dynamic_cast<const MudBlock*>(packedMud);
    auto* mudBricksAsMud = dynamic_cast<const MudBlock*>(mudBricks);

    EXPECT_EQ(packedMudAsMud, nullptr) << "PACKED_MUD should not be MudBlock";
    EXPECT_EQ(mudBricksAsMud, nullptr) << "MUD_BRICKS should not be MudBlock";
}

// ============================================================================
// 碰撞箱测试
// ============================================================================

TEST_F(MudBlockTest, CollisionShapeHeightIs14Over16)
{
    // 泥巴碰撞箱高度应为 14/16 格（0.875），而非完整方块的 1.0
    const Block* mud = VanillaBlocks::MUD;
    ASSERT_NE(mud, nullptr);

    const BlockState& state = mud->defaultState();
    const CollisionShape& shape = mud->getCollisionShape(state);

    ASSERT_EQ(shape.boxCount(), 1u) << "Mud collision shape should be a single box";

    const AxisAlignedBB& box = shape.boxes().front();
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 14.0f / 16.0f) << "Mud collision shape maxY should be 14/16 (0.875)";
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

TEST_F(MudBlockTest, CollisionShapeMaxYIsLessThanFullBlock)
{
    // 碰撞箱高度应小于完整方块（1.0）
    const Block* mud = VanillaBlocks::MUD;
    ASSERT_NE(mud, nullptr);

    const BlockState& state = mud->defaultState();
    const CollisionShape& shape = mud->getCollisionShape(state);
    const AxisAlignedBB& box = shape.boxes().front();

    EXPECT_LT(box.maxY, 1.0f) << "Mud collision shape should be shorter than a full block";
}

// ============================================================================
// allowsMovement 测试
// ============================================================================

TEST_F(MudBlockTest, AllowsMovementReturnsFalse)
{
    // 泥巴不可被路径寻找通过（MC 原版 MudBlock.isPathfindable 返回 false）
    const Block* mud = VanillaBlocks::MUD;
    ASSERT_NE(mud, nullptr);

    MudBlockTestWorld world;
    const BlockState& state = mud->defaultState();
    BlockPos pos(0, 0, 0);

    EXPECT_FALSE(mud->allowsMovement(state, world, pos)) << "Mud should not allow movement/pathfinding through it";
}

// ============================================================================
// 泥巴系列方块注册完整性测试
// ============================================================================

TEST_F(MudBlockTest, AllMudBlocksAreRegistered)
{
    // 验证泥巴系列所有方块均已注册
    ASSERT_NE(VanillaBlocks::MUD, nullptr);
    ASSERT_NE(VanillaBlocks::PACKED_MUD, nullptr);
    ASSERT_NE(VanillaBlocks::MUD_BRICKS, nullptr);
    ASSERT_NE(VanillaBlocks::MUD_BRICK_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::MUD_BRICK_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::MUD_BRICK_WALL, nullptr);
}

TEST_F(MudBlockTest, MudBlockProperties)
{
    // 验证泥巴方块的基本属性
    const Block* mud = VanillaBlocks::MUD;
    ASSERT_NE(mud, nullptr);

    // 验证资源位置
    EXPECT_EQ(mud->blockLocation(), ResourceLocation("minecraft", "mud"));
}

TEST_F(MudBlockTest, PackedMudBlockProperties)
{
    const Block* packedMud = VanillaBlocks::PACKED_MUD;
    ASSERT_NE(packedMud, nullptr);

    EXPECT_EQ(packedMud->blockLocation(), ResourceLocation("minecraft", "packed_mud"));
}

TEST_F(MudBlockTest, MudBricksBlockProperties)
{
    const Block* mudBricks = VanillaBlocks::MUD_BRICKS;
    ASSERT_NE(mudBricks, nullptr);

    EXPECT_EQ(mudBricks->blockLocation(), ResourceLocation("minecraft", "mud_bricks"));
}
