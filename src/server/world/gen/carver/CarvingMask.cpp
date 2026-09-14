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

CarvingMask::CarvingMask(ChunkCoord chunkX, ChunkCoord chunkZ, i32 minY, i32 height)
    : m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_minY(minY)
    , m_height(height)
    , m_mask(static_cast<size_t>(world::CHUNK_WIDTH) * world::CHUNK_WIDTH * static_cast<size_t>(height), false)
{}

bool CarvingMask::isCarved(BlockCoord x, i32 y, BlockCoord z) const
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH || y < m_minY || y >= m_minY + m_height) {
        return false;
    }
    const i32 index = (x) | ((z) << world::CHUNK_SHIFT) | ((y - m_minY) << (world::CHUNK_SHIFT + world::SECTION_SHIFT));
    return m_mask[static_cast<size_t>(index)];
}

void CarvingMask::setCarved(BlockCoord x, i32 y, BlockCoord z)
{
    if (x < 0 || x >= world::CHUNK_WIDTH || z < 0 || z >= world::CHUNK_WIDTH || y < m_minY || y >= m_minY + m_height) {
        return;
    }
    const i32 index = (x) | ((z) << world::CHUNK_SHIFT) | ((y - m_minY) << (world::CHUNK_SHIFT + world::SECTION_SHIFT));
    m_mask[static_cast<size_t>(index)] = true;
}

} // namespace mc
