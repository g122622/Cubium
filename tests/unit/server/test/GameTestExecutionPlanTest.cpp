#include "server/test/base/execution/GameTestExecutionPlan.hpp"
#include <algorithm>
#include <chrono>
#include <limits>
#include <set>
#include <gtest/gtest.h>

namespace mc::test {
namespace {

TEST(GameTestExecutionPlanTest, ValidatesExplicitShardBoundaries)
{
    EXPECT_TRUE(GameTestExecutionPlan::parseShard("0/24"));
    EXPECT_TRUE(GameTestExecutionPlan::parseShard("23/24"));
    for (const auto input : {"24/24", "0/0", "-1/24", "0/65", "1/24junk", "1/2/3", "", "4294967296/24"}) {
        EXPECT_FALSE(GameTestExecutionPlan::parseShard(input)) << input;
    }
}

TEST(GameTestExecutionPlanTest, PartitionsAreCompleteDisjointAndIndependentOfRegistrationOrder)
{
    const std::vector<GameTestWorkItem> items = {{"long", 8000},
        {"medium", 1200},
        {"short", 20},
        {"skip", 0},
        {"another-long", 6500},
        {"another-short", 10},
        {"last", 200}};
    auto reversed = items;
    std::reverse(reversed.begin(), reversed.end());
    std::set<std::string> all;
    for (u32 shard = 0; shard < 3; ++shard) {
        std::set<std::string> first;
        std::set<std::string> second;
        for (const auto index : GameTestExecutionPlan::select(items, {shard, 3})) {
            EXPECT_TRUE(all.insert(items[index].id).second);
            first.insert(items[index].id);
        }
        for (const auto index : GameTestExecutionPlan::select(reversed, {shard, 3})) {
            second.insert(reversed[index].id);
        }
        EXPECT_EQ(first, second);
        EXPECT_FALSE(first.empty());
    }
    EXPECT_EQ(all.size(), items.size());
}

TEST(GameTestExecutionPlanTest, BatchBudgetHonorsLongDeclarationsAndRetainsSuiteCap)
{
    using namespace std::chrono_literals;
    EXPECT_EQ(GameTestExecutionPlan::batchBudget(0), 60s);
    EXPECT_EQ(GameTestExecutionPlan::batchBudget(1800), 240s);
    EXPECT_EQ(GameTestExecutionPlan::batchBudget(8000), 860s);
    EXPECT_EQ(GameTestExecutionPlan::batchBudget(std::numeric_limits<u64>::max()), 1800s);
}

TEST(GameTestExecutionPlanTest, TickDeadlineKeepsOrdinaryTickLimitAndCapsSynchronousGeneration)
{
    using namespace std::chrono_literals;
    const auto now = std::chrono::steady_clock::time_point{};
    const auto batchDeadline = now + GameTestExecutionPlan::batchBudget(200);
    const auto suiteDeadline = now + 30min;
    EXPECT_EQ(GameTestExecutionPlan::tickDeadline(now, std::min(batchDeadline, suiteDeadline), false), now + 2s);
    EXPECT_EQ(GameTestExecutionPlan::tickDeadline(now, std::min(batchDeadline, suiteDeadline), true), batchDeadline);
    // 批次或整轮只剩半秒时，普通与同步生成 tick 都不能获得额外宽限。
    for (const bool synchronousWork : {false, true}) {
        EXPECT_EQ(GameTestExecutionPlan::tickDeadline(now, now + 500ms, synchronousWork), now + 500ms);
        EXPECT_EQ(GameTestExecutionPlan::tickDeadline(suiteDeadline, suiteDeadline, synchronousWork), suiteDeadline);
    }
}

} // namespace
} // namespace mc::test
