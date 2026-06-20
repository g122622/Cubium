#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

class TestSerializationWorld : public IWorld {
public:
    TestSerializationWorld()
        : IWorld()
        , m_tickManager(*this)
    {
    }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        MC_ASSERT_RELEASE(entity != nullptr);
        const EntityId id = m_nextId++;
        entity->setId(id);
        entity->setWorld(this);
        m_entities.emplace(id, std::move(entity));
        return id;
    }

    Entity* getEntity(EntityId id) override
    {
        auto it = m_entities.find(id);
        return it == m_entities.end() ? nullptr : it->second.get();
    }

    const Entity* getEntity(EntityId id) const override
    {
        auto it = m_entities.find(id);
        return it == m_entities.end() ? nullptr : it->second.get();
    }

    // IWorld 纯虚方法实现
    const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
    i32 getHeight(i32, i32) const override { return 0; }
    u8 getBlockLight(i32, i32, i32) const override { return 0; }
    u8 getSkyLight(i32, i32, i32) const override { return 15; }
    bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    std::vector<Entity*> getPlayers() const override { return {}; }
    PhysicsEngine* physicsEngine() override { return nullptr; }
    const PhysicsEngine* physicsEngine() const override { return nullptr; }
    world::tick::TickManager& tickManager() override { return m_tickManager; }
    const world::tick::TickManager& tickManager() const override { return m_tickManager; }
    DimensionId dimension() const override { return 0; }
    u64 seed() const override { return 0; }
    u64 currentTick() const override { return 0; }
    i64 dayTime() const override { return 0; }
    bool isClientSide() const override { return false; }
    bool isHardcore() const override { return false; }
    Difficulty difficulty() const override { return Difficulty::Normal; }
    math::Random& getRandom() override { return m_random; }
    const math::Random& getRandom() const override { return m_random; }
    world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    std::unordered_map<EntityId, std::unique_ptr<Entity>> m_entities;
    EntityId m_nextId = 1;
    math::Random m_random{12345};
    world::tick::TickManager m_tickManager;
    world::border::WorldBorder m_worldBorder;
};

TEST(EntitySerializationTest, DeserializePassengersRequiresWorldContext)
{
    entity::EntityRegistry::instance().clear();
    auto registerResult = entity::EntityRegistry::instance().registerType("test:entity",
        entity::EntityType::Builder(
            [](IWorld* world) -> std::unique_ptr<Entity> { return std::make_unique<Entity>(0, world); },
            entity::EntityClassification::Misc)
            .build());
    ASSERT_TRUE(registerResult.success());

    nbt::tags::compound_tag root;
    root.put("id", std::string("test:entity"));

    nbt::tags::compound_tag passenger;
    passenger.put("id", std::string("test:entity"));

    auto passengers = std::make_unique<nbt::tags::compound_list_tag>();
    passengers->value.push_back(passenger);
    root.value.emplace("Passengers", std::move(passengers));

    auto deserializeResult = entity::serialization::EntityDeserializer::deserialize(root, nullptr);
    EXPECT_TRUE(deserializeResult.failed());

    entity::EntityRegistry::instance().clear();
}

TEST(EntitySerializationTest, DeserializePassengersSpawnsAndAttachesPassenger)
{
    entity::EntityRegistry::instance().clear();
    auto registerResult = entity::EntityRegistry::instance().registerType("test:entity",
        entity::EntityType::Builder(
            [](IWorld* world) -> std::unique_ptr<Entity> { return std::make_unique<Entity>(0, world); },
            entity::EntityClassification::Misc)
            .build());
    ASSERT_TRUE(registerResult.success());

    nbt::tags::compound_tag root;
    root.put("id", std::string("test:entity"));

    nbt::tags::compound_tag passenger;
    passenger.put("id", std::string("test:entity"));

    auto passengers = std::make_unique<nbt::tags::compound_list_tag>();
    passengers->value.push_back(passenger);
    root.value.emplace("Passengers", std::move(passengers));

    TestSerializationWorld world;
    auto deserializeResult = entity::serialization::EntityDeserializer::deserialize(root, &world);
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();
    auto vehicle = std::move(deserializeResult.value());
    ASSERT_NE(vehicle, nullptr);

    EXPECT_TRUE(vehicle->hasPassengers());
    ASSERT_EQ(vehicle->getPassengers().size(), 1u);
    Entity* spawnedPassenger = world.getEntity(vehicle->getPassengers()[0]);
    ASSERT_NE(spawnedPassenger, nullptr);
    EXPECT_EQ(spawnedPassenger->getVehicle(), vehicle->id());

    entity::EntityRegistry::instance().clear();
}

} // namespace
} // namespace mc
