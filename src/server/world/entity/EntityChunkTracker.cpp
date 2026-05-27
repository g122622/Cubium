#include "EntityChunkTracker.hpp"

namespace mc::server {

// ============================================================================
// EntityChunkTracker 实现
// ============================================================================

void EntityChunkTracker::onEntityMoved(
    EntityId id, ChunkCoord oldCx, ChunkCoord oldCz, ChunkCoord newCx, ChunkCoord newCz)
{
    // 如果区块没变，不做任何操作
    if (oldCx == newCx && oldCz == newCz) {
        return;
    }

    // 从旧区块移除
    i64 oldKey = packChunkPos(oldCx, oldCz);
    auto it = m_chunkEntities.find(oldKey);
    if (it != m_chunkEntities.end()) {
        it->second.erase(id);
        if (it->second.empty()) {
            m_chunkEntities.erase(it);
        }
    }

    // 添加到新区块
    i64 newKey = packChunkPos(newCx, newCz);
    m_chunkEntities[newKey].insert(id);
    m_entityChunks[id] = {newCx, newCz};
}

void EntityChunkTracker::onEntityAdded(EntityId id, ChunkCoord cx, ChunkCoord cz)
{
    i64 key = packChunkPos(cx, cz);
    m_chunkEntities[key].insert(id);
    m_entityChunks[id] = {cx, cz};
}

void EntityChunkTracker::onEntityRemoved(EntityId id)
{
    auto it = m_entityChunks.find(id);
    if (it == m_entityChunks.end()) {
        return;
    }

    auto [cx, cz] = it->second;
    i64 key = packChunkPos(cx, cz);

    auto chunkIt = m_chunkEntities.find(key);
    if (chunkIt != m_chunkEntities.end()) {
        chunkIt->second.erase(id);
        if (chunkIt->second.empty()) {
            m_chunkEntities.erase(chunkIt);
        }
    }

    m_entityChunks.erase(it);
}

std::vector<EntityId> EntityChunkTracker::getEntitiesInChunk(ChunkCoord cx, ChunkCoord cz) const
{
    std::vector<EntityId> result;
    i64 key = packChunkPos(cx, cz);
    auto it = m_chunkEntities.find(key);
    if (it != m_chunkEntities.end()) {
        result.reserve(it->second.size());
        for (EntityId id : it->second) {
            result.push_back(id);
        }
    }
    return result;
}

std::optional<std::pair<ChunkCoord, ChunkCoord>> EntityChunkTracker::getEntityChunk(EntityId id) const
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

i64 EntityChunkTracker::packChunkPos(ChunkCoord cx, ChunkCoord cz)
{
    // 高32位为 chunkX，低32位为 chunkZ
    return (static_cast<i64>(cx) << 32) | (static_cast<i64>(cz) & 0xFFFFFFFFLL);
}

} // namespace mc::server
