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

#include <gtest/gtest.h>

#include "common/network/sync/ChunkSerializer.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

using namespace mc;
using namespace mc::network;

// ============================================================================
// ChunkSerializer 测试
// ============================================================================

class ChunkSerializerTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChunkSerializerTest, SerializeEmptyChunk)
{
    ChunkData chunk(10, -5);

    auto result = ChunkSerializer::serializeChunk(chunk);
    EXPECT_TRUE(result.success());

    const auto& data = result.value();
    EXPECT_FALSE(data.empty());
}

TEST_F(ChunkSerializerTest, SerializeChunkWithBlocks)
{
    ChunkData chunk(0, 0);

    // 填充一些方块
    auto section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);

    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            section->setBlockStateId(x, 0, z, stoneStateId);
        }
    }

    auto result = ChunkSerializer::serializeChunk(chunk);
    EXPECT_TRUE(result.success());

    const auto& data = result.value();
    EXPECT_FALSE(data.empty());

    // 验证大小
    size_t expectedSize = ChunkSerializer::calculateChunkSize(chunk);
    EXPECT_EQ(data.size(), expectedSize);
}

TEST_F(ChunkSerializerTest, DeserializeChunk)
{
    // 创建并序列化一个区块
    ChunkData original(100, -200);

    auto section = original.createSection(4);
    ASSERT_NE(section, nullptr);

    // 设置一些方块
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    u32 dirtStateId = VanillaBlocks::DIRT->defaultState().stateId();
    section->setBlockStateId(5, 5, 5, stoneStateId);
    section->setBlockStateId(10, 10, 10, dirtStateId);

    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    auto deserializeResult = ChunkSerializer::deserializeChunk(100, -200, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();
    EXPECT_EQ(restored->x(), 100);
    EXPECT_EQ(restored->z(), -200);
    EXPECT_TRUE(restored->isFullyGenerated());
}

TEST_F(ChunkSerializerTest, DeserializeChunkPreservesBiomeData)
{
    ChunkData original(3, 7);

    BiomeContainer biomes;
    // setBiome 的 x/y/z 是 section 内的 4x4x4 采样坐标(0-3)，sectionIndex 是区块段索引。
    // getBiomeAtBlock 接受世界方块坐标，会自行映射到 section 与采样点：
    //   block (0,0,0)   -> section 4 (yOffset=64), 采样 (0,0,0)
    //   block (15,63,15)-> section 7 (yOffset=127), 采样 (3,3,3)
    biomes.setBiome(4, 0, 0, 0, Biomes::Forest);
    biomes.setBiome(7, 3, 3, 3, Biomes::Badlands);
    original.setBiomes(std::move(biomes));

    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = ChunkSerializer::deserializeChunk(3, 7, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    const auto& restored = deserializeResult.value();
    EXPECT_EQ(restored->getBiomeAtBlock(0, 0, 0), Biomes::Forest);
    EXPECT_EQ(restored->getBiomeAtBlock(15, 63, 15), Biomes::Badlands);
}

TEST_F(ChunkSerializerTest, SectionMask)
{
    ChunkData chunk(0, 0);

    // 空区块，位掩码应为0
    u16 mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, 0);

    // 创建一个非空区块段
    auto section = chunk.createSection(5);
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(0, 0, 0, stoneStateId);

    mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, (1 << 5)); // 第5位应该被设置
}

TEST_F(ChunkSerializerTest, SectionSize)
{
    ChunkSection section;

    size_t size = ChunkSerializer::calculateSectionSize(section);
    // 新格式: 方块数据 (4096 * 4) + 天空光照 (2048) + 方块光照 (2048) + 计数 (2)
    EXPECT_EQ(size, 2 + ChunkSection::VOLUME * 4 + 2048 + 2048);
}

// ============================================================================
// ChunkSerializer 扩展测试
// ============================================================================

class ChunkSerializerExtendedTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChunkSerializerExtendedTest, SerializeDeserializeConsistency)
{
    // 创建一个复杂的区块
    ChunkData original(42, -100);

    // 填充多个区块段
    for (int sectionY = 0; sectionY < 5; ++sectionY) {
        auto section = original.createSection(sectionY);
        ASSERT_NE(section, nullptr);

        u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
        u32 dirtStateId = VanillaBlocks::DIRT->defaultState().stateId();
        u32 grassStateId = VanillaBlocks::GRASS_BLOCK->defaultState().stateId();

        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                // 底层石头
                section->setBlockStateId(x, 0, z, stoneStateId);
                // 中层泥土
                section->setBlockStateId(x, 1, z, dirtStateId);
                // 顶层草地
                section->setBlockStateId(x, 2, z, grassStateId);
            }
        }
    }

    // 设置生物群系
    BiomeContainer biomes;
    biomes.setBiome(0, 0, 0, 0, Biomes::Forest);
    biomes.setBiome(0, 7, 7, 7, Biomes::Desert);
    biomes.setBiome(0, 15, 15, 15, Biomes::Ocean);
    original.setBiomes(std::move(biomes));

    // 序列化
    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    auto deserializeResult = ChunkSerializer::deserializeChunk(42, -100, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();
    EXPECT_EQ(restored->x(), 42);
    EXPECT_EQ(restored->z(), -100);

    // 验证区块段
    for (int sectionY = 0; sectionY < 5; ++sectionY) {
        EXPECT_TRUE(restored->hasSection(sectionY));
    }
}

TEST_F(ChunkSerializerExtendedTest, SerializeChunkWithAir)
{
    ChunkData chunk(0, 0);
    auto section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);

    // 填充空气
    u32 airStateId = VanillaBlocks::AIR->defaultState().stateId();
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 16; ++y) {
            for (int z = 0; z < 16; ++z) {
                section->setBlockStateId(x, y, z, airStateId);
            }
        }
    }

    auto result = ChunkSerializer::serializeChunk(chunk);
    EXPECT_TRUE(result.success());
}

TEST_F(ChunkSerializerExtendedTest, EmptySectionMask)
{
    ChunkData chunk(0, 0);

    // 不创建任何区块段
    u16 mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, 0);
}

TEST_F(ChunkSerializerExtendedTest, MultipleSectionsMask)
{
    ChunkData chunk(0, 0);

    // 创建多个区块段
    chunk.createSection(0);
    chunk.createSection(5);
    chunk.createSection(10);

    // 设置一些方块使其非空
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    chunk.getSection(0)->setBlockStateId(0, 0, 0, stoneStateId);
    chunk.getSection(5)->setBlockStateId(0, 0, 0, stoneStateId);
    chunk.getSection(10)->setBlockStateId(0, 0, 0, stoneStateId);

    u16 mask = ChunkSerializer::calculateSectionMask(chunk);
    EXPECT_EQ(mask, (1 << 0) | (1 << 5) | (1 << 10));
}

TEST_F(ChunkSerializerExtendedTest, DeserializeInvalidData)
{
    // 太小的数据
    std::vector<u8> smallData = {0x01, 0x02, 0x03};
    auto result1 = ChunkSerializer::deserializeChunk(0, 0, smallData);
    EXPECT_FALSE(result1.success());

    // 坐标不匹配
    ChunkData chunk(10, 20);
    auto serializeResult = ChunkSerializer::serializeChunk(chunk);
    ASSERT_TRUE(serializeResult.success());
    auto result2 = ChunkSerializer::deserializeChunk(30, 40, serializeResult.value());
    EXPECT_FALSE(result2.success());
}

TEST_F(ChunkSerializerExtendedTest, ChunkSizeCalculation)
{
    ChunkData chunk(0, 0);

    // 空区块
    size_t emptySize = ChunkSerializer::calculateChunkSize(chunk);

    // 添加一个区块段
    auto section = chunk.createSection(0);
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(0, 0, 0, stoneStateId);

    size_t oneSectionSize = ChunkSerializer::calculateChunkSize(chunk);
    EXPECT_GT(oneSectionSize, emptySize);

    // 验证实际序列化大小
    auto result = ChunkSerializer::serializeChunk(chunk);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), oneSectionSize);
}

// ============================================================================
// 光照数据序列化测试
// ============================================================================

class ChunkSerializerLightTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChunkSerializerLightTest, SerializeDeserializeLightData)
{
    // 创建一个区块并设置光照数据
    ChunkData original(0, 0);
    auto section = original.createSection(4); // Y=64-79
    ASSERT_NE(section, nullptr);

    // 设置一些方块
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(5, 5, 5, stoneStateId);
    section->setBlockStateId(10, 10, 10, stoneStateId);

    // 设置天空光照
    section->setSkyLight(0, 0, 0, 15);
    section->setSkyLight(5, 5, 5, 10);
    section->setSkyLight(10, 10, 10, 5);

    // 设置方块光照
    section->setBlockLight(0, 0, 0, 0);
    section->setBlockLight(5, 5, 5, 8);
    section->setBlockLight(10, 10, 10, 12);

    // 序列化
    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    // 反序列化
    auto deserializeResult = ChunkSerializer::deserializeChunk(0, 0, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();
    ASSERT_TRUE(restored->hasSection(4));

    const ChunkSection* restoredSection = restored->getSection(4);
    ASSERT_NE(restoredSection, nullptr);

    // 验证天空光照
    EXPECT_EQ(restoredSection->getSkyLight(0, 0, 0), 15);
    EXPECT_EQ(restoredSection->getSkyLight(5, 5, 5), 10);
    EXPECT_EQ(restoredSection->getSkyLight(10, 10, 10), 5);

    // 验证方块光照
    EXPECT_EQ(restoredSection->getBlockLight(0, 0, 0), 0);
    EXPECT_EQ(restoredSection->getBlockLight(5, 5, 5), 8);
    EXPECT_EQ(restoredSection->getBlockLight(10, 10, 10), 12);
}

TEST_F(ChunkSerializerLightTest, LightDataNibbleArrayFormat)
{
    // 测试 NibbleArray 的打包和解包
    ChunkData original(0, 0);
    auto section = original.createSection(0);
    ASSERT_NE(section, nullptr);

    // 设置一些方块使区块段非空（否则不会被序列化）
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section->setBlockStateId(0, 0, 0, stoneStateId);

    // 设置多种光照值
    for (int i = 0; i < 16; ++i) {
        section->setSkyLight(i, 0, 0, static_cast<u8>(i));
        section->setBlockLight(0, i, 0, static_cast<u8>(15 - i));
    }

    // 序列化和反序列化
    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = ChunkSerializer::deserializeChunk(0, 0, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();
    const ChunkSection* restoredSection = restored->getSection(0);
    ASSERT_NE(restoredSection, nullptr);

    // 验证所有光照值
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(restoredSection->getSkyLight(i, 0, 0), static_cast<u8>(i)) << "Sky light mismatch at i=" << i;
        EXPECT_EQ(restoredSection->getBlockLight(0, i, 0), static_cast<u8>(15 - i))
            << "Block light mismatch at i=" << i;
    }
}

TEST_F(ChunkSerializerLightTest, MultipleSectionsLightData)
{
    // 测试多个区块段的光照数据
    ChunkData original(0, 0);

    for (int sectionY = 0; sectionY < 16; ++sectionY) {
        auto section = original.createSection(sectionY);
        ASSERT_NE(section, nullptr);

        // 设置不同段的光照
        section->setSkyLight(0, 0, 0, static_cast<u8>(sectionY));
        section->setBlockLight(0, 0, 0, static_cast<u8>(15 - sectionY));

        // 设置一些方块使其非空
        u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
        section->setBlockStateId(0, 0, 0, stoneStateId);
    }

    // 序列化和反序列化
    auto serializeResult = ChunkSerializer::serializeChunk(original);
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = ChunkSerializer::deserializeChunk(0, 0, serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    auto restored = deserializeResult.value();

    // 验证每个段的光照数据
    for (int sectionY = 0; sectionY < 16; ++sectionY) {
        EXPECT_TRUE(restored->hasSection(sectionY));
        const ChunkSection* section = restored->getSection(sectionY);
        ASSERT_NE(section, nullptr);

        EXPECT_EQ(section->getSkyLight(0, 0, 0), static_cast<u8>(sectionY))
            << "Sky light mismatch in section " << sectionY;
        EXPECT_EQ(section->getBlockLight(0, 0, 0), static_cast<u8>(15 - sectionY))
            << "Block light mismatch in section " << sectionY;
    }
}

TEST_F(ChunkSerializerLightTest, LightDataSectionSizeCalculation)
{
    // 测试区块段大小计算是否包含光照数据
    ChunkSection section;

    // 设置一些方块
    u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    section.setBlockStateId(0, 0, 0, stoneStateId);

    size_t size = ChunkSerializer::calculateSectionSize(section);

    // 新格式: 2(计数) + 4096*4(方块) + 2048(天空光照) + 2048(方块光照) = 18434 字节
    EXPECT_EQ(size, 2 + ChunkSection::VOLUME * 4 + 2048 + 2048);
}
