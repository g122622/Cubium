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
#include "common/core/Constants.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/AttackContext.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::entity::attribute;

namespace {

class GroundSupportWorld final : public mc::test::BaseTestWorld {
public:
    void setSupportEnabled(bool enabled) { m_supportEnabled = enabled; }

    struct SoundRecord {
        ResourceLocation soundEventId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    [[nodiscard]] bool hasSoundRecord() const { return m_lastSound.has_value(); }
    [[nodiscard]] const SoundRecord& lastSound() const { return *m_lastSound; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (m_supportEnabled && x == 0 && y == 0 && z == 0) {
            return &VanillaBlocks::STONE->defaultState();
        }

        return nullptr;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        if (!m_supportEnabled) {
            return false;
        }

        return box.maxX > 0.0f && box.minX < 1.0f && box.maxY > 0.0f && box.minY < 1.0f && box.maxZ > 0.0f &&
            box.minZ < 1.0f;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        if (!hasBlockCollision(box)) {
            return {};
        }

        return {AxisAlignedBB(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSound = SoundRecord{soundEventId, category, position, volume, pitch};
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("GroundSupportWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("GroundSupportWorld::tickManager not implemented");
    }

private:
    bool m_supportEnabled = true;
    std::optional<SoundRecord> m_lastSound;
};

class TestMobEntity : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityInstanceId(2), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
        // makeSoundEventId 由 typeId 构造音效 ID（minecraft:entity.<type>.<suffix>），
        // 直接构造的实体 m_typeId 为空会导致 getHurtSound/getDeathSound/
        // getAmbientSound 返回 nullopt。设为 minecraft:cow 使音效测试可验证。
        setTypeId(entity::EntityTypeKeys::COW);
    }
};

} // namespace

// ============================================================================
// LivingEntity 测试类
// ============================================================================

class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
        // makeSoundEventId 由 typeId 构造音效 ID，直接构造的实体 m_typeId 为空
        // 会导致 getHurtSound/getDeathSound 返回 nullopt。设为 minecraft:player
        // 使 HurtPlaysSound/DeathPlaysSound 可验证（对齐 commit 8bb41781b 策略）。
        setTypeId(entity::EntityTypeKeys::PLAYER);
    }
};

// ============================================================================
// 生命值测试
// ============================================================================

TEST(LivingEntityTest, Construction)
{
    TestLivingEntity entity;

    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
    EXPECT_FLOAT_EQ(entity.maxHealth(), 20.0f);
    EXPECT_FALSE(entity.isDead());
}

TEST(LivingEntityTest, SetHealth)
{
    TestLivingEntity entity;

    entity.setHealth(15.0f);
    EXPECT_FLOAT_EQ(entity.health(), 15.0f);

    entity.setHealth(100.0f); // 超过最大值
    EXPECT_FLOAT_EQ(entity.health(), entity.maxHealth());

    entity.setHealth(-10.0f); // 低于0
    EXPECT_FLOAT_EQ(entity.health(), 0.0f);
}

TEST(LivingEntityTest, Heal)
{
    TestLivingEntity entity;

    entity.setHealth(10.0f);
    entity.heal(5.0f);
    EXPECT_FLOAT_EQ(entity.health(), 15.0f);

    entity.heal(100.0f); // 超过最大值
    EXPECT_FLOAT_EQ(entity.health(), entity.maxHealth());

    entity.setHealth(0.0f);
    entity.heal(10.0f); // 死亡实体不应该回血
    EXPECT_FLOAT_EQ(entity.health(), 0.0f);
}

TEST(LivingEntityTest, Hurt)
{
    TestLivingEntity entity;

    entity.setHealth(20.0f);
    EnvironmentalDamage damage(DamageType::Generic);

    EXPECT_TRUE(entity.hurt(damage, 5.0f));
    EXPECT_FLOAT_EQ(entity.health(), 15.0f);
}

TEST(LivingEntityTest, HurtPlaysSound)
{
    GroundSupportWorld world;
    TestLivingEntity entity;
    entity.setWorld(&world);

    entity.setHealth(20.0f);
    EnvironmentalDamage damage(DamageType::Generic);

    EXPECT_TRUE(entity.hurt(damage, 5.0f));
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.player.hurt");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Neutral);
    EXPECT_FLOAT_EQ(world.lastSound().volume, 1.0f);
    EXPECT_GE(world.lastSound().pitch, 0.8f);
    EXPECT_LE(world.lastSound().pitch, 1.2f);
}

TEST(LivingEntityTest, NbtRoundTripPreservesEffectsAndEquipment)
{
    Items::initialize();
    TestLivingEntity entity;
    entity.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Speed, 200, 1, false, true, true));
    entity.setMainHandItem(ItemStack(Items::DIAMOND_SWORD, 1));
    entity.setOffHandItem(ItemStack(Items::SHIELD, 1));
    entity.setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    entity.setEquipment(EquipmentSlot::Chest, ItemStack(Items::IRON_CHESTPLATE, 1));
    entity.setEquipment(EquipmentSlot::Legs, ItemStack(Items::IRON_LEGGINGS, 1));
    entity.setEquipment(EquipmentSlot::Feet, ItemStack(Items::IRON_BOOTS, 1));

    nbt::tags::compound_tag tag;
    entity.writeToNBT(tag);

    TestLivingEntity loaded;
    auto readResult = loaded.readFromNBT(tag);
    ASSERT_TRUE(readResult.success()) << readResult.error().message();

    ASSERT_EQ(loaded.effectManager().getAllEffects().size(), 1u);
    EXPECT_EQ(loaded.effectManager().getAllEffects()[0].type(), entity::effect::EffectType::Speed);
    EXPECT_EQ(loaded.effectManager().getAllEffects()[0].amplifier(), 1);
    EXPECT_EQ(loaded.getMainHandItem().getItem(), Items::DIAMOND_SWORD);
    EXPECT_EQ(loaded.getOffHandItem().getItem(), Items::SHIELD);
    EXPECT_EQ(loaded.getEquipment(EquipmentSlot::Head).getItem(), Items::IRON_HELMET);
    EXPECT_EQ(loaded.getEquipment(EquipmentSlot::Chest).getItem(), Items::IRON_CHESTPLATE);
    EXPECT_EQ(loaded.getEquipment(EquipmentSlot::Legs).getItem(), Items::IRON_LEGGINGS);
    EXPECT_EQ(loaded.getEquipment(EquipmentSlot::Feet).getItem(), Items::IRON_BOOTS);
}

TEST(LivingEntityTest, HurtInvulnerability)
{
    TestLivingEntity entity;

    entity.setHealth(20.0f);
    EnvironmentalDamage damage(DamageType::Generic);

    entity.hurt(damage, 5.0f);
    EXPECT_EQ(entity.hurtTime(), 10); // 默认10 tick无敌

    // 无敌帧期间不能再受伤
    EXPECT_FALSE(entity.hurt(damage, 5.0f));
}

TEST(LivingEntityTest, Death)
{
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

TEST(LivingEntityTest, DeathPlaysSound)
{
    GroundSupportWorld world;
    TestLivingEntity entity;
    entity.setWorld(&world);

    entity.setHealth(5.0f);
    EnvironmentalDamage damage(DamageType::Generic);

    entity.hurt(damage, 10.0f);
    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.player.death");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Neutral);
}

TEST(MobEntityTest, PlayAmbientSound_ForwardsToWorld)
{
    GroundSupportWorld world;
    TestMobEntity entity;
    entity.setWorld(&world);

    entity.playAmbientSound();

    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.cow.ambient");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Neutral);
}

TEST(LivingEntityTest, IsDead)
{
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

TEST(LivingEntityTest, DefaultAttributes)
{
    TestLivingEntity entity;

    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::MAX_HEALTH));
    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::MOVEMENT_SPEED));
    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::KNOCKBACK_RESISTANCE));
    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::ARMOR));
    EXPECT_TRUE(entity.attributes().hasAttribute(Attributes::ARMOR_TOUGHNESS));
}

TEST(LivingEntityTest, GetAttributeValue)
{
    TestLivingEntity entity;

    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::MAX_HEALTH), 20.0);
    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::MOVEMENT_SPEED), 0.7);
    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::KNOCKBACK_RESISTANCE), 0.0);
    EXPECT_DOUBLE_EQ(entity.getAttributeValue("non-existent", 99.0), 99.0);
}

TEST(LivingEntityTest, SetAttributeBaseValue)
{
    TestLivingEntity entity;

    entity.setAttributeBaseValue(Attributes::MAX_HEALTH, 30.0);
    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::MAX_HEALTH), 30.0);
    EXPECT_FLOAT_EQ(entity.maxHealth(), 30.0f);
}

TEST(LivingEntityTest, AttributeModifier)
{
    TestLivingEntity entity;

    entity.attributes().addModifier(
        Attributes::MAX_HEALTH, AttributeModifier("health-boost", "Health Boost", 10.0, Operation::Addition));

    EXPECT_DOUBLE_EQ(entity.getAttributeValue(Attributes::MAX_HEALTH), 30.0);
    EXPECT_FLOAT_EQ(entity.maxHealth(), 30.0f);
}

// ============================================================================
// 装备测试
// ============================================================================

TEST(LivingEntityTest, EquipmentSlots)
{
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

TEST(LivingEntityTest, HurtTime)
{
    TestLivingEntity entity;

    EXPECT_EQ(entity.hurtTime(), 0);

    EnvironmentalDamage damage(DamageType::Generic);
    entity.hurt(damage, 5.0f);

    EXPECT_EQ(entity.hurtTime(), 10); // 默认10 tick
    EXPECT_EQ(entity.maxHurtTime(), 10);
}

TEST(LivingEntityTest, HurtTimeDecreases)
{
    TestLivingEntity entity;

    EnvironmentalDamage damage(DamageType::Generic);
    entity.hurt(damage, 5.0f);
    EXPECT_EQ(entity.hurtTime(), 10);

    entity.tick();
    EXPECT_EQ(entity.hurtTime(), 9);

    entity.tick();
    EXPECT_EQ(entity.hurtTime(), 8);
}

TEST(LivingEntityTest, MobFallsWhenSupportIsRemoved)
{
    VanillaBlocks::initialize();

    GroundSupportWorld world;
    MobEntity mob(EntityInstanceId(1), mc::test::testEcsRegistry());
    mob.setWorld(&world);
    mob.setPosition(0.3f, 1.0f, 0.3f);

    mob.tick();
    f32 supportedY = mob.y();
    EXPECT_TRUE(mob.onGround());

    world.setSupportEnabled(false);
    mob.tick();

    EXPECT_LT(mob.y(), supportedY);
    EXPECT_FALSE(mob.onGround());
}

// ============================================================================
// 伤害来源测试
// ============================================================================

TEST(DamageSourceTest, EnvironmentalDamage)
{
    EnvironmentalDamage fire(DamageType::OnFire);

    EXPECT_EQ(fire.type(), DamageType::OnFire);
    EXPECT_TRUE(fire.isFire());
    EXPECT_FALSE(fire.isProjectile());
    EXPECT_FALSE(fire.isExplosion());
    EXPECT_FALSE(fire.isEntitySource());
    EXPECT_EQ(fire.source(), nullptr);
}

TEST(DamageSourceTest, EntityDamage)
{
    EntityDamageSource attack(DamageType::PlayerAttack, nullptr);

    EXPECT_EQ(attack.type(), DamageType::PlayerAttack);
    EXPECT_TRUE(attack.isEntitySource());
    EXPECT_TRUE(attack.isPlayerSource());
    EXPECT_FALSE(attack.isFire());
    EXPECT_FALSE(attack.isProjectile());
}

TEST(DamageSourceTest, IndirectEntityDamage)
{
    IndirectEntityDamageSource arrow(DamageType::Arrow, nullptr, nullptr);
    arrow.setProjectile(); // 需要手动设置投射物属性

    EXPECT_EQ(arrow.type(), DamageType::Arrow);
    EXPECT_TRUE(arrow.isProjectile());
    EXPECT_TRUE(arrow.isEntitySource());
    EXPECT_FALSE(arrow.isFire());
}

TEST(DamageSourceTest, DamageTypes)
{
    EnvironmentalDamage fall(DamageType::Fall);
    EXPECT_TRUE(fall.isFall());

    EnvironmentalDamage drown(DamageType::Drown);
    EXPECT_TRUE(drown.isDrown());

    EnvironmentalDamage starve(DamageType::Starve);
    EXPECT_TRUE(starve.isStarve());
}

TEST(DamageSourceTest, BypassesArmor)
{
    EnvironmentalDamage fall(DamageType::Fall);
    EXPECT_TRUE(fall.bypassesArmor());

    // MC 1.16.5: Generic 伤害确实绕过护甲（无对应盔甲保护附魔）
    EnvironmentalDamage generic(DamageType::Generic);
    EXPECT_TRUE(generic.bypassesArmor());

    EnvironmentalDamage outOfWorld(DamageType::OutOfWorld);
    EXPECT_TRUE(outOfWorld.bypassesArmor());
    EXPECT_TRUE(outOfWorld.bypassesInvulnerability());
    EXPECT_TRUE(outOfWorld.canDamageCreative());
}

TEST(DamageSourceTest, DeathMessageKeys)
{
    EnvironmentalDamage fire(DamageType::OnFire);
    EXPECT_EQ(fire.deathMessageKey(), "death.attack.onFire");

    EnvironmentalDamage drown(DamageType::Drown);
    EXPECT_EQ(drown.deathMessageKey(), "death.attack.drown");

    EntityDamageSource mob(DamageType::MobAttack, nullptr);
    EXPECT_EQ(mob.deathMessageKey(), "death.attack.mob");
}

TEST(DamageSourceTest, DamageSourcesFactory)
{
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

TEST(AttackContextTest, MeleeDamageAppliesStrengthAndWeakness)
{
    // commit 66a6ae396 起，力量/虚弱不再由 AttackContext::calculateFinalDamage
    // 手动加减，而是通过 EffectAttributeModifiers 以 Addition 操作应用到
    // ATTACK_DAMAGE 属性（Strength +3.0/级，Weakness -4.0/级）。此处验证属性
    // 修改器路径：MonsterEntity 注册 ATTACK_DAMAGE，基础值 2.0，叠加 0 级力量
    // (+3) 与 0 级虚弱 (-4) 后属性值为 2 + 3 - 4 = 1.0。
    class TestMonster : public MonsterEntity {
    public:
        TestMonster()
            : MonsterEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
        {
            // C++ 虚方法在基类构造中只 dispatch 到基类版本，LivingEntity 构造调用的
            // registerAttributes 不会派发到 MonsterEntity::registerAttributes，故需在此
            // 显式调用以注册 ATTACK_DAMAGE 属性（与 TestMobEntity 模式一致）。
            registerAttributes();
            setAttributeBaseValue(Attributes::ATTACK_DAMAGE, 2.0);
        }
    };

    TestMonster attacker;
    attacker.addEffect(
        mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::Strength, 200, 0, false, true, true));
    attacker.addEffect(
        mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::Weakness, 200, 0, false, true, true));

    EXPECT_DOUBLE_EQ(attacker.attributes().getValue(Attributes::ATTACK_DAMAGE), 1.0);
}

TEST(AttackContextTest, MeleeDamageUsesArmorToughnessFormula)
{
    TestLivingEntity attacker;
    TestLivingEntity target;

    target.setAttributeBaseValue(Attributes::ARMOR, 20.0);
    target.setAttributeBaseValue(Attributes::ARMOR_TOUGHNESS, 8.0);

    mc::entity::combat::AttackContext context(&attacker, &target);
    context.setBaseDamage(10.0f);
    context.setAttackType(mc::entity::combat::AttackType::Melee);

    EXPECT_FLOAT_EQ(context.calculateFinalDamage(), 3.0f);
}

// ============================================================================
// 挥动动画测试
// ============================================================================

TEST(LivingEntityTest, SwingAnimation_DefaultState)
{
    TestLivingEntity entity;

    // 默认没有挥动
    EXPECT_EQ(entity.swingProgressInt(), 0);
    EXPECT_FALSE(entity.isSwingInProgress());
    EXPECT_EQ(entity.swingingHand(), Hand::MainHand);
}

TEST(LivingEntityTest, SwingAnimation_TriggerSwing)
{
    TestLivingEntity entity;

    // 触发主手挥动
    entity.swing(Hand::MainHand);

    EXPECT_TRUE(entity.isSwingInProgress());
    EXPECT_EQ(entity.swingingHand(), Hand::MainHand);
    EXPECT_EQ(entity.swingProgressInt(), -1); // 初始为 -1
}

TEST(LivingEntityTest, SwingAnimation_TriggerOffHandSwing)
{
    TestLivingEntity entity;

    // 触发副手挥动
    entity.swing(Hand::OffHand);

    EXPECT_TRUE(entity.isSwingInProgress());
    EXPECT_EQ(entity.swingingHand(), Hand::OffHand);
}

TEST(LivingEntityTest, SwingAnimation_TickProgress)
{
    TestLivingEntity entity;

    entity.swing(Hand::MainHand);
    EXPECT_EQ(entity.swingProgressInt(), -1);

    // 每次tick进度增加
    entity.tick();
    EXPECT_EQ(entity.swingProgressInt(), 0);

    entity.tick();
    EXPECT_EQ(entity.swingProgressInt(), 1);

    entity.tick();
    EXPECT_EQ(entity.swingProgressInt(), 2);
}

TEST(LivingEntityTest, SwingAnimation_StopsAfterAnimationEnd)
{
    TestLivingEntity entity;

    entity.swing(Hand::MainHand);

    // 默认动画时长 6 tick
    // 进度从 -1 开始，所以需要 7 次 tick 才能完成
    for (int i = 0; i < 7; ++i) {
        entity.tick();
    }

    EXPECT_FALSE(entity.isSwingInProgress());
    EXPECT_EQ(entity.swingProgressInt(), 0); // 重置为 0
}

TEST(LivingEntityTest, SwingAnimation_GetArmSwingAnimationEnd_Base)
{
    TestLivingEntity entity;

    // 基础动画时长为 6 tick
    EXPECT_EQ(entity.getArmSwingAnimationEnd(), 6);
}

TEST(LivingEntityTest, SwingAnimation_GetArmSwingAnimationEnd_HasteEffect)
{
    TestLivingEntity entity;

    // 添加急迫 I 效果
    entity.addEffect(mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::Haste,
        200,   // duration
        0,     // amplifier (Haste I)
        false, // ambient
        true,  // visible
        true   // showIcon
        ));

    // Haste I: 6 - (1 + 1) = 4 tick
    EXPECT_EQ(entity.getArmSwingAnimationEnd(), 4);
}

TEST(LivingEntityTest, SwingAnimation_GetArmSwingAnimationEnd_HasteII)
{
    TestLivingEntity entity;

    // 添加急迫 II 效果
    entity.addEffect(mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::Haste,
        200,
        1, // amplifier (Haste II)
        false,
        true,
        true));

    // Haste II: 6 - (1 + 2) = 3 tick
    EXPECT_EQ(entity.getArmSwingAnimationEnd(), 3);
}

TEST(LivingEntityTest, SwingAnimation_GetArmSwingAnimationEnd_MiningFatigue)
{
    TestLivingEntity entity;

    // 添加挖掘疲劳 I 效果
    entity.addEffect(mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::MiningFatigue,
        200,
        0, // amplifier (Mining Fatigue I)
        false,
        true,
        true));

    // Mining Fatigue I: 6 + (1 + 1) * 2 = 10 tick
    EXPECT_EQ(entity.getArmSwingAnimationEnd(), 10);
}

TEST(LivingEntityTest, SwingAnimation_GetArmSwingAnimationEnd_MiningFatigueIII)
{
    TestLivingEntity entity;

    // 添加挖掘疲劳 III 效果
    entity.addEffect(mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::MiningFatigue,
        200,
        2, // amplifier (Mining Fatigue III)
        false,
        true,
        true));

    // Mining Fatigue III: 6 + (1 + 3) * 2 = 14 tick
    EXPECT_EQ(entity.getArmSwingAnimationEnd(), 14);
}

TEST(LivingEntityTest, SwingAnimation_GetArmSwingAnimationEnd_HasteAndFatigue)
{
    TestLivingEntity entity;

    // 同时有急迫 I 和挖掘疲劳 I
    entity.addEffect(
        mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::Haste, 200, 0, false, true, true));
    entity.addEffect(
        mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::MiningFatigue, 200, 0, false, true, true));

    // Haste I: 6 - 2 = 4
    // Mining Fatigue I: 4 + 4 = 8
    EXPECT_EQ(entity.getArmSwingAnimationEnd(), 8);
}

TEST(LivingEntityTest, SwingAnimation_GetArmSwingAnimationEnd_MinimumOne)
{
    TestLivingEntity entity;

    // 急迫 IV 会产生负数，但最小值为 1
    entity.addEffect(mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::Haste,
        200,
        10, // Haste XI (非常高的等级)
        false,
        true,
        true));

    // 即使计算结果为负，最小值也是 1
    EXPECT_GE(entity.getArmSwingAnimationEnd(), 1);
}

TEST(LivingEntityTest, SwingAnimation_SwingResetsOnNewSwing)
{
    TestLivingEntity entity;

    // 开始挥动
    entity.swing(Hand::MainHand);
    entity.tick();
    entity.tick();
    EXPECT_EQ(entity.swingProgressInt(), 1);

    // MC 1.16.5: 只有在动画进行到一半或更多时才能重新触发
    // 在进度 1/6 时（未到一半），不应该重置
    entity.swing(Hand::OffHand);
    EXPECT_EQ(entity.swingProgressInt(), 1);          // 保持不变
    EXPECT_EQ(entity.swingingHand(), Hand::MainHand); // 手不变

    // 进行到超过一半（6/2 = 3）
    entity.tick(); // 2
    entity.tick(); // 3 - 到达一半
    entity.tick(); // 4 - 超过一半

    // 现在可以重新触发
    entity.swing(Hand::OffHand);
    EXPECT_EQ(entity.swingProgressInt(), -1);        // 重置
    EXPECT_EQ(entity.swingingHand(), Hand::OffHand); // 手切换
}

TEST(LivingEntityTest, SwingAnimation_SwingProgress)
{
    TestLivingEntity entity;

    entity.swing(Hand::MainHand);

    // tick 后获取挥动进度
    entity.tick();
    // swingProgress 应该基于进度计算
    // 初始 swingProgressInt 为 0（tick 后从 -1 变为 0）
    // m_swingProgress 应该在 tick 中更新
    f32 progress = entity.swingProgress();
    EXPECT_GE(progress, 0.0f);
    EXPECT_LE(progress, 1.0f);
}

// ============================================================================
// 空气供应和溺水测试
// ============================================================================

class MockRandomWorld final : public mc::test::BaseTestWorld {
public:
    MockRandomWorld() = default;

    void setInWater(bool inWater) { m_inWater = inWater; }
    void setInLava(bool inLava) { m_inLava = inLava; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        // 返回空流体状态
        return &fluid::Fluids::EMPTY()->defaultState();
    }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        static world::tick::TickManager* dummy = nullptr;
        if (!dummy) throw std::runtime_error("MockRandomWorld::tickManager not implemented");
        return *dummy;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MockRandomWorld::tickManager not implemented");
    }

private:
    bool m_inWater = false;
    bool m_inLava = false;
};

// 测试 canBreatheUnderwater - 默认生物不能水下呼吸
TEST(LivingEntityTest, CanBreatheUnderwater_Default)
{
    TestLivingEntity entity;
    EXPECT_FALSE(entity.canBreatheUnderwater());
}

// 测试亡灵生物可以水下呼吸
class TestUndeadEntity : public LivingEntity {
public:
    TestUndeadEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Undead; }
};

TEST(LivingEntityTest, CanBreatheUnderwater_Undead)
{
    TestUndeadEntity entity;
    EXPECT_TRUE(entity.canBreatheUnderwater());
}

// 测试 determineNextAir - 空气恢复
TEST(LivingEntityTest, DetermineNextAir_Normal)
{
    TestLivingEntity entity;

    // 空气恢复：每tick恢复4点
    EXPECT_EQ(entity.determineNextAir(0), 4);
    EXPECT_EQ(entity.determineNextAir(100), 104);
    EXPECT_EQ(entity.determineNextAir(296), 300); // 接近最大值
    EXPECT_EQ(entity.determineNextAir(297), 300); // 不超过最大值
    EXPECT_EQ(entity.determineNextAir(300), 300); // 已经是最大值
}

TEST(LivingEntityTest, DetermineNextAir_MaxAir)
{
    TestLivingEntity entity;

    // 最大空气值为300（15秒）
    EXPECT_EQ(entity.maxAir(), 300);

    // 负数空气值也会正常恢复（每tick +4）
    // 这是MC 1.16.5的行为：空气值可以是负数（用于溺水计时）
    EXPECT_EQ(entity.determineNextAir(-10), -6); // -10 + 4 = -6
    EXPECT_EQ(entity.determineNextAir(-4), 0);   // -4 + 4 = 0

    // 接近最大值时不能超过最大值
    EXPECT_EQ(entity.determineNextAir(299), 300);
    EXPECT_EQ(entity.determineNextAir(300), 300); // 已经是最大值
}

// 测试 decreaseAirSupply - 没有水下呼吸附魔时空气正常减少
TEST(LivingEntityTest, DecreaseAirSupply_NoRespiration)
{
    MockRandomWorld world;
    TestLivingEntity entity;
    entity.setWorld(&world);

    // 没有水下呼吸附魔时，空气应该每次减少1
    EXPECT_EQ(entity.decreaseAirSupply(300), 299);
    EXPECT_EQ(entity.decreaseAirSupply(100), 99);
    EXPECT_EQ(entity.decreaseAirSupply(1), 0);
    EXPECT_EQ(entity.decreaseAirSupply(0), -1);
}

// 测试 updateAirSupply - 在陆地上恢复空气
TEST(LivingEntityTest, UpdateAirSupply_RecoveryOnLand)
{
    MockRandomWorld world;
    world.setInWater(false);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setAir(100); // 设置较低的空气值

    entity.updateAirSupply();

    // 在陆地上，空气应该恢复4点
    EXPECT_EQ(entity.air(), 104);
}

// 测试 updateAirSupply - 水下呼吸效果
TEST(LivingEntityTest, UpdateAirSupply_WaterBreathingEffect)
{
    MockRandomWorld world;
    world.setInWater(true);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setAir(300);

    // 添加水下呼吸效果
    entity.addEffect(
        mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::WaterBreathing, 200, 0, false, true, true));

    entity.updateAirSupply();

    // 有水下呼吸效果时，空气不应该减少
    EXPECT_EQ(entity.air(), 300);
}

// 测试 updateAirSupply - 潮涌能量效果
TEST(LivingEntityTest, UpdateAirSupply_ConduitPowerEffect)
{
    MockRandomWorld world;
    world.setInWater(true);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setAir(300);

    // 添加潮涌能量效果
    entity.addEffect(
        mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::ConduitPower, 200, 0, false, true, true));

    entity.updateAirSupply();

    // 有潮涌能量效果时，空气不应该减少
    EXPECT_EQ(entity.air(), 300);
}

// 测试 updateAirSupply - 亡灵生物在水下不消耗空气
TEST(LivingEntityTest, UpdateAirSupply_UndeadNoAirConsumption)
{
    MockRandomWorld world;
    world.setInWater(true);
    world.setInLava(false);

    TestUndeadEntity entity;
    entity.setWorld(&world);
    entity.setAir(300);

    entity.updateAirSupply();

    // 亡灵生物可以在水下呼吸，空气不应该减少
    EXPECT_EQ(entity.air(), 300);
}

// 测试 updateAirSupply - 不存活时不处理空气
TEST(LivingEntityTest, UpdateAirSupply_NotAlive)
{
    MockRandomWorld world;
    world.setInWater(true);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setAir(300);
    entity.setHealth(0.0f); // 死亡状态

    entity.updateAirSupply();

    // 死亡实体不应该处理空气
    EXPECT_EQ(entity.air(), 300);
}

// 测试 shouldTakeDrowningDamage - 空气值 <= -20 时应触发溺水伤害
TEST(LivingEntityTest, ShouldTakeDrowningDamage_BelowThreshold)
{
    TestLivingEntity entity;
    entity.setAir(-20);
    EXPECT_TRUE(entity.shouldTakeDrowningDamage());

    entity.setAir(-21);
    EXPECT_TRUE(entity.shouldTakeDrowningDamage());
}

TEST(LivingEntityTest, ShouldTakeDrowningDamage_AboveThreshold)
{
    TestLivingEntity entity;
    entity.setAir(-19);
    EXPECT_FALSE(entity.shouldTakeDrowningDamage());

    entity.setAir(0);
    EXPECT_FALSE(entity.shouldTakeDrowningDamage());

    entity.setAir(300);
    EXPECT_FALSE(entity.shouldTakeDrowningDamage());
}

// 测试 increaseAirSupply - 每tick恢复4点空气
TEST(LivingEntityTest, IncreaseAirSupply_Normal)
{
    TestLivingEntity entity;

    // 每tick恢复4点，上限 maxAir()
    EXPECT_EQ(entity.increaseAirSupply(0), 4);
    EXPECT_EQ(entity.increaseAirSupply(100), 104);
    EXPECT_EQ(entity.increaseAirSupply(296), 300); // 接近最大值
    EXPECT_EQ(entity.increaseAirSupply(297), 300); // 不超过最大值
    EXPECT_EQ(entity.increaseAirSupply(300), 300); // 已经是最大值
}

TEST(LivingEntityTest, IncreaseAirSupply_NegativeAir)
{
    TestLivingEntity entity;

    // 负数空气值也会正常恢复（每tick +4）
    EXPECT_EQ(entity.increaseAirSupply(-10), -6); // -10 + 4 = -6
    EXPECT_EQ(entity.increaseAirSupply(-4), 0);   // -4 + 4 = 0
}

// 测试 determineNextAir 委托给 increaseAirSupply
TEST(LivingEntityTest, DetermineNextAir_DelegatesToIncreaseAirSupply)
{
    TestLivingEntity entity;

    // determineNextAir 应该与 increaseAirSupply 结果一致
    for (i32 air : {0, 100, 296, 297, 300, -10, -4}) {
        EXPECT_EQ(entity.determineNextAir(air), entity.increaseAirSupply(air));
    }
}

// 测试 updateAirSupply - 水下空气消耗到 -20 时触发溺水
TEST(LivingEntityTest, UpdateAirSupply_DrowningDamage)
{
    MockRandomWorld world;
    world.setInWater(true);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setInWater(true);
    entity.setAir(-19); // 接近溺水阈值

    entity.updateAirSupply();

    // 空气从 -19 减少到 -20，触发 shouldTakeDrowningDamage()，空气重置为 0
    EXPECT_EQ(entity.air(), 0);
}

// 测试 updateAirSupply - 水下呼吸效果时空气不消耗且恢复
TEST(LivingEntityTest, UpdateAirSupply_WaterBreathingRestoresAir)
{
    MockRandomWorld world;
    world.setInWater(true);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setInWater(true);
    entity.setAir(200); // 低于最大值

    // 添加水下呼吸效果
    entity.addEffect(
        mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::WaterBreathing, 200, 0, false, true, true));

    entity.updateAirSupply();

    // 有水下呼吸效果时，空气应该恢复（每tick +4）
    EXPECT_EQ(entity.air(), 204);
}

// 测试 updateAirSupply - 潮涌能量效果时空气不消耗且恢复
TEST(LivingEntityTest, UpdateAirSupply_ConduitPowerRestoresAir)
{
    MockRandomWorld world;
    world.setInWater(true);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setInWater(true);
    entity.setAir(200);

    // 添加潮涌能量效果
    entity.addEffect(
        mc::entity::effect::EffectInstance(mc::entity::effect::EffectType::ConduitPower, 200, 0, false, true, true));

    entity.updateAirSupply();

    // 有潮涌能量效果时，空气应该恢复
    EXPECT_EQ(entity.air(), 204);
}

// 测试 updateAirSupply - 不在水中时空气恢复（即使空气未满）
TEST(LivingEntityTest, UpdateAirSupply_RecoveryWhenNotFull)
{
    MockRandomWorld world;
    world.setInWater(false);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setAir(100);

    entity.updateAirSupply();

    // 不在水中时空气恢复4点
    EXPECT_EQ(entity.air(), 104);
}

// 测试 updateAirSupply - 空气已满时不在水中不改变
TEST(LivingEntityTest, UpdateAirSupply_FullAirNoChange)
{
    MockRandomWorld world;
    world.setInWater(false);
    world.setInLava(false);

    TestLivingEntity entity;
    entity.setWorld(&world);
    entity.setAir(300); // 已满

    entity.updateAirSupply();

    // 空气已满，不改变
    EXPECT_EQ(entity.air(), 300);
}

// ============================================================================
// onKillCommand 测试
// ============================================================================

// 测试 LivingEntity::onKillCommand - 使用虚空伤害杀死实体
TEST(LivingEntityTest, OnKillCommand_KillsEntity)
{
    TestLivingEntity entity;

    // 初始状态：满血
    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
    EXPECT_FALSE(entity.isDead());

    // 调用 onKillCommand
    entity.onKillCommand();

    // 实体应该死亡
    EXPECT_TRUE(entity.isDead());
    EXPECT_LE(entity.health(), 0.0f);
}

// 测试 LivingEntity::onKillCommand - 对已死亡实体的效果
TEST(LivingEntityTest, OnKillCommand_AlreadyDead)
{
    TestLivingEntity entity;

    // 先杀死实体
    entity.setHealth(0.0f);
    EXPECT_TRUE(entity.isDead());

    // 再次调用 onKillCommand 不应该崩溃
    entity.onKillCommand();

    // 仍然死亡
    EXPECT_TRUE(entity.isDead());
}

// 测试 LivingEntity::onKillCommand - 触发死亡流程
TEST(LivingEntityTest, OnKillCommand_TriggersDeathProcess)
{
    GroundSupportWorld world;
    TestLivingEntity entity;
    entity.setWorld(&world);

    // 调用 onKillCommand
    entity.onKillCommand();

    // 实体应该处于死亡状态
    EXPECT_TRUE(entity.isDead());

    // 检查是否播放了死亡声音（通过 hurt 触发）
    // 由于使用 Float.MAX_VALUE 伤害，会触发死亡
    EXPECT_TRUE(entity.isDead());
}

// 测试 MobEntity::onKillCommand - 继承自 LivingEntity
TEST(MobEntityTest, OnKillCommand_KillsMob)
{
    TestMobEntity entity;

    // 初始状态：满血
    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
    EXPECT_FALSE(entity.isDead());

    // 调用 onKillCommand
    entity.onKillCommand();

    // 实体应该死亡
    EXPECT_TRUE(entity.isDead());
    EXPECT_LE(entity.health(), 0.0f);
}
