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

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/breeze/BreezeEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"

#include <memory>
#include <vector>

using namespace mc;
using mc::entity::WindChargeEntity;

// ============================================================================
// 测试访问器：通过 friend 声明访问 BreezeEntity 的 private shootWindCharge
// ============================================================================
//
// BreezeEntity 被声明为 final，无法通过继承子类暴露 private 方法。
// 测试中通过 BreezeShootTestAccessor 这个 friend 类以间接方式访问
// private shootWindCharge() 与 protected setShootCooldown()，
// 避免修改生产代码的可见性。
// BreezeEntity.hpp 中已声明 `friend class test::BreezeShootTestAccessor;`。

namespace mc::test {

class BreezeShootTestAccessor {
public:
    explicit BreezeShootTestAccessor(BreezeEntity& breeze)
        : m_breeze(breeze)
    {}

    void shootWindCharge() { m_breeze.shootWindCharge(); }

    void setShootCooldown(i32 ticks) { m_breeze.setShootCooldown(ticks); }

private:
    BreezeEntity& m_breeze;
};

} // namespace mc::test

namespace mc {
namespace {

/// 测试用 LivingEntity 子类：用于作为 Breeze 的攻击目标。
class TestTargetEntity : public LivingEntity {
public:
    explicit TestTargetEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr)
    {}

    [[nodiscard]] std::string getTypeId() const override { return "minecraft:test_target"; }

    // 测试中允许直接控制目标高度，以验证不同 partialY 比例下的瞄准点
    [[nodiscard]] f32 height() const override { return m_height; }
    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 eyeHeight() const override { return m_height * 0.85f; }

    void setHeight(f32 h) { m_height = h; }

private:
    f32 m_height = 1.8f;
};

/// 测试用载具实体：作为目标的骑乘对象，用于触发 isRiding() 分支。
class TestVehicleEntity : public LivingEntity {
public:
    explicit TestVehicleEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr)
    {}

    [[nodiscard]] std::string getTypeId() const override { return "minecraft:test_vehicle"; }
};

/**
 * @brief shootWindCharge 测试用世界桩
 *
 * 在 BaseTestWorld 基础上覆写：
 * - spawnEntity：捕获传入的 WindChargeEntity 的所有权，记录其速度向量
 * - getEntity：通过实体ID查找预注册的实体（用于 startRiding 关系建立）
 * - playSound / addParticle / addBlockParticle：空操作（避免测试噪音）
 *
 * 关键设计：spawnEntity 默认实现会销毁 entity，本桩改为接管所有权，
 * 让测试能在 shootWindCharge 调用完成后检查风弹的速度向量。
 */
class BreezeShootTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BreezeShootTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BreezeShootTestWorld::tickManager not implemented");
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return EntityInstanceId(1);
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entityById.find(id);
        return it != m_entityById.end() ? it->second : nullptr;
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 测试中忽略音效
    }

    void addBlockParticle(particle::ParticleTypeId, const Vector3&, const Vector3&, const BlockState&) override
    {
        // 测试中忽略粒子
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子
    }

    /// 注册实体到 ID 索引，便于 getEntity 查找（用于骑乘关系建立）
    void registerEntity(Entity* entity) { m_entityById[entity->id()] = entity; }

    /// 获取捕获到的所有 spawn 调用产生的实体
    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::unordered_map<EntityInstanceId, Entity*> m_entityById;
};

} // namespace
} // namespace mc

// ============================================================================
// shootWindCharge 目标 Y 坐标计算测试
// ============================================================================
//
// 本测试集验证 BreezeEntity::shootWindCharge 中目标 Y 坐标计算对齐
// MC 1.21.11 Shoot.tick() 的算法：
//   d1 = livingentity.getY(livingentity.isPassenger() ? 0.8 : 0.3) - breeze.getFiringYPosition();
//
// 关键点：
// - 非骑乘目标：partialY = 0.3（瞄准躯干下部）
// - 骑乘目标：partialY = 0.8（瞄准接近头部，避开载具碰撞盒）
// - 发射点 Y：breeze.y + breeze.height * 0.5 + 0.3（对应 MC Breeze.getFiringYPosition）
// - 通过 spawnEntity 捕获的 WindChargeEntity 的速度向量验证 dy 计算结果

class BreezeShootWindChargeTest : public ::testing::Test {
protected:
    static constexpr f32 BREEZE_HEIGHT = 1.77f;
    static constexpr f32 TARGET_HEIGHT = 1.8f;
    // MC 1.21.11 Shoot.tick(): PROJECTILE_MOVEMENT_SCALE = 0.7F
    static constexpr f32 PROJECTILE_VELOCITY = 0.7f;

    void SetUp() override
    {
        // 旋风人位于原点
        m_breeze = std::make_unique<BreezeEntity>(EntityInstanceId(1));
        m_breeze->setPosition(0.0f, 0.0f, 0.0f);
        m_breeze->setWorld(&m_world);
        m_accessor = std::make_unique<test::BreezeShootTestAccessor>(*m_breeze);

        // 目标实体位于 +X 方向 10 格，与旋风人同一高度
        m_target = std::make_unique<TestTargetEntity>(EntityInstanceId(2));
        m_target->setPosition(10.0f, 0.0f, 0.0f);
        m_target->setWorld(&m_world);
        m_target->setHeight(TARGET_HEIGHT);
        m_world.registerEntity(m_target.get());

        // 清除射击冷却，允许射击
        m_accessor->setShootCooldown(0);

        // 设置攻击目标
        m_breeze->setAttackTarget(m_target.get());
    }

    /// 获取捕获到的最后一个 WindChargeEntity 的速度向量
    [[nodiscard]] Vector3 lastSpawnedVelocity() const
    {
        const auto& spawned = m_world.spawnedEntities();
        if (spawned.empty()) {
            return Vector3(0.0f, 0.0f, 0.0f);
        }
        const auto* windCharge = dynamic_cast<const WindChargeEntity*>(spawned.back().get());
        if (windCharge == nullptr) {
            return Vector3(0.0f, 0.0f, 0.0f);
        }
        return windCharge->velocity();
    }

    BreezeShootTestWorld m_world;
    std::unique_ptr<BreezeEntity> m_breeze;
    std::unique_ptr<test::BreezeShootTestAccessor> m_accessor;
    std::unique_ptr<TestTargetEntity> m_target;
};

// ============================================================================
// 非骑乘目标：partialY = 0.3
// ============================================================================

TEST_F(BreezeShootWindChargeTest, NonRidingTarget_UsesPartialY_0_3)
{
    // 非骑乘目标应使用 partialY = 0.3
    // 期望 dy = (targetY + 0.3 * targetHeight) - (breezeY + 0.5 * breezeHeight + 0.3)
    //        = (0 + 0.3 * 1.8) - (0 + 0.5 * 1.77 + 0.3)
    //        = 0.54 - 1.185 = -0.645
    // dx = 10 - 0 = 10
    // dz = 0 - 0 = 0
    // 由于 shoot 内部归一化方向向量后乘以 velocity，期望速度向量为：
    //   v = normalize(10, -0.645, 0) * 0.7
    // 注意：Easy 难度下 inaccuracy=1，shoot 会加入确定性高斯散布（种子由实体ID和
    // ticksExisted 决定，本测试中两者均为 0，故散布为固定值）。散布量级约 0.0075，
    // 因此速度分量容差放宽至 0.02 以容纳该确定性偏移，同时仍能捕捉 partialY 用错
    // （如 0.5）导致的 ~0.04 量级回归。
    m_accessor->shootWindCharge();

    const Vector3 v = lastSpawnedVelocity();
    ASSERT_NEAR(v.x, 0.7f, 0.02f); // 归一化后 X 分量主导，约为 0.7（含散布偏移）
    EXPECT_LT(v.y, 0.0f);          // Y 分量应为负（瞄准点在发射点下方）
    EXPECT_NEAR(v.z, 0.0f, 0.02f); // Z 分量应接近 0（含散布偏移）

    // 验证 Y 分量与 X 分量的比例符合 0.3 * targetHeight - firingY 的预期
    // 期望 dy/dx = -0.645 / 10 = -0.0645
    // 散布会略微改变该比例，容差 0.01 足以容纳散布且区分 partialY=0.5（比例 -0.0285）
    const f32 expectedRatio = -0.645f / 10.0f;
    const f32 actualRatio = v.y / v.x;
    EXPECT_NEAR(actualRatio, expectedRatio, 0.01f);
}

TEST_F(BreezeShootWindChargeTest, NonRidingTarget_FiringY_EqualsHalfHeightPlus0_3)
{
    // 验证发射点 Y 坐标对齐 MC 1.21.11 Breeze.getFiringYPosition()：
    //   firingY = breeze.y + breeze.height / 2 + 0.3
    // 这里通过对比捕获的速度向量反推发射点位置：
    //   dy = targetY + 0.3 * targetHeight - firingY
    //   => firingY = targetY + 0.3 * targetHeight - dy
    // 通过 dy/dx 比例反推 dy，再计算 firingY
    // 注意：shoot 加入的确定性散布会略微改变反推的 dy，容差 0.05 足以容纳。
    m_accessor->shootWindCharge();

    const Vector3 v = lastSpawnedVelocity();
    ASSERT_GT(v.x, 0.0f);

    // 归一化向量长度应为 velocity（0.7），方向向量长度为 1
    // 原始 dx=10, dy=?, dz=0，归一化后 v.x = dx / |d| * 0.7
    // 所以 |d| = dx / v.x * 0.7 = 10 / v.x * 0.7
    const f32 dx = 10.0f;
    const f32 dirLength = dx / v.x * PROJECTILE_VELOCITY;
    const f32 dy = v.y / PROJECTILE_VELOCITY * dirLength;

    const f32 expectedDy = (0.0f + 0.3f * TARGET_HEIGHT) - (0.0f + 0.5f * BREEZE_HEIGHT + 0.3f);
    EXPECT_NEAR(dy, expectedDy, 0.05f);
}

TEST_F(BreezeShootWindChargeTest, NonRidingTarget_DoesNotUsePartialY_0_5)
{
    // 验证修复后不再使用 0.5（旧实现）
    // 旧实现 dy = 0.5 * 1.8 - 1.185 = 0.9 - 1.185 = -0.285
    // 新实现 dy = 0.3 * 1.8 - 1.185 = 0.54 - 1.185 = -0.645
    // 差异明显，新实现的 dy 应明显更小（更负）
    m_accessor->shootWindCharge();

    const Vector3 v = lastSpawnedVelocity();
    ASSERT_GT(v.x, 0.0f);
    const f32 dirLength = 10.0f / v.x * PROJECTILE_VELOCITY;
    const f32 dy = v.y / PROJECTILE_VELOCITY * dirLength;

    const f32 oldDy = 0.5f * TARGET_HEIGHT - (0.5f * BREEZE_HEIGHT + 0.3f);
    EXPECT_LT(dy, oldDy - 0.1f); // 新 dy 明显小于旧 dy
}

// ============================================================================
// 骑乘目标：partialY = 0.8
// ============================================================================

TEST_F(BreezeShootWindChargeTest, RidingTarget_UsesPartialY_0_8)
{
    // 创建一个测试载具，让目标骑乘它
    auto vehicle = std::make_unique<TestVehicleEntity>(EntityInstanceId(3));
    vehicle->setPosition(10.0f, 0.0f, 0.0f);
    vehicle->setWorld(&m_world);
    TestVehicleEntity* vehiclePtr = vehicle.get();
    m_world.registerEntity(vehiclePtr);

    // 触发 startRiding：目标作为乘客，载具作为被骑乘者
    // 注意 startRiding 会调用 vehicle->addPassenger(*target)，需要载具的 world 已设置
    ASSERT_TRUE(m_target->startRiding(*vehiclePtr));
    ASSERT_TRUE(m_target->isRiding()); // 验证目标已进入骑乘状态

    // 骑乘目标应使用 partialY = 0.8
    // 期望 dy = (targetY + 0.8 * targetHeight) - (breezeY + 0.5 * breezeHeight + 0.3)
    //        = (0 + 0.8 * 1.8) - (0 + 0.5 * 1.77 + 0.3)
    //        = 1.44 - 1.185 = 0.255
    // 此时 dy 为正（瞄准点在发射点上方）
    m_accessor->shootWindCharge();

    const Vector3 v = lastSpawnedVelocity();
    ASSERT_GT(v.x, 0.0f);
    EXPECT_GT(v.y, 0.0f);          // Y 分量应为正（瞄准点在发射点上方）
    EXPECT_NEAR(v.z, 0.0f, 0.02f); // Z 分量应接近 0（含散布偏移）

    // 验证 dy/dx 比例符合 0.8 * targetHeight - firingY 的预期
    // 期望 dy = 0.255, 期望比例 = 0.0255
    // 散布会略微改变该比例，容差 0.01 足以容纳且区分 partialY=0.3（比例 -0.0645）
    const f32 expectedDy = (0.0f + 0.8f * TARGET_HEIGHT) - (0.0f + 0.5f * BREEZE_HEIGHT + 0.3f);
    const f32 expectedRatio = expectedDy / 10.0f;
    const f32 actualRatio = v.y / v.x;
    EXPECT_NEAR(actualRatio, expectedRatio, 0.01f);
}

TEST_F(BreezeShootWindChargeTest, RidingTarget_HigherAimThanNonRiding)
{
    // 骑乘目标的瞄准点应高于非骑乘目标（0.8 > 0.3）
    // 先测非骑乘情况
    m_accessor->shootWindCharge();
    const Vector3 vNonRiding = lastSpawnedVelocity();
    const f32 ratioNonRiding = vNonRiding.y / vNonRiding.x;

    // 清除捕获的实体并重置射击冷却
    m_accessor->setShootCooldown(0);

    // 让目标骑乘载具
    auto vehicle = std::make_unique<TestVehicleEntity>(EntityInstanceId(3));
    vehicle->setPosition(10.0f, 0.0f, 0.0f);
    vehicle->setWorld(&m_world);
    TestVehicleEntity* vehiclePtr = vehicle.get();
    m_world.registerEntity(vehiclePtr);
    ASSERT_TRUE(m_target->startRiding(*vehiclePtr));

    m_accessor->shootWindCharge();
    const Vector3 vRiding = lastSpawnedVelocity();
    const f32 ratioRiding = vRiding.y / vRiding.x;

    // 骑乘时瞄准点更高，dy/dx 比例更大
    EXPECT_GT(ratioRiding, ratioNonRiding);
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(BreezeShootWindChargeTest, NoWorld_DoesNothing)
{
    // 无世界时不应崩溃，也不应产生 spawn
    BreezeEntity breeze(EntityInstanceId(10));
    breeze.setWorld(nullptr);
    test::BreezeShootTestAccessor accessor(breeze);
    accessor.setShootCooldown(0);
    accessor.shootWindCharge();
    // 无 spawn 调用即视为通过（不崩溃）
    SUCCEED();
}

TEST_F(BreezeShootWindChargeTest, NoAttackTarget_DoesNothing)
{
    // 无攻击目标时不应产生 spawn
    m_breeze->setAttackTarget(nullptr);
    m_accessor->shootWindCharge();
    EXPECT_TRUE(m_world.spawnedEntities().empty());
}

TEST_F(BreezeShootWindChargeTest, ShootCooldownActive_DoesNothing)
{
    // 射击冷却中不应产生 spawn
    m_accessor->setShootCooldown(20);
    m_accessor->shootWindCharge();
    EXPECT_TRUE(m_world.spawnedEntities().empty());
}

TEST_F(BreezeShootWindChargeTest, ShootCooldownZero_AllowsShooting)
{
    // 射击冷却为 0 时应产生 spawn
    m_accessor->setShootCooldown(0);
    m_accessor->shootWindCharge();
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

TEST_F(BreezeShootWindChargeTest, SpawnedEntityIsWindCharge)
{
    // 验证 spawn 出的实体确实是 WindChargeEntity
    m_accessor->shootWindCharge();
    ASSERT_EQ(m_world.spawnedEntities().size(), 1u);
    EXPECT_NE(dynamic_cast<WindChargeEntity*>(m_world.spawnedEntities().back().get()), nullptr);
}

TEST_F(BreezeShootWindChargeTest, SpawnedWindCharge_PositionMatchesFiringY)
{
    // 验证 WindChargeEntity 的初始位置对应 Breeze.getFiringYPosition()
    //   firingPos = (breeze.x, breeze.y + breeze.height * 0.5 + 0.3, breeze.z)
    m_accessor->shootWindCharge();
    ASSERT_EQ(m_world.spawnedEntities().size(), 1u);

    const auto* windCharge = dynamic_cast<WindChargeEntity*>(m_world.spawnedEntities().back().get());
    ASSERT_NE(windCharge, nullptr);

    const f32 expectedFiringY = 0.0f + BREEZE_HEIGHT * 0.5f + 0.3f;
    EXPECT_FLOAT_EQ(windCharge->x(), 0.0f);
    EXPECT_FLOAT_EQ(windCharge->y(), expectedFiringY);
    EXPECT_FLOAT_EQ(windCharge->z(), 0.0f);
}

TEST_F(BreezeShootWindChargeTest, SetsShootCooldownAfterShooting)
{
    // 射击后应设置射击冷却（20 ticks）
    m_accessor->shootWindCharge();
    // 通过再次射击无反应验证（虽然不直接，但可观测）
    m_accessor->shootWindCharge();
    // 第二次射击不应产生新的 spawn（冷却中）
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}
