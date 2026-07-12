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
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/engine/BaseLightEngine.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"

#include <unordered_map>
#include <vector>

namespace {

/**
 * @brief 测试用的光照引擎
 *
 * 实现必要的抽象方法，记录访问位置用于测试验证。
 */
class TestBaseLightEngine final : public mc::StarLightEngine {
public:
    TestBaseLightEngine()
        : StarLightEngine(false)
    { // false = 方块光照
        // 设置世界高度范围
        m_minSection = 0;
        m_maxSection = 15;
        m_minLightSection = -1;
        m_maxLightSection = 16;
    }

    struct Visit {
        mc::i64 pos = 0;
        bool isDecreasing = false;
    };

    [[nodiscard]] const std::vector<Visit>& visits() const { return m_visits; }

    void clearVisits() { m_visits.clear(); }

    // 实现抽象方法
    [[nodiscard]] const bool* getEmptinessMap(const mc::IChunk*) const override { return nullptr; }
    void setEmptinessMap(const mc::IChunk*, const bool*) override {}
    [[nodiscard]] mc::SWMRNibbleArray* const* getNibblesOnChunk(const mc::IChunk*) const override { return nullptr; }
    void setNibbles(const mc::IChunk*, mc::SWMRNibbleArray* const*) override {}
    [[nodiscard]] bool canUseChunk(const mc::IChunk*) const override { return true; }
    void initNibble(mc::i32, mc::i32, mc::i32, bool, bool) override {}
    void setNibbleNull(mc::i32, mc::i32, mc::i32) override {}

    void checkBlock(mc::StarLightLightingProvider*, mc::i32, mc::i32, mc::i32) override {}
    [[nodiscard]] mc::i32 calculateLightValue(
        mc::StarLightLightingProvider*, mc::i32, mc::i32, mc::i32, mc::i32 expected) override
    {
        return expected;
    }
    void propagateBlockChanges(
        mc::StarLightLightingProvider*, const mc::IChunk*, const std::vector<mc::BlockPos>&) override
    {}
    void lightChunk(mc::StarLightLightingProvider*, const mc::IChunk*, bool) override {}

    void setData(const mc::SectionPos&, const mc::NibbleArray&, bool) override {}
    [[nodiscard]] mc::SWMRNibbleArray* getData(const mc::SectionPos&) override { return nullptr; }
    [[nodiscard]] const mc::SWMRNibbleArray* getData(const mc::SectionPos&) const override { return nullptr; }

    // 暴露 protected 方法用于测试
    using mc::StarLightEngine::appendToDecreaseQueue;
    using mc::StarLightEngine::appendToIncreaseQueue;
    using mc::StarLightEngine::performLightDecrease;
    using mc::StarLightEngine::performLightIncrease;
    using mc::StarLightEngine::setupEncodeOffset;

protected:
    void notifyNeighborsOfLightUpdate(mc::i64 pos, mc::i32 level, bool isDecreasing)
    {
        m_visits.push_back(Visit{pos, isDecreasing});
    }

private:
    std::unordered_map<mc::i64, mc::i32> m_levels;
    std::vector<Visit> m_visits;
};

class TestChunkProvider final : public mc::StarLightLightingProvider {
public:
    void setChunk(mc::ChunkData* chunk) { m_chunk = chunk; }

    mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override
    {
        if (m_chunk != nullptr && m_chunk->x() == x && m_chunk->z() == z) {
            return m_chunk;
        }
        return nullptr;
    }

    const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override
    {
        if (m_chunk != nullptr && m_chunk->x() == x && m_chunk->z() == z) {
            return m_chunk;
        }
        return nullptr;
    }

    const mc::BlockState* getBlockStateForLight(const mc::BlockPos&) const override { return nullptr; }

    mc::IWorld* getWorld() override { return nullptr; }

    const mc::IWorld* getWorld() const override { return nullptr; }

    void markLightChanged(mc::LightType, const mc::SectionPos&) override {}

    bool hasSkyLight() const override { return false; }

    mc::i32 getMinBuildHeight() const override { return 0; }

    mc::i32 getMaxBuildHeight() const override { return mc::world::MAX_BUILD_HEIGHT; }

    mc::i32 getSectionCount() const override { return mc::world::CHUNK_SECTIONS; }

private:
    mc::ChunkData* m_chunk = nullptr;
};

/**
 * @brief 编码队列元素
 *
 * 格式:
 * - 位 0-5: X坐标（相对于编码偏移）
 * - 位 6-11: Z坐标
 * - 位 12-27: Y坐标
 * - 位 28-31: 传播级别
 * - 位 32-37: 传播方向位集
 */
mc::u64 encodeQueueEntry(mc::i32 x, mc::i32 y, mc::i32 z, mc::i32 level, mc::i32 directionBitset)
{
    // 编码格式参考 BaseLightEngine.hpp
    return static_cast<mc::u64>(x & 0x3F) | (static_cast<mc::u64>(z & 0x3F) << 6) |
        (static_cast<mc::u64>(y & 0xFFFF) << 12) | (static_cast<mc::u64>(level & 0xF) << 28) |
        (static_cast<mc::u64>(directionBitset & 0x3F) << 32);
}

TEST(BaseLightEngineQueueTest, ScheduleUpdateWorks)
{
    TestBaseLightEngine graph;

    const mc::i64 posA = mc::LightEngineUtils::packPos(0, 64, 0);
    const mc::i64 posB = mc::LightEngineUtils::packPos(1, 64, 0);
    const mc::i64 posC = mc::LightEngineUtils::packPos(2, 64, 0);

    // 安排更新
    graph.scheduleUpdate(posA);
    graph.scheduleUpdate(posB);
    graph.scheduleUpdate(posC);

    EXPECT_EQ(graph.queuedUpdateSize(), 3);
    EXPECT_TRUE(graph.needsUpdate());
    EXPECT_TRUE(graph.hasWork());
}

TEST(BaseLightEngineQueueTest, HasWorkReturnsFalseWhenEmpty)
{
    TestBaseLightEngine graph;

    EXPECT_FALSE(graph.needsUpdate());
    EXPECT_EQ(graph.queuedUpdateSize(), 0);
    EXPECT_FALSE(graph.hasWork());
}

TEST(BaseLightEngineQueueTest, IncreaseQueueAcceptsEntries)
{
    TestBaseLightEngine graph;

    // 设置编码偏移
    graph.setupEncodeOffset(0, 64, 0);

    // 添加到增亮队列
    graph.appendToIncreaseQueue(encodeQueueEntry(0, 64, 0, 3, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToIncreaseQueue(encodeQueueEntry(1, 64, 0, 3, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToIncreaseQueue(encodeQueueEntry(2, 64, 0, 3, mc::ALL_DIRECTIONS_BITSET));

    EXPECT_EQ(graph.queuedUpdateSize(), 3);
    EXPECT_TRUE(graph.needsUpdate());
}

TEST(BaseLightEngineQueueTest, DecreaseQueueAcceptsEntries)
{
    TestBaseLightEngine graph;

    // 设置编码偏移
    graph.setupEncodeOffset(0, 70, 0);

    // 添加到减亮队列
    graph.appendToDecreaseQueue(encodeQueueEntry(0, 70, 0, 5, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToDecreaseQueue(encodeQueueEntry(1, 70, 0, 5, mc::ALL_DIRECTIONS_BITSET));

    EXPECT_EQ(graph.queuedUpdateSize(), 2);
    EXPECT_TRUE(graph.needsUpdate());
}

TEST(BaseLightEngineQueueTest, MixedQueuesWork)
{
    TestBaseLightEngine graph;

    // 设置编码偏移
    graph.setupEncodeOffset(0, 80, 0);

    // 混合添加到增亮和减亮队列
    graph.appendToIncreaseQueue(encodeQueueEntry(0, 80, 0, 4, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToDecreaseQueue(encodeQueueEntry(1, 80, 0, 4, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToIncreaseQueue(encodeQueueEntry(2, 80, 0, 4, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToDecreaseQueue(encodeQueueEntry(3, 80, 0, 4, mc::ALL_DIRECTIONS_BITSET));

    EXPECT_EQ(graph.queuedUpdateSize(), 4);
    EXPECT_TRUE(graph.hasWork());
}

TEST(BaseLightEngineQueueTest, BlocksChangedInChunkWritesEmptinessMap)
{
    TestChunkProvider provider;
    mc::ChunkData chunk(0, 0);
    chunk.setStatus(mc::ChunkLoadStatus::Generated);
    chunk.setBlockEmptinessMap(nullptr);
    provider.setChunk(&chunk);

    mc::BlockStarLightEngine engine;
    const std::vector<mc::BlockPos> positions;
    const std::vector<bool> changedSections(mc::world::CHUNK_SECTIONS, true);

    engine.blocksChangedInChunk(&provider, 0, 0, positions, changedSections);

    const bool* emptinessMap = chunk.getBlockEmptinessMap();
    ASSERT_NE(emptinessMap, nullptr);

    for (mc::i32 sectionIndex = 0; sectionIndex < mc::world::CHUNK_SECTIONS; ++sectionIndex) {
        EXPECT_TRUE(emptinessMap[sectionIndex]);
    }

    for (mc::i32 sectionIndex = mc::world::CHUNK_SECTIONS; sectionIndex < mc::ChunkData::LIGHT_SECTIONS;
        ++sectionIndex) {
        EXPECT_FALSE(emptinessMap[sectionIndex]);
    }
}

TEST(BaseLightEngineQueueTest, ChunkDataEmptinessMapsRoundTripWithoutNullCrash)
{
    mc::ChunkData chunk(0, 0);

    std::array<bool, mc::ChunkData::LIGHT_SECTIONS> skyMap{};
    std::array<bool, mc::ChunkData::LIGHT_SECTIONS> blockMap{};

    for (mc::i32 sectionIndex = 0; sectionIndex < mc::ChunkData::LIGHT_SECTIONS; ++sectionIndex) {
        skyMap[static_cast<size_t>(sectionIndex)] = (sectionIndex % 2) == 0;
        blockMap[static_cast<size_t>(sectionIndex)] = (sectionIndex % 2) != 0;
    }

    chunk.setSkyEmptinessMap(skyMap.data());
    chunk.setBlockEmptinessMap(blockMap.data());

    const bool* skyResult = chunk.getSkyEmptinessMap();
    const bool* blockResult = chunk.getBlockEmptinessMap();

    ASSERT_NE(skyResult, nullptr);
    ASSERT_NE(blockResult, nullptr);

    for (mc::i32 sectionIndex = 0; sectionIndex < mc::ChunkData::LIGHT_SECTIONS; ++sectionIndex) {
        EXPECT_EQ(skyResult[sectionIndex], skyMap[static_cast<size_t>(sectionIndex)]);
        EXPECT_EQ(blockResult[sectionIndex], blockMap[static_cast<size_t>(sectionIndex)]);
    }

    chunk.setSkyEmptinessMap(nullptr);
    chunk.setBlockEmptinessMap(nullptr);

    EXPECT_EQ(chunk.getSkyEmptinessMap(), nullptr);
    EXPECT_EQ(chunk.getBlockEmptinessMap(), nullptr);
}

} // namespace
