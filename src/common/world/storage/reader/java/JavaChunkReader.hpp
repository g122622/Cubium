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
#include "common/util/NibbleArray.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <array>
#include <limits>
#include <optional>
#include <string>
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
    struct SectionBiomePalette {
        i32 sectionY = 0;
        std::vector<BiomeId> palette;
        std::vector<u32> indices;
    };

    explicit JavaChunkReader(JavaBlockStateMapper& blockMapper, JavaBiomeMapper& biomeMapper) noexcept;

    /**
     * @brief 从位压缩的 long 数组解包索引
     * @param data long 数组
     * @param bitsPerEntry 每个条目的位数
     * @param entryCount 条目数
     * @return 解包后的索引数组
     */
    static std::vector<u32> unpackLongArray(
        const std::vector<i64>& data, i32 bitsPerEntry, i32 entryCount, bool usePaddedFormat);

    /**
     * @brief 从 NBT compound 读取一个 Section
     * @param sectionNbt Section 的 NBT 数据
     * @param chunk 目标区块数据
     * @param sectionY Section 的 Y 坐标
     * @param hasSkyLight 该维度是否有天空光照
     */
    Result<void> readSection(
        const nbt::tags::compound_tag& sectionNbt, ChunkData& chunk, i32 sectionY, bool hasSkyLight);

    /**
     * @brief 读取高度图
     */
    void readHeightmaps(const nbt::tags::compound_tag& levelNbt, ChunkData& chunk);

    [[nodiscard]] BiomeId mapBiomeName(const std::string& biomeName) const;
    [[nodiscard]] BiomeId mapBiomeId(i32 biomeId) const;
    [[nodiscard]] Result<std::optional<SectionBiomePalette>> readSectionBiomePalette(
        const nbt::tags::compound_tag& sectionNbt) const;

private:
    static constexpr std::array<i32, 192> MAGIC = {-1,
        -1,
        0,
        std::numeric_limits<i32>::min(),
        0,
        0,
        1431655765,
        1431655765,
        0,
        std::numeric_limits<i32>::min(),
        0,
        1,
        858993459,
        858993459,
        0,
        715827882,
        715827882,
        0,
        613566756,
        613566756,
        0,
        std::numeric_limits<i32>::min(),
        0,
        2,
        477218588,
        477218588,
        0,
        429496729,
        429496729,
        0,
        390451572,
        390451572,
        0,
        357913941,
        357913941,
        0,
        330382099,
        330382099,
        0,
        306783378,
        306783378,
        0,
        286331153,
        286331153,
        0,
        std::numeric_limits<i32>::min(),
        0,
        3,
        252645135,
        252645135,
        0,
        238609294,
        238609294,
        0,
        226050910,
        226050910,
        0,
        214748364,
        214748364,
        0,
        204522252,
        204522252,
        0,
        195225786,
        195225786,
        0,
        186737708,
        186737708,
        0,
        178956970,
        178956970,
        0,
        171798691,
        171798691,
        0,
        165191049,
        165191049,
        0,
        159072862,
        159072862,
        0,
        153391689,
        153391689,
        0,
        148102320,
        148102320,
        0,
        143165576,
        143165576,
        0,
        138547332,
        138547332,
        0,
        std::numeric_limits<i32>::min(),
        0,
        4,
        130150524,
        130150524,
        0,
        126322567,
        126322567,
        0,
        122713351,
        122713351,
        0,
        119304647,
        119304647,
        0,
        116080197,
        116080197,
        0,
        113025455,
        113025455,
        0,
        110127366,
        110127366,
        0,
        107374182,
        107374182,
        0,
        104755299,
        104755299,
        0,
        102261126,
        102261126,
        0,
        99882960,
        99882960,
        0,
        97612893,
        97612893,
        0,
        95443717,
        95443717,
        0,
        93368854,
        93368854,
        0,
        91382282,
        91382282,
        0,
        89478485,
        89478485,
        0,
        87652393,
        87652393,
        0,
        85899345,
        85899345,
        0,
        84215045,
        84215045,
        0,
        82595524,
        82595524,
        0,
        81037118,
        81037118,
        0,
        79536431,
        79536431,
        0,
        78090314,
        78090314,
        0,
        76695844,
        76695844,
        0,
        75350303,
        75350303,
        0,
        74051160,
        74051160,
        0,
        72796055,
        72796055,
        0,
        71582788,
        71582788,
        0,
        70409299,
        70409299,
        0,
        69273666,
        69273666,
        0,
        68174084,
        68174084,
        0,
        std::numeric_limits<i32>::min(),
        0,
        5};

    static std::vector<u32> unpackCompactLongArray(const std::vector<i64>& data, i32 bitsPerEntry, i32 entryCount);
    static std::vector<u32> unpackPaddedLongArray(const std::vector<i64>& data, i32 bitsPerEntry, i32 entryCount);

    /**
     * @brief 读取方块状态调色板和数据
     */
    Result<void> readBlockStates(
        const nbt::tags::compound_tag& sectionNbt, ChunkData& chunk, i32 sectionY, bool hasSkyLight);

    /**
     * @brief 读取光照数据
     * @param sectionNbt Section 的 NBT 数据
     * @param section 目标区块段
     * @param hasSkyLight 该维度是否有天空光照，若无则跳过天空光照数据
     */
    void readLightData(const nbt::tags::compound_tag& sectionNbt, ChunkSection& section, bool hasSkyLight);

    JavaBlockStateMapper& m_blockMapper;
    JavaBiomeMapper& m_biomeMapper;
};

} // namespace mc::world::storage::reader::java
