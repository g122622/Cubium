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

#include "BedrockWorldReader.hpp"
#include "LevelDBKey.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/storage/reader/bedrock/BedrockColumnReader.hpp"
#include "common/world/storage/reader/bedrock/BedrockLevelDb.hpp"
#include <cstddef>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc::world::storage::reader::bedrock {

BedrockWorldReader::BedrockWorldReader(BedrockColumnReader& columnReader)
    : m_columnReader(columnReader)
{}

Result<std::optional<ChunkData>> BedrockWorldReader::readChunk(
    ChunkCoord x, ChunkCoord z, DimensionId dimension, BedrockLevelDb& db)
{
    auto chunkResult = m_columnReader.readColumn(x, z, dimension, db);
    if (chunkResult.failed()) {
        return chunkResult.error();
    }
    if (!chunkResult.value()) {
        return std::optional<ChunkData>{};
    }
    return std::optional<ChunkData>(std::move(*chunkResult.value()));
}

Result<std::vector<ChunkPos>> BedrockWorldReader::listChunks(DimensionId dimension, BedrockLevelDb& db)
{
    std::vector<ChunkPos> chunks;
    std::unordered_set<u64> seen;

    // 从字节数组读取小端序32位整数
    auto readLe32 = [](const std::vector<u8>& key, size_t offset) -> i32 {
        return static_cast<i32>(key[offset]) | (static_cast<i32>(key[offset + 1]) << 8) |
            (static_cast<i32>(key[offset + 2]) << 16) | (static_cast<i32>(key[offset + 3]) << 24);
    };

    // 遍历数据库中所有键，查找版本键以确定存在的区块
    // 基岩版 LevelDB 键格式：
    // - 主世界: [chunkX(4字节)][chunkZ(4字节)][type(1字节)] = 9字节
    // - 其他维度: [chunkX(4字节)][chunkZ(4字节)][dimension(4字节)][type(1字节)] = 13字节
    auto iterateResult = db.iteratePrefix(std::vector<u8>{},
        [dimension, &chunks, &seen, &readLe32](const std::vector<u8>& key, const std::vector<u8>& value) -> bool {
            MC_UNUSED(value);

            const auto versionType = static_cast<u8>(LevelDBKey::ChunkType::Version);
            if (dimension == 0) {
                // 主世界键长度为9字节
                if (key.size() != 9 || key[8] != versionType) {
                    return true;
                }
            } else {
                // 其他维度键长度为13字节，且维度ID位于偏移8处
                if (key.size() != 13 || key[12] != versionType) {
                    return true;
                }
                if (readLe32(key, 8) != dimension) {
                    return true;
                }
            }

            // 提取区块坐标并去重
            const i32 chunkX = readLe32(key, 0);
            const i32 chunkZ = readLe32(key, 4);
            const u64 packed = (static_cast<u64>(static_cast<u32>(chunkX)) << 32) | static_cast<u32>(chunkZ);
            if (seen.insert(packed).second) {
                chunks.emplace_back(chunkX, chunkZ);
            }
            return true;
        });
    if (iterateResult.failed()) {
        return iterateResult.error();
    }

    return chunks;
}

} // namespace mc::world::storage::reader::bedrock
