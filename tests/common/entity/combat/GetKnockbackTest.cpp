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
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/util/math/MathUtils.hpp"

using namespace mc;
using namespace mc::entity::attribute;

namespace {

// ============================================================================
// 测试辅助：带世界的 LivingEntity
// ============================================================================

class KnockbackTestWorld : public mc::test::BaseTestWorld {
public:
    KnockbackTestWorld() = default;
};

class KnockbackTestEntity : public LivingEntity {
public:
    explicit KnockbackTestEntity(EntityInstanceId id, ecs::EntityRegistry& registry = mc::test::testEcsRegistry())
        : LivingEntity(id, nullptr, registry)
    {
        registerAttributes();
        attributes().setBaseValue(Attributes::MAX_HEALTH, 20.0);
        attributes().setBaseValue(Attributes::KNOCKBACK_RESISTANCE, 0.0);
        // 注册 ATTACK_KNOCKBACK 属性（基类不注册，需要子类手动注册）
        attributes().registerAttribute(*Attributes::attackKnockback());
        attributes().setBaseValue(Attributes::ATTACK_KNOCKBACK, 0.0);
        setHealth(20.0f);
    }

    void setKnockbackResistance(f64 value) { attributes().setBaseValue(Attributes::KNOCKBACK_RESISTANCE, value); }
    void setAttackKnockback(f64 value) { attributes().setBaseValue(Attributes::ATTACK_KNOCKBACK, value); }

    static std::unique_ptr<Entity> create(IWorld* /*world*/)
    {
        return std::make_unique<KnockbackTestEntity>(0, mc::test::testEcsRegistry());
    }
};

class KnockbackTestMob : public MobEntity {
public:
    explicit KnockbackTestMob(EntityInstanceId id)
        : MobEntity(id, mc::test::testEcsRegistry())
    {
        registerAttributes();
        attributes().setBaseValue(Attributes::MAX_HEALTH, 20.0);
        attributes().setBaseValue(Attributes::KNOCKBACK_RESISTANCE, 0.0);
        attributes().setBaseValue(Attributes::ATTACK_DAMAGE, 2.0);
        // 注册 ATTACK_KNOCKBACK 属性（基类不注册，需要子类手动注册）
        attributes().registerAttribute(*Attributes::attackKnockback());
        attributes().setBaseValue(Attributes::ATTACK_KNOCKBACK, 0.0);
        setHealth(20.0f);
    }

    void setAttackKnockback(f64 value) { attributes().setBaseValue(Attributes::ATTACK_KNOCKBACK, value); }

    static std::unique_ptr<Entity> create(IWorld* /*world*/) { return std::make_unique<KnockbackTestMob>(0); }
};

// ============================================================================
// LivingEntity::getKnockback 测试
// ============================================================================

TEST(GetKnockbackTest, DefaultAttributeReturnsZero)
{
    // 默认 ATTACK_KNOCKBACK 为 0.0，无附魔武器时应返回 0.0
    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());

    f32 knockback = mob.getKnockback(target);

    // (0.0 + 0) / 2.0 = 0.0
    EXPECT_FLOAT_EQ(knockback, 0.0f);
}

TEST(GetKnockbackTest, AttributeKnockbackHalved)
{
    // ATTACK_KNOCKBACK 属性值为 1.0 时，应返回 0.5
    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());

    mob.setAttackKnockback(1.0);

    f32 knockback = mob.getKnockback(target);

    // (1.0 + 0) / 2.0 = 0.5
    EXPECT_FLOAT_EQ(knockback, 0.5f);
}

TEST(GetKnockbackTest, LargeAttributeKnockbackHalved)
{
    // ATTACK_KNOCKBACK 属性值为 2.0 时，应返回 1.0
    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());

    mob.setAttackKnockback(2.0);

    f32 knockback = mob.getKnockback(target);

    // (2.0 + 0) / 2.0 = 1.0
    EXPECT_FLOAT_EQ(knockback, 1.0f);
}

TEST(GetKnockbackTest, ZeroKnockbackMeansNoExtraKnockbackFromMob)
{
    // 大多数生物 ATTACK_KNOCKBACK 默认为 0.0
    // 这意味着 getKnockback() 返回 0.0，causeExtraKnockback 不施加额外击退
    // 但 hurt() 中的基础击退仍然存在
    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());

    f32 knockback = mob.getKnockback(target);
    EXPECT_FLOAT_EQ(knockback, 0.0f);
}

TEST(GetKnockbackTest, KnockbackDivisionByTwo)
{
    // getKnockback 始终将结果除以 2.0
    // 这是与 hurt() 中基础击退 (0.4) 的配合设计
    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());

    // 测试各种属性值
    mob.setAttackKnockback(0.5);
    EXPECT_FLOAT_EQ(mob.getKnockback(target), 0.25f);

    mob.setAttackKnockback(3.0);
    EXPECT_FLOAT_EQ(mob.getKnockback(target), 1.5f);
}

// ============================================================================
// LivingEntity::getKnockback 击退附魔接入测试（任务 #310）
// ============================================================================
//
// 验证 getKnockback 正确接入 KnockbackEnchantment::getKnockbackBonus（每级 +1.0，对齐 vanilla
// KNOCKBACK 附魔组件 linear(base=1.0, per_level_above_first=1.0)），并经 /2.0 得最终强度。
//
// 对齐 vanilla LivingEntity.java:1515-1520：
//   getKnockback = (getAttributeValue(ATTACK_KNOCKBACK) + EnchantmentHelper.modifyKnockback) / 2.0F
//   modifyKnockback 累加 KNOCKBACK 附魔组件值（每级 +1.0）。
//   玩家/mob ATTACK_KNOCKBACK 默认 0，故：
//     Knockback I  getKnockback = (0 + 1.0) / 2.0 = 0.5
//     Knockback II getKnockback = (0 + 2.0) / 2.0 = 1.0
//
// 任务 #310 修复前偏差：KnockbackEnchantment::getKnockbackBonus = level*0.5（应为 level*1.0），
//   致 Knockback II getKnockback = (0+1.0)/2.0 = 0.5（vanilla 1.0，2 倍偏差）。且 Player::attack
//   绕过 getKnockback 直接用 getEnchantmentLevel 传 causeExtraKnockback。本测试固定 getKnockback
//   数值，捕捉 getKnockbackBonus 回归。

TEST(GetKnockbackTest, KnockbackEnchantmentLevelOneBonus)
{
    // Knockback I 武器：getKnockback = (0 + 1.0) / 2.0 = 0.5
    item::enchant::EnchantmentRegistry::clear();
    item::enchant::EnchantmentRegistry::initialize();
    Items::initialize();

    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());

    ItemStack sword(Items::DIAMOND_SWORD, 1);
    sword.addEnchantment("minecraft:knockback", 1);
    mob.setMainHandItem(sword);

    // Knockback I：(ATTACK_KNOCKBACK=0 + getKnockbackBonus(1)=1.0) / 2.0 = 0.5
    EXPECT_FLOAT_EQ(mob.getKnockback(target), 0.5f);

    item::enchant::EnchantmentRegistry::clear();
}

TEST(GetKnockbackTest, KnockbackEnchantmentLevelTwoBonus)
{
    // Knockback II 武器：getKnockback = (0 + 2.0) / 2.0 = 1.0（对齐 vanilla，任务 #310 修复点）
    item::enchant::EnchantmentRegistry::clear();
    item::enchant::EnchantmentRegistry::initialize();
    Items::initialize();

    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());

    ItemStack sword(Items::DIAMOND_SWORD, 1);
    sword.addEnchantment("minecraft:knockback", 2);
    mob.setMainHandItem(sword);

    // Knockback II：(0 + getKnockbackBonus(2)=2.0) / 2.0 = 1.0
    // 修复前 getKnockbackBonus=level*0.5 → (0+1.0)/2.0=0.5，本断言会失败（expect 1.0 got 0.5）。
    EXPECT_FLOAT_EQ(mob.getKnockback(target), 1.0f);

    item::enchant::EnchantmentRegistry::clear();
}

TEST(GetKnockbackTest, KnockbackEnchantmentStacksWithAttackKnockbackAttribute)
{
    // Knockback II + ATTACK_KNOCKBACK 属性 1.0：getKnockback = (1.0 + 2.0) / 2.0 = 1.5
    item::enchant::EnchantmentRegistry::clear();
    item::enchant::EnchantmentRegistry::initialize();
    Items::initialize();

    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());
    mob.setAttackKnockback(1.0);

    ItemStack sword(Items::DIAMOND_SWORD, 1);
    sword.addEnchantment("minecraft:knockback", 2);
    mob.setMainHandItem(sword);

    // (ATTACK_KNOCKBACK=1.0 + getKnockbackBonus(2)=2.0) / 2.0 = 1.5
    EXPECT_FLOAT_EQ(mob.getKnockback(target), 1.5f);

    item::enchant::EnchantmentRegistry::clear();
}

TEST(GetKnockbackTest, NoEnchantmentNoBonus)
{
    // 无附魔武器：getKnockback = (0 + 0) / 2.0 = 0（仅属性贡献）
    item::enchant::EnchantmentRegistry::clear();
    item::enchant::EnchantmentRegistry::initialize();
    Items::initialize();

    KnockbackTestMob mob(1);
    KnockbackTestEntity target(2, mc::test::testEcsRegistry());

    ItemStack sword(Items::DIAMOND_SWORD, 1); // 无附魔
    mob.setMainHandItem(sword);

    EXPECT_FLOAT_EQ(mob.getKnockback(target), 0.0f);

    item::enchant::EnchantmentRegistry::clear();
}

// ============================================================================
// LivingEntity::applyKnockback 零向量随机扰动测试
// ============================================================================

class ApplyKnockbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity = std::make_unique<KnockbackTestEntity>(1, mc::test::testEcsRegistry());
        entity->setWorld(&world);
        entity->setHealth(20.0f);
    }

    KnockbackTestWorld world;
    std::unique_ptr<KnockbackTestEntity> entity;
};

TEST_F(ApplyKnockbackTest, NormalDirectionAppliesKnockback)
{
    // 正常方向向量应正确施加击退
    entity->setOnGround(true);
    entity->setVelocity(0.0f, 0.0f, 0.0f);

    // ratioX=1.0, ratioZ=0.0 -> 归一化后方向为 (1, 0)
    entity->applyKnockback(1.0f, 1.0, 0.0);

    // 击退后：velocity.x = 0/2 - 1.0*1.0 = -1.0
    // 击退后：velocity.z = 0/2 - 0.0*1.0 = 0.0
    EXPECT_FLOAT_EQ(entity->velocity().x, -1.0f);
    EXPECT_FLOAT_EQ(entity->velocity().z, 0.0f);
    // 在地面时 Y 速度 = min(0.4, 0/2 + 1.0) = 0.4
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.4f);
}

TEST_F(ApplyKnockbackTest, ZeroDirectionAppliesRandomPerturbation)
{
    // 零方向向量时应通过随机扰动应用击退，而不是直接返回
    entity->setOnGround(true);
    entity->setVelocity(0.0f, 0.0f, 0.0f);

    // ratioX=0, ratioZ=0 -> 触发随机扰动路径
    entity->applyKnockback(1.0f, 0.0, 0.0);

    // 由于随机扰动，方向不确定，但击退应该已应用
    // 关键检查：实体不应留在原位，速度应已改变
    bool velocityChanged = entity->velocity().x != 0.0f || entity->velocity().z != 0.0f;
    EXPECT_TRUE(velocityChanged);

    // Y 速度应在地面时被设置
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.4f);
    // 应标记为不在地面
    EXPECT_FALSE(entity->onGround());
}

TEST_F(ApplyKnockbackTest, NearZeroDirectionAppliesRandomPerturbation)
{
    // 极小方向向量（长度平方 < 1.0E-5）也应触发随机扰动
    entity->setOnGround(true);
    entity->setVelocity(0.0f, 0.0f, 0.0f);

    // ratioX=0.001, ratioZ=0.001 -> lengthSquared = 0.000002 < 1.0E-5，触发扰动
    entity->applyKnockback(1.0f, 0.001, 0.001);

    // 击退应已应用
    bool velocityChanged = entity->velocity().x != 0.0f || entity->velocity().z != 0.0f;
    EXPECT_TRUE(velocityChanged);
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.4f);
}

TEST_F(ApplyKnockbackTest, ZeroStrengthNoKnockback)
{
    // 击退强度为 0 时不施加击退
    entity->setOnGround(true);
    entity->setVelocity(0.5f, 0.0f, 0.3f);

    entity->applyKnockback(0.0f, 1.0, 0.0);

    // 速度不变
    EXPECT_FLOAT_EQ(entity->velocity().x, 0.5f);
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.0f);
    EXPECT_FLOAT_EQ(entity->velocity().z, 0.3f);
}

TEST_F(ApplyKnockbackTest, KnockbackResistanceReducesStrength)
{
    // 击退抗性应降低击退强度
    entity->setKnockbackResistance(0.5);
    entity->setOnGround(true);
    entity->setVelocity(0.0f, 0.0f, 0.0f);

    // 50% 击退抗性：effective_strength = 1.0 * (1 - 0.5) = 0.5
    entity->applyKnockback(1.0f, 1.0, 0.0);

    // 击退减半：velocity.x = 0/2 - 1.0*0.5 = -0.5
    EXPECT_FLOAT_EQ(entity->velocity().x, -0.5f);
}

TEST_F(ApplyKnockbackTest, FullKnockbackResistanceNoKnockback)
{
    // 100% 击退抗性应完全免疫击退
    entity->setKnockbackResistance(1.0);
    entity->setOnGround(true);
    entity->setVelocity(0.5f, 0.0f, 0.3f);

    entity->applyKnockback(1.0f, 1.0, 0.0);

    // 速度不变
    EXPECT_FLOAT_EQ(entity->velocity().x, 0.5f);
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.0f);
    EXPECT_FLOAT_EQ(entity->velocity().z, 0.3f);
}

TEST_F(ApplyKnockbackTest, AirborneKeepsYVelocity)
{
    // 空中时 Y 速度保持不变
    entity->setOnGround(false);
    entity->setVelocity(0.0f, 0.5f, 0.0f);

    entity->applyKnockback(1.0f, 1.0, 0.0);

    // Y 速度保持 0.5
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.5f);
}

TEST_F(ApplyKnockbackTest, GroundYVelocityCapped)
{
    // 地面时 Y 速度 = min(0.4, currentY/2 + strength)
    entity->setOnGround(true);
    entity->setVelocity(0.0f, 0.0f, 0.0f);

    // strength = 1.0: Y = min(0.4, 0/2 + 1.0) = min(0.4, 1.0) = 0.4
    entity->applyKnockback(1.0f, 1.0, 0.0);
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.4f);
}

TEST_F(ApplyKnockbackTest, GroundYVelocitySmallStrength)
{
    // 小击退强度时 Y 速度不超过 0.4
    entity->setOnGround(true);
    entity->setVelocity(0.0f, 0.0f, 0.0f);

    // strength = 0.2: Y = min(0.4, 0/2 + 0.2) = min(0.4, 0.2) = 0.2
    entity->applyKnockback(0.2f, 1.0, 0.0);
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.2f);
}

TEST_F(ApplyKnockbackTest, SetsOnGroundFalse)
{
    // 击退后应设置 onGround 为 false
    entity->setOnGround(true);
    entity->applyKnockback(1.0f, 1.0, 0.0);
    EXPECT_FALSE(entity->onGround());
}

TEST_F(ApplyKnockbackTest, MarksHurt)
{
    // 击退后应标记 hurt
    entity->setOnGround(true);
    EXPECT_FALSE(entity->isHurtMarked());

    entity->applyKnockback(1.0f, 1.0, 0.0);

    EXPECT_TRUE(entity->isHurtMarked());
}

TEST_F(ApplyKnockbackTest, NormalDirectionNotPerturbed)
{
    // 正常方向向量（长度平方 >= 1.0E-5）不应被随机扰动
    // 验证方向向量的确定性
    entity->setOnGround(true);
    entity->setVelocity(0.0f, 0.0f, 0.0f);

    // ratioX=1.0, ratioZ=0.0 -> lengthSquared = 1.0 >= 1.0E-5，不触发扰动
    entity->applyKnockback(1.0f, 1.0, 0.0);

    // 方向应确定性：velocity.x = 0/2 - 1.0 = -1.0, velocity.z = 0/2 - 0 = 0
    EXPECT_FLOAT_EQ(entity->velocity().x, -1.0f);
    EXPECT_FLOAT_EQ(entity->velocity().z, 0.0f);
}

TEST_F(ApplyKnockbackTest, ExistingVelocityHalvedBeforeKnockback)
{
    // 击退公式：velocity = currentVelocity/2 - knockbackDirection * strength
    entity->setOnGround(false);
    entity->setVelocity(2.0f, 0.5f, -1.0f);

    // ratioX=1.0, ratioZ=0.0, strength=0.5
    entity->applyKnockback(0.5f, 1.0, 0.0);

    // velocity.x = 2.0/2 - 1.0*0.5 = 1.0 - 0.5 = 0.5
    EXPECT_FLOAT_EQ(entity->velocity().x, 0.5f);
    // velocity.y 保持 0.5（空中）
    EXPECT_FLOAT_EQ(entity->velocity().y, 0.5f);
    // velocity.z = -1.0/2 - 0.0 = -0.5
    EXPECT_FLOAT_EQ(entity->velocity().z, -0.5f);
}

} // namespace
