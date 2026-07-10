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
#include "common/util/NibbleArray.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"

namespace {

void ensureVanillaBlocksInitialized()
{
    static bool initialized = false;
    if (!initialized) {
        mc::VanillaBlocks::initialize();
        initialized = true;
    }
}

class SkyLightChunkProvider : public mc::StarLightLightingProvider {
public:
    SkyLightChunkProvider(mc::i32 minBuildHeight, mc::i32 maxBuildHeight)
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

    bool hasSkyLight() const override { return true; }

    mc::i32 getMinBuildHeight() const override { return m_minBuildHeight; }

    mc::i32 getMaxBuildHeight() const override { return m_maxBuildHeight; }

    mc::i32 getSectionCount() const override { return (m_maxBuildHeight - m_minBuildHeight) >> 4; }

private:
    mc::ChunkData* m_chunk = nullptr;
    mc::i32 m_minBuildHeight;
    mc::i32 m_maxBuildHeight;
};

TEST(SkyLightRegressionTest, FloatingStoneUndersideHasNonZeroSkyLight)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated); // 设置区块状态，使光照引擎可以使用它
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine;

    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    chunk.setBlockState(8, 70, 8, stoneState);

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);

    // 使用 light() 方法初始化区块光照（这会设置缓存）
    engine.light(&provider, &chunk, false);

    // 从 ChunkData 的 SWMRNibbleArray 读取光照数据
    // 主世界 MIN_BUILD_HEIGHT=-64 → m_minSection=-4 → m_minLightSection=-5。
    // Starlight nibble 数组按光照段坐标索引：nibbles[sectionY - m_minLightSection]。
    // Y=69 在 section 4 (Y=64-79)，nibble 索引 = 4 - (-5) = 9。
    // （历史 bug：旧用例写作 nibbles[5]，对应 m_minLightSection=-1 的下界(minY=0)维度，对主世界错误。）
    auto* nibbles = chunk.getSkyNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[9]; // section 4
    ASSERT_NE(nibble, nullptr);

    // 检查状态
    EXPECT_TRUE(nibble->isInitializedUpdating()) << "Nibble array should be initialized";

    // 读取 (8, 69, 8) 位置的光照值
    // localY = 69 - 64 = 5 (section 4 从 Y=64 开始)
    mc::u8 belowLight = nibble->getUpdating(8, 69 - 64, 8);
    EXPECT_GT(belowLight, static_cast<mc::u8>(0)) << "Light at (8, 69, 8) should be > 0";
}

TEST(SkyLightRegressionTest, LightRebuildsSkyEmptinessMapWhenMissing)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    chunk.setSkyEmptinessMap(nullptr);
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine;

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);

    engine.light(&provider, &chunk, false);

    const bool* emptinessMap = chunk.getSkyEmptinessMap();
    ASSERT_NE(emptinessMap, nullptr);

    for (mc::i32 sectionIndex = 0; sectionIndex < mc::world::CHUNK_SECTIONS; ++sectionIndex) {
        EXPECT_TRUE(emptinessMap[sectionIndex]);
    }

    for (mc::i32 sectionIndex = mc::world::CHUNK_SECTIONS; sectionIndex < mc::ChunkData::LIGHT_SECTIONS;
        ++sectionIndex) {
        EXPECT_FALSE(emptinessMap[sectionIndex]);
    }
}

TEST(SkyLightRegressionTest, SealedRoofDropsCaveSkyLightBelow15)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated); // 设置区块状态
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine;
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();

    // 在 section=4 顶层铺满石头，封闭下方空间。
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            chunk.setBlockState(x, 79, z, stoneState);
        }
    }

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);

    // 使用 light() 方法初始化区块光照
    engine.light(&provider, &chunk, false);

    // 从 ChunkData 的 SWMRNibbleArray 读取光照数据
    // nibble 索引 = sectionY - m_minLightSection(-5) = 4 - (-5) = 9（详见首个用例注释）
    auto* nibbles = chunk.getSkyNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[9]; // section 4
    ASSERT_NE(nibble, nullptr);

    // 读取 (8, 78, 8) 位置的光照值
    // localY = 78 % 16 = 14
    mc::u8 caveSkyLight = nibble->getUpdating(8, 14, 8);
    EXPECT_LT(caveSkyLight, static_cast<mc::u8>(15));
}

TEST(SkyLightRegressionTest, OpeningRoofRestoresCaveSkyLight)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated); // 设置区块状态
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine;
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    const mc::BlockState* airState = &mc::VanillaBlocks::AIR->defaultState();

    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            chunk.setBlockState(x, 79, z, stoneState);
        }
    }

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);

    // 使用 light() 方法初始化区块光照
    engine.light(&provider, &chunk, false);

    // 从 ChunkData 的 SWMRNibbleArray 读取光照数据
    // nibble 索引 = sectionY - m_minLightSection(-5) = 4 - (-5) = 9（详见首个用例注释）
    auto* nibbles = chunk.getSkyNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[9]; // section 4
    ASSERT_NE(nibble, nullptr);

    mc::u8 before = nibble->getUpdating(8, 14, 8); // Y=78 -> localY=14
    EXPECT_LT(before, static_cast<mc::u8>(15));

    // 打开一个洞
    chunk.setBlockState(8, 79, 8, airState);

    // 重新运行光照检查
    engine.light(&provider, &chunk, false);

    mc::u8 after = nibble->getUpdating(8, 14, 8);
    EXPECT_GT(after, before);
}

TEST(SkyLightRegressionTest, CheckBlockMatchesCheckBlock)
{
    ensureVanillaBlocksInitialized();

    SkyLightChunkProvider provider(mc::world::MIN_BUILD_HEIGHT, mc::world::MAX_BUILD_HEIGHT);
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated); // 设置区块状态
    provider.setChunk(&chunk);

    mc::SkyStarLightEngine engine;
    const mc::BlockState* stoneState = &mc::VanillaBlocks::STONE->defaultState();
    chunk.setBlockState(8, 70, 8, stoneState);

    const mc::SectionPos sectionPos(0, 4, 0);
    engine.updateSectionStatus(sectionPos, false);

    // 使用 light() 方法初始化区块光照
    engine.light(&provider, &chunk, false);

    // 从 ChunkData 的 SWMRNibbleArray 读取光照数据
    // nibble 索引 = sectionY - m_minLightSection(-5) = 4 - (-5) = 9（详见首个用例注释）
    auto* nibbles = chunk.getSkyNibbles();
    ASSERT_NE(nibbles, nullptr);
    mc::SWMRNibbleArray* nibble = nibbles[9]; // section 4
    ASSERT_NE(nibble, nullptr);

    // Y=69 -> localY=5
    mc::u8 light = nibble->getUpdating(8, 5, 8);
    EXPECT_GT(light, static_cast<mc::u8>(0));
}

} // namespace
