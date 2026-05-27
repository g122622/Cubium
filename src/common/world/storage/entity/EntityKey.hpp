#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {

class Entity; // 前向声明，Entity 在 mc 命名空间

namespace world::storage {

/**
 * @brief 实体存储键
 *
 * 格式: {chunkX}:{chunkZ}:{uuid}
 * 例如: "12:-34:550e8400e29b41d4a716446655440000"
 *
 * 此格式允许：
 * - 通过区块坐标前缀扫描获取该区块所有实体
 * - 通过 UUID 唯一标识每个实体
 * - 按字典序排列时同一区块的实体聚集在一起
 */
struct EntityKey {
    ChunkCoord chunkX = 0;
    ChunkCoord chunkZ = 0;
    std::string uuid;

    /** @brief 转换为字符串键 */
    [[nodiscard]] std::string toString() const;

    /** @brief 从字符串键解析 */
    [[nodiscard]] static Result<EntityKey> parse(const std::string& str);

    /** @brief 从实体创建键 */
    [[nodiscard]] static EntityKey fromEntity(const Entity& entity);

    /** @brief 构建区块前缀，用于范围查询 */
    [[nodiscard]] static std::string buildChunkPrefix(ChunkCoord chunkX, ChunkCoord chunkZ);
};

} // namespace world::storage
} // namespace mc
