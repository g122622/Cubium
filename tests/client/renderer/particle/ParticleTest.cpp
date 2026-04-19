#include <gtest/gtest.h>
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/particles/RainParticle.hpp"
#include "client/renderer/trident/particle/data/BasicParticleData.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include <glm/glm.hpp>

using namespace mc::client::renderer::trident::particle;
using namespace mc::client::renderer::trident::particle::data;
using namespace mc::client::renderer::trident::particle::particles;

namespace mc::client::renderer::trident::particle {

/**
 * @brief 粒子层最小世界接口实现
 *
 * 只保留雨滴碰撞所需的 `getBlockState(...)`，便于在测试里构造轻量 stub。
 */
class ClientWorld {
public:
    virtual ~ClientWorld() = default;

    /**
     * @brief 获取指定方块状态
     *
     * @param x 方块 x 坐标
     * @param y 方块 y 坐标
     * @param z 方块 z 坐标
     * @return 方块状态指针
     */
    [[nodiscard]] virtual const mc::BlockState* getBlockState(mc::i32 x, mc::i32 y, mc::i32 z) const = 0;
};

} // namespace mc::client::renderer::trident::particle

namespace {

/**
 * @brief 雨滴测试用世界
 *
 * 只重写 `getBlockState(...)`，用于验证雨滴的轻量方块碰撞逻辑。
 */
class RainParticleTestWorld final : public mc::client::renderer::trident::particle::ClientWorld {
public:
    /**
     * @brief 设置方块状态
     *
     * @param state 目标方块状态；传入空指针表示当前位置为空气。
     */
    void setBlockState(const mc::BlockState* state) {
        m_groundState = state;
    }

    /**
     * @brief 获取方块状态
     *
     * 仅在测试目标方块位置返回预设状态，其余位置视为空气。
     *
     * @param x 方块 x 坐标
     * @param y 方块 y 坐标
     * @param z 方块 z 坐标
     * @return 方块状态指针
     */
    [[nodiscard]] const mc::BlockState* getBlockState(mc::i32 x, mc::i32 y, mc::i32 z) const override {
        if (x == 0 && y == 63 && z == 0) {
            return m_groundState;
        }
        return nullptr;
    }

private:
    const mc::BlockState* m_groundState = nullptr;
};

} // namespace

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
 * @brief 雨滴碰到方块后进入落地状态
 */
TEST(ParticleTest, RainParticle_SetOnGroundWhenBlockBelow) {
    mc::VanillaBlocks::initialize();

    RainParticleTestWorld world;
    world.setBlockState(&mc::VanillaBlocks::STONE->defaultState());

    RainParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f));
    particle.tick(&world);

    EXPECT_TRUE(particle.onGround());
}

/**
 * @brief 雨滴下方为空气时不会进入落地状态
 */
TEST(ParticleTest, RainParticle_StayAirborneWithoutBlockBelow) {
    mc::VanillaBlocks::initialize();

    RainParticleTestWorld world;
    world.setBlockState(nullptr);

    RainParticle particle(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f));
    particle.tick(&world);

    EXPECT_FALSE(particle.onGround());
}

/**
 * @brief 测试粒子颜色淡出
 */
/**
 * @brief 验证雨滴工厂方法返回正确的粒子类型
 *
 * 这里显式传入世界对象，确保工厂函数的参数类型和命名空间绑定正确，
 * 同时确认它返回的是雨滴粒子，而不是基类默认实现。
 */
TEST(ParticleTest, RainParticle_CreateReturnsRainParticle) {
    mc::VanillaBlocks::initialize();

    RainParticleTestWorld world;
    world.setBlockState(&mc::VanillaBlocks::STONE->defaultState());

    auto particle = RainParticle::create(glm::vec3(0.5f, 64.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f), &world);
    ASSERT_NE(particle, nullptr);

    EXPECT_EQ(particle->getRenderType(), ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT);
    EXPECT_EQ(particle->getTextureLocation(), mc::ResourceLocation("minecraft:particle/rain"));
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
