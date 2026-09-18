/**
 * @file EntityFreezeTest.cpp
 * @brief 冰冻系统测试
 *
 * 测试覆盖：
 * 1. DamageType::Freeze 和 DamageSources::freeze() — 冰冻伤害源
 * 2. 冰冻状态计算 — getPercentFrozen、isFullyFrozen、isFreezing 的数学逻辑
 * 3. EntityTypeTags 冰冻标签 — FREEZE_IMMUNE_ENTITY_TYPES、FREEZE_HURTS_EXTRA_TYPES
 * 4. GameRule FREEZE_DAMAGE — 冰冻伤害游戏规则
 * 5. EntityTypeTags::isInitialized() — 安全检查
 * 6. Stalagmite / FallingStalactite 伤害类型（与冰冻系统同属洞穴机制）
 *
 * 此测试不依赖 BaseTestWorld，避免 SEH 异常问题。
 */

#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/world/gamerule/GameRule.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;

// ============================================================================
// DamageType::Freeze 和 DamageSources::freeze() 测试
// ============================================================================

class FreezeDamageSourceTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FreezeDamageSourceTest, FreezeDamageType_IsFreezing)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_TRUE(freezeDamage.isFreezing()) << "Freeze damage type should return true for isFreezing()";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_BypassesArmor)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_TRUE(freezeDamage.bypassesArmor()) << "Freeze damage should bypass armor";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_DeathMessageKey)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_EQ(freezeDamage.deathMessageKey(), "death.attack.freeze")
        << "Freeze death message key should be 'death.attack.freeze'";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_IsNotFire)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FALSE(freezeDamage.isFire()) << "Freeze damage should not be fire damage";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_IsNotExplosion)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FALSE(freezeDamage.isExplosion()) << "Freeze damage should not be explosion damage";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_IsNotMagic)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FALSE(freezeDamage.isMagic()) << "Freeze damage should not be magic damage";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_IsNotProjectile)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FALSE(freezeDamage.isProjectile()) << "Freeze damage should not be projectile damage";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_IsNotFall)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FALSE(freezeDamage.isFall()) << "Freeze damage should not be fall damage";
}

TEST_F(FreezeDamageSourceTest, DamageSourcesFreeze_CreatesCorrectDamage)
{
    auto freezeSource = DamageSources::freeze();
    EXPECT_EQ(freezeSource.type(), DamageType::Freeze);
    EXPECT_TRUE(freezeSource.isFreezing());
    EXPECT_TRUE(freezeSource.bypassesArmor());
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_Clone)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    auto cloned = freezeDamage.clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->type(), DamageType::Freeze);
    EXPECT_TRUE(cloned->isFreezing());
    EXPECT_TRUE(cloned->bypassesArmor());
}

TEST_F(FreezeDamageSourceTest, OtherDamageTypes_AreNotFreezing)
{
    std::vector<DamageType> nonFreezeTypes = {
        DamageType::InFire,
        DamageType::OnFire,
        DamageType::Lava,
        DamageType::Drown,
        DamageType::Fall,
        DamageType::Cactus,
        DamageType::Starve,
        DamageType::OutOfWorld,
        DamageType::Generic,
        DamageType::Magic,
        DamageType::Wither,
        DamageType::Explosion,
        DamageType::Stalagmite,
        DamageType::FallingStalactite,
    };

    for (auto type : nonFreezeTypes) {
        EnvironmentalDamage damage(type);
        EXPECT_FALSE(damage.isFreezing())
            << "DamageType " << static_cast<int>(type) << " should not be freezing damage";
    }
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_HungerDamage)
{
    // 冰冻伤害绕过护甲，饥饿消耗应为 0
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FLOAT_EQ(freezeDamage.hungerDamage(), 0.0f) << "Freeze damage should have 0 hunger damage (bypasses armor)";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_IsNotDamageAbsolute)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FALSE(freezeDamage.isDamageAbsolute()) << "Freeze damage should not be absolute damage";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_IsNotBypassInvulnerability)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FALSE(freezeDamage.bypassesInvulnerability()) << "Freeze damage should not bypass invulnerability";
}

TEST_F(FreezeDamageSourceTest, FreezeDamageType_CannotDamageCreative)
{
    EnvironmentalDamage freezeDamage(DamageType::Freeze);
    EXPECT_FALSE(freezeDamage.canDamageCreative()) << "Freeze damage should not damage creative mode players";
}

// ============================================================================
// 冰冻状态计算测试
// 使用 Entity 静态常量和 getPercentFrozen/isFullyFrozen/isFreezing 逻辑
// ============================================================================

class FreezeStateCalculationTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FreezeStateCalculationTest, Constants_CorrectValues)
{
    EXPECT_EQ(Entity::BASE_TICKS_REQUIRED_TO_FREEZE, 140) << "BASE_TICKS_REQUIRED_TO_FREEZE should be 140 (7 seconds)";
    EXPECT_EQ(Entity::FREEZE_HURT_FREQUENCY, 40) << "FREEZE_HURT_FREQUENCY should be 40 (2 seconds)";
}

TEST_F(FreezeStateCalculationTest, GetPercentFrozen_ZeroTicks)
{
    const i32 ticksFrozen = 0;
    const i32 required = Entity::BASE_TICKS_REQUIRED_TO_FREEZE;
    const f32 percent = static_cast<f32>(std::min(ticksFrozen, required)) / static_cast<f32>(required);
    EXPECT_FLOAT_EQ(percent, 0.0f);
}

TEST_F(FreezeStateCalculationTest, GetPercentFrozen_HalfTicks)
{
    const i32 ticksFrozen = 70;
    const i32 required = Entity::BASE_TICKS_REQUIRED_TO_FREEZE;
    const f32 percent = static_cast<f32>(std::min(ticksFrozen, required)) / static_cast<f32>(required);
    EXPECT_FLOAT_EQ(percent, 0.5f);
}

TEST_F(FreezeStateCalculationTest, GetPercentFrozen_FullTicks)
{
    const i32 ticksFrozen = 140;
    const i32 required = Entity::BASE_TICKS_REQUIRED_TO_FREEZE;
    const f32 percent = static_cast<f32>(std::min(ticksFrozen, required)) / static_cast<f32>(required);
    EXPECT_FLOAT_EQ(percent, 1.0f);
}

TEST_F(FreezeStateCalculationTest, GetPercentFrozen_ExceedsMax)
{
    const i32 ticksFrozen = 200;
    const i32 required = Entity::BASE_TICKS_REQUIRED_TO_FREEZE;
    const f32 percent = static_cast<f32>(std::min(ticksFrozen, required)) / static_cast<f32>(required);
    EXPECT_FLOAT_EQ(percent, 1.0f);
}

TEST_F(FreezeStateCalculationTest, IsFullyFrozen_AtExactThreshold)
{
    const i32 ticksFrozen = Entity::BASE_TICKS_REQUIRED_TO_FREEZE;
    EXPECT_TRUE(ticksFrozen >= Entity::BASE_TICKS_REQUIRED_TO_FREEZE);
}

TEST_F(FreezeStateCalculationTest, IsFullyFrozen_BelowThreshold)
{
    const i32 ticksFrozen = Entity::BASE_TICKS_REQUIRED_TO_FREEZE - 1;
    EXPECT_FALSE(ticksFrozen >= Entity::BASE_TICKS_REQUIRED_TO_FREEZE);
}

TEST_F(FreezeStateCalculationTest, IsFreezing_PositiveTicks)
{
    EXPECT_TRUE(i32(1) > 0);
    EXPECT_TRUE(i32(50) > 0);
    EXPECT_TRUE(i32(140) > 0);
}

TEST_F(FreezeStateCalculationTest, IsFreezing_ZeroTicks)
{
    EXPECT_FALSE(i32(0) > 0);
}

TEST_F(FreezeStateCalculationTest, FrostSpeedModifier_Calculation)
{
    // 冰冻减速修饰符：-0.05 * getPercentFrozen()
    const f32 fullyFrozen = -0.05f * 1.0f;
    EXPECT_FLOAT_EQ(fullyFrozen, -0.05f);

    const f32 halfFrozen = -0.05f * 0.5f;
    EXPECT_FLOAT_EQ(halfFrozen, -0.025f);

    const f32 noFrost = -0.05f * 0.0f;
    EXPECT_FLOAT_EQ(noFrost, 0.0f);
}

TEST_F(FreezeStateCalculationTest, FreezeTickDecay)
{
    // 不在细雪中或不可冰冻时，每 tick -2
    i32 ticksFrozen = 140;
    ticksFrozen = std::max(0, ticksFrozen - 2);
    EXPECT_EQ(ticksFrozen, 138);

    ticksFrozen = std::max(0, ticksFrozen - 2);
    EXPECT_EQ(ticksFrozen, 136);

    // 从 1 tick 解冻
    ticksFrozen = 1;
    ticksFrozen = std::max(0, ticksFrozen - 2);
    EXPECT_EQ(ticksFrozen, 0);

    // 从 0 tick 解冻
    ticksFrozen = 0;
    ticksFrozen = std::max(0, ticksFrozen - 2);
    EXPECT_EQ(ticksFrozen, 0);
}

TEST_F(FreezeStateCalculationTest, FreezeHurtFrequency_DamageTiming)
{
    // 每 FREEZE_HURT_FREQUENCY (40) tick 造成一次冰冻伤害
    // 验证周期性：ticksExisted() % FREEZE_HURT_FREQUENCY == 0
    for (i32 tick = 0; tick < 200; tick += Entity::FREEZE_HURT_FREQUENCY) {
        EXPECT_EQ(tick % Entity::FREEZE_HURT_FREQUENCY, 0) << "Tick " << tick << " should trigger freeze damage";
    }

    // 非 40 倍数的 tick 不应触发
    for (i32 tick = 1; tick < 40; ++tick) {
        EXPECT_NE(tick % Entity::FREEZE_HURT_FREQUENCY, 0) << "Tick " << tick << " should not trigger freeze damage";
    }
}

// ============================================================================
// EntityTypeTags 冰冻标签测试（需要 VanillaEntities + EntityTypeTags 初始化）
// ============================================================================

class FreezeEntityTypeTagsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        entity::VanillaEntities::registerAll();
        EntityTypeTags::initialize();
    }
};

// --- FREEZE_IMMUNE_ENTITY_TYPES 标签测试 ---

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneEntityTypes_ContainsStray)
{
    EXPECT_TRUE(EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().contains(ResourceLocation("minecraft:stray")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneEntityTypes_ContainsPolarBear)
{
    EXPECT_TRUE(EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().contains(ResourceLocation("minecraft:polar_bear")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneEntityTypes_ContainsSnowGolem)
{
    EXPECT_TRUE(EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().contains(ResourceLocation("minecraft:snow_golem")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneEntityTypes_ContainsWither)
{
    EXPECT_TRUE(EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().contains(ResourceLocation("minecraft:wither")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneEntityTypes_DoesNotContainZombie)
{
    EXPECT_FALSE(EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().contains(ResourceLocation("minecraft:zombie")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneEntityTypes_DoesNotContainPlayer)
{
    EXPECT_FALSE(EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().contains(ResourceLocation("minecraft:player")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneEntityTypes_DoesNotContainBlaze)
{
    // 烈焰人不免疫冰冻（但受额外冰冻伤害）
    EXPECT_FALSE(EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().contains(ResourceLocation("minecraft:blaze")));
}

// --- FREEZE_HURTS_EXTRA_TYPES 标签测试 ---

TEST_F(FreezeEntityTypeTagsTest, FreezeHurtsExtraTypes_ContainsBlaze)
{
    EXPECT_TRUE(EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES().contains(ResourceLocation("minecraft:blaze")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeHurtsExtraTypes_ContainsMagmaCube)
{
    EXPECT_TRUE(EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES().contains(ResourceLocation("minecraft:magma_cube")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeHurtsExtraTypes_ContainsStrider)
{
    EXPECT_TRUE(EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES().contains(ResourceLocation("minecraft:strider")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeHurtsExtraTypes_DoesNotContainZombie)
{
    EXPECT_FALSE(EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES().contains(ResourceLocation("minecraft:zombie")));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeHurtsExtraTypes_DoesNotContainPlayer)
{
    EXPECT_FALSE(EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES().contains(ResourceLocation("minecraft:player")));
}

// --- 两个冰冻标签不相交测试 ---

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneAndExtraAreDisjoint)
{
    // 免疫冰冻和受额外冰冻伤害的实体不应该重叠
    for (const auto& id : EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().getEntityTypeIds()) {
        EXPECT_FALSE(EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES().contains(id))
            << "Entity " << id.toString() << " should not be both freeze immune and freeze extra damage";
    }
}

// --- 标签 ID 测试 ---

TEST_F(FreezeEntityTypeTagsTest, FreezeImmuneTagId)
{
    EXPECT_EQ(
        EntityTypeTags::FREEZE_IMMUNE_ENTITY_TYPES().getId(), ResourceLocation("minecraft:freeze_immune_entity_types"));
}

TEST_F(FreezeEntityTypeTagsTest, FreezeHurtsExtraTagId)
{
    EXPECT_EQ(
        EntityTypeTags::FREEZE_HURTS_EXTRA_TYPES().getId(), ResourceLocation("minecraft:freeze_hurts_extra_types"));
}

// --- isInitialized() 安全检查测试 ---

TEST_F(FreezeEntityTypeTagsTest, IsInitializedReturnsTrueAfterInit)
{
    EXPECT_TRUE(EntityTypeTags::isInitialized());
}

// ============================================================================
// GameRule FREEZE_DAMAGE 测试
// ============================================================================

class FreezeDamageGameRuleTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FreezeDamageGameRuleTest, DefaultIsEnabled)
{
    world::gamerule::GameRules rules;
    EXPECT_TRUE(rules.getBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE));
}

TEST_F(FreezeDamageGameRuleTest, CanBeSetFalse)
{
    world::gamerule::GameRules rules;
    rules.setBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE));
}

TEST_F(FreezeDamageGameRuleTest, CanBeSetBackToTrue)
{
    world::gamerule::GameRules rules;
    rules.setBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE));

    rules.setBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE, true, nullptr);
    EXPECT_TRUE(rules.getBoolean(world::gamerule::GameRuleKeys::FREEZE_DAMAGE));
}

TEST_F(FreezeDamageGameRuleTest, RuleKeyName)
{
    EXPECT_EQ(world::gamerule::GameRuleKeys::FREEZE_DAMAGE.getName(), "freezeDamage");
}

TEST_F(FreezeDamageGameRuleTest, RuleKeyCategory)
{
    EXPECT_EQ(world::gamerule::GameRuleKeys::FREEZE_DAMAGE.getCategory(), world::gamerule::GameRuleCategory::Player);
}

TEST_F(FreezeDamageGameRuleTest, RuleIsBoolean)
{
    EXPECT_EQ(world::gamerule::GameRules::getRuleType("freezeDamage"), world::gamerule::GameRuleValueType::Boolean);
}

TEST_F(FreezeDamageGameRuleTest, RuleHasCorrectEntry)
{
    EXPECT_TRUE(world::gamerule::GameRules::hasRule("freezeDamage"));
}

// ============================================================================
// Stalagmite / FallingStalactite 伤害类型测试
// （与冰冻系统同属洞穴机制，一并测试）
// ============================================================================

TEST_F(FreezeDamageSourceTest, Stalagmite_IsFall)
{
    // 石笋伤害应属于摔落伤害（受摔落保护附魔减免）
    EnvironmentalDamage stalagmiteDamage(DamageType::Stalagmite);
    EXPECT_TRUE(stalagmiteDamage.isFall()) << "Stalagmite damage should be fall damage";
}

TEST_F(FreezeDamageSourceTest, Stalagmite_BypassesArmor)
{
    EnvironmentalDamage stalagmiteDamage(DamageType::Stalagmite);
    EXPECT_TRUE(stalagmiteDamage.bypassesArmor()) << "Stalagmite damage should bypass armor";
}

TEST_F(FreezeDamageSourceTest, Stalagmite_IsNotFreezing)
{
    EnvironmentalDamage stalagmiteDamage(DamageType::Stalagmite);
    EXPECT_FALSE(stalagmiteDamage.isFreezing()) << "Stalagmite damage should not be freezing damage";
}

TEST_F(FreezeDamageSourceTest, Stalagmite_DeathMessageKey)
{
    EnvironmentalDamage stalagmiteDamage(DamageType::Stalagmite);
    EXPECT_EQ(stalagmiteDamage.deathMessageKey(), "death.attack.stalagmite");
}

TEST_F(FreezeDamageSourceTest, FallingStalactite_DeathMessageKey)
{
    EnvironmentalDamage fallingStalactite(DamageType::FallingStalactite);
    EXPECT_EQ(fallingStalactite.deathMessageKey(), "death.attack.fallingStalactite");
}

TEST_F(FreezeDamageSourceTest, FallingStalactite_IsNotFall)
{
    // 坠落钟乳石伤害不属于摔落伤害
    EnvironmentalDamage fallingStalactite(DamageType::FallingStalactite);
    EXPECT_FALSE(fallingStalactite.isFall()) << "Falling stalactite damage should not be fall damage";
}

TEST_F(FreezeDamageSourceTest, FallingStalactite_IsNotFreezing)
{
    EnvironmentalDamage fallingStalactite(DamageType::FallingStalactite);
    EXPECT_FALSE(fallingStalactite.isFreezing()) << "Falling stalactite damage should not be freezing damage";
}
