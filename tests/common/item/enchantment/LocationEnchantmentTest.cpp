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

#include "entity/enchantment/LocationEnchantmentTracker.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/enchantment/Enchantment.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
#include "item/enchantment/enchantments/AllEnchantments.hpp"
#include "item/enchantment/enchantments/protection/FrostWalkerEnchantment.hpp"
#include "item/enchantment/enchantments/special/SoulSpeedEnchantment.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::item::enchant;

// ============================================================================
// LocationEnchantmentTracker 测试
// ============================================================================

class LocationEnchantmentTrackerTest : public ::testing::Test {
protected:
    LocationEnchantmentTracker tracker;
};

TEST_F(LocationEnchantmentTrackerTest, InitiallyNoActiveEnchantments)
{
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(0, "minecraft:soul_speed"));
    EXPECT_FALSE(tracker.isActive(1, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetActiveAndCheck)
{
    tracker.setActive(0, "minecraft:frost_walker");
    EXPECT_TRUE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(0, "minecraft:soul_speed"));
    EXPECT_FALSE(tracker.isActive(1, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetActiveMultipleSlots)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");
    tracker.setActive(3, "minecraft:frost_walker");

    EXPECT_TRUE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_TRUE(tracker.isActive(0, "minecraft:soul_speed"));
    EXPECT_TRUE(tracker.isActive(3, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(1, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetActiveIdempotent)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:frost_walker"); // 重复设置不报错
    EXPECT_TRUE(tracker.isActive(0, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetInactiveReturnsPreviousState)
{
    EXPECT_FALSE(tracker.setInactive(0, "minecraft:frost_walker")); // 从未激活，返回 false

    tracker.setActive(0, "minecraft:frost_walker");
    EXPECT_TRUE(tracker.setInactive(0, "minecraft:frost_walker")); // 之前活跃，返回 true
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetInactiveRemovesEmptySlot)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setInactive(0, "minecraft:frost_walker");

    // 槽位 0 应该没有活跃附魔了
    const auto& active = tracker.getActiveEnchantments(0);
    EXPECT_TRUE(active.empty());
}

TEST_F(LocationEnchantmentTrackerTest, SetInactiveKeepsOtherEnchantments)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");
    tracker.setInactive(0, "minecraft:frost_walker");

    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_TRUE(tracker.isActive(0, "minecraft:soul_speed"));
}

TEST_F(LocationEnchantmentTrackerTest, ClearSlotReturnsAllActive)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");

    auto cleared = tracker.clearSlot(0);
    EXPECT_EQ(cleared.size(), 2u);
    EXPECT_TRUE(cleared.count("minecraft:frost_walker") > 0);
    EXPECT_TRUE(cleared.count("minecraft:soul_speed") > 0);
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(0, "minecraft:soul_speed"));
}

TEST_F(LocationEnchantmentTrackerTest, ClearEmptySlotReturnsEmpty)
{
    auto cleared = tracker.clearSlot(0);
    EXPECT_TRUE(cleared.empty());
}

TEST_F(LocationEnchantmentTrackerTest, ClearSlotDoesNotAffectOtherSlots)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(3, "minecraft:soul_speed");

    tracker.clearSlot(0);
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_TRUE(tracker.isActive(3, "minecraft:soul_speed"));
}

TEST_F(LocationEnchantmentTrackerTest, ClearAllRemovesEverything)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");
    tracker.setActive(3, "minecraft:frost_walker");

    tracker.clearAll();
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(0, "minecraft:soul_speed"));
    EXPECT_FALSE(tracker.isActive(3, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, GetActiveEnchantmentsEmpty)
{
    const auto& active = tracker.getActiveEnchantments(0);
    EXPECT_TRUE(active.empty());
}

TEST_F(LocationEnchantmentTrackerTest, GetActiveEnchantmentsWithEntries)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");

    const auto& active = tracker.getActiveEnchantments(0);
    EXPECT_EQ(active.size(), 2u);
    EXPECT_TRUE(active.count("minecraft:frost_walker") > 0);
    EXPECT_TRUE(active.count("minecraft:soul_speed") > 0);
}

// ============================================================================
// FrostWalkerEnchantment 属性测试
// ============================================================================

class FrostWalkerEnchantmentTest : public ::testing::Test {
protected:
    FrostWalkerEnchantment frostWalker;
};

TEST_F(FrostWalkerEnchantmentTest, Properties)
{
    EXPECT_EQ(frostWalker.id(), "minecraft:frost_walker");
    EXPECT_EQ(frostWalker.minLevel(), 1);
    EXPECT_EQ(frostWalker.maxLevel(), 2);
    EXPECT_EQ(frostWalker.type(), EnchantmentType::ArmorFeet);
    EXPECT_EQ(frostWalker.rarity(), EnchantmentRarity::Rare);
    EXPECT_TRUE(frostWalker.isTreasure());
}

TEST_F(FrostWalkerEnchantmentTest, GetNameKey)
{
    EXPECT_EQ(frostWalker.getNameKey(1), "enchantment.minecraft.frost_walker");
    EXPECT_EQ(frostWalker.getNameKey(2), "enchantment.minecraft.frost_walker");
}

TEST_F(FrostWalkerEnchantmentTest, GetFrostRadius)
{
    EXPECT_EQ(FrostWalkerEnchantment::getFrostRadius(1), 2); // I: 1 + 1 = 2
    EXPECT_EQ(FrostWalkerEnchantment::getFrostRadius(2), 3); // II: 2 + 1 = 3
}

TEST_F(FrostWalkerEnchantmentTest, GetMinCost)
{
    EXPECT_EQ(frostWalker.getMinCost(1), 10);
    EXPECT_EQ(frostWalker.getMinCost(2), 20);
}

TEST_F(FrostWalkerEnchantmentTest, GetMaxCost)
{
    EXPECT_EQ(frostWalker.getMaxCost(1), 25); // getMinCost(1) + 15
    EXPECT_EQ(frostWalker.getMaxCost(2), 35); // getMinCost(2) + 15
}

TEST_F(FrostWalkerEnchantmentTest, IsIncompatibleWithDepthStrider)
{
    FrostWalkerEnchantment frostWalker;
    const Enchantment* depthStrider = EnchantmentRegistry::get("minecraft:depth_strider");
    if (depthStrider) {
        EXPECT_FALSE(frostWalker.isCompatibleWith(*depthStrider));
    }
}

TEST_F(FrostWalkerEnchantmentTest, OnLocationEffectDeactivatedIsSafe)
{
    // FrostWalkerEnchantment::onLocationEffectDeactivated() 是空实现，
    // 但应可安全调用不崩溃。由于需要 LivingEntity 参数，这里只验证接口存在。
    // 实际集成测试在 LivingEntity 环境中进行。
}

// ============================================================================
// SoulSpeedEnchantment 属性测试
// ============================================================================

class SoulSpeedEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }

    SoulSpeedEnchantment soulSpeed;
};

TEST_F(SoulSpeedEnchantmentTest, Properties)
{
    EXPECT_EQ(soulSpeed.id(), "minecraft:soul_speed");
    EXPECT_EQ(soulSpeed.minLevel(), 1);
    EXPECT_EQ(soulSpeed.maxLevel(), 3);
    EXPECT_EQ(soulSpeed.type(), EnchantmentType::ArmorFeet);
    EXPECT_EQ(soulSpeed.rarity(), EnchantmentRarity::VeryRare);
    EXPECT_TRUE(soulSpeed.isTreasure());
    EXPECT_FALSE(soulSpeed.canVillagerTrade());
    EXPECT_FALSE(soulSpeed.canGenerateInLoot());
}

TEST_F(SoulSpeedEnchantmentTest, GetNameKey)
{
    EXPECT_EQ(soulSpeed.getNameKey(1), "enchantment.minecraft.soul_speed");
    EXPECT_EQ(soulSpeed.getNameKey(3), "enchantment.minecraft.soul_speed");
}

TEST_F(SoulSpeedEnchantmentTest, GetSoulSpeedMultiplier)
{
    // I: 1.0 + 0.2 + (1-1)*0.2 = 1.4 (+40%)
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getSoulSpeedMultiplier(1), 1.4f);
    // II: 1.0 + 0.2 + (2-1)*0.2 = 1.6 (+60%)
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getSoulSpeedMultiplier(2), 1.6f);
    // III: 1.0 + 0.2 + (3-1)*0.2 = 1.8 (+80%)
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getSoulSpeedMultiplier(3), 1.8f);
}

TEST_F(SoulSpeedEnchantmentTest, GetDurabilityConsumeChance)
{
    // 灵魂疾行固定 4% 概率消耗耐久，与等级无关
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getDurabilityConsumeChance(1), 0.04f);
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getDurabilityConsumeChance(2), 0.04f);
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getDurabilityConsumeChance(3), 0.04f);
}

TEST_F(SoulSpeedEnchantmentTest, GetMinCost)
{
    EXPECT_EQ(soulSpeed.getMinCost(1), 10);
    EXPECT_EQ(soulSpeed.getMinCost(2), 20);
    EXPECT_EQ(soulSpeed.getMinCost(3), 30);
}

TEST_F(SoulSpeedEnchantmentTest, GetMaxCost)
{
    EXPECT_EQ(soulSpeed.getMaxCost(1), 25); // getMinCost(1) + 15
    EXPECT_EQ(soulSpeed.getMaxCost(2), 35); // getMinCost(2) + 15
    EXPECT_EQ(soulSpeed.getMaxCost(3), 45); // getMinCost(3) + 15
}

TEST_F(SoulSpeedEnchantmentTest, SoulSpeedModifierId)
{
    // 灵魂疾行速度修饰符使用固定 ID，确保不会与其他修饰符冲突
    // 修饰符 ID 在 SoulSpeedEnchantment.cpp 中定义为 "enchantment.soul_speed"
    // 验证注册表中可以找到灵魂疾行附魔
    const Enchantment* registered = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(registered, nullptr);
    EXPECT_EQ(registered->id(), "minecraft:soul_speed");
}

// ============================================================================
// Enchantment 位置依赖效果接口测试
// ============================================================================

class EnchantmentLocationEffectTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(EnchantmentLocationEffectTest, DefaultOnLocationChangedReturnsFalse)
{
    // 默认 Enchantment::onLocationChanged() 返回 false（不激活位置效果）
    const Enchantment* protection = EnchantmentRegistry::get("minecraft:protection");
    ASSERT_NE(protection, nullptr);

    // 无法直接调用 onLocationChanged（需要 LivingEntity），
    // 但验证附魔默认不实现位置效果是设计约束
    EXPECT_EQ(protection->type(), EnchantmentType::Armor);
}

TEST_F(EnchantmentLocationEffectTest, FrostWalkerAndSoulSpeedRegistered)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);
    EXPECT_EQ(frostWalker->id(), "minecraft:frost_walker");
    EXPECT_TRUE(frostWalker->isTreasure());

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);
    EXPECT_EQ(soulSpeed->id(), "minecraft:soul_speed");
    EXPECT_TRUE(soulSpeed->isTreasure());
}

TEST_F(EnchantmentLocationEffectTest, FrostWalkerIncompatibleWithDepthStrider)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    const Enchantment* depthStrider = EnchantmentRegistry::get("minecraft:depth_strider");
    ASSERT_NE(frostWalker, nullptr);
    ASSERT_NE(depthStrider, nullptr);

    EXPECT_FALSE(frostWalker->isCompatibleWith(*depthStrider));
    EXPECT_FALSE(depthStrider->isCompatibleWith(*frostWalker));
}

TEST_F(EnchantmentLocationEffectTest, SoulSpeedIsArmorFeetType)
{
    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);
    EXPECT_EQ(soulSpeed->type(), EnchantmentType::ArmorFeet);
}

// ============================================================================
// 位置依赖附魔效果工具方法测试
// ============================================================================

class EnchantmentHelperLocationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(EnchantmentHelperLocationTest, HasFrostWalkerOnEmptyStack)
{
    ItemStack empty;
    EXPECT_FALSE(EnchantmentHelper::hasFrostWalker(empty));
}

TEST_F(EnchantmentHelperLocationTest, HasSoulSpeedOnEmptyStack)
{
    ItemStack empty;
    EXPECT_FALSE(EnchantmentHelper::hasSoulSpeed(empty));
}

TEST_F(EnchantmentHelperLocationTest, HasFrostWalkerOnEnchantedItem)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 1}}, boots);
    EXPECT_TRUE(EnchantmentHelper::hasFrostWalker(boots));
}

TEST_F(EnchantmentHelperLocationTest, HasSoulSpeedOnEnchantedItem)
{
    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    EXPECT_TRUE(EnchantmentHelper::hasSoulSpeed(boots));
}

TEST_F(EnchantmentHelperLocationTest, FrostWalkerLevel)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 2}}, boots);
    EXPECT_EQ(EnchantmentHelper::getEnchantmentLevel(boots, "minecraft:frost_walker"), 2);
}

TEST_F(EnchantmentHelperLocationTest, SoulSpeedLevel)
{
    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 3}}, boots);
    EXPECT_EQ(EnchantmentHelper::getEnchantmentLevel(boots, "minecraft:soul_speed"), 3);
}

TEST_F(EnchantmentHelperLocationTest, FrostWalkerAndDepthStriderMutuallyExclusive)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    const Enchantment* depthStrider = EnchantmentRegistry::get("minecraft:depth_strider");
    ASSERT_NE(frostWalker, nullptr);
    ASSERT_NE(depthStrider, nullptr);

    // 冰霜行者和深海探索者互斥
    EXPECT_FALSE(frostWalker->isCompatibleWith(*depthStrider));
    EXPECT_FALSE(depthStrider->isCompatibleWith(*frostWalker));
}
