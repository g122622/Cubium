/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SoulSandBlock.hpp"
#include "../ocean/BubbleColumnBlock.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <utility>

namespace mc::blocks {

SoulSandBlock::SoulSandBlock(BlockProperties properties)
    : Block(std::move(properties))
{}

void SoulSandBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 调度 tick 以检查上方水源并生成涌流气泡柱。
    Block& block = state.getBlockMutable();
    world.tickManager().scheduleBlockTick(pos, block, 20);
}

void SoulSandBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(isMoving);

    // 当上方变为水时调度 tick，以便在水源位置生成涌流气泡柱。
    if (neighborPos.x == pos.x && neighborPos.y == pos.y + 1 && neighborPos.z == pos.z) {
        const BlockState* aboveState = world.getBlockState(neighborPos);
        if (aboveState != nullptr) {
            const fluid::FluidState* fluidState = aboveState->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty() &&
                fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                world.tickManager().scheduleBlockTick(pos, *const_cast<SoulSandBlock*>(this), 20);
            }
        }
    }
}

void SoulSandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);
    MC_UNUSED(random);

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);

    // 调用 BubbleColumnBlock 的静态方法放置涌流气泡柱。
    // false = 非拖拽（DRAG=false，向上推动，由灵魂沙产生），与岩浆块的涡流（DRAG=true）相反。
    BubbleColumnBlock::placeBubbleColumn(world, abovePos, false);
}

} // namespace mc::blocks
