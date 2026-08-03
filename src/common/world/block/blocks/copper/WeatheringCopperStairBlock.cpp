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

#include "WeatheringCopperStairBlock.hpp"
#include "../../../IWorld.hpp"
#include "IOxidizableBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/building/StairsBlock.hpp"

namespace mc {
namespace blocks {

WeatheringCopperStairBlock::WeatheringCopperStairBlock(
    const BlockState& baseState, const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel)
    : StairsBlock(baseState, properties)
    , m_oxidationLevel(oxidationLevel)
{
    if (m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized) {
        m_ticksRandomly = true;
    }
}

void WeatheringCopperStairBlock::randomTick(
    IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    (void)tryOxidize(world, pos, state, random);
}

} // namespace blocks
} // namespace mc
