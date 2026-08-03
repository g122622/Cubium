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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc {
class BlockPos;
}

namespace mc::world::storage {

// 前向声明
class RocksDBDatabase;

/**
 * @brief 方块实体存储管理器
 *
 * 负责方块实体的持久化存储，使用 RocksDB 列族。
 *
 * 存储格式：
 * - 键: {chunkX}:{chunkZ}:{x}:{y}:{z} (字符串)
 * - 值: gzip 压缩的 NBT 二进制数据 (Java 格式)
 *
 * 列族映射：
 * - block_entities_overworld: 主世界方块实体
 * - block_entities_nether: 下界方块实体
 * - block_entities_the_end: 末地方块实体
 */
class BlockEntityStorageManager {
public:
    /**
     * @brief 构造函数
     * @param db RocksDB 数据库引用
     */
    explicit BlockEntityStorageManager(RocksDBDatabase& db);

    ~BlockEntityStorageManager() = default;

    // 禁止拷贝
    BlockEntityStorageManager(const BlockEntityStorageManager&) = delete;
    BlockEntityStorageManager& operator=(const BlockEntityStorageManager&) = delete;

    // ========== 单方块实体操作 ==========

    /**
     * @brief 保存方块实体到存储
     * @param blockEntity 方块实体引用
     * @param dimension 维度ID
     * @return 成功或错误
     */
    Result<void> saveBlockEntity(const BlockEntity& blockEntity, DimensionId dimension);

    /**
     * @brief 从存储加载单个方块实体
     * @param pos 方块位置
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @return 方块实体实例或错误
     */
    Result<std::unique_ptr<BlockEntity>> loadBlockEntity(
        const BlockPos& pos, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension);

    /**
     * @brief 从存储删除方块实体
     * @param pos 方块位置
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @return 成功或错误
     */
    Result<void> deleteBlockEntity(const BlockPos& pos, ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension);

    // ========== 区块级操作 ==========

    /**
     * @brief 加载区块内所有方块实体
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @return 方块实体列表
     */
    Result<std::vector<std::unique_ptr<BlockEntity>>> loadBlockEntitiesInChunk(
        ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension);

    /**
     * @brief 保存区块内所有方块实体
     * @param blockEntities 方块实体引用列表
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @return 成功或错误
     */
    Result<void> saveBlockEntitiesInChunk(const std::vector<std::reference_wrapper<const BlockEntity>>& blockEntities,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        DimensionId dimension);

    /**
     * @brief 保存当前已加载的全部方块实体
     *
     * 用于统一全量保存入口。
     *
     * @param blockEntities 全部方块实体引用
     * @param dimension 维度ID
     * @return 成功保存数量或错误
     */
    Result<size_t> saveAllBlockEntities(
        const std::vector<std::reference_wrapper<const BlockEntity>>& blockEntities, DimensionId dimension);

    /**
     * @brief 删除区块内所有方块实体
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param dimension 维度ID
     * @return 成功或错误
     */
    Result<void> deleteBlockEntitiesInChunk(ChunkCoord chunkX, ChunkCoord chunkZ, DimensionId dimension);

private:
    [[nodiscard]] static const char* _columnFamilyName(DimensionId dimension);

    /**
     * @brief 构建方块实体键
     * 格式: {chunkX}:{chunkZ}:{x}:{y}:{z}
     */
    [[nodiscard]] static std::string _buildKey(const BlockPos& pos, ChunkCoord chunkX, ChunkCoord chunkZ);

    [[nodiscard]] static std::vector<u8> _makeKey(const std::string& key);
    [[nodiscard]] static std::vector<u8> _makeChunkPrefixKey(ChunkCoord chunkX, ChunkCoord chunkZ);
    [[nodiscard]] static std::vector<u8> _makeChunkEndKey(ChunkCoord chunkX, ChunkCoord chunkZ);

    RocksDBDatabase& m_db;
};

} // namespace mc::world::storage
