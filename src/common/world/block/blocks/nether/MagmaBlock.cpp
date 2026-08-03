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

#include "MagmaBlock.hpp"
#include "../ocean/BubbleColumnBlock.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <utility>

namespace mc::blocks {

MagmaBlock::MagmaBlock(BlockProperties properties)
    : Block(std::move(properties))
{}

void MagmaBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 调度 tick 以检查气泡柱
    Block& block = state.getBlockMutable();
    world.tickManager().scheduleBlockTick(pos, block, 20);
}

void MagmaBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(isMoving);

    // 当上方有水时调度 tick
    if (neighborPos.x == pos.x && neighborPos.y == pos.y + 1 && neighborPos.z == pos.z) {
        const BlockState* aboveState = world.getBlockState(neighborPos);
        if (aboveState != nullptr) {
            const fluid::FluidState* fluidState = aboveState->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty() &&
                fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                // 需要调度 tick，使用 MagmaBlock 自身
                world.tickManager().scheduleBlockTick(pos, *const_cast<MagmaBlock*>(this), 20);
            }
        }
    }
}

void MagmaBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);
    MC_UNUSED(random);

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);

    // 调用 BubbleColumnBlock 的静态方法放置气泡柱
    // true = DRAG（下拖气泡柱，由岩浆块产生）
    BubbleColumnBlock::placeBubbleColumn(world, abovePos, true);
}

void MagmaBlock::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(&world);
    MC_UNUSED(&pos);

    // 非潜行的活体生物踩在岩浆块上受到烫脚伤害（1点 = 半颗心）
    if (!entity.isSteppingCarefully()) {
        auto* living = dynamic_cast<LivingEntity*>(&entity);
        if (living != nullptr) {
            auto damage = DamageSources::hotFloor();
            living->hurt(damage, 1.0f);
        }
    }
}

} // namespace mc::blocks
