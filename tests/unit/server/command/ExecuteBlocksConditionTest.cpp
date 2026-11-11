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
 * @file ExecuteBlocksConditionTest.cpp
 * @brief `/execute if blocks` 的区域比对
 *
 * `blocksMatch` 接受查方块回调，因此这里用内存 map 模拟世界，不需要真实 ServerWorld
 * （真实世界要加载区块，慢且易受生成结果影响）。
 */

#include "server/command/commands/ExecuteBlocksCondition.hpp"

#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"

#include <gtest/gtest.h>

#include <unordered_map>

using mc::BlockPos;
using mc::BlockState;
using mc::i32;
using mc::i64;
using mc::VanillaBlocks;
using mc::command::blocksMatch;

namespace {

/// 用 map 模拟的世界：只记录显式写入的位置，未写入即 nullptr（模拟未加载）。
class MapLookup {
public:
    void set(const BlockPos& pos, const BlockState* state)
    {
        m_states[key(pos)] = state;
    }

    [[nodiscard]] const BlockState* get(const BlockPos& pos) const
    {
        const auto it = m_states.find(key(pos));
        return it == m_states.end() ? nullptr : it->second;
    }

    [[nodiscard]] mc::command::BlockStateLookup asLookup() const
    {
        return [this](const BlockPos& pos) {
            return get(pos);
        };
    }

private:
    [[nodiscard]] static i64 key(const BlockPos& pos)
    {
        return (static_cast<i64>(pos.x) << 42) ^ (static_cast<i64>(pos.y) << 21) ^ static_cast<i64>(pos.z);
    }

    std::unordered_map<i64, const BlockState*> m_states;
};

const BlockState* stone()
{
    return &VanillaBlocks::STONE->defaultState();
}

const BlockState* air()
{
    return &VanillaBlocks::AIR->defaultState();
}

const BlockState* dirt()
{
    return &VanillaBlocks::DIRT->defaultState();
}

class ExecuteBlocksConditionTest : public testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
    }
};

} // namespace

// 3x1x1 区域完全一致 → all 模式成立。
TEST_F(ExecuteBlocksConditionTest, AllMatchesIdenticalRegion)
{
    MapLookup world;
    for (i32 x = 0; x < 3; ++x) {
        world.set(BlockPos(x, 0, 0), stone());
        world.set(BlockPos(10 + x, 0, 0), stone());
    }

    EXPECT_TRUE(blocksMatch(BlockPos(0, 0, 0), BlockPos(2, 0, 0), BlockPos(10, 0, 0), false, world.asLookup()));
}

// 目标区域有一格不同 → all 模式不成立。
TEST_F(ExecuteBlocksConditionTest, AllFailsOnSingleBlockDifference)
{
    MapLookup world;
    world.set(BlockPos(0, 0, 0), stone());
    world.set(BlockPos(1, 0, 0), stone());
    world.set(BlockPos(10, 0, 0), stone());
    world.set(BlockPos(11, 0, 0), dirt());

    EXPECT_FALSE(blocksMatch(BlockPos(0, 0, 0), BlockPos(1, 0, 0), BlockPos(10, 0, 0), false, world.asLookup()));
}

// all 模式严格比较空气：源是空气、目标不是 → 不成立。
TEST_F(ExecuteBlocksConditionTest, AllComparesAirStrictly)
{
    MapLookup world;
    world.set(BlockPos(0, 0, 0), air());
    world.set(BlockPos(10, 0, 0), stone());

    EXPECT_FALSE(blocksMatch(BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(10, 0, 0), false, world.asLookup()));
}

// masked 模式跳过源区域的空气 → 源空、目标有方块也算成立。
TEST_F(ExecuteBlocksConditionTest, MaskedSkipsSourceAir)
{
    MapLookup world;
    world.set(BlockPos(0, 0, 0), air());
    world.set(BlockPos(10, 0, 0), stone());

    EXPECT_TRUE(blocksMatch(BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(10, 0, 0), true, world.asLookup()));
}

// masked 模式仍然比较源区域的非空气方块。
TEST_F(ExecuteBlocksConditionTest, MaskedStillComparesSolidBlocks)
{
    MapLookup world;
    world.set(BlockPos(0, 0, 0), stone());
    world.set(BlockPos(10, 0, 0), dirt());

    EXPECT_FALSE(blocksMatch(BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(10, 0, 0), true, world.asLookup()));
}

// start/end 顺序颠倒时要按闭区间处理，不能比较空区域（空区域恒为真会漏判）。
TEST_F(ExecuteBlocksConditionTest, ReversedCornersStillCompareFullRegion)
{
    MapLookup world;
    world.set(BlockPos(0, 0, 0), stone());
    world.set(BlockPos(1, 0, 0), stone());
    world.set(BlockPos(10, 0, 0), stone());
    world.set(BlockPos(11, 0, 0), dirt());

    // 源区间以 (1,0,0)~(0,0,0) 给出，仍应覆盖两格并发现差异。
    EXPECT_FALSE(blocksMatch(BlockPos(1, 0, 0), BlockPos(0, 0, 0), BlockPos(10, 0, 0), false, world.asLookup()));
}

// 任一侧读不到（未加载）→ 不成立，不能把"未知"当成"相同"。
TEST_F(ExecuteBlocksConditionTest, UnloadedPositionIsNotAMatch)
{
    MapLookup world;
    world.set(BlockPos(0, 0, 0), stone());
    // 目标位置故意不写入，get() 返回 nullptr。

    EXPECT_FALSE(blocksMatch(BlockPos(0, 0, 0), BlockPos(0, 0, 0), BlockPos(10, 0, 0), false, world.asLookup()));
}
