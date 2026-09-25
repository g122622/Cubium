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

#include "common/network/backend/java/mappings/JavaBlockStateIdMap.hpp"
#include "common/network/sync/VanillaChunkWire.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

using namespace mc;
using namespace mc::world;

// ============================================================================
// VanillaChunkWire 测试
// ============================================================================

/**
 * @brief ChunkData ↔ LevelChunkWithLight IR 的往返转换测试
 *
 * 重点覆盖**非空气方块计数**：它是 `ChunkSection::isEmpty()` 的唯一判据，而 `isEmpty()`
 * 同时决定"序列化是否整段跳过方块数据"与"渲染剔除是否跳过该段"。计数一旦失真，
 * 方块数据会以"空段"的名义被整段丢弃，且不产生任何报错。
 *
 * 写入方 `readLevelChunkWithLightIR` 曾经先写计数再写方块状态，而写方块状态本身会
 * 维护计数，导致最终值为服务端报告值的两倍——本文件的存在就是为了钉住这个顺序。
 */
class VanillaChunkWireTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        mc::world::biome::BiomeRegistry::instance().initialize();
        ASSERT_TRUE(mc::network::backend::java::JavaBlockStateIdMap::instance().initialize().success());
        ASSERT_TRUE(mc::world::biome::JavaBiomeRegistryIdMap::instance().initialize().success());
    }

    /// 造一个含若干方块的区块，落在指定的段索引上
    static std::unique_ptr<ChunkData> makeChunk(i32 sectionIndex, i32 blockCount)
    {
        auto chunk = std::make_unique<ChunkData>(0, 0);
        ChunkSection* section = chunk->createSection(sectionIndex);
        EXPECT_NE(section, nullptr);
        if (section == nullptr) {
            return chunk;
        }

        const u32 stoneId = VanillaBlocks::STONE->defaultState().stateId();
        for (i32 i = 0; i < blockCount && i < ChunkSection::VOLUME; ++i) {
            const i32 x = i % ChunkSection::SIZE;
            const i32 y = (i / ChunkSection::SIZE) % ChunkSection::SIZE;
            const i32 z = (i / (ChunkSection::SIZE * ChunkSection::SIZE)) % ChunkSection::SIZE;
            section->setBlockStateIdFast(ChunkSection::blockIndex(x, y, z), stoneId);
        }
        return chunk;
    }
};

TEST_F(VanillaChunkWireTest, ReadLevelChunkWithLightDoesNotDoubleCountBlocks)
{
    constexpr i32 SECTION_INDEX = 4;
    constexpr i32 BLOCK_COUNT = 7;

    auto source = makeChunk(SECTION_INDEX, BLOCK_COUNT);
    const ChunkSection* sourceSection = source->getSection(SECTION_INDEX);
    ASSERT_NE(sourceSection, nullptr);
    ASSERT_EQ(sourceSection->getBlockCount(), BLOCK_COUNT);

    auto ir = mc::world::chunk::VanillaChunkWire::buildLevelChunkWithLightIR(*source);
    ASSERT_TRUE(ir.success()) << ir.error().message();
    ASSERT_EQ(ir.value().sections[static_cast<size_t>(SECTION_INDEX)].nonEmptyBlockCount, BLOCK_COUNT);

    auto restored = mc::world::chunk::VanillaChunkWire::readLevelChunkWithLightIR(ir.value());
    ASSERT_TRUE(restored.success()) << restored.error().message();

    const std::unique_ptr<ChunkData>& restoredChunk = restored.value();
    ASSERT_NE(restoredChunk, nullptr);

    const ChunkSection* restoredSection = restoredChunk->getSection(SECTION_INDEX);
    ASSERT_NE(restoredSection, nullptr);

    // 计数必须回到服务端报告值，而不是它的两倍
    EXPECT_EQ(restoredSection->getBlockCount(), BLOCK_COUNT) << "客户端还原的段把非空方块计数累加了两次";
    EXPECT_FALSE(restoredSection->isEmpty());

    // 方块本身也要原样还原
    for (i32 i = 0; i < BLOCK_COUNT; ++i) {
        const i32 x = i % ChunkSection::SIZE;
        const i32 y = (i / ChunkSection::SIZE) % ChunkSection::SIZE;
        const i32 z = (i / (ChunkSection::SIZE * ChunkSection::SIZE)) % ChunkSection::SIZE;
        EXPECT_EQ(restoredSection->getBlockStateId(x, y, z), VanillaBlocks::STONE->defaultState().stateId());
    }
}

TEST_F(VanillaChunkWireTest, ReadLevelChunkWithLightKeepsEmptySectionEmpty)
{
    auto source = std::make_unique<ChunkData>(0, 0);

    auto ir = mc::world::chunk::VanillaChunkWire::buildLevelChunkWithLightIR(*source);
    ASSERT_TRUE(ir.success()) << ir.error().message();

    auto restored = mc::world::chunk::VanillaChunkWire::readLevelChunkWithLightIR(ir.value());
    ASSERT_TRUE(restored.success()) << restored.error().message();

    const std::unique_ptr<ChunkData>& restoredChunk = restored.value();
    ASSERT_NE(restoredChunk, nullptr);

    for (i32 sec = 0; sec < mc::world::CHUNK_SECTIONS; ++sec) {
        const ChunkSection* section = restoredChunk->getSection(sec);
        if (section != nullptr) {
            EXPECT_EQ(section->getBlockCount(), 0);
            EXPECT_TRUE(section->isEmpty());
        }
    }
}

TEST_F(VanillaChunkWireTest, RoundTripPreservesBlockCountAcrossMultipleSections)
{
    auto source = makeChunk(2, 3);
    source->createSection(9)->setBlockStateIdFast(0, VanillaBlocks::DIRT->defaultState().stateId());
    source->createSection(20)->setBlockStateIdFast(0, VanillaBlocks::GRASS_BLOCK->defaultState().stateId());
    source->createSection(20)->setBlockStateIdFast(1, VanillaBlocks::STONE->defaultState().stateId());

    auto ir = mc::world::chunk::VanillaChunkWire::buildLevelChunkWithLightIR(*source);
    ASSERT_TRUE(ir.success()) << ir.error().message();

    auto restored = mc::world::chunk::VanillaChunkWire::readLevelChunkWithLightIR(ir.value());
    ASSERT_TRUE(restored.success()) << restored.error().message();

    const std::unique_ptr<ChunkData>& restoredChunk = restored.value();
    ASSERT_NE(restoredChunk, nullptr);

    const ChunkSection* sec2 = restoredChunk->getSection(2);
    const ChunkSection* sec9 = restoredChunk->getSection(9);
    const ChunkSection* sec20 = restoredChunk->getSection(20);
    ASSERT_NE(sec2, nullptr);
    ASSERT_NE(sec9, nullptr);
    ASSERT_NE(sec20, nullptr);

    EXPECT_EQ(sec2->getBlockCount(), 3);
    EXPECT_EQ(sec9->getBlockCount(), 1);
    EXPECT_EQ(sec20->getBlockCount(), 2);
}
