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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/breeze/BreezeEntity.hpp"
#include "common/entity/entities/projectile/ProjectileDeflection.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathConstants.hpp"

namespace mc {
namespace {

using entity::RayTraceResult;
using entity::RayTraceResultType;

// ============================================================================
// 测试辅助实体
// ============================================================================

/**
 * @brief 测试弹射物，暴露 onEntityHit 调用计数
 */
class CountingProjectile : public entity::ProjectileEntity {
public:
    explicit CountingProjectile(EntityInstanceId id)
        : ProjectileEntity(id, mc::test::testEcsRegistry())
    {}

    [[nodiscard]] std::string getTypeId() const override { return "minecraft:arrow"; }

    int entityHitCount = 0;

protected:
    void onEntityHit(const RayTraceResult& /*result*/) override { ++entityHitCount; }
};

/**
 * @brief 可偏转实体（测试桩，复用 DEFLECTS_PROJECTILES 标签成员 breeze 的身份）
 *
 * typeId 设为 "minecraft:breeze"（vanilla 1.21.11 DEFLECTS_PROJECTILES 标签唯一成员），
 * 不 override deflection，用基类 Entity::deflection 查 DEFLECTS_PROJECTILES 标签返回 Reverse。
 * 以此测试"标签驱动的基类偏转"路径（区别于 BreezeEntity 自身 deflection override 的路径，
 * 后者由 BreezeDeflectsNonWindChargeProjectiles/BreezeDoesNotDeflectWindCharge 覆盖）。
 *
 * 注意：此前用 typeId="minecraft:shulker" 作为标签成员，但 shulker 在 vanilla 不在
 * DEFLECTS_PROJECTILES 标签中（vanilla 潜影贝不偏转投射物），Cubium 误加 shulker 已修正移除，
 * 故测试桩改用 vanilla 真正的标签成员 breeze。
 */
class DeflectingEntity : public Entity {
public:
    explicit DeflectingEntity(EntityInstanceId id)
        : Entity(id, nullptr, mc::test::testEcsRegistry())
    {
        setTypeId("minecraft:breeze");
    }

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }
    [[nodiscard]] std::string getTypeId() const override { return "minecraft:breeze"; }
    [[nodiscard]] bool canBeCollidedWith() const override { return true; }
};

/**
 * @brief 不可偏转实体
 */
class NonDeflectingEntity : public Entity {
public:
    explicit NonDeflectingEntity(EntityInstanceId id)
        : Entity(id, nullptr, mc::test::testEcsRegistry())
    {
        setTypeId("minecraft:zombie");
    }

    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.8f; }
    [[nodiscard]] std::string getTypeId() const override { return "minecraft:zombie"; }
    [[nodiscard]] bool canBeCollidedWith() const override { return true; }
};

/**
 * @brief 自定义偏转行为的实体
 */
class CustomDeflectionEntity : public Entity {
public:
    explicit CustomDeflectionEntity(EntityInstanceId id)
        : Entity(id, nullptr, mc::test::testEcsRegistry())
    {
        setTypeId("minecraft:custom_deflector");
    }

    [[nodiscard]] f32 width() const override { return 1.0f; }
    [[nodiscard]] f32 height() const override { return 1.0f; }
    [[nodiscard]] std::string getTypeId() const override { return "minecraft:custom_deflector"; }
    [[nodiscard]] bool canBeCollidedWith() const override { return true; }

    ProjectileDeflection customDeflection = ProjectileDeflection::None;

    [[nodiscard]] ProjectileDeflection deflection(const entity::ProjectileEntity& /*projectile*/) const override
    {
        return customDeflection;
    }
};

/**
 * @brief 偏转测试专用世界
 */
class DeflectionTestWorld : public mc::test::BaseTestWorld {
public:
    DeflectionTestWorld() = default;

    void registerEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entities[entity->id()] = entity;
        }
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

private:
    std::unordered_map<EntityInstanceId, Entity*> m_entities;
};

// ============================================================================
// 测试夹具
// ============================================================================

class ProjectileDeflectionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<DeflectionTestWorld>();
        // 确保 EntityTypeTags 已初始化
        if (!EntityTypeTags::isInitialized()) {
            entity::VanillaEntities::registerAll();
            EntityTypeTags::initialize();
        }
    }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<DeflectionTestWorld> m_world;
};

// ============================================================================
// ProjectileDeflection 枚举测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, DeflectionEnumValues)
{
    EXPECT_EQ(ProjectileDeflection::None, ProjectileDeflection::None);
    EXPECT_EQ(ProjectileDeflection::Reverse, ProjectileDeflection::Reverse);
    EXPECT_EQ(ProjectileDeflection::AimDeflect, ProjectileDeflection::AimDeflect);
    EXPECT_EQ(ProjectileDeflection::MomentumDeflect, ProjectileDeflection::MomentumDeflect);

    EXPECT_NE(ProjectileDeflection::None, ProjectileDeflection::Reverse);
    EXPECT_NE(ProjectileDeflection::Reverse, ProjectileDeflection::AimDeflect);
    EXPECT_NE(ProjectileDeflection::AimDeflect, ProjectileDeflection::MomentumDeflect);
}

// ============================================================================
// Entity::deflection() 默认行为测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, DefaultDeflectionNoneForNonTaggedEntity)
{
    // 不属于 DEFLECTS_PROJECTILES 标签的实体默认返回 None
    NonDeflectingEntity entity(EntityInstanceId(1));
    CountingProjectile projectile(EntityInstanceId(2));

    EXPECT_EQ(entity.deflection(projectile), ProjectileDeflection::None);
}

TEST_F(ProjectileDeflectionTest, DefaultDeflectionReverseForTaggedEntity)
{
    // 属于 DEFLECTS_PROJECTILES 标签的实体（breeze）默认返回 Reverse
    DeflectingEntity entity(EntityInstanceId(1));
    CountingProjectile projectile(EntityInstanceId(2));

    EXPECT_EQ(entity.deflection(projectile), ProjectileDeflection::Reverse);
}

// ============================================================================
// BreezeEntity::deflection() 测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, BreezeDeflectsNonWindChargeProjectiles)
{
    // 旋风人偏转非风弹投射物
    BreezeEntity breeze(EntityInstanceId(1), mc::test::testEcsRegistry());
    breeze.setTypeId("minecraft:breeze");
    breeze.setWorld(m_world.get());
    CountingProjectile projectile(EntityInstanceId(2));

    ProjectileDeflection result = breeze.deflection(projectile);
    EXPECT_EQ(result, ProjectileDeflection::Reverse);
}

TEST_F(ProjectileDeflectionTest, BreezeDoesNotDeflectWindCharge)
{
    // 旋风人不偏转风弹
    BreezeEntity breeze(EntityInstanceId(1), mc::test::testEcsRegistry());
    breeze.setWorld(m_world.get());
    entity::WindChargeEntity windCharge(EntityInstanceId(2), mc::test::testEcsRegistry());

    ProjectileDeflection result = breeze.deflection(windCharge);
    EXPECT_EQ(result, ProjectileDeflection::None);
}

// ============================================================================
// applyProjectileDeflection() Reverse 测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, ReverseDeflectionModifiesVelocity)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    projectile.setVelocity(1.0f, 2.0f, 3.0f);
    Vector3 originalVelocity = projectile.velocity();

    bool result = applyProjectileDeflection(ProjectileDeflection::Reverse, projectile, deflector);
    EXPECT_TRUE(result);

    // Reverse 偏转：速度乘以 -0.5
    Vector3 expectedVelocity = originalVelocity * -0.5f;
    EXPECT_NEAR(projectile.velocity().x, expectedVelocity.x, 0.001f);
    EXPECT_NEAR(projectile.velocity().y, expectedVelocity.y, 0.001f);
    EXPECT_NEAR(projectile.velocity().z, expectedVelocity.z, 0.001f);
}

TEST_F(ProjectileDeflectionTest, ReverseDeflectionModifiesYaw)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    projectile.setVelocity(1.0f, 0.0f, 0.0f);
    const f32 originalYaw = 45.0f;
    const f32 originalPrevYaw = 44.0f;
    projectile.setYaw(originalYaw);
    projectile.setPrevYaw(originalPrevYaw);

    applyProjectileDeflection(ProjectileDeflection::Reverse, projectile, deflector);

    // 偏航角应该增加 170~190 度（随机值）
    f32 yawDelta = projectile.yaw() - originalYaw;
    EXPECT_GE(yawDelta, 170.0f);
    EXPECT_LE(yawDelta, 190.0f);

    // prevYaw 也应该增加相同的量
    f32 prevYawDelta = projectile.prevYaw() - originalPrevYaw;
    EXPECT_FLOAT_EQ(yawDelta, prevYawDelta);
}

TEST_F(ProjectileDeflectionTest, ReverseDeflectionChangesShooter)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity originalShooter(EntityInstanceId(2));
    NonDeflectingEntity deflector(EntityInstanceId(3));

    projectile.setWorld(m_world.get());
    m_world->registerEntity(&originalShooter);
    m_world->registerEntity(&deflector);

    projectile.setShooter(&originalShooter);
    ASSERT_NE(projectile.getShooter(), nullptr);
    EXPECT_EQ(projectile.getShooter()->id(), originalShooter.id());

    applyProjectileDeflection(ProjectileDeflection::Reverse, projectile, deflector);

    // 偏转后，偏转者成为新的发射者
    ASSERT_NE(projectile.getShooter(), nullptr);
    EXPECT_EQ(projectile.getShooter()->id(), deflector.id());
}

// ============================================================================
// applyProjectileDeflection() AimDeflect 测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, AimDeflectSetsVelocityToDeflectorLookDirection)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    // 设置弹射物初始速度（5.0 m/s 向 +X 方向）
    projectile.setVelocity(5.0f, 0.0f, 0.0f);

    // 设置偏转者面向 -Z 方向（yaw=0, pitch=0）
    deflector.setYaw(0.0f);
    deflector.setPrevYaw(0.0f);

    applyProjectileDeflection(ProjectileDeflection::AimDeflect, projectile, deflector);

    // AimDeflect: speed = 5.0, direction = (-sin(0)*cos(0), -sin(0), cos(0)*cos(0)) = (0, 0, 1)
    EXPECT_NEAR(projectile.velocity().x, 0.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().y, 0.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().z, 5.0f, 0.001f);
}

TEST_F(ProjectileDeflectionTest, AimDeflectChangesShooter)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    projectile.setWorld(m_world.get());
    m_world->registerEntity(&deflector);

    projectile.setVelocity(5.0f, 0.0f, 0.0f);
    applyProjectileDeflection(ProjectileDeflection::AimDeflect, projectile, deflector);

    ASSERT_NE(projectile.getShooter(), nullptr);
    EXPECT_EQ(projectile.getShooter()->id(), deflector.id());
}

// ============================================================================
// applyProjectileDeflection() MomentumDeflect 测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, MomentumDeflectUsesDeflectorVelocity)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    projectile.setVelocity(5.0f, 0.0f, 0.0f);
    deflector.setVelocity(0.0f, 0.0f, 2.0f);

    applyProjectileDeflection(ProjectileDeflection::MomentumDeflect, projectile, deflector);

    // 动量偏转：direction = (0, 0, 2.0) normalized = (0, 0, 1.0), speed = 5.0
    EXPECT_NEAR(projectile.velocity().x, 0.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().y, 0.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().z, 5.0f, 0.001f);
}

TEST_F(ProjectileDeflectionTest, MomentumDeflectWithZeroVelocityLeavesVelocityUnchanged)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    projectile.setVelocity(5.0f, 0.0f, 0.0f);
    deflector.setVelocity(0.0f, 0.0f, 0.0f); // 偏转者静止

    applyProjectileDeflection(ProjectileDeflection::MomentumDeflect, projectile, deflector);

    // 偏转者速度为零时，不修改弹射物速度（motionLen < 1e-4）
    EXPECT_NEAR(projectile.velocity().x, 0.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().y, 0.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().z, 0.0f, 0.001f);
}

// ============================================================================
// applyProjectileDeflection() None 测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, NoneDeflectionReturnsFalse)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    bool result = applyProjectileDeflection(ProjectileDeflection::None, projectile, deflector);
    EXPECT_FALSE(result);
}

// ============================================================================
// ProjectileEntity::deflect() 测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, DeflectMethodReturnsFalseForNone)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    bool result = projectile.deflect(ProjectileDeflection::None, deflector);
    EXPECT_FALSE(result);
}

TEST_F(ProjectileDeflectionTest, DeflectMethodReturnsTrueForReverse)
{
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    projectile.setWorld(m_world.get());
    m_world->registerEntity(&deflector);

    projectile.setVelocity(1.0f, 2.0f, 3.0f);

    bool result = projectile.deflect(ProjectileDeflection::Reverse, deflector);
    EXPECT_TRUE(result);
}

TEST_F(ProjectileDeflectionTest, DeflectCallsOnDeflection)
{
    class DeflectionTrackingProjectile : public entity::ProjectileEntity {
    public:
        explicit DeflectionTrackingProjectile(EntityInstanceId id)
            : ProjectileEntity(id, mc::test::testEcsRegistry())
        {}

        [[nodiscard]] std::string getTypeId() const override { return "minecraft:arrow"; }

        bool onDeflectionCalled = false;
        bool wasPlayerDeflect = false;

    protected:
        void onDeflection(bool wasPlayerDeflect) override
        {
            onDeflectionCalled = true;
            this->wasPlayerDeflect = wasPlayerDeflect;
        }
    };

    DeflectionTrackingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity deflector(EntityInstanceId(2));

    projectile.setWorld(m_world.get());
    m_world->registerEntity(&deflector);

    projectile.setVelocity(1.0f, 0.0f, 0.0f);
    projectile.deflect(ProjectileDeflection::Reverse, deflector, false);

    EXPECT_TRUE(projectile.onDeflectionCalled);
    EXPECT_FALSE(projectile.wasPlayerDeflect);

    // 测试 wasPlayerDeflect=true 的情况
    projectile.onDeflectionCalled = false;
    projectile.deflect(ProjectileDeflection::Reverse, deflector, true);

    EXPECT_TRUE(projectile.onDeflectionCalled);
    EXPECT_TRUE(projectile.wasPlayerDeflect);
}

// ============================================================================
// ProjectileEntity::onImpact() 偏转检查测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, OnImpactDeflectionPreventsEntityHit)
{
    // 当实体返回非 None 偏转类型时，onImpact 不调用 onEntityHit
    CountingProjectile projectile(EntityInstanceId(1));
    DeflectingEntity deflector(EntityInstanceId(2));

    projectile.setWorld(m_world.get());

    RayTraceResult hitResult = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector);

    projectile.onImpact(hitResult);

    // DeflectingEntity（typeId=breeze，在 DEFLECTS_PROJECTILES 标签中）弹射物应被偏转，不调用 onEntityHit
    EXPECT_EQ(projectile.entityHitCount, 0);
}

TEST_F(ProjectileDeflectionTest, OnImpactNoDeflectionCallsEntityHit)
{
    // 当实体返回 None 偏转类型时，onImpact 正常调用 onEntityHit
    CountingProjectile projectile(EntityInstanceId(1));
    NonDeflectingEntity target(EntityInstanceId(2));

    projectile.setWorld(m_world.get());

    RayTraceResult hitResult = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &target);

    projectile.onImpact(hitResult);

    // 僵尸不在 DEFLECTS_PROJECTILES 标签中，弹射物正常命中
    EXPECT_EQ(projectile.entityHitCount, 1);
}

TEST_F(ProjectileDeflectionTest, OnImpactCustomDeflectionPreventsEntityHit)
{
    // 使用自定义偏转行为的实体
    CountingProjectile projectile(EntityInstanceId(1));
    CustomDeflectionEntity deflector(EntityInstanceId(2));
    deflector.customDeflection = ProjectileDeflection::Reverse;

    projectile.setWorld(m_world.get());

    RayTraceResult hitResult = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector);

    projectile.onImpact(hitResult);

    // 自定义偏转实体返回 Reverse，不调用 onEntityHit
    EXPECT_EQ(projectile.entityHitCount, 0);
}

TEST_F(ProjectileDeflectionTest, OnImpactCustomNoneDeflectionCallsEntityHit)
{
    // 自定义偏转实体返回 None，正常调用 onEntityHit
    CountingProjectile projectile(EntityInstanceId(1));
    CustomDeflectionEntity deflector(EntityInstanceId(2));
    deflector.customDeflection = ProjectileDeflection::None;

    projectile.setWorld(m_world.get());

    RayTraceResult hitResult = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector);

    projectile.onImpact(hitResult);

    EXPECT_EQ(projectile.entityHitCount, 1);
}

// ============================================================================
// m_lastDeflectedById 防止连续偏转测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, SameDeflectorCannotDeflectTwiceInARow)
{
    CountingProjectile projectile(EntityInstanceId(1));
    DeflectingEntity deflector(EntityInstanceId(2));

    projectile.setWorld(m_world.get());
    projectile.setVelocity(1.0f, 0.0f, 0.0f);

    // 第一次命中：被偏转
    RayTraceResult hit1 = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector);
    projectile.onImpact(hit1);
    EXPECT_EQ(projectile.entityHitCount, 0); // 偏转，未命中

    // 第二次命中同一实体：由于该实体仍返回非 None 偏转类型，
    // onImpact 仍会进入偏转分支（不调用 onEntityHit），
    // 但由于 m_lastDeflectedById 防护，deflect() 不会被再次调用
    // 对应 MC Java: entity != this.lastDeflectedBy 检查失败时跳过 deflect()，
    // 但方法仍返回非 None 偏转类型，不会 fall through 到 onHit()
    RayTraceResult hit2 = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector);
    projectile.onImpact(hit2);
    EXPECT_EQ(projectile.entityHitCount, 0); // 仍不调用 onEntityHit
}

TEST_F(ProjectileDeflectionTest, DifferentDeflectorCanDeflectSequentially)
{
    CountingProjectile projectile(EntityInstanceId(1));
    DeflectingEntity deflector1(EntityInstanceId(2));
    DeflectingEntity deflector2(EntityInstanceId(3));

    projectile.setWorld(m_world.get());
    projectile.setVelocity(1.0f, 0.0f, 0.0f);

    // 第一个偏转者
    RayTraceResult hit1 = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector1);
    projectile.onImpact(hit1);
    EXPECT_EQ(projectile.entityHitCount, 0); // 被偏转

    // 第二个不同的偏转者
    projectile.setVelocity(1.0f, 0.0f, 0.0f);
    RayTraceResult hit2 = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector2);
    projectile.onImpact(hit2);
    EXPECT_EQ(projectile.entityHitCount, 0); // 也被偏转（不同的偏转者）
}

TEST_F(ProjectileDeflectionTest, SameDeflectorTwiceDoesNotCallDeflectAgain)
{
    // 验证同一实体连续偏转时，deflect() 不会被再次调用（速度不变）
    CountingProjectile projectile(EntityInstanceId(1));
    DeflectingEntity deflector(EntityInstanceId(2));

    projectile.setWorld(m_world.get());
    projectile.setVelocity(2.0f, 0.0f, 0.0f);

    // 第一次命中：被偏转，速度改变
    RayTraceResult hit1 = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector);
    projectile.onImpact(hit1);

    // Reverse 偏转后速度应乘以 -0.5
    EXPECT_NEAR(projectile.velocity().x, -1.0f, 0.001f);
    const Vector3 afterFirstDeflect = projectile.velocity();

    // 第二次命中同一实体：偏转被跳过，速度不应再改变
    RayTraceResult hit2 = RayTraceResult::entity(Vector3(0.0f, 0.0f, 0.0f), &deflector);
    projectile.onImpact(hit2);

    // 速度应该和第一次偏转后一样，没有被再次偏转
    EXPECT_NEAR(projectile.velocity().x, afterFirstDeflect.x, 0.001f);
    EXPECT_NEAR(projectile.velocity().y, afterFirstDeflect.y, 0.001f);
    EXPECT_NEAR(projectile.velocity().z, afterFirstDeflect.z, 0.001f);
}

// ============================================================================
// onImpact 对 Block 和 Miss 结果不触发偏转测试
// ============================================================================

TEST_F(ProjectileDeflectionTest, OnImpactBlockHitDoesNotTriggerDeflection)
{
    CountingProjectile projectile(EntityInstanceId(1));

    RayTraceResult blockResult = RayTraceResult::block(Vector3(0.0f, 0.0f, 0.0f), BlockPos(0, 0, 0), Direction::Up);

    projectile.onImpact(blockResult);

    // 方块命中后速度归零（onBlockHit 默认行为）
    EXPECT_NEAR(projectile.velocity().x, 0.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().y, 0.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().z, 0.0f, 0.001f);
}

TEST_F(ProjectileDeflectionTest, OnImpactMissDoesNotTriggerDeflection)
{
    CountingProjectile projectile(EntityInstanceId(1));
    projectile.setVelocity(1.0f, 2.0f, 3.0f);

    RayTraceResult missResult = RayTraceResult::miss();
    projectile.onImpact(missResult);

    // Miss 不修改速度
    EXPECT_NEAR(projectile.velocity().x, 1.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().y, 2.0f, 0.001f);
    EXPECT_NEAR(projectile.velocity().z, 3.0f, 0.001f);
    EXPECT_EQ(projectile.entityHitCount, 0);
}

} // namespace
} // namespace mc
