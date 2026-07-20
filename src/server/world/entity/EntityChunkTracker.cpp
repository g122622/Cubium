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

#include "EntityChunkTracker.hpp"

namespace mc::server {

// ============================================================================
// EntityChunkTracker 实现
// ============================================================================

void EntityChunkTracker::onEntityMoved(
    EntityInstanceId id, ChunkCoord oldCx, ChunkCoord oldCz, ChunkCoord newCx, ChunkCoord newCz)
{
    // 如果区块没变，不做任何操作
    if (oldCx == newCx && oldCz == newCz) {
        return;
    }

    // 从旧区块移除
    i64 oldKey = _packChunkPos(oldCx, oldCz);
    auto it = m_chunkEntities.find(oldKey);
    if (it != m_chunkEntities.end()) {
        it->second.erase(id);
        if (it->second.empty()) {
            m_chunkEntities.erase(it);
        }
    }

    // 添加到新区块
    i64 newKey = _packChunkPos(newCx, newCz);
    m_chunkEntities[newKey].insert(id);
    m_entityChunks[id] = {newCx, newCz};
}

void EntityChunkTracker::onEntityAdded(EntityInstanceId id, ChunkCoord cx, ChunkCoord cz)
{
    i64 key = _packChunkPos(cx, cz);
    m_chunkEntities[key].insert(id);
    m_entityChunks[id] = {cx, cz};
}

void EntityChunkTracker::onEntityRemoved(EntityInstanceId id)
{
    auto it = m_entityChunks.find(id);
    if (it == m_entityChunks.end()) {
        return;
    }

    auto [cx, cz] = it->second;
    i64 key = _packChunkPos(cx, cz);

    auto chunkIt = m_chunkEntities.find(key);
    if (chunkIt != m_chunkEntities.end()) {
        chunkIt->second.erase(id);
        if (chunkIt->second.empty()) {
            m_chunkEntities.erase(chunkIt);
        }
    }

    m_entityChunks.erase(it);
}

std::vector<EntityInstanceId> EntityChunkTracker::getEntitiesInChunk(ChunkCoord cx, ChunkCoord cz) const
{
    std::vector<EntityInstanceId> result;
    i64 key = _packChunkPos(cx, cz);
    auto it = m_chunkEntities.find(key);
    if (it != m_chunkEntities.end()) {
        result.reserve(it->second.size());
        for (EntityInstanceId id : it->second) {
            result.push_back(id);
        }
    }
    return result;
}

std::optional<std::pair<ChunkCoord, ChunkCoord>> EntityChunkTracker::getEntityChunk(EntityInstanceId id) const
{
    auto it = m_entityChunks.find(id);
    if (it != m_entityChunks.end()) {
        return it->second;
    }
    return std::nullopt;
}

size_t EntityChunkTracker::entityCount() const
{
    return m_entityChunks.size();
}

void EntityChunkTracker::clear()
{
    m_entityChunks.clear();
    m_chunkEntities.clear();
}

i64 EntityChunkTracker::_packChunkPos(ChunkCoord cx, ChunkCoord cz)
{
    // 高32位为 chunkX，低32位为 chunkZ
    return (static_cast<i64>(cx) << 32) | (static_cast<i64>(cz) & 0xFFFFFFFFLL);
}

} // namespace mc::server
