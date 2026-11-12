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

/**
 * @file WanderingTraderSpawnerStateTest.cpp
 * @brief 流浪商人生成倒计时/概率的存档恢复语义
 *
 * 这两项状态由生成器逐次推进（倒计时每次减 1200，概率在失败后 +25），必须随
 * level.dat 持久化，否则每次重启都会回到"再等一整天、从 25% 开始"。
 *
 * 恢复时的关键判据：level.dat 里两项同时为 0 表示"从未保存过"（新世界或旧存档），
 * 必须按原版规则初始化为 24000 / 25%；若误当作有效值直接采用，倒计时 0 会让下一次
 * tick 立刻进入生成分支，等于世界一启动就刷商人。
 */

#include "server/world/spawn/WanderingTraderSpawner.hpp"

#include <gtest/gtest.h>

using namespace mc;
using mc::server::spawn::WanderingTraderSpawner;

/**
 * @test 默认构造使用原版初始值
 */
TEST(WanderingTraderSpawnerStateTest, DefaultConstructionUsesVanillaDefaults)
{
    const WanderingTraderSpawner spawner;

    EXPECT_EQ(spawner.spawnDelay(), WanderingTraderSpawner::DEFAULT_SPAWN_DELAY);
    EXPECT_EQ(spawner.spawnChance(), WanderingTraderSpawner::INITIAL_SPAWN_CHANCE);
    EXPECT_EQ(spawner.tickDelay(), WanderingTraderSpawner::TICK_DELAY);
}

/**
 * @test 存档中的有效值被原样恢复
 *
 * 恢复后必须保留存档里的倒计时与概率，而不是重置为默认值——否则持久化毫无意义。
 */
TEST(WanderingTraderSpawnerStateTest, SavedStateIsRestored)
{
    const WanderingTraderSpawner spawner(4800, 50);

    EXPECT_EQ(spawner.spawnDelay(), 4800);
    EXPECT_EQ(spawner.spawnChance(), 50);
}

/**
 * @test 两项全为 0 视为"从未保存过"，初始化为默认值
 *
 * 这是最重要的分支：0/0 若被当作有效状态，倒计时已归零，世界一启动就会走生成分支。
 */
TEST(WanderingTraderSpawnerStateTest, BothZeroMeansNeverSavedAndResets)
{
    const WanderingTraderSpawner spawner(0, 0);

    EXPECT_EQ(spawner.spawnDelay(), WanderingTraderSpawner::DEFAULT_SPAWN_DELAY);
    EXPECT_EQ(spawner.spawnChance(), WanderingTraderSpawner::INITIAL_SPAWN_CHANCE);
}

/**
 * @test 只有倒计时归零时保留概率，不整体重置
 *
 * 倒计时减到 0 是正常的运行中状态（下次 tick 就会重置为 24000），
 * 不能因为它是 0 就把概率也一起清零。
 */
TEST(WanderingTraderSpawnerStateTest, ZeroDelayWithNonZeroChanceIsPreserved)
{
    const WanderingTraderSpawner spawner(0, 75);

    EXPECT_EQ(spawner.spawnDelay(), 0);
    EXPECT_EQ(spawner.spawnChance(), 75);
}

/**
 * @test 越界概率被夹回合法区间
 *
 * 存档可能被外部工具改坏。概率 > 100 会让生成无条件通过，负值则永远失败，
 * 两者都会让流浪商人彻底失衡，因此在恢复时就要夹紧。
 */
TEST(WanderingTraderSpawnerStateTest, OutOfRangeChanceIsClamped)
{
    const WanderingTraderSpawner tooHigh(24000, 500);
    EXPECT_EQ(tooHigh.spawnChance(), WanderingTraderSpawner::MAX_SPAWN_CHANCE);

    const WanderingTraderSpawner tooLow(24000, -30);
    EXPECT_EQ(tooLow.spawnChance(), WanderingTraderSpawner::MIN_SPAWN_CHANCE);
}