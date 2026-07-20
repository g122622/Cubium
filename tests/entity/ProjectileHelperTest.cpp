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
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace {

class ProjectileHelperTestWorld : public test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override
    {
        std::vector<Entity*> result;
        for (const auto& entity : m_entities) {
            if (entity.get() == except || entity->isRemoved()) {
                continue;
            }
            if (box.intersects(entity->boundingBox())) {
                result.push_back(entity.get());
            }
        }
        return result;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity* = nullptr) const override
    {
        return {};
    }

    template <typename T, typename... Args>
    T& addEntity(Args&&... args)
    {
        auto entity = std::make_unique<T>(std::forward<Args>(args)...);
        entity->setWorld(this);
        T& reference = *entity;
        m_entities.push_back(std::move(entity));
        return reference;
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ProjectileHelperTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ProjectileHelperTestWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
private:
    std::vector<std::unique_ptr<Entity>> m_entities;
};

class TestTargetEntity : public Entity {
public:
    TestTargetEntity(EntityInstanceId id, bool collidable)
        : Entity(id)
        , m_collidable(collidable)
    {}

    [[nodiscard]] bool canBeCollidedWith() const override { return m_collidable; }

private:
    bool m_collidable;
};

class RotationProbeEntity : public Entity {
public:
    explicit RotationProbeEntity(EntityInstanceId id)
        : Entity(id)
    {}
};

class ExposedProjectileEntity : public entity::ProjectileEntity {
public:
    explicit ExposedProjectileEntity(EntityInstanceId id)
        : ProjectileEntity(id)
    {}

    void setLeftShooterFlag(bool value) { m_leftShooter = value; }
};

TEST(ProjectileHelperTest, RotateTowardsMovementMatchesVanillaYawPitch)
{
    RotationProbeEntity probe(1);
    probe.setVelocity(0.0f, 0.0f, 1.0f);

    entity::ProjectileHelper::rotateTowardsMovement(probe, 1.0f);

    EXPECT_NEAR(probe.yaw(), 180.0f, 1.0e-3f);
    EXPECT_NEAR(probe.pitch(), 0.0f, 1.0e-3f);
}

TEST(ProjectileHelperTest, RayTraceEntitiesReturnsNearestCollidableTarget)
{
    ProjectileHelperTestWorld world;
    RotationProbeEntity projectile(1);
    projectile.setPosition(0.0f, 64.0f, 0.0f);

    auto& first = world.addEntity<TestTargetEntity>(2, true);
    first.setPosition(2.0f, 64.0f, 0.0f);

    auto& second = world.addEntity<TestTargetEntity>(3, true);
    second.setPosition(4.0f, 64.0f, 0.0f);

    const Vector3 start = projectile.position();
    const Vector3 end = start + Vector3(5.0f, 0.0f, 0.0f);
    const AxisAlignedBB searchBox = entity::ProjectileHelper::createMovementSearchBox(projectile, end - start, 1.0f);

    const entity::RayTraceResult result = entity::ProjectileHelper::rayTraceEntities(
        world, projectile, start, end, searchBox, [](const Entity& candidate) {
            return candidate.canBeCollidedWith();
        });

    EXPECT_EQ(result.type, entity::RayTraceResultType::Entity);
    ASSERT_NE(result.hitEntity, nullptr);
    EXPECT_EQ(result.hitEntity->id(), first.id());
}

TEST(ProjectileHelperTest, ProjectileCanHitEntityHonorsCollisionAndShooterState)
{
    ProjectileHelperTestWorld world;
    ExposedProjectileEntity projectile(10);
    projectile.setWorld(&world);

    auto& shooter = world.addEntity<TestTargetEntity>(20, true);
    auto& nonCollidable = world.addEntity<TestTargetEntity>(21, false);

    projectile.setShooter(&shooter);

    EXPECT_FALSE(projectile.canHitEntity(shooter));
    EXPECT_FALSE(projectile.canHitEntity(nonCollidable));

    projectile.setLeftShooterFlag(true);

    EXPECT_TRUE(projectile.canHitEntity(shooter));
}

} // namespace
} // namespace mc
