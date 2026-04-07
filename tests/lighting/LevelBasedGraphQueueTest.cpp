#include <gtest/gtest.h>

#include "common/world/lighting/engine/LevelBasedGraph.hpp"
#include "common/world/lighting/engine/LightEngineUtils.hpp"

#include <unordered_map>
#include <vector>

namespace {

class TestLevelBasedGraph final : public mc::StarLightEngine {
public:
    explicit TestLevelBasedGraph(mc::i32 expectedUpdates)
        : StarLightEngine(16, expectedUpdates, nullptr) {
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

protected:
    [[nodiscard]] bool isRoot(mc::i64) const override {
        return false;
    }

    [[nodiscard]] mc::i32 computeLevel(mc::i64, mc::i64, mc::i32 level) override {
        return level;
    }

    void notifyNeighbors(mc::i64 pos,
                         mc::i32,
                         bool isDecreasing,
                         mc::u8) override {
        m_visits.push_back(Visit{pos, isDecreasing});
    }

    [[nodiscard]] mc::i32 getLevel(mc::i64 pos) const override {
        auto it = m_levels.find(pos);
        if (it == m_levels.end()) {
            return 15;
        }
        return it->second;
    }

    void setLevel(mc::i64 pos, mc::i32 level) override {
        m_levels[pos] = level;
    }

    [[nodiscard]] mc::i32 getEdgeLevel(mc::i64,
                                       mc::i64,
                                       mc::i32 startLevel) override {
        return startLevel;
    }

private:
    std::unordered_map<mc::i64, mc::i32> m_levels;
    std::vector<Visit> m_visits;
};

TEST(LevelBasedGraphQueueTest, IncreaseQueueKeepsFifoAcrossTickBudget) {
    TestLevelBasedGraph graph(8);

    const mc::i64 posA = mc::LightEngineUtils::packPos(0, 64, 0);
    const mc::i64 posB = mc::LightEngineUtils::packPos(1, 64, 0);
    const mc::i64 posC = mc::LightEngineUtils::packPos(2, 64, 0);

    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posA, 3, true);
    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posB, 3, true);
    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posC, 3, true);

    EXPECT_EQ(graph.queuedUpdateSize(), 3);

    graph.processUpdates(2);
    ASSERT_EQ(graph.visits().size(), 2U);
    EXPECT_EQ(graph.visits()[0].pos, posA);
    EXPECT_EQ(graph.visits()[1].pos, posB);
    EXPECT_FALSE(graph.visits()[0].isDecreasing);
    EXPECT_FALSE(graph.visits()[1].isDecreasing);
    EXPECT_EQ(graph.queuedUpdateSize(), 1);
    EXPECT_TRUE(graph.needsUpdate());

    graph.processUpdates(8);
    ASSERT_EQ(graph.visits().size(), 3U);
    EXPECT_EQ(graph.visits()[2].pos, posC);
    EXPECT_EQ(graph.queuedUpdateSize(), 0);
    EXPECT_FALSE(graph.needsUpdate());
}

TEST(LevelBasedGraphQueueTest, IncreaseQueueWrapAroundKeepsOrder) {
    TestLevelBasedGraph graph(3);

    const mc::i64 posA = mc::LightEngineUtils::packPos(0, 70, 0);
    const mc::i64 posB = mc::LightEngineUtils::packPos(1, 70, 0);
    const mc::i64 posC = mc::LightEngineUtils::packPos(2, 70, 0);
    const mc::i64 posD = mc::LightEngineUtils::packPos(3, 70, 0);

    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posA, 4, true);
    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posB, 4, true);
    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posC, 4, true);

    graph.processUpdates(1);
    ASSERT_EQ(graph.visits().size(), 1U);
    EXPECT_EQ(graph.visits()[0].pos, posA);

    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posD, 4, true);

    graph.processUpdates(8);
    ASSERT_EQ(graph.visits().size(), 4U);
    EXPECT_EQ(graph.visits()[1].pos, posB);
    EXPECT_EQ(graph.visits()[2].pos, posC);
    EXPECT_EQ(graph.visits()[3].pos, posD);
}

TEST(LevelBasedGraphQueueTest, CancelUpdateWorksAfterWrapAround) {
    TestLevelBasedGraph graph(3);

    const mc::i64 posA = mc::LightEngineUtils::packPos(0, 80, 0);
    const mc::i64 posB = mc::LightEngineUtils::packPos(1, 80, 0);
    const mc::i64 posC = mc::LightEngineUtils::packPos(2, 80, 0);
    const mc::i64 posD = mc::LightEngineUtils::packPos(3, 80, 0);

    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posA, 5, true);
    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posB, 5, true);
    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posC, 5, true);

    graph.processUpdates(1);
    ASSERT_EQ(graph.visits().size(), 1U);
    EXPECT_EQ(graph.visits()[0].pos, posA);

    graph.scheduleUpdate(mc::LightEngineUtils::ROOT_POS, posD, 5, true);
    graph.cancelUpdate(posC);

    graph.processUpdates(8);
    ASSERT_EQ(graph.visits().size(), 3U);
    EXPECT_EQ(graph.visits()[1].pos, posB);
    EXPECT_EQ(graph.visits()[2].pos, posD);
    EXPECT_EQ(graph.queuedUpdateSize(), 0);
    EXPECT_FALSE(graph.needsUpdate());
}

} // namespace