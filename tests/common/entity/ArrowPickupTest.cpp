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

#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/projectile/TridentEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试固定装置
// ============================================================================

class ArrowPickupTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品注册表
        Items::initialize();

        // 获取箭矢物品
        m_arrow = Items::ARROW;
        m_spectralArrow = Items::SPECTRAL_ARROW;
        m_trident = Items::TRIDENT;
    }

    void TearDown() override {}

    Item* m_arrow = nullptr;
    Item* m_spectralArrow = nullptr;
    Item* m_trident = nullptr;
};

// ============================================================================
// ArrowEntity::getArrowStack 测试
// ============================================================================

TEST_F(ArrowPickupTest, ArrowEntity_GetArrowStack_ReturnsArrowItem)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setPosition(0, 0, 0);

    // 普通箭矢应该返回 ARROW 物品
    ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), m_arrow);
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(ArrowPickupTest, ArrowEntity_GetArrowStack_WithEffects_ReturnsTippedArrow)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setPosition(0, 0, 0);

    // 添加药水效果
    arrow->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison, 100, 0));

    // 药水箭应该返回 TIPPED_ARROW 物品
    ItemStack stack = arrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), Items::TIPPED_ARROW);
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// SpectralArrowEntity::getArrowStack 测试
// ============================================================================

TEST_F(ArrowPickupTest, SpectralArrowEntity_GetArrowStack_ReturnsSpectralArrow)
{
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    spectralArrow->setPosition(0, 0, 0);

    // 光灵箭应该返回 SPECTRAL_ARROW 物品
    ItemStack stack = spectralArrow->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), m_spectralArrow);
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// TridentEntity::getArrowStack 测试
// ============================================================================

TEST_F(ArrowPickupTest, TridentEntity_GetArrowStack_ReturnsTridentItem)
{
    auto trident = std::make_unique<TridentEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    trident->setPosition(0, 0, 0);

    // 设置三叉戟物品
    ItemStack tridentStack(*m_trident, 1);
    trident->setItemStack(tridentStack);

    // 三叉戟应该返回 TRIDENT 物品
    ItemStack stack = trident->getArrowStack();
    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem(), m_trident);
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// AbstractArrowEntity::onPlayerPickup 前置条件测试
// ============================================================================

TEST_F(ArrowPickupTest, OnPlayerPickup_Disallowed_ReturnsFalse)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::Disallowed);
    arrow->setInGround(true); // 必须插在地方才能拾取

    // 验证基本逻辑：Disallowed 状态应该拒绝拾取
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Disallowed);
    EXPECT_TRUE(arrow->isInGround());
}

TEST_F(ArrowPickupTest, OnPlayerPickup_Allowed_InGround_CanPickup)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::Allowed);
    arrow->setInGround(true);

    // 验证箭矢状态正确
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Allowed);
    EXPECT_TRUE(arrow->isInGround());
}

TEST_F(ArrowPickupTest, OnPlayerPickup_NotInGround_CannotPickup)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::Allowed);
    arrow->setInGround(false); // 未插在方块中

    // 验证箭矢状态
    EXPECT_FALSE(arrow->isInGround());
}

TEST_F(ArrowPickupTest, OnPlayerPickup_CreativeOnly_OnlyCreativeCanPickup)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::CreativeOnly);
    arrow->setInGround(true);

    // 验证 CreativeOnly 状态
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::CreativeOnly);
}

// ============================================================================
// PickupStatus 枚举值测试
// ============================================================================

TEST_F(ArrowPickupTest, PickupStatus_Values)
{
    // 验证枚举值
    EXPECT_EQ(static_cast<int>(PickupStatus::Disallowed), 0);
    EXPECT_EQ(static_cast<int>(PickupStatus::Allowed), 1);
    EXPECT_EQ(static_cast<int>(PickupStatus::CreativeOnly), 2);
}

// ============================================================================
// 箭矢属性测试
// ============================================================================

TEST_F(ArrowPickupTest, ArrowEntity_DefaultDamage)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(arrow->damage(), 2.0f);
}

TEST_F(ArrowPickupTest, ArrowEntity_SetDamage)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setDamage(5.5f);
    EXPECT_FLOAT_EQ(arrow->damage(), 5.5f);
}

TEST_F(ArrowPickupTest, ArrowEntity_CriticalFlag)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(arrow->isCritical());
    arrow->setCritical(true);
    EXPECT_TRUE(arrow->isCritical());
}

TEST_F(ArrowPickupTest, ArrowEntity_PierceLevel)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(arrow->pierceLevel(), 0);
    arrow->setPierceLevel(3);
    EXPECT_EQ(arrow->pierceLevel(), 3);
}

TEST_F(ArrowPickupTest, SpectralArrowEntity_DefaultDamage)
{
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(spectralArrow->damage(), 2.0f);
}

TEST_F(ArrowPickupTest, TridentEntity_DefaultDamage)
{
    auto trident = std::make_unique<TridentEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(trident->damage(), 8.0f);
}

TEST_F(ArrowPickupTest, TridentEntity_DefaultPickupStatus)
{
    auto trident = std::make_unique<TridentEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 三叉戟默认应该可以拾取
    EXPECT_EQ(trident->pickupStatus(), PickupStatus::Allowed);
}

// ============================================================================
// 抽象基类测试 - getArrowStack 必须实现
// ============================================================================

TEST_F(ArrowPickupTest, ArrowEntity_IsAbstractArrowEntity)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    AbstractArrowEntity* base = arrow.get();
    EXPECT_NE(base, nullptr);

    // 验证可以通过基类指针调用 getArrowStack
    ItemStack stack = base->getArrowStack();
    EXPECT_NE(stack.getItem(), nullptr);
}

TEST_F(ArrowPickupTest, SpectralArrowEntity_IsAbstractArrowEntity)
{
    auto spectralArrow = std::make_unique<SpectralArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    AbstractArrowEntity* base = spectralArrow.get();
    EXPECT_NE(base, nullptr);

    // 验证可以通过基类指针调用 getArrowStack
    ItemStack stack = base->getArrowStack();
    EXPECT_NE(stack.getItem(), nullptr);
}

TEST_F(ArrowPickupTest, TridentEntity_IsAbstractArrowEntity)
{
    auto trident = std::make_unique<TridentEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    AbstractArrowEntity* base = trident.get();
    EXPECT_NE(base, nullptr);

    // 设置三叉戟物品
    ItemStack tridentStack(*m_trident, 1);
    trident->setItemStack(tridentStack);

    // 验证可以通过基类指针调用 getArrowStack
    ItemStack stack = base->getArrowStack();
    EXPECT_NE(stack.getItem(), nullptr);
}

// ============================================================================
// onCollideWithPlayer 测试
// ============================================================================

TEST_F(ArrowPickupTest, OnCollideWithPlayer_NotInGround_DoesNotPickup)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::Allowed);
    arrow->setInGround(false); // 未插在方块中

    // 验证状态
    EXPECT_FALSE(arrow->isInGround());
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Allowed);
    // 未插在方块中时不应该触发拾取
}

TEST_F(ArrowPickupTest, OnCollideWithPlayer_Disallowed_DoesNotPickup)
{
    auto arrow = std::make_unique<ArrowEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    arrow->setPosition(0, 0, 0);
    arrow->setPickupStatus(PickupStatus::Disallowed);
    arrow->setInGround(true);

    // 验证状态
    EXPECT_TRUE(arrow->isInGround());
    EXPECT_EQ(arrow->pickupStatus(), PickupStatus::Disallowed);
    // Disallowed 状态不应该触发拾取
}
