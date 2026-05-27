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

} // namespace
} // namespace mc::world::storage::reader::java
