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

#include "common/util/nbt/Nbt.hpp"
#include "common/world/village/Village.hpp"
#include "common/world/village/VillageGossip.hpp"
#include "common/world/village/VillageGossipType.hpp"
#include <memory>
#include <algorithm>

using namespace mc;
using namespace mc::world::village;

/**
 * @brief VillageGossipManager 治愈僵尸村民声望测试
 *
 * 测试治愈僵尸村民时村庄声望的更新。
 *
 * 声誉计算公式：reputation += getReputationImpact(type) * value
 *
 * 其中声誉影响值（已包含权重）：
 * - MajorPositive: 100 声誉影响（每次治愈事件价值 20）
 * - MinorPositive: 20 声誉影响（每次治愈事件价值 25）
 *
 * 治愈僵尸村民后：
 * - MajorPositive value=20 → 声誉 = 100 * 20 = 2000 → clamp 到 1000
 * - MinorPositive value=25 → 声誉 = 20 * 25 = 500
 *
 * 注意：当前实现与 MC 1.16.5 的权重系统略有不同。
 */
class VillageGossipCureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建村庄
        m_village = std::make_unique<Village>(BlockPos(0, 64, 0));
        m_village->setId(1);
    }

    void TearDown() override
    {
        m_village.reset();
    }

    std::unique_ptr<Village> m_village;
};

// ========== 治愈僵尸村民声望测试 ==========

TEST_F(VillageGossipCureTest, CureZombieVillager_AddsMajorPositive)
{
    // 玩家标识符（使用 std::hash<std::string> 生成）
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // 模拟治愈僵尸村民后的声望更新
    // MC 1.16.5: this.gossip.add(target.getUniqueID(), GossipType.MAJOR_POSITIVE, 20);
    m_village->addGossip(playerId, VillageGossipType::MajorPositive, 20);

    // 验证流言值
    i32 gossipValue = m_village->getGossipManager().getGossipValue(playerId, VillageGossipType::MajorPositive);
    EXPECT_EQ(gossipValue, 20);

    // 验证声望值
    // MajorPositive impact=100, value=20 → 100 * 20 = 2000，但 clamp 到 1000
    i32 reputation = m_village->getPlayerReputation(playerId);
    EXPECT_EQ(reputation, 1000); // clamp 到最大声誉
}

TEST_F(VillageGossipCureTest, CureZombieVillager_AddsMinorPositive)
{
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // 模拟治愈僵尸村民后的声望更新
    // MC 1.16.5: this.gossip.add(target.getUniqueID(), GossipType.MINOR_POSITIVE, 25);
    m_village->addGossip(playerId, VillageGossipType::MinorPositive, 25);

    // 验证流言值
    i32 gossipValue = m_village->getGossipManager().getGossipValue(playerId, VillageGossipType::MinorPositive);
    EXPECT_EQ(gossipValue, 25);

    // 验证声望值
    // MinorPositive impact=20, value=25 → 20 * 25 = 500
    i32 reputation = m_village->getPlayerReputation(playerId);
    EXPECT_EQ(reputation, 500);
}

TEST_F(VillageGossipCureTest, CureZombieVillager_AddsBothGossipTypes)
{
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // 模拟治愈僵尸村民后的完整声望更新
    // MC 1.16.5: VillagerEntity.updateReputation(IReputationType.ZOMBIE_VILLAGER_CURED)
    m_village->addGossip(playerId, VillageGossipType::MajorPositive, 20);
    m_village->addGossip(playerId, VillageGossipType::MinorPositive, 25);

    // 验证流言值
    EXPECT_EQ(m_village->getGossipManager().getGossipValue(playerId, VillageGossipType::MajorPositive), 20);
    EXPECT_EQ(m_village->getGossipManager().getGossipValue(playerId, VillageGossipType::MinorPositive), 25);

    // 验证总声望值（被 clamp）
    // MajorPositive: 100 * 20 = 2000, MinorPositive: 20 * 25 = 500
    // 总计: 2500 → clamp 到 1000
    i32 reputation = m_village->getPlayerReputation(playerId);
    EXPECT_EQ(reputation, 1000);
}

TEST_F(VillageGossipCureTest, CureZombieVillager_SingleCureReputation)
{
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // 单次治愈：只添加 1 点 MajorPositive 来测试基础逻辑
    m_village->addGossip(playerId, VillageGossipType::MajorPositive, 1);
    // 声誉 = 100 * 1 = 100
    EXPECT_EQ(m_village->getPlayerReputation(playerId), 100);

    // 只添加 1 点 MinorPositive
    m_village->addGossip(playerId, VillageGossipType::MinorPositive, 1);
    // 声誉 = 100 + 20 * 1 = 120
    EXPECT_EQ(m_village->getPlayerReputation(playerId), 120);
}

TEST_F(VillageGossipCureTest, CureZombieVillager_MaxValueCap)
{
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // MajorPositive 最大累积值是 20
    // 添加超过最大值
    m_village->addGossip(playerId, VillageGossipType::MajorPositive, 50);

    // 验证被限制在最大值
    i32 gossipValue = m_village->getGossipManager().getGossipValue(playerId, VillageGossipType::MajorPositive);
    EXPECT_EQ(gossipValue, 20); // 最大值

    // MinorPositive 最大累积值是 200
    m_village->addGossip(playerId, VillageGossipType::MinorPositive, 300);
    gossipValue = m_village->getGossipManager().getGossipValue(playerId, VillageGossipType::MinorPositive);
    EXPECT_EQ(gossipValue, 200); // 最大值
}

TEST_F(VillageGossipCureTest, CureZombieVillager_DifferentPlayers)
{
    u64 player1 = std::hash<std::string>{}("player-1-uuid");
    u64 player2 = std::hash<std::string>{}("player-2-uuid");

    // 玩家1治愈僵尸村民
    m_village->addGossip(player1, VillageGossipType::MajorPositive, 1);
    m_village->addGossip(player1, VillageGossipType::MinorPositive, 1);

    // 玩家2治愈僵尸村民（两次）
    m_village->addGossip(player2, VillageGossipType::MajorPositive, 2);
    m_village->addGossip(player2, VillageGossipType::MinorPositive, 2);

    // 验证两个玩家的声望是独立的
    // player1: 100 + 20 = 120
    EXPECT_EQ(m_village->getPlayerReputation(player1), 120);
    // player2: 200 + 40 = 240
    EXPECT_EQ(m_village->getPlayerReputation(player2), 240);
}

TEST_F(VillageGossipCureTest, CureZombieVillager_PriceModifier)
{
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // 单次治愈：声誉 = 100 + 20 = 120
    m_village->addGossip(playerId, VillageGossipType::MajorPositive, 1);
    m_village->addGossip(playerId, VillageGossipType::MinorPositive, 1);

    // 验证价格修正因子
    // reputation = 120, modifier = 1.0 - 120/1000 = 0.88
    f32 priceModifier = m_village->getPriceModifier(playerId);
    EXPECT_NEAR(priceModifier, 0.88f, 0.01f);
}

TEST_F(VillageGossipCureTest, CureZombieVillager_PriceModifierMaxDiscount)
{
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // 达到最大声誉
    m_village->addGossip(playerId, VillageGossipType::MajorPositive, 20); // 2000 → clamp to 1000

    // 验证声誉达到最大值
    i32 reputation = m_village->getPlayerReputation(playerId);
    EXPECT_EQ(reputation, 1000);

    // 验证价格修正因子达到最低值
    f32 priceModifier = m_village->getPriceModifier(playerId);
    EXPECT_NEAR(priceModifier, 0.5f, 0.001f); // 最低价格修正因子
}

TEST_F(VillageGossipCureTest, CureZombieVillager_PriceModifierNegativeReputation)
{
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // 添加负面声望
    m_village->addGossip(playerId, VillageGossipType::MajorNegative, 5); // -100 * 5 = -500
    m_village->addGossip(playerId, VillageGossipType::MinorNegative, 10); // -20 * 10 = -200

    // 验证声誉
    EXPECT_EQ(m_village->getPlayerReputation(playerId), -700);

    // 验证价格修正因子提高
    // modifier = 1.0 - (-700)/1000 = 1.7 → clamp to 1.5
    f32 priceModifier = m_village->getPriceModifier(playerId);
    EXPECT_NEAR(priceModifier, 1.5f, 0.001f);
}

// ========== Village 位置检测测试 ==========

TEST_F(VillageGossipCureTest, Village_PositionCheck)
{
    // 村庄中心在 (0, 64, 0)
    EXPECT_TRUE(m_village->isWithinVillage(BlockPos(0, 64, 0)));
    EXPECT_TRUE(m_village->isWithinVillage(BlockPos(30, 64, 30)));
    EXPECT_FALSE(m_village->isWithinVillage(BlockPos(100, 64, 100)));
}

TEST_F(VillageGossipCureTest, Village_SerializeDeserializeWithGossip)
{
    u64 playerId = std::hash<std::string>{}("test-player-uuid");

    // 添加较小的声望值
    m_village->addGossip(playerId, VillageGossipType::MinorPositive, 10); // 200 声誉

    // 序列化
    nbt::tags::compound_tag tag;
    m_village->serialize(tag);

    // 反序列化
    Village deserialized = Village::deserialize(tag);

    // 验证流言数据保留
    i32 gossipValue = deserialized.getGossipManager().getGossipValue(playerId, VillageGossipType::MinorPositive);
    EXPECT_EQ(gossipValue, 10);

    // 验证声誉
    // MinorPositive impact=20, value=10 → 20 * 10 = 200
    EXPECT_EQ(deserialized.getPlayerReputation(playerId), 200);
}

// ========== VillageGossipManager 独立测试 ==========

class VillageGossipManagerTest : public ::testing::Test {
protected:
    VillageGossipManager m_manager;
};

TEST_F(VillageGossipManagerTest, AddGossip_NewPlayer)
{
    u64 playerId = 12345;

    m_manager.addGossip(playerId, VillageGossipType::MajorPositive, 10);

    EXPECT_TRUE(m_manager.hasGossip(playerId));
    EXPECT_EQ(m_manager.getGossipValue(playerId, VillageGossipType::MajorPositive), 10);
}

TEST_F(VillageGossipManagerTest, AddGossip_ExistingPlayer)
{
    u64 playerId = 12345;

    m_manager.addGossip(playerId, VillageGossipType::MajorPositive, 10);
    m_manager.addGossip(playerId, VillageGossipType::MajorPositive, 5);

    EXPECT_EQ(m_manager.getGossipValue(playerId, VillageGossipType::MajorPositive), 15);
}

TEST_F(VillageGossipManagerTest, AddGossip_MaxValueCap)
{
    u64 playerId = 12345;

    // MajorPositive 最大值是 20
    m_manager.addGossip(playerId, VillageGossipType::MajorPositive, 30);

    EXPECT_EQ(m_manager.getGossipValue(playerId, VillageGossipType::MajorPositive), 20);
}

TEST_F(VillageGossipManagerTest, RemoveGossip)
{
    u64 playerId = 12345;

    m_manager.addGossip(playerId, VillageGossipType::MajorPositive, 10);
    m_manager.addGossip(playerId, VillageGossipType::MinorPositive, 25);

    m_manager.removeGossip(playerId, VillageGossipType::MajorPositive);

    EXPECT_EQ(m_manager.getGossipValue(playerId, VillageGossipType::MajorPositive), 0);
    EXPECT_EQ(m_manager.getGossipValue(playerId, VillageGossipType::MinorPositive), 25);
}

TEST_F(VillageGossipManagerTest, ClearGossip)
{
    u64 playerId = 12345;

    m_manager.addGossip(playerId, VillageGossipType::MajorPositive, 10);
    m_manager.addGossip(playerId, VillageGossipType::MinorPositive, 25);

    m_manager.clearGossip(playerId);

    EXPECT_FALSE(m_manager.hasGossip(playerId));
    EXPECT_EQ(m_manager.getReputation(playerId), 0);
}

TEST_F(VillageGossipManagerTest, GetReputation_MultipleTypes)
{
    u64 playerId = 12345;

    // 添加多种类型的声望（使用较小的值）
    m_manager.addGossip(playerId, VillageGossipType::MajorPositive, 1);   // +100 * 1 = +100
    m_manager.addGossip(playerId, VillageGossipType::MinorPositive, 1);   // +20 * 1 = +20
    m_manager.addGossip(playerId, VillageGossipType::Trading, 5);          // +2 * 5 = +10
    m_manager.addGossip(playerId, VillageGossipType::MinorNegative, 1);    // -20 * 1 = -20
    m_manager.addGossip(playerId, VillageGossipType::MajorNegative, 1);    // -100 * 1 = -100

    // 总声誉: 100 + 20 + 10 - 20 - 100 = 10
    EXPECT_EQ(m_manager.getReputation(playerId), 10);
}

TEST_F(VillageGossipManagerTest, GetReputation_Clamped)
{
    u64 player1 = 12345;

    // 添加大量正面声望
    m_manager.addGossip(player1, VillageGossipType::MajorPositive, 20);  // 2000 → clamp
    m_manager.addGossip(player1, VillageGossipType::MinorPositive, 200); // 4000

    // 声誉应该被限制在 1000
    EXPECT_EQ(m_manager.getReputation(player1), 1000);

    // 添加大量负面声望
    u64 player2 = 67890;
    m_manager.addGossip(player2, VillageGossipType::MajorNegative, 100);  // -10000
    m_manager.addGossip(player2, VillageGossipType::MinorNegative, 200);  // -4000

    // 声誉应该被限制在 -1000
    EXPECT_EQ(m_manager.getReputation(player2), -1000);
}

TEST_F(VillageGossipManagerTest, GetAllPlayers)
{
    u64 player1 = 12345;
    u64 player2 = 67890;
    u64 player3 = 11111;

    m_manager.addGossip(player1, VillageGossipType::MajorPositive, 10);
    m_manager.addGossip(player2, VillageGossipType::Trading, 5);
    m_manager.addGossip(player3, VillageGossipType::MinorNegative, 10);

    std::vector<u64> players = m_manager.getAllPlayers();
    EXPECT_EQ(players.size(), 3);

    // 验证所有玩家都在列表中
    EXPECT_NE(std::find(players.begin(), players.end(), player1), players.end());
    EXPECT_NE(std::find(players.begin(), players.end(), player2), players.end());
    EXPECT_NE(std::find(players.begin(), players.end(), player3), players.end());
}

TEST_F(VillageGossipManagerTest, ClearAll)
{
    m_manager.addGossip(12345, VillageGossipType::MajorPositive, 10);
    m_manager.addGossip(67890, VillageGossipType::Trading, 5);

    m_manager.clearAll();

    EXPECT_EQ(m_manager.getAllPlayers().size(), 0);
}

// ========== GossipTypeHelper 测试 ==========

TEST(VillageGossipTypeHelperTest, GetReputationImpact)
{
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::MajorNegative), -100);
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::MinorNegative), -20);
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::Trading), 2);
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::MinorPositive), 20);
    EXPECT_EQ(GossipTypeHelper::getReputationImpact(VillageGossipType::MajorPositive), 100);
}

TEST(VillageGossipTypeHelperTest, GetMaxValue)
{
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::MajorNegative), 100);
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::MinorNegative), 200);
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::Trading), 100);
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::MinorPositive), 200);
    EXPECT_EQ(GossipTypeHelper::getMaxValue(VillageGossipType::MajorPositive), 20);
}

TEST(VillageGossipTypeHelperTest, GetDecayInterval)
{
    // 负面声望衰减更快
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::MajorNegative), 12000);
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::MinorNegative), 24000);

    // 正面声望衰减较慢
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::Trading), 24000);
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::MinorPositive), 24000);
    EXPECT_EQ(GossipTypeHelper::getDecayInterval(VillageGossipType::MajorPositive), 48000);
}

TEST(VillageGossipTypeHelperTest, IsNegativeIsPositive)
{
    EXPECT_TRUE(GossipTypeHelper::isNegative(VillageGossipType::MajorNegative));
    EXPECT_TRUE(GossipTypeHelper::isNegative(VillageGossipType::MinorNegative));
    EXPECT_FALSE(GossipTypeHelper::isNegative(VillageGossipType::Trading));
    EXPECT_FALSE(GossipTypeHelper::isNegative(VillageGossipType::MinorPositive));
    EXPECT_FALSE(GossipTypeHelper::isNegative(VillageGossipType::MajorPositive));

    EXPECT_FALSE(GossipTypeHelper::isPositive(VillageGossipType::MajorNegative));
    EXPECT_FALSE(GossipTypeHelper::isPositive(VillageGossipType::MinorNegative));
    EXPECT_TRUE(GossipTypeHelper::isPositive(VillageGossipType::Trading));
    EXPECT_TRUE(GossipTypeHelper::isPositive(VillageGossipType::MinorPositive));
    EXPECT_TRUE(GossipTypeHelper::isPositive(VillageGossipType::MajorPositive));
}

// main 函数由 gtest_main 库提供
