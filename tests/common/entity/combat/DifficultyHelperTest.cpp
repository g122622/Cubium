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

#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"

using namespace mc::entity::combat;
using namespace mc;

// ============================================================================
// DifficultyHelper 测试
// ============================================================================

// ========== 玩家伤害调整测试 ==========

TEST(DifficultyHelperTest, AdjustPlayerDamage_Peaceful)
{
    // 和平模式玩家不受怪物伤害
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Peaceful, 10.0f), 0.0f);
}

TEST(DifficultyHelperTest, AdjustPlayerDamage_Easy)
{
    // MC 1.16.5: min(damage/2 + 1, damage)
    // damage=10 -> min(5+1, 10) = 6
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 10.0f), 6.0f);
    // damage=2 -> min(1+1, 2) = 2
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 2.0f), 2.0f);
    // damage=1 -> min(0.5+1, 1) = 1
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Easy, 1.0f), 1.0f);
}

TEST(DifficultyHelperTest, AdjustPlayerDamage_Normal)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Normal, 10.0f), 10.0f);
}

TEST(DifficultyHelperTest, AdjustPlayerDamage_Hard)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::adjustPlayerDamage(Difficulty::Hard, 10.0f), 15.0f);
}

// ========== 怪物伤害调整测试 ==========

TEST(DifficultyHelperTest, GetMobDamageAdjustment_Peaceful)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Peaceful), 0.0f);
}

TEST(DifficultyHelperTest, GetMobDamageAdjustment_Easy)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Easy), -2.0f);
}

TEST(DifficultyHelperTest, GetMobDamageAdjustment_Normal)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Normal), 0.0f);
}

TEST(DifficultyHelperTest, GetMobDamageAdjustment_Hard)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(Difficulty::Hard), 2.0f);
}

TEST(DifficultyHelperTest, GetMobDamageAdjustment_ByIntId)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(0), 0.0f);  // Peaceful
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(1), -2.0f); // Easy
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(2), 0.0f);  // Normal
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(3), 2.0f);  // Hard
}

// ========== 饥饿最小生命值测试 ==========

TEST(DifficultyHelperTest, GetStarvationMinHealth_Peaceful)
{
    // 和平模式返回最大生命值，不受饥饿伤害
    EXPECT_FLOAT_EQ(DifficultyHelper::getStarvationMinHealth(Difficulty::Peaceful), 20.0f);
}

TEST(DifficultyHelperTest, GetStarvationMinHealth_Easy)
{
    // 简单模式：5颗心
    EXPECT_FLOAT_EQ(DifficultyHelper::getStarvationMinHealth(Difficulty::Easy), 10.0f);
}

TEST(DifficultyHelperTest, GetStarvationMinHealth_Normal)
{
    // 普通模式：半颗心
    EXPECT_FLOAT_EQ(DifficultyHelper::getStarvationMinHealth(Difficulty::Normal), 1.0f);
}

TEST(DifficultyHelperTest, GetStarvationMinHealth_Hard)
{
    // 困难模式：可以饿死
    EXPECT_FLOAT_EQ(DifficultyHelper::getStarvationMinHealth(Difficulty::Hard), 0.0f);
}

// ========== 火焰机制测试 ==========

TEST(DifficultyHelperTest, GetFireDurationMultiplier_Peaceful)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getFireDurationMultiplier(Difficulty::Peaceful), 0.25f);
}

TEST(DifficultyHelperTest, GetFireDurationMultiplier_Easy)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getFireDurationMultiplier(Difficulty::Easy), 0.5f);
}

TEST(DifficultyHelperTest, GetFireDurationMultiplier_Normal)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getFireDurationMultiplier(Difficulty::Normal), 1.0f);
}

TEST(DifficultyHelperTest, GetFireDurationMultiplier_Hard)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getFireDurationMultiplier(Difficulty::Hard), 1.5f);
}

TEST(DifficultyHelperTest, GetFireSpreadBonus)
{
    EXPECT_EQ(DifficultyHelper::getFireSpreadBonus(Difficulty::Peaceful), 0);
    EXPECT_EQ(DifficultyHelper::getFireSpreadBonus(Difficulty::Easy), 7);
    EXPECT_EQ(DifficultyHelper::getFireSpreadBonus(Difficulty::Normal), 14);
    EXPECT_EQ(DifficultyHelper::getFireSpreadBonus(Difficulty::Hard), 21);
}

// ========== 特殊机制测试 ==========

TEST(DifficultyHelperTest, CanZombieReinforce)
{
    // 只有困难模式僵尸才能召唤增援
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Peaceful));
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Easy));
    EXPECT_FALSE(DifficultyHelper::canZombieReinforce(Difficulty::Normal));
    EXPECT_TRUE(DifficultyHelper::canZombieReinforce(Difficulty::Hard));
}

TEST(DifficultyHelperTest, GetVillagerInfectionChance)
{
    // 和平和简单：0%
    EXPECT_FLOAT_EQ(DifficultyHelper::getVillagerInfectionChance(Difficulty::Peaceful), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getVillagerInfectionChance(Difficulty::Easy), 0.0f);
    // 普通：50%
    EXPECT_FLOAT_EQ(DifficultyHelper::getVillagerInfectionChance(Difficulty::Normal), 0.5f);
    // 困难：100%
    EXPECT_FLOAT_EQ(DifficultyHelper::getVillagerInfectionChance(Difficulty::Hard), 1.0f);
}

TEST(DifficultyHelperTest, GetRaidWaves)
{
    // 和平模式不发生袭击
    EXPECT_EQ(DifficultyHelper::getRaidWaves(Difficulty::Peaceful), 0);
    // 简单：3波
    EXPECT_EQ(DifficultyHelper::getRaidWaves(Difficulty::Easy), 3);
    // 普通：5波
    EXPECT_EQ(DifficultyHelper::getRaidWaves(Difficulty::Normal), 5);
    // 困难：7波
    EXPECT_EQ(DifficultyHelper::getRaidWaves(Difficulty::Hard), 7);
}

TEST(DifficultyHelperTest, AllowsMobSpawning)
{
    // 只有和平模式不允许怪物生成
    EXPECT_FALSE(DifficultyHelper::allowsMobSpawning(Difficulty::Peaceful));
    EXPECT_TRUE(DifficultyHelper::allowsMobSpawning(Difficulty::Easy));
    EXPECT_TRUE(DifficultyHelper::allowsMobSpawning(Difficulty::Normal));
    EXPECT_TRUE(DifficultyHelper::allowsMobSpawning(Difficulty::Hard));
}

TEST(DifficultyHelperTest, GetRegionalDifficultyBase)
{
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Peaceful), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Easy), 0.75f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Normal), 1.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getRegionalDifficultyBase(Difficulty::Hard), 1.0f);
}

// ========== 远程攻击不精确度测试 ==========

TEST(DifficultyHelperTest, GetRangedAttackInaccuracy_Peaceful)
{
    // 和平模式：14 - 0*4 = 14
    EXPECT_FLOAT_EQ(DifficultyHelper::getRangedAttackInaccuracy(Difficulty::Peaceful), 14.0f);
}

TEST(DifficultyHelperTest, GetRangedAttackInaccuracy_Easy)
{
    // 简单模式：14 - 1*4 = 10
    EXPECT_FLOAT_EQ(DifficultyHelper::getRangedAttackInaccuracy(Difficulty::Easy), 10.0f);
}

TEST(DifficultyHelperTest, GetRangedAttackInaccuracy_Normal)
{
    // 普通模式：14 - 2*4 = 6
    EXPECT_FLOAT_EQ(DifficultyHelper::getRangedAttackInaccuracy(Difficulty::Normal), 6.0f);
}

TEST(DifficultyHelperTest, GetRangedAttackInaccuracy_Hard)
{
    // 困难模式：14 - 3*4 = 2
    EXPECT_FLOAT_EQ(DifficultyHelper::getRangedAttackInaccuracy(Difficulty::Hard), 2.0f);
}

// ========== 边界测试 ==========

TEST(DifficultyHelperTest, InvalidDifficultyId)
{
    // 无效的难度ID应返回普通难度的默认值
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(-1), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(4), 0.0f);
    EXPECT_FLOAT_EQ(DifficultyHelper::getMobDamageAdjustment(100), 0.0f);
}
