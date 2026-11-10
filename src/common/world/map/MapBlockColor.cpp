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

#include "common/world/map/MapBlockColor.hpp"

#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc::world::map {

const BlockState* mapColorStateFor(IWorld& world, const BlockState& state, const BlockPos& pos)
{
    // 只有"含流体且顶面不是完整支撑面"时才替换：例如水下的台阶、栅栏等，地图应显示水色。
    // 顶面完整的含水方块（如 waterlogged 石台阶朝上）仍显示自身颜色。
    const fluid::FluidState* fluidState = state.getBlock().getFluidState(state);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return &state;
    }

    if (state.isFaceSturdy(world, pos, Direction::Up, SupportType::Full)) {
        return &state;
    }

    const BlockState* legacyBlock = fluidState->getBlockState();
    return legacyBlock != nullptr ? legacyBlock : &state;
}

} // namespace mc::world::map
