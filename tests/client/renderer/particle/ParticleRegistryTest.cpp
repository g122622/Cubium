#include <gtest/gtest.h>
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <glm/glm.hpp>

// 前置声明 ClientWorld
namespace mc::client {
class ClientWorld;
}

using namespace mc::client::renderer::trident::particle;
using namespace mc;

/**
 * @brief 测试粒子注册表单例
 */
TEST(ParticleRegistryTest, SingletonInstance) {
    ParticleRegistry& instance1 = ParticleRegistry::instance();
    ParticleRegistry& instance2 = ParticleRegistry::instance();

    EXPECT_EQ(&instance1, &instance2);
}

/**
 * @brief 测试内置粒子类型注册
 */
TEST(ParticleRegistryTest, BuiltinTypesRegistered) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    // 检查一些内置类型是否已注册
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Flame));
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Smoke));
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Lava));
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Portal));
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Rain));
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Snowflake));
    EXPECT_TRUE(registry.isRegistered(ParticleTypeId::Heart));

    // 通过名称检查
    EXPECT_TRUE(registry.isRegistered("minecraft:flame"));
    EXPECT_TRUE(registry.isRegistered("minecraft:smoke"));
    EXPECT_TRUE(registry.isRegistered("minecraft:lava"));
}

/**
 * @brief 测试获取类型名称
 */
TEST(ParticleRegistryTest, GetTypeName) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    EXPECT_EQ(registry.getTypeName(ParticleTypeId::Flame), "minecraft:flame");
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::Smoke), "minecraft:smoke");
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::Lava), "minecraft:lava");
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::Rain), "minecraft:rain");
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::Heart), "minecraft:heart");

    // 无效类型
    EXPECT_EQ(registry.getTypeName(ParticleTypeId::Invalid), "minecraft:invalid");
    EXPECT_EQ(registry.getTypeName(static_cast<ParticleTypeId>(999)), "minecraft:invalid");
}

/**
 * @brief 测试通过名称获取类型 ID
 */
TEST(ParticleRegistryTest, GetTypeIdByName) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    auto flameId = registry.getTypeId("minecraft:flame");
    EXPECT_TRUE(flameId.has_value());
    EXPECT_EQ(flameId.value(), ParticleTypeId::Flame);

    auto smokeId = registry.getTypeId("minecraft:smoke");
    EXPECT_TRUE(smokeId.has_value());
    EXPECT_EQ(smokeId.value(), ParticleTypeId::Smoke);

    // 不存在的名称
    auto invalidId = registry.getTypeId("minecraft:nonexistent");
    EXPECT_FALSE(invalidId.has_value());

    // 空字符串
    auto emptyId = registry.getTypeId("");
    EXPECT_FALSE(emptyId.has_value());
}

/**
 * @brief 测试通过资源位置获取类型 ID
 */
TEST(ParticleRegistryTest, GetTypeIdByResourceLocation) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    // 注册表使用 "minecraft:flame" 格式
    auto id = registry.getTypeId(mc::ResourceLocation("minecraft", "flame"));
    EXPECT_TRUE(id.has_value());
    EXPECT_EQ(id.value(), ParticleTypeId::Flame);
}

/**
 * @brief 测试获取类型信息
 */
TEST(ParticleRegistryTest, GetTypeInfo) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    const ParticleTypeInfo* flameInfo = registry.getTypeInfo(ParticleTypeId::Flame);
    ASSERT_NE(flameInfo, nullptr);
    EXPECT_EQ(flameInfo->id, ParticleTypeId::Flame);
    EXPECT_EQ(flameInfo->name, "minecraft:flame");

    // 无效类型
    const ParticleTypeInfo* invalidInfo = registry.getTypeInfo(ParticleTypeId::Invalid);
    EXPECT_EQ(invalidInfo, nullptr);

    const ParticleTypeInfo* outOfRangeInfo = registry.getTypeInfo(static_cast<ParticleTypeId>(999));
    EXPECT_EQ(outOfRangeInfo, nullptr);
}

/**
 * @brief 测试获取所有类型 ID
 */
TEST(ParticleRegistryTest, GetAllTypeIds) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    std::vector<ParticleTypeId> ids = registry.getAllTypeIds();

    // 应该有多个类型
    EXPECT_GT(ids.size(), 0u);

    // 检查一些已知类型是否在列表中
    bool hasFlame = false;
    bool hasSmoke = false;
    bool hasRain = false;

    for (ParticleTypeId id : ids) {
        if (id == ParticleTypeId::Flame) hasFlame = true;
        if (id == ParticleTypeId::Smoke) hasSmoke = true;
        if (id == ParticleTypeId::Rain) hasRain = true;
    }

    EXPECT_TRUE(hasFlame);
    EXPECT_TRUE(hasSmoke);
    EXPECT_TRUE(hasRain);
}

/**
 * @brief 测试类型数量
 */
TEST(ParticleRegistryTest, TypeCount) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    // 应该有多个类型注册
    EXPECT_GT(registry.typeCount(), 0u);
}

/**
 * @brief 测试自定义粒子类型注册
 */
TEST(ParticleRegistryTest, RegisterCustomType) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    // 注册一个自定义粒子类型
    ParticleFactory customFactory = [](
        const glm::vec3& pos,
        const glm::vec3& vel,
        mc::client::ClientWorld* world) -> std::unique_ptr<Particle> {
        MC_UNUSED(world);
        return std::make_unique<Particle>(pos, vel);
    };

    // 使用未使用的ID范围
    ParticleTypeId customId = static_cast<ParticleTypeId>(100);

    registry.registerType(
        customId,
        "test:custom_particle",
        customFactory,
        ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT,
        30.0f,  // lifetime
        true,   // hasPhysics
        false   // ignoreDistance
    );

    EXPECT_TRUE(registry.isRegistered(customId));
    EXPECT_TRUE(registry.isRegistered("test:custom_particle"));

    auto id = registry.getTypeId("test:custom_particle");
    EXPECT_TRUE(id.has_value());
    EXPECT_EQ(id.value(), customId);

    const ParticleTypeInfo* info = registry.getTypeInfo(customId);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->name, "test:custom_particle");
    EXPECT_EQ(info->defaultRenderType, ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_FLOAT_EQ(info->defaultLifetime, 30.0f);
    EXPECT_TRUE(info->hasPhysics);
    EXPECT_FALSE(info->ignoreDistance);
}

/**
 * @brief 测试创建粒子实例
 */
TEST(ParticleRegistryTest, CreateParticle) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    // 注册一个可创建的类型
    ParticleFactory testFactory = [](
        const glm::vec3& pos,
        const glm::vec3& vel,
        mc::client::ClientWorld* world) -> std::unique_ptr<Particle> {
        MC_UNUSED(world);
        auto particle = std::make_unique<Particle>(pos, vel);
        particle->setMaxAge(50.0f);
        return particle;
    };

    // 使用一个未使用的 ID
    ParticleTypeId testId = static_cast<ParticleTypeId>(101);
    registry.registerSimpleType(testId, "test:create_test", testFactory);

    glm::vec3 pos(1.0f, 2.0f, 3.0f);
    glm::vec3 vel(0.1f, 0.2f, 0.3f);

    std::unique_ptr<Particle> particle = registry.createParticle(testId, pos, vel, nullptr);

    ASSERT_NE(particle, nullptr);
    EXPECT_FLOAT_EQ(particle->position().x, 1.0f);
    EXPECT_FLOAT_EQ(particle->position().y, 2.0f);
    EXPECT_FLOAT_EQ(particle->position().z, 3.0f);
    EXPECT_FLOAT_EQ(particle->velocity().x, 0.1f);
    EXPECT_FLOAT_EQ(particle->velocity().y, 0.2f);
    EXPECT_FLOAT_EQ(particle->velocity().z, 0.3f);
    EXPECT_FLOAT_EQ(particle->maxAge(), 50.0f);
}

/**
 * @brief 测试通过名称创建粒子实例
 */
TEST(ParticleRegistryTest, CreateParticleByName) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    ParticleFactory testFactory = [](
        const glm::vec3& pos,
        const glm::vec3& vel,
        mc::client::ClientWorld* world) -> std::unique_ptr<Particle> {
        MC_UNUSED(world);
        return std::make_unique<Particle>(pos, vel);
    };

    ParticleTypeId testId = static_cast<ParticleTypeId>(102);
    registry.registerSimpleType(testId, "test:name_create_test", testFactory);

    glm::vec3 pos(0.0f, 0.0f, 0.0f);
    glm::vec3 vel(0.0f, 0.0f, 0.0f);

    std::unique_ptr<Particle> particle = registry.createParticle("test:name_create_test", pos, vel, nullptr);
    ASSERT_NE(particle, nullptr);

    // 测试不存在的名称
    std::unique_ptr<Particle> invalidParticle = registry.createParticle("test:nonexistent", pos, vel, nullptr);
    EXPECT_EQ(invalidParticle, nullptr);
}

/**
 * @brief 测试无效类型创建
 */
TEST(ParticleRegistryTest, CreateInvalidParticle) {
    ParticleRegistry& registry = ParticleRegistry::instance();

    glm::vec3 pos(0.0f);
    glm::vec3 vel(0.0f);

    // 无效 ID
    std::unique_ptr<Particle> invalidIdParticle = registry.createParticle(
        ParticleTypeId::Invalid, pos, vel, nullptr);
    EXPECT_EQ(invalidIdParticle, nullptr);

    // 未注册的 ID
    std::unique_ptr<Particle> unregisteredParticle = registry.createParticle(
        static_cast<ParticleTypeId>(200), pos, vel, nullptr);
    EXPECT_EQ(unregisteredParticle, nullptr);
}
