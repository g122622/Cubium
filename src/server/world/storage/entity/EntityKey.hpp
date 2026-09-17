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

#include <string>

namespace mc {

class Entity; // 前向声明，Entity 在 mc 命名空间

namespace world::storage {

/**
 * @brief 实体存储键
 *
 * 用于在实体存储系统中唯一标识一个实体。键格式设计为支持按区块范围查询，
 * 这在区块加载/卸载时批量读取/写入实体数据非常有用。
 *
 * 键格式: {chunkX}:{chunkZ}:{uuid}
 * 例如: "12:-34:550e8400e29b41d4a716446655440000"
 *
 * 设计优势：
 * - 通过区块坐标前缀扫描获取该区块所有实体
 * - 通过 UUID 唯一标识每个实体
 * - 按字典序排列时同一区块的实体聚集在一起
 * - 支持高效的范围查询操作
 */
struct EntityKey {
    /// 实体所在区块的 X 坐标
    ChunkCoord chunkX = 0;

    /// 实体所在区块的 Z 坐标
    ChunkCoord chunkZ = 0;

    /// 实体的 UUID（32字符十六进制字符串）
    std::string uuid;

    /**
     * @brief 转换为字符串键
     * @return 格式化的字符串 "{chunkX}:{chunkZ}:{uuid}"
     */
    [[nodiscard]] std::string toString() const;

    /**
     * @brief 从字符串键解析
     * @param str 要解析的字符串
     * @return 解析成功返回 EntityKey，失败返回错误信息
     */
    [[nodiscard]] static Result<EntityKey> parse(const std::string& str);

    /**
     * @brief 从实体创建键
     * @param entity 实体引用
     * @return 包含实体位置和UUID的键
     */
    [[nodiscard]] static EntityKey fromEntity(const Entity& entity);

    /**
     * @brief 构建区块前缀，用于范围查询
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @return 格式化的前缀字符串 "{chunkX}:{chunkZ}:"
     */
    [[nodiscard]] static std::string buildChunkPrefix(ChunkCoord chunkX, ChunkCoord chunkZ);
};

} // namespace world::storage
} // namespace mc
