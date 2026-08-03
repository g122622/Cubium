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

#include "BedrockColumnReader.hpp"
#include "BedrockConstants.hpp"
#include "LevelDBKey.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/storage/reader/bedrock/BedrockChunkReader.hpp"
#include "common/world/storage/reader/bedrock/BedrockLevelDb.hpp"
#include <memory>

namespace mc::world::storage::reader::bedrock {

BedrockColumnReader::BedrockColumnReader(BedrockChunkReader& chunkReader)
    : m_chunkReader(chunkReader)
{}

Result<std::unique_ptr<ChunkData>> BedrockColumnReader::readColumn(
    ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db)
{
    auto chunk = std::make_unique<ChunkData>(x, z);

    auto subChunkResult = _readSubChunks(x, z, dimension, db, *chunk);
    if (subChunkResult.failed()) {
        return subChunkResult.error();
    }
    if (!chunk->isLoaded()) {
        // 区块不存在，返回空指针
        return std::unique_ptr<ChunkData>{};
    }

    auto biomeResult = _readBiomeAndHeight(x, z, dimension, db, *chunk);
    if (biomeResult.failed()) {
        return biomeResult.error();
    }

    chunk->setFullyGenerated(true);
    chunk->setDirty(false);
    return chunk;
}

Result<void> BedrockColumnReader::_readSubChunks(
    ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db, ChunkData& chunk)
{
    // 基岩版子区块索引范围由 BEDROCK_MIN_SUB_CHUNK_Y 和 BEDROCK_MAX_SUB_CHUNK_Y 定义
    // 对应 Y 坐标范围 -1024 到 1023（每个子区块高度 16）
    bool hasAnySection = false;
    for (i8 subY = BEDROCK_MIN_SUB_CHUNK_Y; subY < BEDROCK_MAX_SUB_CHUNK_Y; ++subY) {
        auto subKey = LevelDBKey::key(dimension, ChunkPos(x, z), subY, LevelDBKey::ChunkType::SubChunkPrefix);
        auto subResult = db.get(subKey);
        if (subResult.failed()) {
            continue;
        }
        if (!subResult.value().has_value()) {
            continue;
        }

        auto sectionResult = m_chunkReader.readSubChunk(subResult.value().value(), subY, chunk);
        if (sectionResult.failed()) {
            // 子区块读取失败，跳过继续处理其他子区块
            continue;
        }
        hasAnySection = true;
    }

    if (hasAnySection) {
        chunk.setLoaded(true);
    }
    return {};
}

Result<void> BedrockColumnReader::_readBiomeAndHeight(
    ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db, ChunkData& chunk)
{
    // 优先尝试读取新版本的 BiomeState 数据
    auto biomeStateKey = LevelDBKey::key(dimension, ChunkPos(x, z), LevelDBKey::ChunkType::BiomeState);
    auto biomeStateResult = db.get(biomeStateKey);
    if (biomeStateResult.success() && biomeStateResult.value().has_value()) {
        auto biomeResult = m_chunkReader.readBiomeState(biomeStateResult.value().value(), chunk);
        if (biomeResult.failed()) {
            // BiomeState 读取失败，回退到 Data2D
        } else {
            return {};
        }
    }

    // 回退读取旧版本的 Data2D 数据
    auto data2DKey = LevelDBKey::key(dimension, ChunkPos(x, z), LevelDBKey::ChunkType::Data2D);
    auto data2DResult = db.get(data2DKey);
    if (data2DResult.success() && data2DResult.value().has_value()) {
        auto result = m_chunkReader.readData2D(data2DResult.value().value(), chunk);
        if (result.failed()) {
            // Data2D 读取失败，返回错误
            return result.error();
        }
    }

    return {};
}

} // namespace mc::world::storage::reader::bedrock
