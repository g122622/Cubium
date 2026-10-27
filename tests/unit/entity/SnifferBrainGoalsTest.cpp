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

// 嗅探兽状态机目标链测试（对齐 vanilla SnifferAi 的 IDLE / SNIFF / DIG 活动）。
//
// 覆盖：
//   - SnifferSniffGoal：仅在 Idling、非幼年、canSniff 时启动，持续 40~80 tick 后
//     记录嗅探目标并回到 Idling（对齐 Sniffing.stop 在 timedOut 时设置 SNIFFER_SNIFFING_TARGET）。
//   - SnifferSearchGoal：有嗅探目标且可嗅探时启动进入 Searching，找到可挖掘方块后结束并
//     保留嗅探目标（对齐 Searching.stop 设置 SNIFFER_DIGGING）。
//   - SnifferHappyGoal：开心标记存在时执行一次 FeelingHappy 并清除标记。
//   - SnifferDigGoal：无嗅探目标时不启动（挖掘必须由嗅探→搜索链路触发）。
//   - 已探索位置记忆：storeExploredPosition 去重、最多保留 20 条并最新在前。
//
// Ref: vanilla net/minecraft/world/entity/animal/sniffer/SnifferAi.java
// Ref: vanilla net/minecraft/world/entity/animal/sniffer/Sniffer.java（canSniff/canDig/storeExploredPosition）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/goals/special/SnifferGoals.hpp"
#include "common/entity/entities/passive/special/SnifferEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <vector>

using namespace mc;

namespace {

// BaseTestWorld 默认构造为 protected，派生公开以作 fixture 成员。
class SnifferTestWorld final : public mc::test::BaseTestWorld {
public:
    SnifferTestWorld() = default;

    /// 除显式放置的可挖掘方块外，其余位置为空。
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        for (const BlockPos& diggable : diggablePositions) {
            if (diggable == pos) {
                return &VanillaBlocks::GRASS_BLOCK->defaultState();
            }
        }
        return nullptr;
    }

    /// 可挖掘方块位置（草方块，属于 #minecraft:sniffer_diggable_block）。
    std::vector<BlockPos> diggablePositions;
};

} // namespace

class SnifferBrainGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    SnifferTestWorld m_world;
};

TEST_F(SnifferBrainGoalsTest, SniffGoal_RunsFromIdlingAndRecordsSniffingTarget)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);
    sniffer.setPosition(0.5, 64.0, 0.5);
    sniffer.setHealth(sniffer.maxHealth());
    sniffer.setOnGround(true);

    entity::ai::goal::SnifferSniffGoal goal(&sniffer);
    ASSERT_TRUE(goal.shouldExecute());
    goal.startExecuting();
    EXPECT_EQ(sniffer.getState(), SnifferEntity::State::Sniffing);

    // 持续 tick 直至自然结束（40~80 tick 闭区间）。
    i32 ticks = 0;
    while (goal.shouldContinueExecuting()) {
        ASSERT_LT(++ticks, 1000) << "sniff goal did not terminate";
    }
    EXPECT_GE(ticks + 1, entity::ai::goal::SnifferSniffGoal::MIN_TICKS);
    EXPECT_LE(ticks + 1, entity::ai::goal::SnifferSniffGoal::MAX_TICKS);

    goal.resetTask();
    // 自然结束后回到 Idling，并记录嗅探目标（可挖掘）以驱动后续搜索。
    EXPECT_EQ(sniffer.getState(), SnifferEntity::State::Idling);
    EXPECT_TRUE(sniffer.hasSniffingTarget());
}

TEST_F(SnifferBrainGoalsTest, SniffGoal_NotStartedForBabyOrNonIdling)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);
    sniffer.setPosition(0.5, 64.0, 0.5);
    sniffer.setHealth(sniffer.maxHealth());
    sniffer.setOnGround(true);

    entity::ai::goal::SnifferSniffGoal goal(&sniffer);

    // 处于嗅探/闻气味等非空闲状态：不启动。
    sniffer.setState(SnifferEntity::State::Scenting);
    EXPECT_FALSE(goal.shouldExecute());

    // 幼年嗅探兽：不启动（对齐 Sniffing.checkExtraStartConditions 的 !isBaby()）。
    sniffer.setState(SnifferEntity::State::Idling);
    sniffer.setChild(true);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(SnifferBrainGoalsTest, SearchGoal_FindsDiggableBlockAndKeepsTarget)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);
    sniffer.setPosition(0.5, 64.0, 0.5);
    sniffer.setHealth(sniffer.maxHealth());
    sniffer.setOnGround(true);

    m_world.diggablePositions.push_back(BlockPos(3, 63, 3));

    sniffer.setSniffingTarget(true);
    entity::ai::goal::SnifferSearchGoal goal(&sniffer);
    ASSERT_TRUE(goal.shouldExecute());
    goal.startExecuting();
    EXPECT_EQ(sniffer.getState(), SnifferEntity::State::Searching);

    goal.tick();
    // 找到挖掘点：退出 Searching 并保留嗅探目标供 SnifferDigGoal 接管。
    EXPECT_EQ(sniffer.getState(), SnifferEntity::State::Idling);
    EXPECT_FALSE(goal.shouldContinueExecuting());
    goal.resetTask();
    EXPECT_TRUE(sniffer.hasSniffingTarget());
}

TEST_F(SnifferBrainGoalsTest, SearchGoal_NoDiggableBlock_EndsAndClearsTarget)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);
    sniffer.setPosition(0.5, 64.0, 0.5);
    sniffer.setHealth(sniffer.maxHealth());
    sniffer.setOnGround(true);

    sniffer.setSniffingTarget(true);
    entity::ai::goal::SnifferSearchGoal goal(&sniffer);
    ASSERT_TRUE(goal.shouldExecute());
    goal.startExecuting();
    goal.tick();
    // 周围没有可挖掘方块：搜索不到结果。
    EXPECT_TRUE(goal.shouldContinueExecuting());

    goal.resetTask();
    EXPECT_FALSE(sniffer.hasSniffingTarget());
}

TEST_F(SnifferBrainGoalsTest, HappyGoal_RunsOnceAndClearsFlag)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);
    sniffer.setHealth(sniffer.maxHealth());

    entity::ai::goal::SnifferHappyGoal goal(&sniffer);
    EXPECT_FALSE(goal.shouldExecute());

    sniffer.setHappy(true);
    ASSERT_TRUE(goal.shouldExecute());
    goal.startExecuting();
    EXPECT_EQ(sniffer.getState(), SnifferEntity::State::FeelingHappy);

    goal.resetTask();
    EXPECT_EQ(sniffer.getState(), SnifferEntity::State::Idling);
    EXPECT_FALSE(sniffer.isHappy());
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(SnifferBrainGoalsTest, DigGoal_RequiresSniffingTarget)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);
    sniffer.setPosition(0.5, 64.0, 0.5);
    sniffer.setHealth(sniffer.maxHealth());
    sniffer.setOnGround(true);

    m_world.diggablePositions.push_back(BlockPos(1, 63, 0));

    entity::ai::goal::SnifferDigGoal goal(&sniffer, 1.0);
    // 未经过嗅探→搜索链路（无嗅探目标）：不启动挖掘。
    EXPECT_FALSE(goal.shouldExecute());

    sniffer.setSniffingTarget(true);
    EXPECT_TRUE(goal.shouldExecute());
}

TEST_F(SnifferBrainGoalsTest, ExploredPositions_DeduplicatedNewestFirstAndCapped)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);

    sniffer.storeExploredPosition(BlockPos(0, 0, 0));
    sniffer.storeExploredPosition(BlockPos(1, 0, 0));
    EXPECT_TRUE(sniffer.isExploredPosition(BlockPos(0, 0, 0)));
    EXPECT_TRUE(sniffer.isExploredPosition(BlockPos(1, 0, 0)));
    EXPECT_FALSE(sniffer.isExploredPosition(BlockPos(2, 0, 0)));

    // 重复记录同一位置不产生副本（去重后仍可查询）。
    sniffer.storeExploredPosition(BlockPos(0, 0, 0));
    EXPECT_TRUE(sniffer.isExploredPosition(BlockPos(0, 0, 0)));

    // 最多保留 20 条：记录 30 个不同位置后，最早的若干条被淘汰。
    for (i32 i = 100; i < 130; ++i) {
        sniffer.storeExploredPosition(BlockPos(i, 0, 0));
    }
    EXPECT_TRUE(sniffer.isExploredPosition(BlockPos(129, 0, 0)));
    EXPECT_FALSE(sniffer.isExploredPosition(BlockPos(1, 0, 0)));
}
