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

#include "TintedGlassBlock.hpp"

#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <utility>

namespace mc {
namespace blocks {

TintedGlassBlock::TintedGlassBlock(BlockProperties properties)
    : SimpleBlock(std::move(properties))
{}

i32 TintedGlassBlock::getOpacity(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 对齐 vanilla TintedGlassBlock#getLightBlock：恒定 15，把整格光线全部吃掉。
    return 15;
}

bool TintedGlassBlock::propagatesSkylightDown(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 对齐 vanilla TintedGlassBlock#propagatesSkylightDown：恒定 false。
    return false;
}

f32 TintedGlassBlock::getShadeBrightness(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 挡光但不投影：它挡的是光的传播，不是面的明暗。
    return 1.0f;
}

} // namespace blocks
} // namespace mc
