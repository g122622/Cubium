/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software of
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

#include "BedrockBiomeMapper.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::world::storage::reader::bedrock {

/**
 * @brief 基岩版子区块 / 调色板解码器
 *
 * 只负责局部 payload 解码，world / column / key 组织由外层 reader 负责。
 */
class BedrockChunkReader {
public:
    explicit BedrockChunkReader(BedrockBiomeMapper& biomeMapper);
    [[nodiscard]] Result<void> readSubChunk(const std::vector<u8>& data, i8 subChunkY, ChunkData& chunk);
    [[nodiscard]] Result<void> readData2D(const std::vector<u8>& data, ChunkData& chunk);
    [[nodiscard]] Result<void> readBiomeState(const std::vector<u8>& data, ChunkData& chunk);

private:
    [[nodiscard]] i32 _resolveSectionIndex(u8 version, i8 keySubChunkY, const std::vector<u8>& data, size_t& pos) const;

    struct BiomeSectionData {
        i32 sectionY = 0;
        std::array<BiomeId, BiomeContainer::SECTION_BIOME_SIZE> biomes{};
    };

    /**
     * @brief 读取子区块数据
     */
    Result<BiomeSectionData> _readBiomeSectionPalette(
        const std::vector<u8>& data, size_t& pos, i32 sectionY, DimensionId dimension) const;
    void _applyBiomeSectionsToChunk(const std::vector<BiomeSectionData>& sections, ChunkData& chunk) const;
    Result<u32> _readPaletteEntry(const std::vector<u8>& data, size_t& pos, bool isRuntimeEncoding);
    u32 _mapBlockState(const std::string& blockName, const std::unordered_map<std::string, std::string>& states);
    void _applyBlockPalette(ChunkSection& section,
        const std::vector<u32>& indices,
        const std::vector<u32>& paletteIds,
        bool isAuxiliaryLayer);

    BedrockBiomeMapper& m_biomeMapper;
    std::unordered_map<std::string, u32> m_blockStateCache;
};

} // namespace mc::world::storage::reader::bedrock
