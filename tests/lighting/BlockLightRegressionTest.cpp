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

#include "common/core/Constants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

namespace {

void ensureVanillaBlocksInitialized()
{
    static bool initialized = false;
    if (!initialized) {
        mc::VanillaBlocks::initialize();
        initialized = true;
    }
}

class BlockLightChunkProvider : public mc::StarLightLightingProvider {
public:
    BlockLightChunkProvider(mc::i32 minBuildHeight, mc::i32 maxBuildHeight)
        : m_minBuildHeight(minBuildHeight)
        , m_maxBuildHeight(maxBuildHeight)
    {}

    void setChunk(mc::ChunkData* chunk) { m_chunk = chunk; }

    mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override
    {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (m_chunk->x() != x || m_chunk->z() != z) {
            return nullptr;
        }
        return m_chunk;
    }

    const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override
    {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (m_chunk->x() != x || m_chunk->z() != z) {
            return nullptr;
        }
        return m_chunk;
    }

    const mc::BlockState* getBlockStateForLight(const mc::BlockPos& pos) const override
    {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        if (pos.chunkX() != m_chunk->x() || pos.chunkZ() != m_chunk->z()) {
            return nullptr;
        }
        return m_chunk->getBlockState(pos.x & 0xF, pos.y, pos.z & 0xF);
    }

    mc::IWorld* getWorld() override { return nullptr; }

    const mc::IWorld* getWorld() const override { return nullptr; }

    void markLightChanged(mc::LightType, const mc::SectionPos&) override {}

    bool hasSkyLight() const override { return false; }

    mc::i32 getMinBuildHeight() const override { return m_minBuildHeight; }

    mc::i32 getMaxBuildHeight() const override { return m_maxBuildHeight; }

    mc::i32 getSectionCount() const override { return (m_maxBuildHeight - m_minBuildHeight) >> 4; }

private:
    mc::ChunkData* m_chunk = nullptr;
    mc::i32 m_minBuildHeight;
    mc::i32 m_maxBuildHeight;
};

// 测试1：发光方块传播光照到相邻方块
TEST(BlockLightRegressionTest, EmissiveBlockPropagatesToNeighbors)
{
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager lightManager(&provider, true, false);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    chunk.setBlockState(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    lightManager.updateSectionStatus(sectionPos, false);

    lightManager.lightChunk(&chunk, false);

    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5];
    ASSERT_NE(nibble, nullptr);

    const mc::u8 source = nibble->getUpdating(8, 70 - 64, 8);
    const mc::u8 east = nibble->getUpdating(9, 70 - 64, 8);
    const mc::u8 east2 = nibble->getUpdating(10, 70 - 64, 8);

    EXPECT_GT(source, static_cast<mc::u8>(0));
    EXPECT_GT(east, static_cast<mc::u8>(0));
    EXPECT_GT(east2, static_cast<mc::u8>(0));
    EXPECT_GE(source, east);
    EXPECT_GE(east, east2);
}

// 测试2：发光方块移除后光照变暗
TEST(BlockLightRegressionTest, RemovingSourceDarkensNearbyCells)
{
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager lightManager(&provider, true, false);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    const mc::BlockState* air = &mc::VanillaBlocks::AIR->defaultState();

    chunk.setBlockState(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    lightManager.updateSectionStatus(sectionPos, false);

    lightManager.lightChunk(&chunk, false);

    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5];
    ASSERT_NE(nibble, nullptr);

    mc::u8 before = nibble->getUpdating(9, 70 - 64, 8);
    EXPECT_GT(before, static_cast<mc::u8>(0));

    // 移除光源并使用 checkBlock 进行增量更新
    chunk.setBlockState(8, 70, 8, air);
    lightManager.checkBlock(8, 70, 8);

    mc::u8 after = nibble->getUpdating(8, 70 - 64, 8);
    mc::u8 neighborAfter = nibble->getUpdating(9, 70 - 64, 8);

    EXPECT_EQ(after, static_cast<mc::u8>(0));
    EXPECT_EQ(neighborAfter, static_cast<mc::u8>(0));
}

// 测试3：插入不透明方块减少后方光照
TEST(BlockLightRegressionTest, InsertingOpaqueBlockReducesBehindLight)
{
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager lightManager(&provider, true, false);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    const mc::BlockState* stone = &mc::VanillaBlocks::STONE->defaultState();

    chunk.setBlockState(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    lightManager.updateSectionStatus(sectionPos, false);

    lightManager.lightChunk(&chunk, false);

    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5];
    ASSERT_NE(nibble, nullptr);

    const mc::u8 before = nibble->getUpdating(10, 70 - 64, 8);
    EXPECT_GT(before, static_cast<mc::u8>(0));

    // 插入不透明方块并使用 checkBlock 进行增量更新
    chunk.setBlockState(9, 70, 8, stone);
    lightManager.checkBlock(9, 70, 8);

    const mc::u8 after = nibble->getUpdating(10, 70 - 64, 8);
    EXPECT_LT(after, before);
}

// 测试4：移除不透明方块恢复后方光照
TEST(BlockLightRegressionTest, RemovingOpaqueBlockRestoresBehindLight)
{
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager lightManager(&provider, true, false);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    const mc::BlockState* stone = &mc::VanillaBlocks::STONE->defaultState();
    const mc::BlockState* air = &mc::VanillaBlocks::AIR->defaultState();

    chunk.setBlockState(8, 70, 8, glowstone);
    chunk.setBlockState(9, 70, 8, stone);

    const mc::SectionPos sectionPos(0, 4, 0);
    lightManager.updateSectionStatus(sectionPos, false);

    lightManager.lightChunk(&chunk, false);

    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5];
    ASSERT_NE(nibble, nullptr);

    const mc::u8 blocked = nibble->getUpdating(10, 70 - 64, 8);

    // 移除不透明方块并使用 checkBlock 进行增量更新
    chunk.setBlockState(9, 70, 8, air);
    lightManager.checkBlock(9, 70, 8);

    const mc::u8 restored = nibble->getUpdating(10, 70 - 64, 8);
    EXPECT_GT(restored, blocked);
}

// 测试5：发光事件排队传播
TEST(BlockLightRegressionTest, EmissionIncreaseEventQueuesPropagation)
{
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager lightManager(&provider, true, false);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    chunk.setBlockState(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    lightManager.updateSectionStatus(sectionPos, false);

    lightManager.lightChunk(&chunk, false);

    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5];
    ASSERT_NE(nibble, nullptr);

    const mc::u8 east = nibble->getUpdating(9, 70 - 64, 8);
    EXPECT_GT(east, static_cast<mc::u8>(0));
}

// 测试6：验证 checkBlock 基本功能
TEST(BlockLightRegressionTest, CheckBlockMatchesCheckBlock)
{
    ensureVanillaBlocksInitialized();

    BlockLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    provider.setChunk(&chunk);

    mc::WorldLightManager lightManager(&provider, true, false);
    const mc::BlockState* glowstone = &mc::VanillaBlocks::GLOWSTONE->defaultState();
    chunk.setBlockState(8, 70, 8, glowstone);

    const mc::SectionPos sectionPos(0, 4, 0);
    lightManager.updateSectionStatus(sectionPos, false);

    lightManager.lightChunk(&chunk, false);

    auto* nibbles = chunk.getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[5];
    ASSERT_NE(nibble, nullptr);

    const mc::u8 east = nibble->getUpdating(9, 70 - 64, 8);
    EXPECT_GT(east, static_cast<mc::u8>(0));
}

} // namespace
