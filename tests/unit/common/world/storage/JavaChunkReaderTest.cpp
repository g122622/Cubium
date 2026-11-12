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

#include "server/world/storage/reader/java/JavaChunkReader.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "server/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include <gtest/gtest.h>

namespace mc::world::storage::reader::java {
namespace {

std::vector<i64> packPadded(const std::vector<u32>& values, i32 bitsPerEntry)
{
    const i32 valuesPerLong = 64 / bitsPerEntry;
    std::vector<i64> packed(static_cast<size_t>((values.size() + valuesPerLong - 1) / valuesPerLong), 0);
    for (size_t i = 0; i < values.size(); ++i) {
        const i32 longIndex = static_cast<i32>(i) / valuesPerLong;
        const i32 bitOffset = (static_cast<i32>(i) % valuesPerLong) * bitsPerEntry;
        packed[static_cast<size_t>(longIndex)] |= static_cast<i64>(values[i]) << bitOffset;
    }
    return packed;
}

std::vector<i64> packCompact(const std::vector<u32>& values, i32 bitsPerEntry)
{
    const size_t packedSize = static_cast<size_t>((static_cast<i64>(values.size()) * bitsPerEntry + 63) / 64);
    std::vector<i64> packed(packedSize, 0);
    const u64 mask = (1ULL << bitsPerEntry) - 1ULL;
    for (size_t i = 0; i < values.size(); ++i) {
        const i32 bitIndex = static_cast<i32>(i) * bitsPerEntry;
        const i32 startLong = bitIndex / 64;
        const i32 endLong = (bitIndex + bitsPerEntry - 1) / 64;
        const i32 startOffset = bitIndex % 64;
        packed[static_cast<size_t>(startLong)] |= static_cast<i64>((static_cast<u64>(values[i]) & mask) << startOffset);
        if (startLong != endLong) {
            const i32 spillBits = 64 - startOffset;
            packed[static_cast<size_t>(endLong)] |= static_cast<i64>((static_cast<u64>(values[i]) & mask) >> spillBits);
        }
    }
    return packed;
}

TEST(JavaChunkReaderTest, UnpackPaddedLongArrayMatchesJava116Layout)
{
    std::vector<u32> values(4096);
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<u32>(i % 17);
    }

    const auto packed = packPadded(values, 5);
    const auto unpacked = JavaChunkReader::unpackLongArray(packed, 5, 4096, true);
    EXPECT_EQ(unpacked, values);
}

TEST(JavaChunkReaderTest, UnpackCompactLongArrayMatchesCrossLongLayout)
{
    std::vector<u32> values(64);
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<u32>(i % 9);
    }

    const auto packed = packCompact(values, 4);
    const auto unpacked = JavaChunkReader::unpackLongArray(packed, 4, static_cast<i32>(values.size()), false);
    EXPECT_EQ(unpacked, values);
}

// ============================================================================
// 群系 palette 解包格式（1.21.11 群系段）
// ============================================================================

/**
 * 群系索引必须是 padded 格式：与 block_states 一样，palette 达 16 项以上时
 * 单 long 内可容纳的项数为 2 的幂，尾部高位弃用，元素不跨 long。
 *
 * 这里用 palette 5 项（bits=3）构造：该位宽是两种布局唯一不同的地方之一
 * （bits 为 1/2/4 时两种布局恰好重合，因此这类缺陷在多数区块上不可见）。
 * 若按 compact 解包，第 21 项起会因跨 long 拼接而读到错误的值甚至越界索引。
 */
TEST(JavaChunkReaderTest, BiomePaletteIsUnpackedAsPaddedFormat)
{
    constexpr i32 kCount = BiomeContainer::SECTION_BIOME_SIZE; // 64
    constexpr i32 kBits = 3;                                   // 5 项 palette
    std::vector<u32> values(kCount);
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<u32>(i % 5);
    }

    const auto packed = packPadded(values, kBits);
    // padded 布局：64 / 3 = 21 项/long，64 项需 ceil(64/21) = 4 个 long
    ASSERT_EQ(packed.size(), 4u);

    const auto asPadded = JavaChunkReader::unpackLongArray(packed, kBits, kCount, true);
    EXPECT_EQ(asPadded, values);

    // 反向断言：同一份数据按 compact 解释必然出错——这正是本次修复前的行为，
    // 保留该断言可防止日后有人"顺手统一"成 compact。
    const auto asCompact = JavaChunkReader::unpackLongArray(packed, kBits, kCount, false);
    EXPECT_NE(asCompact, values);
}

} // namespace

// ============================================================================
// 光照导入
// ============================================================================

namespace {

/// 按 Java 存档的 nibble 布局打包：元素 i 落在第 i/2 字节的第 (i&1)*4 位起 4 位。
std::vector<std::int8_t> packNibbles(const std::vector<std::uint8_t>& values)
{
    std::vector<std::int8_t> packed(2048, 0);
    for (size_t i = 0; i < values.size() && i < 4096; ++i) {
        const std::uint8_t nibble = static_cast<std::uint8_t>(values[i] & 0x0F);
        auto& byte = packed[i >> 1];
        byte = static_cast<std::int8_t>(
            (static_cast<std::uint8_t>(byte) & ~(0x0F << ((i & 1) * 4))) | (nibble << ((i & 1) * 4)));
    }
    return packed;
}

} // namespace

/**
 * @test 天空光/方块光按 nibble 布局逐值导入，且置 lightCorrect
 *
 * Java 存档的 SkyLight/BlockLight 是 2048 字节 nibble 数组，布局与 SWMRNibbleArray 一致。
 * 导入正确的直接收益是 ChunkLoadLightTask 可以跳过整块全量光照计算；导入错位则表现为
 * 光照错乱且不会被任何断言拦住，因此这里逐值核对。
 */
TEST(JavaChunkReaderTest, SectionLightIsImportedAndMarksLightCorrect)
{
    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader reader(blockMapper, biomeMapper);

    ChunkData chunk(static_cast<ChunkCoord>(0), static_cast<ChunkCoord>(0));

    // 造一段非平凡的光照：值随位置变化，能同时检验字节内高低半字节与元素顺序。
    std::vector<std::uint8_t> skyValues(4096);
    std::vector<std::uint8_t> blockValues(4096);
    for (size_t i = 0; i < 4096; ++i) {
        skyValues[i] = static_cast<std::uint8_t>(i % 16);
        blockValues[i] = static_cast<std::uint8_t>((i * 7 + 3) % 16);
    }

    nbt::tags::compound_tag section;
    section.put("Y", static_cast<i32>(-4)); // 主世界 y=-64 所在的段
    auto skyPacked = packNibbles(skyValues);
    auto blockPacked = packNibbles(blockValues);
    section.value.emplace("SkyLight", std::make_unique<nbt::tags::bytearray_tag>(skyPacked));
    section.value.emplace("BlockLight", std::make_unique<nbt::tags::bytearray_tag>(blockPacked));

    bool lightComplete = false;
    auto result = reader.readSection(section, chunk, -4, lightComplete);
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(lightComplete) << "a section with both light arrays must report complete light";

    const auto& sky = chunk.skyNibbleAt(0);
    const auto& block = chunk.blockNibbleAt(0);
    for (i32 i = 0; i < 4096; ++i) {
        ASSERT_EQ(sky.getUpdating(i), skyValues[static_cast<size_t>(i)]) << "sky index " << i;
        ASSERT_EQ(block.getUpdating(i), blockValues[static_cast<size_t>(i)]) << "block index " << i;
    }
}

/**
 * @test 段缺光照时必须报告不完整
 *
 * 只要有一个段缺光照就整块重算：若误报完整，ChunkLoadLightTask 会走"只重载边缘"的
 * 快路径，缺光的那一段会整段全黑，且不会自行恢复。
 */
TEST(JavaChunkReaderTest, SectionWithoutLightReportsIncomplete)
{
    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader reader(blockMapper, biomeMapper);

    ChunkData chunk(static_cast<ChunkCoord>(0), static_cast<ChunkCoord>(0));

    nbt::tags::compound_tag section;
    section.put("Y", static_cast<i32>(-4));

    bool lightComplete = true;
    auto result = reader.readSection(section, chunk, -4, lightComplete);
    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_FALSE(lightComplete) << "a section without SkyLight/BlockLight must not claim complete light";
}

/**
 * @test 光照数组长度异常时降级为"无光照"而不是让区块加载失败
 *
 * 光照不是地形数据：字段损坏时应当重算光照，而不是拒绝整个区块——否则一个损坏的
 * 光照字段会让玩家永远进不去那个区块。
 */
TEST(JavaChunkReaderTest, MalformedLightArrayDegradesToRecompute)
{
    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader reader(blockMapper, biomeMapper);

    ChunkData chunk(static_cast<ChunkCoord>(0), static_cast<ChunkCoord>(0));

    nbt::tags::compound_tag section;
    section.put("Y", static_cast<i32>(-4));
    std::vector<std::int8_t> shortArray(100, 0);
    section.value.emplace("SkyLight", std::make_unique<nbt::tags::bytearray_tag>(shortArray));

    bool lightComplete = true;
    auto result = reader.readSection(section, chunk, -4, lightComplete);
    EXPECT_TRUE(result.success()) << "malformed light must not fail the whole chunk load";
    EXPECT_FALSE(lightComplete);
}

/**
 * @test 全零光照走 Zero 快路径且数值正确
 */
TEST(JavaChunkReaderTest, AllZeroLightIsImportedAsZero)
{
    JavaBlockStateMapper blockMapper;
    JavaBiomeMapper biomeMapper;
    JavaChunkReader reader(blockMapper, biomeMapper);

    ChunkData chunk(static_cast<ChunkCoord>(0), static_cast<ChunkCoord>(0));

    nbt::tags::compound_tag section;
    section.put("Y", static_cast<i32>(-4));
    std::vector<std::int8_t> zeros(2048, 0);
    section.value.emplace("BlockLight", std::make_unique<nbt::tags::bytearray_tag>(zeros));
    section.value.emplace("SkyLight", std::make_unique<nbt::tags::bytearray_tag>(zeros));

    bool lightComplete = false;
    auto result = reader.readSection(section, chunk, -4, lightComplete);
    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(lightComplete);

    const auto& block = chunk.blockNibbleAt(0);
    for (i32 i = 0; i < 4096; ++i) {
        ASSERT_EQ(block.getUpdating(i), 0) << "index " << i;
    }
}
} // namespace mc::world::storage::reader::java
