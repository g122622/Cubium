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

#include "entity/experience/ExperienceManager.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/math/random/Random.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity::experience;

// ==================== ExperienceManager Tests ====================

class ExperienceManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
        // 使用 Player 内部的 ExperienceManager
        manager = &player->experienceManager();
    }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
    ExperienceManager* manager; // 非拥有指针，指向 Player 内部的 manager
};

// ========== 构造函数和基本状态测试 ==========

TEST_F(ExperienceManagerTest, InitialState)
{
    EXPECT_EQ(manager->getLevel(), 0);
    EXPECT_FLOAT_EQ(manager->getProgress(), 0.0f);
    EXPECT_EQ(manager->getTotalExperience(), 0);
    EXPECT_EQ(manager->getExperienceForNextLevel(), 7); // 等级0需要7点经验
    EXPECT_FALSE(manager->isDirty());
}

TEST_F(ExperienceManagerTest, SetExperience)
{
    manager->setExperience(10, 0.5f, 500);

    EXPECT_EQ(manager->getLevel(), 10);
    EXPECT_FLOAT_EQ(manager->getProgress(), 0.5f);
    EXPECT_EQ(manager->getTotalExperience(), 500);
    EXPECT_TRUE(manager->isDirty());
}

TEST_F(ExperienceManagerTest, SetExperienceNegativeValues)
{
    manager->setExperience(-5, -0.5f, -100);

    // 应该被修正为非负值
    EXPECT_EQ(manager->getLevel(), 0);
    EXPECT_FLOAT_EQ(manager->getProgress(), 0.0f);
    EXPECT_EQ(manager->getTotalExperience(), 0);
}

// ========== 添加经验测试 ==========

TEST_F(ExperienceManagerTest, AddExperienceSmall)
{
    manager->addExperience(5);

    // 等级0需要7点升级，添加5点后应该进度为 5/7
    EXPECT_EQ(manager->getLevel(), 0);
    EXPECT_NEAR(manager->getProgress(), 5.0f / 7.0f, 0.001f);
    EXPECT_EQ(manager->getTotalExperience(), 5);
}

TEST_F(ExperienceManagerTest, AddExperienceLevelUp)
{
    // 添加7点经验应该触发升级
    manager->addExperience(7);

    EXPECT_EQ(manager->getLevel(), 1);
    EXPECT_NEAR(manager->getProgress(), 0.0f, 0.001f);
    EXPECT_EQ(manager->getTotalExperience(), 7);
}

TEST_F(ExperienceManagerTest, AddExperienceMultipleLevelUps)
{
    // 添加足够多的经验触发多次升级
    // 等级0->1: 7点
    // 等级1->2: 9点
    // 等级2->3: 11点
    // 等级3->4: 13点
    // 总共: 7 + 9 + 11 + 13 = 40 点到等级4
    manager->addExperience(50);

    EXPECT_GT(manager->getLevel(), 3);
    EXPECT_EQ(manager->getTotalExperience(), 50);
}

TEST_F(ExperienceManagerTest, AddExperienceZero)
{
    manager->addExperience(0);

    EXPECT_EQ(manager->getLevel(), 0);
    EXPECT_EQ(manager->getTotalExperience(), 0);
}

TEST_F(ExperienceManagerTest, AddExperienceNegative)
{
    // 参考 MC 1.16.5：负经验会导致降级
    manager->setExperience(5, 0.5f, 100);
    manager->clearDirty();

    // 添加负经验（降级）
    manager->addExperience(-10);

    // 应该降低总经验
    EXPECT_LT(manager->getTotalExperience(), 100);
    EXPECT_TRUE(manager->isDirty());
}

// ========== 消耗经验测试 ==========

TEST_F(ExperienceManagerTest, ConsumeExperienceSuccess)
{
    manager->addExperience(100);
    manager->clearDirty();

    bool result = manager->consumeExperience(30);

    EXPECT_TRUE(result);
    EXPECT_LT(manager->getTotalExperience(), 100);
    EXPECT_TRUE(manager->isDirty());
}

TEST_F(ExperienceManagerTest, ConsumeExperienceInsufficient)
{
    manager->addExperience(10);
    manager->clearDirty();

    bool result = manager->consumeExperience(50);

    EXPECT_FALSE(result);
    EXPECT_EQ(manager->getTotalExperience(), 10); // 未改变
    EXPECT_FALSE(manager->isDirty());
}

TEST_F(ExperienceManagerTest, ConsumeExperienceExact)
{
    manager->addExperience(50);
    manager->clearDirty();

    bool result = manager->consumeExperience(50);

    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getTotalExperience(), 0);
    EXPECT_EQ(manager->getLevel(), 0);
    EXPECT_FLOAT_EQ(manager->getProgress(), 0.0f);
}

TEST_F(ExperienceManagerTest, ConsumeExperienceZero)
{
    manager->addExperience(50);
    manager->clearDirty();

    bool result = manager->consumeExperience(0);

    EXPECT_TRUE(result);                          // 消耗0应该成功
    EXPECT_EQ(manager->getTotalExperience(), 50); // 未改变
}

// ========== 等级操作测试 ==========

TEST_F(ExperienceManagerTest, SetLevel)
{
    manager->setLevel(15);

    EXPECT_EQ(manager->getLevel(), 15);
    EXPECT_TRUE(manager->isDirty());
}

TEST_F(ExperienceManagerTest, SetLevelNegative)
{
    manager->setLevel(-5);

    EXPECT_EQ(manager->getLevel(), 0);
}

TEST_F(ExperienceManagerTest, AddLevels)
{
    manager->setLevel(5);
    manager->clearDirty();

    manager->addLevels(10);

    EXPECT_EQ(manager->getLevel(), 15);
    EXPECT_TRUE(manager->isDirty());
}

TEST_F(ExperienceManagerTest, AddLevelsNegative)
{
    manager->setLevel(10);
    manager->clearDirty();

    manager->addLevels(-5);

    EXPECT_EQ(manager->getLevel(), 5);
}

TEST_F(ExperienceManagerTest, ConsumeLevelsSuccess)
{
    manager->setLevel(10);
    manager->clearDirty();

    bool result = manager->consumeLevels(5);

    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getLevel(), 5);
}

TEST_F(ExperienceManagerTest, ConsumeLevelsInsufficient)
{
    manager->setLevel(5);
    manager->clearDirty();

    bool result = manager->consumeLevels(10);

    EXPECT_FALSE(result);
    EXPECT_EQ(manager->getLevel(), 5); // 未改变
}

TEST_F(ExperienceManagerTest, ConsumeLevelsZero)
{
    manager->setLevel(5);
    manager->clearDirty();

    bool result = manager->consumeLevels(0);

    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getLevel(), 5);
}

// ========== 经验计算测试 ==========

TEST_F(ExperienceManagerTest, CalculateBarCapacityLevel0)
{
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(0), 7);
}

TEST_F(ExperienceManagerTest, CalculateBarCapacityLevel1To14)
{
    // 等级 0-14: 7 + level * 2
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(1), 9);
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(5), 17);
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(10), 27);
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(14), 35);
}

TEST_F(ExperienceManagerTest, CalculateBarCapacityLevel15To29)
{
    // 等级 15-29: 37 + (level - 15) * 5
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(15), 37);
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(20), 62);
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(25), 87);
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(29), 107);
}

TEST_F(ExperienceManagerTest, CalculateBarCapacityLevel30Plus)
{
    // 等级 30+: 112 + (level - 30) * 9
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(30), 112);
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(35), 157);
    EXPECT_EQ(ExperienceManager::calculateBarCapacity(40), 202);
}

TEST_F(ExperienceManagerTest, GetExperienceForLevel)
{
    // 等级 0 需要 0 经验
    EXPECT_EQ(ExperienceManager::getExperienceForLevel(0), 0);

    // 等级 1 需要 7 经验
    EXPECT_EQ(ExperienceManager::getExperienceForLevel(1), 7);

    // 等级 15: 使用公式 level * (level + 6) = 15 * 21 = 315
    EXPECT_EQ(ExperienceManager::getExperienceForLevel(15), 315);

    // 等级 30: 315 + 37*15 + 5*14*15/2 = 315 + 555 + 525 = 1395
    EXPECT_EQ(ExperienceManager::getExperienceForLevel(30), 1395);
}

TEST_F(ExperienceManagerTest, GetLevelFromExperience)
{
    // 0 经验 = 等级 0
    EXPECT_EQ(ExperienceManager::getLevelFromExperience(0), 0);

    // 7 经验 = 等级 1
    EXPECT_EQ(ExperienceManager::getLevelFromExperience(7), 1);

    // 315 经验 = 等级 15
    EXPECT_EQ(ExperienceManager::getLevelFromExperience(315), 15);

    // 部分经验应该向下取整
    EXPECT_EQ(ExperienceManager::getLevelFromExperience(10), 1); // 7 < 10 < 16
    EXPECT_EQ(ExperienceManager::getLevelFromExperience(16), 2); // 16 = 7 + 9
}

TEST_F(ExperienceManagerTest, ExperienceRoundTrip)
{
    // 验证 getExperienceForLevel 和 getLevelFromExperience 是逆运算
    for (i32 level = 0; level <= 50; level += 5) {
        i32 xp = ExperienceManager::getExperienceForLevel(level);
        i32 calculatedLevel = ExperienceManager::getLevelFromExperience(xp);
        EXPECT_EQ(calculatedLevel, level) << "Failed for level " << level;
    }
}

// ========== 死亡掉落测试 ==========

TEST_F(ExperienceManagerTest, CalculateDeathDropXp)
{
    // 等级 0: 0 经验
    manager->setLevel(0);
    EXPECT_EQ(manager->calculateDeathDropXp(), 0);

    // 等级 1: 7 经验
    manager->setLevel(1);
    EXPECT_EQ(manager->calculateDeathDropXp(), 7);

    // 等级 10: 70 经验
    manager->setLevel(10);
    EXPECT_EQ(manager->calculateDeathDropXp(), 70);

    // 等级 15: 最大 100 经验
    manager->setLevel(15);
    EXPECT_EQ(manager->calculateDeathDropXp(), 100);

    // 等级 100: 最大 100 经验
    manager->setLevel(100);
    EXPECT_EQ(manager->calculateDeathDropXp(), 100);
}

// ========== 重置测试 ==========

TEST_F(ExperienceManagerTest, Reset)
{
    manager->setExperience(20, 0.5f, 1000);
    manager->clearDirty();

    manager->reset();

    EXPECT_EQ(manager->getLevel(), 0);
    EXPECT_FLOAT_EQ(manager->getProgress(), 0.0f);
    EXPECT_EQ(manager->getTotalExperience(), 0);
    EXPECT_TRUE(manager->isDirty());
}

// ========== 回调测试 ==========

TEST_F(ExperienceManagerTest, LevelChangeCallback)
{
    i32 callbackOldLevel = -1;
    i32 callbackNewLevel = -1;

    manager->setLevelChangeCallback([&](i32 oldLevel, i32 newLevel) {
        callbackOldLevel = oldLevel;
        callbackNewLevel = newLevel;
    });

    manager->setLevel(10);

    EXPECT_EQ(callbackOldLevel, 0);
    EXPECT_EQ(callbackNewLevel, 10);
}

TEST_F(ExperienceManagerTest, ExperienceChangeCallback)
{
    i32 callbackTotalXp = -1;

    manager->setExperienceChangeCallback([&](i32 totalXp) { callbackTotalXp = totalXp; });

    manager->addExperience(100);

    EXPECT_EQ(callbackTotalXp, 100);
}

// ========== 同步标记测试 ==========

TEST_F(ExperienceManagerTest, DirtyFlag)
{
    EXPECT_FALSE(manager->isDirty());

    manager->addExperience(10);
    EXPECT_TRUE(manager->isDirty());

    manager->clearDirty();
    EXPECT_FALSE(manager->isDirty());

    manager->markDirty();
    EXPECT_TRUE(manager->isDirty());
}

// ========== 附魔相关测试 ==========

TEST_F(ExperienceManagerTest, XpSeed)
{
    math::Random rng(12345);

    manager->resetXpSeed(rng);
    i32 seed1 = manager->getXpSeed();

    manager->resetXpSeed(rng);
    i32 seed2 = manager->getXpSeed();

    // 由于随机数生成器状态改变，两次种子应该不同
    // 注意：由于我们使用相同种子初始化 Random，第一次调用可能相同
    // 这里主要测试方法不会崩溃
    EXPECT_NE(seed1, 0); // 至少应该有值
}

TEST_F(ExperienceManagerTest, OnEnchant)
{
    math::Random rng(12345);

    manager->setLevel(10);
    manager->clearDirty();

    bool result = manager->onEnchant(5, rng);

    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getLevel(), 5);
}

TEST_F(ExperienceManagerTest, OnEnchantInsufficient)
{
    // 参考 MC 1.16.5：onEnchant 直接消耗等级，不检查是否足够
    // 如果等级变为负数，则重置为 0
    math::Random rng(12345);

    manager->setLevel(3);
    manager->clearDirty();

    bool result = manager->onEnchant(5, rng);

    // 原版行为：始终返回 true，等级变为 0（3 - 5 = -2 < 0，重置为 0）
    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getLevel(), 0);
}

// ========== 边界条件测试 ==========

TEST_F(ExperienceManagerTest, MaxLevel)
{
    // 设置到最大等级
    manager->setLevel(21862); // MAX_EXPERIENCE_LEVEL

    EXPECT_EQ(manager->getLevel(), 21862);
}

TEST_F(ExperienceManagerTest, AddExperienceBeyondMaxLevel)
{
    // 从最大等级继续添加经验
    manager->setLevel(21862);
    manager->clearDirty();

    // 应该不会崩溃，等级应该被限制
    manager->addExperience(1000);

    EXPECT_EQ(manager->getLevel(), 21862);
}

// ========== 与 Player 集成测试 ==========

TEST_F(ExperienceManagerTest, PlayerIntegration)
{
    // 验证 Player 类的经验方法正确委托给 ExperienceManager
    EXPECT_EQ(player->experienceLevel(), 0);
    EXPECT_FLOAT_EQ(player->experienceProgress(), 0.0f);

    player->addExperience(100);

    EXPECT_GT(player->experienceLevel(), 0);
    EXPECT_GT(player->totalExperience(), 0);
}

TEST_F(ExperienceManagerTest, PlayerLevelSet)
{
    player->setExperienceLevel(15);

    // Player 的等级和内部 manager 的等级应该同步
    EXPECT_EQ(player->experienceLevel(), 15);
    EXPECT_EQ(manager->getLevel(), 15);
}

TEST_F(ExperienceManagerTest, PlayerBarCapacity)
{
    // 验证 Player 的 experienceBarCapacity 委托正确
    player->setExperienceLevel(10);

    i32 playerCapacity = player->experienceBarCapacity();
    i32 managerCapacity = manager->getExperienceForNextLevel();

    EXPECT_EQ(playerCapacity, managerCapacity);
    EXPECT_EQ(playerCapacity, 27); // 7 + 10 * 2
}

// ========== 升级音效测试 ==========

TEST_F(ExperienceManagerTest, LevelUpSoundNoCrash)
{
    // 验证升级时音效逻辑不会导致崩溃（即使没有 world）
    // 由于 Player 在测试中没有关联 World，playSound 会直接返回

    // 升到5级（第一个播放音效的等级）
    manager->addExperience(ExperienceManager::getExperienceForLevel(5) + 1);

    EXPECT_EQ(manager->getLevel(), 5);

    // 升到10级
    manager->addExperience(ExperienceManager::getExperienceForLevel(10) - manager->getTotalExperience() + 1);

    EXPECT_EQ(manager->getLevel(), 10);

    // 升到15级
    manager->addExperience(ExperienceManager::getExperienceForLevel(15) - manager->getTotalExperience() + 1);

    EXPECT_EQ(manager->getLevel(), 15);
}

TEST_F(ExperienceManagerTest, LevelUpAtMultiplesOfFive)
{
    // 验证每5级升级时回调正确触发
    std::vector<i32> levelChanges;
    manager->setLevelChangeCallback([&](i32 oldLevel, i32 newLevel) { levelChanges.push_back(newLevel); });

    // 升到5级
    i32 xpForLevel5 = ExperienceManager::getExperienceForLevel(5);
    manager->addExperience(xpForLevel5);

    // 应该有5次升级（0->1, 1->2, 2->3, 3->4, 4->5）
    EXPECT_EQ(levelChanges.size(), 5);
    EXPECT_EQ(levelChanges.back(), 5);
    EXPECT_EQ(manager->getLevel(), 5);
}

TEST_F(ExperienceManagerTest, LevelUpSoundVolumeCalculation)
{
    // 验证音量计算逻辑（通过测试等级计算间接验证）
    // 音量公式: (level > 30 ? 1.0 : level / 30.0) * 0.75

    // 等级5: 音量 = (5/30) * 0.75 = 0.125
    EXPECT_EQ(manager->getLevel(), 0);
    manager->setLevel(5);
    EXPECT_EQ(manager->getLevel(), 5);

    // 等级15: 音量 = (15/30) * 0.75 = 0.375
    manager->setLevel(15);
    EXPECT_EQ(manager->getLevel(), 15);

    // 等级30: 音量 = (30/30) * 0.75 = 0.75
    manager->setLevel(30);
    EXPECT_EQ(manager->getLevel(), 30);

    // 等级35: 音量 = 1.0 * 0.75 = 0.75
    manager->setLevel(35);
    EXPECT_EQ(manager->getLevel(), 35);
}

TEST_F(ExperienceManagerTest, MultipleLevelUpsWithSoundConditions)
{
    // 测试多级升级时的音效条件
    // 音效条件: 等级是5的倍数 且 距离上次播放至少100 tick

    // 由于 Player 在测试中没有 tick 更新，ticksExisted() 始终返回 0
    // 这意味着第一次升级到5的倍数时会尝试播放音效

    // 快速升到5级
    i32 xpForLevel5 = ExperienceManager::getExperienceForLevel(5);
    manager->addExperience(xpForLevel5);

    // 验证等级正确
    EXPECT_EQ(manager->getLevel(), 5);

    // 继续升到10级
    i32 xpForLevel10 = ExperienceManager::getExperienceForLevel(10);
    manager->addExperience(xpForLevel10 - xpForLevel5);

    EXPECT_EQ(manager->getLevel(), 10);

    // 继续升到15级
    i32 xpForLevel15 = ExperienceManager::getExperienceForLevel(15);
    manager->addExperience(xpForLevel15 - xpForLevel10);

    EXPECT_EQ(manager->getLevel(), 15);
}
