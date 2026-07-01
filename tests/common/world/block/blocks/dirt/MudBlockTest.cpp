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

TEST_F(MudBlockTest, CollisionShapeFullBoundaryConditions)
{
    // 验证泥巴碰撞箱的完整边界条件
    const Block* mud = VanillaBlocks::MUD;
    ASSERT_NE(mud, nullptr);

    const BlockState& state = mud->defaultState();
    const CollisionShape& shape = mud->getCollisionShape(state);
    const AxisAlignedBB& box = shape.boxes().front();

    // X/Z 方向应为完整方块范围 [0, 1]
    EXPECT_FLOAT_EQ(box.minX, 0.0f) << "Mud collision shape should start at minX=0";
    EXPECT_FLOAT_EQ(box.maxX, 1.0f) << "Mud collision shape should end at maxX=1";
    EXPECT_FLOAT_EQ(box.minZ, 0.0f) << "Mud collision shape should start at minZ=0";
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f) << "Mud collision shape should end at maxZ=1";

    // Y 方向底部对齐方块底部
    EXPECT_FLOAT_EQ(box.minY, 0.0f) << "Mud collision shape should start at minY=0";

    // Y 方向顶部为 14/16（0.875），低于完整方块的 1.0
    EXPECT_FLOAT_EQ(box.maxY, 0.875f) << "Mud collision shape should end at maxY=14/16=0.875";
}

TEST_F(MudBlockTest, CollisionShapeShorterThanDirt)
{
    // 泥巴的碰撞箱应比普通泥土（DIRT）矮
    // MC 原版中泥巴碰撞箱为 column(16, 0, 14)，而普通泥土为完整方块
    const Block* mud = VanillaBlocks::MUD;
    const Block* dirt = VanillaBlocks::DIRT;
    ASSERT_NE(mud, nullptr);
    ASSERT_NE(dirt, nullptr);

    const BlockState& mudState = mud->defaultState();
    const BlockState& dirtState = dirt->defaultState();

    const CollisionShape& mudShape = mud->getCollisionShape(mudState);
    const CollisionShape& dirtShape = dirt->getCollisionShape(dirtState);

    ASSERT_EQ(mudShape.boxCount(), 1u);
    ASSERT_EQ(dirtShape.boxCount(), 1u);

    const AxisAlignedBB& mudBox = mudShape.boxes().front();
    const AxisAlignedBB& dirtBox = dirtShape.boxes().front();

    // 泥巴的 maxY 应小于泥土的 maxY
    EXPECT_LT(mudBox.maxY, dirtBox.maxY) << "Mud collision shape should be shorter than dirt collision shape";

    // 泥土的碰撞箱应为完整方块（maxY = 1.0）
    EXPECT_FLOAT_EQ(dirtBox.maxY, 1.0f) << "Dirt should have full block collision shape";

    // 泥巴的 maxY 应为 0.875 (14/16)
    EXPECT_FLOAT_EQ(mudBox.maxY, 0.875f) << "Mud should have 14/16 height collision shape";
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
    //
    // 注意：此重写不是冗余的。泥巴碰撞箱比完整方块矮（14/16格高），
    // 导致 isCollisionShapeFullBlock() 返回 false。默认的 allowsMovement 实现
    // 对非完整碰撞箱方块返回 true（允许路径寻找通过），因此泥巴必须显式
    // 重写返回 false，以防止实体将泥巴视为可通过的路径。
    // 相比之下，普通泥土（DIRT）有完整碰撞箱，默认实现已返回 false。
    const Block* mud = VanillaBlocks::MUD;
    ASSERT_NE(mud, nullptr);

    MudBlockTestWorld world;
    const BlockState& state = mud->defaultState();
    BlockPos pos(0, 0, 0);

    EXPECT_FALSE(mud->allowsMovement(state, world, pos)) << "Mud should not allow movement/pathfinding through it";
}

TEST_F(MudBlockTest, AllowsMovementSameAsDirtDespiteShorterCollisionShape)
{
    // 泥巴和泥土的 allowsMovement 行为应一致（都返回 false），
    // 尽管泥巴的碰撞箱比泥土矮。
    // MC 原版中两者 isPathfindable 均返回 false，但实现方式不同：
    // - 泥土：碰撞箱为完整方块，默认 isPathfindable 返回 false
    // - 泥巴：碰撞箱不是完整方块，默认会返回 true，需显式重写为 false
    const Block* mud = VanillaBlocks::MUD;
    const Block* dirt = VanillaBlocks::DIRT;
    ASSERT_NE(mud, nullptr);
    ASSERT_NE(dirt, nullptr);

    MudBlockTestWorld world;
    const BlockState& mudState = mud->defaultState();
    const BlockState& dirtState = dirt->defaultState();
    BlockPos pos(0, 0, 0);

    EXPECT_FALSE(mud->allowsMovement(mudState, world, pos));
    EXPECT_FALSE(dirt->allowsMovement(dirtState, world, pos));
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
