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

#include "WeatheringCopperDoorBlock.hpp"
#include "../../../IWorld.hpp"
#include "IOxidizableBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/DoorBlock.hpp"

namespace mc {
namespace blocks {

WeatheringCopperDoorBlock::WeatheringCopperDoorBlock(
    const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel)
    : DoorBlock(properties, true) // 铜门只能红石控制，类似铁门
    , m_oxidationLevel(oxidationLevel)
{
    if (m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized) {
        m_ticksRandomly = true;
    }
}

void WeatheringCopperDoorBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 只有下半部分方块才触发氧化，避免上下半部分双重氧化
    if (state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()) != BlockStateProperties::DoubleBlockHalf::Lower) {
        return;
    }

    // 尝试氧化下半部分（tryOxidize内部会保留共有属性）
    if (tryOxidize(world, pos, state, random)) {
        // 上半部分也需要一起替换，使用withPropertiesOf保留共有属性
        BlockPos upperPos = pos.up();
        const BlockState* upperState = world.getBlockState(upperPos);
        if (upperState != nullptr && upperState->is(this)) {
            const BlockState& upperNext = m_nextOxidationBlock->defaultState().withPropertiesOf(*upperState);
            world.setBlockState(upperPos, &upperNext, 3);
        }
    }
}

} // namespace blocks
} // namespace mc
