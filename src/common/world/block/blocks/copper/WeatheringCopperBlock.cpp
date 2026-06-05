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

#include "WeatheringCopperBlock.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

WeatheringCopperBlock::WeatheringCopperBlock(
    const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel)
    : Block(properties)
    , m_oxidationLevel(oxidationLevel)
{
    // 只有未达到最高氧化等级的方块需要随机刻
    if (m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized) {
        m_ticksRandomly = true;
    }
}

Block* WeatheringCopperBlock::getNextOxidationBlock() const
{
    return m_nextOxidationBlock;
}

void WeatheringCopperBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 已是最高氧化等级则不处理
    if (m_oxidationLevel == BlockStateProperties::OxidationLevel::Oxidized) {
        return;
    }

    // 获取下一氧化等级的方块
    Block* nextBlock = getNextOxidationBlock();
    if (nextBlock == nullptr) {
        return;
    }

    // MC 1.21 氧化概率: 基础概率约 5.7% (21/369)
    // 但受到周围方块影响
    // 简化实现: 使用基础概率
    // TODO: 完整实现应检查4格曼哈顿距离内的铜方块氧化状态
    if (random.nextFloat() < 0.0569f) {
        const BlockState& nextState = nextBlock->defaultState();
        world.setBlockState(pos, &nextState, 3);
    }
}

} // namespace blocks
} // namespace mc
