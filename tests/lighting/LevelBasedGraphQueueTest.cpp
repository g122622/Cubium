#include <gtest/gtest.h>

#include "common/world/lighting/engine/LevelBasedGraph.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"

#include <unordered_map>
#include <vector>

namespace {

/**
 * @brief 测试用的光照引擎
 *
 * 实现必要的抽象方法，记录访问位置用于测试验证。
 */
class TestLevelBasedGraph final : public mc::StarLightEngine {
public:
    TestLevelBasedGraph()
        : StarLightEngine(false) {  // false = 方块光照
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

    [[nodiscard]] const std::vector<Visit>& visits() const {
        return m_visits;
    }

    void clearVisits() {
        m_visits.clear();
    }

    // 实现抽象方法
    [[nodiscard]] const bool* getEmptinessMap(const mc::IChunk*) const override { return nullptr; }
    void setEmptinessMap(const mc::IChunk*, const bool*) override {}
    [[nodiscard]] mc::SWMRNibbleArray* const* getNibblesOnChunk(const mc::IChunk*) const override { return nullptr; }
    void setNibbles(const mc::IChunk*, mc::SWMRNibbleArray* const*) override {}
    [[nodiscard]] bool canUseChunk(const mc::IChunk*) const override { return true; }
    void initNibble(mc::i32, mc::i32, mc::i32, bool, bool) override {}
    void setNibbleNull(mc::i32, mc::i32, mc::i32) override {}

    void checkBlock(mc::StarLightLightingProvider*, mc::i32, mc::i32, mc::i32) override {}
    [[nodiscard]] mc::i32 calculateLightValue(mc::StarLightLightingProvider*,
                                               mc::i32, mc::i32, mc::i32,
                                               mc::i32 expected) override { return expected; }
    void propagateBlockChanges(mc::StarLightLightingProvider*,
                               const mc::IChunk*,
                               const std::vector<mc::BlockPos>&) override {}
    void lightChunk(mc::StarLightLightingProvider*, const mc::IChunk*, bool) override {}

    [[nodiscard]] mc::u8 getLightFor(mc::i32, mc::i32, mc::i32) const override { return 0; }
    void setData(const mc::SectionPos&, const mc::NibbleArray&, bool) override {}
    [[nodiscard]] mc::SWMRNibbleArray* getData(const mc::SectionPos&) override { return nullptr; }

    // 暴露 protected 方法用于测试
    using mc::StarLightEngine::setupEncodeOffset;
    using mc::StarLightEngine::appendToIncreaseQueue;
    using mc::StarLightEngine::appendToDecreaseQueue;
    using mc::StarLightEngine::performLightIncrease;
    using mc::StarLightEngine::performLightDecrease;

protected:
    void notifyNeighborsOfLightUpdate(mc::i64 pos, mc::i32 level, bool isDecreasing) {
        m_visits.push_back(Visit{pos, isDecreasing});
    }

private:
    std::unordered_map<mc::i64, mc::i32> m_levels;
    std::vector<Visit> m_visits;
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
mc::u64 encodeQueueEntry(mc::i32 x, mc::i32 y, mc::i32 z, mc::i32 level, mc::i32 directionBitset) {
    // 编码格式参考 LevelBasedGraph.hpp
    return static_cast<mc::u64>(x & 0x3F) |
           (static_cast<mc::u64>(z & 0x3F) << 6) |
           (static_cast<mc::u64>(y & 0xFFFF) << 12) |
           (static_cast<mc::u64>(level & 0xF) << 28) |
           (static_cast<mc::u64>(directionBitset & 0x3F) << 32);
}

TEST(LevelBasedGraphQueueTest, ScheduleUpdateWorks) {
    TestLevelBasedGraph graph;

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

TEST(LevelBasedGraphQueueTest, HasWorkReturnsFalseWhenEmpty) {
    TestLevelBasedGraph graph;

    EXPECT_FALSE(graph.needsUpdate());
    EXPECT_EQ(graph.queuedUpdateSize(), 0);
    EXPECT_FALSE(graph.hasWork());
}

TEST(LevelBasedGraphQueueTest, IncreaseQueueAcceptsEntries) {
    TestLevelBasedGraph graph;

    // 设置编码偏移
    graph.setupEncodeOffset(0, 64, 0);

    // 添加到增亮队列
    graph.appendToIncreaseQueue(encodeQueueEntry(0, 64, 0, 3, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToIncreaseQueue(encodeQueueEntry(1, 64, 0, 3, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToIncreaseQueue(encodeQueueEntry(2, 64, 0, 3, mc::ALL_DIRECTIONS_BITSET));

    EXPECT_EQ(graph.queuedUpdateSize(), 3);
    EXPECT_TRUE(graph.needsUpdate());
}

TEST(LevelBasedGraphQueueTest, DecreaseQueueAcceptsEntries) {
    TestLevelBasedGraph graph;

    // 设置编码偏移
    graph.setupEncodeOffset(0, 70, 0);

    // 添加到减亮队列
    graph.appendToDecreaseQueue(encodeQueueEntry(0, 70, 0, 5, mc::ALL_DIRECTIONS_BITSET));
    graph.appendToDecreaseQueue(encodeQueueEntry(1, 70, 0, 5, mc::ALL_DIRECTIONS_BITSET));

    EXPECT_EQ(graph.queuedUpdateSize(), 2);
    EXPECT_TRUE(graph.needsUpdate());
}

TEST(LevelBasedGraphQueueTest, MixedQueuesWork) {
    TestLevelBasedGraph graph;

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

} // namespace
