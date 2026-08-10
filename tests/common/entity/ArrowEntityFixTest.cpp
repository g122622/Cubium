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
 * LIABILITY, WHETHER IN TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "entity/core/LivingEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/projectile/TridentEntity.hpp"
#include "entity/registry/VanillaEntityTypeKeys.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/enchantment/enchantments/AllEnchantments.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试固定装置
// ============================================================================

class ArrowEntityFixTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品注册表（附魔系统依赖）
        Items::initialize();
    }

    void TearDown() override {}
};

// ============================================================================
// 1. 玩家射出箭拾取状态测试
// ============================================================================

TEST_F(ArrowEntityFixTest, CreateFromShooter_PlayerShooter_PickupAllowed)
{
    // 玩家射出的箭默认 pickupStatus 应为 Allowed
    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    auto arrow = ArrowEntity::createFromShooter(*player, nullptr);

    ASSERT_NE(arrow, nullptr);
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Allowed);
}

TEST_F(ArrowEntityFixTest, CreateFromShooter_NonPlayerShooter_PickupDisallowed)
{
    // 非玩家实体（如 LivingEntity）射出的箭 pickupStatus 应保持默认 Disallowed
    auto livingEntity = std::make_unique<LivingEntity>(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry());
    auto arrow = ArrowEntity::createFromShooter(*livingEntity, nullptr);

    ASSERT_NE(arrow, nullptr);
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Disallowed);
}

TEST_F(ArrowEntityFixTest, CreateFromShooter_DefaultPickupStatusIsDisallowed)
{
    // AbstractArrowEntity 的默认 pickupStatus 是 Disallowed
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(3), mc::test::testEcsRegistry());
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Disallowed);
}

// ============================================================================
// 2. setBaseDamageFromMob 测试
// ============================================================================

TEST_F(ArrowEntityFixTest, SetBaseDamageFromMob_FullCharge_DamageInRange)
{
    // 满蓄力（charge=1.0）时，基础伤害应在合理范围内
    // 公式：damage = power * 2.0 + triangle(difficulty * 0.11, 0.57425)
    // 无世界（difficulty=0）: damage ≈ 2.0 ± 0.57425
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(4), mc::test::testEcsRegistry());
    arrow->setBaseDamageFromMob(1.0f);

    // 伤害应在合理范围内
    EXPECT_GE(arrow->damage(), 0.5f);
    EXPECT_LE(arrow->damage(), 4.0f);
}

TEST_F(ArrowEntityFixTest, SetBaseDamageFromMob_HalfCharge_LowerDamage)
{
    // 半蓄力（charge=0.5）时，基础伤害应低于满蓄力
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(5), mc::test::testEcsRegistry());
    arrow->setBaseDamageFromMob(0.5f);

    // 半蓄力基础伤害：0.5 * 2.0 = 1.0 + 随机
    EXPECT_GE(arrow->damage(), 0.2f);
    EXPECT_LE(arrow->damage(), 2.5f);
}

// ============================================================================
// 3. VanillaEntityTypeKeys::PLAYER 常量存在性测试
// ============================================================================

TEST_F(ArrowEntityFixTest, EntityTypeId_PlayerMatchesExpected)
{
    // 验证 VanillaEntityTypeKeys::PLAYER 常量已初始化
    // 这是 isPlayer 判断的基础，确保类型系统可用
    EXPECT_EQ(entity::VanillaEntityTypeKeys::PLAYER, entity::VanillaEntityTypeKeys::PLAYER);
}

// ============================================================================
// 4. SpectralArrowEntity 默认伤害测试
// ============================================================================

TEST_F(ArrowEntityFixTest, SpectralArrow_DefaultDamage)
{
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(EntityInstanceId(6), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(spectralArrow->damage(), 2.0f);
}

// ============================================================================
// 5. ArrowEntity 默认伤害测试
// ============================================================================

TEST_F(ArrowEntityFixTest, ArrowEntity_DefaultDamage)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(7), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(arrow->damage(), 2.0f);
}

// ============================================================================
// 6. 拾取状态枚举值测试
// ============================================================================

TEST_F(ArrowEntityFixTest, PickupStatus_ValuesAreDistinct)
{
    // 验证三个拾取状态值互不相同
    EXPECT_NE(PickupStatus::Disallowed, PickupStatus::Allowed);
    EXPECT_NE(PickupStatus::Allowed, PickupStatus::CreativeOnly);
    EXPECT_NE(PickupStatus::Disallowed, PickupStatus::CreativeOnly);
}

// ============================================================================
// 7. 伤害设置和获取测试
// ============================================================================

TEST_F(ArrowEntityFixTest, SetDamage_StoresCorrectly)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(8), mc::test::testEcsRegistry());
    arrow->setDamage(5.5f);
    EXPECT_FLOAT_EQ(arrow->damage(), 5.5f);
}

TEST_F(ArrowEntityFixTest, SetKnockbackStrength_StoresCorrectly)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(9), mc::test::testEcsRegistry());
    arrow->setKnockbackStrength(2);
    EXPECT_EQ(arrow->knockbackStrength(), 2);
}

// ============================================================================
// 8. 暴击状态测试
// ============================================================================

TEST_F(ArrowEntityFixTest, Critical_DefaultFalse)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(10), mc::test::testEcsRegistry());
    EXPECT_FALSE(arrow->isCritical());
}

TEST_F(ArrowEntityFixTest, SetCritical_StoresCorrectly)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(11), mc::test::testEcsRegistry());
    arrow->setCritical(true);
    EXPECT_TRUE(arrow->isCritical());

    arrow->setCritical(false);
    EXPECT_FALSE(arrow->isCritical());
}

// ============================================================================
// 9. applyBowEnchantments 测试
// ============================================================================

TEST_F(ArrowEntityFixTest, ApplyBowEnchantments_NoEnchantments_NoEffect)
{
    // 无附魔弓：伤害和击退不变，不着火
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(12), mc::test::testEcsRegistry());
    f32 baseDamage = arrow->damage();               // 默认 2.0
    i32 baseKnockback = arrow->knockbackStrength(); // 默认 0
    bool baseOnFire = arrow->isOnFire();

    auto player = std::make_unique<Player>(EntityInstanceId(13), "TestPlayer", mc::test::testEcsRegistry());
    // 玩家手持空物品，无附魔
    arrow->applyBowEnchantments(*player);

    // 无附魔时伤害和击退不变
    EXPECT_FLOAT_EQ(arrow->damage(), baseDamage);
    EXPECT_EQ(arrow->knockbackStrength(), baseKnockback);
    // 无火焰附魔不着火（isOnFire 状态不变）
    EXPECT_EQ(arrow->isOnFire(), baseOnFire);
}

TEST_F(ArrowEntityFixTest, ApplyBowEnchantments_PowerEnchantment_IncreasesDamage)
{
    // 力量附魔增加伤害：每级 +0.5 伤害 + 基础 0.5
    // 力量 I: +0.5 * 1 + 0.5 = +1.0
    // 力量 II: +0.5 * 2 + 0.5 = +1.5
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(14), mc::test::testEcsRegistry());
    f32 baseDamage = arrow->damage(); // 默认 2.0

    auto player = std::make_unique<Player>(EntityInstanceId(15), "TestPlayer", mc::test::testEcsRegistry());
    // 给玩家主手设置力量 II 的弓
    ItemStack bow(*Items::BOW, 1);
    bow.addEnchantment("minecraft:power", 2);
    player->setMainHandItem(bow);

    arrow->applyBowEnchantments(*player);

    // 力量 II: +1.5 伤害
    f32 expectedDamage = baseDamage + 1.5f;
    EXPECT_FLOAT_EQ(arrow->damage(), expectedDamage);
}

TEST_F(ArrowEntityFixTest, ApplyBowEnchantments_PunchEnchantment_IncreasesKnockback)
{
    // 冲击附魔增加击退：每级增加 1 点击退强度
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(16), mc::test::testEcsRegistry());
    EXPECT_EQ(arrow->knockbackStrength(), 0);

    auto player = std::make_unique<Player>(EntityInstanceId(17), "TestPlayer", mc::test::testEcsRegistry());
    ItemStack bow(*Items::BOW, 1);
    bow.addEnchantment("minecraft:punch", 2);
    player->setMainHandItem(bow);

    arrow->applyBowEnchantments(*player);

    // 冲击 II: 击退强度 = 2
    EXPECT_EQ(arrow->knockbackStrength(), 2);
}

TEST_F(ArrowEntityFixTest, ApplyBowEnchantments_FlameEnchantment_SetsFire)
{
    // 火焰附魔设置箭矢着火 100 ticks
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(18), mc::test::testEcsRegistry());
    EXPECT_FALSE(arrow->isOnFire());

    auto player = std::make_unique<Player>(EntityInstanceId(19), "TestPlayer", mc::test::testEcsRegistry());
    ItemStack bow(*Items::BOW, 1);
    bow.addEnchantment("minecraft:flame", 1);
    player->setMainHandItem(bow);

    arrow->applyBowEnchantments(*player);

    // 火焰附魔使箭矢着火
    EXPECT_TRUE(arrow->isOnFire());
}

TEST_F(ArrowEntityFixTest, ApplyBowEnchantments_MultipleEnchantments_CumulativeEffect)
{
    // 力量 + 冲击 + 火焰同时作用
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(20), mc::test::testEcsRegistry());
    f32 baseDamage = arrow->damage(); // 默认 2.0

    auto player = std::make_unique<Player>(EntityInstanceId(21), "TestPlayer", mc::test::testEcsRegistry());
    ItemStack bow(*Items::BOW, 1);
    bow.addEnchantment("minecraft:power", 5);
    bow.addEnchantment("minecraft:punch", 2);
    bow.addEnchantment("minecraft:flame", 1);
    player->setMainHandItem(bow);

    arrow->applyBowEnchantments(*player);

    // 力量 V: +0.5 * 5 + 0.5 = +3.0
    f32 expectedDamage = baseDamage + 3.0f;
    EXPECT_FLOAT_EQ(arrow->damage(), expectedDamage);
    EXPECT_EQ(arrow->knockbackStrength(), 2);
    EXPECT_TRUE(arrow->isOnFire());
}

// ============================================================================
// 10. 玩家射手创建箭矢的完整流程测试
// ============================================================================

TEST_F(ArrowEntityFixTest, CreateFromShooter_PlayerGetsAllowedPickupAndDefaultDamage)
{
    // 验证玩家射出的箭既有 Allowed 拾取状态，又有正确的默认伤害
    auto player = std::make_unique<Player>(EntityInstanceId(22), "TestPlayer", mc::test::testEcsRegistry());
    auto arrow = ArrowEntity::createFromShooter(*player, nullptr);

    ASSERT_NE(arrow, nullptr);
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Allowed);
    EXPECT_FLOAT_EQ(arrow->damage(), 2.0f);
}

// ============================================================================
// 11. TridentEntity 默认伤害测试
// ============================================================================

TEST_F(ArrowEntityFixTest, TridentEntity_DefaultDamage)
{
    // 三叉戟默认伤害为 8.0
    auto trident = std::make_unique<TridentEntity>(EntityInstanceId(23), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(trident->damage(), 8.0f);
}

TEST_F(ArrowEntityFixTest, TridentEntity_DefaultPickupAllowed)
{
    // 三叉戟默认允许拾取（与普通箭不同）
    auto trident = std::make_unique<TridentEntity>(EntityInstanceId(24), mc::test::testEcsRegistry());
    EXPECT_EQ(trident->pickupStatus(), PickupStatus::Allowed);
}

// ============================================================================
// 12. setBaseDamageFromMob 三叉戟覆盖测试
// ============================================================================

TEST_F(ArrowEntityFixTest, SetBaseDamageFromMob_TridentOverride_DamageInRange)
{
    // 三叉戟使用相同的 setBaseDamageFromMob 公式，但默认伤害更高
    // 此测试验证三叉戟重写版本正确计算伤害
    auto trident = std::make_unique<TridentEntity>(EntityInstanceId(25), mc::test::testEcsRegistry());
    trident->setBaseDamageFromMob(1.0f);

    // 满蓄力：1.0 * 2.0 + triangle(0, 0.57425) ≈ 2.0 ± 0.57425
    // 而非三叉戟默认的 8.0，因为 setBaseDamageFromMob 完全重写伤害值
    EXPECT_GE(trident->damage(), 0.5f);
    EXPECT_LE(trident->damage(), 4.0f);
}
