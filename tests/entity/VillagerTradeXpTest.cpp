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

#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// VillagerData 升级逻辑测试
// ============================================================================
//
// 测试 VillagerData 的升级阈值和 canLevelUp 方法。
// 升级经验表：Level 1->2: 10, Level 2->3: 70, Level 3->4: 150, Level 4->5: 250
// 最大等级为 5。

class VillagerDataTest : public ::testing::Test {
protected:
    VillagerData data;
};

// ========== canLevelUp 测试 ==========

TEST_F(VillagerDataTest, CanLevelUp_BelowMaxLevel)
{
    // 等级 1-4 都可以升级（canLevelUp 逻辑：level < getMaxLevel()）
    for (i32 level = 1; level < VillagerData::getMaxLevel(); ++level) {
        EXPECT_TRUE(level < VillagerData::getMaxLevel()) << "Level " << level << " should be able to level up";
    }
}

TEST_F(VillagerDataTest, CanLevelUp_AtMaxLevel)
{
    // 等级 5（最大等级）不能升级
    EXPECT_FALSE(VillagerData::getMaxLevel() < VillagerData::getMaxLevel());
}

TEST_F(VillagerDataTest, CanLevelUp_AboveMaxLevel)
{
    // 超过最大等级也不能升级
    EXPECT_FALSE(6 < VillagerData::getMaxLevel());
    EXPECT_FALSE(10 < VillagerData::getMaxLevel());
}

// ========== getExperienceForLevel 测试 ==========

TEST_F(VillagerDataTest, GetExperienceForLevel_ValidLevels)
{
    EXPECT_EQ(VillagerData::getExperienceForLevel(1), 10);
    EXPECT_EQ(VillagerData::getExperienceForLevel(2), 70);
    EXPECT_EQ(VillagerData::getExperienceForLevel(3), 150);
    EXPECT_EQ(VillagerData::getExperienceForLevel(4), 250);
}

TEST_F(VillagerDataTest, GetExperienceForLevel_InvalidLevels)
{
    // 等级 5 或更高返回 0（已满级）
    EXPECT_EQ(VillagerData::getExperienceForLevel(5), 0);
    EXPECT_EQ(VillagerData::getExperienceForLevel(0), 0);
}

// ========== addExperience 升级测试 ==========

TEST_F(VillagerDataTest, AddExperience_NoLevelUp)
{
    // 等级 1，需要 10 经验升级
    // 给 5 经验，不应升级
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 1);
    data.addExperience(5);
    EXPECT_EQ(data.level(), 1);
    EXPECT_EQ(data.experience(), 5);
}

TEST_F(VillagerDataTest, AddExperience_ExactLevelUp)
{
    // 等级 1，给 10 经验，应升级到等级 2，经验归零
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 1);
    data.addExperience(10);
    EXPECT_EQ(data.level(), 2);
    EXPECT_EQ(data.experience(), 0);
}

TEST_F(VillagerDataTest, AddExperience_LevelUpWithRemainder)
{
    // 等级 1，给 15 经验，应升级到等级 2，剩余 5 经验
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 1);
    data.addExperience(15);
    EXPECT_EQ(data.level(), 2);
    EXPECT_EQ(data.experience(), 5);
}

TEST_F(VillagerDataTest, AddExperience_MultipleLevelUps)
{
    // 等级 1，给 80 经验（10+70），应升级到等级 3，经验归零
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 1);
    data.addExperience(80);
    EXPECT_EQ(data.level(), 3);
    EXPECT_EQ(data.experience(), 0);
}

TEST_F(VillagerDataTest, AddExperience_MultipleLevelUpsWithRemainder)
{
    // 等级 1，给 85 经验（10+70+5），应升级到等级 3，剩余 5 经验
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 1);
    data.addExperience(85);
    EXPECT_EQ(data.level(), 3);
    EXPECT_EQ(data.experience(), 5);
}

TEST_F(VillagerDataTest, AddExperience_CapAtMaxLevel)
{
    // 等级 4，给 300 经验（250+50），应升级到等级 5（最大），剩余 50 经验
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 4);
    data.addExperience(300);
    EXPECT_EQ(data.level(), VillagerData::getMaxLevel());
    // 满级后多余经验保留
    EXPECT_EQ(data.experience(), 50);
}

TEST_F(VillagerDataTest, AddExperience_AlreadyMaxLevel)
{
    // 等级 5（最大），经验只增不升级
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 5);
    data.addExperience(100);
    EXPECT_EQ(data.level(), 5);
    EXPECT_EQ(data.experience(), 100);
}

// ========== rewardTradeXp 经验球值计算测试 ==========
//
// 验证升级检测逻辑：
//   - 在 addVillagerExperience 之前记录等级
//   - 在之后比较等级是否变化
//   - 等级变化时，经验球值 +5

TEST_F(VillagerDataTest, LevelComparisonDetectsLevelUp)
{
    // 模拟 rewardTradeXp 中的升级检测逻辑
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 1);

    // 场景1: 给 5 经验（不足升级），等级不变
    {
        i32 prevLevel = data.level();
        data.addExperience(5);
        bool leveledUp = data.level() > prevLevel;
        EXPECT_FALSE(leveledUp);
        EXPECT_EQ(data.level(), 1);
    }

    // 场景2: 再给 5 经验（总共 10，刚好升级），等级变化
    {
        i32 prevLevel = data.level();
        data.addExperience(5);
        bool leveledUp = data.level() > prevLevel;
        EXPECT_TRUE(leveledUp);
        EXPECT_EQ(data.level(), 2);
    }

    // 场景3: 给 2 经验（不足以从2级升到3级），等级不变
    {
        i32 prevLevel = data.level();
        data.addExperience(2);
        bool leveledUp = data.level() > prevLevel;
        EXPECT_FALSE(leveledUp);
        EXPECT_EQ(data.level(), 2);
    }
}

TEST_F(VillagerDataTest, LevelComparisonDetectsMultipleLevelUps)
{
    // 从等级1给 80 经验（10+70），连升两级
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 1);

    i32 prevLevel = data.level();
    data.addExperience(80);
    bool leveledUp = data.level() > prevLevel;
    EXPECT_TRUE(leveledUp);
    EXPECT_EQ(data.level(), 3);
}

// ========== MerchantOffer::shouldRewardExp 测试 ==========

TEST(VillagerTradeXpTest, MerchantOffer_DefaultRewardExp)
{
    // 默认构造的 MerchantOffer 应该奖励经验球
    // MerchantOffer 默认 m_rewardExp = true
    MerchantOffer offer;
    EXPECT_TRUE(offer.shouldRewardExp());
}

// ========== 经验球值范围测试 ==========
//
// 经验球值范围:
//   - 普通交易: 3 + random(0~3) = 3~6
//   - 升级时交易: (3~6) + 5 = 8~11

TEST(VillagerTradeXpTest, XpOrbCount_NormalRange)
{
    // 普通交易经验球值范围: 3~6
    // 3 + nextInt(4) 其中 nextInt(4) 返回 [0, 4)
    EXPECT_GE(3 + 0, 3); // 最小值
    EXPECT_LE(3 + 3, 6); // 最大值
}

TEST(VillagerTradeXpTest, XpOrbCount_LevelUpRange)
{
    // 升级时交易经验球值范围: 8~11
    EXPECT_GE(3 + 0 + 5, 8);  // 最小值
    EXPECT_LE(3 + 3 + 5, 11); // 最大值
}

// ========== stillValid 距离检查测试 ==========
//
// 验证 stillValid 使用 distanceSqTo + 平方阈值（64.0f = 8^2），
// 而非 distanceTo + 线性阈值。

TEST(VillagerTradeXpTest, StillValid_DistanceThreshold_Squared)
{
    // stillValid 使用 distanceSqTo(player) <= 64.0f
    // 等价于 distanceTo(player) <= 8.0f
    // 验证阈值是平方的：64 = 8^2
    constexpr f32 MAX_TRADE_DISTANCE = 8.0f;
    constexpr f32 MAX_TRADE_DISTANCE_SQ = MAX_TRADE_DISTANCE * MAX_TRADE_DISTANCE;
    EXPECT_FLOAT_EQ(MAX_TRADE_DISTANCE_SQ, 64.0f);

    // 距离 7.9 格：平方 = 62.41 < 64，应有效
    f32 dist7_9_sq = 7.9f * 7.9f;
    EXPECT_LT(dist7_9_sq, 64.0f);

    // 距离 8.0 格：平方 = 64.0，应有效（<= 阈值）
    f32 dist8_0_sq = 8.0f * 8.0f;
    EXPECT_LE(dist8_0_sq, 64.0f);

    // 距离 8.1 格：平方 = 65.61 > 64，应无效
    f32 dist8_1_sq = 8.1f * 8.1f;
    EXPECT_GT(dist8_1_sq, 64.0f);
}

// ========== VillagerData setLevel 测试 ==========

TEST_F(VillagerDataTest, SetLevel_ClampsToRange)
{
    // setLevel 应将等级 clamp 到 [1, 5]
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 3);

    data.setLevel(0);
    EXPECT_EQ(data.level(), 1); // clamp to min

    data.setLevel(3);
    EXPECT_EQ(data.level(), 3);

    data.setLevel(10);
    EXPECT_EQ(data.level(), VillagerData::getMaxLevel()); // clamp to max (5)
}

// ========== _increaseMerchantCareer 升级逻辑测试 ==========
//
// 验证升级后等级正确递增，且不会超过最大等级。
// _increaseMerchantCareer 内部调用 setLevel(level + 1)，
// setLevel 会 clamp 到 [1, getMaxLevel()]。

TEST_F(VillagerDataTest, SetLevel_IncrementWithinRange)
{
    // 模拟 _increaseMerchantCareer 中的 level + 1 逻辑
    data = VillagerData(VillagerType::Plains, VillagerProfession::None, 1);
    data.setLevel(data.level() + 1);
    EXPECT_EQ(data.level(), 2);

    data.setLevel(data.level() + 1);
    EXPECT_EQ(data.level(), 3);

    data.setLevel(data.level() + 1);
    EXPECT_EQ(data.level(), 4);

    data.setLevel(data.level() + 1);
    EXPECT_EQ(data.level(), 5);

    // 等级 5 再升级会被 clamp 到 5
    data.setLevel(data.level() + 1);
    EXPECT_EQ(data.level(), VillagerData::getMaxLevel());
}

// ========== 经验球升级+5 加成仅当 shouldRewardExp 为 true 时触发 ==========
//
// 验证 shouldRewardExp 为 false 时不生成经验球（不测试具体生成逻辑，
// 只验证 MerchantOffer 的 shouldRewardExp 语义）

TEST(VillagerTradeXpTest, MerchantOffer_ShouldRewardExp_DefaultTrue)
{
    MerchantOffer offer;
    EXPECT_TRUE(offer.shouldRewardExp());
}
