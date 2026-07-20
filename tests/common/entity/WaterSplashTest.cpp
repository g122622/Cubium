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

#include <map>
#include <tuple>
#include <vector>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::entity;

namespace {

/**
 * @brief 测试记录粒子数据的结构
 */
struct ParticleRecord {
    particle::ParticleTypeId type;
    Vector3 position;
    Vector3 velocity;
};

/**
 * @brief 测试记录声音数据的结构
 */
struct SoundRecord {
    ResourceLocation soundEvent;
    sound::SoundCategory category;
    Vector3 position;
    f32 volume;
    f32 pitch;
};

/**
 * @brief 测试用世界存根，支持粒子、声音记录和随机数
 */
class WaterSplashTestWorld final : public test::BaseTestWorld {
public:
    WaterSplashTestWorld()
        : m_dayTime(0)
        , m_inWater(false)
    {}

    // 粒子和声音记录
    std::vector<ParticleRecord>& particles() { return m_particles; }
    std::vector<SoundRecord>& sounds() { return m_sounds; }
    void clearRecords()
    {
        m_particles.clear();
        m_sounds.clear();
    }

    // 配置方法
    void setInWater(bool inWater) { m_inWater = inWater; }
    void setDayTime(i64 time) { m_dayTime = time; }

    // IWorld 接口实现
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] bool isDaytime() const override { return m_dayTime < 12000; }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        if (m_inWater) {
            // 返回水流体状态（使用 FluidRegistry 获取）
            return fluid::Fluid::getFluidState(fluid::FluidRegistry::WATER_ID);
        }
        return fluid::Fluid::getFluidState(0);
    }

    void playSound(const ResourceLocation& soundEvent,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({soundEvent, category, position, volume, pitch});
    }

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void addParticle(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3&, u32) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] Entity* getEntity(EntityInstanceId) override { return nullptr; }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override { return nullptr; }
    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return EntityInstanceId(1); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WaterSplashTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WaterSplashTestWorld::tickManager not implemented");
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

private:
    i64 m_dayTime;
    bool m_inWater;
    std::vector<ParticleRecord> m_particles;
    std::vector<SoundRecord> m_sounds;
};

/**
 * @brief 测试用实体类，暴露 setVelocity 和其他测试需要的方法
 */
class TestEntity : public Entity {
public:
    TestEntity(IWorld* world)
        : Entity(EntityInstanceId(1), world)
    {
        // 初始化实体
        m_position = Vector3(100.0f, 64.0f, 200.0f);
        m_velocity = Vector3(0.0f, 0.0f, 0.0f);
    }

    void setTestVelocity(const Vector3& vel) { m_velocity = vel; }

    void setTestPosition(const Vector3& pos) { m_position = pos; }

    // 暴露受保护的方法用于测试
    using Entity::doWaterSplashEffect;
};

/**
 * @brief 测试用玩家类
 */
class TestPlayer : public Player {
public:
    TestPlayer(IWorld* world)
        : Player(EntityInstanceId(1), "TestPlayer")
    {
        setWorld(world);
        m_position = Vector3(100.0f, 64.0f, 200.0f);
    }

    void setTestVelocity(const Vector3& vel) { m_velocity = vel; }

    void setTestPosition(const Vector3& pos) { m_position = pos; }
};

} // namespace

// ============================================================================
// Entity::getSplashSound / getHighspeedSplashSound 测试
// ============================================================================

TEST(WaterSplashTest, EntityGetSplashSoundReturnsGenericSplash)
{
    WaterSplashTestWorld world;
    TestEntity entity(&world);

    // Entity 基类应该返回通用溅水声音
    EXPECT_EQ(entity.getSplashSound(), SoundEvents::ENTITY_GENERIC_SPLASH);
    EXPECT_EQ(entity.getHighspeedSplashSound(), SoundEvents::ENTITY_GENERIC_SPLASH);
}

TEST(WaterSplashTest, PlayerGetSplashSoundReturnsPlayerSplash)
{
    WaterSplashTestWorld world;
    TestPlayer player(&world);

    // Player 类应该返回玩家专用溅水声音
    EXPECT_EQ(player.getSplashSound(), SoundEvents::ENTITY_PLAYER_SPLASH);
    EXPECT_EQ(player.getHighspeedSplashSound(), SoundEvents::ENTITY_PLAYER_SPLASH_HIGH_SPEED);
}

// ============================================================================
// Entity::doWaterSplashEffect 粒子测试
// ============================================================================

TEST(WaterSplashTest, DoWaterSplashEffectGeneratesParticles)
{
    WaterSplashTestWorld world;
    TestEntity entity(&world);

    // 设置实体位置和速度
    entity.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    entity.setTestVelocity(Vector3(1.0f, -2.0f, 0.5f));

    // 调用水花效果
    entity.doWaterSplashEffect();

    // 检查粒子生成
    const auto& particles = world.particles();
    EXPECT_FALSE(particles.empty());

    // 应该同时生成气泡和水溅粒子
    using ParticleTypeId = particle::ParticleTypeId;
    i32 bubbleCount = 0;
    i32 splashCount = 0;

    for (const auto& p : particles) {
        if (p.type == ParticleTypeId::Bubble) {
            bubbleCount++;
        } else if (p.type == ParticleTypeId::Splash) {
            splashCount++;
        }
    }

    // 玩家宽度 0.6，粒子数量 = 1 + 0.6 * 20 = 13
    // 气泡和水溅粒子数量应该相同
    EXPECT_GT(bubbleCount, 0);
    EXPECT_GT(splashCount, 0);
    EXPECT_EQ(bubbleCount, splashCount);

    // 粒子数量应该基于实体宽度
    i32 expectedCount = static_cast<i32>(1.0f + entity.width() * 20.0f);
    EXPECT_EQ(bubbleCount, expectedCount);
}

TEST(WaterSplashTest, DoWaterSplashEffectPlaysSound)
{
    WaterSplashTestWorld world;
    TestEntity entity(&world);

    entity.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    entity.setTestVelocity(Vector3(0.0f, 0.0f, 0.0f)); // 低速

    world.clearRecords();
    entity.doWaterSplashEffect();

    const auto& sounds = world.sounds();
    ASSERT_FALSE(sounds.empty());

    // 应该播放溅水声
    EXPECT_EQ(sounds[0].soundEvent, SoundEvents::ENTITY_GENERIC_SPLASH);
    EXPECT_EQ(sounds[0].category, sound::SoundCategory::Neutral);

    // 低速时音量应该很小 (f1 < 0.25)
    EXPECT_LT(sounds[0].volume, 0.25f);
}

TEST(WaterSplashTest, DoWaterSplashEffectHighSpeedPlaysHighSpeedSound)
{
    WaterSplashTestWorld world;
    TestEntity entity(&world);

    entity.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    // 设置高速（向下跳水）
    entity.setTestVelocity(Vector3(0.0f, -10.0f, 0.0f));

    world.clearRecords();
    entity.doWaterSplashEffect();

    const auto& sounds = world.sounds();
    ASSERT_FALSE(sounds.empty());

    // 高速应该播放高速溅水声
    EXPECT_EQ(sounds[0].soundEvent, SoundEvents::ENTITY_GENERIC_SPLASH);
    // 高速时音量应该较大 (f1 >= 0.25)
    EXPECT_GE(sounds[0].volume, 0.25f);
}

TEST(WaterSplashTest, DoWaterSplashEffectParticlePosition)
{
    WaterSplashTestWorld world;
    TestEntity entity(&world);

    Vector3 entityPos(100.5f, 64.3f, 200.7f);
    entity.setTestPosition(entityPos);
    entity.setTestVelocity(Vector3(0.0f, 0.0f, 0.0f));

    world.clearRecords();
    entity.doWaterSplashEffect();

    const auto& particles = world.particles();
    ASSERT_FALSE(particles.empty());

    // 所有粒子的 Y 坐标应该是 floor(posY) + 1.0
    f32 expectedY = std::floor(entityPos.y) + 1.0f;
    for (const auto& p : particles) {
        EXPECT_FLOAT_EQ(p.position.y, expectedY);
    }
}

TEST(WaterSplashTest, DoWaterSplashEffectSoundPitchRandomized)
{
    WaterSplashTestWorld world;
    TestEntity entity(&world);

    entity.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    entity.setTestVelocity(Vector3(0.0f, 0.0f, 0.0f));

    // 运行多次检查音调随机化
    std::vector<f32> pitches;
    for (int i = 0; i < 10; ++i) {
        world.clearRecords();
        entity.doWaterSplashEffect();
        pitches.push_back(world.sounds()[0].pitch);
    }

    // 音调应该在 0.6 到 1.4 范围内 (1.0 +/- 0.4)
    for (f32 pitch : pitches) {
        EXPECT_GE(pitch, 0.6f);
        EXPECT_LE(pitch, 1.4f);
    }

    // 由于随机性，不应该所有音调都相同
    bool hasDifferentPitch = false;
    for (size_t i = 1; i < pitches.size(); ++i) {
        if (pitches[i] != pitches[0]) {
            hasDifferentPitch = true;
            break;
        }
    }
    EXPECT_TRUE(hasDifferentPitch);
}

// ============================================================================
// Player::doWaterSplashEffect 测试
// ============================================================================

TEST(WaterSplashTest, PlayerDoWaterSplashEffectGeneratesParticles)
{
    WaterSplashTestWorld world;
    TestPlayer player(&world);

    player.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    player.setTestVelocity(Vector3(0.0f, -1.0f, 0.0f));

    world.clearRecords();
    player.doWaterSplashEffect();

    // 检查粒子生成
    const auto& particles = world.particles();
    EXPECT_FALSE(particles.empty());

    // 检查声音
    const auto& sounds = world.sounds();
    ASSERT_FALSE(sounds.empty());
    EXPECT_EQ(sounds[0].soundEvent, SoundEvents::ENTITY_PLAYER_SPLASH);
}

TEST(WaterSplashTest, SpectatorPlayerDoesNotGenerateSplash)
{
    WaterSplashTestWorld world;
    TestPlayer player(&world);

    player.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    player.setTestVelocity(Vector3(0.0f, -10.0f, 0.0f));
    player.setGameMode(GameMode::Spectator); // 设置为观察者模式

    world.clearRecords();
    player.doWaterSplashEffect();

    // 观察者模式不应该产生任何粒子或声音
    EXPECT_TRUE(world.particles().empty());
    EXPECT_TRUE(world.sounds().empty());
}

// ============================================================================
// 速度因子 f1 计算测试
// ============================================================================

TEST(WaterSplashTest, VelocityFactorCalculation)
{
    WaterSplashTestWorld world;
    TestEntity entity(&world);

    entity.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));

    // 测试不同速度下的音量
    // f1 = sqrt(vx^2 * 0.2 + vy^2 + vz^2 * 0.2) * 0.2

    // 低速：f1 < 0.25 -> 普通溅水声
    entity.setTestVelocity(Vector3(0.0f, 0.0f, 0.0f));
    world.clearRecords();
    entity.doWaterSplashEffect();
    EXPECT_LT(world.sounds()[0].volume, 0.25f);

    // 中速
    entity.setTestVelocity(Vector3(0.0f, -2.0f, 0.0f)); // f1 = sqrt(4) * 0.2 = 0.4
    world.clearRecords();
    entity.doWaterSplashEffect();
    EXPECT_GE(world.sounds()[0].volume, 0.25f);

    // 高速（限制在 1.0）
    entity.setTestVelocity(Vector3(0.0f, -50.0f, 0.0f)); // f1 会被限制在 1.0
    world.clearRecords();
    entity.doWaterSplashEffect();
    EXPECT_FLOAT_EQ(world.sounds()[0].volume, 1.0f);
}

// ============================================================================
// 粒子速度继承测试
// ============================================================================

TEST(WaterSplashTest, ParticleVelocityInheritance)
{
    WaterSplashTestWorld world;
    TestEntity entity(&world);

    Vector3 entityVel(1.0f, 2.0f, 0.5f);
    entity.setTestPosition(Vector3(100.0f, 64.0f, 200.0f));
    entity.setTestVelocity(entityVel);

    world.clearRecords();
    entity.doWaterSplashEffect();

    const auto& particles = world.particles();
    ASSERT_FALSE(particles.empty());

    using ParticleTypeId = particle::ParticleTypeId;

    // 找到一个水溅粒子，检查速度
    for (const auto& p : particles) {
        if (p.type == ParticleTypeId::Splash) {
            // 水溅粒子应该继承实体的 X 和 Z 速度
            // Y 速度继承实体的 Y 速度
            EXPECT_FLOAT_EQ(p.velocity.x, entityVel.x);
            EXPECT_FLOAT_EQ(p.velocity.z, entityVel.z);
            EXPECT_FLOAT_EQ(p.velocity.y, entityVel.y);
            break;
        }
    }

    // 气泡粒子的 Y 速度应该减去一个随机值 (0 到 0.2)
    for (const auto& p : particles) {
        if (p.type == ParticleTypeId::Bubble) {
            // 气泡 Y 速度 = entityVel.y - random(0, 0.2)
            // 所以应该小于等于实体的 Y 速度
            EXPECT_LE(p.velocity.y, entityVel.y);
            EXPECT_GE(p.velocity.y, entityVel.y - 0.2f);
            EXPECT_FLOAT_EQ(p.velocity.x, entityVel.x);
            EXPECT_FLOAT_EQ(p.velocity.z, entityVel.z);
            break;
        }
    }
}
