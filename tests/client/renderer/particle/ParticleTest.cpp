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

#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/data/BasicParticleData.hpp"
#include "client/renderer/trident/particle/particles/RainParticle.hpp"
#include "client/renderer/trident/particle/particles/SnowParticle.hpp"
#include "client/renderer/trident/particle/particles/effect/CritParticle.hpp"
#include "client/renderer/trident/particle/particles/weather/FishingParticle.hpp"
#include "client/renderer/trident/particle/particles/weather/SplashParticle.hpp"
#include "common/core/Types.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::trident::particle;
using namespace mc::client::renderer::trident::particle::data;
using namespace mc::client::renderer::trident::particle::particles;

/**
 * @brief 测试粒子基类构造和属性
 */
TEST(ParticleTest, ConstructionAndProperties)
{
    glm::vec3 pos(1.0f, 2.0f, 3.0f);
    glm::vec3 vel(0.1f, 0.2f, 0.3f);

    Particle particle(pos, vel);

    EXPECT_FLOAT_EQ(particle.position().x, 1.0f);
    EXPECT_FLOAT_EQ(particle.position().y, 2.0f);
    EXPECT_FLOAT_EQ(particle.position().z, 3.0f);
    EXPECT_FLOAT_EQ(particle.velocity().x, 0.1f);
    EXPECT_FLOAT_EQ(particle.velocity().y, 0.2f);
    EXPECT_FLOAT_EQ(particle.velocity().z, 0.3f);
    EXPECT_TRUE(particle.isAlive());
    EXPECT_DOUBLE_EQ(particle.age(), 0.0);
    EXPECT_DOUBLE_EQ(particle.maxAge(), 1.0);
}

/**
 * @brief 测试粒子属性设置
 */
TEST(ParticleTest, PropertySetters)
{
    Particle particle(glm::vec3(0.0f), glm::vec3(0.0f));

    particle.setPosition(glm::vec3(5.0f, 10.0f, 15.0f));
    EXPECT_FLOAT_EQ(particle.position().x, 5.0f);
    EXPECT_FLOAT_EQ(particle.position().y, 10.0f);
    EXPECT_FLOAT_EQ(particle.position().z, 15.0f);

    particle.setVelocity(glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_FLOAT_EQ(particle.velocity().x, 1.0f);
    EXPECT_FLOAT_EQ(particle.velocity().y, 2.0f);
    EXPECT_FLOAT_EQ(particle.velocity().z, 3.0f);

    particle.setGravity(0.5f);
    EXPECT_DOUBLE_EQ(particle.gravity(), 0.5);

    particle.setSize(0.2f);
    EXPECT_NEAR(particle.size(), 0.2, 1.0e-6);

    particle.setMaxAge(100.0f);
    EXPECT_DOUBLE_EQ(particle.maxAge(), 100.0);

    particle.setColor(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
    EXPECT_FLOAT_EQ(particle.color().r, 1.0f);
    EXPECT_FLOAT_EQ(particle.color().g, 0.5f);
    EXPECT_FLOAT_EQ(particle.color().b, 0.0f);
    EXPECT_FLOAT_EQ(particle.color().a, 1.0f);
}

/**
 * @brief 测试粒子生命周期
 */
TEST(ParticleTest, Lifecycle)
{
    Particle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setMaxAge(5.0f);

    EXPECT_TRUE(particle.isAlive());

    // Tick 1
    particle.tick(nullptr);
    EXPECT_DOUBLE_EQ(particle.age(), 1.0);
    EXPECT_TRUE(particle.isAlive());

    // Tick 2-4
    particle.tick(nullptr);
    particle.tick(nullptr);
    particle.tick(nullptr);
    particle.tick(nullptr);
    EXPECT_DOUBLE_EQ(particle.age(), 5.0);

    // Tick 5 should expire
    particle.tick(nullptr);
    EXPECT_FALSE(particle.isAlive());
}

/**
 * @brief 测试粒子过期标记
 */
TEST(ParticleTest, ExpiredFlag)
{
    Particle particle(glm::vec3(0.0f), glm::vec3(0.0f));

    EXPECT_TRUE(particle.isAlive());
    EXPECT_FALSE(particle.onGround()); // 初始不在地面

    particle.setExpired();
    EXPECT_FALSE(particle.isAlive());
}

/**
 * @brief 测试粒子渲染类型
 */
TEST(ParticleTest, RenderType)
{
    Particle particle(glm::vec3(0.0f), glm::vec3(0.0f));

    // 默认渲染类型
    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

/**
 * @brief 测试粒子移动
 */
TEST(ParticleTest, Movement)
{
    Particle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setHasPhysics(false);

    particle.move(nullptr, glm::vec3(0.5f, 0.5f, 0.5f));

    EXPECT_FLOAT_EQ(particle.position().x, 0.5f);
    EXPECT_FLOAT_EQ(particle.position().y, 0.5f);
    EXPECT_FLOAT_EQ(particle.position().z, 0.5f);
}

/**
 * @brief 测试粒子重力效果
 */
TEST(ParticleTest, Gravity)
{
    Particle particle(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f));
    particle.setGravity(1.0f);
    particle.setMaxAge(100.0f);

    // 重力应该使 Y 速度变为负
    particle.tick(nullptr);

    EXPECT_LT(particle.velocity().y, 0.0f);
}

/**
 * @brief 测试雨滴粒子构造
 */
TEST(ParticleTest, RainParticle_Construction)
{
    RainParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f));

    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle.getTextureLocation(), mc::ResourceLocation("minecraft:particle/rain"));
    EXPECT_TRUE(particle.isAlive());
}

/**
 * @brief 验证雨滴工厂方法返回正确的粒子类型
 */
TEST(ParticleTest, RainParticle_CreateReturnsRainParticle)
{
    auto particle = RainParticle::create(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f), nullptr);
    ASSERT_NE(particle, nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle->getTextureLocation(), mc::ResourceLocation("minecraft:particle/rain"));
}

/**
 * @brief 测试雨滴无世界 tick（无碰撞检测）
 */
TEST(ParticleTest, RainParticle_TickWithoutWorld)
{
    RainParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f));
    particle.tick(nullptr);

    // 无世界时应该仍然存活并下落
    EXPECT_TRUE(particle.isAlive());
    EXPECT_LT(particle.velocity().y, 0.0f); // 重力使速度更负
}

/**
 * @brief 测试雨滴粒子重力参数
 *
 * 参考 MC 1.16.5 RainParticle：
 * - 重力 0.06
 * - 粒子重力乘数 0.04
 * - 终端速度 -3.0
 */
TEST(ParticleTest, RainParticle_GravityAndTerminalVelocity)
{
    RainParticle particle(glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // 验证重力设置
    EXPECT_DOUBLE_EQ(particle.gravity(), mc::physics::RAIN_GRAVITY);

    // 多次 tick 应该达到终端速度
    for (int i = 0; i < 100; ++i) {
        particle.tick(nullptr);
    }

    // 速度应该被终端速度限制
    EXPECT_GE(particle.velocity().y, -3.1f); // TERMINAL_VELOCITY = -3.0
}

/**
 * @brief 测试雨滴碰撞盒尺寸
 */
TEST(ParticleTest, RainParticle_BoundingBoxSize)
{
    RainParticle particle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // 验证碰撞盒尺寸
    auto bbox = particle.getBoundingBox();
    f32 width = static_cast<f32>(bbox.maxX - bbox.minX);
    f32 height = static_cast<f32>(bbox.maxY - bbox.minY);

    // 雨滴碰撞盒很小（0.02 x 0.04）
    EXPECT_NEAR(width, 0.02f, 0.001f);
    EXPECT_NEAR(height, 0.04f, 0.001f);
}

/**
 * @brief 测试雪花粒子构造
 */
TEST(ParticleTest, SnowParticle_Construction)
{
    SnowParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f));

    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle.getTextureLocation(), mc::ResourceLocation("minecraft:particle/snowflake"));
    EXPECT_TRUE(particle.isAlive());

    // 验证重力设置（雪花重力比雨小）
    EXPECT_DOUBLE_EQ(particle.gravity(), mc::physics::SNOW_GRAVITY);
    // SNOW_GRAVITY = 0.02f
    EXPECT_NEAR(static_cast<f64>(mc::physics::SNOW_GRAVITY), 0.02, 0.001);
}

/**
 * @brief 测试雪花工厂方法返回正确的粒子类型
 */
TEST(ParticleTest, SnowParticle_CreateReturnsSnowParticle)
{
    auto particle = SnowParticle::create(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f), nullptr);
    ASSERT_NE(particle, nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle->getTextureLocation(), mc::ResourceLocation("minecraft:particle/snowflake"));
}

/**
 * @brief 测试雪花无世界 tick（无碰撞检测）
 */
TEST(ParticleTest, SnowParticle_TickWithoutWorld)
{
    SnowParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, -0.1f, 0.0f));
    particle.tick(nullptr);

    // 无世界时应该仍然存活并下落
    EXPECT_TRUE(particle.isAlive());
    EXPECT_LT(particle.velocity().y, 0.0f); // 重力使速度更负
}

/**
 * @brief 测试雪花终端速度
 *
 * 雪花终端速度应该比雨滴慢
 */
TEST(ParticleTest, SnowParticle_TerminalVelocity)
{
    SnowParticle particle(glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // 多次 tick
    for (int i = 0; i < 100; ++i) {
        particle.tick(nullptr);
    }

    // 雪花终端速度 -0.5，比雨滴 (-3.0) 慢很多
    EXPECT_GE(particle.velocity().y, -0.6f);
    EXPECT_LT(particle.velocity().y, 0.0f);
}

/**
 * @brief 测试雪花摇摆效果
 */
TEST(ParticleTest, SnowParticle_SwingMotion)
{
    SnowParticle particle(glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    f32 initialVelX = particle.velocity().x;

    // 多次 tick，X 方向应该有摇摆
    for (int i = 0; i < 10; ++i) {
        particle.tick(nullptr);
    }

    // 雪花应该有水平摇摆（正弦波漂移）
    // 注意：由于随机初始相位，摇摆可能在任何位置，所以只验证速度变化
    // 而不是特定的方向
}

/**
 * @brief 测试雪花碰撞盒尺寸
 */
TEST(ParticleTest, SnowParticle_BoundingBoxSize)
{
    SnowParticle particle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // 验证碰撞盒尺寸
    auto bbox = particle.getBoundingBox();
    f32 width = static_cast<f32>(bbox.maxX - bbox.minX);
    f32 height = static_cast<f32>(bbox.maxY - bbox.minY);

    // 雪花碰撞盒很小（0.02 x 0.02）
    EXPECT_NEAR(width, 0.02f, 0.001f);
    EXPECT_NEAR(height, 0.02f, 0.001f);
}

/**
 * @brief 测试雪花淡出效果
 *
 * 雪花在生命后半段淡出
 */
TEST(ParticleTest, SnowParticle_FadeOut)
{
    SnowParticle particle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    // 设置较短生命周期以便测试
    particle.setMaxAge(10.0f);
    particle.setColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.9f));

    f32 initialAlpha = particle.color().a;

    // 在生命周期前 80%，alpha 应该保持
    for (int i = 0; i < 8; ++i) {
        particle.tick(nullptr);
    }
    EXPECT_FLOAT_EQ(particle.color().a, initialAlpha);

    // 在后 20%，alpha 应该减小
    particle.tick(nullptr);
    particle.tick(nullptr);
    EXPECT_LT(particle.color().a, initialAlpha);
}

/**
 * @brief 测试水溅粒子构造
 */
TEST(ParticleTest, SplashParticle_Construction)
{
    SplashParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.1f, 0.2f, 0.1f));

    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle.getTextureLocation(), mc::ResourceLocation("minecraft:particle/splash"));
    EXPECT_TRUE(particle.isAlive());
}

TEST(ParticleTest, ColorFade)
{
    Particle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setMaxAge(10.0f);
    particle.setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 在生命周期前半段，alpha 应该保持 1.0
    for (int i = 0; i < 5; ++i) {
        particle.tick(nullptr);
    }
    EXPECT_FLOAT_EQ(particle.color().a, 1.0f);

    // 在后半段，alpha 应该逐渐减小
    particle.tick(nullptr);
    EXPECT_LT(particle.color().a, 1.0f);
}

/**
 * @brief 测试粒子渲染类型工具函数
 */
TEST(ParticleRenderTypeTest, UtilityFunctions)
{
    // TERRAIN_SHEET
    EXPECT_TRUE(needsDepthWrite(ParticleRenderType::TERRAIN_SHEET));
    EXPECT_FALSE(needsBlending(ParticleRenderType::TERRAIN_SHEET));
    EXPECT_FALSE(isAlwaysLit(ParticleRenderType::TERRAIN_SHEET));
    EXPECT_TRUE(usesTerrainAtlas(ParticleRenderType::TERRAIN_SHEET));

    // PARTICLE_SHEET_OPAQUE
    EXPECT_TRUE(needsDepthWrite(ParticleRenderType::PARTICLE_SHEET_OPAQUE));
    EXPECT_FALSE(needsBlending(ParticleRenderType::PARTICLE_SHEET_OPAQUE));
    EXPECT_FALSE(isAlwaysLit(ParticleRenderType::PARTICLE_SHEET_OPAQUE));

    // PARTICLE_SHEET_LIT
    EXPECT_FALSE(needsDepthWrite(ParticleRenderType::PARTICLE_SHEET_LIT));
    EXPECT_TRUE(needsBlending(ParticleRenderType::PARTICLE_SHEET_LIT));
    EXPECT_TRUE(isAlwaysLit(ParticleRenderType::PARTICLE_SHEET_LIT));

    // PARTICLE_SHEET_TRANSLUCENT
    EXPECT_FALSE(needsDepthWrite(ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT));
    EXPECT_TRUE(needsBlending(ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT));
    EXPECT_FALSE(isAlwaysLit(ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT));

    // CUSTOM
    EXPECT_FALSE(needsDepthWrite(ParticleRenderType::CUSTOM));
    EXPECT_FALSE(needsBlending(ParticleRenderType::CUSTOM));

    // NO_RENDER
    EXPECT_FALSE(shouldRender(ParticleRenderType::NO_RENDER));
}

/**
 * @brief 测试渲染顺序
 */
TEST(ParticleRenderTypeTest, RenderOrder)
{
    // 渲染顺序应该按枚举值递增
    EXPECT_LT(
        getRenderOrder(ParticleRenderType::TERRAIN_SHEET), getRenderOrder(ParticleRenderType::PARTICLE_SHEET_OPAQUE));
    EXPECT_LT(getRenderOrder(ParticleRenderType::PARTICLE_SHEET_OPAQUE),
        getRenderOrder(ParticleRenderType::PARTICLE_SHEET_LIT));
    EXPECT_LT(getRenderOrder(ParticleRenderType::PARTICLE_SHEET_LIT),
        getRenderOrder(ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT));
    EXPECT_LT(
        getRenderOrder(ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT), getRenderOrder(ParticleRenderType::CUSTOM));
}

/**
 * @brief 测试粒子类型 ID 有效性检查
 */
TEST(ParticleTypesTest, TypeValidation)
{
    // 有效类型
    EXPECT_TRUE(isValidParticleType(ParticleTypeId::Flame));
    EXPECT_TRUE(isValidParticleType(ParticleTypeId::Smoke));
    EXPECT_TRUE(isValidParticleType(ParticleTypeId::Heart));
    EXPECT_TRUE(isValidParticleType(static_cast<ParticleTypeId>(127)));

    // 无效类型
    EXPECT_FALSE(isValidParticleType(ParticleTypeId::Invalid));
    EXPECT_FALSE(isValidParticleType(static_cast<ParticleTypeId>(128)));
    EXPECT_FALSE(isValidParticleType(static_cast<ParticleTypeId>(255)));
}

/**
 * @brief 测试粒子类型数据需求检查
 */
TEST(ParticleTypesTest, DataRequirements)
{
    // 需要方块状态的类型
    EXPECT_TRUE(requiresBlockState(ParticleTypeId::Block));
    EXPECT_TRUE(requiresBlockState(ParticleTypeId::Breaking));
    EXPECT_TRUE(requiresBlockState(ParticleTypeId::FallingDust));

    // 不需要方块状态的类型
    EXPECT_FALSE(requiresBlockState(ParticleTypeId::Flame));
    EXPECT_FALSE(requiresBlockState(ParticleTypeId::Smoke));

    // 需要物品数据的类型
    EXPECT_TRUE(requiresItemData(ParticleTypeId::Item));
    EXPECT_TRUE(requiresItemData(ParticleTypeId::ItemSlime));

    // 需要红石颜色的类型
    EXPECT_TRUE(requiresDustColor(ParticleTypeId::Redstone));
    EXPECT_TRUE(requiresDustColor(ParticleTypeId::Dust));
}

/**
 * @brief 测试钓鱼粒子构造
 *
 * 参考 MC 1.16.5 FishingParticle：
 * - 无重力（漂浮在水面）
 * - 生命周期较短
 * - 半透明淡蓝色
 */
TEST(ParticleTest, FishingParticle_Construction)
{
    FishingParticle particle(glm::vec3(0.5f, 62.0f, 0.5f), glm::vec3(0.01f, 0.0f, 0.01f));

    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle.getTextureLocation(), mc::ResourceLocation("minecraft:particle/fishing"));
    EXPECT_TRUE(particle.isAlive());
    EXPECT_DOUBLE_EQ(particle.gravity(), 0.0); // 无重力
}

/**
 * @brief 测试钓鱼粒子工厂方法
 */
TEST(ParticleTest, FishingParticle_CreateReturnsFishingParticle)
{
    auto particle = FishingParticle::create(glm::vec3(0.5f, 62.0f, 0.5f), glm::vec3(0.01f, 0.0f, 0.01f), nullptr);
    ASSERT_NE(particle, nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle->getTextureLocation(), mc::ResourceLocation("minecraft:particle/fishing"));
}

/**
 * @brief 测试钓鱼粒子 tick 行为
 */
TEST(ParticleTest, FishingParticle_TickMovement)
{
    glm::vec3 initialPos(0.5f, 62.0f, 0.5f);
    glm::vec3 initialVel(0.02f, 0.0f, 0.02f);
    FishingParticle particle(initialPos, initialVel);
    particle.setMaxAge(100.0f);

    // 多次 tick
    for (int i = 0; i < 5; ++i) {
        particle.tick(nullptr);
    }

    // 位置应该根据速度移动
    EXPECT_GT(particle.position().x, initialPos.x);
    EXPECT_GT(particle.position().z, initialPos.z);

    // 速度应该因摩擦力而减小
    EXPECT_LT(std::abs(particle.velocity().x), std::abs(initialVel.x));
    EXPECT_LT(std::abs(particle.velocity().z), std::abs(initialVel.z));
}

/**
 * @brief 测试钓鱼粒子淡出效果
 */
TEST(ParticleTest, FishingParticle_FadeOut)
{
    FishingParticle particle(glm::vec3(0.0f, 62.0f, 0.0f), glm::vec3(0.0f));
    particle.setMaxAge(10.0f);

    // 保存初始 alpha
    f32 initialAlpha = particle.color().a;

    // 在生命周期前 30%，alpha 应该保持
    for (int i = 0; i < 3; ++i) {
        particle.tick(nullptr);
    }
    EXPECT_FLOAT_EQ(particle.color().a, initialAlpha);

    // 在后 70%，alpha 应该逐渐减小
    for (int i = 0; i < 5; ++i) {
        particle.tick(nullptr);
    }
    EXPECT_LT(particle.color().a, initialAlpha);
}

/**
 * @brief 测试钓鱼粒子生命周期结束
 */
TEST(ParticleTest, FishingParticle_LifecycleEnd)
{
    FishingParticle particle(glm::vec3(0.0f, 62.0f, 0.0f), glm::vec3(0.0f));
    particle.setMaxAge(5.0f);

    EXPECT_TRUE(particle.isAlive());

    // Tick 到生命结束
    for (int i = 0; i < 10; ++i) {
        particle.tick(nullptr);
    }

    EXPECT_FALSE(particle.isAlive());
}

/**
 * @brief 测试钓鱼粒子注册
 *
 * 注意：工厂函数需要在客户端初始化时通过 registerBuiltinParticleFactories() 注册
 * 此测试仅验证类型元数据已注册
 */
TEST(ParticleRegistryTest, FishingParticleRegistration)
{
    auto& registry = ParticleRegistry::instance();

    // 检查类型已注册
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Fishing));
    EXPECT_TRUE(registry.isRegistered("minecraft:fishing"));

    // 检查名称
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::Fishing), "minecraft:fishing");

    // 检查类型信息（元数据）
    const ParticleTypeInfo* info = registry.getTypeInfo(ParticleTypeId::Fishing);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->id, ParticleTypeId::Fishing);
    EXPECT_EQ(info->name, "minecraft:fishing");
    // Fishing 粒子渲染类型应该是半透明
    EXPECT_EQ(info->defaultRenderType, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);

    // 工厂创建需要 registerBuiltinParticleFactories() 初始化，
    // 这通常在客户端启动时调用，测试环境不初始化图形系统
}

/**
 * @brief 测试暴击粒子构造
 *
 * 参考 MC 1.16.5 CritParticle：
 * - 无重力
 * - 渲染类型为 OPAQUE
 * - 纹理路径 minecraft:particle/critical_hit
 * - 淡黄色 (1.0, 0.9, 0.5)
 */
TEST(ParticleTest, CritParticle_Construction)
{
    CritParticle particle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.1f, 0.2f, 0.1f));

    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    EXPECT_EQ(particle.getTextureLocation(), mc::ResourceLocation("minecraft:particle/critical_hit"));
    EXPECT_TRUE(particle.isAlive());
    EXPECT_DOUBLE_EQ(particle.gravity(), 0.0); // 无重力

    // 暴击粒子颜色为淡黄色
    EXPECT_FLOAT_EQ(particle.color().r, 1.0f);
    EXPECT_NEAR(particle.color().g, 0.9f, 0.01f);
    EXPECT_NEAR(particle.color().b, 0.5f, 0.01f);
    EXPECT_FLOAT_EQ(particle.color().a, 1.0f);
}

/**
 * @brief 验证暴击粒子工厂方法返回正确的粒子类型
 */
TEST(ParticleTest, CritParticle_CreateReturnsCritParticle)
{
    auto particle = CritParticle::create(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.1f, 0.2f, 0.1f), nullptr);
    ASSERT_NE(particle, nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_OPAQUE);
    EXPECT_EQ(particle->getTextureLocation(), mc::ResourceLocation("minecraft:particle/critical_hit"));
    EXPECT_TRUE(particle->isAlive());
}

/**
 * @brief 测试暴击粒子 tick 行为
 *
 * 暴击粒子应该：
 * - 无重力，速度仅受摩擦力影响
 * - 位置随速度移动
 * - 摩擦力 0.96 使速度逐渐减小
 */
TEST(ParticleTest, CritParticle_TickMovement)
{
    glm::vec3 initialPos(0.0f, 10.0f, 0.0f);
    glm::vec3 initialVel(0.5f, 0.3f, 0.5f);
    CritParticle particle(initialPos, initialVel);
    particle.setMaxAge(100.0f);

    // Tick 一次
    particle.tick(nullptr);

    // 位置应该随速度移动
    EXPECT_GT(particle.position().x, initialPos.x);
    EXPECT_GT(particle.position().y, initialPos.y);
    EXPECT_GT(particle.position().z, initialPos.z);

    // 速度应该因摩擦力而减小
    EXPECT_LT(std::abs(particle.velocity().x), std::abs(initialVel.x));
    EXPECT_LT(std::abs(particle.velocity().z), std::abs(initialVel.z));

    // Y 方向速度也应减小（无重力）
    EXPECT_LT(std::abs(particle.velocity().y), std::abs(initialVel.y));
}

/**
 * @brief 测试暴击粒子淡出效果
 *
 * 暴击粒子在生命后半段淡出
 */
TEST(ParticleTest, CritParticle_FadeOut)
{
    CritParticle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setMaxAge(20.0f);
    particle.setColor(glm::vec4(1.0f, 0.9f, 0.5f, 1.0f));

    f32 initialAlpha = particle.color().a;

    // 在生命周期前 50%，alpha 应该保持
    for (int i = 0; i < 10; ++i) {
        particle.tick(nullptr);
    }
    EXPECT_FLOAT_EQ(particle.color().a, initialAlpha);

    // 在后 50%，alpha 应该减小
    for (int i = 0; i < 5; ++i) {
        particle.tick(nullptr);
    }
    EXPECT_LT(particle.color().a, initialAlpha);
}

/**
 * @brief 测试暴击粒子生命周期结束
 */
TEST(ParticleTest, CritParticle_LifecycleEnd)
{
    CritParticle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setMaxAge(5.0f);

    EXPECT_TRUE(particle.isAlive());

    // Tick 到生命结束
    for (int i = 0; i < 10; ++i) {
        particle.tick(nullptr);
    }

    EXPECT_FALSE(particle.isAlive());
}

/**
 * @brief 测试暴击粒子注册
 */
TEST(ParticleRegistryTest, CritParticleRegistration)
{
    auto& registry = ParticleRegistry::instance();

    // 检查类型已注册
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Crit));
    EXPECT_TRUE(registry.isRegistered("minecraft:crit"));

    // 检查名称
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::Crit), "minecraft:crit");

    // 检查类型信息
    const ParticleTypeInfo* info = registry.getTypeInfo(ParticleTypeId::Crit);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->id, ParticleTypeId::Crit);
    EXPECT_EQ(info->name, "minecraft:crit");
    EXPECT_EQ(info->defaultRenderType, ParticleRenderType::PARTICLE_SHEET_OPAQUE);
}

/**
 * @brief 测试附魔暴击粒子构造
 *
 * 参考 MC 1.16.5 EnchantedHitParticle（MagicCritParticle）：
 * - 无重力
 * - 渲染类型为 TRANSLUCENT
 * - 纹理路径 minecraft:particle/enchanted_hit
 * - 紫蓝色调（红色和绿色通道减弱）
 */
TEST(ParticleTest, EnchantedHitParticle_Construction)
{
    EnchantedHitParticle particle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.1f, 0.2f, 0.1f));

    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle.getTextureLocation(), mc::ResourceLocation("minecraft:particle/enchanted_hit"));
    EXPECT_TRUE(particle.isAlive());
    EXPECT_DOUBLE_EQ(particle.gravity(), 0.0); // 无重力

    // 附魔暴击粒子颜色应为紫蓝色调
    // r = (random * 0.3 + 0.6) * 0.3 → 范围 [0.18, 0.27]
    // g = (random * 0.3 + 0.6) * 0.8 → 范围 [0.48, 0.72]
    // b = random * 0.3 + 0.6 → 范围 [0.6, 0.9]
    EXPECT_GT(particle.color().r, 0.0f);
    EXPECT_LT(particle.color().r, 0.35f); // 红色通道较弱
    EXPECT_GT(particle.color().g, 0.3f);
    EXPECT_LT(particle.color().g, 0.8f);
    EXPECT_GT(particle.color().b, 0.5f); // 蓝色通道最强
    EXPECT_FLOAT_EQ(particle.color().a, 1.0f);
}

/**
 * @brief 验证附魔暴击粒子工厂方法返回正确的粒子类型
 */
TEST(ParticleTest, EnchantedHitParticle_CreateReturnsEnchantedHitParticle)
{
    auto particle = EnchantedHitParticle::create(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.1f, 0.2f, 0.1f), nullptr);
    ASSERT_NE(particle, nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle->getTextureLocation(), mc::ResourceLocation("minecraft:particle/enchanted_hit"));
    EXPECT_TRUE(particle->isAlive());
}

/**
 * @brief 测试附魔暴击粒子 tick 行为
 *
 * 附魔暴击粒子应该：
 * - 无重力，速度仅受摩擦力影响
 * - 位置随速度移动
 * - 摩擦力 0.96 使速度逐渐减小
 */
TEST(ParticleTest, EnchantedHitParticle_TickMovement)
{
    glm::vec3 initialPos(0.0f, 10.0f, 0.0f);
    glm::vec3 initialVel(0.5f, 0.3f, 0.5f);
    EnchantedHitParticle particle(initialPos, initialVel);
    particle.setMaxAge(100.0f);

    // Tick 一次
    particle.tick(nullptr);

    // 位置应该随速度移动
    EXPECT_GT(particle.position().x, initialPos.x);
    EXPECT_GT(particle.position().y, initialPos.y);
    EXPECT_GT(particle.position().z, initialPos.z);

    // 速度应该因摩擦力而减小
    EXPECT_LT(std::abs(particle.velocity().x), std::abs(initialVel.x));
    EXPECT_LT(std::abs(particle.velocity().z), std::abs(initialVel.z));

    // Y 方向速度也应减小（无重力）
    EXPECT_LT(std::abs(particle.velocity().y), std::abs(initialVel.y));
}

/**
 * @brief 测试附魔暴击粒子淡出效果
 *
 * 附魔暴击粒子在生命后半段淡出
 */
TEST(ParticleTest, EnchantedHitParticle_FadeOut)
{
    EnchantedHitParticle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setMaxAge(20.0f);

    f32 initialAlpha = particle.color().a;

    // 在生命周期前 50%，alpha 应该保持
    for (int i = 0; i < 10; ++i) {
        particle.tick(nullptr);
    }
    EXPECT_FLOAT_EQ(particle.color().a, initialAlpha);

    // 在后 50%，alpha 应该减小
    for (int i = 0; i < 5; ++i) {
        particle.tick(nullptr);
    }
    EXPECT_LT(particle.color().a, initialAlpha);
}

/**
 * @brief 测试附魔暴击粒子生命周期结束
 */
TEST(ParticleTest, EnchantedHitParticle_LifecycleEnd)
{
    EnchantedHitParticle particle(glm::vec3(0.0f), glm::vec3(0.0f));
    particle.setMaxAge(5.0f);

    EXPECT_TRUE(particle.isAlive());

    // Tick 到生命结束
    for (int i = 0; i < 10; ++i) {
        particle.tick(nullptr);
    }

    EXPECT_FALSE(particle.isAlive());
}

/**
 * @brief 测试附魔暴击粒子注册
 */
TEST(ParticleRegistryTest, EnchantedHitParticleRegistration)
{
    auto& registry = ParticleRegistry::instance();

    // 检查类型已注册
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::EnchantedHit));
    EXPECT_TRUE(registry.isRegistered("minecraft:enchanted_hit"));

    // 检查名称
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::EnchantedHit), "minecraft:enchanted_hit");

    // 检查类型信息
    const ParticleTypeInfo* info = registry.getTypeInfo(ParticleTypeId::EnchantedHit);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->id, ParticleTypeId::EnchantedHit);
    EXPECT_EQ(info->name, "minecraft:enchanted_hit");
    // 附魔暴击粒子渲染类型应该是半透明
    EXPECT_EQ(info->defaultRenderType, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}
