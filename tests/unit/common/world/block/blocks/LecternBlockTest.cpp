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
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/blocks/functional/LecternBlock.hpp"
#include "world/blockentity/interactive/LecternEntity.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;

namespace {

class LecternTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

public:
    explicit LecternTestWorld(LecternBlock& block)
        : m_state(block.defaultState())
        , m_tickManager(*this)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return &m_state; }

    bool setBlockState(i32, i32, i32, const BlockState* state) override
    {
        if (state == nullptr) {
            return false;
        }
        m_state = *state;
        ++m_setBlockCalls;
        return true;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_entities.find(pos);
        return it == m_entities.end() ? nullptr : it->second.get();
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_entities.find(pos);
        return it == m_entities.end() ? nullptr : it->second.get();
    }

    void setOwnedBlockEntity(std::unique_ptr<BlockEntity> entity)
    {
        const BlockPos pos = entity->getPos();
        m_entities[pos] = std::move(entity);
    }

    void updateNeighbors(const BlockPos& pos, Block& sourceBlock) override
    {
        ++m_updateNeighborsCalls;
        m_lastUpdateNeighborsPos = pos;
        MC_UNUSED(sourceBlock);
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override { return m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return m_tickManager; }

    [[nodiscard]] i32 setBlockCalls() const { return m_setBlockCalls; }
    [[nodiscard]] i32 updateNeighborsCalls() const { return m_updateNeighborsCalls; }
    [[nodiscard]] const BlockPos& lastUpdateNeighborsPos() const { return m_lastUpdateNeighborsPos; }

private:
    BlockState m_state;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_entities;
    world::tick::TickManager m_tickManager;
    i32 m_setBlockCalls = 0;
    i32 m_updateNeighborsCalls = 0;
    BlockPos m_lastUpdateNeighborsPos;
};

Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(1));
}

} // namespace

class LecternBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_book = ensureTestItem("book");
        m_stick = ensureTestItem("stick");
        m_block = std::make_unique<LecternBlock>(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    }

    Item* m_book = nullptr;
    Item* m_stick = nullptr;
    std::unique_ptr<LecternBlock> m_block;
};

TEST_F(LecternBlockTest, TryPlaceBook_RejectsNonBook)
{
    BlockState state = m_block->defaultState();
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    ASSERT_FALSE(LecternBlock::tryPlaceBook(world, pos, ItemStack(m_stick, 1)));
    EXPECT_EQ(world.setBlockCalls(), 0);
}

TEST_F(LecternBlockTest, TryPlaceBook_AcceptsBookAndSetsState)
{
    BlockState state = m_block->defaultState();
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    ASSERT_TRUE(LecternBlock::tryPlaceBook(world, pos, ItemStack(m_book, 1)));
    EXPECT_EQ(world.setBlockCalls(), 1);

    const BlockState* applied = world.getBlockState(pos);
    ASSERT_NE(applied, nullptr);
    EXPECT_TRUE(applied->get(BlockStateProperties::HAS_BOOK()));
}

TEST_F(LecternBlockTest, ComparatorInput_UsesLecternEntitySignal)
{
    BlockState state = m_block->defaultState().with(BlockStateProperties::HAS_BOOK(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(4, 5, 6);

    auto lecternEntity = std::make_unique<blockentity::LecternEntity>(pos);
    ASSERT_TRUE(lecternEntity->setBook(ItemStack(m_book, 1)));
    lecternEntity->setPage(0);
    world.setOwnedBlockEntity(std::move(lecternEntity));

    const int signal = m_block->getComparatorInputOverride(state, world, pos);
    EXPECT_EQ(signal, 1);
}

TEST_F(LecternBlockTest, Pulse_SetsPoweredAndSchedulesTick)
{
    BlockState state = m_block->defaultState().with(BlockStateProperties::HAS_BOOK(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(8, 9, 10);

    LecternBlock::pulse(world, pos, state);

    EXPECT_EQ(world.setBlockCalls(), 1);

    // 验证 tick 被调度（通过 TickManager API 检查）
    EXPECT_TRUE(world.tickManager().isBlockTickScheduled(pos, *m_block));
}

// ========== 红石信号更新测试 ==========

TEST_F(LecternBlockTest, Pulse_NotifiesBelowBlock)
{
    BlockState state = m_block->defaultState().with(BlockStateProperties::HAS_BOOK(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(5, 10, 7);

    EXPECT_EQ(world.updateNeighborsCalls(), 0);

    LecternBlock::pulse(world, pos, state);

    // pulse → changePowered → setBlockState + updateBelow
    // changePowered 调用 updateBelow，应通知 pos.down() 位置
    EXPECT_EQ(world.updateNeighborsCalls(), 1);
    EXPECT_EQ(world.lastUpdateNeighborsPos(), pos.down());
}

TEST_F(LecternBlockTest, ChangePowered_True_SetsPoweredAndNotifiesBelow)
{
    BlockState state = m_block->defaultState().with(BlockStateProperties::HAS_BOOK(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(3, 5, 8);

    EXPECT_EQ(world.updateNeighborsCalls(), 0);

    LecternBlock::changePowered(world, pos, state, true);

    // setBlockState + updateBelow 各调用一次
    EXPECT_EQ(world.setBlockCalls(), 1);
    EXPECT_EQ(world.updateNeighborsCalls(), 1);
    EXPECT_EQ(world.lastUpdateNeighborsPos(), pos.down());

    // 验证 POWERED 被设为 true
    const BlockState* applied = world.getBlockState(pos);
    ASSERT_NE(applied, nullptr);
    EXPECT_TRUE(applied->get(BlockStateProperties::POWERED()));
}

TEST_F(LecternBlockTest, ChangePowered_False_ClearsPoweredAndNotifiesBelow)
{
    BlockState state = m_block->defaultState()
                           .with(BlockStateProperties::HAS_BOOK(), true)
                           .with(BlockStateProperties::POWERED(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(3, 5, 8);

    LecternBlock::changePowered(world, pos, state, false);

    EXPECT_EQ(world.setBlockCalls(), 1);
    EXPECT_EQ(world.updateNeighborsCalls(), 1);
    EXPECT_EQ(world.lastUpdateNeighborsPos(), pos.down());

    const BlockState* applied = world.getBlockState(pos);
    ASSERT_NE(applied, nullptr);
    EXPECT_FALSE(applied->get(BlockStateProperties::POWERED()));
}

TEST_F(LecternBlockTest, UpdateBelow_NotifiesPositionBelow)
{
    LecternTestWorld world(*m_block);
    const BlockPos pos(10, 20, 30);

    LecternBlock::updateBelow(world, pos, *m_block);

    EXPECT_EQ(world.updateNeighborsCalls(), 1);
    EXPECT_EQ(world.lastUpdateNeighborsPos(), pos.down());
}

TEST_F(LecternBlockTest, Tick_ClearsPoweredAndNotifiesBelow)
{
    BlockState poweredState = m_block->defaultState()
                                  .with(BlockStateProperties::HAS_BOOK(), true)
                                  .with(BlockStateProperties::POWERED(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(7, 12, 15);
    math::Random rng;

    // tick 应该通过 changePowered(false) 清除 POWERED 并通知下方
    m_block->tick(world, pos, poweredState, rng);

    EXPECT_EQ(world.setBlockCalls(), 1);
    EXPECT_EQ(world.updateNeighborsCalls(), 1);
    EXPECT_EQ(world.lastUpdateNeighborsPos(), pos.down());

    const BlockState* applied = world.getBlockState(pos);
    ASSERT_NE(applied, nullptr);
    EXPECT_FALSE(applied->get(BlockStateProperties::POWERED()));
}

TEST_F(LecternBlockTest, Tick_DoesNothingWhenNotPowered)
{
    BlockState unpoweredState = m_block->defaultState()
                                    .with(BlockStateProperties::HAS_BOOK(), true)
                                    .with(BlockStateProperties::POWERED(), false);
    LecternTestWorld world(*m_block);
    const BlockPos pos(7, 12, 15);
    math::Random rng;

    m_block->tick(world, pos, unpoweredState, rng);

    EXPECT_EQ(world.setBlockCalls(), 0);
    EXPECT_EQ(world.updateNeighborsCalls(), 0);
}

TEST_F(LecternBlockTest, SetHasBook_NotifiesBelowBlock)
{
    BlockState state = m_block->defaultState();
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    EXPECT_EQ(world.updateNeighborsCalls(), 0);

    LecternBlock::setHasBook(world, pos, true);

    // setHasBook 应该设置状态并调用 updateBelow
    EXPECT_EQ(world.setBlockCalls(), 1);
    EXPECT_EQ(world.updateNeighborsCalls(), 1);
    EXPECT_EQ(world.lastUpdateNeighborsPos(), pos.down());

    const BlockState* applied = world.getBlockState(pos);
    ASSERT_NE(applied, nullptr);
    EXPECT_TRUE(applied->get(BlockStateProperties::HAS_BOOK()));
    EXPECT_FALSE(applied->get(BlockStateProperties::POWERED()));
}

TEST_F(LecternBlockTest, OnBlockRemoved_PoweredState_NotifiesBelow)
{
    BlockState poweredState = m_block->defaultState()
                                  .with(BlockStateProperties::HAS_BOOK(), true)
                                  .with(BlockStateProperties::POWERED(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    EXPECT_EQ(world.updateNeighborsCalls(), 0);

    m_block->onBlockRemoved(world, pos, poweredState);

    // onBlockRemoved 在 POWERED 状态时应调用 updateBelow
    EXPECT_EQ(world.updateNeighborsCalls(), 1);
    EXPECT_EQ(world.lastUpdateNeighborsPos(), pos.down());
}

TEST_F(LecternBlockTest, OnBlockRemoved_UnpoweredState_DoesNotNotifyBelow)
{
    BlockState unpoweredState = m_block->defaultState()
                                    .with(BlockStateProperties::HAS_BOOK(), true)
                                    .with(BlockStateProperties::POWERED(), false);
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    m_block->onBlockRemoved(world, pos, unpoweredState);

    // 未供电状态不应触发红石更新
    EXPECT_EQ(world.updateNeighborsCalls(), 0);
}

TEST_F(LecternBlockTest, GetStrongPower_Returns15WhenPoweredAndSideIsUp)
{
    BlockState poweredState = m_block->defaultState().with(BlockStateProperties::POWERED(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    // 讲台强信号只在 Direction::Up 方向输出
    EXPECT_EQ(m_block->getStrongPower(poweredState, world, pos, Direction::Up), 15);
    EXPECT_EQ(m_block->getStrongPower(poweredState, world, pos, Direction::Down), 0);
    EXPECT_EQ(m_block->getStrongPower(poweredState, world, pos, Direction::North), 0);
}

TEST_F(LecternBlockTest, GetWeakPower_Returns15WhenPowered)
{
    BlockState poweredState = m_block->defaultState().with(BlockStateProperties::POWERED(), true);
    LecternTestWorld world(*m_block);
    const BlockPos pos(1, 2, 3);

    // 讲台弱信号向所有方向输出15
    EXPECT_EQ(m_block->getWeakPower(poweredState, world, pos, Direction::Down), 15);
    EXPECT_EQ(m_block->getWeakPower(poweredState, world, pos, Direction::North), 15);
}
