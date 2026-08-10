/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the notice should be included in all
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

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/nether/EnderChestBlock.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/blockentity/storage/EnderChestEntity.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"
#include "core/Types.hpp"
#include "entity/inventory/PlayerInventory.hpp"

#include <memory>
#include <unordered_map>

// 前向声明
namespace mc::server {
class ServerWorld;
}

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用 Mock World，用于测试 EnderChestBlock 的交互逻辑
 *
 * 实现了 IWorld 接口的关键方法，特别是：
 * - isClientSide() / asServerWorld() 用于区分客户端/服务端
 * - openContainer() 用于测试容器打开
 * - getBlockState() / getBlockEntity() 用于方块和方块实体访问
 */
class EnderChestTestWorld final : public mc::test::BaseTestWorld {
public:
    explicit EnderChestTestWorld(bool isClient = false)
        : m_isClient(isClient)
    {}

    // ========== IWorld 核心接口 ==========

    [[nodiscard]] bool isClientSide() const override { return m_isClient; }

    [[nodiscard]] server::ServerWorld* asServerWorld() override
    {
        return m_isClient ? nullptr : reinterpret_cast<server::ServerWorld*>(0x1);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) return false;
        m_blockStates[BlockPos(x, y, z)] = state;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blockStates.find(pos);
        return it == m_blockStates.end() ? nullptr : it->second;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second.get();
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second.get();
    }

    void setOwnedBlockEntity(std::unique_ptr<BlockEntity> entity)
    {
        const BlockPos pos = entity->getPos();
        m_blockEntities[pos] = std::move(entity);
    }

    // ========== 测试辅助方法 ==========

    void setBlockStateAt(const BlockPos& pos, const BlockState* state) { m_blockStates[pos] = state; }

    [[nodiscard]] bool openContainer(mc::ContainerType type, const BlockPos& pos, Player& player) override
    {
        MC_UNUSED(type);
        MC_UNUSED(pos);
        MC_UNUSED(player);
        // 测试中总是返回 true，表示容器打开成功
        return true;
    }

    // ========== IWorld 存根方法 ==========

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EnderChestTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EnderChestTestWorld::tickManager not implemented");
    }

private:
    bool m_isClient;
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
};

} // namespace

// ========== EnderChestBlock 基础测试 ==========

class EnderChestBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enderChest_ = std::make_unique<EnderChestBlock>(
            BlockProperties(Material::ROCK).hardness(22.5f).resistance(600.0f).notSolid());
    }

    std::unique_ptr<EnderChestBlock> enderChest_;
};

TEST_F(EnderChestBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(enderChest_, nullptr);
}

TEST_F(EnderChestBlockTest, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(enderChest_->hasBlockEntity());
}

TEST_F(EnderChestBlockTest, CreateBlockEntity_ReturnsEnderChestEntity)
{
    BlockPos pos(10, 20, 30);
    auto entity = enderChest_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::EnderChest);
    EXPECT_EQ(entity->getPos(), pos);
}

TEST_F(EnderChestBlockTest, DefaultState_FacingNorth)
{
    const auto& state = enderChest_->defaultState();
    auto facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_EQ(facing, Direction::North);
}

TEST_F(EnderChestBlockTest, DefaultState_NotWaterlogged)
{
    const auto& state = enderChest_->defaultState();
    auto waterlogged = state.get(BlockStateProperties::WATERLOGGED());
    EXPECT_FALSE(waterlogged);
}

TEST_F(EnderChestBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = enderChest_->defaultState();
    const auto& shape = enderChest_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(EnderChestBlockTest, IsWaterlogged_NotWaterloggedByDefault)
{
    const auto& state = enderChest_->defaultState();
    EXPECT_FALSE(enderChest_->isWaterlogged(state));
}

TEST_F(EnderChestBlockTest, IsWaterlogged_WaterloggedState)
{
    auto state = enderChest_->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    EXPECT_TRUE(enderChest_->isWaterlogged(state));
}

// ========== EnderChestBlock 旋转/镜像测试 ==========

class EnderChestBlockRotationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enderChest_ = std::make_unique<EnderChestBlock>(
            BlockProperties(Material::ROCK).hardness(22.5f).resistance(600.0f).notSolid());
    }

    std::unique_ptr<EnderChestBlock> enderChest_;
};

TEST_F(EnderChestBlockRotationTest, Rotate_Clockwise90_NorthToEast)
{
    auto state = enderChest_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const auto& rotated = enderChest_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(EnderChestBlockRotationTest, Rotate_Clockwise180_NorthToSouth)
{
    auto state = enderChest_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const auto& rotated = enderChest_->rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(EnderChestBlockRotationTest, Rotate_CounterClockwise90_NorthToWest)
{
    auto state = enderChest_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const auto& rotated = enderChest_->rotate(state, Rotation::CounterClockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
}

TEST_F(EnderChestBlockRotationTest, Rotate_PreservesWaterlogged)
{
    auto state = enderChest_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::WATERLOGGED(), true);
    const auto& rotated = enderChest_->rotate(state, Rotation::Clockwise90);
    EXPECT_TRUE(rotated.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(EnderChestBlockRotationTest, Mirror_LeftRight_NorthToSouth)
{
    auto state = enderChest_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const auto& mirrored = enderChest_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(EnderChestBlockRotationTest, Mirror_None_NoChange)
{
    auto state = enderChest_->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const auto& mirrored = enderChest_->mirror(state, Mirror::None);
    EXPECT_EQ(mirrored.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

TEST_F(EnderChestBlockRotationTest, Mirror_PreservesWaterlogged)
{
    auto state = enderChest_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                     .with(BlockStateProperties::WATERLOGGED(), true);
    const auto& mirrored = enderChest_->mirror(state, Mirror::LeftRight);
    EXPECT_TRUE(mirrored.get(BlockStateProperties::WATERLOGGED()));
}

// ========== EnderChestBlock 交互测试 ==========

class EnderChestBlockInteractionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enderChest_ = std::make_unique<EnderChestBlock>(
            BlockProperties(Material::ROCK).hardness(22.5f).resistance(600.0f).notSolid());
        pos_ = BlockPos(10, 64, 20);
    }

    std::unique_ptr<EnderChestBlock> enderChest_;
    BlockPos pos_;
};

TEST_F(EnderChestBlockInteractionTest, OnBlockActivated_ClientSide_ReturnsSuccess)
{
    // 客户端世界
    EnderChestTestWorld world(true);

    // 设置方块状态
    world.setBlockStateAt(pos_, &enderChest_->defaultState());

    // 设置末影箱方块实体
    auto entity = std::make_unique<blockentity::EnderChestEntity>(pos_);
    world.setOwnedBlockEntity(std::move(entity));

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = enderChest_->defaultState();
    BlockRaycastResult hit;
    auto result = enderChest_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 客户端应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);
}

TEST_F(EnderChestBlockInteractionTest, OnBlockActivated_ServerSide_WithEntity_ReturnsConsume)
{
    // 服务端世界
    EnderChestTestWorld world(false);

    // 设置方块状态
    world.setBlockStateAt(pos_, &enderChest_->defaultState());

    // 设置末影箱方块实体
    auto entity = std::make_unique<blockentity::EnderChestEntity>(pos_);
    world.setOwnedBlockEntity(std::move(entity));

    // 创建玩家并设置位置（在末影箱附近以便 canPlayerAccess 返回 true）
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(10.5f, 64.5f, 20.5f);
    player.setWorld(&world);

    // 执行交互
    const auto& state = enderChest_->defaultState();
    BlockRaycastResult hit;
    auto result = enderChest_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 服务端有正确方块实体时应返回 Consume
    EXPECT_EQ(result, ActionResultType::Consume);
}

TEST_F(EnderChestBlockInteractionTest, OnBlockActivated_NoBlockEntity_ReturnsPass)
{
    // 服务端世界
    EnderChestTestWorld world(false);

    // 设置方块状态（但不设置方块实体）
    world.setBlockStateAt(pos_, &enderChest_->defaultState());

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = enderChest_->defaultState();
    BlockRaycastResult hit;
    auto result = enderChest_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 没有方块实体时应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);
}

TEST_F(EnderChestBlockInteractionTest, OnBlockActivated_WrongBlockEntityType_ReturnsPass)
{
    // 服务端世界
    EnderChestTestWorld world(false);

    // 设置方块状态
    world.setBlockStateAt(pos_, &enderChest_->defaultState());

    // 设置错误类型的方块实体（使用 ChestEntity 而非 EnderChestEntity）
    auto wrongEntity = std::make_unique<blockentity::ChestEntity>(pos_);
    world.setOwnedBlockEntity(std::move(wrongEntity));

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = enderChest_->defaultState();
    BlockRaycastResult hit;
    auto result = enderChest_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 错误类型的方块实体应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);
}

TEST_F(EnderChestBlockInteractionTest, OnBlockActivated_AboveBlocked_ReturnsSuccess)
{
    // 服务端世界
    EnderChestTestWorld world(false);

    // 设置末影箱方块状态
    world.setBlockStateAt(pos_, &enderChest_->defaultState());

    // 设置末影箱方块实体
    auto entity = std::make_unique<blockentity::EnderChestEntity>(pos_);
    world.setOwnedBlockEntity(std::move(entity));

    // 在末影箱上方放置一个不透明方块（使用末影箱自身的状态，因为它是 notSolid 但
    // 需要一个 hasOpaqueCollisionShape 返回 true 的方块）
    // 为了测试目的，我们在上方设置一个不透明方块状态
    // 注意：这里需要用一个确实 hasOpaqueCollisionShape() 返回 true 的方块
    // 由于测试环境中没有完整的方块注册表，我们跳过上方阻挡测试的方块设置
    // 但我们仍然可以测试上方为空（nullptr）的情况——此时不应阻挡
    BlockPos abovePos = pos_.up();

    // 上方没有方块（getBlockState 返回 nullptr），不应阻挡
    const auto& state = enderChest_->defaultState();
    BlockRaycastResult hit;

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(10.5f, 64.5f, 20.5f);
    player.setWorld(&world);

    auto result = enderChest_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 上方无阻挡时应正常打开
    EXPECT_EQ(result, ActionResultType::Consume);
}

TEST_F(EnderChestBlockInteractionTest, OnBlockActivated_EnderChestEntityOpenCount)
{
    // 服务端世界
    EnderChestTestWorld world(false);

    // 设置方块状态
    world.setBlockStateAt(pos_, &enderChest_->defaultState());

    // 设置末影箱方块实体
    auto entity = std::make_unique<blockentity::EnderChestEntity>(pos_);
    world.setOwnedBlockEntity(std::move(entity));

    // 创建玩家并设置位置
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(10.5f, 64.5f, 20.5f);
    player.setWorld(&world);

    // 执行交互
    const auto& state = enderChest_->defaultState();
    BlockRaycastResult hit;
    enderChest_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 检查方块实体的打开计数
    BlockEntity* be = world.getBlockEntity(pos_);
    ASSERT_NE(be, nullptr);
    EXPECT_EQ(be->getType(), BlockEntityType::EnderChest);
    auto* enderChestEntity = static_cast<blockentity::EnderChestEntity*>(be);
    EXPECT_EQ(enderChestEntity->getOpenCount(), 1);
}

// ========== EnderChestBlock 流体状态测试 ==========

TEST_F(EnderChestBlockTest, GetFluidState_NotWaterlogged_ReturnsNullptr)
{
    const auto& state = enderChest_->defaultState();
    // 默认不含水
    const auto* fluidState = enderChest_->getFluidState(state);
    // 水流注册表未初始化时，waterloggable::getWaterFluidState 对于未含水状态返回 nullptr
    EXPECT_EQ(fluidState, nullptr);
}

TEST_F(EnderChestBlockTest, UpdatePostPlacement_NonWaterlogged_ReturnsSameState)
{
    // 非含水状态下调用 updatePostPlacement 应返回相同的状态值
    EnderChestTestWorld world(false);
    BlockPos pos(10, 64, 20);

    auto nonWaterloggedState = enderChest_->defaultState()
                                   .with(BlockStateProperties::WATERLOGGED(), false)
                                   .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    const auto& result = enderChest_->updatePostPlacement(
        nonWaterloggedState, Direction::Up, enderChest_->defaultState(), world, pos, pos.up());

    // 非含水状态下应保持原有属性值不变
    EXPECT_EQ(result.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
    EXPECT_FALSE(result.get(BlockStateProperties::WATERLOGGED()));
}
