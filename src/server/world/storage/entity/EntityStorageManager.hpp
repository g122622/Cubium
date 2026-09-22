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

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "EntityKey.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

namespace mc {

class Entity;

namespace ecs {
class EntityRegistry;
} // namespace ecs

namespace world::storage {

// 前向声明
class RocksDBDatabase;

/**
 * @brief 实体存储管理器
 *
 * 负责实体的持久化存储，使用 RocksDB 列族。
 *
 * 存储格式：
 * - 键: {chunkX}:{chunkZ}:{uuid} (字符串)
 * - 值: gzip 压缩的 NBT 二进制数据 (Java 格式)
 *
 * 列族映射：
 * - entities_overworld: 主世界实体
 * - entities_nether: 下界实体
 * - entities_the_end: 末地实体
 */
class EntityStorageManager {
public:
    /**
     * @brief 构造函数
     * @param db RocksDB 数据库引用
     */
    explicit EntityStorageManager(RocksDBDatabase& db);

    ~EntityStorageManager() = default;

    // 禁止拷贝
    EntityStorageManager(const EntityStorageManager&) = delete;
    EntityStorageManager& operator=(const EntityStorageManager&) = delete;

    // ========== 单实体操作 ==========

    /**
     * @brief 从存储加载单个实体
     *
     * 仅反序列化实体本身，不处理 Passengers。若 NBT 含 Passengers 标签，
     * 会暂存到实体的 m_pendingPassengersNbt，由调用方在 spawn 主实体后
     * 调用 EntityDeserializer::attachPassengers 处理。
     *
     * @param uuid 实体 UUID
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @param registry ECS 实体注册表，透传给 EntityDeserializer（Entity 构造时在此 registry 内
     *   create ECS 实体并 attach 高频组件）。由调用方 ServerWorld 经 *entityRegistry() 传入。
     * @return 实体实例或错误
     */
    Result<std::unique_ptr<Entity>> loadEntity(const std::string& uuid,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        DimensionId dimension,
        ecs::EntityRegistry& registry);

    /**
     * @brief 从存储删除实体
     *
     * @param uuid 实体 UUID
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @return 成功或错误
     */
    Result<void> deleteEntity(const std::string& uuid, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension);

    // ========== 区块级操作 ==========

    /**
     * @brief 加载区块内所有实体
     *
     * 使用 RocksDB 前缀扫描获取指定区块的所有实体。仅反序列化实体本身，
     * Passengers 处理同 loadEntity。调用方应在 spawn 每个实体后调用
     * EntityDeserializer::attachPassengers 挂载乘客。
     *
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @param registry ECS 实体注册表，透传给 EntityDeserializer
     * @return 实体列表
     */
    Result<std::vector<std::unique_ptr<Entity>>> loadEntitiesInChunk(
        ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension, ecs::EntityRegistry& registry);

    /**
     * @brief 保存区块内所有实体（批量写入）
     *
     * @param entities 实体引用列表
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @return 成功或错误
     */
    Result<void> saveEntitiesInChunk(const std::vector<std::reference_wrapper<Entity>>& entities,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        DimensionId dimension);

    /**
     * @brief 保存单个实体到存储
     *
     * 将实体序列化为 NBT 并写入 RocksDB。
     *
     * @param entity 实体引用
     * @param chunkX 存档归属区块 X
     * @param chunkZ 存档归属区块 Z
     * @param dimension 维度ID
     * @return 成功或错误
     *
     * @warning `chunkX/chunkZ` **必须**与调用方枚举该实体所用的区块列一致，且必须是
     *          deleteEntitiesInChunk 使用的同一坐标系：落键与"按区块前缀整段删除"共用一套
     *          坐标口径，二者只要出现分歧，被删的行与写入的行就会错位，留下永不清理的残留行。
     *          因此本方法不按实体自身位置推导区块——那样会在实体跨区块漂移时写出与删除口径
     *          不符的键。实体按区块列整体保存请用 saveEntitiesInChunk。
     */
    Result<void> saveEntity(const Entity& entity, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension);

    /**
     * @brief 删除区块内所有实体
     *
     * 使用 RocksDB 范围删除清除指定区块的所有实体。
     *
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @return 成功或错误
     */
    Result<void> deleteEntitiesInChunk(ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension);

private:
    /**
     * @brief 获取列族名
     * @param dimension 维度ID
     * @return 列族名
     */
    [[nodiscard]] static const char* _columnFamilyName(DimensionId dimension);

    /**
     * @brief 构建数据库键（二进制格式）
     */
    [[nodiscard]] static std::vector<u8> _makeKey(const EntityKey& key);

    /**
     * @brief 构建区块前缀键（用于范围查询）
     */
    [[nodiscard]] static std::vector<u8> _makeChunkPrefixKey(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 构建区块范围结束键
     */
    [[nodiscard]] static std::vector<u8> _makeChunkEndKey(ChunkCoord chunkX, ChunkCoord chunkZ);

    RocksDBDatabase& m_db;
};

} // namespace world::storage
} // namespace mc
