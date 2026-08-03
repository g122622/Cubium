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

#include "BlockStateProvider.hpp"

#include "common/core/Types.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>

namespace mc::world::gen::feature::state {

SimpleBlockStateProvider::SimpleBlockStateProvider(const BlockState* state)
    : m_state(state)
{}

const BlockState* SimpleBlockStateProvider::getState(
    const IWorld& /*world*/, math::IRandom& /*random*/, i32 /*x*/, i32 /*y*/, i32 /*z*/) const
{
    return m_state;
}

const BlockState* SimpleBlockStateProvider::asSingleState() const noexcept
{
    return m_state;
}

std::unique_ptr<BlockStateProvider> SimpleBlockStateProvider::clone() const
{
    return std::make_unique<SimpleBlockStateProvider>(m_state);
}

} // namespace mc::world::gen::feature::state
