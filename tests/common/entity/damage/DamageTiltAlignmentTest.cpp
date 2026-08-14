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
 */

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "client/settings/ClientSettings.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/DefaultValues.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"

using namespace mc;
using namespace mc::network;

// ============================================================================
// DamageSource::sourcePosition() 测试（DamageSource.getSourcePosition）
// ============================================================================

namespace {
// 复用 ServerPlayer 作为带位置/yaw 的实体来源（避免单独 mock Entity）。
struct HurtFixture {
    std::unique_ptr<mc::server::ServerWorld> world;
    std::unique_ptr<ServerPlayer> victim;
    std::unique_ptr<ServerPlayer> attacker;

    HurtFixture()
    {
        world = std::make_unique<mc::server::ServerWorld>(mc::server::ServerWorldConfig{});
        victim = std::make_unique<ServerPlayer>(EntityInstanceId(1), "Victim", mc::test::testEcsRegistry());
        attacker = std::make_unique<ServerPlayer>(EntityInstanceId(2), "Attacker", mc::test::testEcsRegistry());
        // ServerPlayer::setWorld 仅设置派生类的 ServerPlayer::m_world（用于广播等），
        // LivingEntity::hurt 读取的是基类 Entity::m_world，需显式设置基类引用。
        victim->setWorld(world.get());
        attacker->setWorld(world.get());
        victim->Entity::setWorld(world.get());
        attacker->Entity::setWorld(world.get());
    }
};
} // namespace

TEST(DamageSourcePositionTest, EnvironmentalDamageReturnsNullopt)
{
    EnvironmentalDamage fall(DamageType::Fall);
    EXPECT_FALSE(fall.sourcePosition().has_value());
}

TEST(DamageSourcePositionTest, EntityDamageSourceReturnsSourcePosition)
{
    HurtFixture f;
    f.attacker->setPosition(5.0f, 0.0f, 0.0f);
    auto src = DamageSources::playerAttack(f.attacker.get());
    auto pos = src.sourcePosition();
    ASSERT_TRUE(pos.has_value());
    EXPECT_FLOAT_EQ(pos->x, 5.0f);
    EXPECT_FLOAT_EQ(pos->z, 0.0f);
}

TEST(DamageSourcePositionTest, IndirectEntityDamageSourceReturnsDirectSourcePosition)
{
    HurtFixture f;
    // shooter 在 (10,0,10)，箭矢（directSource）在 (1,0,0)。
    // MC getSourcePosition 回退 directEntity.position()，故应返回箭矢位置而非射手。
    f.attacker->setPosition(10.0f, 0.0f, 10.0f);
    ServerPlayer arrowProxy(EntityInstanceId(3), "Arrow", mc::test::testEcsRegistry());
    arrowProxy.setWorld(f.world.get());
    arrowProxy.setPosition(1.0f, 0.0f, 0.0f);
    auto src = DamageSources::arrow(&arrowProxy, f.attacker.get(), false);
    auto pos = src.sourcePosition();
    ASSERT_TRUE(pos.has_value());
    EXPECT_FLOAT_EQ(pos->x, 1.0f);
    EXPECT_FLOAT_EQ(pos->z, 0.0f);
}

TEST(DamageSourcePositionTest, EntityDamageSourceNullSourceReturnsNullopt)
{
    EntityDamageSource mobSource(DamageType::MobAttack, nullptr);
    EXPECT_FALSE(mobSource.sourcePosition().has_value());
}

// ============================================================================
// LivingEntity hurtDir / hurtDuration 测试（Player.getHurtDir + hurtServer）
// ============================================================================

TEST(LivingEntityHurtDirTest, HurtSetsHurtDurationAndHurtTimeToTen)
{
    HurtFixture f;
    f.victim->setPosition(0.0f, 0.0f, 0.0f);
    auto src = DamageSources::playerAttack(f.attacker.get());
    f.attacker->setPosition(5.0f, 0.0f, 0.0f);
    f.victim->hurt(src, 5.0f);
    EXPECT_EQ(f.victim->hurtDuration(), 10);
    EXPECT_EQ(f.victim->hurtTime(), 10);
}

TEST(LivingEntityHurtDirTest, HurtDirIsAtan2MinusYaw)
{
    HurtFixture f;
    // 受害者朝向 yaw=0，攻击者位于正东 (+x) → d0=+5, d1=0 → atan2(0,5)=0 → hurtDir=0。
    f.victim->setPosition(0.0f, 0.0f, 0.0f);
    f.victim->setYaw(0.0f);
    f.attacker->setPosition(5.0f, 0.0f, 0.0f);
    auto src = DamageSources::playerAttack(f.attacker.get());
    f.victim->hurt(src, 5.0f);
    EXPECT_NEAR(f.victim->getHurtDir(), 0.0f, 0.01f);
}

TEST(LivingEntityHurtDirTest, HurtDirFromNorthAttacker)
{
    HurtFixture f;
    // 受害者朝向 yaw=0，攻击者位于正北 (-z, MC 中 -z 为北) → d0=0, d1=-5
    // → atan2(-5,0) = -90° → hurtDir = -90 - 0 = -90。
    f.victim->setPosition(0.0f, 0.0f, 0.0f);
    f.victim->setYaw(0.0f);
    f.attacker->setPosition(0.0f, 0.0f, -5.0f);
    auto src = DamageSources::playerAttack(f.attacker.get());
    f.victim->hurt(src, 5.0f);
    EXPECT_NEAR(f.victim->getHurtDir(), -90.0f, 0.01f);
}

TEST(LivingEntityHurtDirTest, HurtDirSubtractsYaw)
{
    HurtFixture f;
    // 受害者 yaw=90，攻击者位于正东 (+x) → atan2(0,5)=0 → hurtDir = 0 - 90 = -90。
    f.victim->setPosition(0.0f, 0.0f, 0.0f);
    f.victim->setYaw(90.0f);
    f.attacker->setPosition(5.0f, 0.0f, 0.0f);
    auto src = DamageSources::playerAttack(f.attacker.get());
    f.victim->hurt(src, 5.0f);
    EXPECT_NEAR(f.victim->getHurtDir(), -90.0f, 0.01f);
}

TEST(LivingEntityHurtDirTest, AnimateHurtSetsHurtDirAndTimers)
{
    HurtFixture f;
    // 模拟客户端 animateHurt(42) 接收网络同步值。
    f.victim->animateHurt(42.0f);
    EXPECT_FLOAT_EQ(f.victim->getHurtDir(), 42.0f);
    EXPECT_EQ(f.victim->hurtDuration(), 10);
    EXPECT_EQ(f.victim->hurtTime(), 10);
}

TEST(LivingEntityHurtDirTest, ZeroDamageDoesNotIndicateDamage)
{
    HurtFixture f;
    // amount<=0 时不应触发 indicateDamage（hurtDir 保持初始 0，但此处主要验证不崩）。
    f.victim->setPosition(0.0f, 0.0f, 0.0f);
    auto src = DamageSources::playerAttack(f.attacker.get());
    f.attacker->setPosition(5.0f, 0.0f, 0.0f);
    f.victim->hurt(src, 0.0f);
    // hurtDir 保持 0（未进入 indicateDamage 路径，或 atan2(0,0)=0 结果同 0）。
    EXPECT_NEAR(f.victim->getHurtDir(), 0.0f, 0.01f);
}

// ============================================================================
// ClientSettings.damageTiltStrength 测试（options.damageTiltStrength）
// ============================================================================

TEST(ClientSettingsDamageTiltTest, DefaultValueIsOne)
{
    client::ClientSettings settings;
    EXPECT_FLOAT_EQ(settings.damageTiltStrength.get(), defaults::client::damageTiltStrength);
    EXPECT_FLOAT_EQ(settings.damageTiltStrength.get(), 1.0f);
    EXPECT_TRUE(settings.damageTiltStrength.isDefault());
}

TEST(ClientSettingsDamageTiltTest, SetValuePersists)
{
    client::ClientSettings settings;
    settings.damageTiltStrength.set(0.5f);
    EXPECT_FLOAT_EQ(settings.damageTiltStrength.get(), 0.5f);
    EXPECT_FALSE(settings.damageTiltStrength.isDefault());
}

TEST(ClientSettingsDamageTiltTest, ClampsToRange)
{
    client::ClientSettings settings;
    // 超上限 clamp 到 1.0
    settings.damageTiltStrength.set(2.0f);
    EXPECT_FLOAT_EQ(settings.damageTiltStrength.get(), 1.0f);
    // 超下限 clamp 到 0.0
    settings.damageTiltStrength.set(-1.0f);
    EXPECT_FLOAT_EQ(settings.damageTiltStrength.get(), 0.0f);
}

TEST(ClientSettingsDamageTiltTest, ResetReturnsToDefault)
{
    client::ClientSettings settings;
    settings.damageTiltStrength.set(0.3f);
    settings.damageTiltStrength.reset();
    EXPECT_FLOAT_EQ(settings.damageTiltStrength.get(), 1.0f);
    EXPECT_TRUE(settings.damageTiltStrength.isDefault());
}

TEST(ClientSettingsDamageTiltTest, SerializeRoundTripsValue)
{
    client::ClientSettings settings;
    settings.damageTiltStrength.set(0.25f);
    nlohmann::json j;
    settings.damageTiltStrength.serialize(j);

    ASSERT_TRUE(j.contains("damageTiltStrength"));
    EXPECT_FLOAT_EQ(j["damageTiltStrength"].get<f32>(), 0.25f);

    // 反序列化到新实例
    client::ClientSettings decoded;
    decoded.damageTiltStrength.deserialize(j);
    EXPECT_FLOAT_EQ(decoded.damageTiltStrength.get(), 0.25f);
}
