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

#include "common/TestWorldHelper.hpp"
#include "core/Constants.hpp"
#include "core/Types.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/ContainerTypes.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/IWorld.hpp"
#include "world/IWorldWriter.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/EnchantingTableBlock.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/interactive/EnchantingTableEntity.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"

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
 * @brief 测试用 Mock World，用于测试 EnchantingTableBlock 的交互逻辑
 *
 * 实现了 IWorld 接口的关键方法，特别是：
 * - isClientSide() / asServerWorld() 用于区分客户端/服务端
 * - openContainer() 用于测试容器打开
 * - getBlockState() / getBlockEntity() 用于方块和方块实体访问
 */
class EnchantingTableTestWorld final : public mc::test::BaseTestWorld {
public:
    explicit EnchantingTableTestWorld(bool isClient = false)
        : m_isClient(isClient)
        , m_openContainerCalled(false)
        , m_lastContainerType(ContainerType::Player)
        , m_lastContainerPos(0, 0, 0)
        , m_lastContainerPlayer(nullptr)
    {}

    // ========== IWorld 核心接口 ==========

    [[nodiscard]] bool isClientSide() const override { return m_isClient; }

    [[nodiscard]] server::ServerWorld* asServerWorld() override
    {
        return m_isClient ? nullptr : reinterpret_cast<server::ServerWorld*>(0x1); // 非 null 表示服务端
    }

    // IWorldWriter 接口
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

    bool openContainer(ContainerType type, const BlockPos& pos, Player& player) override
    {
        m_openContainerCalled = true;
        m_lastContainerType = type;
        m_lastContainerPos = pos;
        m_lastContainerPlayer = &player;
        return !m_isClient; // 客户端返回 false，服务端返回 true
    }

    // ========== 测试验证方法 ==========

    [[nodiscard]] bool wasOpenContainerCalled() const { return m_openContainerCalled; }
    [[nodiscard]] ContainerType getLastContainerType() const { return m_lastContainerType; }
    [[nodiscard]] BlockPos getLastContainerPos() const { return m_lastContainerPos; }
    [[nodiscard]] Player* getLastContainerPlayer() const { return m_lastContainerPlayer; }

    void setBlockStateAt(const BlockPos& pos, const BlockState* state) { m_blockStates[pos] = state; }

    // ========== IWorld 存根方法 ==========

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EnchantingTableTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EnchantingTableTestWorld::tickManager not implemented");
    }

private:
    bool m_isClient;
    bool m_openContainerCalled;
    ContainerType m_lastContainerType;
    BlockPos m_lastContainerPos;
    Player* m_lastContainerPlayer;
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
};

} // namespace

// ========== EnchantingTableBlock 基础测试 ==========

class EnchantingTableBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enchantingTable_ = std::make_unique<EnchantingTableBlock>(
            BlockProperties(Material::ROCK).hardness(5.0f).resistance(1200.0f).notSolid());
    }

    std::unique_ptr<EnchantingTableBlock> enchantingTable_;
};

TEST_F(EnchantingTableBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(enchantingTable_, nullptr);
}

TEST_F(EnchantingTableBlockTest, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(enchantingTable_->hasBlockEntity());
}

TEST_F(EnchantingTableBlockTest, GetBlockEntityType_ReturnsCorrectType)
{
    EXPECT_EQ(enchantingTable_->getBlockEntityType(), BlockEntityType::EnchantingTable);
}

TEST_F(EnchantingTableBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = enchantingTable_->defaultState();
    const auto& shape = enchantingTable_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(EnchantingTableBlockTest, GetOcclusionShape_CanBeEmpty)
{
    const auto& state = enchantingTable_->defaultState();
    const auto& shape = enchantingTable_->getOcclusionShape(state);
    // 附魔台是非固体方块，遮挡形状可以为空
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(EnchantingTableBlockTest, GetPushReaction_ReturnsBlock)
{
    const auto& state = enchantingTable_->defaultState();
    EXPECT_EQ(enchantingTable_->getPushReaction(state), Material::PushReaction::Block);
}

TEST_F(EnchantingTableBlockTest, CreateBlockEntity_ReturnsEnchantingTableEntity)
{
    BlockPos pos(10, 20, 30);
    auto entity = enchantingTable_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::EnchantingTable);
    EXPECT_EQ(entity->getPos(), pos);
}

// ========== EnchantingTableBlock 交互测试 ==========

class EnchantingTableBlockInteractionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enchantingTable_ = std::make_unique<EnchantingTableBlock>(
            BlockProperties(Material::ROCK).hardness(5.0f).resistance(1200.0f).notSolid());
        pos_ = BlockPos(10, 64, 20);
    }

    std::unique_ptr<EnchantingTableBlock> enchantingTable_;
    BlockPos pos_;
};

TEST_F(EnchantingTableBlockInteractionTest, OnBlockActivated_ClientSide_ReturnsSuccess)
{
    // 客户端世界
    EnchantingTableTestWorld world(true);

    // 设置方块状态
    world.setBlockStateAt(pos_, &enchantingTable_->defaultState());

    // 设置方块实体
    auto entity = std::make_unique<blockentity::EnchantingTableEntity>(pos_);
    world.setOwnedBlockEntity(std::move(entity));

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = enchantingTable_->defaultState();
    BlockRaycastResult hit;
    auto result = enchantingTable_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 客户端应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 客户端不应调用 openContainer
    EXPECT_FALSE(world.wasOpenContainerCalled());
}

TEST_F(EnchantingTableBlockInteractionTest, OnBlockActivated_ServerSide_OpensContainer)
{
    // 服务端世界
    EnchantingTableTestWorld world(false);

    // 设置方块状态
    world.setBlockStateAt(pos_, &enchantingTable_->defaultState());

    // 设置方块实体
    auto entity = std::make_unique<blockentity::EnchantingTableEntity>(pos_);
    world.setOwnedBlockEntity(std::move(entity));

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = enchantingTable_->defaultState();
    BlockRaycastResult hit;
    auto result = enchantingTable_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 服务端应返回 Consume
    EXPECT_EQ(result, ActionResultType::Consume);

    // 服务端应调用 openContainer
    EXPECT_TRUE(world.wasOpenContainerCalled());
    EXPECT_EQ(world.getLastContainerType(), ContainerType::Enchantment);
    EXPECT_EQ(world.getLastContainerPos(), pos_);
    EXPECT_EQ(world.getLastContainerPlayer(), &player);
}

TEST_F(EnchantingTableBlockInteractionTest, OnBlockActivated_NoBlockEntity_ReturnsPass)
{
    // 服务端世界
    EnchantingTableTestWorld world(false);

    // 设置方块状态（但不设置方块实体）
    world.setBlockStateAt(pos_, &enchantingTable_->defaultState());

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = enchantingTable_->defaultState();
    BlockRaycastResult hit;
    auto result = enchantingTable_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 没有方块实体时应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应调用 openContainer
    EXPECT_FALSE(world.wasOpenContainerCalled());
}

TEST_F(EnchantingTableBlockInteractionTest, OnBlockActivated_WrongBlockEntityType_ReturnsPass)
{
    // 服务端世界
    EnchantingTableTestWorld world(false);

    // 设置方块状态
    world.setBlockStateAt(pos_, &enchantingTable_->defaultState());

    // 设置错误类型的方块实体（使用 Chest 实体而非 EnchantingTable 实体）
    auto wrongEntity = std::make_unique<blockentity::ChestEntity>(pos_);
    world.setOwnedBlockEntity(std::move(wrongEntity));

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 执行交互
    const auto& state = enchantingTable_->defaultState();
    BlockRaycastResult hit;
    auto result = enchantingTable_->onBlockActivated(state, world, pos_, player, Hand::MainHand, hit);

    // 错误类型的方块实体应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应调用 openContainer
    EXPECT_FALSE(world.wasOpenContainerCalled());
}

TEST_F(EnchantingTableBlockInteractionTest, OnBlockActivated_OffHand_SameBehavior)
{
    // 服务端世界
    EnchantingTableTestWorld world(false);

    // 设置方块状态
    world.setBlockStateAt(pos_, &enchantingTable_->defaultState());

    // 设置方块实体
    auto entity = std::make_unique<blockentity::EnchantingTableEntity>(pos_);
    world.setOwnedBlockEntity(std::move(entity));

    // 创建玩家
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    // 使用副手执行交互
    const auto& state = enchantingTable_->defaultState();
    BlockRaycastResult hit;
    auto result = enchantingTable_->onBlockActivated(state, world, pos_, player, Hand::OffHand, hit);

    // 副手交互应与服务端行为一致
    EXPECT_EQ(result, ActionResultType::Consume);
    EXPECT_TRUE(world.wasOpenContainerCalled());
    EXPECT_EQ(world.getLastContainerType(), ContainerType::Enchantment);
}
