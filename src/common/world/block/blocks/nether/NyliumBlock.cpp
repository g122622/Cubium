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

#include "NyliumBlock.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"

namespace mc::blocks {

// ============================================================================
// NyliumBlock 实现
// ============================================================================

NyliumBlock::NyliumBlock(BlockProperties properties)
    : Block(std::move(properties))
{}

void NyliumBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{

    MC_UNUSED(random); // 退化不需要随机数

    // 如果位置不够暗，退化为下界岩
    if (!_isDarkEnough(world, pos, state)) {
        world.setBlockState(pos, &VanillaBlocks::NETHERRACK->defaultState());
    }
}

bool NyliumBlock::_isDarkEnough(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    static const BlockState* s_airState = &VanillaBlocks::AIR->defaultState();
    const BlockState& resolvedAboveState = aboveState != nullptr ? *aboveState : *s_airState;
    const i32 lightBlockInto = LightEngineUtils::getLightBlockInto(
        world, state, pos, resolvedAboveState, abovePos, Direction::Up, resolvedAboveState.getOpacity());
    return lightBlockInto < game::MAX_LIGHT_LEVEL;
}

} // namespace mc::blocks
