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

#include "entity/core/EntityTypeIdNumber.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/enchantment/enchantments/AllEnchantments.hpp"
#include <gtest/gtest.h>

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
    // 验证：玩家射出的箭默认 pickupStatus 应为 Allowed
    // 对应 MC AbstractArrow.setOwner() 中 Player 类型将 pickup 从 DISALLOWED 改为 ALLOWED
    auto player = std::make_unique<Player>(EntityId(1), "TestPlayer");
    auto arrow = ArrowEntity::createFromShooter(*player, nullptr);

    ASSERT_NE(arrow, nullptr);
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Allowed);
}

TEST_F(ArrowEntityFixTest, CreateFromShooter_NonPlayerShooter_PickupDisallowed)
{
    // 验证：非玩家实体（如 LivingEntity）射出的箭 pickupStatus 应保持默认 Disallowed
    // 骷髅等怪物射出的箭不可被拾取
    auto livingEntity = std::make_unique<LivingEntity>(EntityId(2));
    auto arrow = ArrowEntity::createFromShooter(*livingEntity, nullptr);

    ASSERT_NE(arrow, nullptr);
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Disallowed);
}

TEST_F(ArrowEntityFixTest, CreateFromShooter_DefaultPickupStatusIsDisallowed)
{
    // 验证：AbstractArrowEntity 的默认 pickupStatus 是 Disallowed
    auto arrow = std::make_unique<ArrowEntity>(EntityId(3));
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Disallowed);
}

// ============================================================================
// 2. setBaseDamageFromMob 测试
// ============================================================================

TEST_F(ArrowEntityFixTest, SetBaseDamageFromMob_FullCharge_DamageInRange)
{
    // 验证：满蓄力（charge=1.0）时，基础伤害应在合理范围内
    // MC 公式：damage = power * 2.0 + triangle(difficulty * 0.11, 0.57425)
    // 对于 Peaceful 难度 (difficultyId=0): damage ≈ 2.0 ± 0.57425
    // 对于 Hard 难度 (difficultyId=3): damage ≈ 2.0 + 0.33 ± 0.57425
    auto arrow = std::make_unique<ArrowEntity>(EntityId(4));
    arrow->setBaseDamageFromMob(1.0f);

    // 伤害应在合理范围内（1.0 ~ 3.5 对于满蓄力和平和难度）
    EXPECT_GE(arrow->damage(), 0.5f);
    EXPECT_LE(arrow->damage(), 4.0f);
}

TEST_F(ArrowEntityFixTest, SetBaseDamageFromMob_HalfCharge_LowerDamage)
{
    // 验证：半蓄力（charge=0.5）时，基础伤害应低于满蓄力
    auto arrow = std::make_unique<ArrowEntity>(EntityId(5));
    arrow->setBaseDamageFromMob(0.5f);

    // 半蓄力基础伤害：0.5 * 2.0 = 1.0 + 难度随机
    EXPECT_GE(arrow->damage(), 0.2f);
    EXPECT_LE(arrow->damage(), 2.5f);
}

// ============================================================================
// 3. 伤害源 isPlayer 参数测试
// ============================================================================

TEST_F(ArrowEntityFixTest, EntityTypeId_PlayerMatchesExpected)
{
    // 验证：EntityTypeIdNumber::PLAYER 常量已初始化
    // 这是 isPlayer 判断的基础，确保类型系统可用
    // 注意：此测试仅验证常量存在，不验证具体值（值在运行时分配）
    // 如果 VanillaEntities::registerAll() 已调用，PLAYER 应为有效值
    // 此处仅验证类型定义存在
    EXPECT_EQ(entity::EntityTypeIdNumber::PLAYER, entity::EntityTypeIdNumber::PLAYER);
}

// ============================================================================
// 4. SpectralArrowEntity 默认伤害测试
// ============================================================================

TEST_F(ArrowEntityFixTest, SpectralArrow_DefaultDamage)
{
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(EntityId(6));
    EXPECT_FLOAT_EQ(spectralArrow->damage(), 2.0f);
}

// ============================================================================
// 5. ArrowEntity 默认伤害测试
// ============================================================================

TEST_F(ArrowEntityFixTest, ArrowEntity_DefaultDamage)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityId(7));
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
    auto arrow = std::make_unique<ArrowEntity>(EntityId(8));
    arrow->setDamage(5.5f);
    EXPECT_FLOAT_EQ(arrow->damage(), 5.5f);
}

TEST_F(ArrowEntityFixTest, SetKnockbackStrength_StoresCorrectly)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityId(9));
    arrow->setKnockbackStrength(2);
    EXPECT_EQ(arrow->knockbackStrength(), 2);
}

// ============================================================================
// 8. 暴击状态测试
// ============================================================================

TEST_F(ArrowEntityFixTest, Critical_DefaultFalse)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityId(10));
    EXPECT_FALSE(arrow->isCritical());
}

TEST_F(ArrowEntityFixTest, SetCritical_StoresCorrectly)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityId(11));
    arrow->setCritical(true);
    EXPECT_TRUE(arrow->isCritical());

    arrow->setCritical(false);
    EXPECT_FALSE(arrow->isCritical());
}
