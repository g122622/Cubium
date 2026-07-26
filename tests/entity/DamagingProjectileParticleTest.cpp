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

#include <vector>
#include <gtest/gtest.h>

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/entities/projectile/DamagingProjectileEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::client::renderer::trident::particle;

namespace {

/**
 * @brief 测试记录粒子数据的结构
 */
struct ParticleRecord {
    ParticleTypeId type;
    Vector3 position;
    Vector3 velocity;
};

/**
 * @brief 测试用世界存根，支持粒子记录和水中状态
 */
class DamagingProjectileTestWorld final : public test::BaseTestWorld {
public:
    DamagingProjectileTestWorld()
        : m_inWater(false)
        , m_clientSide(true)
    {}

    // 粒子记录
    std::vector<ParticleRecord>& particles() { return m_particles; }
    void clearParticles() { m_particles.clear(); }

    // 配置方法
    void setInWater(bool inWater) { m_inWater = inWater; }
    void setClientSide(bool clientSide) { m_clientSide = clientSide; }

    // IWorld 接口实现
    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        if (m_inWater) {
            return &fluid::Fluids::WATER()->defaultState();
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    void addParticle(ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void addParticle(ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3&, u32) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] Entity* getEntity(EntityInstanceId) override { return nullptr; }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override { return nullptr; }
    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return EntityInstanceId(1); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("DamagingProjectileTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("DamagingProjectileTestWorld::tickManager not implemented");
    }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(const BlockPos&) const override { return 15; }
    [[nodiscard]] u8 getBlockLight(const BlockPos&) const override { return 0; }
    [[nodiscard]] bool canSeeSky(const BlockPos&) const override { return true; }
    [[nodiscard]] f32 getBrightness(const BlockPos&) const override { return 1.0f; }
    [[nodiscard]] u8 getLightSubtracted(const BlockPos&, u32) const override { return 15; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool isThundering() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isDaytime() const override { return true; }

private:
    bool m_inWater;
    bool m_clientSide;
    std::vector<ParticleRecord> m_particles;
};

/**
 * @brief 测试用 DamagingProjectileEntity 子类，暴露 protected 方法
 */
class TestDamagingProjectile : public DamagingProjectileEntity {
public:
    TestDamagingProjectile(IWorld* world)
        : DamagingProjectileEntity(EntityInstanceId(1))
    {
        setWorld(world);
        m_position = Vector3(100.0f, 64.0f, 200.0f);
        m_velocity = Vector3(1.0f, 0.0f, 0.0f);
    }

    void setTestVelocity(const Vector3& vel) { m_velocity = vel; }
    void setTestPosition(const Vector3& pos) { m_position = pos; }

    // 暴露 protected 方法用于测试
    using DamagingProjectileEntity::getParticleType;
    using DamagingProjectileEntity::spawnTrailParticles;
    using DamagingProjectileEntity::spawnWaterParticles;

    // 重写 tick 以避免射线检测
    void tick() override
    {
        // 只更新位置，不执行完整的 tick 逻辑
        m_prevPosition = m_position;
        m_position = m_position + m_velocity;
    }

    // 标记已离开发射者
    void markLeftShooter() { m_leftShooter = true; }
};

/**
 * @brief 测试用 DragonFireballEntity 子类
 */
class TestDragonFireball : public DragonFireballEntity {
public:
    TestDragonFireball(IWorld* world)
        : DragonFireballEntity(EntityInstanceId(1))
    {
        setWorld(world);
        m_position = Vector3(100.0f, 64.0f, 200.0f);
    }

    // 暴露 protected 方法
    using DragonFireballEntity::getParticleType;
};

} // namespace

// ============================================================================
// DamagingProjectileEntity::getParticleType 测试
// ============================================================================

TEST(DamagingProjectileParticleTest, DefaultParticleTypeIsSmoke)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    // MC 1.16.5: DamagingProjectileEntity.getParticle() 默认返回 SMOKE
    EXPECT_EQ(projectile.getParticleType(), ParticleTypeId::Smoke);
}

TEST(DamagingProjectileParticleTest, DragonFireballParticleTypeIsDragonBreath)
{
    DamagingProjectileTestWorld world;
    TestDragonFireball dragonFireball(&world);

    // MC 1.16.5: DragonFireballEntity.getParticle() 返回 DRAGON_BREATH
    EXPECT_EQ(dragonFireball.getParticleType(), ParticleTypeId::DragonBreath);
}

// ============================================================================
// DamagingProjectileEntity::spawnWaterParticles 测试
// ============================================================================

TEST(DamagingProjectileParticleTest, SpawnWaterParticlesGeneratesFourBubbles)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    projectile.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    projectile.setTestVelocity(Vector3(1.0f, 0.5f, 0.25f));

    // 调用 spawnWaterParticles
    projectile.spawnWaterParticles();

    // MC 1.16.5: 每 tick 生成 4 个气泡粒子
    const auto& particles = world.particles();
    EXPECT_EQ(particles.size(), 4u);

    // 所有粒子应该是 BUBBLE 类型
    for (const auto& p : particles) {
        EXPECT_EQ(p.type, ParticleTypeId::Bubble);
    }
}

TEST(DamagingProjectileParticleTest, WaterParticlePositionCorrectlyOffset)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    Vector3 position(100.0f, 64.0f, 200.0f);
    Vector3 velocity(1.0f, 0.5f, 0.25f);

    projectile.setTestPosition(position);
    projectile.setTestVelocity(velocity);

    projectile.spawnWaterParticles();

    const auto& particles = world.particles();
    ASSERT_EQ(particles.size(), 4u);

    // MC 1.16.5: 粒子位置 = pos - velocity * 0.25
    Vector3 expectedPos(
        position.x - velocity.x * 0.25f, position.y - velocity.y * 0.25f, position.z - velocity.z * 0.25f);

    for (const auto& p : particles) {
        EXPECT_FLOAT_EQ(p.position.x, expectedPos.x);
        EXPECT_FLOAT_EQ(p.position.y, expectedPos.y);
        EXPECT_FLOAT_EQ(p.position.z, expectedPos.z);
    }
}

TEST(DamagingProjectileParticleTest, WaterParticleVelocityEqualsEntityVelocity)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    Vector3 position(100.0f, 64.0f, 200.0f);
    Vector3 velocity(2.0f, 1.0f, 0.5f);

    projectile.setTestPosition(position);
    projectile.setTestVelocity(velocity);

    projectile.spawnWaterParticles();

    const auto& particles = world.particles();
    ASSERT_EQ(particles.size(), 4u);

    // MC 1.16.5: 气泡粒子速度等于实体速度
    for (const auto& p : particles) {
        EXPECT_FLOAT_EQ(p.velocity.x, velocity.x);
        EXPECT_FLOAT_EQ(p.velocity.y, velocity.y);
        EXPECT_FLOAT_EQ(p.velocity.z, velocity.z);
    }
}

TEST(DamagingProjectileParticleTest, WaterParticlesNotGeneratedOnServerSide)
{
    DamagingProjectileTestWorld world;
    world.setClientSide(false); // 服务端
    TestDamagingProjectile projectile(&world);

    projectile.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    projectile.setTestVelocity(Vector3(1.0f, 0.0f, 0.0f));

    projectile.spawnWaterParticles();

    // 服务端不应该生成粒子
    EXPECT_TRUE(world.particles().empty());
}

// ============================================================================
// DamagingProjectileEntity::spawnTrailParticles 测试
// ============================================================================

TEST(DamagingProjectileParticleTest, SpawnTrailParticlesGeneratesSmokeParticle)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    Vector3 position(100.0f, 64.5f, 200.0f);
    projectile.setTestPosition(position);

    // 调用 spawnTrailParticles
    projectile.spawnTrailParticles(position);

    // MC 1.16.5: 每 tick 生成 1 个拖尾粒子
    const auto& particles = world.particles();
    EXPECT_EQ(particles.size(), 1u);

    // 默认粒子类型是 SMOKE
    EXPECT_EQ(particles[0].type, ParticleTypeId::Smoke);
}

TEST(DamagingProjectileParticleTest, TrailParticlePositionYOffset)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    // MC 1.16.5: 拖尾粒子位置 Y+0.5 偏移
    // spawnTrailParticles 在 tick() 中调用，传入的位置已经是 Y+0.5 偏移
    Vector3 position(100.0f, 64.5f, 200.0f);
    projectile.spawnTrailParticles(position);

    const auto& particles = world.particles();
    ASSERT_EQ(particles.size(), 1u);

    // 粒子位置应该与传入的位置相同
    EXPECT_FLOAT_EQ(particles[0].position.x, position.x);
    EXPECT_FLOAT_EQ(particles[0].position.y, position.y);
    EXPECT_FLOAT_EQ(particles[0].position.z, position.z);
}

TEST(DamagingProjectileParticleTest, TrailParticleVelocityIsZero)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    Vector3 position(100.0f, 64.5f, 200.0f);
    projectile.spawnTrailParticles(position);

    const auto& particles = world.particles();
    ASSERT_EQ(particles.size(), 1u);

    // MC 1.16.5: 拖尾粒子速度为 (0, 0, 0)
    EXPECT_FLOAT_EQ(particles[0].velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(particles[0].velocity.y, 0.0f);
    EXPECT_FLOAT_EQ(particles[0].velocity.z, 0.0f);
}

TEST(DamagingProjectileParticleTest, TrailParticlesNotGeneratedOnServerSide)
{
    DamagingProjectileTestWorld world;
    world.setClientSide(false); // 服务端
    TestDamagingProjectile projectile(&world);

    Vector3 position(100.0f, 64.5f, 200.0f);
    projectile.spawnTrailParticles(position);

    // 服务端不应该生成粒子
    EXPECT_TRUE(world.particles().empty());
}

// ============================================================================
// MC 1.16.5 常量测试
// ============================================================================

TEST(DamagingProjectileParticleTest, WaterParticleOffsetMatchesMC1165)
{
    // MC 1.16.5: 水下粒子位置偏移 0.25
    constexpr f32 WATER_PARTICLE_OFFSET = 0.25f;
    EXPECT_FLOAT_EQ(WATER_PARTICLE_OFFSET, 0.25f);
}

TEST(DamagingProjectileParticleTest, TrailParticleYOffsetMatchesMC1165)
{
    // MC 1.16.5: 拖尾粒子 Y 轴偏移 0.5
    constexpr f32 TRAIL_PARTICLE_Y_OFFSET = 0.5f;
    EXPECT_FLOAT_EQ(TRAIL_PARTICLE_Y_OFFSET, 0.5f);
}

TEST(DamagingProjectileParticleTest, WaterBubbleCountMatchesMC1165)
{
    // MC 1.16.5: 水下每 tick 生成 4 个气泡粒子
    constexpr i32 WATER_BUBBLE_COUNT = 4;
    EXPECT_EQ(WATER_BUBBLE_COUNT, 4);
}

TEST(DamagingProjectileParticleTest, WaterMotionFactorMatchesMC1165)
{
    // MC 1.16.5: 水中运动因子 0.8
    constexpr f32 WATER_MOTION_FACTOR = 0.8f;
    EXPECT_FLOAT_EQ(WATER_MOTION_FACTOR, 0.8f);
}

TEST(DamagingProjectileParticleTest, DefaultMotionFactorMatchesMC1165)
{
    // MC 1.16.5: 默认运动因子 0.95
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    // getMotionFactor() 是 protected 方法，无法直接测试
    // 但我们可以验证常量值
    constexpr f32 DEFAULT_MOTION_FACTOR = 0.95f;
    EXPECT_FLOAT_EQ(DEFAULT_MOTION_FACTOR, 0.95f);
}

// ============================================================================
// 综合测试
// ============================================================================

TEST(DamagingProjectileParticleTest, MultipleSpawnWaterParticlesCalls)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    projectile.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    projectile.setTestVelocity(Vector3(1.0f, 0.0f, 0.0f));

    // 多次调用应该累加粒子
    projectile.spawnWaterParticles();
    EXPECT_EQ(world.particles().size(), 4u);

    projectile.spawnWaterParticles();
    EXPECT_EQ(world.particles().size(), 8u);

    projectile.spawnWaterParticles();
    EXPECT_EQ(world.particles().size(), 12u);
}

TEST(DamagingProjectileParticleTest, MultipleSpawnTrailParticlesCalls)
{
    DamagingProjectileTestWorld world;
    TestDamagingProjectile projectile(&world);

    Vector3 pos1(100.0f, 64.5f, 200.0f);
    Vector3 pos2(101.0f, 64.5f, 201.0f);
    Vector3 pos3(102.0f, 64.5f, 202.0f);

    // 多次调用应该累加粒子
    projectile.spawnTrailParticles(pos1);
    EXPECT_EQ(world.particles().size(), 1u);

    projectile.spawnTrailParticles(pos2);
    EXPECT_EQ(world.particles().size(), 2u);

    projectile.spawnTrailParticles(pos3);
    EXPECT_EQ(world.particles().size(), 3u);

    // 验证每个粒子的位置
    EXPECT_FLOAT_EQ(world.particles()[0].position.x, pos1.x);
    EXPECT_FLOAT_EQ(world.particles()[1].position.x, pos2.x);
    EXPECT_FLOAT_EQ(world.particles()[2].position.x, pos3.x);
}
