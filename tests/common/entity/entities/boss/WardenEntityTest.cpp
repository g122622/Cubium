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
 * - 伤害免疫（Drown / Wither）
 * - 声音 ID 不为空
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/boss/WardenEntity.hpp"
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
            // 初始化 EntityTypeIdNumber 缓存
            entity::EntityTypeIdNumber::WARDEN = registry.getType("minecraft:warden")->id();
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

TEST_F(WardenEntityTest, EntityType_IdCachedInEntityTypeIdNumber)
{
    // 确保 EntityTypeIdNumber::WARDEN 与注册表中的 ID 一致
    const entity::EntityType* wardenType = entity::EntityRegistry::instance().getType("minecraft:warden");
    ASSERT_NE(wardenType, nullptr);
    EXPECT_EQ(entity::EntityTypeIdNumber::WARDEN, wardenType->id());
    EXPECT_NE(entity::EntityTypeIdNumber::WARDEN, 0);
}

// ========== 工厂方法测试 ==========

TEST_F(WardenEntityTest, Create_ReturnsNonNullEntity)
{
    auto entity = entity::WardenEntity::create(nullptr);
    EXPECT_NE(entity, nullptr);
}

TEST_F(WardenEntityTest, Create_ReturnsWardenEntityType)
{
    auto entity = entity::WardenEntity::create(nullptr);
    ASSERT_NE(entity, nullptr);
    // 创建的实体类型应为 WardenEntity 实例
    auto* warden = dynamic_cast<entity::WardenEntity*>(entity.get());
    EXPECT_NE(warden, nullptr);
}

// ========== 尺寸测试 ==========

TEST_F(WardenEntityTest, Dimensions_MatchMCJavaValues)
{
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FLOAT_EQ(warden.width(), 0.9f);
    EXPECT_FLOAT_EQ(warden.height(), 2.9f);
    EXPECT_FLOAT_EQ(warden.eyeHeight(), 2.4f);
}

// ========== 属性测试 ==========

TEST_F(WardenEntityTest, Attributes_MaxHealth_Is500)
{
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FLOAT_EQ(warden.maxHealth(), 500.0f);
}

TEST_F(WardenEntityTest, Attributes_AttackDamage_Is30)
{
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::ATTACK_DAMAGE), 30.0f);
}

TEST_F(WardenEntityTest, Attributes_MovementSpeed_Is0_3)
{
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::MOVEMENT_SPEED), 0.3f);
}

TEST_F(WardenEntityTest, Attributes_KnockbackResistance_Is1_0)
{
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE), 1.0f);
}

TEST_F(WardenEntityTest, Attributes_AttackKnockback_Is1_5)
{
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::ATTACK_KNOCKBACK), 1.5f);
}

TEST_F(WardenEntityTest, Attributes_FollowRange_Is24)
{
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FLOAT_EQ(warden.attributes().getValue(entity::attribute::Attributes::FOLLOW_RANGE), 24.0f);
}

// ========== Boss 行为重写测试 ==========

TEST_F(WardenEntityTest, IsNonBoss_ReturnsFalse)
{
    // 监守者不参与普通怪物的生成限制
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FALSE(warden.isNonBoss());
}

TEST_F(WardenEntityTest, PreventDespawn_ReturnsTrue)
{
    // 监守者永不自然消失（对应 MC Warden.removeWhenFarAway() == false）
    entity::WardenEntity warden(EntityId(1));
    EXPECT_TRUE(warden.preventDespawn());
}

TEST_F(WardenEntityTest, IsDespawnPeaceful_ReturnsTrue)
{
    // 和平难度下监守者消失（继承自 MonsterEntity）
    entity::WardenEntity warden(EntityId(1));
    EXPECT_TRUE(warden.isDespawnPeaceful());
}

// ========== 振动抑制测试 ==========

TEST_F(WardenEntityTest, DampensVibrations_ReturnsTrue)
{
    // 对应 MC 1.21.11 Warden.dampensVibrations() == true
    entity::WardenEntity warden(EntityId(1));
    EXPECT_TRUE(warden.dampensVibrations());
}

// ========== 摔落免疫测试 ==========

TEST_F(WardenEntityTest, OnLivingFall_ReturnsFalse_NoFallDamage)
{
    entity::WardenEntity warden(EntityId(1));
    EXPECT_FALSE(warden.onLivingFall(10.0f, 1.0f));
    EXPECT_FALSE(warden.onLivingFall(0.0f, 1.0f));
}

// ========== 伤害免疫测试 ==========

TEST_F(WardenEntityTest, IsInvulnerableTo_Drown_ReturnsTrue)
{
    entity::WardenEntity warden(EntityId(1));
    EnvironmentalDamage source = DamageSources::drown();
    EXPECT_TRUE(warden.isInvulnerableTo(source));
}

TEST_F(WardenEntityTest, IsInvulnerableTo_Wither_ReturnsTrue)
{
    entity::WardenEntity warden(EntityId(1));
    EnvironmentalDamage source = DamageSources::wither();
    EXPECT_TRUE(warden.isInvulnerableTo(source));
}

// ========== 声音测试 ==========

TEST_F(WardenEntityTest, GetAmbientSound_NotEmpty)
{
    entity::WardenEntity warden(EntityId(1));
    // makeSoundEventId 依赖 getTypeId() 返回非 "unknown" 字符串，
    // 因此需要先设置实体类型 ID。
    warden.setTypeId("minecraft:warden");
    auto sound = warden.getAmbientSound();
    EXPECT_TRUE(sound.has_value());
    if (sound) {
        EXPECT_EQ(sound->toString(), "minecraft:entity.warden.ambient");
    }
}

TEST_F(WardenEntityTest, GetHurtSound_NotEmpty)
{
    entity::WardenEntity warden(EntityId(1));
    warden.setTypeId("minecraft:warden");
    EnvironmentalDamage source = DamageSources::generic();
    auto sound = warden.getHurtSound(source);
    EXPECT_TRUE(sound.has_value());
    if (sound) {
        EXPECT_EQ(sound->toString(), "minecraft:entity.warden.hurt");
    }
}

TEST_F(WardenEntityTest, GetDeathSound_NotEmpty)
{
    entity::WardenEntity warden(EntityId(1));
    warden.setTypeId("minecraft:warden");
    auto sound = warden.getDeathSound();
    EXPECT_TRUE(sound.has_value());
    if (sound) {
        EXPECT_EQ(sound->toString(), "minecraft:entity.warden.death");
    }
}

// ========== 经验值测试 ==========

TEST_F(WardenEntityTest, ExperienceValue_Is5)
{
    // MC 1.21.11 Warden 构造函数: this.xpReward = 5
    entity::WardenEntity warden(EntityId(1));
    EXPECT_EQ(warden.experienceValue(), 5);
}

} // namespace
} // namespace mc
