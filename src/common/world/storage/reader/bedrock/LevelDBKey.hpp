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

#include "common/core/Types.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace mc::world::storage::reader::bedrock {

class LevelDBKey {
public:
    enum class ChunkType : u8 {
        Data3D = 43,
        Version = 44,
        Data2D = 45,
        Data2DLegacy = 46,
        SubChunkPrefix = 47,
        LegacyTerrain = 48,
        BlockEntity = 49,
        Entity = 50,
        PendingTicks = 51,
        BlockExtraData = 52,
        BiomeState = 53,
        FinalizedState = 54,
        BorderBlocks = 55,
        HardCodedDecorations = 56,
    };

    [[nodiscard]] static bool startsWith(const std::vector<u8>& input, const std::vector<u8>& prefix);
    [[nodiscard]] static std::string extractSuffix(const std::vector<u8>& input, const std::vector<u8>& prefix);

    [[nodiscard]] static std::vector<u8> key(DimensionId dimension, const ChunkPos& pos, ChunkType type);
    [[nodiscard]] static std::vector<u8> key(DimensionId dimension, const ChunkPos& pos, i8 y, ChunkType type);
    [[nodiscard]] static std::vector<u8> key(const std::vector<u8>& prefix, DimensionId dimension, const ChunkPos& pos);
    [[nodiscard]] static std::vector<u8> key(const std::vector<u8>& prefix, std::string_view suffix);
    [[nodiscard]] static std::vector<u8> chunkPrefix(DimensionId dimension, const ChunkPos& pos);

    [[nodiscard]] static const std::vector<u8>& actorPrefix();
    [[nodiscard]] static const std::vector<u8>& biomeIdsTable();
    [[nodiscard]] static const std::vector<u8>& digpPrefix();
    [[nodiscard]] static const std::vector<u8>& dimensionNameIdTable();
    [[nodiscard]] static const std::vector<u8>& localPlayer();
    [[nodiscard]] static const std::vector<u8>& mapPrefix();
    [[nodiscard]] static const std::vector<u8>& portals();
    [[nodiscard]] static const std::vector<u8>& posTrackDb();
    [[nodiscard]] static const std::vector<u8>& posTrackDbLastId();
};

} // namespace mc::world::storage::reader::bedrock
