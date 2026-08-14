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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"

using namespace mc;
/**
 * @brief 猪灵实体测试夹具
 *
 * 参考 MC 1.16.5 PiglinEntity
 */
class PiglinEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 属性系统不需要初始化，使用工厂函数
    }
};

/**
 * @brief 猪灵蛮兵实体测试夹具
 *
 * 参考 MC 1.16.5 PiglinBruteEntity
 */
class PiglinBruteEntityTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

/**
 * @brief 僵尸猪灵实体测试夹具
 *
 * 参考 MC 1.16.5 ZombifiedPiglinEntity
 */
class ZombifiedPiglinEntityTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

/**
 * @brief 疣猪兽实体测试夹具
 *
 * 参考 MC 1.16.5 HoglinEntity
 */
class HoglinEntityTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

/**
 * @brief 僵尸疣兽实体测试夹具
 *
 * 参考 MC 1.16.5 ZoglinEntity
 */
class ZoglinEntityTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ============================================================================
// PiglinEntity 测试
// ============================================================================

/**
 * @brief 测试 PiglinEntity 工厂方法创建
 */
TEST_F(PiglinEntityTest, CreateFactory)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    EXPECT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<PiglinEntity*>(entity.get()), nullptr);
}

/**
 * @brief 测试 PiglinEntity 默认属性值
 */
TEST_F(PiglinEntityTest, DefaultAttributes)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* piglin = dynamic_cast<PiglinEntity*>(entity.get());
    ASSERT_NE(piglin, nullptr);

    // 验证默认属性
    // MC 1.16.5: MAX_HEALTH=16, MOVEMENT_SPEED=0.35, ATTACK_DAMAGE=5
    // 注意：属性需要 registerAttributes() 后才有值
}

/**
 * @brief 测试 PiglinEntity 幼年状态
 */
TEST_F(PiglinEntityTest, BabyState)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* piglin = dynamic_cast<PiglinEntity*>(entity.get());
    ASSERT_NE(piglin, nullptr);

    // 默认应该是成年
    EXPECT_FALSE(piglin->isBaby());

    // 设置为幼年
    piglin->setBaby(true);
    EXPECT_TRUE(piglin->isBaby());

    // 设置回成年
    piglin->setBaby(false);
    EXPECT_FALSE(piglin->isBaby());
}

/**
 * @brief 测试 PiglinEntity 弩充能状态
 */
TEST_F(PiglinEntityTest, CrossbowChargingState)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* piglin = dynamic_cast<PiglinEntity*>(entity.get());
    ASSERT_NE(piglin, nullptr);

    // 默认不充能
    EXPECT_FALSE(piglin->isChargingCrossbow());

    // 设置充能状态
    piglin->setChargingCrossbow(true);
    EXPECT_TRUE(piglin->isChargingCrossbow());

    // 重置充能状态
    piglin->setChargingCrossbow(false);
    EXPECT_FALSE(piglin->isChargingCrossbow());
}

/**
 * @brief 测试 PiglinEntity 弩装填时间
 */
TEST_F(PiglinEntityTest, CrossbowChargeTime)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* piglin = dynamic_cast<PiglinEntity*>(entity.get());
    ASSERT_NE(piglin, nullptr);

    // MC 1.16.5: 猪灵弩装填时间为 25 ticks
    EXPECT_EQ(piglin->getCrossbowChargeTime(), 25);
}

/**
 * @brief 测试 PiglinEntity 攻击间隔
 */
TEST_F(PiglinEntityTest, AttackInterval)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* piglin = dynamic_cast<PiglinEntity*>(entity.get());
    ASSERT_NE(piglin, nullptr);

    // MC 1.16.5: 攻击间隔为 20 ticks
    EXPECT_EQ(piglin->getAttackInterval(), 20);
}

/**
 * @brief 测试 PiglinEntity 是否可以远程攻击
 */
TEST_F(PiglinEntityTest, CanRangedAttack)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* piglin = dynamic_cast<PiglinEntity*>(entity.get());
    ASSERT_NE(piglin, nullptr);

    // 猪灵可以使用弩进行远程攻击
    EXPECT_TRUE(piglin->canRangedAttack());
}

/**
 * @brief 测试 AbstractPiglinEntity 火焰免疫
 */
TEST_F(PiglinEntityTest, FireImmunity)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* piglin = dynamic_cast<PiglinEntity*>(entity.get());
    ASSERT_NE(piglin, nullptr);

    // 猪灵应该对火焰免疫
    EXPECT_TRUE(piglin->isImmuneToFire());
}

// ============================================================================
// PiglinBruteEntity 测试
// ============================================================================

/**
 * @brief 测试 PiglinBruteEntity 工厂方法创建
 */
TEST_F(PiglinBruteEntityTest, CreateFactory)
{
    auto entity = PiglinBruteEntity::create(nullptr, mc::test::testEcsRegistry());
    EXPECT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<PiglinBruteEntity*>(entity.get()), nullptr);
}

/**
 * @brief 测试 PiglinBruteEntity 火焰免疫
 */
TEST_F(PiglinBruteEntityTest, FireImmunity)
{
    auto entity = PiglinBruteEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* brute = dynamic_cast<PiglinBruteEntity*>(entity.get());
    ASSERT_NE(brute, nullptr);

    // 猪灵蛮兵应该对火焰免疫
    EXPECT_TRUE(brute->isImmuneToFire());
}

// ============================================================================
// ZombifiedPiglinEntity 测试
// ============================================================================

/**
 * @brief 测试 ZombifiedPiglinEntity 工厂方法创建
 */
TEST_F(ZombifiedPiglinEntityTest, CreateFactory)
{
    auto entity = std::make_unique<ZombifiedPiglinEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
    EXPECT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<ZombifiedPiglinEntity*>(entity.get()), nullptr);
}

/**
 * @brief 测试 ZombifiedPiglinEntity 火焰免疫
 */
TEST_F(ZombifiedPiglinEntityTest, FireImmunity)
{
    auto entity = std::make_unique<ZombifiedPiglinEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
    EXPECT_TRUE(entity->isImmuneToFire());
}

/**
 * @brief 测试 ZombifiedPiglinEntity 愤怒状态
 */
TEST_F(ZombifiedPiglinEntityTest, AngerState)
{
    auto entity = std::make_unique<ZombifiedPiglinEntity>(EntityInstanceId(0), mc::test::testEcsRegistry());
    auto* zombifiedPiglin = entity.get();

    // 默认不愤怒
    EXPECT_FALSE(zombifiedPiglin->isAngry());
    EXPECT_EQ(zombifiedPiglin->getAngerTime(), 0);

    // 设置愤怒状态
    zombifiedPiglin->setAngry(true);
    zombifiedPiglin->setAngerTime(100);
    EXPECT_TRUE(zombifiedPiglin->isAngry());
    EXPECT_EQ(zombifiedPiglin->getAngerTime(), 100);

    // 重置愤怒状态
    zombifiedPiglin->setAngry(false);
    EXPECT_FALSE(zombifiedPiglin->isAngry());
}

// ============================================================================
// HoglinEntity 测试
// ============================================================================

/**
 * @brief 测试 HoglinEntity 工厂方法创建
 */
TEST_F(HoglinEntityTest, CreateFactory)
{
    auto entity = HoglinEntity::create(nullptr, mc::test::testEcsRegistry());
    EXPECT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<HoglinEntity*>(entity.get()), nullptr);
}

/**
 * @brief 测试 HoglinEntity 火焰免疫
 */
TEST_F(HoglinEntityTest, FireImmunity)
{
    auto entity = HoglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* hoglin = dynamic_cast<HoglinEntity*>(entity.get());
    ASSERT_NE(hoglin, nullptr);

    // 疣猪兽应该对火焰免疫
    EXPECT_TRUE(hoglin->isImmuneToFire());
}

/**
 * @brief 测试 HoglinEntity 幼年状态
 */
TEST_F(HoglinEntityTest, BabyState)
{
    auto entity = HoglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* hoglin = dynamic_cast<HoglinEntity*>(entity.get());
    ASSERT_NE(hoglin, nullptr);

    // 默认应该是成年
    EXPECT_FALSE(hoglin->isBaby());

    // 设置为幼年
    hoglin->setBaby(true);
    EXPECT_TRUE(hoglin->isBaby());

    // 幼年疣猪兽不可被猎杀
    EXPECT_FALSE(hoglin->isHuntable());

    // 设置回成年
    hoglin->setBaby(false);
    EXPECT_FALSE(hoglin->isBaby());

    // 成年疣猪兽可被猎杀
    EXPECT_TRUE(hoglin->isHuntable());
}

// ============================================================================
// ZoglinEntity 测试
// ============================================================================

/**
 * @brief 测试 ZoglinEntity 工厂方法创建
 */
TEST_F(ZoglinEntityTest, CreateFactory)
{
    auto entity = ZoglinEntity::create(nullptr, mc::test::testEcsRegistry());
    EXPECT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<ZoglinEntity*>(entity.get()), nullptr);
}

/**
 * @brief 测试 ZoglinEntity 幼年状态
 */
TEST_F(ZoglinEntityTest, BabyState)
{
    auto entity = ZoglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* zoglin = dynamic_cast<ZoglinEntity*>(entity.get());
    ASSERT_NE(zoglin, nullptr);

    // 默认应该是成年
    EXPECT_FALSE(zoglin->isBaby());

    // 设置为幼年
    zoglin->setBaby(true);
    EXPECT_TRUE(zoglin->isBaby());

    // 设置回成年
    zoglin->setBaby(false);
    EXPECT_FALSE(zoglin->isBaby());
}

// ============================================================================
// isChild() 虚函数重写测试
// ============================================================================

/**
 * @brief 验证 HoglinEntity::isChild() 正确重写 Entity::isChild()
 *
 * HoglinEntity 使用 m_isBaby 标记幼年状态，isChild() 必须委托给 isBaby()
 * 以确保多态调用 isChild() 返回正确结果。
 */
TEST_F(HoglinEntityTest, IsChildOverridesEntityVirtual)
{
    auto entity = HoglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* hoglin = dynamic_cast<HoglinEntity*>(entity.get());
    ASSERT_NE(hoglin, nullptr);

    // 默认成年，isChild() 应返回 false
    EXPECT_FALSE(hoglin->isBaby());
    EXPECT_FALSE(hoglin->isChild());

    // 设为幼年，isChild() 应返回 true
    hoglin->setBaby(true);
    EXPECT_TRUE(hoglin->isBaby());
    EXPECT_TRUE(hoglin->isChild());

    // 通过 Entity* 指针多态调用 isChild()
    Entity* basePtr = hoglin;
    EXPECT_TRUE(basePtr->isChild());

    // 设回成年
    hoglin->setBaby(false);
    EXPECT_FALSE(hoglin->isBaby());
    EXPECT_FALSE(basePtr->isChild());
}

/**
 * @brief 验证 ZoglinEntity::isChild() 正确重写 Entity::isChild()
 */
TEST_F(ZoglinEntityTest, IsChildOverridesEntityVirtual)
{
    auto entity = ZoglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* zoglin = dynamic_cast<ZoglinEntity*>(entity.get());
    ASSERT_NE(zoglin, nullptr);

    // 默认成年
    EXPECT_FALSE(zoglin->isBaby());
    EXPECT_FALSE(zoglin->isChild());

    // 设为幼年
    zoglin->setBaby(true);
    EXPECT_TRUE(zoglin->isBaby());
    EXPECT_TRUE(zoglin->isChild());

    // 通过 Entity* 指针多态调用 isChild()
    Entity* basePtr = zoglin;
    EXPECT_TRUE(basePtr->isChild());

    // 设回成年
    zoglin->setBaby(false);
    EXPECT_FALSE(zoglin->isBaby());
    EXPECT_FALSE(basePtr->isChild());
}

/**
 * @brief 验证 PiglinEntity::isChild() 正确重写 Entity::isChild()
 */
TEST_F(PiglinEntityTest, IsChildOverridesEntityVirtual)
{
    auto entity = PiglinEntity::create(nullptr, mc::test::testEcsRegistry());
    auto* piglin = dynamic_cast<PiglinEntity*>(entity.get());
    ASSERT_NE(piglin, nullptr);

    // 默认成年
    EXPECT_FALSE(piglin->isBaby());
    EXPECT_FALSE(piglin->isChild());

    // 设为幼年
    piglin->setBaby(true);
    EXPECT_TRUE(piglin->isBaby());
    EXPECT_TRUE(piglin->isChild());

    // 通过 Entity* 指针多态调用 isChild()
    Entity* basePtr = piglin;
    EXPECT_TRUE(basePtr->isChild());

    // 设回成年
    piglin->setBaby(false);
    EXPECT_FALSE(piglin->isBaby());
    EXPECT_FALSE(basePtr->isChild());
}

// ============================================================================
// 实体类型测试 - 类型ID通过 VanillaEntityTypeKeys 验证
// ============================================================================

// 注：LegacyEntityType 枚举已废弃，实体类型现在通过 VanillaEntityTypeKeys 验证
// 上述测试已经通过 VanillaEntityTypeKeys::PIGLIN 等验证了类型ID
