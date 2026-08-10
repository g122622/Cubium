#include "common/TestWorldHelper.hpp"
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
    {}

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        MC_ASSERT_RELEASE(entity != nullptr);
        const EntityInstanceId id = m_nextId++;
        entity->setId(id);
        entity->setWorld(this);
        m_entities.emplace(id, std::move(entity));
        return id;
    }

    Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entities.find(id);
        return it == m_entities.end() ? nullptr : it->second.get();
    }

    const Entity* getEntity(EntityInstanceId id) const override
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
    std::unordered_map<EntityInstanceId, std::unique_ptr<Entity>> m_entities;
    EntityInstanceId m_nextId = 1;
    math::Random m_random{12345};
    world::tick::TickManager m_tickManager;
    world::border::WorldBorder m_worldBorder;
};

// 注册测试实体类型，返回 void 以便在测试中统一调用
void registerTestEntityType()
{
    entity::EntityRegistry::instance().clear();
    auto registerResult = entity::EntityRegistry::instance().registerType("test:entity",
        entity::EntityType::Builder(
            [](IWorld* world, ecs::EntityRegistry& registry) -> std::unique_ptr<Entity> { return std::make_unique<Entity>(0, world, registry); },
            entity::EntityClassification::Misc)
            .build());
    ASSERT_TRUE(registerResult.success());
}

// 构建含单个乘客的 NBT
nbt::tags::compound_tag buildVehicleNbtWithOnePassenger()
{
    nbt::tags::compound_tag root;
    root.put("id", std::string("test:entity"));

    nbt::tags::compound_tag passenger;
    passenger.put("id", std::string("test:entity"));

    auto passengers = std::make_unique<nbt::tags::compound_list_tag>();
    passengers->value.push_back(passenger);
    root.value.emplace("Passengers", std::move(passengers));
    return root;
}

// 构建含两层乘客的 NBT（vehicle → passenger1 → passenger2）
nbt::tags::compound_tag buildVehicleNbtWithNestedPassengers()
{
    nbt::tags::compound_tag root;
    root.put("id", std::string("test:entity"));

    // 最内层乘客（passenger2，骑乘 passenger1）
    nbt::tags::compound_tag passenger2;
    passenger2.put("id", std::string("test:entity"));

    auto passenger2List = std::make_unique<nbt::tags::compound_list_tag>();
    passenger2List->value.push_back(passenger2);

    // 中间乘客（passenger1，骑乘 vehicle，自身有乘客 passenger2）
    nbt::tags::compound_tag passenger1;
    passenger1.put("id", std::string("test:entity"));
    passenger1.value.emplace("Passengers", std::move(passenger2List));

    auto passengersList = std::make_unique<nbt::tags::compound_list_tag>();
    passengersList->value.push_back(passenger1);
    root.value.emplace("Passengers", std::move(passengersList));
    return root;
}

// ============================================================================
// 测试 1：反序列化阶段不 spawn 乘客，Passengers NBT 暂存到主实体
//
// 验证新 API 的核心约束：EntityDeserializer::deserialize 不再接收 IWorld 参数，
// 遇到 Passengers 标签时不会 spawn 乘客，而是把 NBT 暂存到实体的 m_pendingPassengersNbt。
// 调用方应在 spawn 主实体后调用 attachPassengers 处理。
// ============================================================================
TEST(EntitySerializationTest, DeserializeDefersPassengerSpawn)
{
    registerTestEntityType();

    auto root = buildVehicleNbtWithOnePassenger();

    // 反序列化：不 spawn 乘客，仅暂存 Passengers NBT
    auto deserializeResult = entity::serialization::EntityDeserializer::deserialize(root, mc::test::testEcsRegistry());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();
    auto vehicle = std::move(deserializeResult.value());
    ASSERT_NE(vehicle, nullptr);

    // 主实体尚未 spawn，id 仍为 0，但已有待挂载的乘客 NBT
    EXPECT_TRUE(vehicle->hasPendingPassengersNbt());
    // 主实体尚未 spawn，不应有实际乘客
    EXPECT_FALSE(vehicle->hasPassengers());

    entity::EntityRegistry::instance().clear();
}

// ============================================================================
// 测试 2：spawn 主实体后 attachPassengers 正确挂载乘客，m_vehicle 指向真实 id
//
// 验证修复后的核心流程：
// 1. deserialize 主实体（Passengers 暂存）
// 2. spawnEntity 主实体（分配真实 id）
// 3. attachPassengers 递归 spawn 乘客并 startRiding
// 4. 乘客的 m_vehicle 等于主实体的真实 id（而非 0）
// ============================================================================
TEST(EntitySerializationTest, AttachPassengersAfterSpawnBindsRealVehicleId)
{
    registerTestEntityType();

    auto root = buildVehicleNbtWithOnePassenger();

    auto deserializeResult = entity::serialization::EntityDeserializer::deserialize(root, mc::test::testEcsRegistry());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();
    auto vehicle = std::move(deserializeResult.value());
    ASSERT_NE(vehicle, nullptr);
    ASSERT_TRUE(vehicle->hasPendingPassengersNbt());

    TestSerializationWorld world;
    EntityInstanceId vehicleId = world.spawnEntity(std::move(vehicle));
    ASSERT_NE(vehicleId, 0);

    Entity* spawnedVehicle = world.getEntity(vehicleId);
    ASSERT_NE(spawnedVehicle, nullptr);

    // spawn 前不应有乘客（Passengers 仍暂存）
    EXPECT_FALSE(spawnedVehicle->hasPassengers());
    EXPECT_TRUE(spawnedVehicle->hasPendingPassengersNbt());

    // 挂载乘客
    auto attachResult = entity::serialization::EntityDeserializer::attachPassengers(*spawnedVehicle, world);
    ASSERT_TRUE(attachResult.success()) << attachResult.error().message();

    // 验证骑乘关系
    EXPECT_TRUE(spawnedVehicle->hasPassengers());
    ASSERT_EQ(spawnedVehicle->getPassengers().size(), 1u);

    Entity* spawnedPassenger = world.getEntity(spawnedVehicle->getPassengers()[0]);
    ASSERT_NE(spawnedPassenger, nullptr);
    // 核心断言：乘客的 m_vehicle 必须是主实体的真实 id，而非 0
    EXPECT_EQ(spawnedPassenger->getVehicle(), vehicleId);
    // 暂存 NBT 应已清空
    EXPECT_FALSE(spawnedVehicle->hasPendingPassengersNbt());

    entity::EntityRegistry::instance().clear();
}

// ============================================================================
// 测试 3：多层骑乘（vehicle → passenger1 → passenger2）的递归 attachPassengers
//
// 验证 Boat → Zombie → BabyZombie 这类多层骑乘场景：
// - vehicle 被 spawn 后 attachPassengers 递归处理
// - passenger1 被 spawn 并 startRiding(vehicle)
// - passenger2 被 spawn 并 startRiding(passenger1)
// - 每一层的 m_vehicle 都指向上一层的真实 id
// ============================================================================
TEST(EntitySerializationTest, AttachPassengersHandlesNestedPassengers)
{
    registerTestEntityType();

    auto root = buildVehicleNbtWithNestedPassengers();

    auto deserializeResult = entity::serialization::EntityDeserializer::deserialize(root, mc::test::testEcsRegistry());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();
    auto vehicle = std::move(deserializeResult.value());
    ASSERT_NE(vehicle, nullptr);
    ASSERT_TRUE(vehicle->hasPendingPassengersNbt());

    TestSerializationWorld world;
    EntityInstanceId vehicleId = world.spawnEntity(std::move(vehicle));
    ASSERT_NE(vehicleId, 0);

    Entity* spawnedVehicle = world.getEntity(vehicleId);
    ASSERT_NE(spawnedVehicle, nullptr);

    auto attachResult = entity::serialization::EntityDeserializer::attachPassengers(*spawnedVehicle, world);
    ASSERT_TRUE(attachResult.success()) << attachResult.error().message();

    // 验证第一层：vehicle 的乘客是 passenger1
    EXPECT_TRUE(spawnedVehicle->hasPassengers());
    ASSERT_EQ(spawnedVehicle->getPassengers().size(), 1u);
    EntityInstanceId passenger1Id = spawnedVehicle->getPassengers()[0];
    EXPECT_NE(passenger1Id, 0);

    Entity* passenger1 = world.getEntity(passenger1Id);
    ASSERT_NE(passenger1, nullptr);
    EXPECT_EQ(passenger1->getVehicle(), vehicleId);

    // 验证第二层：passenger1 的乘客是 passenger2
    EXPECT_TRUE(passenger1->hasPassengers());
    ASSERT_EQ(passenger1->getPassengers().size(), 1u);
    EntityInstanceId passenger2Id = passenger1->getPassengers()[0];
    EXPECT_NE(passenger2Id, 0);

    Entity* passenger2 = world.getEntity(passenger2Id);
    ASSERT_NE(passenger2, nullptr);
    EXPECT_EQ(passenger2->getVehicle(), passenger1Id);

    // 最内层乘客不应有乘客
    EXPECT_FALSE(passenger2->hasPassengers());

    entity::EntityRegistry::instance().clear();
}

// ============================================================================
// 测试 4：无 Passengers 标签的实体 attachPassengers 为空操作
//
// 验证 attachPassengers 在没有待挂载乘客时不会产生副作用。
// ============================================================================
TEST(EntitySerializationTest, AttachPassengersNoOpWhenNoPendingPassengers)
{
    registerTestEntityType();

    nbt::tags::compound_tag root;
    root.put("id", std::string("test:entity"));

    auto deserializeResult = entity::serialization::EntityDeserializer::deserialize(root, mc::test::testEcsRegistry());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();
    auto vehicle = std::move(deserializeResult.value());
    ASSERT_NE(vehicle, nullptr);
    EXPECT_FALSE(vehicle->hasPendingPassengersNbt());

    TestSerializationWorld world;
    EntityInstanceId vehicleId = world.spawnEntity(std::move(vehicle));
    ASSERT_NE(vehicleId, 0);

    Entity* spawnedVehicle = world.getEntity(vehicleId);
    ASSERT_NE(spawnedVehicle, nullptr);

    auto attachResult = entity::serialization::EntityDeserializer::attachPassengers(*spawnedVehicle, world);
    ASSERT_TRUE(attachResult.success()) << attachResult.error().message();

    EXPECT_FALSE(spawnedVehicle->hasPassengers());

    entity::EntityRegistry::instance().clear();
}

// ============================================================================
// 测试 5：序列化-反序列化往返（round-trip）保留骑乘关系
//
// 验证完整的持久化流程：
// 1. 创建主实体和乘客，建立骑乘关系
// 2. serializeToBinary 序列化主实体（含 Passengers 嵌套 NBT）
// 3. deserializeFromBinary 反序列化（Passengers 暂存）
// 4. spawnEntity + attachPassengers 恢复骑乘关系
// 5. 验证 m_vehicle 指向真实 id
// ============================================================================
TEST(EntitySerializationTest, SerializeDeserializeRoundTripPreservesPassengers)
{
    registerTestEntityType();

    // 创建主实体并设置类型 id
    TestSerializationWorld setupWorld;
    auto vehicle = std::make_unique<Entity>(0, &setupWorld, mc::test::testEcsRegistry());
    vehicle->setTypeId("test:entity");
    vehicle->setPosition(Vector3(1.0, 2.0, 3.0));
    EntityInstanceId vehicleId = setupWorld.spawnEntity(std::move(vehicle));
    ASSERT_NE(vehicleId, 0);

    // 创建乘客并建立骑乘关系
    auto passenger = std::make_unique<Entity>(0, &setupWorld, mc::test::testEcsRegistry());
    passenger->setTypeId("test:entity");
    passenger->setPosition(Vector3(1.0, 2.0, 3.0));
    EntityInstanceId passengerId = setupWorld.spawnEntity(std::move(passenger));
    ASSERT_NE(passengerId, 0);

    Entity* setupVehicle = setupWorld.getEntity(vehicleId);
    Entity* setupPassenger = setupWorld.getEntity(passengerId);
    ASSERT_NE(setupVehicle, nullptr);
    ASSERT_NE(setupPassenger, nullptr);
    ASSERT_TRUE(setupPassenger->startRiding(*setupVehicle));
    ASSERT_TRUE(setupVehicle->hasPassengers());
    ASSERT_EQ(setupPassenger->getVehicle(), vehicleId);

    // 序列化主实体（含 Passengers）
    auto serializeResult = entity::serialization::EntityDeserializer::serializeToBinary(*setupVehicle);
    ASSERT_TRUE(serializeResult.success()) << serializeResult.error().message();
    auto& binaryData = serializeResult.value();
    EXPECT_FALSE(binaryData.empty());

    // 反序列化（新世界，模拟存档加载）
    TestSerializationWorld loadWorld;
    auto deserializeResult = entity::serialization::EntityDeserializer::deserializeFromBinary(binaryData, mc::test::testEcsRegistry());
    ASSERT_TRUE(deserializeResult.success()) << deserializeResult.error().message();
    auto loadedVehicle = std::move(deserializeResult.value());
    ASSERT_NE(loadedVehicle, nullptr);
    EXPECT_TRUE(loadedVehicle->hasPendingPassengersNbt());

    // spawn 主实体并挂载乘客
    EntityInstanceId loadedVehicleId = loadWorld.spawnEntity(std::move(loadedVehicle));
    ASSERT_NE(loadedVehicleId, 0);

    Entity* spawnedVehicle = loadWorld.getEntity(loadedVehicleId);
    ASSERT_NE(spawnedVehicle, nullptr);

    auto attachResult = entity::serialization::EntityDeserializer::attachPassengers(*spawnedVehicle, loadWorld);
    ASSERT_TRUE(attachResult.success()) << attachResult.error().message();

    // 验证骑乘关系恢复
    EXPECT_TRUE(spawnedVehicle->hasPassengers());
    ASSERT_EQ(spawnedVehicle->getPassengers().size(), 1u);

    Entity* loadedPassenger = loadWorld.getEntity(spawnedVehicle->getPassengers()[0]);
    ASSERT_NE(loadedPassenger, nullptr);
    EXPECT_EQ(loadedPassenger->getVehicle(), loadedVehicleId);

    entity::EntityRegistry::instance().clear();
}

} // namespace
} // namespace mc
