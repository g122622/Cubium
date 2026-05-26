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

#include "JavaBiomeMapper.hpp"
#include "JavaBlockStateMapper.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include <memory>
#include <vector>

namespace mc::nbt::tags {
struct compound_tag;
struct list_tag;
} // namespace mc::nbt::tags

namespace mc::world::storage::reader::java {

/**
 * @brief Java 版区块 NBT 解析器
 *
 * 解析 Java 1.16.5 区块 NBT 结构，转换为项目的 ChunkData。
 * 支持：
 * - 区块 Sections（palette + long array 方块状态）
 * - Biomes（1.16.5 格式：int[1024]）
 * - Heightmaps（long array）
 * - BlockEntities
 * - 光照数据
 */
class JavaChunkReader {
public:
    explicit JavaChunkReader(JavaBlockStateMapper& blockMapper, JavaBiomeMapper& biomeMapper);

    /**
     * @brief 从 NBT 字节流解析区块数据
     * @param nbtData 已解压的 NBT 字节流
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param dimension 维度 ID
     * @return 区块数据，解析失败返回错误
     */
    Result<std::unique_ptr<ChunkData>> readChunk(
        const std::vector<u8>& nbtData, ChunkCoord x, ChunkCoord z, DimensionId dimension);

    /**
     * @brief 从位压缩的 long 数组解包索引
     * @param data long 数组
     * @param bitsPerEntry 每个条目的位数
     * @param entryCount 条目数
     * @return 解包后的索引数组
     */
    static std::vector<u32> unpackLongArray(const std::vector<i64>& data, i32 bitsPerEntry, i32 entryCount);

    /**
     * @brief 读取 Java 1.16.5 的 int[1024] 生物群系数组并投影到当前 ChunkData
     * @param levelNbt 含 `Biomes` 字段的 NBT 复合标签
     * @param chunk 目标区块
     * @return 成功或错误
     */
    Result<void> readBiomes(const nbt::tags::compound_tag& levelNbt, ChunkData& chunk);

private:
    /**
     * @brief 从 NBT compound 读取一个 Section
     */
    Result<void> readSection(const nbt::tags::compound_tag& sectionNbt, ChunkData& chunk, i32 sectionY);

    /**
     * @brief 读取方块状态调色板和数据
     */
    Result<void> readBlockStates(const nbt::tags::compound_tag& sectionNbt, ChunkData& chunk, i32 sectionY);

    /**
     * @brief 读取高度图
     */
    void readHeightmaps(const nbt::tags::compound_tag& levelNbt, ChunkData& chunk);

    /**
     * @brief 读取光照数据
     */
    void readLightData(const nbt::tags::compound_tag& sectionNbt, ChunkSection& section);

    JavaBlockStateMapper& m_blockMapper;
    JavaBiomeMapper& m_biomeMapper;
};

} // namespace mc::world::storage::reader::java
