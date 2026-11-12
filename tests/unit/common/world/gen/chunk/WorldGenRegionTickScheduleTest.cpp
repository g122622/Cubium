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
 * @file WorldGenRegionTickScheduleTest.cpp
 * @brief WorldGenRegion 方块刻/流体刻调度的落点语义测试
 *
 * 生成期的调度只应落到目标区块自己的 ChunkPrimer 待执行列表：
 * - 区域内：写入对应 proto chunk；
 * - 区域外：直接丢弃（原版 WorldGenTickAccess 同样是 `getChunk(pos).getBlockTicks()`，
 *   取不到区块即失败，不会转发给 ServerLevel）；
 * - 重复调度：同一位置同一方块只保留一条。
 */

#include <gtest/gtest.h>

#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::world::chunk;

namespace {

/**
 * @brief 构造 3x3（半径 1）区域的区块载体
 *
 * 区块按 Z 外层、X 内层排列，与 WorldGenRegion 的内部布局一致。
 */
struct RegionPack {
    std::vector<std::unique_ptr<ChunkPrimer>> primers;
    std::vector<IChunk*> ptrs;
    ChunkPrimer* center = nullptr;
};

RegionPack makePack(ChunkCoord mainX, ChunkCoord mainZ, i32 radius)
{
    RegionPack pack;
    const i32 diameter = radius * 2 + 1;
    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            auto primer = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);
            if (dx == 0 && dz == 0) {
                pack.center = primer.get();
            }
            pack.ptrs.push_back(primer.get());
            pack.primers.push_back(std::move(primer));
        }
    }
    MC_ASSERT_RELEASE(static_cast<i32>(pack.ptrs.size()) == diameter * diameter);
    return pack;
}

} // namespace

class WorldGenRegionTickScheduleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
};

TEST_F(WorldGenRegionTickScheduleTest, BlockTickInsideRegionLandsInThatChunkPrimer)
{
    RegionPack pack = makePack(0, 0, 1);
    WorldGenRegion region(0, 0, 1, pack.ptrs);

    // 主区块内（0,70,0）与相邻区块（-1 区块内）各调度一次。
    const Block& stone = *VanillaBlocks::STONE;
    region.scheduleTick(BlockPos(1, 70, 1), stone, 5, world::tick::TickPriority::Normal);
    region.scheduleTick(BlockPos(-5, 70, 1), stone, 5, world::tick::TickPriority::Normal);

    EXPECT_EQ(pack.center->pendingBlockTicks().size(), 1u);
    EXPECT_EQ(pack.center->pendingBlockTicks()[0].pos, BlockPos(1, 70, 1));

    // X 为 -5 落在区块 (-1, 0)，即半径 1 窗口里的西侧邻居。
    const auto& neighborTicks = pack.primers[3]->pendingBlockTicks();
    ASSERT_EQ(neighborTicks.size(), 1u);
    EXPECT_EQ(neighborTicks[0].pos, BlockPos(-5, 70, 1));
}

TEST_F(WorldGenRegionTickScheduleTest, BlockTickOutsideRegionIsDropped)
{
    RegionPack pack = makePack(0, 0, 1);
    WorldGenRegion region(0, 0, 1, pack.ptrs);

    // (100, 70, 100) 在半径 1 的窗口之外，取不到区块——必须被丢弃而不是崩溃或误记。
    const Block& stone = *VanillaBlocks::STONE;
    region.scheduleTick(BlockPos(100, 70, 100), stone, 5, world::tick::TickPriority::Normal);

    for (const auto& primer : pack.primers) {
        EXPECT_TRUE(primer->pendingBlockTicks().empty());
    }
}

TEST_F(WorldGenRegionTickScheduleTest, DuplicateBlockTickKeepsOnlyFirst)
{
    RegionPack pack = makePack(0, 0, 1);
    WorldGenRegion region(0, 0, 1, pack.ptrs);

    const Block& stone = *VanillaBlocks::STONE;
    region.scheduleTick(BlockPos(2, 70, 2), stone, 5, world::tick::TickPriority::Normal);
    region.scheduleTick(BlockPos(2, 70, 2), stone, 40, world::tick::TickPriority::High);

    const auto& ticks = pack.center->pendingBlockTicks();
    ASSERT_EQ(ticks.size(), 1u);
    // 保留第一条（较早的时间与优先级）。
    EXPECT_EQ(ticks[0].delay, 5);
    EXPECT_EQ(ticks[0].priority, world::tick::TickPriority::Normal);
}

TEST_F(WorldGenRegionTickScheduleTest, FluidTickUsesSameRegionRule)
{
    RegionPack pack = makePack(0, 0, 1);
    WorldGenRegion region(0, 0, 1, pack.ptrs);

    const fluid::Fluid* water = fluid::Fluids::WATER();
    ASSERT_NE(water, nullptr);

    region.scheduleTick(BlockPos(3, 70, 3), *water, 5, world::tick::TickPriority::Normal);
    region.scheduleTick(BlockPos(200, 70, 200), *water, 5, world::tick::TickPriority::Normal);

    ASSERT_EQ(pack.center->pendingFluidTicks().size(), 1u);
    EXPECT_EQ(pack.center->pendingFluidTicks()[0].pos, BlockPos(3, 70, 3));
    // 其余区块（含主区块自身——它已在上面断言过一条）不应再收到流体刻。
    for (const auto& primer : pack.primers) {
        if (primer.get() == pack.center) {
            continue;
        }
        EXPECT_TRUE(primer->pendingFluidTicks().empty());
    }
}
