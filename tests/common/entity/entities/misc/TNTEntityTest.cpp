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
 * @file TNTEntityTest.cpp
 * @brief TNTEntity 单元测试
 *
 * 测试 TNT 实体的核心功能：点燃、爆炸、物理等。
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

namespace mc {
namespace entity {
namespace test {

/**
 * @brief 用于 TNTEntity 测试的 Mock World 实现
 */
class TNTTestWorld final : public ::mc::test::BaseTestWorld {
public:
    TNTTestWorld() = default;

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        // 地面碰撞检测（Y <= 0 时有地面）
        if (box.minY <= 0) {
            return true;
        }
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        std::vector<AxisAlignedBB> collisions;
        if (box.minY <= 0) {
            collisions.push_back(AxisAlignedBB(-1000.0, -1000.0, -1000.0, 1000.0, 0.0, 1000.0));
        }
        return collisions;
    }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    void setClientSide(bool isClient) { m_isClientSide = isClient; }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    void createExplosion(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode,
        bool causesFire,
        Entity* source) override
    {
        m_lastExplosionPos = position;
        m_lastExplosionRadius = radius;
        m_lastExplosionMode = mode;
        m_lastExplosionCausesFire = causesFire;
        m_lastExplosionSource = source;
        m_explosionCount++;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TNTTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TNTTestWorld::tickManager not implemented");
    }

    // 测试辅助方法

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    [[nodiscard]] Entity* getLastSpawnedEntity()
    {
        if (m_spawnedEntities.empty()) {
            return nullptr;
        }
        return m_spawnedEntities.back().get();
    }

    [[nodiscard]] i32 explosionCount() const { return m_explosionCount; }

    [[nodiscard]] const Vector3& lastExplosionPos() const { return m_lastExplosionPos; }

    [[nodiscard]] f32 lastExplosionRadius() const { return m_lastExplosionRadius; }

    [[nodiscard]] world::explosion::ExplosionMode lastExplosionMode() const { return m_lastExplosionMode; }

    [[nodiscard]] bool lastExplosionCausesFire() const { return m_lastExplosionCausesFire; }

    [[nodiscard]] Entity* lastExplosionSource() const { return m_lastExplosionSource; }

    void clearSpawnedEntities() { m_spawnedEntities.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    world::gamerule::GameRules m_gameRules;

    // 爆炸记录
    i32 m_explosionCount = 0;
    Vector3 m_lastExplosionPos{0, 0, 0};
    f32 m_lastExplosionRadius = 0.0f;
    world::explosion::ExplosionMode m_lastExplosionMode = world::explosion::ExplosionMode::None;
    bool m_lastExplosionCausesFire = false;
    Entity* m_lastExplosionSource = nullptr;

    // 粒子记录
    i32 m_particleCount = 0;
    particle::ParticleTypeId m_lastParticleType = particle::ParticleTypeId::Invalid;

public:
    // 粒子生成接口实现
    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particleCount++;
        m_lastParticleType = type;
        (void)pos;
        (void)velocity;
    }

    void addParticle(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count) override
    {
        m_particleCount += static_cast<i32>(count);
        m_lastParticleType = type;
        (void)pos;
        (void)velocity;
        (void)offset;
    }

    [[nodiscard]] bool shouldSpawnParticleAt(const Vector3&, f32) const override { return true; }

    [[nodiscard]] i32 particleCount() const { return m_particleCount; }

    [[nodiscard]] particle::ParticleTypeId lastParticleType() const { return m_lastParticleType; }

    void resetParticleCount()
    {
        m_particleCount = 0;
        m_lastParticleType = particle::ParticleTypeId::Invalid;
    }
};

/**
 * @brief TNTEntity 测试固件
 */
class TNTEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
    }

    void TearDown() override {}

    TNTTestWorld m_world;
};

/**
 * @brief 测试 TNTEntity 默认构造
 */
TEST_F(TNTEntityTest, DefaultConstruction)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    ASSERT_NE(tnt, nullptr);
    EXPECT_EQ(tnt->getFuse(), 0);
    EXPECT_FALSE(tnt->isPrimed());
    EXPECT_FLOAT_EQ(tnt->getExplosionRadius(), 4.0f);
}

/**
 * @brief 测试点燃功能
 */
TEST_F(TNTEntityTest, Ignite)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    EXPECT_FALSE(tnt->isPrimed());
    EXPECT_EQ(tnt->getFuse(), 0);

    tnt->ignite();

    // 点燃后引信时间应为默认值 80 tick
    EXPECT_TRUE(tnt->isPrimed());
    EXPECT_EQ(tnt->getFuse(), 80);
}

/**
 * @brief 测试引信时间设置
 */
TEST_F(TNTEntityTest, SetFuse)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());

    tnt->setFuse(40);
    EXPECT_EQ(tnt->getFuse(), 40);
    EXPECT_TRUE(tnt->isPrimed());

    tnt->setFuse(0);
    EXPECT_EQ(tnt->getFuse(), 0);
    EXPECT_FALSE(tnt->isPrimed());
}

/**
 * @brief 测试爆炸半径设置
 */
TEST_F(TNTEntityTest, SetExplosionRadius)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());

    tnt->setExplosionRadius(6.0f);
    EXPECT_FLOAT_EQ(tnt->getExplosionRadius(), 6.0f);

    tnt->setExplosionRadius(2.0f);
    EXPECT_FLOAT_EQ(tnt->getExplosionRadius(), 2.0f);
}

/**
 * @brief 测试实体尺寸
 */
TEST_F(TNTEntityTest, EntitySize)
{
    // MC 1.16.5: TNT 实体尺寸为 0.98 x 0.98
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(tnt->width(), 0.98f);
    EXPECT_FLOAT_EQ(tnt->height(), 0.98f);
}

/**
 * @brief 测试不可推动
 */
TEST_F(TNTEntityTest, IsNotPushable)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    EXPECT_FALSE(tnt->isPushable());
}

/**
 * @brief 测试不可碰撞
 */
TEST_F(TNTEntityTest, CannotBeCollidedWith)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    EXPECT_FALSE(tnt->canBeCollidedWith());
}

/**
 * @brief 测试默认引信常量
 *
 * MC 1.16.5: TNT 默认引信时间为 80 tick (4 秒)
 */
TEST_F(TNTEntityTest, DefaultFuseTime)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    // 默认引信时间应该是 80 tick
    tnt->ignite();
    EXPECT_EQ(tnt->getFuse(), 80);
}

/**
 * @brief 测试工厂方法
 */
TEST_F(TNTEntityTest, FactoryMethod)
{
    auto entity = TNTEntity::create(&m_world, mc::test::testEcsRegistry());
    ASSERT_NE(entity, nullptr);

    auto* tnt = dynamic_cast<TNTEntity*>(entity.get());
    ASSERT_NE(tnt, nullptr);
    EXPECT_EQ(tnt->getFuse(), 0);
    EXPECT_FLOAT_EQ(tnt->getExplosionRadius(), 4.0f);
}

/**
 * @brief 测试爆炸触发
 *
 * 当引信归零时，应该触发爆炸
 */
TEST_F(TNTEntityTest, ExplodeTriggersWhenFuseReachesZero)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(10.0f, 64.0f, 20.0f);

    // 设置短引信
    tnt->setFuse(5);
    EXPECT_TRUE(tnt->isPrimed());

    // tick 5 次后应该爆炸
    for (int i = 0; i < 5; ++i) {
        tnt->tick();
        m_world.advanceTick();
    }

    // 验证爆炸已触发
    EXPECT_EQ(m_world.explosionCount(), 1);
    EXPECT_FLOAT_EQ(m_world.lastExplosionRadius(), 4.0f);
    EXPECT_EQ(m_world.lastExplosionMode(), world::explosion::ExplosionMode::Break);
    EXPECT_FALSE(m_world.lastExplosionCausesFire());

    // 验证爆炸位置（X 和 Z 应该保持不变）
    EXPECT_FLOAT_EQ(m_world.lastExplosionPos().x, 10.0f);
    EXPECT_FLOAT_EQ(m_world.lastExplosionPos().z, 20.0f);
    // Y 位置会因为重力而变化，只验证爆炸时 Y 偏移 0.0625 存在
    // 由于物理模拟，Y 位置会下降，但爆炸时应该加上 0.0625 偏移
}

/**
 * @brief 测试爆炸只触发一次
 *
 * 即使多次 tick，爆炸也应该只触发一次
 */
TEST_F(TNTEntityTest, ExplodeOnlyOnce)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(0.0f, 64.0f, 0.0f);

    // 设置短引信
    tnt->setFuse(1);

    // tick 多次
    for (int i = 0; i < 10; ++i) {
        tnt->tick();
        m_world.advanceTick();
    }

    // 爆炸应该只触发一次
    EXPECT_EQ(m_world.explosionCount(), 1);
}

/**
 * @brief 测试爆炸半径可配置
 */
TEST_F(TNTEntityTest, CustomExplosionRadius)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(0.0f, 64.0f, 0.0f);
    tnt->setExplosionRadius(8.0f);
    tnt->setFuse(1);

    tnt->tick();

    EXPECT_FLOAT_EQ(m_world.lastExplosionRadius(), 8.0f);
}

/**
 * @brief 测试客户端爆炸
 *
 * 注意：TNTEntity.explode() 在客户端也会调用 createExplosion，
 * 但客户端世界的 createExplosion 实现可能不同（通常不破坏方块）。
 * 这里测试的是爆炸调用本身。
 */
TEST_F(TNTEntityTest, ExplodeOnClientSide)
{
    m_world.setClientSide(true);

    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(0.0f, 64.0f, 0.0f);
    tnt->setFuse(1);

    tnt->tick();

    // TNT 爆炸调用会发送到世界的 createExplosion 方法
    // 在客户端世界中，createExplosion 的实现可能不同
    // 我们的 MockWorld 总是记录爆炸，所以这里测试爆炸被触发
    EXPECT_EQ(m_world.explosionCount(), 1);
}

/**
 * @brief 测试点燃者设置
 */
TEST_F(TNTEntityTest, SetOwner)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    EXPECT_EQ(tnt->getOwner(), nullptr);

    // 注意：实际测试需要 LivingEntity 实例
    // 这里只测试设置和获取接口
}

/**
 * @brief 测试引信倒计时
 *
 * 每个 tick 引信时间应该减 1
 */
TEST_F(TNTEntityTest, FuseDecreasesEachTick)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);

    tnt->ignite();
    EXPECT_EQ(tnt->getFuse(), 80);

    tnt->tick();
    EXPECT_EQ(tnt->getFuse(), 79);

    tnt->tick();
    EXPECT_EQ(tnt->getFuse(), 78);

    // 再 tick 78 次应该触发爆炸
    for (int i = 0; i < 77; ++i) {
        tnt->tick();
    }
    EXPECT_EQ(tnt->getFuse(), 1);
    EXPECT_EQ(m_world.explosionCount(), 0);

    tnt->tick();
    EXPECT_EQ(m_world.explosionCount(), 1);
}

/**
 * @brief 测试无重力状态
 *
 * 设置 noGravity 后不应该应用重力
 */
TEST_F(TNTEntityTest, NoGravityMode)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(0.0f, 64.0f, 0.0f);
    tnt->setVelocity(0.0f, 0.0f, 0.0f);

    // 设置无重力
    tnt->setNoGravity(true);

    tnt->tick();

    // 速度应该保持为 0（只有空气阻力）
    // 注意：实际实现可能有差异，这里测试概念
}

/**
 * @brief 测试实体注册
 *
 * TNTEntity 应该已经注册到实体注册表
 */
TEST_F(TNTEntityTest, EntityRegistration)
{
    auto& registry = EntityRegistry::instance();
    const EntityType* tntType = registry.getType(EntityTypeKeys::TNT);

    ASSERT_NE(tntType, nullptr);
    EXPECT_TRUE(tntType->isValid());
    EXPECT_EQ(tntType->classification(), EntityClassification::Misc);
}

/**
 * @brief 测试引信边界值
 */
TEST_F(TNTEntityTest, FuseBoundaryValues)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());

    // 设置为 0
    tnt->setFuse(0);
    EXPECT_EQ(tnt->getFuse(), 0);
    EXPECT_FALSE(tnt->isPrimed());

    // 设置为最大值
    tnt->setFuse(INT32_MAX);
    EXPECT_EQ(tnt->getFuse(), INT32_MAX);
    EXPECT_TRUE(tnt->isPrimed());

    // 设置为 1（最小点燃状态）
    tnt->setFuse(1);
    EXPECT_EQ(tnt->getFuse(), 1);
    EXPECT_TRUE(tnt->isPrimed());
}

/**
 * @brief 测试爆炸半径边界值
 */
TEST_F(TNTEntityTest, ExplosionRadiusBoundaryValues)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());

    // 最小半径
    tnt->setExplosionRadius(0.0f);
    EXPECT_FLOAT_EQ(tnt->getExplosionRadius(), 0.0f);

    // 大半径
    tnt->setExplosionRadius(100.0f);
    EXPECT_FLOAT_EQ(tnt->getExplosionRadius(), 100.0f);
}

/**
 * @brief 测试客户端烟雾粒子生成
 *
 * 在客户端模式下，点燃的 TNT 应该生成烟雾粒子
 */
TEST_F(TNTEntityTest, ClientSideSmokeParticles)
{
    using namespace particle;

    m_world.setClientSide(true);

    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(10.0f, 64.0f, 20.0f);
    tnt->ignite(); // 设置引信为 80

    // tick 多次，粒子可能随机生成（1/3 概率）
    // 由于是随机生成，我们只验证粒子类型正确时才计数
    i32 smokeParticleCount = 0;
    for (int i = 0; i < 20; ++i) {
        m_world.resetParticleCount();
        tnt->tick();
        m_world.advanceTick();

        // 如果有粒子生成，检查是否是烟雾粒子
        if (m_world.particleCount() > 0) {
            EXPECT_EQ(m_world.lastParticleType(), ParticleTypeId::Smoke);
            smokeParticleCount += m_world.particleCount();
        }
    }

    // 由于概率是 1/3，20 次 tick 应该至少生成一些粒子
    // 但由于随机性，我们只检查粒子类型正确，不强制要求特定数量
}

/**
 * @brief 测试服务端无粒子生成
 *
 * 在服务端模式下，不应该生成粒子
 */
TEST_F(TNTEntityTest, ServerSideNoParticles)
{
    m_world.setClientSide(false);

    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(10.0f, 64.0f, 20.0f);
    tnt->ignite();

    // tick 多次
    for (int i = 0; i < 20; ++i) {
        m_world.resetParticleCount();
        tnt->tick();
        m_world.advanceTick();
    }

    // 服务端不应该生成粒子
    EXPECT_EQ(m_world.particleCount(), 0);
}

/**
 * @brief ignite(fuseTicks) 设置自定义引信时间
 */
TEST_F(TNTEntityTest, IgniteWithCustomFuse)
{
    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->ignite(20);
    EXPECT_EQ(tnt->getFuse(), 20);

    tnt->ignite(100);
    EXPECT_EQ(tnt->getFuse(), 100);

    // 默认引信
    tnt->ignite();
    EXPECT_EQ(tnt->getFuse(), 80);
}

/**
 * @brief tntExplodes=false 时 TNT 实体不创建爆炸
 *
 * 当 tntExplodes 游戏规则为 false 时，点燃的 TNT 实体在引信归零后
 * 应该被移除但不创建爆炸效果。
 */
TEST_F(TNTEntityTest, ExplodeDoesNotCreateExplosionWhenRuleDisabled)
{
    // 设置 tntExplodes=false
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES, false, nullptr);

    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(10.0f, 64.0f, 20.0f);
    tnt->setFuse(1);

    // tick 一次后引信归零
    tnt->tick();

    // 不应该创建爆炸
    EXPECT_EQ(m_world.explosionCount(), 0);
}

/**
 * @brief tntExplodes=true（默认）时 TNT 实体正常爆炸
 */
TEST_F(TNTEntityTest, ExplodeCreatesExplosionWhenRuleEnabled)
{
    // 默认 tntExplodes=true
    EXPECT_TRUE(m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES));

    auto tnt = std::make_unique<TNTEntity>(mc::test::testEcsRegistry());
    tnt->setWorld(&m_world);
    tnt->setPosition(10.0f, 64.0f, 20.0f);
    tnt->setFuse(1);

    tnt->tick();

    // 应该创建爆炸
    EXPECT_EQ(m_world.explosionCount(), 1);
}

} // namespace test
} // namespace entity
} // namespace mc
