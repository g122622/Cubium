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

#include "WeatheringCopperTrapDoorBlock.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

WeatheringCopperTrapDoorBlock::WeatheringCopperTrapDoorBlock(
    const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel)
    : TrapDoorBlock(properties, true) // 铜活板门只能红石控制，类似铁活板门
    , m_oxidationLevel(oxidationLevel)
{
    // 只有未达到最高氧化等级的方块需要随机刻
    if (m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized) {
        m_ticksRandomly = true;
    }
}

void WeatheringCopperTrapDoorBlock::randomTick(
    IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 已是最高氧化等级则不处理
    if (m_oxidationLevel == BlockStateProperties::OxidationLevel::Oxidized) {
        return;
    }

    // 获取下一氧化等级的方块
    if (m_nextOxidationBlock == nullptr) {
        return;
    }

    // MC 1.21 氧化概率: 基础概率约 5.7%
    if (random.nextFloat() < 0.0569f) {
        const BlockState& nextState = m_nextOxidationBlock->defaultState();
        world.setBlockState(pos, &nextState, 3);
    }
}

} // namespace blocks
} // namespace mc
