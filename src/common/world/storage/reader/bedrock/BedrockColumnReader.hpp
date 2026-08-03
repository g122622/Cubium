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

#include "BedrockChunkReader.hpp"
#include "BedrockLevelDb.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <memory>

namespace mc::world::storage::reader::bedrock {

/**
 * @brief 基岩版区块列读取器
 *
 * 负责从LevelDB数据库中读取完整的区块列（Column）数据，
 * 包括所有子区块和生物群系/高度数据。
 */
class BedrockColumnReader {
public:
    explicit BedrockColumnReader(BedrockChunkReader& chunkReader);

    /**
     * @brief 读取完整的区块列数据
     *
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @param dimension 维度ID
     * @param db LevelDB数据库实例
     * @return 成功返回ChunkData，区块不存在返回空指针，失败返回错误
     */
    [[nodiscard]] Result<std::unique_ptr<ChunkData>> readColumn(
        ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db);

private:
    /**
     * @brief 读取所有子区块数据
     */
    [[nodiscard]] Result<void> _readSubChunks(
        ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db, ChunkData& chunk);

    /**
     * @brief 读取生物群系和高度数据
     */
    [[nodiscard]] Result<void> _readBiomeAndHeight(
        ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db, ChunkData& chunk);

    BedrockChunkReader& m_chunkReader;
};

} // namespace mc::world::storage::reader::bedrock
