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
 */

#include "GenerationChunkCache.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>

namespace mc::server {

GenerationChunkCache::GenerationChunkCache(ChunkCoord centerX, ChunkCoord centerZ, i32 radius)
    : m_centerX(centerX)
    , m_centerZ(centerZ)
    , m_radius(radius)
    , m_diameter(radius * 2 + 1)
    , m_entries(static_cast<size_t>(m_diameter * m_diameter), nullptr)
{}

ChunkPrimer* GenerationChunkCache::get(ChunkCoord x, ChunkCoord z) const
{
    if (!_inBounds(x, z)) {
        return nullptr;
    }
    return m_entries[static_cast<size_t>(_index(x, z))];
}

void GenerationChunkCache::set(ChunkCoord x, ChunkCoord z, ChunkPrimer* primer)
{
    MC_ASSERT_RELEASE(_inBounds(x, z));
    MC_ASSERT_RELEASE(primer != nullptr);
    m_entries[static_cast<size_t>(_index(x, z))] = primer;
}

ChunkPrimer& GenerationChunkCache::getOrCreateOwned(ChunkCoord x, ChunkCoord z)
{
    MC_ASSERT_RELEASE(_inBounds(x, z));
    const size_t index = static_cast<size_t>(_index(x, z));
    if (m_entries[index] != nullptr) {
        return *m_entries[index];
    }

    auto primer = std::make_unique<ChunkPrimer>(x, z);
    ChunkPrimer* result = primer.get();
    m_ownedEntries.push_back(std::move(primer));
    m_entries[index] = result;
    return *result;
}

bool GenerationChunkCache::owns(ChunkCoord x, ChunkCoord z) const
{
    if (!_inBounds(x, z)) {
        return false;
    }
    const ChunkPrimer* primer = m_entries[static_cast<size_t>(_index(x, z))];
    return std::any_of(m_ownedEntries.begin(), m_ownedEntries.end(), [primer](const auto& owned) {
        return owned.get() == primer;
    });
}

bool GenerationChunkCache::contains(ChunkCoord x, ChunkCoord z) const
{
    return _inBounds(x, z);
}

IChunk* GenerationChunkCache::getOrFallback(ChunkCoord x, ChunkCoord z, IChunk* fallback) const
{
    ChunkPrimer* primer = get(x, z);
    if (primer != nullptr) {
        return primer;
    }
    return fallback;
}

i32 GenerationChunkCache::_index(ChunkCoord x, ChunkCoord z) const
{
    const i32 dx = static_cast<i32>(x) - static_cast<i32>(m_centerX) + m_radius;
    const i32 dz = static_cast<i32>(z) - static_cast<i32>(m_centerZ) + m_radius;
    return dz * m_diameter + dx;
}

bool GenerationChunkCache::_inBounds(ChunkCoord x, ChunkCoord z) const
{
    const i32 dx = static_cast<i32>(x) - static_cast<i32>(m_centerX);
    const i32 dz = static_cast<i32>(z) - static_cast<i32>(m_centerZ);
    return dx >= -m_radius && dx <= m_radius && dz >= -m_radius && dz <= m_radius;
}

} // namespace mc::server
