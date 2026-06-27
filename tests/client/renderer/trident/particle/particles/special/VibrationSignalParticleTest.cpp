/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/particles/special/VibrationSignalParticle.hpp"

#include <cmath>
#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

class VibrationSignalParticleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        startPos = glm::vec3(0.0f, 0.0f, 0.0f);
        targetPos = Vector3d(10.0, 0.0, 0.0);
        arrivalTicks = 10;
    }

    glm::vec3 startPos;
    Vector3d targetPos;
    i32 arrivalTicks;
};

// ==================== 工厂方法测试 ====================

TEST_F(VibrationSignalParticleTest, CreateWithTarget_ReturnsValidParticle)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST_F(VibrationSignalParticleTest, CreateWithTarget_SetsPosition)
{
    glm::vec3 pos(10.0f, 64.0f, -20.0f);
    Vector3d target(100.0, 70.0, -50.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 15);

    EXPECT_FLOAT_EQ(particle->position().x, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 64.0f);
    EXPECT_FLOAT_EQ(particle->position().z, -20.0f);
}

TEST_F(VibrationSignalParticleTest, Create_ReturnsValidParticle)
{
    glm::vec3 pos(5.0f, 10.0f, 15.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = VibrationSignalParticle::create(pos, velocity, nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
    // 默认工厂：目标为正上方 8 格，8 tick 到达
    EXPECT_FLOAT_EQ(particle->position().x, 5.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().z, 15.0f);
}

TEST_F(VibrationSignalParticleTest, Create_SetsMaxAge)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = VibrationSignalParticle::create(pos, velocity, nullptr);

    // 默认工厂设置 maxAge = 8
    EXPECT_DOUBLE_EQ(particle->maxAge(), 8.0);
}

// ==================== 渲染属性测试 ====================

TEST_F(VibrationSignalParticleTest, GetRenderType_ReturnsParticleSheetLit)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    // 振动信号粒子是发光粒子，使用 PARTICLE_SHEET_LIT
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST_F(VibrationSignalParticleTest, GetTextureLocation_ReturnsVibration)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    ResourceLocation texture = particle->getTextureLocation();
    EXPECT_EQ(texture.toString(), "minecraft:particle/vibration");
}

TEST_F(VibrationSignalParticleTest, GetLightColor_ReturnsMaxBrightness)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    // 发光粒子返回固定高亮度 15728880 (blockLight=15, skyLight=15)
    u32 lightColor = particle->getLightColor(nullptr);
    EXPECT_EQ(lightColor, 15728880u);
}

// ==================== 物理属性测试 ====================

TEST_F(VibrationSignalParticleTest, HasNoGravity)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    // 振动信号粒子无重力
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

TEST_F(VibrationSignalParticleTest, HasNoPhysics)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    // 振动信号粒子无碰撞检测
    EXPECT_FALSE(particle->hasPhysics());
}

TEST_F(VibrationSignalParticleTest, HasFrictionOne)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    // 振动信号粒子摩擦力为 1.0（无速度衰减，因为运动是目标驱动的）
    EXPECT_FLOAT_EQ(particle->friction(), 1.0f);
}

// ==================== 颜色测试 ====================

TEST_F(VibrationSignalParticleTest, Color_IsPaleBlueInitially)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    const glm::vec4& color = particle->color();

    // 初始颜色: 淡蓝色发光 (0.75, 0.85, 1.0, 1.0)
    EXPECT_FLOAT_EQ(color.r, 0.75f);
    EXPECT_FLOAT_EQ(color.g, 0.85f);
    EXPECT_FLOAT_EQ(color.b, 1.0f);
    EXPECT_FLOAT_EQ(color.a, 1.0f);
}

// ==================== Tick 插值行为测试 ====================

TEST_F(VibrationSignalParticleTest, Tick_MovesTowardTarget)
{
    // 粒子从 (0,0,0) 飞向 (10,0,0)，10 tick 到达
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    f32 initialX = particle->position().x;

    particle->tick(nullptr);

    // 每次tick后位置应该向目标方向移动
    // MC 原版插值: pos = lerp(1/remainingTicks, pos, target)
    // 第 1 tick 后: remainingTicks = 10-1 = 9, lerpFactor = 1/9
    // pos.x = lerp(1/9, 0, 10) = 10/9 ≈ 1.111
    EXPECT_GT(particle->position().x, initialX);
    EXPECT_LT(particle->position().x, 10.0f); // 不应超过目标
}

TEST_F(VibrationSignalParticleTest, Tick_ExponentialEasing)
{
    // 验证指数缓动：越接近目标，步长越小
    // 使用较大距离和较多 tick 使缓动效果更明显
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(1000.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 50);

    f32 prevX = particle->position().x;
    f64 firstStep = 0.0;
    f64 lateStep = 0.0;

    // 第 1 tick
    particle->tick(nullptr);
    firstStep = static_cast<f64>(particle->position().x) - static_cast<f64>(prevX);

    // tick 到后期（接近目标时步长更小）
    for (int i = 0; i < 30; ++i) {
        particle->tick(nullptr);
    }
    prevX = particle->position().x;
    particle->tick(nullptr);
    lateStep = static_cast<f64>(particle->position().x) - static_cast<f64>(prevX);

    // 指数缓动：后期步骤应比前期步骤更短
    EXPECT_LT(lateStep, firstStep);
}

TEST_F(VibrationSignalParticleTest, Tick_ApproachesTargetOverTime)
{
    // 粒子应在生命周期内逐渐接近目标位置
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(100.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 50);

    f32 lastX = particle->position().x;

    // tick 10 次
    for (int i = 0; i < 10; ++i) {
        particle->tick(nullptr);
    }

    // 10 tick 后应已移动了显著距离
    EXPECT_GT(particle->position().x, lastX);

    // 继续移动
    lastX = particle->position().x;
    for (int i = 0; i < 20; ++i) {
        particle->tick(nullptr);
    }

    // 30 tick 后更接近目标
    EXPECT_GT(particle->position().x, lastX);
}

TEST_F(VibrationSignalParticleTest, Tick_DoesNotOvershootTarget)
{
    // 粒子不应超过目标位置（指数缓动特性）
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(5.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 20);

    // tick 直到接近过期但不过期
    for (int i = 0; i < 19; ++i) {
        particle->tick(nullptr);
    }

    // 粒子不应超过目标位置（指数缓动永远不会到达目标）
    EXPECT_LE(particle->position().x, 5.0f);
}

TEST_F(VibrationSignalParticleTest, Tick_UpdatesAllAxes)
{
    // 粒子在三个轴上同时移动
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 20.0, 30.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 15);

    particle->tick(nullptr);

    // 所有三个轴都应向目标方向移动
    EXPECT_GT(particle->position().x, 0.0f);
    EXPECT_GT(particle->position().y, 0.0f);
    EXPECT_GT(particle->position().z, 0.0f);
}

TEST_F(VibrationSignalParticleTest, Tick_NegativeDirection)
{
    // 粒子向负方向移动
    glm::vec3 pos(10.0f, 20.0f, 30.0f);
    Vector3d target(0.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 15);

    particle->tick(nullptr);

    // 位置应向目标（更小的值）方向移动
    EXPECT_LT(particle->position().x, 10.0f);
    EXPECT_LT(particle->position().y, 20.0f);
    EXPECT_LT(particle->position().z, 30.0f);
}

// ==================== 生命周期测试 ====================

TEST_F(VibrationSignalParticleTest, Tick_IncreasesAge)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    f64 initialAge = particle->age();

    particle->tick(nullptr);

    EXPECT_GT(particle->age(), initialAge);
}

TEST_F(VibrationSignalParticleTest, Tick_ExpiresAfterArrivalTicks)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    // tick 直到过期
    for (int i = 0; i < arrivalTicks; ++i) {
        particle->tick(nullptr);
    }

    EXPECT_FALSE(particle->isAlive());
}

TEST_F(VibrationSignalParticleTest, Tick_DoesNotExpireBeforeArrivalTicks)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    // tick (arrivalTicks - 1) 次
    for (int i = 0; i < arrivalTicks - 1; ++i) {
        particle->tick(nullptr);
    }

    // 还不应过期
    EXPECT_TRUE(particle->isAlive());
}

TEST_F(VibrationSignalParticleTest, Tick_SingleTickArrival)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 1);

    // 1 tick 到达，tick 一次后应过期
    particle->tick(nullptr);

    EXPECT_FALSE(particle->isAlive());
}

TEST_F(VibrationSignalParticleTest, Tick_LongLifetime)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 100);

    // tick 99 次后仍应存活
    for (int i = 0; i < 99; ++i) {
        particle->tick(nullptr);
    }
    EXPECT_TRUE(particle->isAlive());

    // 第 100 次后应过期
    particle->tick(nullptr);
    EXPECT_FALSE(particle->isAlive());
}

// ==================== 旋转测试 ====================

TEST_F(VibrationSignalParticleTest, Tick_Rotates)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    f64 initialRoll = particle->roll();

    particle->tick(nullptr);

    // 每次tick旋转增加 0.05
    EXPECT_GT(particle->roll(), initialRoll);
}

TEST_F(VibrationSignalParticleTest, Tick_RollAccumulates)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, 20);

    f64 initialRoll = particle->roll();

    for (int i = 0; i < 5; ++i) {
        particle->tick(nullptr);
    }

    // 5 次 tick 后旋转应累积
    EXPECT_NEAR(particle->roll(), initialRoll + 5 * 0.05, 0.001);
}

// ==================== Alpha 淡出测试 ====================

TEST_F(VibrationSignalParticleTest, Tick_FadesOutInLatePhase)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 10);

    // 初始 alpha 为 1.0
    EXPECT_FLOAT_EQ(particle->color().a, 1.0f);

    // tick 到后期阶段 (ageRatio > 0.7)
    // 对于 10 tick 的粒子，0.7 * 10 = 7 tick 后开始淡出
    for (int i = 0; i < 8; ++i) {
        particle->tick(nullptr);
    }

    // 后期 alpha 应开始淡出
    EXPECT_LT(particle->color().a, 1.0f);
}

TEST_F(VibrationSignalParticleTest, Tick_NoFadeInEarlyPhase)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 20);

    // tick 前期 (ageRatio < 0.7)
    for (int i = 0; i < 10; ++i) {
        particle->tick(nullptr);
    }

    // 前期 alpha 应保持 1.0 (10/20 = 0.5, 还没到 0.7)
    EXPECT_FLOAT_EQ(particle->color().a, 1.0f);
}

// ==================== 缩放测试 ====================

TEST_F(VibrationSignalParticleTest, GetScale_GrowInPhase)
{
    // ageRatio < 0.1: scale 从 0.5 增长到 1.0
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, 20);

    // 初始 ageRatio ≈ 0, scale ≈ 0.5
    f64 scale = particle->getScale(0.0);
    EXPECT_NEAR(scale, 0.5, 0.01);
}

TEST_F(VibrationSignalParticleTest, GetScale_SteadyPhase)
{
    // 0.1 <= ageRatio < 0.8: scale = 1.0
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, 20);

    // tick 到 ageRatio ≈ 0.25 (5/20)
    for (int i = 0; i < 5; ++i) {
        particle->tick(nullptr);
    }

    f64 scale = particle->getScale(0.0);
    EXPECT_NEAR(scale, 1.0, 0.05);
}

TEST_F(VibrationSignalParticleTest, GetScale_ShrinkOutPhase)
{
    // ageRatio >= 0.8: scale 从 1.0 缩小到 0.5
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    Vector3d target(10.0, 0.0, 0.0);

    auto particle = VibrationSignalParticle::createWithTarget(pos, target, 10);

    // tick 到 ageRatio ≈ 0.9 (9/10)
    for (int i = 0; i < 9; ++i) {
        particle->tick(nullptr);
    }

    f64 scale = particle->getScale(0.0);
    // ageRatio = 9/10 = 0.9, 在 [0.8, 1.0) 区间
    // scale = 1.0 - (0.9 - 0.8) / 0.2 * 0.5 = 1.0 - 0.25 = 0.75
    EXPECT_NEAR(scale, 0.75, 0.05);
    EXPECT_LT(scale, 1.0);
    EXPECT_GT(scale, 0.5);
}

// ==================== prevPosition 更新测试 ====================

TEST_F(VibrationSignalParticleTest, Tick_UpdatesPreviousPosition)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    glm::vec3 posBeforeTick = particle->position();

    particle->tick(nullptr);

    // prevPosition 应为 tick 前的 position
    EXPECT_FLOAT_EQ(particle->prevPosition().x, posBeforeTick.x);
    EXPECT_FLOAT_EQ(particle->prevPosition().y, posBeforeTick.y);
    EXPECT_FLOAT_EQ(particle->prevPosition().z, posBeforeTick.z);
}

// ==================== 零速度测试 ====================

TEST_F(VibrationSignalParticleTest, ZeroVelocity)
{
    auto particle = VibrationSignalParticle::createWithTarget(startPos, targetPos, arrivalTicks);

    // 振动信号粒子速度为零（运动由目标位置驱动）
    EXPECT_FLOAT_EQ(particle->velocity().x, 0.0f);
    EXPECT_FLOAT_EQ(particle->velocity().y, 0.0f);
    EXPECT_FLOAT_EQ(particle->velocity().z, 0.0f);
}

} // namespace
} // namespace mc
