#pragma once

#include "common/core/Types.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::server {

/**
 * @brief 实体区块归属跟踪器
 *
 * 跟踪每个实体当前所属的区块坐标。
 * 当实体跨区块移动时更新映射。
 * 提供按区块查询所有实体的能力。
 *
 * 用于：
 * - 区块卸载时批量保存/移除实体
 * - 区块加载时恢复实体
 * - 实体移动时更新归属关系
 */
class EntityChunkTracker {
public:
    /**
     * @brief 实体移动时更新区块归属
     *
     * 如果新区块与旧区块相同，不做任何操作。
     *
     * @param id 实体ID
     * @param oldCx 旧区块X坐标
     * @param oldCz 旧区块Z坐标
     * @param newCx 新区块X坐标
     * @param newCz 新区块Z坐标
     */
    void onEntityMoved(EntityId id, ChunkCoord oldCx, ChunkCoord oldCz, ChunkCoord newCx, ChunkCoord newCz);

    /**
     * @brief 实体添加到世界时注册
     *
     * @param id 实体ID
     * @param cx 区块X坐标
     * @param cz 区块Z坐标
     */
    void onEntityAdded(EntityId id, ChunkCoord cx, ChunkCoord cz);

    /**
     * @brief 实体从世界移除时注销
     *
     * @param id 实体ID
     */
    void onEntityRemoved(EntityId id);

    /**
     * @brief 获取区块内所有实体ID
     *
     * @param cx 区块X坐标
     * @param cz 区块Z坐标
     * @return 实体ID列表
     */
    [[nodiscard]] std::vector<EntityId> getEntitiesInChunk(ChunkCoord cx, ChunkCoord cz) const;

    /**
     * @brief 获取实体所在区块坐标
     *
     * @param id 实体ID
     * @return 区块坐标（如果实体未注册返回 std::nullopt）
     */
    [[nodiscard]] std::optional<std::pair<ChunkCoord, ChunkCoord>> getEntityChunk(EntityId id) const;

    /**
     * @brief 获取被跟踪的实体总数
     */
    [[nodiscard]] size_t entityCount() const;

    /**
     * @brief 清空所有跟踪数据
     */
    void clear();

private:
    /** 实体ID → 区块坐标映射 */
    std::unordered_map<EntityId, std::pair<ChunkCoord, ChunkCoord>> m_entityChunks;

    /** 区块坐标 → 实体ID集合映射 */
    std::unordered_map<i64, std::unordered_set<EntityId>> m_chunkEntities;

    /**
     * @brief 将区块坐标打包为 i64 键
     * 高32位为 chunkX，低32位为 chunkZ
     */
    [[nodiscard]] static i64 packChunkPos(ChunkCoord cx, ChunkCoord cz);
};

} // namespace mc::server
