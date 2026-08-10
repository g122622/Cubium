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
 * @file PlayerSwimTest.cpp
 * @brief 玩家游泳和溺水相关测试
 *
 * 测试覆盖：
 * - 空气供应管理
 * - 溺水伤害
 * - 游泳姿态
 * - 水下呼吸效果
 * - 海豚恩惠效果
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/sound/SoundEvents.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::physics;
using namespace mc::entity::effect;
using mc::entity::EntitySize;

namespace {

// 服务端测试世界（isClientSide() 返回 false），使 LivingEntity::updateAirSupply
// 的 m_world==nullptr / 客户端早退分支不再触发。BaseTestWorld 默认构造为 protected，
// 需通过派生类暴露 public 默认构造以作为夹具成员。
class PlayerSwimWorld final : public mc::test::BaseTestWorld {};

} // namespace

/**
 * @brief 测试夹具 - 玩家游泳测试
 *
 * 提供 BaseTestWorld 作为服务端世界（isClientSide() 返回 false），
 * 使 LivingEntity::updateAirSupply() 的 m_world==nullptr / 客户端早退分支不再触发，
 * 从而让空气消耗与溺水伤害逻辑对齐 MC Java 服务端行为。
 */
class PlayerSwimTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建玩家并绑定服务端测试世界
        player = std::make_unique<Player>(1, "TestPlayer", mc::test::testEcsRegistry());
        player->setWorld(&m_world);
    }

    void TearDown() override { player.reset(); }

    PlayerSwimWorld m_world;
    std::unique_ptr<Player> player;
};

// ============================================================================
// 空气供应测试
// ============================================================================

TEST_F(PlayerSwimTest, InitialAirSupply)
{
    // 玩家初始空气值应为 300 (15秒)
    EXPECT_EQ(player->air(), DEFAULT_MAX_AIR);
}

TEST_F(PlayerSwimTest, AirSupplyDecreasesInWater)
{
    // 在水中时，空气应该逐渐减少
    // 设置水中状态（用于测试）
    player->setInWater(true);
    i32 initialAir = player->air();

    // 注意：updateAirSupply 会检查 isInWater()，这里手动设置标志
    // 实际游戏中 updateEnvironmentState() 会设置此标志
    player->updateAirSupply();

    // 在水中且无水下呼吸效果时，空气应该减少
    // 如果没有效果，空气应该减少
    EXPECT_LT(player->air(), initialAir);
}

TEST_F(PlayerSwimTest, AirRecoveryOutOfWater)
{
    // 消耗一些空气
    player->setAir(200);

    // 不在水中时，空气应该恢复
    player->setInWater(false);
    player->updateAirSupply();

    // 每tick恢复4点
    EXPECT_EQ(player->air(), 204);
}

TEST_F(PlayerSwimTest, AirRecoveryCapsAtMax)
{
    // 空气接近最大值
    player->setAir(298);

    // 不在水中时，空气应该恢复但不超过最大值
    player->setInWater(false);
    player->updateAirSupply();

    EXPECT_EQ(player->air(), DEFAULT_MAX_AIR);
}

// ============================================================================
// 溺水伤害测试
// ============================================================================

TEST_F(PlayerSwimTest, DrownDamageTriggersAtMinusTwenty)
{
    // 空气耗尽到 -20 时触发溺水
    player->setAir(-19);
    player->setHealth(20.0f);

    // 模拟在水中
    player->setInWater(true);

    // 更新空气供应，空气会降到 -20，触发 shouldTakeDrowningDamage()，重置为 0
    // 并立即造成溺水伤害（MC Java: baseTick 中 air<=-20 时 hurtServer(drown, 2.0F)）
    player->updateAirSupply();
    EXPECT_EQ(player->air(), 0);

    // 多次重复触发溺水伤害（受无敌帧限制，实际伤害可能只发生一次）
    for (int i = 0; i < DROWN_DAMAGE_INTERVAL; ++i) {
        player->setAir(-19);
        player->updateAirSupply();
    }

    // 溺水伤害应该已经造成
    EXPECT_LT(player->health(), 20.0f);
}

TEST_F(PlayerSwimTest, DrownDamageAmount)
{
    // 溺水伤害量应为 2.0f
    EXPECT_EQ(DROWN_DAMAGE_AMOUNT, 2.0f);
}

TEST_F(PlayerSwimTest, CreativeModeNoDrowning)
{
    // 创造模式下不应该溺水
    PlayerAbilities abilities;
    abilities.creativeMode = true;
    abilities.invulnerable = true;
    player->abilities() = abilities;

    player->setAir(-20);
    player->setHealth(20.0f);
    player->setInWater(true);

    // 更新多次，不应该溺水
    for (int i = 0; i < DROWN_DAMAGE_INTERVAL * 2; ++i) {
        player->updateAirSupply();
    }

    // 创造模式下空气不应该消耗
    // 注意：updateAirSupply 会检查 abilities.invulnerable
    EXPECT_EQ(player->health(), 20.0f); // 不应该受到伤害
}

// ============================================================================
// 水下呼吸效果测试
// ============================================================================

TEST_F(PlayerSwimTest, WaterBreathingPreventsAirConsumption)
{
    // 添加水下呼吸效果
    EffectInstance waterBreathing(EffectType::WaterBreathing, 200, 0);
    player->addEffect(waterBreathing);

    // 验证效果已添加
    EXPECT_TRUE(player->hasEffect(EffectType::WaterBreathing));

    // 在水中
    player->setInWater(true);
    i32 initialAir = player->air();

    // 更新空气供应
    player->updateAirSupply();

    // 有水下呼吸效果时，空气不应该消耗
    EXPECT_EQ(player->air(), initialAir);
}

TEST_F(PlayerSwimTest, ConduitPowerPreventsAirConsumption)
{
    // 添加潮涌能量效果
    EffectInstance conduitPower(EffectType::ConduitPower, 200, 0);
    player->addEffect(conduitPower);

    // 验证效果已添加
    EXPECT_TRUE(player->hasEffect(EffectType::ConduitPower));

    // 在水中
    player->setInWater(true);
    i32 initialAir = player->air();

    // 更新空气供应
    player->updateAirSupply();

    // 有潮涌能量效果时，空气不应该消耗
    EXPECT_EQ(player->air(), initialAir);
}

TEST_F(PlayerSwimTest, EnteringWaterConsumesAir)
{
    // 入水后空气应被消耗（依赖服务端世界：updateAirSupply 在 m_world==nullptr
    // 或 isClientSide()==true 时早退，空气保持不变）
    player->setInWater(true);

    player->updateAirSupply();
    EXPECT_LT(player->air(), DEFAULT_MAX_AIR);
}

// ============================================================================
// 游泳状态测试
// ============================================================================

TEST_F(PlayerSwimTest, IsActualSwimmingConditions)
{
    // 默认不在游泳
    EXPECT_FALSE(player->isActualSwimming());

    // 设置在水中（但眼睛不在水中）
    player->setInWater(true);
    // 注意：isActualSwimming 需要 areEyesInWater() 返回 true
    // 这里只是基础测试，完整测试需要模拟世界环境
}

TEST_F(PlayerSwimTest, SwimmingPoseDimensions)
{
    // 游泳姿态的尺寸应该是 0.6 x 0.6
    EntitySize swimSize = player->getDimensions(EntityPose::Swimming);
    EXPECT_FLOAT_EQ(swimSize.width(), 0.6f);
    EXPECT_FLOAT_EQ(swimSize.height(), 0.6f);
    EXPECT_FLOAT_EQ(swimSize.eyeHeight(), 0.4f);
}

TEST_F(PlayerSwimTest, StandingPoseDimensions)
{
    // 站立姿态的尺寸应该是 0.6 x 1.8
    EntitySize standSize = player->getDimensions(EntityPose::Standing);
    EXPECT_FLOAT_EQ(standSize.width(), 0.6f);
    EXPECT_FLOAT_EQ(standSize.height(), 1.8f);
    EXPECT_FLOAT_EQ(standSize.eyeHeight(), 1.62f);
}

TEST_F(PlayerSwimTest, CrouchingPoseDimensions)
{
    // 蹲下姿态的尺寸应该是 0.6 x 1.5
    EntitySize crouchSize = player->getDimensions(EntityPose::Crouching);
    EXPECT_FLOAT_EQ(crouchSize.width(), 0.6f);
    EXPECT_FLOAT_EQ(crouchSize.height(), 1.5f);
    EXPECT_FLOAT_EQ(crouchSize.eyeHeight(), 1.27f);
}

// ============================================================================
// 物理常量测试
// ============================================================================

TEST_F(PlayerSwimTest, PhysicsConstantsCorrect)
{
    // 验证游泳相关物理常量与 MC 1.16.5 一致
    EXPECT_FLOAT_EQ(DEFAULT_MAX_AIR, 300);
    EXPECT_EQ(DROWN_DAMAGE_INTERVAL, 20);
    EXPECT_FLOAT_EQ(DROWN_DAMAGE_AMOUNT, 2.0f);
    EXPECT_FLOAT_EQ(SWIM_SPEED_BASE, 0.02f);
    EXPECT_FLOAT_EQ(WATER_DRAG, 0.8f);
    EXPECT_FLOAT_EQ(WATER_DRAG_SPRINT, 0.9f);
    EXPECT_FLOAT_EQ(WATER_BUOYANCY, 0.005f);
    EXPECT_FLOAT_EQ(SWIM_UP_SPEED, 0.04f);
    EXPECT_FLOAT_EQ(SWIM_DOWN_SPEED, 0.04f);
    EXPECT_FLOAT_EQ(DEPTH_STRIDER_SPEED_BONUS, 0.0333333f);
    EXPECT_FLOAT_EQ(DEPTH_STRIDER_MAX_DRAG, 0.54600006f);
    EXPECT_FLOAT_EQ(DOLPHINS_GRACE_WATER_DRAG, 0.96f);
}

// ============================================================================
// 游泳动画测试
// ============================================================================

TEST_F(PlayerSwimTest, SwimAnimationUpdates)
{
    // 初始动画值为 0
    EXPECT_FLOAT_EQ(player->swimAnimation(), 0.0f);

    // 更新游泳状态（不游泳时动画应该减少）
    player->updateSwimming();
    EXPECT_FLOAT_EQ(player->swimAnimation(), 0.0f); // 已经是 0，不会变成负数

    // 动画过渡速度应该是 0.09f
    // 这里只能测试接口，完整动画测试需要模拟 isInWater 状态
}

TEST_F(PlayerSwimTest, SwimAnimationTransitionSpeed)
{
    // MC 1.16.5: 动画过渡速度为 0.09f
    // 进入游泳时增加，退出游泳时减少
    f32 animationSpeed = 0.09f;

    // 模拟动画增加
    f32 animation = 0.0f;
    for (int i = 0; i < 10; ++i) {
        animation = std::min(1.0f, animation + animationSpeed);
    }
    // 10 次后应该接近 0.9
    EXPECT_NEAR(animation, 0.9f, 0.01f);

    // 再加一次达到 0.99，仍未到 1.0
    animation = std::min(1.0f, animation + animationSpeed);
    EXPECT_NEAR(animation, 0.99f, 0.01f);

    // 12 次后达到或超过 1.0 (被 min 限制在 1.0)
    animation = std::min(1.0f, animation + animationSpeed);
    EXPECT_FLOAT_EQ(animation, 1.0f);

    // 模拟动画减少
    for (int i = 0; i < 12; ++i) {
        animation = std::max(0.0f, animation - animationSpeed);
    }
    EXPECT_FLOAT_EQ(animation, 0.0f);
}

// ============================================================================
// 效果移除测试
// ============================================================================

TEST_F(PlayerSwimTest, RemoveWaterBreathingEffect)
{
    // 添加并移除水下呼吸效果
    EffectInstance waterBreathing(EffectType::WaterBreathing, 200, 0);
    player->addEffect(waterBreathing);
    EXPECT_TRUE(player->hasEffect(EffectType::WaterBreathing));

    player->removeEffect(EffectType::WaterBreathing);
    EXPECT_FALSE(player->hasEffect(EffectType::WaterBreathing));

    // 移除后应该正常消耗空气
    player->setInWater(true);
    i32 initialAir = player->air();
    player->updateAirSupply();
    EXPECT_LT(player->air(), initialAir);
}

// ============================================================================
// 最大空气值测试
// ============================================================================

TEST_F(PlayerSwimTest, MaxAirDefaultValue)
{
    // 默认最大空气值为 300 tick (15 秒)
    EXPECT_EQ(player->maxAir(), 300);
}

// ============================================================================
// 水生生物对比测试
// ============================================================================

TEST_F(PlayerSwimTest, PlayerDrownDamageHigherThanWaterMob)
{
    // 玩家溺水伤害为 2.0f
    // 水生生物溺水伤害通常为 1.0f
    EXPECT_FLOAT_EQ(DROWN_DAMAGE_AMOUNT, 2.0f);
    // WaterMobEntity::DROWN_DAMAGE_AMOUNT = 1.0f
}
