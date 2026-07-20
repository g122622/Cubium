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
 * IMPLIED, NONINFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

/**
 * @brief 带乘客管理功能的测试实体
 *
 * 支持 startRiding/stopRiding 和 getVehicle/isRiding，
 * 用于测试 ProjectileEntity::canHitEntity 中骑乘同一载具的逻辑。
 */
class TestRideableEntity : public Entity {
public:
    explicit TestRideableEntity(EntityInstanceId id)
        : Entity(id)
    {}

    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.8f; }
    [[nodiscard]] std::string getTypeId() const override { return "test:rideable"; }
    [[nodiscard]] bool canBeCollidedWith() const override { return true; }
};

/**
 * @brief 测试弹射物
 */
class TestProjectile : public entity::ProjectileEntity {
public:
    explicit TestProjectile(EntityInstanceId id)
        : ProjectileEntity(id)
    {}

    [[nodiscard]] std::string getTypeId() const override { return "test:projectile"; }
};

/**
 * @brief canHitEntity 专用测试世界
 *
 * 重写 getEntity 以支持骑乘相关的实体查找和 getShooter 查找。
 */
class CanHitEntityTestWorld : public test::BaseTestWorld {
public:
    CanHitEntityTestWorld() = default;

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
// canHitEntity 测试
// ============================================================================

class CanHitEntityTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<CanHitEntityTestWorld>(); }
    void TearDown() override { m_world.reset(); }

    std::unique_ptr<CanHitEntityTestWorld> m_world;
};

/**
 * @brief 测试弹射物不能命中已移除的实体
 */
TEST_F(CanHitEntityTest, CannotHitRemovedEntity)
{
    TestProjectile projectile(EntityInstanceId(1));
    TestRideableEntity target(EntityInstanceId(2));

    target.remove();

    EXPECT_FALSE(projectile.canHitEntity(target));
}

/**
 * @brief 测试弹射物可以命中活着的可碰撞实体
 */
TEST_F(CanHitEntityTest, CanHitCollidableAliveEntity)
{
    TestProjectile projectile(EntityInstanceId(1));
    TestRideableEntity target(EntityInstanceId(2));

    // 活着的可碰撞实体可以被命中
    EXPECT_TRUE(projectile.canHitEntity(target));
}

/**
 * @brief 测试弹射物未离开发射者时不能命中发射者自身
 *
 * 对应 MC Java Projectile.canHitEntity: 当 leftShooter 为 false 且
 * shooter == target 时，isRidingSameEntity(target) 返回 true
 * （两者都未骑乘时，getLowestRidingEntity() 返回自身，指针相等），
 * 因此 canHitEntity 返回 false。
 *
 * 注意：getShooter() 通过 world->getEntity() 查找发射者，
 * 所以弹射物和发射者都需要设置到世界中。
 */
TEST_F(CanHitEntityTest, CannotHitShooterBeforeLeaving)
{
    TestRideableEntity shooter(EntityInstanceId(1));
    TestProjectile projectile(EntityInstanceId(2));

    // 设置世界并注册实体，以便 getShooter() 能找到发射者
    shooter.setWorld(m_world.get());
    projectile.setWorld(m_world.get());
    m_world->registerEntity(&shooter);
    m_world->registerEntity(&projectile);

    projectile.setShooter(&shooter);
    // m_leftShooter 默认为 false

    // 发射者未离开时，不能命中自己
    // isRidingSameEntity 两者都未骑乘，getLowestRidingEntity() 返回自身
    // shooter.isRidingSameEntity(shooter) == true（自身比较）
    EXPECT_FALSE(projectile.canHitEntity(shooter));
}

/**
 * @brief 测试无发射者时可以命中任何可碰撞实体
 */
TEST_F(CanHitEntityTest, CanHitAnyEntityWhenNoShooter)
{
    TestRideableEntity target(EntityInstanceId(2));
    TestProjectile projectile(EntityInstanceId(3));

    // 不设置发射者，可以命中任何可碰撞实体
    EXPECT_TRUE(projectile.canHitEntity(target));
}

/**
 * @brief 测试弹射物未离开时可以命中与发射者不同载具的实体
 *
 * 当发射者和目标未骑乘同一载具时，isRidingSameEntity 返回 false，
 * canHitEntity 应返回 true。
 */
TEST_F(CanHitEntityTest, CanHitEntityNotOnSameVehicle)
{
    TestRideableEntity shooter(EntityInstanceId(1));
    TestRideableEntity target(EntityInstanceId(2));
    TestProjectile projectile(EntityInstanceId(3));

    // 设置世界并注册实体
    shooter.setWorld(m_world.get());
    projectile.setWorld(m_world.get());
    m_world->registerEntity(&shooter);
    m_world->registerEntity(&projectile);

    projectile.setShooter(&shooter);
    // m_leftShooter 默认为 false
    // shooter 和 target 都没有骑乘，所以 getLowestRidingEntity() 返回自身
    // shooter.isRidingSameEntity(target) == false（不同实体）
    EXPECT_TRUE(projectile.canHitEntity(target));
}

/**
 * @brief 测试弹射物未离开时不能命中骑乘同一载具的实体
 *
 * 对应 MC Java 的 isPassengerOfSameVehicle 逻辑。
 * 当两个实体骑乘同一载具时，getLowestRidingEntity() 返回同一载具，
 * 因此 isRidingSameEntity 返回 true，canHitEntity 返回 false。
 */
TEST_F(CanHitEntityTest, CannotHitEntityOnSameVehicle)
{
    entity::BoatEntity vehicle(entity::BoatEntity::Type::OAK);
    // BoatEntity 默认 EntityInstanceId 为 0，等同于 INVALID_ENTITY_ID，
    // 会导致骑乘系统无法正常工作，需要设置为有效的非零 ID
    vehicle.setId(EntityInstanceId(100));

    TestRideableEntity shooter(EntityInstanceId(10));
    TestRideableEntity target(EntityInstanceId(11));
    TestProjectile projectile(EntityInstanceId(12));

    // 所有实体需要设置到世界中，以便 startRiding/getEntity 正常工作
    vehicle.setWorld(m_world.get());
    shooter.setWorld(m_world.get());
    target.setWorld(m_world.get());
    projectile.setWorld(m_world.get());

    // 注册所有实体到测试世界中
    m_world->registerEntity(&vehicle);
    m_world->registerEntity(&shooter);
    m_world->registerEntity(&target);
    m_world->registerEntity(&projectile);

    // 让射击者和目标都骑乘同一艘船
    bool shooterRode = shooter.startRiding(vehicle);
    ASSERT_TRUE(shooterRode) << "射击者应该能成功骑乘船只";
    bool targetRode = target.startRiding(vehicle);
    ASSERT_TRUE(targetRode) << "目标应该能成功骑乘船只";

    projectile.setShooter(&shooter);
    // m_leftShooter 默认为 false

    // 两人骑乘同一载具，不能互相命中
    EXPECT_FALSE(projectile.canHitEntity(target));
}

// ============================================================================
// canBeHitByProjectile 测试
// ============================================================================

/**
 * @brief 不可碰撞的实体不能被弹射物命中
 *
 * canBeHitByProjectile() 默认返回 isAlive() && canBeCollidedWith()，
 * 当 canBeCollidedWith() 返回 false 时，canBeHitByProjectile() 也返回 false。
 */
TEST_F(CanHitEntityTest, CannotHitNonCollidableEntity)
{
    class NonCollidableEntity : public Entity {
    public:
        explicit NonCollidableEntity(EntityInstanceId id)
            : Entity(id)
        {}
        [[nodiscard]] f32 width() const override { return 0.6f; }
        [[nodiscard]] f32 height() const override { return 1.8f; }
        [[nodiscard]] std::string getTypeId() const override { return "test:non_collidable"; }
        [[nodiscard]] bool canBeCollidedWith() const override { return false; }
    };

    TestProjectile projectile(EntityInstanceId(1));
    NonCollidableEntity target(EntityInstanceId(2));

    // canBeCollidedWith() 返回 false → canBeHitByProjectile() 返回 false → canHitEntity 返回 false
    EXPECT_FALSE(target.canBeHitByProjectile());
    EXPECT_FALSE(projectile.canHitEntity(target));
}

/**
 * @brief 已死亡的实体不能被弹射物命中
 *
 * canBeHitByProjectile() 默认包含 isAlive() 检查，
 * 已死亡的实体 isAlive() 返回 false，因此 canBeHitByProjectile() 返回 false。
 */
TEST_F(CanHitEntityTest, CannotHitDeadEntity)
{
    class DeadEntity : public Entity {
    public:
        explicit DeadEntity(EntityInstanceId id)
            : Entity(id)
        {}
        [[nodiscard]] f32 width() const override { return 0.6f; }
        [[nodiscard]] f32 height() const override { return 1.8f; }
        [[nodiscard]] std::string getTypeId() const override { return "test:dead"; }
        [[nodiscard]] bool canBeCollidedWith() const override { return true; }
        [[nodiscard]] bool isAlive() const override { return false; }
    };

    TestProjectile projectile(EntityInstanceId(1));
    DeadEntity target(EntityInstanceId(2));

    // isAlive() 返回 false → canBeHitByProjectile() 返回 false → canHitEntity 返回 false
    EXPECT_FALSE(target.canBeHitByProjectile());
    EXPECT_FALSE(projectile.canHitEntity(target));
}

/**
 * @brief 可碰撞且存活的实体可以被弹射物命中
 *
 * canBeHitByProjectile() = isAlive() && canBeCollidedWith()
 * 当两者都为 true 时，canBeHitByProjectile() 返回 true。
 */
TEST_F(CanHitEntityTest, CanHitByProjectileWhenCollidableAndAlive)
{
    TestRideableEntity target(EntityInstanceId(2));
    TestProjectile projectile(EntityInstanceId(1));

    EXPECT_TRUE(target.canBeHitByProjectile());
    EXPECT_TRUE(projectile.canHitEntity(target));
}

} // namespace
} // namespace mc
