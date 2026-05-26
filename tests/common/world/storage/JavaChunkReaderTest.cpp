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

#include "common/world/storage/reader/java/JavaChunkReader.hpp"

#include "common/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "common/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include <gtest/gtest.h>

namespace mc::world::storage::reader::java {
namespace {

class JavaChunkReaderTest : public ::testing::Test {
protected:
    JavaBiomeMapper biomeMapper;
    JavaBlockStateMapper blockMapper;
    JavaChunkReader reader{blockMapper, biomeMapper};
};

TEST_F(JavaChunkReaderTest, UnpackLongArrayReadsMultipleLongs)
{
    constexpr i32 bitsPerEntry = 4;
    constexpr i32 entryCount = 32;
    constexpr i32 valuesPerLong = 64 / bitsPerEntry;
    std::vector<i64> packed(2, 0);

    for (i32 i = 0; i < entryCount; ++i) {
        const i32 longIndex = i / valuesPerLong;
        const i32 bitOffset = (i % valuesPerLong) * bitsPerEntry;
        packed[static_cast<size_t>(longIndex)] |= static_cast<i64>((i % 16) << bitOffset);
    }

    auto unpacked = JavaChunkReader::unpackLongArray(packed, bitsPerEntry, entryCount);
    ASSERT_EQ(unpacked.size(), static_cast<size_t>(entryCount));
    for (i32 i = 0; i < entryCount; ++i) {
        EXPECT_EQ(unpacked[static_cast<size_t>(i)], static_cast<u32>(i % 16));
    }
}

TEST_F(JavaChunkReaderTest, ReadBiomesSamplesBottomSectionAgainstCurrentContainerShape)
{
    mc::nbt::tags::compound_tag level;
    std::vector<i32> biomes(1024, 0);

    for (i32 sy = 0; sy < BiomeContainer::BIOME_HEIGHT; ++sy) {
        for (i32 sz = 0; sz < BiomeContainer::BIOME_DEPTH; ++sz) {
            for (i32 sx = 0; sx < BiomeContainer::BIOME_WIDTH; ++sx) {
                const i32 globalY = sy * 4;
                const i32 srcIdx = (globalY / 4) * 16 + sz * 4 + sx;
                biomes[static_cast<size_t>(srcIdx)] = 1 + sy * 16 + sz * 4 + sx;
            }
        }
    }
    level.put("Biomes", biomes);

    ChunkData chunk(0, 0);
    auto result = reader.readBiomes(level, chunk);
    ASSERT_TRUE(result.success());

    for (i32 sy = 0; sy < BiomeContainer::BIOME_HEIGHT; ++sy) {
        for (i32 sz = 0; sz < BiomeContainer::BIOME_DEPTH; ++sz) {
            for (i32 sx = 0; sx < BiomeContainer::BIOME_WIDTH; ++sx) {
                const BiomeId expected = static_cast<BiomeId>(1 + sy * 16 + sz * 4 + sx);
                EXPECT_EQ(chunk.getBiomes().getBiome(sx, sy, sz), expected);
            }
        }
    }
}

} // namespace
} // namespace mc::world::storage::reader::java
