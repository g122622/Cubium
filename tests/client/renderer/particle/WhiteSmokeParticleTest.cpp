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

#include "client/renderer/trident/particle/particles/effect/WhiteSmokeParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/WorldEvents.hpp"
#include <glm/glm.hpp>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world;
using namespace mc::client::renderer::trident::particle;
using namespace mc::client::renderer::trident::particle::particles;

// ============================================================================
// WhiteSmokeParticle 测试
// ============================================================================

/**
 * @brief 测试白色烟雾粒子构造
 */
TEST(WhiteSmokeParticleTest, Construction)
{
    WhiteSmokeParticle particle(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.1f, 0.2f, 0.3f));

    EXPECT_TRUE(particle.isAlive());
    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    EXPECT_EQ(particle.getTextureLocation(), mc::ResourceLocation("minecraft:particle/smoke"));

    // 白灰色调 (R=0.729, G=0.694, B=0.761)
    EXPECT_NEAR(particle.color().r, 0.729f, 0.01f);
    EXPECT_NEAR(particle.color().g, 0.694f, 0.01f);
    EXPECT_NEAR(particle.color().b, 0.761f, 0.01f);
    EXPECT_FLOAT_EQ(particle.color().a, 1.0f);

    // 重力为负值（向上漂移）
    EXPECT_DOUBLE_EQ(particle.gravity(), -0.1);

    // 摩擦力 0.96
    EXPECT_DOUBLE_EQ(particle.friction(), 0.96);

    // 启用碰撞检测
    EXPECT_TRUE(particle.hasPhysics());
}

/**
 * @brief 测试白色烟雾粒子工厂方法
 */
TEST(WhiteSmokeParticleTest, CreateFactory)
{
    auto particle = WhiteSmokeParticle::create(glm::vec3(5.0f, 10.0f, 15.0f), glm::vec3(0.0f, 0.1f, 0.0f), nullptr);
    ASSERT_NE(particle, nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    EXPECT_EQ(particle->getTextureLocation(), mc::ResourceLocation("minecraft:particle/smoke"));
    EXPECT_TRUE(particle->isAlive());

    EXPECT_FLOAT_EQ(particle->position().x, 5.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 10.0f);
    EXPECT_FLOAT_EQ(particle->position().z, 15.0f);
}

/**
 * @brief 测试白色烟雾粒子 tick 行为
 */
TEST(WhiteSmokeParticleTest, TickMovement)
{
    glm::vec3 initialPos(0.0f, 10.0f, 0.0f);
    glm::vec3 initialVel(0.1f, 0.0f, 0.1f);
    WhiteSmokeParticle particle(initialPos, initialVel);

    // Tick 一次
    particle.tick(nullptr);

    // 由于重力为负值 (-0.1)，Y 方向速度应该增加（向上漂移）
    // m_velocity.y -= m_gravity => m_velocity.y -= (-0.1) => m_velocity.y += 0.1
    EXPECT_GT(particle.velocity().y, 0.0f);

    // 水平位置应该随速度移动（加上随机漂移）
    // 摩擦力使速度衰减
}

/**
 * @brief 测试白色烟雾粒子淡出效果
 */
TEST(WhiteSmokeParticleTest, FadeOut)
{
    WhiteSmokeParticle particle(glm::vec3(0.0f), glm::vec3(0.0f));

    f32 initialAlpha = particle.color().a;

    // 在生命周期前段，alpha 应该逐渐减小
    for (int i = 0; i < 5; ++i) {
        particle.tick(nullptr);
    }

    // alpha 应该减小
    EXPECT_LT(particle.color().a, initialAlpha);
}

/**
 * @brief 测试白色烟雾粒子生命周期结束
 */
TEST(WhiteSmokeParticleTest, LifecycleEnd)
{
    WhiteSmokeParticle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setMaxAge(5.0f);

    EXPECT_TRUE(particle.isAlive());

    // Tick 到生命结束
    for (int i = 0; i < 20; ++i) {
        particle.tick(nullptr);
    }

    EXPECT_FALSE(particle.isAlive());
}

/**
 * @brief 测试白色烟雾粒子尺寸增长
 */
TEST(WhiteSmokeParticleTest, SizeGrowth)
{
    WhiteSmokeParticle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setMaxAge(20.0f);

    f64 initialSize = particle.size();

    // 多次 tick
    for (int i = 0; i < 10; ++i) {
        particle.tick(nullptr);
    }

    // 尺寸应该随年龄增大
    EXPECT_GT(particle.size(), initialSize);
}

// ============================================================================
// WhiteSmoke 粒子类型注册测试
// ============================================================================

/**
 * @brief 测试 WhiteSmoke 粒子类型在注册表中已注册
 */
TEST(WhiteSmokeParticleTest, RegistryRegistration)
{
    ParticleRegistry& registry = ParticleRegistry::instance();

    // 通过 ID 检查
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::WhiteSmoke));

    // 通过名称检查
    EXPECT_TRUE(registry.isRegistered("minecraft:white_smoke"));

    // 检查名称映射
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::WhiteSmoke), "minecraft:white_smoke");

    // 通过名称查找 ID
    auto id = registry.getTypeId("minecraft:white_smoke");
    EXPECT_TRUE(id.has_value());
    EXPECT_EQ(id.value(), ParticleTypeId::WhiteSmoke);

    // 检查类型信息
    const ParticleTypeInfo* info = registry.getTypeInfo(ParticleTypeId::WhiteSmoke);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->id, ParticleTypeId::WhiteSmoke);
    EXPECT_EQ(info->name, "minecraft:white_smoke");
    EXPECT_EQ(info->defaultRenderType, ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

// ============================================================================
// WorldEvents 常量测试
// ============================================================================

/**
 * @brief 测试烟雾相关世界事件常量
 */
TEST(WhiteSmokeParticleTest, WorldEventConstants)
{
    // SHOOT_WHITE_SMOKE 事件 ID 应为 2010
    EXPECT_EQ(WorldEvents::SHOOT_WHITE_SMOKE, 2010);

    // DISPENSER_SMOKE 事件 ID 应为 2000
    EXPECT_EQ(WorldEvents::DISPENSER_SMOKE, 2000);
}

// ============================================================================
// Direction 工具函数测试（验证方向性烟雾事件所需的 Direction 解析）
// ============================================================================

/**
 * @brief 测试方向枚举值与 WorldEvents data 参数的对应
 */
TEST(WhiteSmokeParticleTest, DirectionOffsets)
{
    // 验证 6 个方向偏移量正确（用于方向性烟雾粒子）
    EXPECT_EQ(Directions::xOffset(Direction::Down), 0);
    EXPECT_EQ(Directions::yOffset(Direction::Down), -1);
    EXPECT_EQ(Directions::zOffset(Direction::Down), 0);

    EXPECT_EQ(Directions::xOffset(Direction::Up), 0);
    EXPECT_EQ(Directions::yOffset(Direction::Up), 1);
    EXPECT_EQ(Directions::zOffset(Direction::Up), 0);

    EXPECT_EQ(Directions::xOffset(Direction::North), 0);
    EXPECT_EQ(Directions::yOffset(Direction::North), 0);
    EXPECT_EQ(Directions::zOffset(Direction::North), -1);

    EXPECT_EQ(Directions::xOffset(Direction::South), 0);
    EXPECT_EQ(Directions::yOffset(Direction::South), 0);
    EXPECT_EQ(Directions::zOffset(Direction::South), 1);

    EXPECT_EQ(Directions::xOffset(Direction::West), -1);
    EXPECT_EQ(Directions::yOffset(Direction::West), 0);
    EXPECT_EQ(Directions::zOffset(Direction::West), 0);

    EXPECT_EQ(Directions::xOffset(Direction::East), 1);
    EXPECT_EQ(Directions::yOffset(Direction::East), 0);
    EXPECT_EQ(Directions::zOffset(Direction::East), 0);
}

/**
 * @brief 测试 Direction 枚举值作为 data 参数的正确解析
 */
TEST(WhiteSmokeParticleTest, DirectionFromEventData)
{
    // 模拟 WorldEvents::data 参数到 Direction 的转换
    // data = 0 => Down, 1 => Up, 2 => North, 3 => South, 4 => West, 5 => East
    i32 dataDown = 0;
    Direction dirDown = static_cast<Direction>(dataDown);
    EXPECT_EQ(Directions::yOffset(dirDown), -1);

    i32 dataUp = 1;
    Direction dirUp = static_cast<Direction>(dataUp);
    EXPECT_EQ(Directions::yOffset(dirUp), 1);

    i32 dataNorth = 2;
    Direction dirNorth = static_cast<Direction>(dataNorth);
    EXPECT_EQ(Directions::zOffset(dirNorth), -1);

    i32 dataSouth = 3;
    Direction dirSouth = static_cast<Direction>(dataSouth);
    EXPECT_EQ(Directions::zOffset(dirSouth), 1);

    i32 dataWest = 4;
    Direction dirWest = static_cast<Direction>(dataWest);
    EXPECT_EQ(Directions::xOffset(dirWest), -1);

    i32 dataEast = 5;
    Direction dirEast = static_cast<Direction>(dataEast);
    EXPECT_EQ(Directions::xOffset(dirEast), 1);
}

/**
 * @brief 测试方向性粒子发射速度计算
 */
TEST(WhiteSmokeParticleTest, DirectionalVelocityCalculation)
{
    // 验证方向性烟雾的速度方向正确
    // 粒子速度 = direction_step * speed + gaussian_noise
    // 对于向上的方向 (Up)，Y 分量应为正
    {
        Direction dir = Direction::Up;
        f32 speed = 0.1f;
        f32 svx = static_cast<f32>(Directions::xOffset(dir)) * speed;
        f32 svy = static_cast<f32>(Directions::yOffset(dir)) * speed;
        f32 svz = static_cast<f32>(Directions::zOffset(dir)) * speed;
        EXPECT_FLOAT_EQ(svx, 0.0f);
        EXPECT_FLOAT_EQ(svy, speed);
        EXPECT_FLOAT_EQ(svz, 0.0f);
    }

    // 对于东方 (East)，X 分量应为正
    {
        Direction dir = Direction::East;
        f32 speed = 0.15f;
        f32 svx = static_cast<f32>(Directions::xOffset(dir)) * speed;
        f32 svy = static_cast<f32>(Directions::yOffset(dir)) * speed;
        f32 svz = static_cast<f32>(Directions::zOffset(dir)) * speed;
        EXPECT_FLOAT_EQ(svx, speed);
        EXPECT_FLOAT_EQ(svy, 0.0f);
        EXPECT_FLOAT_EQ(svz, 0.0f);
    }

    // 对于北方 (North)，Z 分量应为负
    {
        Direction dir = Direction::North;
        f32 speed = 0.12f;
        f32 svx = static_cast<f32>(Directions::xOffset(dir)) * speed;
        f32 svy = static_cast<f32>(Directions::yOffset(dir)) * speed;
        f32 svz = static_cast<f32>(Directions::zOffset(dir)) * speed;
        EXPECT_FLOAT_EQ(svx, 0.0f);
        EXPECT_FLOAT_EQ(svy, 0.0f);
        EXPECT_FLOAT_EQ(svz, -speed);
    }
}
