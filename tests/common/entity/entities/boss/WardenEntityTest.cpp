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

/**
 * @file WardenEntityTest.cpp
 * @brief WardenEntity 单元测试
 *
 * 测试内容：
 * - 实体类型注册（minecraft:warden 存在、canSummon 为 true）
 * - 工厂方法 create() 返回有效实体
 * - 实体尺寸（width=0.9, height=2.9, eyeHeight=2.4）
 * - 属性值（HP=500, 攻击=30, 速度=0.3, 击退抗性=1.0, 攻击击退=1.5, 跟随=24）
 * - Boss 行为重写（isNonBoss=false, preventDespawn=true, isDespawnPeaceful=true）
 * - dampensVibrations=true
 * - 摔落免疫（onLivingFall=false）
 * - 伤害判定对齐 vanilla（Warden 不免疫溺水/凋零，对齐 Warden.java:151-154）
 * - 声音 ID（getAmbientSound / getHurtSound / getDeathSound）
 * - 怒气等级（WardenAngerLevel）切换与环境音效映射
 * - 怒气增加、上限、清空、客户端同步
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/boss/WardenAngerLevel.hpp"
#include "common/entity/entities/boss/WardenEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace {

/**
 * @brief 测试夹具
 *
 * 在每个测试用例运行前注册所有原版实体类型，确保 EntityRegistry 包含 minecraft:warden。
 * 注意：EntityRegistry 是单例，注册操作在整个测试进程内只生效一次。
 */
class WardenEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto& registry = entity::EntityRegistry::instance();
        if (!registry.hasType("minecraft:warden")) {
            // 注册监守者实体类型（与 VanillaEntities.hpp 中的注册保持一致）
            registry.registerType("minecraft:warden",
                entity::EntityType::Builder(&entity::WardenEntity::create, entity::EntityClassification::Monster)
                    .size(0.9f, 2.9f)
                    .trackingRange(16)
                    .updateInterval(3)
                    .canSummon(true)
                    .build());
            // 初始化 VanillaEntityTypeKeys 缓存
            entity::VanillaEntityTypeKeys::WARDEN = registry.getType("minecraft:warden");
        }
    }
};

// ========== 实体类型注册测试 ==========

TEST_F(WardenEntityTest, EntityType_RegisteredInRegistry)
{
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* wardenType = registry.getType("minecraft:warden");
    ASSERT_NE(wardenType, nullptr);
    EXPECT_EQ(wardenType->name(), "minecraft:warden");
}

TEST_F(WardenEntityTest, EntityType_CanSummon_IsTrue)
{
    // canSummon 是 SculkShriekerHelper._trySummonWarden 的契约关键
    const entity::EntityType* wardenType = entity::EntityRegistry::instance().getType("minecraft:warden");
    ASSERT_NE(wardenType, nullptr);
    EXPECT_TRUE(wardenType->canSummon());
}

TEST_F(WardenEntityTest, EntityType_IdCachedInVanillaEntityTypeKeys)
{
    // 确保 VanillaEntityTypeKeys::WARDEN 与注册表中的类型指针一致（同源指针别名）
    const entity::EntityType* wardenType = entity::EntityRegistry::instance().getType("minecraft:warden");
    ASSERT_NE(wardenType, nullptr);
    EXPECT_EQ(entity::VanillaEntityTypeKeys::WARDEN, wardenType);
    EXPECT_NE(entity::VanillaEntityTypeKeys::WARDEN, nullptr);
}

// ========== 工厂方法测试 ==========

TEST_F(WardenEntityTest, Create_ReturnsNonNullEntity)
{
    auto entity = entity::WardenEntity::create(nullptr, mc::test::testEcsRegistry());
    EXPECT_NE(entity, nullptr);
}

TEST_F(WardenEntityTest, Create_ReturnsWardenEntityType)
{
    auto entity = entity::WardenEntity::create(nullptr, mc::test::testEcsRegistry());
    ASSERT_NE(entity, nullptr);
    // 创建的实体类型应为 WardenEntity 实例
    auto* warden = dynamic_cast<entity::WardenEntity*>(entity.get());
    EXPECT_NE(warden, nullptr);
}

// ========== 尺寸测试 ==========

TEST_F(WardenEntityTest, Dimensions_MatchMCJavaValues)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(warden.width(), 0.9f);
    EXPECT_FLOAT_EQ(warden.height(), 2.9f);
    EXPECT_FLOAT_EQ(warden.eyeHeight(), 2.4f);
}

// ========== 属性测试 ==========

TEST_F(WardenEntityTest, Attributes_MaxHealth_Is500)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(warden.maxHealth(), 500.0f);
}

TEST_F(WardenEntityTest, Attributes_AttackDamage_Is30)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::ATTACK_DAMAGE), 30.0f);
}

TEST_F(WardenEntityTest, Attributes_MovementSpeed_Is0_3)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::MOVEMENT_SPEED), 0.3f);
}

TEST_F(WardenEntityTest, Attributes_KnockbackResistance_Is1_0)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE), 1.0f);
}

TEST_F(WardenEntityTest, Attributes_AttackKnockback_Is1_5)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::ATTACK_KNOCKBACK), 1.5f);
}

TEST_F(WardenEntityTest, Attributes_FollowRange_Is24)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::FOLLOW_RANGE), 24.0f);
}

// ========== Boss 行为重写测试 ==========

TEST_F(WardenEntityTest, IsNonBoss_ReturnsFalse)
{
    // 监守者不参与普通怪物的生成限制
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(warden.isNonBoss());
}

TEST_F(WardenEntityTest, PreventDespawn_ReturnsTrue)
{
    // 监守者永不自然消失（对应 MC Warden.removeWhenFarAway() == false）
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(warden.preventDespawn());
}

TEST_F(WardenEntityTest, IsDespawnPeaceful_ReturnsTrue)
{
    // 和平难度下监守者消失（继承自 MonsterEntity）
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(warden.isDespawnPeaceful());
}

// ========== 振动抑制测试 ==========

TEST_F(WardenEntityTest, DampensVibrations_ReturnsTrue)
{
    // 对应 MC 1.21.11 Warden.dampensVibrations() == true
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(warden.dampensVibrations());
}

// ========== 摔落免疫测试 ==========

TEST_F(WardenEntityTest, OnLivingFall_ReturnsFalse_NoFallDamage)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(warden.onLivingFall(10.0f, 1.0f));
    EXPECT_FALSE(warden.onLivingFall(0.0f, 1.0f));
}

// ========== 伤害判定测试 ==========

// 监守者【不免疫】溺水伤害（对齐 vanilla Warden.java:151-154）。
//
// vanilla Warden.isInvulnerableTo 仅在 Digging/Emerging 姿态下免疫除 BYPASSES_INVULNERABILITY
// 外的所有伤害，无任何溺水免疫。且 Warden 不在 CAN_BREATHE_UNDER_WATER 标签中
// （can_breathe_under_water.json 成员=#undead+水生生物，Warden 非亡灵非水生），
// 故 vanilla Warden 在水中会正常溺水。Cubium 此前硬编码 source.type()==Drown → return true，
// 是与 vanilla 直接冲突的真实缺陷（已删除，委托基类 MonsterEntity::isInvulnerableTo）。
//
// 注意：WardenEntityTest::SetUp 不初始化 DamageTypeTags，但 Drown 经基类 LivingEntity::isInvulnerableTo
// 判定时不在 IS_FALL/IS_FIRE 门控（Drown 非这两个标签成员），不依赖标签成员集，故无需初始化。
TEST_F(WardenEntityTest, IsInvulnerableTo_Drown_ReturnsFalse_AlignedWithVanilla)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EnvironmentalDamage source = DamageSources::drown();
    // 对齐 vanilla：Warden 不免疫溺水，isInvulnerableTo 返回 false。
    EXPECT_FALSE(warden.isInvulnerableTo(source));
}

// 监守者【不免疫】凋零伤害（对齐 vanilla Warden.java:151-154）。
//
// vanilla Warden 无凋零伤害类型免疫。凋零效果的免疫应通过效果体系（MobEffect）处理，
// 非 isInvulnerableTo。Cubium 此前硬编码 source.type()==Wither → return true 是与 vanilla
// 冲突的真实缺陷（已删除）。凋零伤害类型（Wither）经基类判定不触发任何门控，故返回 false。
TEST_F(WardenEntityTest, IsInvulnerableTo_Wither_ReturnsFalse_AlignedWithVanilla)
{
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EnvironmentalDamage source = DamageSources::wither();
    // 对齐 vanilla：Warden 不免疫凋零伤害，isInvulnerableTo 返回 false。
    EXPECT_FALSE(warden.isInvulnerableTo(source));
}

// ========== 声音测试 ==========

TEST_F(WardenEntityTest, GetAmbientSound_Calmed_ReturnsWardenAmbient)
{
    // 初始怒气为 0 → Calmed → WARDEN_AMBIENT
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto sound = warden.getAmbientSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), SoundEvents::ENTITY_WARDEN_AMBIENT.toString());
}

TEST_F(WardenEntityTest, GetAmbientSound_Agitated_ReturnsWardenAgitated)
{
    // 怒气 40-79 → Agitated → WARDEN_AGITATED
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(50);
    auto sound = warden.getAmbientSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), SoundEvents::ENTITY_WARDEN_AGITATED.toString());
}

TEST_F(WardenEntityTest, GetAmbientSound_Angry_ReturnsWardenAngry)
{
    // 怒气 ≥ 80 → Angry → WARDEN_ANGRY
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(80);
    auto sound = warden.getAmbientSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), SoundEvents::ENTITY_WARDEN_ANGRY.toString());
}

TEST_F(WardenEntityTest, GetHurtSound_ReturnsWardenHurt)
{
    // MC 1.21.11 Warden.getHurtSound() 返回 SoundEvents.WARDEN_HURT
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EnvironmentalDamage source = DamageSources::generic();
    auto sound = warden.getHurtSound(source);
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), SoundEvents::ENTITY_WARDEN_HURT.toString());
}

TEST_F(WardenEntityTest, GetDeathSound_ReturnsWardenDeath)
{
    // MC 1.21.11 Warden.getDeathSound() 返回 SoundEvents.WARDEN_DEATH
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    auto sound = warden.getDeathSound();
    ASSERT_TRUE(sound.has_value());
    EXPECT_EQ(sound->toString(), SoundEvents::ENTITY_WARDEN_DEATH.toString());
}

// ========== 经验值测试 ==========

TEST_F(WardenEntityTest, ExperienceValue_Is5)
{
    // MC 1.21.11 Warden 构造函数: this.xpReward = 5
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(warden.experienceValue(), 5);
}

// ========== 怒气等级（WardenAngerLevel）测试 ==========

TEST_F(WardenEntityTest, AngerLevel_Initial_IsCalmed)
{
    // 新生成的监守者怒气为 0，对应 Calmed 等级
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Calmed);
    EXPECT_EQ(warden.getClientAngerLevel(), 0);
}

TEST_F(WardenEntityTest, AngerLevel_IncreaseBelow40_StaysCalmed)
{
    // 怒气 < 40 → Calmed
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(39);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Calmed);
    EXPECT_EQ(warden.getClientAngerLevel(), 39);
}

TEST_F(WardenEntityTest, AngerLevel_IncreaseTo40_BecomesAgitated)
{
    // 怒气 = 40 → Agitated（阈值边界）
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(40);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Agitated);
    EXPECT_EQ(warden.getClientAngerLevel(), 40);
}

TEST_F(WardenEntityTest, AngerLevel_IncreaseTo79_StaysAgitated)
{
    // 怒气 40-79 → Agitated
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(79);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Agitated);
}

TEST_F(WardenEntityTest, AngerLevel_IncreaseTo80_BecomesAngry)
{
    // 怒气 = 80 → Angry（阈值边界）
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(80);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Angry);
    EXPECT_EQ(warden.getClientAngerLevel(), 80);
}

TEST_F(WardenEntityTest, AngerLevel_IncreaseBeyondLimit_ClampedTo150)
{
    // 怒气上限 150（防止无限增长）
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(1000);
    EXPECT_EQ(warden.getClientAngerLevel(), 150);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Angry);
}

TEST_F(WardenEntityTest, AngerLevel_IncreaseMultipleTimes_Accumulates)
{
    // 多次增加怒气应累加
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(30);
    warden.increaseAnger(30);
    EXPECT_EQ(warden.getClientAngerLevel(), 60);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Agitated);
    warden.increaseAnger(30);
    EXPECT_EQ(warden.getClientAngerLevel(), 90);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Angry);
}

TEST_F(WardenEntityTest, AngerLevel_ClearAnger_ResetsToZero)
{
    // clearAnger() 应将怒气归零
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(100);
    ASSERT_EQ(warden.getClientAngerLevel(), 100);
    warden.clearAnger();
    EXPECT_EQ(warden.getClientAngerLevel(), 0);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Calmed);
}

TEST_F(WardenEntityTest, AngerLevel_IncreaseNegative_NoEffect)
{
    // increaseAnger 负数应无效果
    entity::WardenEntity warden(EntityInstanceId(1), mc::test::testEcsRegistry());
    warden.increaseAnger(-10);
    EXPECT_EQ(warden.getClientAngerLevel(), 0);
    EXPECT_EQ(warden.getAngerLevel(), entity::WardenAngerLevel::Calmed);
}

// ========== WardenAngerLevel 工具函数测试 ==========

TEST_F(WardenEntityTest, WardenAngerLevel_ByAnger_BoundariesMatchMC)
{
    // 与 MC 1.21.11 AngerLevel.byAnger 行为一致
    using entity::WardenAngerLevel;
    EXPECT_EQ(entity::wardenAngerLevelByAnger(0), WardenAngerLevel::Calmed);
    EXPECT_EQ(entity::wardenAngerLevelByAnger(39), WardenAngerLevel::Calmed);
    EXPECT_EQ(entity::wardenAngerLevelByAnger(40), WardenAngerLevel::Agitated);
    EXPECT_EQ(entity::wardenAngerLevelByAnger(79), WardenAngerLevel::Agitated);
    EXPECT_EQ(entity::wardenAngerLevelByAnger(80), WardenAngerLevel::Angry);
    EXPECT_EQ(entity::wardenAngerLevelByAnger(150), WardenAngerLevel::Angry);
}

TEST_F(WardenEntityTest, WardenAngerLevel_MinimumAnger_MatchesMC)
{
    // 与 MC 1.21.11 AngerLevel.getMinimumAnger() 一致
    using entity::WardenAngerLevel;
    EXPECT_EQ(entity::wardenAngerLevelMinimumAnger(WardenAngerLevel::Calmed), 0);
    EXPECT_EQ(entity::wardenAngerLevelMinimumAnger(WardenAngerLevel::Agitated), 40);
    EXPECT_EQ(entity::wardenAngerLevelMinimumAnger(WardenAngerLevel::Angry), 80);
}

TEST_F(WardenEntityTest, WardenAngerLevel_IsAngry_OnlyForAngryLevel)
{
    // 对应 MC 1.21.11 AngerLevel.isAngry()
    using entity::WardenAngerLevel;
    EXPECT_FALSE(entity::wardenAngerLevelIsAngry(WardenAngerLevel::Calmed));
    EXPECT_FALSE(entity::wardenAngerLevelIsAngry(WardenAngerLevel::Agitated));
    EXPECT_TRUE(entity::wardenAngerLevelIsAngry(WardenAngerLevel::Angry));
}

TEST_F(WardenEntityTest, WardenAngerLevel_AmbientSound_MatchesMC)
{
    // 验证各等级环境音效映射
    using entity::WardenAngerLevel;
    EXPECT_EQ(entity::wardenAngerLevelAmbientSound(WardenAngerLevel::Calmed).toString(),
        SoundEvents::ENTITY_WARDEN_AMBIENT.toString());
    EXPECT_EQ(entity::wardenAngerLevelAmbientSound(WardenAngerLevel::Agitated).toString(),
        SoundEvents::ENTITY_WARDEN_AGITATED.toString());
    EXPECT_EQ(entity::wardenAngerLevelAmbientSound(WardenAngerLevel::Angry).toString(),
        SoundEvents::ENTITY_WARDEN_ANGRY.toString());
}

TEST_F(WardenEntityTest, WardenAngerLevel_ListeningSound_MatchesMC)
{
    // 验证各等级倾听音效映射
    using entity::WardenAngerLevel;
    EXPECT_EQ(entity::wardenAngerLevelListeningSound(WardenAngerLevel::Calmed).toString(),
        SoundEvents::ENTITY_WARDEN_LISTENING.toString());
    EXPECT_EQ(entity::wardenAngerLevelListeningSound(WardenAngerLevel::Agitated).toString(),
        SoundEvents::ENTITY_WARDEN_LISTENING_ANGRY.toString());
    EXPECT_EQ(entity::wardenAngerLevelListeningSound(WardenAngerLevel::Angry).toString(),
        SoundEvents::ENTITY_WARDEN_LISTENING_ANGRY.toString());
}

} // namespace
} // namespace mc
