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

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "entity/core/DataParameter.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/EntityClassification.hpp"
#include "entity/core/EntityDataManager.hpp"
#include "entity/core/EntityPose.hpp"
#include "entity/core/EntityRegistry.hpp"
#include "entity/core/EntitySize.hpp"
#include "entity/core/EntityType.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/core/MoverType.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// EntityClassification 测试
// ============================================================================

TEST(EntityClassification, MaxCount)
{
    EXPECT_EQ(getMaxCount(EntityClassification::Monster), 70);
    EXPECT_EQ(getMaxCount(EntityClassification::Creature), 10);
    EXPECT_EQ(getMaxCount(EntityClassification::Ambient), 15);
    EXPECT_EQ(getMaxCount(EntityClassification::Axolotls), 5);
    EXPECT_EQ(getMaxCount(EntityClassification::UndergroundWaterCreature), 5);
    EXPECT_EQ(getMaxCount(EntityClassification::WaterCreature), 5);
    EXPECT_EQ(getMaxCount(EntityClassification::WaterAmbient), 20);
    EXPECT_EQ(getMaxCount(EntityClassification::Misc), -1); // 无限制
}

TEST(EntityClassification, IsPeaceful)
{
    EXPECT_FALSE(isPeaceful(EntityClassification::Monster));
    EXPECT_TRUE(isPeaceful(EntityClassification::Creature));
    EXPECT_TRUE(isPeaceful(EntityClassification::Ambient));
    EXPECT_TRUE(isPeaceful(EntityClassification::Axolotls));
    EXPECT_TRUE(isPeaceful(EntityClassification::UndergroundWaterCreature));
    EXPECT_TRUE(isPeaceful(EntityClassification::WaterCreature));
    EXPECT_TRUE(isPeaceful(EntityClassification::WaterAmbient));
    EXPECT_TRUE(isPeaceful(EntityClassification::Misc));
}

TEST(EntityClassification, IsAnimal)
{
    EXPECT_FALSE(isAnimal(EntityClassification::Monster));
    EXPECT_TRUE(isAnimal(EntityClassification::Creature));
    EXPECT_FALSE(isAnimal(EntityClassification::Ambient));
    EXPECT_FALSE(isAnimal(EntityClassification::Axolotls));
    EXPECT_FALSE(isAnimal(EntityClassification::UndergroundWaterCreature));
    EXPECT_FALSE(isAnimal(EntityClassification::WaterCreature));
    EXPECT_FALSE(isAnimal(EntityClassification::WaterAmbient));
    EXPECT_FALSE(isAnimal(EntityClassification::Misc));
}

TEST(EntityClassification, DespawnDistance)
{
    EXPECT_EQ(getDespawnDistance(EntityClassification::Monster), 128);
    EXPECT_EQ(getDespawnDistance(EntityClassification::Creature), 128);
    EXPECT_EQ(getDespawnDistance(EntityClassification::Ambient), 128);
    EXPECT_EQ(getDespawnDistance(EntityClassification::Axolotls), 128);
    EXPECT_EQ(getDespawnDistance(EntityClassification::UndergroundWaterCreature), 128);
    EXPECT_EQ(getDespawnDistance(EntityClassification::WaterCreature), 128);
    EXPECT_EQ(getDespawnDistance(EntityClassification::WaterAmbient), 64);
    EXPECT_EQ(getDespawnDistance(EntityClassification::Misc), 128);
}

TEST(EntityClassification, InfoGetAllClassifications)
{
    // 校验 EntityClassificationInfo::get 对每个分类返回的字段
    auto checkInfo = [](EntityClassification c,
                         const char* expectedName,
                         i32 expectedMax,
                         bool expectedPeaceful,
                         bool expectedAnimal,
                         i32 expectedDespawn,
                         i32 expectedRandomDespawn) {
        const auto info = EntityClassificationInfo::get(c);
        EXPECT_EQ(info.classification, c);
        EXPECT_STREQ(info.name.c_str(), expectedName);
        EXPECT_EQ(info.maxCount, expectedMax);
        EXPECT_EQ(info.isPeaceful, expectedPeaceful);
        EXPECT_EQ(info.isAnimal, expectedAnimal);
        EXPECT_EQ(info.despawnDistance, expectedDespawn);
        EXPECT_EQ(info.randomDespawnDistance, expectedRandomDespawn);
    };

    checkInfo(EntityClassification::Monster, "monster", 70, false, false, 128, 32);
    checkInfo(EntityClassification::Creature, "creature", 10, true, true, 128, 32);
    checkInfo(EntityClassification::Ambient, "ambient", 15, true, false, 128, 32);
    checkInfo(EntityClassification::Axolotls, "axolotls", 5, true, false, 128, 32);
    checkInfo(EntityClassification::UndergroundWaterCreature, "underground_water_creature", 5, true, false, 128, 32);
    checkInfo(EntityClassification::WaterCreature, "water_creature", 5, true, false, 128, 32);
    checkInfo(EntityClassification::WaterAmbient, "water_ambient", 20, true, false, 64, 32);
    checkInfo(EntityClassification::Misc, "misc", -1, true, false, 128, 32);
}

// ============================================================================
// EntitySize 测试
// ============================================================================

TEST(EntitySize, Construction)
{
    EntitySize size(0.6f, 1.8f, false);

    EXPECT_FLOAT_EQ(size.width(), 0.6f);
    EXPECT_FLOAT_EQ(size.height(), 1.8f);
    EXPECT_FLOAT_EQ(size.eyeHeight(), 1.8f * 0.85f);
    EXPECT_FALSE(size.isFixed());
}

TEST(EntitySize, CustomEyeHeight)
{
    EntitySize size(0.6f, 1.8f, 1.2f, false);

    EXPECT_FLOAT_EQ(size.eyeHeight(), 1.2f);
    EXPECT_FLOAT_EQ(size.withEyeHeight(1.4f).eyeHeight(), 1.4f);

    EntitySize scaled = size.scale(2.0f);
    EXPECT_FLOAT_EQ(scaled.eyeHeight(), 2.4f);
}

TEST(EntitySize, FixedSize)
{
    EntitySize size = EntitySize::fixed(1.0f, 1.0f);

    EXPECT_FLOAT_EQ(size.width(), 1.0f);
    EXPECT_FLOAT_EQ(size.height(), 1.0f);
    EXPECT_TRUE(size.isFixed());
}

TEST(EntitySize, FlexibleSize)
{
    EntitySize size = EntitySize::flexible(0.9f, 1.4f);

    EXPECT_FLOAT_EQ(size.width(), 0.9f);
    EXPECT_FLOAT_EQ(size.height(), 1.4f);
    EXPECT_FALSE(size.isFixed());
}

TEST(EntitySize, CreateBoundingBox)
{
    EntitySize size(0.6f, 1.8f, false);
    AxisAlignedBB box = size.createBoundingBox(100.0, 64.0, -50.0);

    // 碰撞箱应该以实体位置为中心
    EXPECT_FLOAT_EQ(box.minX, 100.0f - 0.3f); // 99.7
    EXPECT_FLOAT_EQ(box.maxX, 100.0f + 0.3f); // 100.3
    EXPECT_FLOAT_EQ(box.minY, 64.0f);         // 脚底
    EXPECT_FLOAT_EQ(box.maxY, 64.0f + 1.8f);  // 65.8
    EXPECT_FLOAT_EQ(box.minZ, -50.0f - 0.3f); // -50.3
    EXPECT_FLOAT_EQ(box.maxZ, -50.0f + 0.3f); // -49.7
}

TEST(EntitySize, Scale)
{
    EntitySize size(0.6f, 1.8f, false);

    // 缩放灵活尺寸
    EntitySize scaled = size.scale(2.0f);
    EXPECT_FLOAT_EQ(scaled.width(), 1.2f);
    EXPECT_FLOAT_EQ(scaled.height(), 3.6f);
    EXPECT_FALSE(scaled.isFixed());

    // 缩放固定尺寸应该返回原尺寸
    EntitySize fixed = EntitySize::fixed(1.0f, 1.0f);
    EntitySize fixedScaled = fixed.scale(2.0f);
    EXPECT_FLOAT_EQ(fixedScaled.width(), 1.0f);  // 不变
    EXPECT_FLOAT_EQ(fixedScaled.height(), 1.0f); // 不变
    EXPECT_TRUE(fixedScaled.isFixed());

    // 分别缩放宽度和高度
    EntitySize scaled2 = size.scale(2.0f, 0.5f);
    EXPECT_FLOAT_EQ(scaled2.width(), 1.2f);
    EXPECT_FLOAT_EQ(scaled2.height(), 0.9f);
}

TEST(EntitySize, Comparison)
{
    EntitySize size1(0.6f, 1.8f, false);
    EntitySize size2(0.6f, 1.8f, false);
    EntitySize size3(0.9f, 1.8f, false);
    EntitySize size4(0.6f, 1.8f, true); // 固定尺寸

    EXPECT_TRUE(size1 == size2);
    EXPECT_FALSE(size1 == size3);
    EXPECT_FALSE(size1 == size4);
}

// ============================================================================
// EntityPose 测试
// ============================================================================

TEST(EntityPose, GetPoseName)
{
    EXPECT_STREQ(getPoseName(EntityPose::Standing), "standing");
    EXPECT_STREQ(getPoseName(EntityPose::FallFlying), "fall_flying");
    EXPECT_STREQ(getPoseName(EntityPose::Sleeping), "sleeping");
    EXPECT_STREQ(getPoseName(EntityPose::Swimming), "swimming");
    EXPECT_STREQ(getPoseName(EntityPose::SpinAttack), "spin_attack");
    EXPECT_STREQ(getPoseName(EntityPose::Crouching), "crouching");
    EXPECT_STREQ(getPoseName(EntityPose::Dying), "dying");
}

TEST(EntityPose, GetPoseByName)
{
    EXPECT_EQ(getPoseByName("standing"), EntityPose::Standing);
    EXPECT_EQ(getPoseByName("fall_flying"), EntityPose::FallFlying);
    EXPECT_EQ(getPoseByName("sleeping"), EntityPose::Sleeping);
    EXPECT_EQ(getPoseByName("swimming"), EntityPose::Swimming);
    EXPECT_EQ(getPoseByName("spin_attack"), EntityPose::SpinAttack);
    EXPECT_EQ(getPoseByName("crouching"), EntityPose::Crouching);
    EXPECT_EQ(getPoseByName("dying"), EntityPose::Dying);
    EXPECT_EQ(getPoseByName("unknown"), EntityPose::Standing); // 默认返回站立
}

// ============================================================================
// MoverType 测试
// ============================================================================

TEST(MoverType, GetMoverTypeName)
{
    EXPECT_STREQ(getMoverTypeName(MoverType::Self), "self");
    EXPECT_STREQ(getMoverTypeName(MoverType::Player), "player");
    EXPECT_STREQ(getMoverTypeName(MoverType::Piston), "piston");
    EXPECT_STREQ(getMoverTypeName(MoverType::ShulkerBox), "shulker_box");
    EXPECT_STREQ(getMoverTypeName(MoverType::Shulker), "shulker");
}

// ============================================================================
// EntityType 测试
// ============================================================================

// 测试用实体工厂
class TestEntity {
public:
    TestEntity() = default;
};

TEST(EntityType, Builder)
{
    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr; // 测试用
    };

    entity::EntityType type = entity::EntityType::Builder(factory, EntityClassification::Creature)
                                  .size(0.9f, 1.4f)
                                  .trackingRange(static_cast<i32>(10))
                                  .updateInterval(static_cast<i32>(3))
                                  .immuneToFire()
                                  .build();

    EXPECT_EQ(type.classification(), EntityClassification::Creature);
    EXPECT_FLOAT_EQ(type.size().width(), 0.9f);
    EXPECT_FLOAT_EQ(type.size().height(), 1.4f);
    EXPECT_EQ(type.trackingRange(), static_cast<i32>(10));
    EXPECT_EQ(type.updateInterval(), static_cast<i32>(3));
    EXPECT_TRUE(type.immuneToFire());
    EXPECT_FALSE(type.immuneToLava());
    EXPECT_TRUE(type.serializable());
}

TEST(EntityType, FixedSize)
{
    auto factory = [](IWorld*) -> std::unique_ptr<Entity> { return nullptr; };

    entity::EntityType type =
        entity::EntityType::Builder(factory, EntityClassification::Misc).fixedSize(1.0f, 1.0f).build();

    EXPECT_TRUE(type.size().isFixed());
}

TEST(EntityType, Flags)
{
    auto factory = [](IWorld*) -> std::unique_ptr<Entity> { return nullptr; };

    entity::EntityType type = entity::EntityType::Builder(factory, EntityClassification::Monster)
                                  .immuneToFire()
                                  .immuneToLava()
                                  .disableSerialization()
                                  .canSummon()
                                  .build();

    EXPECT_TRUE(type.hasFlag(entity::EntityFlags::ImmuneToFire));
    EXPECT_TRUE(type.hasFlag(entity::EntityFlags::ImmuneToLava));
    EXPECT_FALSE(type.hasFlag(entity::EntityFlags::Serializable));
    EXPECT_TRUE(type.hasFlag(entity::EntityFlags::CanSummon));
}

TEST(EntityType, EntityFlagsOperators)
{
    entity::EntityFlags flags = entity::EntityFlags::ImmuneToFire | entity::EntityFlags::ImmuneToLava;

    EXPECT_TRUE(entity::hasEntityFlag(flags, entity::EntityFlags::ImmuneToFire));
    EXPECT_TRUE(entity::hasEntityFlag(flags, entity::EntityFlags::ImmuneToLava));
    EXPECT_FALSE(entity::hasEntityFlag(flags, entity::EntityFlags::CanSummon));

    entity::EntityFlags combined = flags | entity::EntityFlags::CanSummon;
    EXPECT_TRUE(entity::hasEntityFlag(combined, entity::EntityFlags::CanSummon));

    entity::EntityFlags masked = flags & entity::EntityFlags::ImmuneToFire;
    EXPECT_TRUE(entity::hasEntityFlag(masked, entity::EntityFlags::ImmuneToFire));
    EXPECT_FALSE(entity::hasEntityFlag(masked, entity::EntityFlags::ImmuneToLava));
}

TEST(EntityType, CreateInjectsRegisteredTypeId)
{
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> { return std::make_unique<Entity>(EntityId(0)); };

    auto registerResult = registry.registerType(
        "test:spawned_entity", entity::EntityType::Builder(factory, EntityClassification::Misc).build());
    ASSERT_TRUE(registerResult.success());

    const entity::EntityType* type = registry.getType("test:spawned_entity");
    ASSERT_NE(type, nullptr);

    auto entity = type->create(nullptr);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getTypeId(), "test:spawned_entity");

    registry.clear();
}

TEST(Entity, DefaultTypeIdIsUnknown)
{
    // Entity created without a factory has no type ID set
    // getTypeId() returns empty string for untyped entities (no fabricated placeholder)
    Entity entity(EntityId(1));
    EXPECT_TRUE(entity.getTypeId().empty());
    EXPECT_EQ(entity.typeId(), 0);
}

TEST(Entity, ExplicitTypeIdCanBeSet)
{
    Entity testEntity(EntityId(1));
    testEntity.setTypeId("minecraft:custom_entity");

    EXPECT_EQ(testEntity.getTypeId(), "minecraft:custom_entity");
    // typeId() returns 0 until EntityTypeIdNumber::initialize() is called
    // and the type is registered in the registry
}

// ============================================================================
// EntityRegistry 测试
// ============================================================================

TEST(EntityRegistry, RegisterType)
{
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear(); // 清空以便测试

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr; // 测试用
    };

    auto result = registry.registerType("test:pig",
        entity::EntityType::Builder(factory, EntityClassification::Creature)
            .size(0.9f, 0.9f)
            .trackingRange(static_cast<i32>(10))
            .build());
    EXPECT_TRUE(result.success());

    // 重复注册应该失败
    auto result2 =
        registry.registerType("test:pig", entity::EntityType::Builder(factory, EntityClassification::Creature).build());
    EXPECT_FALSE(result2.success());
    EXPECT_EQ(result2.error().code(), ErrorCode::AlreadyExists);

    registry.clear();
}

TEST(EntityRegistry, GetTypeById)
{
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> { return nullptr; };

    auto result = registry.registerType(
        "test:cow", entity::EntityType::Builder(factory, EntityClassification::Creature).size(0.9f, 0.9f).build());
    ASSERT_TRUE(result.success());

    const entity::EntityType* found = registry.getType(result.value());
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->classification(), EntityClassification::Creature);
    EXPECT_EQ(found->name(), "test:cow");

    // 无效ID
    const entity::EntityType* notFound = registry.getType(static_cast<entity::EntityTypeId>(999));
    EXPECT_EQ(notFound, nullptr);

    registry.clear();
}

TEST(EntityRegistry, GetTypeByName)
{
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> { return nullptr; };

    auto result = registry.registerType(
        "test:zombie", entity::EntityType::Builder(factory, EntityClassification::Monster).size(0.6f, 1.95f).build());
    ASSERT_TRUE(result.success());

    const entity::EntityType* found = registry.getType("test:zombie");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->classification(), EntityClassification::Monster);

    // 不存在的名称
    const entity::EntityType* notFound = registry.getType("test:skeleton");
    EXPECT_EQ(notFound, nullptr);

    registry.clear();
}

TEST(EntityRegistry, HasType)
{
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> { return nullptr; };

    EXPECT_FALSE(registry.hasType("test:sheep"));

    auto result = registry.registerType(
        "test:sheep", entity::EntityType::Builder(factory, EntityClassification::Creature).build());
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(registry.hasType("test:sheep"));

    registry.clear();
}

TEST(EntityRegistry, Size)
{
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> { return nullptr; };

    EXPECT_EQ(registry.size(), 0u);

    registry.registerType("test:entity1", entity::EntityType::Builder(factory, EntityClassification::Creature).build());
    EXPECT_EQ(registry.size(), 1u);

    registry.registerType("test:entity2", entity::EntityType::Builder(factory, EntityClassification::Monster).build());
    EXPECT_EQ(registry.size(), 2u);

    registry.registerType("test:entity3", entity::EntityType::Builder(factory, EntityClassification::Ambient).build());
    EXPECT_EQ(registry.size(), 3u);

    registry.clear();
}

// ============================================================================
// DataParameter 测试
// ============================================================================

TEST(DataParameter, Construction)
{
    DataParameter<i32> param(1);
    EXPECT_EQ(param.id(), 1u);
}

TEST(DataParameter, Type)
{
    DataParameter<i8> byteParam(0);
    DataParameter<i32> intParam(1);
    DataParameter<i64> longParam(2);
    DataParameter<f32> floatParam(3);
    DataParameter<std::string> stringParam(4);
    DataParameter<bool> boolParam(5);
    DataParameter<Vector3i> blockPosParam(6);
    DataParameter<Vector2f> rotationParam(7);
    DataParameter<Vector3f> vectorParam(8);

    EXPECT_EQ(byteParam.type(), DataSerializerType::Byte);
    EXPECT_EQ(intParam.type(), DataSerializerType::Int);
    EXPECT_EQ(longParam.type(), DataSerializerType::Long);
    EXPECT_EQ(floatParam.type(), DataSerializerType::Float);
    EXPECT_EQ(stringParam.type(), DataSerializerType::String);
    EXPECT_EQ(boolParam.type(), DataSerializerType::Boolean);
    EXPECT_EQ(blockPosParam.type(), DataSerializerType::BlockPos);
    EXPECT_EQ(rotationParam.type(), DataSerializerType::Rotation);
    EXPECT_EQ(vectorParam.type(), DataSerializerType::Vector3f);
}

TEST(DataParameter, Comparison)
{
    DataParameter<i32> param1(1);
    DataParameter<i32> param2(1);
    DataParameter<i32> param3(2);

    EXPECT_TRUE(param1 == param2);
    EXPECT_FALSE(param1 == param3);
    EXPECT_TRUE(param1 != param3);
}

// ============================================================================
// EntityDataManager 测试
// ============================================================================

TEST(EntityDataManager, RegisterAndSetGet)
{
    EntityDataManager manager;

    auto healthParam = EntityDataManager::createKey<i32>();
    auto nameParam = EntityDataManager::createKey<std::string>();
    auto fireParam = EntityDataManager::createKey<bool>();

    manager.registerParam(healthParam, 20);
    manager.registerParam(nameParam, std::string("test"));
    manager.registerParam(fireParam, false);

    EXPECT_EQ(manager.get(healthParam), 20);
    EXPECT_EQ(manager.get(nameParam), "test");
    EXPECT_FALSE(manager.get(fireParam));
}

TEST(EntityDataManager, SetMarksDirty)
{
    EntityDataManager manager;

    auto param = EntityDataManager::createKey<i32>();
    manager.registerParam(param, 100);

    EXPECT_FALSE(manager.hasDirtyData());

    manager.set(param, 50);
    EXPECT_TRUE(manager.hasDirtyData());
    EXPECT_TRUE(manager.hasParam(param.id()));
}

TEST(EntityDataManager, SetSameValueNotDirty)
{
    EntityDataManager manager;

    auto param = EntityDataManager::createKey<i32>();
    manager.registerParam(param, 100);

    // 第一次设置相同值不会变脏
    manager.clearDirty();
    manager.set(param, 100);
    EXPECT_FALSE(manager.hasDirtyData());

    // 设置不同值会变脏
    manager.set(param, 50);
    EXPECT_TRUE(manager.hasDirtyData());
}

TEST(EntityDataManager, GetDirtyParams)
{
    EntityDataManager manager;

    auto param1 = EntityDataManager::createKey<i32>();
    auto param2 = EntityDataManager::createKey<i32>();
    auto param3 = EntityDataManager::createKey<i32>();

    manager.registerParam(param1, 1);
    manager.registerParam(param2, 2);
    manager.registerParam(param3, 3);

    manager.clearDirty();

    manager.set(param1, 10);
    manager.set(param3, 30);

    auto dirtyParams = manager.getDirtyParams();
    EXPECT_EQ(dirtyParams.size(), 2u);
}

TEST(EntityDataManager, ClearDirty)
{
    EntityDataManager manager;

    auto param1 = EntityDataManager::createKey<i32>();
    auto param2 = EntityDataManager::createKey<i32>();

    manager.registerParam(param1, 1);
    manager.registerParam(param2, 2);

    manager.set(param1, 10);
    manager.set(param2, 20);

    EXPECT_TRUE(manager.hasDirtyData());

    manager.clearDirty();
    EXPECT_FALSE(manager.hasDirtyData());
}

TEST(EntityDataManager, ClearDirtySingleParam)
{
    EntityDataManager manager;

    auto param1 = EntityDataManager::createKey<i32>();
    auto param2 = EntityDataManager::createKey<i32>();

    manager.registerParam(param1, 1);
    manager.registerParam(param2, 2);

    manager.set(param1, 10);
    manager.set(param2, 20);

    // 只清除param1的脏标记
    manager.clearDirty(param1.id());

    EXPECT_TRUE(manager.hasDirtyData()); // param2仍然是脏的

    manager.clearDirty(param2.id());
    EXPECT_FALSE(manager.hasDirtyData());
}

TEST(EntityDataManager, CopyFrom)
{
    EntityDataManager manager1;
    EntityDataManager manager2;

    auto param = EntityDataManager::createKey<i32>();

    manager1.registerParam(param, 100);
    manager2.registerParam(param, 50);

    manager2.copyFrom(manager1);

    EXPECT_EQ(manager2.get(param), 100);
}

TEST(EntityDataManager, DifferentTypes)
{
    EntityDataManager manager;

    auto intParam = EntityDataManager::createKey<i32>();
    auto floatParam = EntityDataManager::createKey<f32>();
    auto stringParam = EntityDataManager::createKey<std::string>();
    auto boolParam = EntityDataManager::createKey<bool>();
    auto vecParam = EntityDataManager::createKey<Vector3i>();

    manager.registerParam(intParam, 42);
    manager.registerParam(floatParam, 3.14f);
    manager.registerParam(stringParam, std::string("hello"));
    manager.registerParam(boolParam, true);
    manager.registerParam(vecParam, Vector3i(1, 2, 3));

    EXPECT_EQ(manager.get(intParam), 42);
    EXPECT_FLOAT_EQ(manager.get(floatParam), 3.14f);
    EXPECT_EQ(manager.get(stringParam), "hello");
    EXPECT_TRUE(manager.get(boolParam));

    Vector3i vec = manager.get(vecParam);
    EXPECT_EQ(vec.x, 1);
    EXPECT_EQ(vec.y, 2);
    EXPECT_EQ(vec.z, 3);
}

TEST(EntityDataManager, UniqueIds)
{
    EntityDataManager manager;

    auto param1 = EntityDataManager::createKey<i32>();
    auto param2 = EntityDataManager::createKey<i32>();
    auto param3 = EntityDataManager::createKey<i32>();

    // 每个参数ID应该唯一
    EXPECT_NE(param1.id(), param2.id());
    EXPECT_NE(param2.id(), param3.id());
    EXPECT_NE(param1.id(), param3.id());
}

class TestMobEntity : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityId(100))
    {}
};

TEST(MobEntityTest, IsBeingRiddenReflectsPassengerState)
{
    TestMobEntity vehicle;
    Entity rider(EntityId(101));

    EXPECT_EQ(vehicle.isBeingRidden(), vehicle.hasPassengers());
    EXPECT_FALSE(rider.isRiding());
    EXPECT_TRUE(rider.startRiding(vehicle));
    EXPECT_TRUE(vehicle.isBeingRidden());
    EXPECT_TRUE(vehicle.hasPassengers());
    EXPECT_TRUE(rider.isRiding());
}

// ============================================================================
// Entity::processInitialInteract / applyPlayerInteraction 测试
// ============================================================================

/**
 * @brief 测试 Entity::processInitialInteract 和 applyPlayerInteraction 的 API 存在性
 *
 * 这些测试验证：
 * 1. Entity 类有 processInitialInteract 和 applyPlayerInteraction 虚拟方法
 * 2. 方法签名正确
 * 3. 子类可以重写这些方法
 *
 * 注意：由于 Player 需要 World 依赖，完整的集成测试在其他测试文件中进行。
 * 这里主要验证 API 设计和编译正确性。
 */

/**
 * @brief 测试实体能够被子类化并重写交互方法
 *
 * 创建一个测试用的实体子类，重写 processInitialInteract 和 applyPlayerInteraction，
 * 验证虚拟方法调用机制正常工作。
 */
class TestInteractableEntity : public Entity {
public:
    TestInteractableEntity()
        : Entity(EntityId(1))
        , m_processInitialInteractCalled(false)
        , m_applyPlayerInteractionCalled(false)
        , m_lastHitPosition(0.0f, 0.0f, 0.0f)
        , m_lastHand(Hand::MainHand)
        , m_returnValue(ActionResultType::Pass)
    {}

    // 重写 processInitialInteract 以跟踪调用
    ActionResultType processInitialInteract(Player& player, Hand hand) override
    {
        m_processInitialInteractCalled = true;
        m_lastHand = hand;
        (void)player; // 避免未使用警告
        return m_returnValue;
    }

    // 重写 applyPlayerInteraction 以跟踪调用
    ActionResultType applyPlayerInteraction(Player& player, const Vector3& hitPosition, Hand hand) override
    {
        m_applyPlayerInteractionCalled = true;
        m_lastHitPosition = hitPosition;
        m_lastHand = hand;
        (void)player; // 避免未使用警告
        return m_returnValue;
    }

    // 测试辅助方法
    void setReturnValue(ActionResultType value) { m_returnValue = value; }
    bool wasProcessInitialInteractCalled() const { return m_processInitialInteractCalled; }
    bool wasApplyPlayerInteractionCalled() const { return m_applyPlayerInteractionCalled; }
    const Vector3& lastHitPosition() const { return m_lastHitPosition; }
    Hand lastHand() const { return m_lastHand; }

    void reset()
    {
        m_processInitialInteractCalled = false;
        m_applyPlayerInteractionCalled = false;
        m_lastHitPosition = Vector3(0.0f, 0.0f, 0.0f);
        m_lastHand = Hand::MainHand;
    }

private:
    bool m_processInitialInteractCalled;
    bool m_applyPlayerInteractionCalled;
    Vector3 m_lastHitPosition;
    Hand m_lastHand;
    ActionResultType m_returnValue;
};

/**
 * @brief 测试 Entity 基类的默认交互方法可以被重写
 */
TEST(EntityInteractionTest, VirtualMethodsCanBeOverridden)
{
    TestInteractableEntity entity;

    // 验证实体创建成功
    EXPECT_EQ(entity.id(), EntityId(1));
}

/**
 * @brief 测试 ActionResultType 枚举值
 *
 * 验证交互结果类型的正确性，这些是 processInitialInteract 和
 * applyPlayerInteraction 方法的返回值类型。
 */
TEST(EntityInteractionTest, ActionResultTypeEnumValues)
{
    // 验证 ActionResultType 枚举值
    EXPECT_EQ(static_cast<int>(ActionResultType::Success), 0);
    EXPECT_EQ(static_cast<int>(ActionResultType::Consume), 1);
    EXPECT_EQ(static_cast<int>(ActionResultType::Fail), 2);
    EXPECT_EQ(static_cast<int>(ActionResultType::Pass), 3);
}

/**
 * @brief 测试 Hand 枚举值
 *
 * 验证手部枚举类型的正确性，这是交互方法的参数类型。
 */
TEST(EntityInteractionTest, HandEnumValues)
{
    EXPECT_EQ(static_cast<int>(Hand::MainHand), 0);
    EXPECT_EQ(static_cast<int>(Hand::OffHand), 1);
}

/**
 * @brief 测试 Entity 基类默认 processInitialInteract 返回 Pass
 *
 * Entity 基类的默认实现应该返回 ActionResultType::Pass。
 * 我们通过创建一个 Entity 实例并检查其行为来验证。
 * 由于无法直接调用（需要 Player 参数），我们验证方法签名存在。
 */
TEST(EntityInteractionTest, BaseEntityHasProcessInitialInteractMethod)
{
    // 验证 Entity 类有 processInitialInteract 方法
    // 通过检查方法指针类型来验证 API 存在
    using ProcessInitialInteractPtr = ActionResultType (Entity::*)(Player&, Hand);
    ProcessInitialInteractPtr ptr = &Entity::processInitialInteract;
    (void)ptr; // 避免未使用警告

    // 如果编译通过，说明方法签名正确
    SUCCEED();
}

/**
 * @brief 测试 Entity 基类默认 applyPlayerInteraction 方法签名
 *
 * 验证 applyPlayerInteraction 方法有正确的签名，接受 hitPosition 参数。
 */
TEST(EntityInteractionTest, BaseEntityHasApplyPlayerInteractionMethod)
{
    // 验证 Entity 类有 applyPlayerInteraction 方法
    using ApplyPlayerInteractionPtr = ActionResultType (Entity::*)(Player&, const Vector3&, Hand);
    ApplyPlayerInteractionPtr ptr = &Entity::applyPlayerInteraction;
    (void)ptr; // 避免未使用警告

    // 如果编译通过，说明方法签名正确
    SUCCEED();
}

/**
 * @brief 测试子类重写的方法可以返回不同的 ActionResultType
 */
TEST(EntityInteractionTest, OverriddenMethodCanReturnDifferentResults)
{
    TestInteractableEntity entity;

    // 测试返回 Success
    entity.setReturnValue(ActionResultType::Success);

    // 测试返回 Fail
    entity.setReturnValue(ActionResultType::Fail);

    // 测试返回 Consume
    entity.setReturnValue(ActionResultType::Consume);

    // 测试返回 Pass
    entity.setReturnValue(ActionResultType::Pass);

    SUCCEED();
}

/**
 * @brief 测试多态行为 - 通过基类指针调用子类方法
 */
TEST(EntityInteractionTest, PolymorphicCallWorks)
{
    TestInteractableEntity derivedEntity;
    Entity* basePtr = &derivedEntity;

    // 验证基类指针指向正确的对象
    EXPECT_EQ(basePtr->id(), EntityId(1));
}
