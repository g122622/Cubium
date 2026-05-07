#include <gtest/gtest.h>
#include "world/storage/db/SectionKey.hpp"
#include "world/storage/db/SectionCodec.hpp"
#include "world/chunk/ChunkData.hpp"
#include "core/Types.hpp"

namespace mc::world::storage {
namespace {

// 测试用生物群系 ID
namespace TestBiomes {
    constexpr BiomeId PLAINS = 1;
    constexpr BiomeId DESERT = 2;
    constexpr BiomeId FOREST = 4;
}

class SectionKeyTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SectionKeyTest, Construction)
{
    SectionKey key(10, 20, 5, 0);  // Overworld
    EXPECT_EQ(key.chunkX, 10);
    EXPECT_EQ(key.chunkZ, 20);
    EXPECT_EQ(key.sectionY, 5);
    EXPECT_EQ(key.dimension, 0);
}

TEST_F(SectionKeyTest, NegativeCoordinates)
{
    SectionKey key(-100, -200, 0, 1);  // Nether
    EXPECT_EQ(key.chunkX, -100);
    EXPECT_EQ(key.chunkZ, -200);
    EXPECT_EQ(key.sectionY, 0);
    EXPECT_EQ(key.dimension, 1);
}

TEST_F(SectionKeyTest, SerializeDeserialize)
{
    SectionKey original(123, -456, 10, 2);  // The End
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
    void SetUp() override {}
};

SectionData makeMalformedSectionData() {
    SectionData data;
    data.blockStates.clear();
    data.biomes.clear();
    data.skyLight = std::vector<u8>{};
    data.blockLight.reset();
    data.nonEmptyBlockCount = 3558;
    return data;
}

TEST_F(SectionCodecTest, EmptySection)
{
    ChunkSection section;

    SectionKey key(0, 0, 0, 0);
    auto result = SectionCodec::fromChunkSection(section, key);
    ASSERT_TRUE(result.success());

    SectionData& data = result.value();
    EXPECT_EQ(data.key.chunkX, 0);
    EXPECT_EQ(data.key.chunkZ, 0);
    EXPECT_EQ(data.key.sectionY, 0);
    EXPECT_EQ(data.nonEmptyBlockCount, 0u);
}

TEST_F(SectionCodecTest, SingleBlock)
{
    ChunkSection section;
    section.setBlockStateId(0, 0, 0, 1);  // Set a single block
    section.setBlockCount(1);  // 手动设置方块数量

    SectionKey key(5, 10, 3, 0);
    auto result = SectionCodec::fromChunkSection(section, key);
    ASSERT_TRUE(result.success());

    SectionData& data = result.value();
    EXPECT_EQ(data.key.chunkX, 5);
    EXPECT_EQ(data.key.chunkZ, 10);
    EXPECT_EQ(data.key.sectionY, 3);
    // nonEmptyBlockCount 会在 fromChunkSection 中计算
}

TEST_F(SectionCodecTest, MultipleBlocks)
{
    ChunkSection section;

    // Set some blocks
    u16 blockCount = 0;
    for (int y = 0; y < 4; ++y) {
        for (int z = 0; z < 4; ++z) {
            for (int x = 0; x < 4; ++x) {
                section.setBlockStateId(x, y, z, 2);  // Grass block
                ++blockCount;
            }
        }
    }
    section.setBlockCount(blockCount);

    SectionKey key(0, 0, 0, 0);
    auto result = SectionCodec::fromChunkSection(section, key);
    ASSERT_TRUE(result.success());

    SectionData& data = result.value();
    // fromChunkSection 会计算非空方块数
}

TEST_F(SectionCodecTest, SerializeDeserialize)
{
    ChunkSection original;
    original.setBlockStateId(5, 10, 15, 42);
    original.setBlockStateId(7, 8, 9, 100);
    original.setBlockCount(2);

    SectionKey key(100, -200, 7, 1);  // Nether
    auto encodeResult = SectionCodec::fromChunkSection(original, key);
    ASSERT_TRUE(encodeResult.success());

    auto& data = encodeResult.value();

    // key 应该被正确设置
    EXPECT_EQ(data.key.chunkX, 100);
    EXPECT_EQ(data.key.chunkZ, -200);
    EXPECT_EQ(data.key.sectionY, 7);
    EXPECT_EQ(data.key.dimension, 1);

    auto serializedResult = data.serialize();
    ASSERT_TRUE(serializedResult.success()) << "serialize failed: " << serializedResult.error().message();
    auto& serialized = serializedResult.value();
    ASSERT_FALSE(serialized.empty());

    // 注意：key 作为 RocksDB 键单独存储，不包含在序列化数据中
    auto decodeResult = SectionData::deserialize(serialized.data(), serialized.size());
    ASSERT_TRUE(decodeResult.success()) << "deserialize failed: " << decodeResult.error().message();

    SectionData& decoded = decodeResult.value();

    // 解码后的 key 需要单独设置（通常从 RocksDB 键恢复）
    decoded.key = key;

    EXPECT_EQ(decoded.key.chunkX, 100);
    EXPECT_EQ(decoded.key.chunkZ, -200);
    EXPECT_EQ(decoded.key.sectionY, 7);
    EXPECT_EQ(decoded.key.dimension, 1);

    // Verify block states
    EXPECT_EQ(decoded.getBlockStateId(5, 10, 15), 42u);
    EXPECT_EQ(decoded.getBlockStateId(7, 8, 9), 100u);
}

TEST_F(SectionCodecTest, RoundTripWithBiomes)
{
    ChunkSection section;
    section.setBlockStateId(0, 0, 0, 1);
    section.setBlockCount(1);

    // Create biome data (4x4x4 = 64 biomes)
    std::vector<BiomeId> biomes(64, TestBiomes::PLAINS);
    biomes[0] = TestBiomes::DESERT;
    biomes[63] = TestBiomes::FOREST;

    SectionKey key(0, 0, 0, 0);
    auto encodeResult = SectionCodec::fromChunkSection(section, key, biomes);
    ASSERT_TRUE(encodeResult.success());

    auto serializedResult = encodeResult.value().serialize();
    ASSERT_TRUE(serializedResult.success());

    auto decodeResult = SectionData::deserialize(serializedResult.value().data(), serializedResult.value().size());
    ASSERT_TRUE(decodeResult.success());

    SectionData& decoded = decodeResult.value();
    EXPECT_FALSE(decoded.biomes.empty());
    EXPECT_EQ(decoded.biomes[0], TestBiomes::DESERT);
    EXPECT_EQ(decoded.biomes[63], TestBiomes::FOREST);
}

TEST_F(SectionCodecTest, ToChunkSection)
{
    ChunkSection original;
    original.setBlockStateId(3, 5, 7, 123);
    original.setBlockStateId(10, 12, 14, 456);
    original.setBlockCount(2);

    SectionKey key(0, 0, 0, 0);
    auto encodeResult = SectionCodec::fromChunkSection(original, key);
    ASSERT_TRUE(encodeResult.success());

    auto serializedResult = encodeResult.value().serialize();
    ASSERT_TRUE(serializedResult.success());

    auto decodeResult = SectionData::deserialize(serializedResult.value().data(), serializedResult.value().size());
    ASSERT_TRUE(decodeResult.success());

    ChunkSection restored;
    auto applyResult = SectionCodec::toChunkSection(decodeResult.value(), restored);
    ASSERT_TRUE(applyResult.success());

    EXPECT_EQ(restored.getBlockStateId(3, 5, 7), 123u);
    EXPECT_EQ(restored.getBlockStateId(10, 12, 14), 456u);
}

TEST_F(SectionCodecTest, SerializeRejectsMalformedLayout)
{
    auto data = makeMalformedSectionData();

    auto serializedResult = data.serialize();

    ASSERT_TRUE(serializedResult.failed());
    EXPECT_EQ(serializedResult.error().code(), ErrorCode::InvalidData);
}

TEST_F(SectionCodecTest, ToChunkSectionRejectsMalformedLayout)
{
    auto data = makeMalformedSectionData();
    ChunkSection restored;

    auto applyResult = SectionCodec::toChunkSection(data, restored);

    ASSERT_TRUE(applyResult.failed());
    EXPECT_EQ(applyResult.error().code(), ErrorCode::InvalidData);
}

TEST_F(SectionCodecTest, DeserializeRejectsImpossibleBlockCount)
{
    ChunkSection original;
    original.setBlockStateId(0, 0, 0, 1);
    original.setBlockCount(1);

    SectionKey key(0, 0, 0, 0);
    auto encodeResult = SectionCodec::fromChunkSection(original, key);
    ASSERT_TRUE(encodeResult.success());

    auto serializedResult = encodeResult.value().serialize();
    ASSERT_TRUE(serializedResult.success());

    auto serialized = serializedResult.value();
    serialized[4] = 0x10;
    serialized[5] = 0x01;

    auto decodeResult = SectionData::deserialize(serialized.data(), serialized.size());

    ASSERT_TRUE(decodeResult.failed());
    EXPECT_EQ(decodeResult.error().code(), ErrorCode::InvalidData);
}

TEST_F(SectionCodecTest, LargeBlockStateIds)
{
    ChunkSection section;

    // Test large block state IDs (beyond 12 bits, used in modern versions)
    section.setBlockStateId(0, 0, 0, 10000);
    section.setBlockStateId(15, 15, 15, 50000);
    section.setBlockCount(2);

    SectionKey key(0, 0, 0, 0);
    auto encodeResult = SectionCodec::fromChunkSection(section, key);
    ASSERT_TRUE(encodeResult.success());

    auto serializedResult = encodeResult.value().serialize();
    ASSERT_TRUE(serializedResult.success());

    auto decodeResult = SectionData::deserialize(serializedResult.value().data(), serializedResult.value().size());
    ASSERT_TRUE(decodeResult.success());

    ChunkSection restored;
    auto applyResult = SectionCodec::toChunkSection(decodeResult.value(), restored);
    ASSERT_TRUE(applyResult.success());

    EXPECT_EQ(restored.getBlockStateId(0, 0, 0), 10000u);
    EXPECT_EQ(restored.getBlockStateId(15, 15, 15), 50000u);
}

TEST_F(SectionCodecTest, FullSection)
{
    ChunkSection section;

    // Fill entire section with blocks
    for (u32 y = 0; y < 16; ++y) {
        for (u32 z = 0; z < 16; ++z) {
            for (u32 x = 0; x < 16; ++x) {
                u32 blockId = (x + y + z) % 1000 + 1;
                section.setBlockStateId(x, y, z, blockId);
            }
        }
    }
    section.setBlockCount(4096);

    SectionKey key(0, 0, 0, 0);
    auto encodeResult = SectionCodec::fromChunkSection(section, key);
    ASSERT_TRUE(encodeResult.success());

    auto serializedResult = encodeResult.value().serialize();
    ASSERT_TRUE(serializedResult.success());

    auto decodeResult = SectionData::deserialize(serializedResult.value().data(), serializedResult.value().size());
    ASSERT_TRUE(decodeResult.success());

    ChunkSection restored;
    auto applyResult = SectionCodec::toChunkSection(decodeResult.value(), restored);
    ASSERT_TRUE(applyResult.success());

    // Verify all blocks
    for (u32 y = 0; y < 16; ++y) {
        for (u32 z = 0; z < 16; ++z) {
            for (u32 x = 0; x < 16; ++x) {
                u32 expected = (x + y + z) % 1000 + 1;
                EXPECT_EQ(restored.getBlockStateId(x, y, z), expected);
            }
        }
    }
}

} // namespace
} // namespace mc::world::storage
