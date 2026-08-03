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

#include "BedrockColumnReader.hpp"
#include "BedrockLevelDb.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <optional>
#include <vector>

namespace mc::world::storage::reader::bedrock {

/**
 * @brief 基岩版世界读取器
 *
 * 提供读取基岩版世界存档的高级接口，封装了区块列读取器，
 * 用于读取完整的区块数据和列出世界中所有区块的位置。
 */
class BedrockWorldReader {
public:
    /**
     * @brief 构造函数
     * @param columnReader 区块列读取器引用，用于实际读取区块数据
     */
    explicit BedrockWorldReader(BedrockColumnReader& columnReader);

    /**
     * @brief 读取指定位置的区块数据
     *
     * @param x 区块X坐标
     * @param z 区块Z坐标
     * @param dimension 维度ID
     * @param db 基岩版LevelDB数据库
     * @return 成功时返回ChunkData（如果区块存在）或空optional（如果区块不存在），
     *         失败时返回错误信息
     */
    [[nodiscard]] Result<std::optional<ChunkData>> readChunk(
        ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db);

    /**
     * @brief 列出指定维度中的所有区块位置
     *
     * 遍历数据库中的所有键，找出属于指定维度的区块坐标。
     *
     * @param dimension 维度ID
     * @param db 基岩版LevelDB数据库
     * @return 成功时返回区块位置列表，失败时返回错误信息
     */
    [[nodiscard]] Result<std::vector<ChunkPos>> listChunks(DimensionId dimension, BedrockLevelDb& db);

private:
    BedrockColumnReader& m_columnReader;
};

} // namespace mc::world::storage::reader::bedrock
