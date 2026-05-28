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
#include "LevelDBKey.hpp"
#include <spdlog/spdlog.h>

namespace mc::world::storage::reader::bedrock {

BedrockColumnReader::BedrockColumnReader(BedrockChunkReader& chunkReader)
    : m_chunkReader(chunkReader)
{}

Result<std::unique_ptr<ChunkData>> BedrockColumnReader::readColumn(
    ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db)
{
    auto chunk = std::make_unique<ChunkData>(x, z);

    auto subChunkResult = readSubChunks(x, z, dimension, db, *chunk);
    if (subChunkResult.failed()) {
        return subChunkResult.error();
    }
    if (!chunk->isLoaded()) {
        return std::unique_ptr<ChunkData>{};
    }

    auto biomeResult = readBiomeAndHeight(x, z, dimension, db, *chunk);
    if (biomeResult.failed()) {
        return biomeResult.error();
    }

    chunk->setFullyGenerated(true);
    chunk->setDirty(false);
    return chunk;
}

Result<void> BedrockColumnReader::readSubChunks(
    ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db, ChunkData& chunk)
{
    bool hasAnySection = false;
    for (i8 subY = -64; subY < 64; ++subY) {
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
            spdlog::debug("BedrockColumnReader: Failed to read sub-chunk {} for ({}, {}): {}",
                subY,
                x,
                z,
                sectionResult.error().message());
            continue;
        }
        hasAnySection = true;
    }

    if (hasAnySection) {
        chunk.setLoaded(true);
    }
    return {};
}

Result<void> BedrockColumnReader::readBiomeAndHeight(
    ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db, ChunkData& chunk)
{
    auto biomeStateKey = LevelDBKey::key(dimension, ChunkPos(x, z), LevelDBKey::ChunkType::BiomeState);
    auto biomeStateResult = db.get(biomeStateKey);
    if (biomeStateResult.success() && biomeStateResult.value().has_value()) {
        auto biomeResult = m_chunkReader.readBiomeState(biomeStateResult.value().value(), chunk);
        if (biomeResult.failed()) {
            spdlog::debug("BedrockColumnReader: Failed to read biome state for ({}, {})", x, z);
        }
        return {};
    }

    auto data2DKey = LevelDBKey::key(dimension, ChunkPos(x, z), LevelDBKey::ChunkType::Data2D);
    auto data2DResult = db.get(data2DKey);
    if (data2DResult.success() && data2DResult.value().has_value()) {
        auto biomeResult = m_chunkReader.readData2D(data2DResult.value().value(), chunk);
        if (biomeResult.failed()) {
            spdlog::debug("BedrockColumnReader: Failed to read Data2D for ({}, {})", x, z);
        }
    }

    return {};
}

} // namespace mc::world::storage::reader::bedrock
