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
 */

#include "WaterlilyBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../Block.hpp"
#include "../../registry/VanillaBlocks.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"

namespace mc {
namespace blocks {

WaterlilyBlock::WaterlilyBlock(const BlockProperties& properties)
    : BushBlock(properties)
{}

bool WaterlilyBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    // 下方须为水或冰，且睡莲自身位置（groundPos 上方一格）无流体。
    // 对齐 vanilla WaterlilyBlock.mayPlaceOn:
    //   (getFluidState(pos).getType() == Fluids.WATER || block instanceof IceBlock)
    //   && getFluidState(pos.above()).getType() == Fluids.EMPTY

    // 上方（睡莲位置）须无流体
    const fluid::FluidState* aboveFluid = world.getFluidState(groundPos.up());
    if (aboveFluid != nullptr && !aboveFluid->isEmpty()) {
        return false;
    }

    // 下方为水（用 FluidTags::WATER 标签，含流动水，与项目 SeagrassBlock 同惯例）
    const fluid::FluidState* belowFluid = world.getFluidState(groundPos);
    if (belowFluid != nullptr && !belowFluid->isEmpty() && belowFluid->getFluid().isIn(fluid::FluidTags::WATER())) {
        return true;
    }

    // 下方为冰方块
    if (groundState.is(VanillaBlocks::ICE)) {
        return true;
    }

    return false;
}

} // namespace blocks
} // namespace mc
