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
#include "BedrockLevelDb.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::world::storage::reader::bedrock {

/**
 * @brief 基岩版区块调色板解析器
 *
 * 解析基岩版的子区块数据（SubChunkPrefix，键类型 47）。
 * 基岩版使用 32 位 int 调色板压缩（不同于 Java 的 64 位 long）。
 */
class BedrockChunkReader {
public:
    explicit BedrockChunkReader(BedrockBiomeMapper& biomeMapper);

    /**
     * @brief 从 LevelDB 读取完整区块
     * @param chunkX 区块 X
     * @param chunkZ 区块 Z
     * @param dimension 维度 ID
     * @param db LevelDB 实例
     * @return 区块数据，不存在返回空 optional
     */
    Result<std::unique_ptr<ChunkData>> readChunk(i32 chunkX, i32 chunkZ, DimensionId dimension, BedrockLevelDb& db);

    /**
     * @brief 读取 LEB128 无符号变长整数
     */
    Result<u32> readVarUint(const std::vector<u8>& data, size_t& pos) const;

    /**
     * @brief 从缓冲区读取单个 Bedrock 调色板段
     */
    Result<std::vector<u32>> readPackedIndices(
        const std::vector<u8>& data, size_t& pos, i32 bitsPerEntry, i32 entryCount, i32 wordBits) const;

private:
    struct BiomeSectionData {
        i32 sectionY = 0;
        std::array<BiomeId, BiomeContainer::BIOME_SIZE> biomes{};
    };

    /**
     * @brief 读取子区块数据
     */
    Result<void> readSubChunk(const std::vector<u8>& data, i8 subChunkY, ChunkData& chunk);

    /**
     * @brief 读取 DATA_2D（旧版生物群系 + 高度图）
     */
    Result<void> readData2D(const std::vector<u8>& data, ChunkData& chunk);

    /**
     * @brief 读取 BiomeState（新版生物群系，1.18+）
     */
    Result<void> readBiomeState(const std::vector<u8>& data, ChunkData& chunk);

    /**
     * @brief 读取生物群系 section 调色板
     */
    Result<BiomeSectionData> readBiomeSectionPalette(
        const std::vector<u8>& data, size_t& pos, i32 sectionY, DimensionId dimension) const;

    /**
     * @brief 将 section 级生物群系投影到当前 ChunkData 可表达的 4x4x4 容器
     */
    void applyBiomeSectionsToChunk(const std::vector<BiomeSectionData>& sections, ChunkData& chunk) const;

    /**
     * @brief 读取单个 Bedrock 调色板条目
     */
    Result<u32> readPaletteEntry(const std::vector<u8>& data, size_t& pos, bool isRuntimeEncoding);

    /**
     * @brief 将 Bedrock 方块名和状态映射到内部 stateId
     */
    u32 mapBlockState(const std::string& blockName, const std::unordered_map<std::string, std::string>& states);

    /**
     * @brief 从调色板索引数组填充 ChunkSection 方块状态
     */
    void applyBlockPalette(ChunkSection& section,
        const std::vector<u32>& indices,
        const std::vector<u32>& paletteIds,
        bool isAuxiliaryLayer);

    BedrockBiomeMapper& m_biomeMapper;
    std::unordered_map<std::string, u32> m_blockStateCache;
};

} // namespace mc::world::storage::reader::bedrock
