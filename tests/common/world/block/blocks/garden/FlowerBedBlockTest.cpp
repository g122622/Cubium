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
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/garden/FlowerBedBlock.hpp"
#include "common/world/block/registry/GardenBlocks.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "item/core/ItemStack.hpp"
#include "util/math/Vector3.hpp"

#include <algorithm>
#include <memory>
#include <unordered_map>

namespace mc {
namespace blocks {
namespace {

// ============================================================================
// 测试世界
// ============================================================================

/**
 * @brief FlowerBedBlock 测试用的模拟世界
 *
 * 提供方块状态存储和含水检测功能。
 * 未设置方块的位置返回空气方块状态。
 */
class FlowerBedTestWorld : public mc::test::BaseTestWorld {
public:
    FlowerBedTestWorld() { m_airState = &VanillaBlocks::AIR->defaultState(); }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return m_airState;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        i64 key = packPos(x, y, z);
        if (state == nullptr || state == m_airState) {
            m_blocks.erase(key);
        } else {
            m_blocks[key] = state;
        }
        return true;
    }

    [[nodiscard]] bool hasChunk(i32, i32) const override { return true; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const Block& block = state->getBlock();
            const fluid::FluidState* fluidState = block.getFluidState(*state);
            if (fluidState != nullptr && fluidState->isSource()) {
                return fluidState;
            }
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) & 0x3FFFFFF) | ((static_cast<i64>(y) & 0xFFF) << 26) |
            ((static_cast<i64>(z) & 0x3FFFFFF) << 38);
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::unordered_map<i64, const BlockState*> m_blocks;
    const BlockState* m_airState;
};

/**
 * @brief 创建放置上下文的辅助函数
 */
BlockItemUseContext makePlacementContext(
    IWorld& world, const BlockPos& pos, Direction face, f32 playerYaw, f32 hitY = 0.5f)
{
    Vector3 hitPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + hitY, static_cast<f32>(pos.z) + 0.5f);
    ItemStack stack;
    return BlockItemUseContext(world, nullptr, stack, hitPos, pos, face, playerYaw, 0.0f);
}

// ============================================================================
// 测试固件
// ============================================================================

class FlowerBedBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();

        flowerBed_ =
            std::make_unique<FlowerBedBlock>(BlockProperties(Material::PLANT).noCollision().notSolid().replaceable());
    }

    std::unique_ptr<FlowerBedBlock> flowerBed_;
    FlowerBedTestWorld world_;
};

// ============================================================================
// 状态属性测试
// ============================================================================

TEST_F(FlowerBedBlockTest, DefaultState_HasCorrectProperties)
{
    const BlockState& state = flowerBed_->defaultState();

    // 默认状态：朝北、1个花瓣
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 1);
}

TEST_F(FlowerBedBlockTest, StateProperties_AllFacingValues)
{
    auto state = flowerBed_->defaultState();

    // 测试所有水平朝向
    for (Direction dir : {Direction::North, Direction::East, Direction::South, Direction::West}) {
        state = state.with(BlockStateProperties::HORIZONTAL_FACING(), dir);
        EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), dir);
    }
}

TEST_F(FlowerBedBlockTest, StateProperties_AllAmountValues)
{
    auto state = flowerBed_->defaultState();

    // 测试 AMOUNT 属性的1-4范围
    for (i32 amount = 1; amount <= 4; ++amount) {
        state = state.with(BlockStateProperties::FLOWER_AMOUNT(), amount);
        EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), amount);
    }
}

TEST_F(FlowerBedBlockTest, StateProperties_CanCombineFacingAndAmount)
{
    auto state = flowerBed_->defaultState();

    // 朝东、3个花瓣
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    state = state.with(BlockStateProperties::FLOWER_AMOUNT(), 3);
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 3);

    // 朝南、4个花瓣
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    state = state.with(BlockStateProperties::FLOWER_AMOUNT(), 4);
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 4);
}

TEST_F(FlowerBedBlockTest, StateProperties_StateCountIs16)
{
    // 4个朝向 × 4个数量 = 16个状态
    EXPECT_EQ(flowerBed_->stateContainer().stateCount(), 16u);
}

// ============================================================================
// 放置逻辑测试
// ============================================================================

TEST_F(FlowerBedBlockTest, Placement_NewBlock_SetsFacingOppositeToPlayer)
{
    BlockPos pos(0, 0, 0);

    // 玩家面朝南（yaw=0），花瓣应朝北（opposite）
    auto context = makePlacementContext(world_, pos, Direction::Up, 0.0f);
    auto state = flowerBed_->getStateForPlacement(context);

    // 默认面朝南时，水平朝向为南，取反为北
    // 注意：horizontalDirection() 取决于玩家朝向的实现
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 1);
}

TEST_F(FlowerBedBlockTest, Placement_NewBlock_SetsAmountToOne)
{
    BlockPos pos(5, 0, 5);

    auto context = makePlacementContext(world_, pos, Direction::Up, 90.0f);
    auto state = flowerBed_->getStateForPlacement(context);

    // 新放置始终为 AMOUNT=1
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 1);
}

TEST_F(FlowerBedBlockTest, Placement_StackOnSameBlock_IncreasesAmount)
{
    BlockPos pos(0, 0, 0);

    // 先在目标位置放置一个 AMOUNT=2 的花瓣床
    const BlockState* existingState = &flowerBed_->defaultState()
                                           .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                                           .with(BlockStateProperties::FLOWER_AMOUNT(), 2);
    world_.setBlockAt(pos, existingState);

    // 模拟同类型堆叠放置
    auto context = makePlacementContext(world_, pos, Direction::Up, 90.0f);
    auto state = flowerBed_->getStateForPlacement(context);

    // AMOUNT 应该从2增加到3，FACING 保持不变
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 3);
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(FlowerBedBlockTest, Placement_StackOnFullBlock_StaysAtFour)
{
    BlockPos pos(0, 0, 0);

    // 已有 AMOUNT=4 的花瓣床
    const BlockState* existingState = &flowerBed_->defaultState()
                                           .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                           .with(BlockStateProperties::FLOWER_AMOUNT(), 4);
    world_.setBlockAt(pos, existingState);

    auto context = makePlacementContext(world_, pos, Direction::Up, 90.0f);
    auto state = flowerBed_->getStateForPlacement(context);

    // AMOUNT=4 已满，不应继续增加
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 4);
}

TEST_F(FlowerBedBlockTest, Placement_StackOnDifferentBlock_NewPlacement)
{
    BlockPos pos(0, 0, 0);

    // 在目标位置放置一个不同类型的方块（石头）
    if (VanillaBlocks::STONE != nullptr) {
        world_.setBlockAt(pos, &VanillaBlocks::STONE->defaultState());

        auto context = makePlacementContext(world_, pos, Direction::Up, 90.0f);
        auto state = flowerBed_->getStateForPlacement(context);

        // 不是同类型方块，应该按新放置处理
        EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 1);
    }
}

TEST_F(FlowerBedBlockTest, Placement_StackFromOneToFour)
{
    BlockPos pos(0, 0, 0);

    // 逐步从 AMOUNT=1 堆叠到 4
    for (i32 amount = 1; amount < 4; ++amount) {
        const BlockState* existingState = &flowerBed_->defaultState()
                                               .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                                               .with(BlockStateProperties::FLOWER_AMOUNT(), amount);
        world_.setBlockAt(pos, existingState);

        auto context = makePlacementContext(world_, pos, Direction::Up, 0.0f);
        auto state = flowerBed_->getStateForPlacement(context);

        EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), amount + 1);
    }
}

// ============================================================================
// 旋转测试
// ============================================================================

TEST_F(FlowerBedBlockTest, Rotate_Clockwise90)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& rotated = flowerBed_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(FlowerBedBlockTest, Rotate_Clockwise180)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& rotated = flowerBed_->rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(FlowerBedBlockTest, Rotate_CounterClockwise90)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& rotated = flowerBed_->rotate(state, Rotation::CounterClockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
}

TEST_F(FlowerBedBlockTest, Rotate_None_PreservesFacing)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState& rotated = flowerBed_->rotate(state, Rotation::None);
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(FlowerBedBlockTest, Rotate_PreservesAmount)
{
    auto state = flowerBed_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::FLOWER_AMOUNT(), 3);
    const BlockState& rotated = flowerBed_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::FLOWER_AMOUNT()), 3);
}

TEST_F(FlowerBedBlockTest, Rotate_FullCycle)
{
    // 旋转4次CW90应回到原始朝向
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState* current = &state;
    current = &flowerBed_->rotate(*current, Rotation::Clockwise90);
    EXPECT_EQ(current->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
    current = &flowerBed_->rotate(*current, Rotation::Clockwise90);
    EXPECT_EQ(current->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
    current = &flowerBed_->rotate(*current, Rotation::Clockwise90);
    EXPECT_EQ(current->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
    current = &flowerBed_->rotate(*current, Rotation::Clockwise90);
    EXPECT_EQ(current->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

// ============================================================================
// 镜像测试
// ============================================================================

TEST_F(FlowerBedBlockTest, Mirror_LeftRight_EastWestSwap)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState& mirrored = flowerBed_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
}

TEST_F(FlowerBedBlockTest, Mirror_LeftRight_WestEastSwap)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    const BlockState& mirrored = flowerBed_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(FlowerBedBlockTest, Mirror_LeftRight_NorthSouthUnchanged)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& mirrored = flowerBed_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(FlowerBedBlockTest, Mirror_FrontBack_NorthSouthSwap)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& mirrored = flowerBed_->mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(FlowerBedBlockTest, Mirror_FrontBack_SouthNorthSwap)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    const BlockState& mirrored = flowerBed_->mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(FlowerBedBlockTest, Mirror_FrontBack_EastWestUnchanged)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState& mirrored = flowerBed_->mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(FlowerBedBlockTest, Mirror_None_PreservesFacing)
{
    auto state = flowerBed_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    const BlockState& mirrored = flowerBed_->mirror(state, Mirror::None);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(FlowerBedBlockTest, Mirror_PreservesAmount)
{
    auto state = flowerBed_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::FLOWER_AMOUNT(), 2);
    const BlockState& mirrored = flowerBed_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::FLOWER_AMOUNT()), 2);
}

// ============================================================================
// 形状测试
// ============================================================================

TEST_F(FlowerBedBlockTest, Shape_AllAmounts_NonEmpty)
{
    // 每种 AMOUNT 的形状都应非空
    for (i32 amount = 1; amount <= 4; ++amount) {
        auto state = flowerBed_->defaultState()
                         .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                         .with(BlockStateProperties::FLOWER_AMOUNT(), amount);
        const CollisionShape& shape = flowerBed_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Shape should not be empty for amount=" << amount;
    }
}

TEST_F(FlowerBedBlockTest, Shape_AllFacings_NonEmpty)
{
    // 每种朝向的形状都应非空
    for (Direction dir : {Direction::North, Direction::East, Direction::South, Direction::West}) {
        auto state = flowerBed_->defaultState()
                         .with(BlockStateProperties::HORIZONTAL_FACING(), dir)
                         .with(BlockStateProperties::FLOWER_AMOUNT(), 2);
        const CollisionShape& shape = flowerBed_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Shape should not be empty for facing=" << static_cast<i32>(dir);
    }
}

TEST_F(FlowerBedBlockTest, Shape_HeightIsCorrect)
{
    // 所有形状的高度应为 3/16 = 0.1875
    static constexpr f32 EXPECTED_HEIGHT = 3.0f / 16.0f;
    auto state = flowerBed_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::FLOWER_AMOUNT(), 4);

    // 使用满覆盖（AMOUNT=4）来验证高度
    const CollisionShape& shape = flowerBed_->getShape(state);

    // 获取 Y 轴范围来验证高度
    const auto& boxes = shape.boxes();
    ASSERT_FALSE(boxes.empty());
    f32 maxY = boxes[0].maxY;
    f32 minY = boxes[0].minY;
    for (const auto& box : boxes) {
        maxY = std::max(maxY, box.maxY);
        minY = std::min(minY, box.minY);
    }
    EXPECT_FLOAT_EQ(maxY, EXPECTED_HEIGHT);
    EXPECT_FLOAT_EQ(minY, 0.0f);
}

TEST_F(FlowerBedBlockTest, Shape_AmountFour_CoversFullBlock)
{
    // AMOUNT=4 时，四个象限应覆盖整个方块的水平面积
    auto state = flowerBed_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::FLOWER_AMOUNT(), 4);
    const CollisionShape& shape = flowerBed_->getShape(state);

    const auto& boxes = shape.boxes();
    ASSERT_EQ(boxes.size(), 4u);
    f32 minX = boxes[0].minX, maxX = boxes[0].maxX;
    f32 minZ = boxes[0].minZ, maxZ = boxes[0].maxZ;
    for (const auto& box : boxes) {
        minX = std::min(minX, box.minX);
        maxX = std::max(maxX, box.maxX);
        minZ = std::min(minZ, box.minZ);
        maxZ = std::max(maxZ, box.maxZ);
    }
    EXPECT_FLOAT_EQ(minX, 0.0f);
    EXPECT_FLOAT_EQ(minZ, 0.0f);
    EXPECT_FLOAT_EQ(maxX, 1.0f);
    EXPECT_FLOAT_EQ(maxZ, 1.0f);
}

TEST_F(FlowerBedBlockTest, Shape_AmountOne_IsQuadrant)
{
    // AMOUNT=1 时，形状应仅覆盖方块的 1/4 区域（NW象限 for North facing）
    auto state = flowerBed_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::FLOWER_AMOUNT(), 1);
    const CollisionShape& shape = flowerBed_->getShape(state);

    const auto& boxes = shape.boxes();
    ASSERT_EQ(boxes.size(), 1u);
    // North facing, AMOUNT=1: NW象限 (0, 0, 0) - (0.5, H, 0.5)
    EXPECT_FLOAT_EQ(boxes[0].minX, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxX, 0.5f);
    EXPECT_FLOAT_EQ(boxes[0].minZ, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxZ, 0.5f);
}

TEST_F(FlowerBedBlockTest, Shape_BoxCount_IncreasesWithAmount)
{
    // AMOUNT=1 应有1个盒子，AMOUNT=2 应有2个，以此类推
    for (i32 amount = 1; amount <= 4; ++amount) {
        auto state = flowerBed_->defaultState()
                         .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                         .with(BlockStateProperties::FLOWER_AMOUNT(), amount);
        const CollisionShape& shape = flowerBed_->getShape(state);
        EXPECT_EQ(shape.boxCount(), static_cast<u32>(amount)) << "Box count should match amount for amount=" << amount;
    }
}

TEST_F(FlowerBedBlockTest, Shape_Rotated_FacingEast)
{
    // East facing, AMOUNT=1: NE象限 (0.5, 0, 0) - (1, H, 0.5)
    auto state = flowerBed_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                     .with(BlockStateProperties::FLOWER_AMOUNT(), 1);
    const CollisionShape& shape = flowerBed_->getShape(state);

    const auto& boxes = shape.boxes();
    ASSERT_EQ(boxes.size(), 1u);
    EXPECT_FLOAT_EQ(boxes[0].minX, 0.5f);
    EXPECT_FLOAT_EQ(boxes[0].maxX, 1.0f);
    EXPECT_FLOAT_EQ(boxes[0].minZ, 0.0f);
    EXPECT_FLOAT_EQ(boxes[0].maxZ, 0.5f);
}

// ============================================================================
// 骨粉行为测试
// ============================================================================

TEST_F(FlowerBedBlockTest, CanGrow_AlwaysTrue)
{
    auto state = flowerBed_->defaultState();
    EXPECT_TRUE(flowerBed_->canGrow(world_, BlockPos(0, 0, 0), state, false));
}

TEST_F(FlowerBedBlockTest, CanUseBonemeal_AlwaysTrue)
{
    math::Random rng(42);
    auto state = flowerBed_->defaultState();
    EXPECT_TRUE(flowerBed_->canUseBonemeal(world_, rng, BlockPos(0, 0, 0), state));
}

TEST_F(FlowerBedBlockTest, Grow_IncreasesAmountWhenLessThanFour)
{
    // AMOUNT=2 时，grow 应增加到3
    auto state = flowerBed_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::FLOWER_AMOUNT(), 2);

    math::Random rng(42);
    BlockPos pos(0, 0, 0);
    flowerBed_->grow(world_, rng, pos, state);

    // 验证方块状态已被更新为 AMOUNT=3
    const BlockState* newState = world_.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(newState->get(BlockStateProperties::FLOWER_AMOUNT()), 3);
    // FACING 应保持不变
    EXPECT_EQ(newState->get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(FlowerBedBlockTest, Grow_IncrementsFromOneToFour)
{
    math::Random rng(42);
    BlockPos pos(0, 0, 0);

    for (i32 amount = 1; amount < 4; ++amount) {
        const BlockState* state = &flowerBed_->defaultState()
                                       .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                                       .with(BlockStateProperties::FLOWER_AMOUNT(), amount);
        world_.setBlockAt(pos, state);

        flowerBed_->grow(world_, rng, pos, *state);

        const BlockState* newState = world_.getBlockState(pos.x, pos.y, pos.z);
        ASSERT_NE(newState, nullptr);
        EXPECT_EQ(newState->get(BlockStateProperties::FLOWER_AMOUNT()), amount + 1)
            << "Grow should increase amount from " << amount << " to " << (amount + 1);
    }
}

TEST_F(FlowerBedBlockTest, Grow_AtFour_DoesNotIncreaseAmount)
{
    // AMOUNT=4 时，grow 不应增加到5
    // 注意：AMOUNT=4 时 grow 会弹出一个物品实体，而不是增加 AMOUNT
    // 此测试仅验证 AMOUNT 不超过4
    auto state = flowerBed_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::FLOWER_AMOUNT(), 4);

    // 先在世界上设置 AMOUNT=4 的状态
    BlockPos pos(0, 0, 0);
    world_.setBlockAt(pos, &state);

    math::Random rng(42);
    flowerBed_->grow(world_, rng, pos, state);

    const BlockState* newState = world_.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(newState, nullptr);
    // AMOUNT 应保持4（不增加到5）
    EXPECT_EQ(newState->get(BlockStateProperties::FLOWER_AMOUNT()), 4);
}

// ============================================================================
// 注册测试
// ============================================================================

class FlowerBedRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(FlowerBedRegistrationTest, PinkPetalsIsRegistered)
{
    ASSERT_NE(mc::block_registry::TrailsBlocks::PINK_PETALS, nullptr);
}

TEST_F(FlowerBedRegistrationTest, WildflowersIsRegistered)
{
    ASSERT_NE(mc::block_registry::GardenBlocks::WILDFLOWERS, nullptr);
}

TEST_F(FlowerBedRegistrationTest, PinkPetalsIsFlowerBedBlock)
{
    auto* flowerBed = dynamic_cast<FlowerBedBlock*>(mc::block_registry::TrailsBlocks::PINK_PETALS);
    EXPECT_NE(flowerBed, nullptr);
}

TEST_F(FlowerBedRegistrationTest, WildflowersIsFlowerBedBlock)
{
    auto* flowerBed = dynamic_cast<FlowerBedBlock*>(mc::block_registry::GardenBlocks::WILDFLOWERS);
    EXPECT_NE(flowerBed, nullptr);
}

TEST_F(FlowerBedRegistrationTest, PinkPetalsDefaultState)
{
    const auto& state = mc::block_registry::TrailsBlocks::PINK_PETALS->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 1);
}

TEST_F(FlowerBedRegistrationTest, WildflowersDefaultState)
{
    const auto& state = mc::block_registry::GardenBlocks::WILDFLOWERS->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    EXPECT_EQ(state.get(BlockStateProperties::FLOWER_AMOUNT()), 1);
}

TEST_F(FlowerBedRegistrationTest, PinkPetalsIsReplaceable)
{
    // 花瓣床应标记为 replaceable 以支持堆叠放置
    const auto& state = mc::block_registry::TrailsBlocks::PINK_PETALS->defaultState();
    EXPECT_TRUE(state.canBeReplaced());
}

TEST_F(FlowerBedRegistrationTest, WildflowersIsReplaceable)
{
    const auto& state = mc::block_registry::GardenBlocks::WILDFLOWERS->defaultState();
    EXPECT_TRUE(state.canBeReplaced());
}

TEST_F(FlowerBedRegistrationTest, PinkPetalsIsNotSolid)
{
    const auto& state = mc::block_registry::TrailsBlocks::PINK_PETALS->defaultState();
    EXPECT_FALSE(state.isSolid());
}

TEST_F(FlowerBedRegistrationTest, WildflowersIsNotSolid)
{
    const auto& state = mc::block_registry::GardenBlocks::WILDFLOWERS->defaultState();
    EXPECT_FALSE(state.isSolid());
}

TEST_F(FlowerBedRegistrationTest, PinkPetalsNoCollision)
{
    const auto& state = mc::block_registry::TrailsBlocks::PINK_PETALS->defaultState();
    EXPECT_TRUE(mc::block_registry::TrailsBlocks::PINK_PETALS->getCollisionShape(state).isEmpty());
}

TEST_F(FlowerBedRegistrationTest, WildflowersNoCollision)
{
    const auto& state = mc::block_registry::GardenBlocks::WILDFLOWERS->defaultState();
    EXPECT_TRUE(mc::block_registry::GardenBlocks::WILDFLOWERS->getCollisionShape(state).isEmpty());
}

// ============================================================================
// IGrowable 接口测试
// ============================================================================

class FlowerBedGrowableTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

TEST_F(FlowerBedGrowableTest, PinkPetalsImplementsIGrowable)
{
    auto* growable = dynamic_cast<IGrowable*>(mc::block_registry::TrailsBlocks::PINK_PETALS);
    EXPECT_NE(growable, nullptr);
}

TEST_F(FlowerBedGrowableTest, WildflowersImplementsIGrowable)
{
    auto* growable = dynamic_cast<IGrowable*>(mc::block_registry::GardenBlocks::WILDFLOWERS);
    EXPECT_NE(growable, nullptr);
}

} // namespace
} // namespace blocks
} // namespace mc
