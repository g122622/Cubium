#include <gtest/gtest.h>
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/particles/RainParticle.hpp"
#include "client/renderer/trident/particle/data/BasicParticleData.hpp"
#include <glm/glm.hpp>

using namespace mc::client::renderer::trident::particle;
using namespace mc::client::renderer::trident::particle::data;
using namespace mc::client::renderer::trident::particle::particles;

/**
 * @brief 测试粒子基类构造和属性
 */
TEST(ParticleTest, ConstructionAndProperties) {
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
TEST(ParticleTest, PropertySetters) {
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
TEST(ParticleTest, Lifecycle) {
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
TEST(ParticleTest, ExpiredFlag) {
    Particle particle(glm::vec3(0.0f), glm::vec3(0.0f));

    EXPECT_TRUE(particle.isAlive());
    EXPECT_FALSE(particle.onGround());  // 初始不在地面

    particle.setExpired();
    EXPECT_FALSE(particle.isAlive());
}

/**
 * @brief 测试粒子渲染类型
 */
TEST(ParticleTest, RenderType) {
    Particle particle(glm::vec3(0.0f), glm::vec3(0.0f));

    // 默认渲染类型
    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
}

/**
 * @brief 测试粒子移动
 */
TEST(ParticleTest, Movement) {
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
TEST(ParticleTest, Gravity) {
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
TEST(ParticleTest, RainParticle_Construction) {
    RainParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f));

    EXPECT_EQ(particle.getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle.getTextureLocation(), mc::ResourceLocation("minecraft:particle/rain"));
    EXPECT_TRUE(particle.isAlive());
}

/**
 * @brief 验证雨滴工厂方法返回正确的粒子类型
 */
TEST(ParticleTest, RainParticle_CreateReturnsRainParticle) {
    auto particle = RainParticle::create(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f), nullptr);
    ASSERT_NE(particle, nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle->getTextureLocation(), mc::ResourceLocation("minecraft:particle/rain"));
}

/**
 * @brief 测试雨滴无世界 tick（无碰撞检测）
 */
TEST(ParticleTest, RainParticle_TickWithoutWorld) {
    RainParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f));
    particle.tick(nullptr);

    // 无世界时应该仍然存活并下落
    EXPECT_TRUE(particle.isAlive());
    EXPECT_LT(particle.velocity().y, 0.0f);  // 重力使速度更负
}

TEST(ParticleTest, ColorFade) {
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
TEST(ParticleRenderTypeTest, UtilityFunctions) {
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
TEST(ParticleRenderTypeTest, RenderOrder) {
    // 渲染顺序应该按枚举值递增
    EXPECT_LT(getRenderOrder(ParticleRenderType::TERRAIN_SHEET),
              getRenderOrder(ParticleRenderType::PARTICLE_SHEET_OPAQUE));
    EXPECT_LT(getRenderOrder(ParticleRenderType::PARTICLE_SHEET_OPAQUE),
              getRenderOrder(ParticleRenderType::PARTICLE_SHEET_LIT));
    EXPECT_LT(getRenderOrder(ParticleRenderType::PARTICLE_SHEET_LIT),
              getRenderOrder(ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT));
    EXPECT_LT(getRenderOrder(ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT),
              getRenderOrder(ParticleRenderType::CUSTOM));
}

/**
 * @brief 测试粒子类型 ID 有效性检查
 */
TEST(ParticleTypesTest, TypeValidation) {
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
TEST(ParticleTypesTest, DataRequirements) {
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
