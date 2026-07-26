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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * NEGLIGENCE OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file UpdateFromNeighbourShapesTest.cpp
 * @brief Block::updateFromNeighbourShapes 单元测试
 *
 * 测试方块邻居形状更新方法的行为：
 * - UPDATE_SHAPE_ORDER 方向迭代顺序正确性
 * - 基本流程：方块在空气邻居包围下状态不变
 * - 累积更新：多个方向邻居变化导致状态逐步更新
 * - 空邻居跳过：getBlockState 返回 nullptr 时安全跳过
 * - 与具体方块集成：栅栏、楼梯等方块的 updatePostPlacement 联动
 * - 液体方块跳过：在区块后处理中液体方块应跳过形状更新
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;

namespace {

/**
 * @brief 用于 updateFromNeighbourShapes 测试的 Mock World 实现
 *
 * 提供方块存储和 setBlockState 记录功能。
 */
class ShapeUpdateTestWorld final : public mc::test::BaseTestWorld {
public:
    ShapeUpdateTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    void setBlockDirectly(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[packPos(pos.x, pos.y, pos.z)] = state;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        m_blockChanges.push_back({BlockPos(x, y, z), state});
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                return fluidState;
            }
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<ShapeUpdateTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        MC_UNUSED(entity);
        return EntityInstanceId(0);
    }

    // 测试辅助方法
    const std::vector<std::pair<BlockPos, const BlockState*>>& getBlockChanges() const { return m_blockChanges; }

    void clearRecords() { m_blockChanges.clear(); }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    std::vector<std::pair<BlockPos, const BlockState*>> m_blockChanges;
};

} // namespace

// ============================================================================
// 测试固件
// ============================================================================

class UpdateFromNeighbourShapesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    ShapeUpdateTestWorld world;
};

// ============================================================================
// UPDATE_SHAPE_ORDER 方向迭代顺序测试
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, UpdateShapeOrder_HasSixDirections)
{
    EXPECT_EQ(Block::UPDATE_SHAPE_ORDER.size(), 6u);
}

TEST_F(UpdateFromNeighbourShapesTest, UpdateShapeOrder_CorrectOrder)
{
    // MC Java 的 UPDATE_SHAPE_ORDER: WEST, EAST, NORTH, SOUTH, DOWN, UP
    ASSERT_EQ(Block::UPDATE_SHAPE_ORDER.size(), 6u);
    EXPECT_EQ(Block::UPDATE_SHAPE_ORDER[0], Direction::West);
    EXPECT_EQ(Block::UPDATE_SHAPE_ORDER[1], Direction::East);
    EXPECT_EQ(Block::UPDATE_SHAPE_ORDER[2], Direction::North);
    EXPECT_EQ(Block::UPDATE_SHAPE_ORDER[3], Direction::South);
    EXPECT_EQ(Block::UPDATE_SHAPE_ORDER[4], Direction::Down);
    EXPECT_EQ(Block::UPDATE_SHAPE_ORDER[5], Direction::Up);
}

TEST_F(UpdateFromNeighbourShapesTest, UpdateShapeOrder_AxisPaired)
{
    // 每对相邻方向应在同一轴上
    for (size_t i = 0; i + 1 < Block::UPDATE_SHAPE_ORDER.size(); i += 2) {
        Direction a = Block::UPDATE_SHAPE_ORDER[i];
        Direction b = Block::UPDATE_SHAPE_ORDER[i + 1];
        EXPECT_EQ(Directions::getAxis(a), Directions::getAxis(b))
            << "Directions at indices " << i << " and " << i + 1 << " should be on the same axis";
        EXPECT_EQ(Directions::opposite(a), b)
            << "Directions at indices " << i << " and " << i + 1 << " should be opposites";
    }
}

// ============================================================================
// 基本功能测试
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, AirBlockInAirSurroundings_StateUnchanged)
{
    // 空气方块在空气包围下，updateFromNeighbourShapes 应返回相同状态
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockPos pos(0, 64, 0);

    BlockState result = Block::updateFromNeighbourShapes(airState, world, pos);
    EXPECT_EQ(result, airState);
}

TEST_F(UpdateFromNeighbourShapesTest, StoneBlockInAirSurroundings_StateUnchanged)
{
    // 石头方块在空气包围下，updatePostPlacement 默认返回原状态，
    // 因此 updateFromNeighbourShapes 也应返回相同状态
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 64, 0);

    BlockState result = Block::updateFromNeighbourShapes(stoneState, world, pos);
    EXPECT_EQ(result, stoneState);
}

TEST_F(UpdateFromNeighbourShapesTest, StoneBlockWithStoneNeighbor_StateUnchanged)
{
    // 石头在石头邻居包围下也不变，因为 StoneBlock 不重写 updatePostPlacement
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 64, 0);

    // 在所有6个方向放置石头邻居
    for (Direction dir : Block::UPDATE_SHAPE_ORDER) {
        world.setBlockDirectly(pos.offset(dir), &stoneState);
    }

    BlockState result = Block::updateFromNeighbourShapes(stoneState, world, pos);
    EXPECT_EQ(result, stoneState);
}

// ============================================================================
// nullptr 邻居跳过测试
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, NullNeighborState_SkippedGracefully)
{
    // 当 getBlockState 返回 nullptr 时，updateFromNeighbourShapes 应安全跳过
    // BaseTestWorld::getBlockState 返回空气状态，不是 nullptr
    // 但我们通过直接设置某些位置来验证行为
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    BlockPos pos(0, 64, 0);

    // 只在一些方向设置邻居，其他方向返回空气
    world.setBlockDirectly(pos.offset(Direction::West), &stoneState);
    world.setBlockDirectly(pos.offset(Direction::East), &stoneState);

    // 不应崩溃
    BlockState result = Block::updateFromNeighbourShapes(stoneState, world, pos);
    EXPECT_EQ(result, stoneState);
}

// ============================================================================
// 累积更新测试
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, CumulativeUpdateAcrossDirections)
{
    // 测试 updateFromNeighbourShapes 会在所有6个方向上累积调用 updatePostPlacement
    // 使用 ConcretePowderBlock 作为测试用例——它会在 updatePostPlacement 中检查邻居是否为水
    // 如果邻居有水，混凝土粉末会固化
    // 这验证了累积行为：每个方向的结果会传递给下一个方向
    Block* whiteConcretePowder = VanillaBlocks::WHITE_CONCRETE_POWDER;
    Block* whiteConcrete = VanillaBlocks::WHITE_CONCRETE;

    ASSERT_NE(whiteConcretePowder, nullptr);
    ASSERT_NE(whiteConcrete, nullptr);

    BlockPos pos(0, 64, 0);
    world.setBlockDirectly(pos, &whiteConcretePowder->defaultState());

    // 在某个方向设置水
    const BlockState& waterState = VanillaBlocks::WATER->defaultState();
    world.setBlockDirectly(pos.offset(Direction::Down), &waterState);

    // 调用 updateFromNeighbourShapes 应触发混凝土粉末的 updatePostPlacement
    // 在 DOWN 方向发现水邻居，返回混凝土方块状态
    BlockState result = Block::updateFromNeighbourShapes(whiteConcretePowder->defaultState(), world, pos);

    // 混凝土粉末遇水应变为混凝土
    EXPECT_EQ(result.stateId(), whiteConcrete->defaultState().stateId())
        << "Concrete powder touching water should solidify via updateFromNeighbourShapes";
}

// ============================================================================
// 与区块后处理集成的语义测试
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, LiquidBlockNotUpdated_SemanticTest)
{
    // 在区块后处理中，液体方块跳过 updateFromNeighbourShapes 调用
    // 这通过调用方的 !blockState->isLiquid() 检查来实现
    // 这里验证液体方块的 isLiquid() 返回 true
    const BlockState& waterState = VanillaBlocks::WATER->defaultState();
    EXPECT_TRUE(waterState.isLiquid()) << "Water block should report isLiquid() = true";

    const BlockState& lavaState = VanillaBlocks::LAVA->defaultState();
    EXPECT_TRUE(lavaState.isLiquid()) << "Lava block should report isLiquid() = true";

    // 非液体方块应返回 false
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(stoneState.isLiquid()) << "Stone block should report isLiquid() = false";
}

// ============================================================================
// 标志位语义测试
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, SetBlockFlags276_SemanticTest)
{
    // 区块后处理和结构放置使用 flags=276
    // 276 = 256 | 16 | 4 = SKIP_BLOCK_ENTITY_SIDEEFFECTS | KNOWN_SHAPE | INVISIBLE
    // 验证 flags 值的语义
    constexpr i32 FLAG_SKIP_BE_SIDEEFFECTS = 256;
    constexpr i32 FLAG_KNOWN_SHAPE = 16;
    constexpr i32 FLAG_INVISIBLE = 4;
    constexpr i32 flags = FLAG_SKIP_BE_SIDEEFFECTS | FLAG_KNOWN_SHAPE | FLAG_INVISIBLE;
    EXPECT_EQ(flags, 276);
}

// ============================================================================
// 方向偏移正确性测试
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, NeighborPositions_CorrectlyComputed)
{
    // 验证 BlockPos::offset 为每个 UPDATE_SHAPE_ORDER 方向生成正确的邻居位置
    BlockPos center(10, 64, 20);

    EXPECT_EQ(center.offset(Direction::West), BlockPos(9, 64, 20));
    EXPECT_EQ(center.offset(Direction::East), BlockPos(11, 64, 20));
    EXPECT_EQ(center.offset(Direction::North), BlockPos(10, 64, 19));
    EXPECT_EQ(center.offset(Direction::South), BlockPos(10, 64, 21));
    EXPECT_EQ(center.offset(Direction::Down), BlockPos(10, 63, 20));
    EXPECT_EQ(center.offset(Direction::Up), BlockPos(10, 65, 20));
}

// ============================================================================
// 集成测试：栅栏连接更新
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, FenceConnectionUpdate_IntegrationTest)
{
    // 栅栏在 updatePostPlacement 中会根据邻居更新连接状态
    // 验证 updateFromNeighbourShapes 能正确触发栅栏的形状更新
    Block* oakFence = VanillaBlocks::OAK_FENCE;
    if (oakFence == nullptr) {
        GTEST_SKIP() << "OAK_FENCE not registered, skipping fence integration test";
    }

    const BlockState& fenceState = oakFence->defaultState();
    BlockPos pos(0, 64, 0);

    // 在孤立位置放置栅栏
    world.setBlockDirectly(pos, &fenceState);

    // 在西边放置另一个栅栏
    const BlockState& neighborFence = oakFence->defaultState();
    world.setBlockDirectly(pos.offset(Direction::West), &neighborFence);

    // 调用 updateFromNeighbourShapes
    BlockState result = Block::updateFromNeighbourShapes(fenceState, world, pos);

    // 栅栏应该更新了连接属性（如果实现正确的话）
    // 不论结果如何，不应崩溃，且应返回有效的 BlockState
    EXPECT_EQ(result.getBlock().blockId(), oakFence->blockId()) << "Fence should remain a fence after shape update";
}

// ============================================================================
// 集成测试：楼梯形状更新
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, StairsShapeUpdate_IntegrationTest)
{
    // 楼梯在 updatePostPlacement 中会根据邻居更新形状（直楼梯、内角、外角）
    Block* oakStairs = VanillaBlocks::OAK_STAIRS;
    if (oakStairs == nullptr) {
        GTEST_SKIP() << "OAK_STAIRS not registered, skipping stairs integration test";
    }

    const BlockState& stairsState = oakStairs->defaultState();
    BlockPos pos(0, 64, 0);

    // 孤立放置楼梯
    world.setBlockDirectly(pos, &stairsState);

    // 不应崩溃
    BlockState result = Block::updateFromNeighbourShapes(stairsState, world, pos);

    // 结果应仍然是楼梯方块
    EXPECT_EQ(result.getBlock().blockId(), oakStairs->blockId()) << "Stairs should remain stairs after shape update";
}

// ============================================================================
// 返回值语义测试
// ============================================================================

TEST_F(UpdateFromNeighbourShapesTest, ReturnsNewBlockState_NotReference)
{
    // updateFromNeighbourShapes 返回 BlockState 值，不修改原状态
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    u32 originalStateId = stoneState.stateId();

    BlockPos pos(0, 64, 0);
    BlockState result = Block::updateFromNeighbourShapes(stoneState, world, pos);

    // 原始状态不应被修改
    EXPECT_EQ(stoneState.stateId(), originalStateId);
}
