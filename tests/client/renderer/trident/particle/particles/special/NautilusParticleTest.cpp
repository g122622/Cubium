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

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/particles/special/NautilusParticle.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>
#include <memory>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::particles;

class NautilusParticleTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ==================== 创建测试 ====================

TEST_F(NautilusParticleTest, Create_ReturnsValidParticle)
{
    glm::vec3 pos(10.0f, 64.0f, 20.0f);
    glm::vec3 velocity(1.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    EXPECT_NE(particle, nullptr);
    EXPECT_TRUE(particle->isAlive());
}

TEST_F(NautilusParticleTest, Create_SetsPosition)
{
    glm::vec3 pos(10.0f, 64.0f, 20.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    EXPECT_FLOAT_EQ(particle->position().x, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 64.0f);
    EXPECT_FLOAT_EQ(particle->position().z, 20.0f);
}

TEST_F(NautilusParticleTest, Create_SetsVelocity)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(2.0f, 1.0f, -1.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    EXPECT_FLOAT_EQ(particle->velocity().x, 2.0f);
    EXPECT_FLOAT_EQ(particle->velocity().y, 1.0f);
    EXPECT_FLOAT_EQ(particle->velocity().z, -1.0f);
}

// ==================== 渲染属性测试 ====================

TEST_F(NautilusParticleTest, GetRenderType_ReturnsParticleSheetLit)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // 鹦鹉螺粒子是发光粒子，使用 PARTICLE_SHEET_LIT 渲染类型
    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_LIT);
}

TEST_F(NautilusParticleTest, GetTextureLocation_ReturnsNautilus)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    ResourceLocation texture = particle->getTextureLocation();
    EXPECT_EQ(texture.toString(), "minecraft:particle/nautilus");
}

TEST_F(NautilusParticleTest, GetLightColor_ReturnsMaxBrightness)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // 发光粒子返回固定高亮度 15728880 (blockLight=15, skyLight=15)
    // 参考 MC 1.16.5 NautilusParticle
    u32 lightColor = particle->getLightColor(nullptr);
    EXPECT_EQ(lightColor, 15728880u);
}

// ==================== 物理属性测试 ====================

TEST_F(NautilusParticleTest, HasNoGravity)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // 鹦鹉螺粒子无重力
    EXPECT_DOUBLE_EQ(particle->gravity(), 0.0);
}

TEST_F(NautilusParticleTest, HasNoPhysics)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // 鹦鹉螺粒子无碰撞检测
    EXPECT_FALSE(particle->hasPhysics());
}

TEST_F(NautilusParticleTest, HasFriction)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // 鹦鹉螺粒子有摩擦力用于速度衰减
    EXPECT_NEAR(particle->friction(), 0.95, 0.001);
}

// ==================== 生命周期测试 ====================

TEST_F(NautilusParticleTest, Tick_UpdatesPosition)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // tick 后位置应该根据速度更新
    // 参考 MC 1.16.5: position += velocity * 0.1
    particle->tick(nullptr);

    // 位置应该移动了 velocity * 0.1
    EXPECT_FLOAT_EQ(particle->position().x, 1.0f); // 10.0 * 0.1 = 1.0
}

TEST_F(NautilusParticleTest, Tick_DecaysVelocity)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(10.0f, 5.0f, 2.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    particle->tick(nullptr);

    // 速度应该衰减 (velocity *= 0.95)
    EXPECT_FLOAT_EQ(particle->velocity().x, 10.0f * 0.95f);
    EXPECT_FLOAT_EQ(particle->velocity().y, 5.0f * 0.95f);
    EXPECT_FLOAT_EQ(particle->velocity().z, 2.0f * 0.95f);
}

TEST_F(NautilusParticleTest, Tick_IncreasesAge)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    f64 initialAge = particle->age();

    particle->tick(nullptr);

    EXPECT_GT(particle->age(), initialAge);
}

TEST_F(NautilusParticleTest, Tick_ExpiresAfterLifetime)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // 鹦鹉螺粒子生命周期约 60 tick
    // tick 多次直到过期
    for (int i = 0; i < 100; ++i) {
        particle->tick(nullptr);
    }

    EXPECT_FALSE(particle->isAlive());
}

// ==================== 缩放测试 ====================

TEST_F(NautilusParticleTest, GetScale_ReturnsValidScale)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // 初始阶段缩放应该有效
    f64 scale = particle->getScale(0.0);
    EXPECT_GT(scale, 0.0);
    EXPECT_LT(scale, 1.0); // 初始时较小
}

TEST_F(NautilusParticleTest, GetScale_MiddlePhase_ReturnsFullSize)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // tick 到中期阶段 (生命周期 30-70%)
    for (int i = 0; i < 40; ++i) {
        particle->tick(nullptr);
    }

    // 中期缩放应该接近初始大小
    f64 scale = particle->getScale(0.0);
    f64 initialSize = particle->size();
    EXPECT_NEAR(scale, initialSize, 0.1);
}

// ==================== 颜色测试 ====================

TEST_F(NautilusParticleTest, Color_IsWhiteOrLightBlue)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    const glm::vec4& color = particle->color();

    // 颜色应该是白色/淡蓝色色调
    // R, G 范围 0.8-1.0, B 固定为 1.0
    EXPECT_GE(color.r, 0.8f);
    EXPECT_LE(color.r, 1.0f);
    EXPECT_GE(color.g, 0.8f);
    EXPECT_LE(color.g, 1.0f);
    EXPECT_FLOAT_EQ(color.b, 1.0f);
    EXPECT_FLOAT_EQ(color.a, 1.0f); // 初始完全不透明
}

TEST_F(NautilusParticleTest, Tick_FadesOutInLatePhase)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    // tick 到后期阶段 (生命周期 > 50%)
    for (int i = 0; i < 50; ++i) {
        particle->tick(nullptr);
    }

    // 后期 alpha 应该开始淡出
    const glm::vec4& color = particle->color();
    EXPECT_LT(color.a, 1.0f);
}

// ==================== 旋转测试 ====================

TEST_F(NautilusParticleTest, Tick_Rotates)
{
    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 velocity(0.0f, 0.0f, 0.0f);

    auto particle = NautilusParticle::create(pos, velocity, nullptr);

    f64 initialRoll = particle->roll();

    particle->tick(nullptr);

    // 粒子应该旋转
    EXPECT_GT(particle->roll(), initialRoll);
}

} // namespace
} // namespace mc
