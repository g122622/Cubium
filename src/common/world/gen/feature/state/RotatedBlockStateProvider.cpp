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

#include "RotatedBlockStateProvider.hpp"

#include "common/util/Direction.hpp"
#include "common/world/block/blocks/RotatedPillarBlock.hpp"

namespace mc::world::gen::feature::state {

RotatedBlockStateProvider::RotatedBlockStateProvider(const Block* block)
    : m_block(block)
{}

const BlockState* RotatedBlockStateProvider::getState(
    const IWorld& /*world*/, math::IRandom& random, i32 /*x*/, i32 /*y*/, i32 /*z*/) const
{
    if (m_block == nullptr) {
        return nullptr;
    }
    const BlockState& def = m_block->defaultState();
    // Direction.Axis.getRandom(random) = VALUES[nextInt(3)]，三轴等概率。
    const Axis axis = Axes::all()[static_cast<size_t>(random.nextInt(3))];
    // trySetValue 语义：无 AXIS 属性则原样返回默认状态。
    if (def.hasProperty(RotatedPillarBlock::AXIS())) {
        return &def.with(RotatedPillarBlock::AXIS(), axis);
    }
    return &def;
}

std::unique_ptr<BlockStateProvider> RotatedBlockStateProvider::clone() const
{
    return std::make_unique<RotatedBlockStateProvider>(m_block);
}

} // namespace mc::world::gen::feature::state
