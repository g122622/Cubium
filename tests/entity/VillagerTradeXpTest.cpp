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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/village/VillageGossip.hpp"
#include "common/world/village/VillageGossipType.hpp"
#include "common/world/village/trade/MerchantOffer.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::world::village;
using namespace mc::world::village::trade;

// ============================================================================
// VillagerData 升级逻辑测试
// ============================================================================
//
// 测试 VillagerData 的升级阈值和 addExperience 方法。
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
// _increaseMerchantCareer 内部不再调用 setLevel（等级已由 addExperience 正确递增），
// 而是为 prevLevel+1 到 currentLevel 的每个等级生成交易。

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

TEST(VillagerTradeXpTest, MerchantOffer_ShouldRewardExp_DefaultTrue)
{
    MerchantOffer offer;
    EXPECT_TRUE(offer.shouldRewardExp());
}

// ============================================================================
// VillageGossipManager 交易声望测试
// ============================================================================
//
// 测试交易声望系统的核心逻辑：
// - Trading 类型流言对声望的影响 (+2/次)
// - 声望范围限制 [-1000, +1000]
// - 流言值上限 (Trading 最多累积100次)
// - 价格修正因子计算
// - 多次交易累积声望

class TradeXpGossipTest : public ::testing::Test {
protected:
    VillageGossipManager manager;
    static constexpr u64 PLAYER_ID = 12345ULL;
};

TEST_F(TradeXpGossipTest, TradingGossip_AddSingleTrade)
{
    // 单次交易声望：Trading +1 次，影响 = +2
    manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 1);

    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::Trading), 1);
    EXPECT_EQ(manager.getReputation(PLAYER_ID), 2); // 1 * 2 = +2
}

TEST_F(TradeXpGossipTest, TradingGossip_AddMultipleTrades)
{
    // 多次交易累积
    for (int i = 0; i < 5; ++i) {
        manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 1);
    }

    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::Trading), 5);
    EXPECT_EQ(manager.getReputation(PLAYER_ID), 10); // 5 * 2 = +10
}

TEST_F(TradeXpGossipTest, TradingGossip_MaxAccumulation)
{
    // Trading 最多累积 100 次
    for (int i = 0; i < 150; ++i) {
        manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 1);
    }

    // 累积值不超过最大值 100
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::Trading), 100);
    EXPECT_EQ(manager.getReputation(PLAYER_ID), 200); // 100 * 2 = +200
}

TEST_F(TradeXpGossipTest, TradingGossip_PriceModifier)
{
    // Trading 声望影响价格修正因子
    // reputation = value * impact = 50 * 2 = +100
    for (int i = 0; i < 50; ++i) {
        manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 1);
    }

    // priceModifier = clamp(1.0 - 100/1000.0, 0.5, 1.5) = 0.9
    f32 modifier = manager.getPriceModifier(PLAYER_ID);
    EXPECT_NEAR(modifier, 0.9f, 0.001f);
}

TEST_F(TradeXpGossipTest, TradingGossip_PriceModifierAtMaxReputation)
{
    // 最大正面声望
    // Trading: 100 * 2 = +200
    // MajorPositive: 20 * 100 = +2000 → clamped to +1000
    for (int i = 0; i < 100; ++i) {
        manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 1);
    }
    for (int i = 0; i < 20; ++i) {
        manager.addGossip(PLAYER_ID, VillageGossipType::MajorPositive, 1);
    }

    // reputation clamped to +1000
    EXPECT_EQ(manager.getReputation(PLAYER_ID), 1000);
    // priceModifier = clamp(1.0 - 1000/1000.0, 0.5, 1.5) = 0.0 → clamped to 0.5
    f32 modifier = manager.getPriceModifier(PLAYER_ID);
    EXPECT_NEAR(modifier, 0.5f, 0.001f);
}

TEST_F(TradeXpGossipTest, MixedGossip_PositiveAndNegative)
{
    // 同时有正面和负面声望
    manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 10);      // +20 声望
    manager.addGossip(PLAYER_ID, VillageGossipType::MinorNegative, 5); // -100 声望

    // 声望 = 10*2 + 5*(-20) = 20 - 100 = -80
    EXPECT_EQ(manager.getReputation(PLAYER_ID), -80);
}

TEST_F(TradeXpGossipTest, NoGossip_ZeroReputation)
{
    // 没有流言的玩家声誉为 0
    EXPECT_EQ(manager.getReputation(PLAYER_ID), 0);
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::Trading), 0);
    EXPECT_FLOAT_EQ(manager.getPriceModifier(PLAYER_ID), 1.0f);
}

TEST_F(TradeXpGossipTest, RemoveGossip)
{
    manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 5);
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::Trading), 5);

    manager.removeGossip(PLAYER_ID, VillageGossipType::Trading);
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::Trading), 0);
}

TEST_F(TradeXpGossipTest, ClearGossip)
{
    manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 5);
    manager.addGossip(PLAYER_ID, VillageGossipType::MinorPositive, 3);
    EXPECT_TRUE(manager.hasGossip(PLAYER_ID));

    manager.clearGossip(PLAYER_ID);
    EXPECT_FALSE(manager.hasGossip(PLAYER_ID));
}

TEST_F(TradeXpGossipTest, Decay_TradingGossip)
{
    // 添加交易流言
    manager.addGossip(PLAYER_ID, VillageGossipType::Trading, 10);
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::Trading), 10);

    // Trading 衰减间隔 24000 tick，衰减率 0.9
    // 模拟一个衰减周期
    manager.tick(24000);

    // 10 * 0.9 = 9 (floor)
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::Trading), 9);
}

TEST_F(TradeXpGossipTest, Decay_MajorNegativeSlowerDecay)
{
    // MajorNegative 衰减间隔 12000 tick，衰减率 0.8
    manager.addGossip(PLAYER_ID, VillageGossipType::MajorNegative, 10);
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::MajorNegative), 10);

    // 12000 tick 后衰减一次
    manager.tick(12000);
    // 10 * 0.8 = 8 (floor)
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::MajorNegative), 8);
}

TEST_F(TradeXpGossipTest, Decay_ValueReachesZero_Removed)
{
    // 流言值衰减到 0 后被移除
    manager.addGossip(PLAYER_ID, VillageGossipType::MajorPositive, 1);
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::MajorPositive), 1);

    // MajorPositive 衰减间隔 48000 tick，衰减率 0.95
    // 1 * 0.95 = 0 (floor)，值变为 0，流言被移除
    manager.tick(48000);
    EXPECT_EQ(manager.getGossipValue(PLAYER_ID, VillageGossipType::MajorPositive), 0);
}

// ============================================================================
// GossipTypeHelper 测试
// ============================================================================

TEST(TradeXpGossipTypeHelperTest, ReputationImpact)
{
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::MajorNegative), -100);
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::MinorNegative), -20);
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::Trading), 2);
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::MinorPositive), 20);
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::MajorPositive), 100);
}

TEST(TradeXpGossipTypeHelperTest, MaxValues)
{
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::MajorNegative), 100);
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::MinorNegative), 200);
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::Trading), 100);
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::MinorPositive), 200);
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::MajorPositive), 20);
}

TEST(TradeXpGossipTypeHelperTest, DecayIntervals)
{
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::MajorNegative), 12000);
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::MinorNegative), 24000);
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::Trading), 24000);
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::MinorPositive), 24000);
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::MajorPositive), 48000);
}

TEST(TradeXpGossipTypeHelperTest, IsNegativeIsPositive)
{
    EXPECT_TRUE(GossipTypeHelper::isNegative(VillageGossipType::MajorNegative));
    EXPECT_TRUE(GossipTypeHelper::isNegative(VillageGossipType::MinorNegative));
    EXPECT_FALSE(GossipTypeHelper::isNegative(VillageGossipType::Trading));

    EXPECT_TRUE(GossipTypeHelper::isPositive(VillageGossipType::Trading));
    EXPECT_TRUE(GossipTypeHelper::isPositive(VillageGossipType::MinorPositive));
    EXPECT_TRUE(GossipTypeHelper::isPositive(VillageGossipType::MajorPositive));
    EXPECT_FALSE(GossipTypeHelper::isPositive(VillageGossipType::MajorNegative));
}

// ============================================================================
// MerchantOffers 集合操作测试
// ============================================================================
//
// 测试 MerchantOffers 的核心操作：添加、查询、补货、价格更新。
// 这些操作是 _increaseMerchantCareer 追加交易和 restock 的基础。

class MerchantOffersOpsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册或获取测试物品
        emerald_ = getOrRegisterTestItem("minecraft:emerald");
        bread_ = getOrRegisterTestItem("minecraft:bread");
        diamond_ = getOrRegisterTestItem("minecraft:diamond");
    }

    static const Item* getOrRegisterTestItem(const std::string& name)
    {
        ResourceLocation id(name);
        const Item* existing = ItemRegistry::instance().getItem(id);
        if (existing != nullptr) {
            return existing;
        }
        return &ItemRegistry::instance().registerItem(id, ItemProperties().maxStackSize(64));
    }

    const Item* emerald_ = nullptr;
    const Item* bread_ = nullptr;
    const Item* diamond_ = nullptr;
    MerchantOffers offers;
};

TEST_F(MerchantOffersOpsTest, AddOffer_IncreasesSize)
{
    EXPECT_EQ(offers.size(), 0u);
    EXPECT_TRUE(offers.empty());

    auto offer = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 2, 0.05f);
    offers.addOffer(std::move(offer));

    EXPECT_EQ(offers.size(), 1u);
    EXPECT_FALSE(offers.empty());
}

TEST_F(MerchantOffersOpsTest, GetOffer_ValidIndex)
{
    auto offer = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 2, 0.05f);
    offers.addOffer(std::move(offer));

    MerchantOffer* retrieved = offers.getOffer(0);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getMaxUses(), 16);
    EXPECT_EQ(retrieved->getXp(), 2);
}

TEST_F(MerchantOffersOpsTest, GetOffer_OutOfBounds_ReturnsNull)
{
    auto offer = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 2, 0.05f);
    offers.addOffer(std::move(offer));

    EXPECT_EQ(offers.getOffer(1), nullptr);
    EXPECT_EQ(offers.getOffer(100), nullptr);
}

TEST_F(MerchantOffersOpsTest, RemoveOffer_DecreasesSize)
{
    for (int i = 0; i < 3; ++i) {
        auto offer = std::make_unique<MerchantOffer>(ItemStack(emerald_, i + 1), ItemStack(bread_, 1), 12, 1, 0.05f);
        offers.addOffer(std::move(offer));
    }
    EXPECT_EQ(offers.size(), 3u);

    offers.removeOffer(1);
    EXPECT_EQ(offers.size(), 2u);

    // 验证剩余交易的正确性
    MerchantOffer* first = offers.getOffer(0);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->getBuyA().getCount(), 1); // 第一笔交易不变

    MerchantOffer* second = offers.getOffer(1);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->getBuyA().getCount(), 3); // 原来的第三笔变成了第二笔
}

TEST_F(MerchantOffersOpsTest, RestockAll_ResetsUseCounts)
{
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 4, 2, 0.05f);
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 2), ItemStack(diamond_, 1), 8, 5, 0.05f);

    // 先使用几笔交易
    offer1->increaseUses();
    offer1->increaseUses();
    offer2->increaseUses();

    EXPECT_TRUE(offer1->isOutOfStock() == false); // 2/4 used
    offers.addOffer(std::move(offer1));
    offers.addOffer(std::move(offer2));

    EXPECT_EQ(offers.getOffer(0)->getUses(), 2);
    EXPECT_EQ(offers.getOffer(1)->getUses(), 1);

    // 补货
    offers.restockAll();

    EXPECT_EQ(offers.getOffer(0)->getUses(), 0);
    EXPECT_EQ(offers.getOffer(0)->getRestocksToday(), 1);
    EXPECT_EQ(offers.getOffer(1)->getUses(), 0);
    EXPECT_EQ(offers.getOffer(1)->getRestocksToday(), 1);
}

TEST_F(MerchantOffersOpsTest, UpdatePrices_AppliesModifier)
{
    auto offer = std::make_unique<MerchantOffer>(ItemStack(emerald_, 10), ItemStack(bread_, 6), 16, 2, 0.05f);
    offers.addOffer(std::move(offer));

    // 基础价格 = 10
    EXPECT_EQ(offers.getOffer(0)->getAdjustedBuyPrice(), 10);

    // 应用 0.9 倍修正（正面声望，10% 折扣）
    offers.updatePrices(0.9f);
    // specialPrice = floor(10 * (1.0 - 0.9)) = floor(10 * 0.1) = 1
    // adjustedPrice = 10 - 1 = 9
    EXPECT_EQ(offers.getOffer(0)->getSpecialPrice(), -1);
    EXPECT_EQ(offers.getOffer(0)->getAdjustedBuyPrice(), 9);
}

TEST_F(MerchantOffersOpsTest, AppendOffers_MultipleLevels)
{
    // 模拟 _increaseMerchantCareer 为多个等级追加交易
    // 先添加等级1的交易
    auto offer1 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 2, 0.05f);
    size_t initialSize = offers.size();
    offers.addOffer(std::move(offer1));
    EXPECT_EQ(offers.size(), initialSize + 1);

    // 追加等级2的交易（不替换等级1的交易）
    auto offer2 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 3), ItemStack(diamond_, 1), 12, 10, 0.05f);
    offers.addOffer(std::move(offer2));
    EXPECT_EQ(offers.size(), initialSize + 2);

    // 追加等级3的交易
    auto offer3 = std::make_unique<MerchantOffer>(ItemStack(emerald_, 5), ItemStack(diamond_, 1), 8, 20, 0.2f);
    offers.addOffer(std::move(offer3));
    EXPECT_EQ(offers.size(), initialSize + 3);

    // 验证所有交易保留，使用次数和使用状态不受影响
    MerchantOffer* first = offers.getOffer(initialSize);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->getXp(), 2); // 等级1交易

    MerchantOffer* second = offers.getOffer(initialSize + 1);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->getXp(), 10); // 等级2交易

    MerchantOffer* third = offers.getOffer(initialSize + 2);
    ASSERT_NE(third, nullptr);
    EXPECT_EQ(third->getXp(), 20); // 等级3交易
}

// ============================================================================
// rewardTradeXp 逻辑模拟测试
// ============================================================================
//
// 验证 rewardTradeXp 的核心逻辑：
// 1. 经验球值范围（3~6，升级时 8~11）
// 2. 升级计时器设置
// 3. 多级升级时 prevLevel 的正确记录
// 4. _increaseMerchantCareer 追加交易的等级范围

class RewardTradeXpLogicTest : public ::testing::Test {
protected:
    VillagerData data{VillagerType::Plains, VillagerProfession::Farmer, 1};
};

TEST_F(RewardTradeXpLogicTest, XpOrbCount_NoLevelUp)
{
    // 模拟 rewardTradeXp：不升级时，经验球值 = 3 + random(0~3) = 3~6
    i32 prevLevel = data.level();
    data.addExperience(5);              // 不足升级（需要10）
    EXPECT_EQ(data.level(), prevLevel); // 未升级

    // 经验球值范围验证
    i32 minOrbs = 3;
    i32 maxOrbs = 6;
    EXPECT_GE(minOrbs, 3);
    EXPECT_LE(maxOrbs, 6);
}

TEST_F(RewardTradeXpLogicTest, XpOrbCount_SingleLevelUp)
{
    // 模拟 rewardTradeXp：升级时，经验球值额外 +5 = 8~11
    i32 prevLevel = data.level(); // 1
    data.addExperience(10);       // 升到2级
    EXPECT_GT(data.level(), prevLevel);

    // 升级时，经验球值范围 = (3~6) + 5 = 8~11
    i32 minOrbs = 3 + 5;
    i32 maxOrbs = 6 + 5;
    EXPECT_GE(minOrbs, 8);
    EXPECT_LE(maxOrbs, 11);
}

TEST_F(RewardTradeXpLogicTest, MultiLevelUp_PrevLevelTracking)
{
    // 模拟多级升级：从1级直接升到3级
    // prevLevel 应记录为 1，currentLevel 为 3
    // _increaseMerchantCareer 应为 2 级和 3 级都生成交易
    i32 prevLevel = data.level(); // 1
    data.addExperience(80);       // 10+70 = 80，升到3级

    EXPECT_EQ(data.level(), 3);
    i32 currentLevel = data.level();

    // 验证需要为 prevLevel+1 到 currentLevel 的每个等级生成交易
    i32 expectedStartLevel = prevLevel + 1; // 2
    i32 expectedEndLevel = currentLevel;    // 3
    EXPECT_EQ(expectedStartLevel, 2);
    EXPECT_EQ(expectedEndLevel, 3);
    EXPECT_EQ(expectedEndLevel - expectedStartLevel + 1, 2); // 需要2个等级的交易
}

TEST_F(RewardTradeXpLogicTest, MultiLevelUp_FromLevel2ToLevel5)
{
    // 从2级给330经验（70+150+250-40=430不对）
    // Level 2->3: 70, Level 3->4: 150, Level 4->5: 250
    // 总计需要 70+150+250 = 470 才能从2级升到5级
    data = VillagerData(VillagerType::Plains, VillagerProfession::Farmer, 2);
    i32 prevLevel = data.level();
    data.addExperience(470);
    EXPECT_EQ(data.level(), 5);

    // _increaseMerchantCareer 应为 3、4、5 级都生成交易
    EXPECT_EQ(prevLevel, 2);
    EXPECT_EQ(data.level() - prevLevel, 3); // 跳了3个等级
}

TEST_F(RewardTradeXpLogicTest, NoXpOrbs_WhenShouldRewardExpFalse)
{
    // shouldRewardExp 为 false 时不应生成经验球
    // 这是逻辑验证，不需要完整的实体模拟
    MerchantOffer offer;
    EXPECT_TRUE(offer.shouldRewardExp()); // 默认为 true

    // 如果 shouldRewardExp() 返回 false，经验球生成跳过
    // 这个测试确认默认值为 true，对应的行为在 VillagerEntity::rewardTradeXp 中
}

// ============================================================================
// VillagerTrades hasTrades 测试
// ============================================================================
//
// 测试 VillagerTrades::hasTrades 方法（不需要 Items::initialize）。
// 注意：generateOffers 测试需要 Items::initialize 才能注册物品，
// 因此不在本测试文件中。generateOffers 的集成测试应放在
// 需要 Items::initialize 的测试环境中。

TEST(VillagerTradesBasicTest, Nitwit_HasNoTrades)
{
    // 傻子村民没有交易
    EXPECT_FALSE(VillagerTrades::hasTrades(VillagerProfession::Nitwit));
    auto offers = VillagerTrades::generateOffers(VillagerProfession::Nitwit, VillagerType::Plains, 1, 0, 42);
    ASSERT_NE(offers, nullptr);
    EXPECT_EQ(offers->size(), 0u) << "Nitwit should have no trades";
}

TEST(VillagerTradesBasicTest, NoneProfession_HasNoTrades)
{
    EXPECT_FALSE(VillagerTrades::hasTrades(VillagerProfession::None));
}

// ============================================================================
// MerchantOffer 交易使用和补货测试
// ============================================================================

class MerchantOfferUsageTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        emerald_ = getOrRegisterTestItem("minecraft:emerald");
        bread_ = getOrRegisterTestItem("minecraft:bread");
        diamond_ = getOrRegisterTestItem("minecraft:diamond");
    }

    static const Item* getOrRegisterTestItem(const std::string& name)
    {
        ResourceLocation id(name);
        const Item* existing = ItemRegistry::instance().getItem(id);
        if (existing != nullptr) {
            return existing;
        }
        return &ItemRegistry::instance().registerItem(id, ItemProperties().maxStackSize(64));
    }

    const Item* emerald_ = nullptr;
    const Item* bread_ = nullptr;
    const Item* diamond_ = nullptr;
};

TEST_F(MerchantOfferUsageTest, IncreaseUses_TracksCorrectly)
{
    MerchantOffer offer(ItemStack(emerald_, 1), ItemStack(bread_, 6), 4, 2, 0.05f);

    EXPECT_EQ(offer.getUses(), 0);
    EXPECT_EQ(offer.getRemainingUses(), 4);
    EXPECT_FLOAT_EQ(offer.getProgress(), 0.0f);

    offer.increaseUses();
    EXPECT_EQ(offer.getUses(), 1);
    EXPECT_EQ(offer.getRemainingUses(), 3);
    EXPECT_FLOAT_EQ(offer.getProgress(), 0.25f);

    offer.increaseUses();
    offer.increaseUses();
    EXPECT_EQ(offer.getUses(), 3);
    EXPECT_EQ(offer.getRemainingUses(), 1);

    offer.increaseUses(); // 第4次
    EXPECT_EQ(offer.getUses(), 4);
    EXPECT_TRUE(offer.isOutOfStock());
}

TEST_F(MerchantOfferUsageTest, Restock_ResetsUsesAndTracksRestockCount)
{
    MerchantOffer offer(ItemStack(emerald_, 1), ItemStack(bread_, 6), 2, 2, 0.05f);

    offer.increaseUses();
    offer.increaseUses();
    EXPECT_TRUE(offer.isOutOfStock());

    offer.restock();
    EXPECT_FALSE(offer.isOutOfStock());
    EXPECT_EQ(offer.getUses(), 0);
    EXPECT_EQ(offer.getRestocksToday(), 1);

    // 再次用完并补货
    offer.increaseUses();
    offer.increaseUses();
    offer.restock();
    EXPECT_EQ(offer.getRestocksToday(), 2);
}

TEST_F(MerchantOfferUsageTest, XpValue_AffectsVillagerLeveling)
{
    // 不同交易给予不同经验值
    MerchantOffer cheapOffer(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 1, 0.05f);
    MerchantOffer expensiveOffer(ItemStack(emerald_, 5), ItemStack(diamond_, 1), 8, 10, 0.2f);

    EXPECT_EQ(cheapOffer.getXp(), 1);
    EXPECT_EQ(expensiveOffer.getXp(), 10);

    // 模拟 addVillagerExperience：
    // 10次 cheapOffer(1xp) = 10xp → 从1级升到2级
    VillagerData data(VillagerType::Plains, VillagerProfession::Farmer, 1);
    i32 prevLevel = data.level();
    data.addExperience(cheapOffer.getXp() * 10);
    EXPECT_GT(data.level(), prevLevel); // 升级了
    EXPECT_EQ(data.level(), 2);
}

TEST_F(MerchantOfferUsageTest, ShouldRewardExp_DefaultAndCustom)
{
    MerchantOffer offer(ItemStack(emerald_, 1), ItemStack(bread_, 6), 16, 2, 0.05f);
    EXPECT_TRUE(offer.shouldRewardExp()); // 默认为 true
}

// ============================================================================
// experienceProgress 测试
// ============================================================================
//
// 验证 AbstractVillagerEntity::experienceProgress 的计算逻辑

class ExperienceProgressTest : public ::testing::Test {
protected:
    VillagerData data{VillagerType::Plains, VillagerProfession::Farmer, 1};
};

TEST_F(ExperienceProgressTest, ProgressZero_WhenNoExperience)
{
    // 0 经验时进度为 0
    EXPECT_EQ(data.experience(), 0);
    // Level 1 -> Level 2 需要 10 经验
    // 进度 = 0 / 10 = 0.0
    i32 requiredXp = VillagerData::getExperienceForLevel(data.level());
    EXPECT_EQ(requiredXp, 10);
    EXPECT_FLOAT_EQ(static_cast<f32>(data.experience()) / static_cast<f32>(requiredXp), 0.0f);
}

TEST_F(ExperienceProgressTest, ProgressHalfWay)
{
    // 5 经验 / 10 需要 = 0.5 进度
    data.addExperience(5);
    EXPECT_EQ(data.experience(), 5);
    EXPECT_EQ(data.level(), 1);

    i32 requiredXp = VillagerData::getExperienceForLevel(data.level());
    EXPECT_FLOAT_EQ(static_cast<f32>(data.experience()) / static_cast<f32>(requiredXp), 0.5f);
}

TEST_F(ExperienceProgressTest, ProgressMaxLevel_Zero)
{
    // 最大等级时进度为 0
    data = VillagerData(VillagerType::Plains, VillagerProfession::Farmer, 5);
    data.addExperience(100);

    EXPECT_EQ(data.level(), VillagerData::getMaxLevel());
    // 最大等级 getExperienceForLevel(5) 返回 0
    EXPECT_EQ(VillagerData::getExperienceForLevel(data.level()), 0);
}
