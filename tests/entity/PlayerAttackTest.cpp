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

#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/protection/ThornsEnchantment.hpp"
#include "common/item/enchantment/enchantments/weapon/SweepingEnchantment.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/effect/EffectEntities.hpp"
#include "world/fluid/FluidRegistry.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试用 Mock Team 类
// ============================================================================

class MockTeamForSweep : public scoreboard::Team {
public:
    explicit MockTeamForSweep(const std::string& name)
        : m_name(name)
    {}

    [[nodiscard]] const std::string& getName() const noexcept override { return m_name; }
    [[nodiscard]] const text::ITextComponent* getDisplayName() const noexcept override { return nullptr; }
    void setDisplayName(std::unique_ptr<text::ITextComponent>) override {}
    [[nodiscard]] const std::set<std::string>& getMembers() const noexcept override
    {
        static std::set<std::string> empty;
        return empty;
    }
    bool addMember(const std::string&) override { return false; }
    bool removeMember(const std::string&) override { return false; }
    [[nodiscard]] bool hasMember(const std::string&) const override { return false; }
    void clearMembers() override {}
    [[nodiscard]] text::TextFormatting getColor() const noexcept override { return text::TextFormatting::White; }
    void setColor(text::TextFormatting) override {}
    [[nodiscard]] const text::ITextComponent* getPrefix() const noexcept override { return nullptr; }
    void setPrefix(std::unique_ptr<text::ITextComponent>) override {}
    [[nodiscard]] const text::ITextComponent* getSuffix() const noexcept override { return nullptr; }
    void setSuffix(std::unique_ptr<text::ITextComponent>) override {}
    [[nodiscard]] bool getAllowFriendlyFire() const noexcept override { return true; }
    void setAllowFriendlyFire(bool) override {}
    [[nodiscard]] bool canSeeFriendlyInvisibles() const noexcept override { return false; }
    void setSeeFriendlyInvisibles(bool) override {}
    [[nodiscard]] scoreboard::TeamVisibility getNameTagVisibility() const noexcept override
    {
        return scoreboard::TeamVisibility::Always;
    }
    void setNameTagVisibility(scoreboard::TeamVisibility) override {}
    [[nodiscard]] scoreboard::TeamVisibility getDeathMessageVisibility() const noexcept override
    {
        return scoreboard::TeamVisibility::Always;
    }
    void setDeathMessageVisibility(scoreboard::TeamVisibility) override {}
    [[nodiscard]] scoreboard::TeamCollisionRule getCollisionRule() const noexcept override
    {
        return scoreboard::TeamCollisionRule::Always;
    }
    void setCollisionRule(scoreboard::TeamCollisionRule) override {}
    [[nodiscard]] std::unique_ptr<text::ITextComponent> formatName(const text::ITextComponent&) const override
    {
        return nullptr;
    }

    [[nodiscard]] std::unique_ptr<text::ITextComponent> getFormattedDisplayName() const override { return nullptr; }

private:
    std::string m_name;
};

// ============================================================================
// 测试用 Mock Entity 类（支持队伍）
// ============================================================================

class MockEntityWithTeamForSweep : public Entity {
public:
    MockEntityWithTeamForSweep()
        : Entity(EntityInstanceId(1))
    {}

    void setTeam(scoreboard::Team* team) { m_team = team; }

    [[nodiscard]] scoreboard::Team* getTeam() override { return m_team; }
    [[nodiscard]] const scoreboard::Team* getTeam() const override { return m_team; }

private:
    scoreboard::Team* m_team = nullptr;
};

// ============================================================================
// 横扫攻击过滤测试
// ============================================================================

class SweepAttackFilterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
    }

    void TearDown() override {}
};

// ============================================================================
// 盔甲架标记模式测试
// ============================================================================

TEST_F(SweepAttackFilterTest, ArmorStand_MarkerMode_ShouldBeExcluded)
{
    // 创建标记模式的盔甲架
    ArmorStandEntity armorStand;
    armorStand.setMarker(true);

    // 标记模式盔甲架应该被排除
    EXPECT_TRUE(armorStand.isMarker());
    EXPECT_FALSE(armorStand.canBeCollidedWith()); // 标记模式不可碰撞
}

TEST_F(SweepAttackFilterTest, ArmorStand_NonMarkerMode_ShouldBeIncluded)
{
    // 创建非标记模式的盔甲架
    ArmorStandEntity armorStand;
    armorStand.setMarker(false);

    // 非标记模式盔甲架不应该被排除
    EXPECT_FALSE(armorStand.isMarker());
    EXPECT_TRUE(armorStand.canBeCollidedWith()); // 可以碰撞
}

TEST_F(SweepAttackFilterTest, ArmorStand_DefaultNotMarker)
{
    // 默认创建的盔甲架不是标记模式
    ArmorStandEntity armorStand;
    EXPECT_FALSE(armorStand.isMarker());
}

TEST_F(SweepAttackFilterTest, ArmorStand_MarkerHasZeroBoundingBox)
{
    // 标记模式的盔甲架碰撞箱大小为 0
    ArmorStandEntity markerStand;
    markerStand.setMarker(true);

    EXPECT_FLOAT_EQ(markerStand.width(), 0.0f);
    EXPECT_FLOAT_EQ(markerStand.height(), 0.0f);
}

TEST_F(SweepAttackFilterTest, ArmorStand_NonMarkerHasNormalBoundingBox)
{
    // 非标记模式的盔甲架有正常碰撞箱
    ArmorStandEntity normalStand;
    normalStand.setMarker(false);

    // MC 1.16.5: 非标记模式盔甲架宽度 0.5，高度约 1.975
    EXPECT_FLOAT_EQ(normalStand.width(), 0.5f);
    EXPECT_FLOAT_EQ(normalStand.height(), 1.975f);
}

// ============================================================================
// 队友排除测试
// ============================================================================

TEST_F(SweepAttackFilterTest, Team_SameTeamEntities_ShouldBeExcluded)
{
    // 创建两个实体和同一队伍
    MockEntityWithTeamForSweep entity1;
    MockEntityWithTeamForSweep entity2;
    MockTeamForSweep team("red");

    entity1.setTeam(&team);
    entity2.setTeam(&team);

    // 同一队伍应该被排除
    EXPECT_TRUE(entity1.isOnSameTeam(entity2));
    EXPECT_TRUE(entity2.isOnSameTeam(entity1));
}

TEST_F(SweepAttackFilterTest, Team_DifferentTeams_ShouldNotBeExcluded)
{
    // 创建两个实体和不同队伍
    MockEntityWithTeamForSweep entity1;
    MockEntityWithTeamForSweep entity2;
    MockTeamForSweep team1("red");
    MockTeamForSweep team2("blue");

    entity1.setTeam(&team1);
    entity2.setTeam(&team2);

    // 不同队伍不应该被排除
    EXPECT_FALSE(entity1.isOnSameTeam(entity2));
    EXPECT_FALSE(entity2.isOnSameTeam(entity1));
}

TEST_F(SweepAttackFilterTest, Team_OneWithTeamOneWithout_ShouldNotBeExcluded)
{
    // 一个实体有队伍，另一个没有
    MockEntityWithTeamForSweep entityWithTeam;
    MockEntityWithTeamForSweep entityNoTeam;
    MockTeamForSweep team("red");

    entityWithTeam.setTeam(&team);
    // entityNoTeam 没有队伍

    // 无队伍实体不应该被视为队友
    EXPECT_FALSE(entityWithTeam.isOnSameTeam(entityNoTeam));
    EXPECT_FALSE(entityNoTeam.isOnSameTeam(entityWithTeam));
}

TEST_F(SweepAttackFilterTest, Team_BothNoTeam_ShouldNotBeTeammates)
{
    // 两个都没有队伍的实体
    MockEntityWithTeamForSweep entity1;
    MockEntityWithTeamForSweep entity2;

    // 都没有队伍，不算队友
    EXPECT_FALSE(entity1.isOnSameTeam(entity2));
    EXPECT_FALSE(entity2.isOnSameTeam(entity1));
}

TEST_F(SweepAttackFilterTest, Team_SelfCheck_ShouldBeTrue)
{
    // 检查自己
    MockEntityWithTeamForSweep entity;
    MockTeamForSweep team("red");
    entity.setTeam(&team);

    // 自己和自己应该是同一队伍
    EXPECT_TRUE(entity.isOnSameTeam(entity));
}

TEST_F(SweepAttackFilterTest, Team_SelfCheckNoTeam_ShouldBeFalse)
{
    // 检查自己（没有队伍）
    MockEntityWithTeamForSweep entityNoTeam;

    // 没有队伍时，自己和自己不算队友
    EXPECT_FALSE(entityNoTeam.isOnSameTeam(entityNoTeam));
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(SweepAttackFilterTest, Team_ComparisonByPointer)
{
    // MC 1.16.5: Team.isSameTeam() 使用对象指针相等性判断
    // 即使两个 Team 有相同的名称，它们也不是同一队伍

    // 创建两个名称相同的不同 Team 对象
    auto teamA = std::make_unique<MockTeamForSweep>("same_name");
    auto teamB = std::make_unique<MockTeamForSweep>("same_name");

    MockEntityWithTeamForSweep entity1;
    MockEntityWithTeamForSweep entity2;

    entity1.setTeam(teamA.get());
    entity2.setTeam(teamB.get());

    // 不同指针，即使名称相同，也不算同一队伍
    EXPECT_FALSE(entity1.isOnSameTeam(entity2));
}

TEST_F(SweepAttackFilterTest, IsOnScoreboardTeam_SameTeamObject_ReturnsTrue)
{
    // 使用相同的 Team 指针
    MockEntityWithTeamForSweep entity1;
    MockEntityWithTeamForSweep entity2;
    MockTeamForSweep team("red");

    entity1.setTeam(&team);
    entity2.setTeam(&team);

    // 使用 entity2 的队伍检查 entity1
    EXPECT_TRUE(entity1.isOnScoreboardTeam(entity2.getTeam()));
    EXPECT_TRUE(entity2.isOnScoreboardTeam(entity1.getTeam()));
}

TEST_F(SweepAttackFilterTest, IsOnScoreboardTeam_NullTeam_ReturnsFalse)
{
    // 检查 null 队伍
    MockEntityWithTeamForSweep entityWithTeam;
    MockTeamForSweep team("red");
    entityWithTeam.setTeam(&team);

    MockEntityWithTeamForSweep entityNoTeam;

    // 有队伍的实体检查 null 队伍
    EXPECT_FALSE(entityWithTeam.isOnScoreboardTeam(nullptr));

    // 无队伍的实体检查 null 队伍
    EXPECT_FALSE(entityNoTeam.isOnScoreboardTeam(nullptr));
}

// ============================================================================
// 横扫攻击场景模拟测试
// ============================================================================

TEST_F(SweepAttackFilterTest, SweepAttack_TeammatesNotHit)
{
    // 模拟横扫攻击场景：队友不应被攻击
    MockEntityWithTeamForSweep attacker;
    MockEntityWithTeamForSweep teammate;
    MockTeamForSweep team("red");

    attacker.setTeam(&team);
    teammate.setTeam(&team);

    // isOnSameTeam 返回 true，表示是队友，不应被攻击
    EXPECT_TRUE(attacker.isOnSameTeam(teammate));
}

TEST_F(SweepAttackFilterTest, SweepAttack_EnemiesCanBeHit)
{
    // 模拟横扫攻击场景：敌人应被攻击
    MockEntityWithTeamForSweep attacker;
    MockEntityWithTeamForSweep enemy;
    MockTeamForSweep redTeam("red");
    MockTeamForSweep blueTeam("blue");

    attacker.setTeam(&redTeam);
    enemy.setTeam(&blueTeam);

    // isOnSameTeam 返回 false，表示不是队友，可以被攻击
    EXPECT_FALSE(attacker.isOnSameTeam(enemy));
}

TEST_F(SweepAttackFilterTest, SweepAttack_NoTeamEntityCanBeHit)
{
    // 模拟横扫攻击场景：没有队伍的实体可以被攻击
    MockEntityWithTeamForSweep attacker;
    MockEntityWithTeamForSweep entityNoTeam;
    MockTeamForSweep team("red");

    attacker.setTeam(&team);
    // entityNoTeam 没有队伍

    // 没有队伍的实体不算队友，可以被攻击
    EXPECT_FALSE(attacker.isOnSameTeam(entityNoTeam));
    EXPECT_FALSE(entityNoTeam.isOnSameTeam(attacker));
}

// ============================================================================
// 荆棘附魔触发条件测试
// ============================================================================

class ThornsEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
    }

    void TearDown() override {}
};

TEST_F(ThornsEnchantmentTest, ShouldTrigger_LevelZero_NeverTriggers)
{
    math::Random rng(12345);

    // 等级 0 不应该触发
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(item::enchant::ThornsEnchantment::shouldTrigger(0, rng));
    }
}

TEST_F(ThornsEnchantmentTest, ShouldTrigger_LevelOne_About15Percent)
{
    math::Random rng(12345);
    int triggers = 0;
    const int iterations = 10000;

    // 等级 1 应该有约 15% 的触发率
    for (int i = 0; i < iterations; ++i) {
        if (item::enchant::ThornsEnchantment::shouldTrigger(1, rng)) {
            ++triggers;
        }
    }

    f32 rate = static_cast<f32>(triggers) / iterations;
    // 允许 12% - 18% 的误差范围
    EXPECT_GT(rate, 0.12f);
    EXPECT_LT(rate, 0.18f);
}

TEST_F(ThornsEnchantmentTest, ShouldTrigger_LevelTwo_About30Percent)
{
    math::Random rng(12345);
    int triggers = 0;
    const int iterations = 10000;

    // 等级 2 应该有约 30% 的触发率
    for (int i = 0; i < iterations; ++i) {
        if (item::enchant::ThornsEnchantment::shouldTrigger(2, rng)) {
            ++triggers;
        }
    }

    f32 rate = static_cast<f32>(triggers) / iterations;
    // 允许 27% - 33% 的误差范围
    EXPECT_GT(rate, 0.27f);
    EXPECT_LT(rate, 0.33f);
}

TEST_F(ThornsEnchantmentTest, ShouldTrigger_LevelThree_About45Percent)
{
    math::Random rng(12345);
    int triggers = 0;
    const int iterations = 10000;

    // 等级 3 应该有约 45% 的触发率
    for (int i = 0; i < iterations; ++i) {
        if (item::enchant::ThornsEnchantment::shouldTrigger(3, rng)) {
            ++triggers;
        }
    }

    f32 rate = static_cast<f32>(triggers) / iterations;
    // 允许 42% - 48% 的误差范围
    EXPECT_GT(rate, 0.42f);
    EXPECT_LT(rate, 0.48f);
}

TEST_F(ThornsEnchantmentTest, GetThornsDamage_LevelOne_ReturnsOneToFour)
{
    math::Random rng(12345);

    // 等级 1-3 应该返回 1-4 伤害
    for (int level = 1; level <= 3; ++level) {
        for (int i = 0; i < 100; ++i) {
            i32 damage = item::enchant::ThornsEnchantment::getThornsDamage(level, rng);
            EXPECT_GE(damage, 1);
            EXPECT_LE(damage, 4);
        }
    }
}

TEST_F(ThornsEnchantmentTest, GetThornsDamage_HighLevel_ReturnsLevelMinusTen)
{
    math::Random rng(12345);

    // 等级 > 10 应该返回 level - 10
    for (int level = 11; level <= 20; ++level) {
        i32 damage = item::enchant::ThornsEnchantment::getThornsDamage(level, rng);
        EXPECT_EQ(damage, level - 10);
    }
}

TEST_F(ThornsEnchantmentTest, GetTriggerChance_CorrectFormula)
{
    // 验证触发概率公式：level * 0.15
    EXPECT_FLOAT_EQ(item::enchant::ThornsEnchantment::getTriggerChance(1), 0.15f);
    EXPECT_FLOAT_EQ(item::enchant::ThornsEnchantment::getTriggerChance(2), 0.30f);
    EXPECT_FLOAT_EQ(item::enchant::ThornsEnchantment::getTriggerChance(3), 0.45f);
    EXPECT_FLOAT_EQ(item::enchant::ThornsEnchantment::getTriggerChance(4), 0.60f);
    EXPECT_FLOAT_EQ(item::enchant::ThornsEnchantment::getTriggerChance(5), 0.75f);
}

// ============================================================================
// 横扫伤害比例测试
// ============================================================================

class SweepDamageTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
    }

    void TearDown() override {}
};

TEST_F(SweepDamageTest, GetSweepingDamageRatio_Formula)
{
    // 验证横扫之刃伤害比例公式：1.0 - 1.0 / (level + 1)
    // 等级 I: 1.0 - 1.0/2 = 0.5
    // 等级 II: 1.0 - 1.0/3 = 0.667
    // 等级 III: 1.0 - 1.0/4 = 0.75

    EXPECT_FLOAT_EQ(item::enchant::SweepingEnchantment::getSweepingDamageRatio(1), 0.5f);
    EXPECT_NEAR(item::enchant::SweepingEnchantment::getSweepingDamageRatio(2), 0.667f, 0.01f);
    EXPECT_FLOAT_EQ(item::enchant::SweepingEnchantment::getSweepingDamageRatio(3), 0.75f);
}

TEST_F(SweepDamageTest, GetSweepingDamageRatio_LevelZero)
{
    // 等级 0 时应该返回 0（无横扫之刃附魔）
    EXPECT_FLOAT_EQ(item::enchant::SweepingEnchantment::getSweepingDamageRatio(0), 0.0f);
}

TEST_F(SweepDamageTest, GetSweepingDamageRatio_HighLevel)
{
    // 高等级横扫之刃接近 100% 伤害
    EXPECT_FLOAT_EQ(item::enchant::SweepingEnchantment::getSweepingDamageRatio(9), 0.9f);
    EXPECT_FLOAT_EQ(item::enchant::SweepingEnchantment::getSweepingDamageRatio(19), 0.95f);
}

// ============================================================================
// 武器耐久测试
// ============================================================================

class WeaponDurabilityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
    }

    void TearDown() override {}
};

TEST_F(WeaponDurabilityTest, ItemStack_IsDamageable_ReturnsTrueForTools)
{
    // 工具和武器应该可损坏
    const Item* sword = Items::DIAMOND_SWORD;
    const Item* axe = Items::DIAMOND_AXE;
    const Item* pickaxe = Items::DIAMOND_PICKAXE;

    ASSERT_NE(sword, nullptr);
    ASSERT_NE(axe, nullptr);
    ASSERT_NE(pickaxe, nullptr);

    ItemStack swordStack(sword, 1);
    ItemStack axeStack(axe, 1);
    ItemStack pickaxeStack(pickaxe, 1);

    EXPECT_TRUE(swordStack.isDamageable());
    EXPECT_TRUE(axeStack.isDamageable());
    EXPECT_TRUE(pickaxeStack.isDamageable());
}

TEST_F(WeaponDurabilityTest, ItemStack_DamageItem_ReducesDurability)
{
    // 创建一个物品堆
    const Item* swordItem = Items::DIAMOND_SWORD;
    ItemStack sword(swordItem, 1);

    i32 initialDamage = sword.getDamage();
    EXPECT_EQ(initialDamage, 0);

    // 造成伤害
    bool broken = sword.attemptDamageItem(1);

    // 钻石剑最大耐久度 1561，消耗 1 点不应该损坏
    EXPECT_FALSE(broken);
    EXPECT_EQ(sword.getDamage(), 1);
}

TEST_F(WeaponDurabilityTest, ItemStack_ExcessiveDamage_BreaksItem)
{
    // 创建一个物品堆
    const Item* swordItem = Items::DIAMOND_SWORD;
    ItemStack sword(swordItem, 1);

    // 造成大量伤害（超过最大耐久度 1561）
    bool broken = sword.attemptDamageItem(2000);

    // 物品应该损坏
    EXPECT_TRUE(broken);
    EXPECT_TRUE(sword.isEmpty());
}

TEST_F(WeaponDurabilityTest, ItemStack_IsDamaged_ReturnsCorrectState)
{
    const Item* swordItem = Items::DIAMOND_SWORD;
    ItemStack sword(swordItem, 1);

    // 新物品不应该损坏
    EXPECT_FALSE(sword.isDamaged());

    // 造成伤害后
    sword.attemptDamageItem(1);
    EXPECT_TRUE(sword.isDamaged());
}

TEST_F(WeaponDurabilityTest, ItemStack_GetMaxDamage_ReturnsCorrectValue)
{
    // 钻石剑最大耐久度 1561 (MC 1.16.5)
    const Item* swordItem = Items::DIAMOND_SWORD;
    ItemStack sword(swordItem, 1);
    EXPECT_EQ(sword.getMaxDamage(), 1561);

    // 钻石镐最大耐久度 1561
    const Item* pickaxeItem = Items::DIAMOND_PICKAXE;
    ItemStack pickaxe(pickaxeItem, 1);
    EXPECT_EQ(pickaxe.getMaxDamage(), 1561);

    // 钻石斧最大耐久度 1561
    const Item* axeItem = Items::DIAMOND_AXE;
    ItemStack axe(axeItem, 1);
    EXPECT_EQ(axe.getMaxDamage(), 1561);
}
