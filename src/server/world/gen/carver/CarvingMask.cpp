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

#include "CarvingMask.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include <cstddef>

namespace mc {

CarvingMask::CarvingMask(i32 minY, i32 height)
    : m_minY(minY)
    , m_height(height)
    , m_mask(static_cast<size_t>(world::CHUNK_WIDTH) * world::CHUNK_WIDTH * static_cast<size_t>(height), false)
{}

i32 CarvingMask::_index(BlockCoord x, i32 y, BlockCoord z) const
{
    // 与原版 CarvingMask#getIndex 逐项对应：X、Z 按区块宽度掩码，Y 用相对 minY 的偏移。
    // 掩码宽度取 world::CHUNK_WIDTH 而不是字面量 15，区块尺寸变化时公式自动跟随。
    const i32 xzMask = world::CHUNK_WIDTH - 1;
    return (static_cast<i32>(x) & xzMask) | ((static_cast<i32>(z) & xzMask) << world::CHUNK_SHIFT) |
        ((y - m_minY) << (world::CHUNK_SHIFT * 2));
}

bool CarvingMask::isCarved(BlockCoord x, i32 y, BlockCoord z) const
{
    return m_mask[static_cast<size_t>(_index(x, y, z))];
}

void CarvingMask::setCarved(BlockCoord x, i32 y, BlockCoord z)
{
    m_mask[static_cast<size_t>(_index(x, y, z))] = true;
}

} // namespace mc
