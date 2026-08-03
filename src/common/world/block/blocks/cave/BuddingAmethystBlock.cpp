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

#include "BuddingAmethystBlock.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/cave/AmethystBlock.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc {
namespace blocks {

static constexpr int GROWTH_CHANCE = 5; // 1/5 概率

BuddingAmethystBlock::BuddingAmethystBlock(const BlockProperties& properties)
    : AmethystBlock(properties)
{
    m_ticksRandomly = true;
}

void BuddingAmethystBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);

    if (random.nextInt(GROWTH_CHANCE) != 0) {
        return;
    }

    // 随机选择一个方向尝试生长
    Direction dir = static_cast<Direction>(random.nextInt(6));
    BlockPos neighborPos = pos.offset(dir);
    const BlockState* neighborState = world.getBlockState(neighborPos);
    if (!neighborState) {
        return;
    }
    const Block& neighborBlock = neighborState->getBlock();

    // 检查该方向是否可以生长
    const Block* growInto = nullptr;

    if (neighborState->isAir()) {
        // 空气或水可以生长小紫晶芽
        auto fluidState = world.getFluidState(neighborPos);
        if (!fluidState || !fluidState->isSource()) {
            growInto = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:small_amethyst_bud"));
        }
    } else {
        // 检查是否是同方向的紫水晶芽，可以升级
        const ResourceLocation& blockLoc = neighborBlock.blockLocation();
        if (blockLoc == ResourceLocation("minecraft:small_amethyst_bud")) {
            growInto = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:medium_amethyst_bud"));
        } else if (blockLoc == ResourceLocation("minecraft:medium_amethyst_bud")) {
            growInto = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:large_amethyst_bud"));
        } else if (blockLoc == ResourceLocation("minecraft:large_amethyst_bud")) {
            growInto = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:amethyst_cluster"));
        }
    }

    if (growInto != nullptr) {
        auto newState = growInto->defaultState().with(BlockStateProperties::FACING(), dir);

        // 如果原位是水，设置含水
        auto fluidState = world.getFluidState(neighborPos);
        if (fluidState && fluidState->isSource()) {
            newState = newState.with(BlockStateProperties::WATERLOGGED(), true);
        }

        world.setBlockState(neighborPos, &newState, 3);
    }
}

} // namespace blocks
} // namespace mc
