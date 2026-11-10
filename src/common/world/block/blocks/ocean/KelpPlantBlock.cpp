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

#include "KelpPlantBlock.hpp"

#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/NetherBlocks.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include <utility>

namespace mc {
namespace blocks {

// 与 SeagrassBlock 同款：无 AGE、无 HALF 的纯水下植物，故无默认状态覆写。
KelpPlantBlock::KelpPlantBlock(BlockProperties properties)
    : SimpleBlock(std::move(properties))
{}

const fluid::FluidState* KelpPlantBlock::getFluidState(const BlockState&) const
{
    return &fluid::Fluids::WATER()->defaultState();
}

bool KelpPlantBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 与 KelpBlock 同一条原版判据（GrowingPlantBlock.canSurvive + 海带头的 canAttachTo）。
    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    if (VanillaBlocks::KELP != nullptr && belowState->is(VanillaBlocks::KELP)) {
        return true;
    }

    if (belowState->is(this)) {
        return true;
    }

    if (block_registry::NetherBlocks::MAGMA != nullptr && belowState->is(block_registry::NetherBlocks::MAGMA)) {
        return false;
    }

    return belowState->isFaceSturdy(world, belowPos, Direction::Up, SupportType::Full);
}

} // namespace blocks
} // namespace mc
