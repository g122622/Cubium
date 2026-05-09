#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileHelper.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace {

class ProjectileHelperTestWorld : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 0; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    [[nodiscard]] Entity* getEntity(EntityId id) override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityId id) const override
    {
        for (const auto& entity : m_entities) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box,
        const Entity* except = nullptr) const override
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

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3&,
        f32,
        const Entity* = nullptr) const override
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
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("ProjectileHelperTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("ProjectileHelperTestWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override {
        throw std::runtime_error("ProjectileHelperTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        throw std::runtime_error("ProjectileHelperTestWorld::getRandom not implemented");
    }

    // WorldBorder interface (stubbed for tests)
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("ProjectileHelperTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("ProjectileHelperTestWorld::worldBorder not implemented");
    }

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
};

class TestTargetEntity : public Entity {
public:
    TestTargetEntity(EntityId id, bool collidable)
        : Entity(LegacyEntityType::Unknown, id)
        , m_collidable(collidable)
    {
    }

    [[nodiscard]] bool canBeCollidedWith() const override { return m_collidable; }

private:
    bool m_collidable;
};

class RotationProbeEntity : public Entity {
public:
    explicit RotationProbeEntity(EntityId id)
        : Entity(LegacyEntityType::Unknown, id)
    {
    }
};

class ExposedProjectileEntity : public entity::ProjectileEntity {
public:
    explicit ExposedProjectileEntity(EntityId id)
        : ProjectileEntity(LegacyEntityType::Unknown, id)
    {
    }

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
    const AxisAlignedBB searchBox =
        entity::ProjectileHelper::createMovementSearchBox(projectile, end - start, 1.0f);

    const entity::RayTraceResult result = entity::ProjectileHelper::rayTraceEntities(
        world,
        projectile,
        start,
        end,
        searchBox,
        [](const Entity& candidate) {
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
