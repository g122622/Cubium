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

#include <memory>
#include <gtest/gtest.h>

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/list/EmptyTickList.hpp"
#include "common/world/tick/list/ServerTickList.hpp"

using namespace mc;
using namespace mc::world::tick;

// ============================================================================
// 测试用 Mock Block
// ============================================================================

class MockBlock : public Block {
public:
    MockBlock()
        : Block(BlockProperties(Material::ROCK))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, auto values, const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts, const std::vector<BlockState*>* allStates, u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
        createBlockState(std::move(container));
    }

    bool isAir(const BlockState& state) const override { return m_isAir; }

    void setAir(bool air) { m_isAir = air; }

private:
    bool m_isAir = false;
};

// ============================================================================
// 简化的 TickList 测试（不依赖ServerWorld）
// ============================================================================

class TickPriorityTest : public ::testing::Test {
protected:
    void SetUp() override { block = std::make_unique<MockBlock>(); }

    std::unique_ptr<MockBlock> block;
};

TEST_F(TickPriorityTest, FromToInt)
{
    EXPECT_EQ(fromInt(-3), TickPriority::ExtremelyHigh);
    EXPECT_EQ(fromInt(-2), TickPriority::VeryHigh);
    EXPECT_EQ(fromInt(-1), TickPriority::High);
    EXPECT_EQ(fromInt(0), TickPriority::Normal);
    EXPECT_EQ(fromInt(1), TickPriority::Low);
    EXPECT_EQ(fromInt(2), TickPriority::VeryLow);
    EXPECT_EQ(fromInt(3), TickPriority::ExtremelyLow);

    // 边界值
    EXPECT_EQ(fromInt(-100), TickPriority::ExtremelyHigh);
    EXPECT_EQ(fromInt(100), TickPriority::ExtremelyLow);

    EXPECT_EQ(toInt(TickPriority::ExtremelyHigh), -3);
    EXPECT_EQ(toInt(TickPriority::Normal), 0);
    EXPECT_EQ(toInt(TickPriority::ExtremelyLow), 3);
}

// ============================================================================
// ScheduledTick 测试
// ============================================================================

class ScheduledTickTest : public ::testing::Test {
protected:
    void SetUp() override { block = std::make_unique<MockBlock>(); }

    std::unique_ptr<MockBlock> block;
};

TEST_F(ScheduledTickTest, ComparisonByTime)
{
    BlockPos pos(10, 64, 10);

    ScheduledTick<Block> tick1(pos, block.get(), 10, TickPriority::Normal, 1);
    ScheduledTick<Block> tick2(pos, block.get(), 20, TickPriority::Normal, 2);

    // 时间早的先执行
    EXPECT_TRUE(tick1 < tick2);
    EXPECT_FALSE(tick2 < tick1);
}

TEST_F(ScheduledTickTest, ComparisonByPriority)
{
    BlockPos pos(10, 64, 10);

    ScheduledTick<Block> tick1(pos, block.get(), 10, TickPriority::Normal, 1);
    ScheduledTick<Block> tick2(pos, block.get(), 10, TickPriority::High, 2);

    // 同时间，优先级高的先执行（数值小的优先级高）
    EXPECT_TRUE(tick2 < tick1);
    EXPECT_FALSE(tick1 < tick2);
}

TEST_F(ScheduledTickTest, ComparisonById)
{
    BlockPos pos(10, 64, 10);

    ScheduledTick<Block> tick1(pos, block.get(), 10, TickPriority::Normal, 1);
    ScheduledTick<Block> tick2(pos, block.get(), 10, TickPriority::Normal, 2);

    // 同时间同优先级，ID小的先执行
    EXPECT_TRUE(tick1 < tick2);
    EXPECT_FALSE(tick2 < tick1);
}

TEST_F(ScheduledTickTest, Equality)
{
    BlockPos pos1(10, 64, 10);
    BlockPos pos2(20, 64, 20);

    ScheduledTick<Block> tick1(pos1, block.get(), 10, TickPriority::Normal, 1);
    ScheduledTick<Block> tick2(pos1, block.get(), 20, TickPriority::High, 2);

    // 相同位置和目标应该相等（忽略时间和优先级）
    EXPECT_TRUE(tick1 == tick2);

    ScheduledTick<Block> tick3(pos2, block.get(), 10, TickPriority::Normal, 3);
    EXPECT_FALSE(tick1 == tick3); // 不同位置
}

TEST_F(ScheduledTickTest, HashCode)
{
    BlockPos pos(10, 64, 10);

    ScheduledTick<Block> tick1(pos, block.get(), 10, TickPriority::Normal, 1);
    ScheduledTick<Block> tick2(pos, block.get(), 20, TickPriority::High, 2);

    // 相同位置和目标应该有相同的hash
    EXPECT_EQ(tick1.hashCode(), tick2.hashCode());
}

TEST_F(ScheduledTickTest, HashFunction)
{
    BlockPos pos(10, 64, 10);

    ScheduledTick<Block> tick(pos, block.get(), 10, TickPriority::Normal, 1);
    ScheduledTickHash<Block> hashFunc;

    size_t hash = hashFunc(tick);
    EXPECT_EQ(hash, tick.hashCode());
}

// ============================================================================
// EmptyTickList 测试
// ============================================================================

class EmptyTickListTest : public ::testing::Test {
protected:
    void SetUp() override { block = std::make_unique<MockBlock>(); }

    std::unique_ptr<MockBlock> block;
};

TEST_F(EmptyTickListTest, AllOperationsAreNoOp)
{
    EmptyTickList<Block> emptyList;

    BlockPos pos(10, 64, 10);

    // 所有操作都应该什么都不做
    emptyList.scheduleTick(pos, *block, 10);
    emptyList.scheduleTick(pos, *block, 10, TickPriority::High);

    EXPECT_FALSE(emptyList.isTickScheduled(pos, *block));
    EXPECT_FALSE(emptyList.isTickPending(pos, *block));
    EXPECT_EQ(emptyList.pendingCount(), 0);
}

TEST_F(EmptyTickListTest, CancelReturnsFalse)
{
    EmptyTickList<Block> emptyList;

    BlockPos pos(10, 64, 10);
    EXPECT_FALSE(emptyList.cancelTick(pos, *block));
}

TEST_F(EmptyTickListTest, SingletonPattern)
{
    // 单例模式
    EmptyTickList<Block>& list1 = EmptyTickList<Block>::get();
    EmptyTickList<Block>& list2 = EmptyTickList<Block>::get();

    EXPECT_EQ(&list1, &list2);
}
