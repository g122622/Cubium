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

// macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "server/world/storage/db/SectionCodec.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Types.hpp"
#include "server/world/storage/db/SectionKey.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <gtest/gtest.h>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

namespace mc::world::storage {
namespace {

// 测试用生物群系 ID
namespace TestBiomes {
constexpr BiomeId PLAINS = 1;
constexpr BiomeId DESERT = 2;
constexpr BiomeId FOREST = 4;
} // namespace TestBiomes

class SectionKeyTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SectionKeyTest, Construction)
{
    SectionKey key(10, 20, 5, 0); // Overworld
    EXPECT_EQ(key.chunkX, 10);
    EXPECT_EQ(key.chunkZ, 20);
    EXPECT_EQ(key.sectionY, 5);
    EXPECT_EQ(key.dimension, 0);
}

TEST_F(SectionKeyTest, NegativeCoordinates)
{
    SectionKey key(-100, -200, 0, 1); // Nether
    EXPECT_EQ(key.chunkX, -100);
    EXPECT_EQ(key.chunkZ, -200);
    EXPECT_EQ(key.sectionY, 0);
    EXPECT_EQ(key.dimension, 1);
}

TEST_F(SectionKeyTest, SerializeDeserialize)
{
    SectionKey original(123, -456, 10, 2); // The End
    auto serialized = original.serialize();

    EXPECT_EQ(serialized.size(), 13u);

    SectionKey deserialized = SectionKey::deserialize(serialized.data());
    EXPECT_EQ(deserialized.chunkX, original.chunkX);
    EXPECT_EQ(deserialized.chunkZ, original.chunkZ);
    EXPECT_EQ(deserialized.sectionY, original.sectionY);
    EXPECT_EQ(deserialized.dimension, original.dimension);
}

TEST_F(SectionKeyTest, AllDimensions)
{
    for (i32 dim = 0; dim <= 2; ++dim) {
        SectionKey key(0, 0, 0, dim);
        auto serialized = key.serialize();
        SectionKey deserialized = SectionKey::deserialize(serialized.data());
        EXPECT_EQ(deserialized.dimension, dim);
    }
}

TEST_F(SectionKeyTest, SectionYRange)
{
    // Y range: -4 to 19 (for -64 to 320)
    SectionKey key1(0, 0, -4, 0);
    EXPECT_EQ(key1.sectionY, -4);

    SectionKey key2(0, 0, 19, 0);
    EXPECT_EQ(key2.sectionY, 19);
}

class SectionCodecTest : public ::testing::Test {
protected:
    void SetUp() override { mc::VanillaBlocks::initialize(); }
};

class _capturedValue final : public rocksdb::WriteBatch::Handler {
public:
    std::string bytes;
    rocksdb::Status PutCF(uint32_t, const rocksdb::Slice&, const rocksdb::Slice& value) override
    {
        bytes.assign(value.data(), value.size());
        return rocksdb::Status::OK();
    }
};

std::vector<u8> _encode(const ChunkData& chunk, i32 index)
{
    rocksdb::WriteBatch batch;
    const SectionKey key(chunk.x(), chunk.z(), static_cast<i8>(world::sectionIndexToCoord(index)), 0);
    auto result = SectionCodec::appendToBatch(batch, nullptr, key, chunk, index);
    EXPECT_TRUE(result.success()) << result.error().message();
    _capturedValue capture;
    EXPECT_TRUE(batch.Iterate(&capture).ok());
    return {capture.bytes.begin(), capture.bytes.end()};
}

void _setWord(std::vector<u8>& bytes, size_t offset, u32 value)
{
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

TEST_F(SectionCodecTest, PreservesCompactArraysAcrossWidthsAndAllowsFurtherMutation)
{
    for (u32 values : {1u, 2u, 3u, 7u, 16u, 17u, 65u, 4096u}) {
        SCOPED_TRACE(values);
        ChunkData source(-12, 34);
        auto* original = source.createSection(0);
        original->fill(100);
        for (i32 i = 0; i < ChunkSection::VOLUME; ++i) {
            original->setBlockStateIdFast(i, 100 + static_cast<u32>(i) % values);
        }
        const auto packed = original->blockStates().packedView();
        auto bytes = _encode(source, 0);
        EXPECT_EQ(bytes.size(),
            8 * sizeof(u32) + packed.palette.size_bytes() + packed.storage.size_bytes() + packed.hashMap.size_bytes() +
                BiomeContainer::SECTION_BIOME_SIZE * sizeof(BiomeId));
        // 缓冲区故意错开一个字节，验证恢复不依赖数据库返回地址的对齐。
        bytes.insert(bytes.begin(), 0);
        ChunkData restored(-12, 34);
        auto result = SectionCodec::restore(std::span<const u8>(bytes).subspan(1), restored, 0);
        ASSERT_TRUE(result.success()) << result.error().message();
        auto* actual = restored.getSection(0);
        ASSERT_NE(actual, nullptr);
        const auto actualPacked = actual->blockStates().packedView();
        EXPECT_TRUE(std::ranges::equal(packed.palette, actualPacked.palette));
        EXPECT_TRUE(std::ranges::equal(packed.storage, actualPacked.storage));
        EXPECT_TRUE(std::ranges::equal(packed.hashMap, actualPacked.hashMap));
        EXPECT_EQ(original->getBlockCount(), actual->getBlockCount());
        EXPECT_EQ(original->blockTickRefCount(), actual->blockTickRefCount());
        EXPECT_EQ(original->fluidRefCount(), actual->fluidRefCount());
        for (i32 i = 0; i < ChunkSection::VOLUME; ++i) {
            EXPECT_EQ(actual->getBlockStateIdFast(i), original->getBlockStateIdFast(i));
        }
        const i32 before = actual->blockStates().paletteSize();
        actual->setBlockStateIdFast(0, 100);
        EXPECT_EQ(actual->blockStates().paletteSize(), before);
        actual->setBlockStateIdFast(1, 50000);
        EXPECT_EQ(actual->getBlockStateIdFast(1), 50000u);
        EXPECT_EQ(actual->blockStates().paletteSize(), before + 1);
    }
}

TEST_F(SectionCodecTest, UniformAirVariantsRemainExact)
{
    for (u32 state : {0u, 1u, 2u}) {
        ChunkData source(0, 0);
        source.createSection(0)->fill(state);
        auto bytes = _encode(source, 0);
        ChunkData restored(0, 0);
        ASSERT_TRUE(SectionCodec::restore(bytes, restored, 0).success());
        EXPECT_TRUE(restored.getSection(0)->isEmpty());
        EXPECT_EQ(restored.getSection(0)->getBlockStateIdFast(2048), state);
        EXPECT_TRUE(restored.getSection(0)->blockStates().packedView().hashMap.empty());
    }
}

TEST_F(SectionCodecTest, BiomesAreSavedPerSectionWithoutFlatteningWholeChunk)
{
    ChunkData source(0, 0);
    ChunkData restored(0, 0);
    for (i32 index : {0, world::CHUNK_SECTIONS - 1}) {
        source.createSection(index)->fill(100);
        auto biomes = source.getBiomes().sectionData(index);
        for (size_t i = 0; i < biomes.size(); ++i) {
            biomes[i] = static_cast<BiomeId>(index * 100 + static_cast<i32>(i));
        }
        auto bytes = _encode(source, index);
        ASSERT_TRUE(SectionCodec::restore(bytes, restored, index).success());
        EXPECT_TRUE(std::ranges::equal(source.getBiomes().sectionData(index), restored.getBiomes().sectionData(index)));
    }
}

TEST_F(SectionCodecTest, BatchOwnsSnapshotAfterSourceChanges)
{
    ChunkData source(0, 0);
    source.createSection(0)->fill(100);
    rocksdb::WriteBatch batch;
    ASSERT_TRUE(
        SectionCodec::appendToBatch(batch, nullptr, SectionKey(0, 0, world::MIN_SECTION_Y, 0), source, 0).success());
    source.getSection(0)->fill(101);
    _capturedValue capture;
    ASSERT_TRUE(batch.Iterate(&capture).ok());
    ChunkData restored(0, 0);
    ASSERT_TRUE(
        SectionCodec::restore({reinterpret_cast<const u8*>(capture.bytes.data()), capture.bytes.size()}, restored, 0)
            .success());
    EXPECT_EQ(restored.getSection(0)->getBlockStateIdFast(0), 100u);
}

TEST_F(SectionCodecTest, RejectsInvalidHeadersTruncationAndTrailingBytes)
{
    ChunkData source(0, 0);
    source.createSection(0)->fill(100);
    const auto bytes = _encode(source, 0);
    for (size_t length = 0; length < bytes.size(); ++length) {
        ChunkData restored(0, 0);
        EXPECT_TRUE(SectionCodec::restore(std::span<const u8>(bytes).first(length), restored, 0).failed());
        EXPECT_EQ(restored.getSection(0), nullptr);
    }
    for (size_t word : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u}) {
        auto malformed = bytes;
        _setWord(malformed, word * sizeof(u32), std::numeric_limits<u32>::max());
        ChunkData restored(0, 0);
        EXPECT_TRUE(SectionCodec::restore(malformed, restored, 0).failed());
    }
    auto trailing = bytes;
    trailing.push_back(0);
    ChunkData restored(0, 0);
    EXPECT_TRUE(SectionCodec::restore(trailing, restored, 0).failed());
}

TEST_F(SectionCodecTest, RejectsCorruptHashSlotsAndUnreachableEntries)
{
    ChunkData source(0, 0);
    auto* section = source.createSection(0);
    section->fill(100);
    section->setBlockStateIdFast(1, 101);
    const auto packed = section->blockStates().packedView();
    const size_t hashOffset = 8 * sizeof(u32) + packed.palette.size_bytes() + packed.storage.size_bytes();
    auto bytes = _encode(source, 0);
    _setWord(bytes, hashOffset, std::numeric_limits<u32>::max());
    ChunkData restored(0, 0);
    EXPECT_TRUE(SectionCodec::restore(bytes, restored, 0).failed());
    bytes = _encode(source, 0);
    std::fill(bytes.begin() + static_cast<ptrdiff_t>(hashOffset),
        bytes.begin() + static_cast<ptrdiff_t>(hashOffset + packed.hashMap.size_bytes()),
        0);
    _setWord(bytes, hashOffset, 1);
    _setWord(bytes, hashOffset + sizeof(u32), 2);
    EXPECT_TRUE(SectionCodec::restore(bytes, restored, 0).failed());
}

TEST_F(SectionCodecTest, RejectsUnusedPackedIndexAndDuplicatePalette)
{
    ChunkData source(0, 0);
    auto* section = source.createSection(0);
    section->fill(100);
    section->setBlockStateIdFast(1, 101);
    section->setBlockStateIdFast(2, 102);
    auto bytes = _encode(source, 0);
    const auto packed = section->blockStates().packedView();
    const size_t storageOffset = 8 * sizeof(u32) + packed.palette.size_bytes();
    bytes[storageOffset] |= 3;
    ChunkData restored(0, 0);
    EXPECT_TRUE(SectionCodec::restore(bytes, restored, 0).failed());
    bytes = _encode(source, 0);
    _setWord(bytes, 8 * sizeof(u32) + sizeof(u32), 100);
    EXPECT_TRUE(SectionCodec::restore(bytes, restored, 0).failed());
}

} // namespace
} // namespace mc::world::storage

#pragma pop_macro("BYTE_SIZE")
