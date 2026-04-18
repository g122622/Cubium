#include <gtest/gtest.h>

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/AttackContext.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"

using namespace mc;
using namespace mc::entity::attribute;

// ============================================================================
// LivingEntity 测试类
// ============================================================================

class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity() : LivingEntity(LegacyEntityType::Player, 1) {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// ============================================================================
// 生命值测试
// ============================================================================

TEST(LivingEntityTest, Construction) {
    TestLivingEntity entity;

    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
    EXPECT_FLOAT_EQ(entity.maxHealth(), 20.0f);
    EXPECT_FALSE(entity.isDead());
}

TEST(LivingEntityTest, SetHealth) {
    TestLivingEntity entity;

    entity.setHealth(15.0f);
    EXPECT_FLOAT_EQ(entity.health(), 15.0f);

    entity.setHealth(100.0f);  // 超过最大值
    EXPECT_FLOAT_EQ(entity.health(), entity.maxHealth());

    entity.setHealth(-10.0f);  // 低于0
    EXPECT_FLOAT_EQ(entity.health(), 0.0f);
}

TEST(LivingEntityTest, Heal) {
    TestLivingEntity entity;

    entity.setHealth(10.0f);
    entity.heal(5.0f);
    EXPECT_FLOAT_EQ(entity.health(), 15.0f);

    entity.heal(100.0f);  // 超过最大值
    EXPECT_FLOAT_EQ(entity.health(), entity.maxHealth());

    entity.setHealth(0.0f);
    entity.heal(10.0f);  // 死亡实体不应该回血
    EXPECT_FLOAT_EQ(entity.health(), 0.0f);
}

TEST(LivingEntityTest, Hurt) {
    TestLivingEntity entity;

    entity.setHealth(20.0f);
    EnvironmentalDamage damage(DamageType::Generic);

    EXPECT_TRUE(entity.hurt(damage, 5.0f));
    EXPECT_FLOAT_EQ(entity.health(), 15.0f);
}

TEST(LivingEntityTest, HurtInvulnerability) {
    TestLivingEntity entity;

    entity.setHealth(20.0f);
    EnvironmentalDamage damage(DamageType::Generic);

    entity.hurt(damage, 5.0f);
    EXPECT_EQ(entity.hurtTime(), 10);  // 默认10 tick无敌

    // 无敌帧期间不能再受伤
    EXPECT_FALSE(entity.hurt(damage, 5.0f));
}

TEST(LivingEntityTest, Death) {
    TestLivingEntity entity;

    entity.setHealth(5.0f);
    EnvironmentalDamage damage(DamageType::Generic);

    entity.hurt(damage, 10.0f);
    EXPECT_TRUE(entity.isDead());

    // 死亡实体tick会处理死亡动画
    entity.tick();
    EXPECT_TRUE(entity.isDying());
    EXPECT_EQ(entity.deathTime(), 1);
}

TEST(LivingEntityTest, IsDead) {
    TestLivingEntity entity;

    EXPECT_FALSE(entity.isDead());

    entity.setHealth(0.0f);
    EXPECT_TRUE(entity.isDead());

    entity.setHealth(10.0f);
    EXPECT_FALSE(entity.isDead());
}

// ============================================================================
// 属性测试
// ============================================================================

TEST(LivingEntityTest, DefaultAttributes) {
    TestLivingEntity entity;

    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::MAX_HEALTH));
    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::MOVEMENT_SPEED));
    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::KNOCKBACK_RESISTANCE));
    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::ARMOR));
    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::ARMOR_TOUGHNESS));
}

TEST(LivingEntityTest, GetAttributeValue) {
    TestLivingEntity entity;

    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::MAX_HEALTH), 20.0);
    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::MOVEMENT_SPEED), 0.7);
    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::KNOCKBACK_RESISTANCE), 0.0);
    EXPECT_DOUBLE_EQ(entity.getAttributeValue("non-existent", 99.0), 99.0);
}

TEST(LivingEntityTest, SetAttributeBaseValue) {
    TestLivingEntity entity;

    entity.setAttributeBaseValue(Attributes::MAX_HEALTH, 30.0);
    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::MAX_HEALTH), 30.0);
    EXPECT_FLOAT_EQ(entity.maxHealth(), 30.0f);
}

TEST(LivingEntityTest, AttributeModifier) {
    TestLivingEntity entity;

    entity.attributes().addModifier(Attributes::MAX_HEALTH,
        AttributeModifier("health-boost", "Health Boost", 10.0, Operation::Addition));

    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::MAX_HEALTH), 30.0);
    EXPECT_FLOAT_EQ(entity.maxHealth(), 30.0f);
}

// ============================================================================
// 装备测试
// ============================================================================

TEST(LivingEntityTest, EquipmentSlots) {
    TestLivingEntity entity;

    // 测试主手
    ItemStack mainHand;
    entity.setMainHandItem(mainHand);
    EXPECT_TRUE(entity.getMainHandItem().isEmpty());

    // 测试副手
    ItemStack offHand;
    entity.setOffHandItem(offHand);
    EXPECT_TRUE(entity.getOffHandItem().isEmpty());

    // 测试所有槽位
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        EXPECT_TRUE(entity.getEquipment(static_cast<EquipmentSlot>(i)).isEmpty());
    }
}

// ============================================================================
// 受伤无敌帧测试
// ============================================================================

TEST(LivingEntityTest, HurtTime) {
    TestLivingEntity entity;

    EXPECT_EQ(entity.hurtTime(), 0);

    EnvironmentalDamage damage(DamageType::Generic);
    entity.hurt(damage, 5.0f);

    EXPECT_EQ(entity.hurtTime(), 10);  // 默认10 tick
    EXPECT_EQ(entity.maxHurtTime(), 10);
}

TEST(LivingEntityTest, HurtTimeDecreases) {
    TestLivingEntity entity;

    EnvironmentalDamage damage(DamageType::Generic);
    entity.hurt(damage, 5.0f);
    EXPECT_EQ(entity.hurtTime(), 10);

    entity.tick();
    EXPECT_EQ(entity.hurtTime(), 9);

    entity.tick();
    EXPECT_EQ(entity.hurtTime(), 8);
}

// ============================================================================
// 伤害来源测试
// ============================================================================

TEST(DamageSourceTest, EnvironmentalDamage) {
    EnvironmentalDamage fire(DamageType::OnFire);

    EXPECT_EQ(fire.type(), DamageType::OnFire);
    EXPECT_TRUE(fire.isFire());
    EXPECT_FALSE(fire.isProjectile());
    EXPECT_FALSE(fire.isExplosion());
    EXPECT_FALSE(fire.isEntitySource());
    EXPECT_EQ(fire.source(), nullptr);
}

TEST(DamageSourceTest, EntityDamage) {
    EntityDamageSource attack(DamageType::PlayerAttack, nullptr);

    EXPECT_EQ(attack.type(), DamageType::PlayerAttack);
    EXPECT_TRUE(attack.isEntitySource());
    EXPECT_TRUE(attack.isPlayerSource());
    EXPECT_FALSE(attack.isFire());
    EXPECT_FALSE(attack.isProjectile());
}

TEST(DamageSourceTest, IndirectEntityDamage) {
    IndirectEntityDamageSource arrow(DamageType::Arrow, nullptr, nullptr);

    EXPECT_EQ(arrow.type(), DamageType::Arrow);
    EXPECT_TRUE(arrow.isProjectile());
    EXPECT_TRUE(arrow.isEntitySource());
    EXPECT_FALSE(arrow.isFire());
}

TEST(DamageSourceTest, DamageTypes) {
    EnvironmentalDamage fall(DamageType::Fall);
    EXPECT_TRUE(fall.isFall());

    EnvironmentalDamage drown(DamageType::Drown);
    EXPECT_TRUE(drown.isDrown());

    EnvironmentalDamage starve(DamageType::Starve);
    EXPECT_TRUE(starve.isStarve());
}

TEST(DamageSourceTest, BypassesArmor) {
    EnvironmentalDamage fall(DamageType::Fall);
    EXPECT_TRUE(fall.bypassesArmor());

    EnvironmentalDamage generic(DamageType::Generic);
    EXPECT_FALSE(generic.bypassesArmor());

    EnvironmentalDamage outOfWorld(DamageType::OutOfWorld);
    EXPECT_TRUE(outOfWorld.bypassesArmor());
    EXPECT_TRUE(outOfWorld.bypassesInvulnerability());
    EXPECT_TRUE(outOfWorld.canDamageCreative());
}

TEST(DamageSourceTest, DeathMessageKeys) {
    EnvironmentalDamage fire(DamageType::OnFire);
    EXPECT_EQ(fire.deathMessageKey(), "death.attack.onFire");

    EnvironmentalDamage drown(DamageType::Drown);
    EXPECT_EQ(drown.deathMessageKey(), "death.attack.drown");

    EntityDamageSource mob(DamageType::MobAttack, nullptr);
    EXPECT_EQ(mob.deathMessageKey(), "death.attack.mob");
}

TEST(DamageSourceTest, DamageSourcesFactory) {
    auto fire = DamageSources::inFire();
    EXPECT_EQ(fire.type(), DamageType::InFire);
    EXPECT_TRUE(fire.isFire());

    auto fall = DamageSources::fall();
    EXPECT_EQ(fall.type(), DamageType::Fall);
    EXPECT_TRUE(fall.isFall());

    auto arrow = DamageSources::arrow(nullptr, nullptr);
    EXPECT_EQ(arrow.type(), DamageType::Arrow);
    EXPECT_TRUE(arrow.isProjectile());

    auto playerAttack = DamageSources::playerAttack(nullptr);
    EXPECT_EQ(playerAttack.type(), DamageType::PlayerAttack);
    EXPECT_TRUE(playerAttack.isPlayerSource());
}

TEST(AttackContextTest, MeleeDamageAppliesStrengthAndWeakness) {
    TestLivingEntity attacker;
    TestLivingEntity target;

    attacker.addEffect(mc::entity::effect::EffectInstance(
        mc::entity::effect::EffectType::Strength,
        200,
        0,
        false,
        true,
        true
    ));

    attacker.addEffect(mc::entity::effect::EffectInstance(
        mc::entity::effect::EffectType::Weakness,
        200,
        0,
        false,
        true,
        true
    ));

    mc::entity::combat::AttackContext context(&attacker, &target);
    context.setBaseDamage(5.0f);
    context.setAttackType(mc::entity::combat::AttackType::Melee);

    EXPECT_FLOAT_EQ(context.calculateFinalDamage(), 4.0f);
}

TEST(AttackContextTest, MeleeDamageUsesArmorToughnessFormula) {
    TestLivingEntity attacker;
    TestLivingEntity target;

    target.setAttributeBaseValue(Attributes::ARMOR, 20.0);
    target.setAttributeBaseValue(Attributes::ARMOR_TOUGHNESS, 8.0);

    mc::entity::combat::AttackContext context(&attacker, &target);
    context.setBaseDamage(10.0f);
    context.setAttackType(mc::entity::combat::AttackType::Melee);

    EXPECT_FLOAT_EQ(context.calculateFinalDamage(), 3.0f);
}
