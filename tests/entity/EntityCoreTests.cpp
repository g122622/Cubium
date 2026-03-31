#include <gtest/gtest.h>

#include "entity/core/EntityClassification.hpp"
#include "entity/core/EntitySize.hpp"
#include "entity/core/EntityPose.hpp"
#include "entity/core/MoverType.hpp"
#include "entity/core/EntityType.hpp"
#include "entity/core/EntityRegistry.hpp"
#include "entity/core/DataParameter.hpp"
#include "entity/core/EntityDataManager.hpp"
#include "entity/core/Entity.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// EntityClassification 测试
// ============================================================================

TEST(EntityClassification, MaxCount) {
    EXPECT_EQ(getMaxCount(EntityClassification::Monster), 70);
    EXPECT_EQ(getMaxCount(EntityClassification::Creature), 10);
    EXPECT_EQ(getMaxCount(EntityClassification::Ambient), 15);
    EXPECT_EQ(getMaxCount(EntityClassification::WaterCreature), 5);
    EXPECT_EQ(getMaxCount(EntityClassification::WaterAmbient), 20);
    EXPECT_EQ(getMaxCount(EntityClassification::Misc), -1);  // 无限制
}

TEST(EntityClassification, IsPeaceful) {
    EXPECT_FALSE(isPeaceful(EntityClassification::Monster));
    EXPECT_TRUE(isPeaceful(EntityClassification::Creature));
    EXPECT_TRUE(isPeaceful(EntityClassification::Ambient));
    EXPECT_TRUE(isPeaceful(EntityClassification::WaterCreature));
    EXPECT_TRUE(isPeaceful(EntityClassification::WaterAmbient));
    EXPECT_TRUE(isPeaceful(EntityClassification::Misc));
}

TEST(EntityClassification, IsAnimal) {
    EXPECT_FALSE(isAnimal(EntityClassification::Monster));
    EXPECT_TRUE(isAnimal(EntityClassification::Creature));
    EXPECT_FALSE(isAnimal(EntityClassification::Ambient));
    EXPECT_FALSE(isAnimal(EntityClassification::WaterCreature));
    EXPECT_FALSE(isAnimal(EntityClassification::WaterAmbient));
    EXPECT_FALSE(isAnimal(EntityClassification::Misc));
}

TEST(EntityClassification, DespawnDistance) {
    EXPECT_EQ(getDespawnDistance(EntityClassification::Monster), 128);
    EXPECT_EQ(getDespawnDistance(EntityClassification::Creature), 128);
    EXPECT_EQ(getDespawnDistance(EntityClassification::WaterAmbient), 64);
}

// ============================================================================
// EntitySize 测试
// ============================================================================

TEST(EntitySize, Construction) {
    EntitySize size(0.6f, 1.8f, false);

    EXPECT_FLOAT_EQ(size.width(), 0.6f);
    EXPECT_FLOAT_EQ(size.height(), 1.8f);
    EXPECT_FALSE(size.isFixed());
}

TEST(EntitySize, FixedSize) {
    EntitySize size = EntitySize::fixed(1.0f, 1.0f);

    EXPECT_FLOAT_EQ(size.width(), 1.0f);
    EXPECT_FLOAT_EQ(size.height(), 1.0f);
    EXPECT_TRUE(size.isFixed());
}

TEST(EntitySize, FlexibleSize) {
    EntitySize size = EntitySize::flexible(0.9f, 1.4f);

    EXPECT_FLOAT_EQ(size.width(), 0.9f);
    EXPECT_FLOAT_EQ(size.height(), 1.4f);
    EXPECT_FALSE(size.isFixed());
}

TEST(EntitySize, CreateBoundingBox) {
    EntitySize size(0.6f, 1.8f, false);
    AxisAlignedBB box = size.createBoundingBox(100.0, 64.0, -50.0);

    // 碰撞箱应该以实体位置为中心
    EXPECT_FLOAT_EQ(box.minX, 100.0f - 0.3f);  // 99.7
    EXPECT_FLOAT_EQ(box.maxX, 100.0f + 0.3f);  // 100.3
    EXPECT_FLOAT_EQ(box.minY, 64.0f);           // 脚底
    EXPECT_FLOAT_EQ(box.maxY, 64.0f + 1.8f);    // 65.8
    EXPECT_FLOAT_EQ(box.minZ, -50.0f - 0.3f);   // -50.3
    EXPECT_FLOAT_EQ(box.maxZ, -50.0f + 0.3f);   // -49.7
}

TEST(EntitySize, Scale) {
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

TEST(EntitySize, Comparison) {
    EntitySize size1(0.6f, 1.8f, false);
    EntitySize size2(0.6f, 1.8f, false);
    EntitySize size3(0.9f, 1.8f, false);
    EntitySize size4(0.6f, 1.8f, true);  // 固定尺寸

    EXPECT_TRUE(size1 == size2);
    EXPECT_FALSE(size1 == size3);
    EXPECT_FALSE(size1 == size4);
}

// ============================================================================
// EntityPose 测试
// ============================================================================

TEST(EntityPose, GetPoseName) {
    EXPECT_STREQ(getPoseName(EntityPose::Standing), "standing");
    EXPECT_STREQ(getPoseName(EntityPose::FallFlying), "fall_flying");
    EXPECT_STREQ(getPoseName(EntityPose::Sleeping), "sleeping");
    EXPECT_STREQ(getPoseName(EntityPose::Swimming), "swimming");
    EXPECT_STREQ(getPoseName(EntityPose::SpinAttack), "spin_attack");
    EXPECT_STREQ(getPoseName(EntityPose::Crouching), "crouching");
    EXPECT_STREQ(getPoseName(EntityPose::Dying), "dying");
}

TEST(EntityPose, GetPoseByName) {
    EXPECT_EQ(getPoseByName("standing"), EntityPose::Standing);
    EXPECT_EQ(getPoseByName("fall_flying"), EntityPose::FallFlying);
    EXPECT_EQ(getPoseByName("sleeping"), EntityPose::Sleeping);
    EXPECT_EQ(getPoseByName("swimming"), EntityPose::Swimming);
    EXPECT_EQ(getPoseByName("spin_attack"), EntityPose::SpinAttack);
    EXPECT_EQ(getPoseByName("crouching"), EntityPose::Crouching);
    EXPECT_EQ(getPoseByName("dying"), EntityPose::Dying);
    EXPECT_EQ(getPoseByName("unknown"), EntityPose::Standing);  // 默认返回站立
}

// ============================================================================
// MoverType 测试
// ============================================================================

TEST(MoverType, GetMoverTypeName) {
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

TEST(EntityType, Builder) {
    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr;  // 测试用
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

TEST(EntityType, FixedSize) {
    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr;
    };

    entity::EntityType type = entity::EntityType::Builder(factory, EntityClassification::Misc)
        .fixedSize(1.0f, 1.0f)
        .build();

    EXPECT_TRUE(type.size().isFixed());
}

TEST(EntityType, Flags) {
    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr;
    };

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

TEST(EntityType, EntityFlagsOperators) {
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

// ============================================================================
// EntityRegistry 测试
// ============================================================================

TEST(EntityRegistry, RegisterType) {
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();  // 清空以便测试

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr;  // 测试用
    };

    auto result = registry.registerType("test:pig",
        entity::EntityType::Builder(factory, EntityClassification::Creature)
            .size(0.9f, 0.9f)
            .trackingRange(static_cast<i32>(10))
            .build());
    EXPECT_TRUE(result.success());

    // 重复注册应该失败
    auto result2 = registry.registerType("test:pig",
        entity::EntityType::Builder(factory, EntityClassification::Creature).build());
    EXPECT_FALSE(result2.success());
    EXPECT_EQ(result2.error().code(), ErrorCode::AlreadyExists);

    registry.clear();
}

TEST(EntityRegistry, GetTypeById) {
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr;
    };

    auto result = registry.registerType("test:cow",
        entity::EntityType::Builder(factory, EntityClassification::Creature)
            .size(0.9f, 0.9f)
            .build());
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

TEST(EntityRegistry, GetTypeByName) {
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr;
    };

    auto result = registry.registerType("test:zombie",
        entity::EntityType::Builder(factory, EntityClassification::Monster)
            .size(0.6f, 1.95f)
            .build());
    ASSERT_TRUE(result.success());

    const entity::EntityType* found = registry.getType("test:zombie");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->classification(), EntityClassification::Monster);

    // 不存在的名称
    const entity::EntityType* notFound = registry.getType("test:skeleton");
    EXPECT_EQ(notFound, nullptr);

    registry.clear();
}

TEST(EntityRegistry, HasType) {
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr;
    };

    EXPECT_FALSE(registry.hasType("test:sheep"));

    auto result = registry.registerType("test:sheep",
        entity::EntityType::Builder(factory, EntityClassification::Creature).build());
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(registry.hasType("test:sheep"));

    registry.clear();
}

TEST(EntityRegistry, Size) {
    EntityRegistry& registry = EntityRegistry::instance();
    registry.clear();

    auto factory = [](IWorld*) -> std::unique_ptr<Entity> {
        return nullptr;
    };

    EXPECT_EQ(registry.size(), 0u);

    registry.registerType("test:entity1",
        entity::EntityType::Builder(factory, EntityClassification::Creature).build());
    EXPECT_EQ(registry.size(), 1u);

    registry.registerType("test:entity2",
        entity::EntityType::Builder(factory, EntityClassification::Monster).build());
    EXPECT_EQ(registry.size(), 2u);

    registry.registerType("test:entity3",
        entity::EntityType::Builder(factory, EntityClassification::Ambient).build());
    EXPECT_EQ(registry.size(), 3u);

    registry.clear();
}

// ============================================================================
// DataParameter 测试
// ============================================================================

TEST(DataParameter, Construction) {
    DataParameter<i32> param(1);
    EXPECT_EQ(param.id(), 1u);
}

TEST(DataParameter, Type) {
    DataParameter<i8> byteParam(0);
    DataParameter<i32> intParam(1);
    DataParameter<i64> longParam(2);
    DataParameter<f32> floatParam(3);
    DataParameter<String> stringParam(4);
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

TEST(DataParameter, Comparison) {
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

TEST(EntityDataManager, RegisterAndSetGet) {
    EntityDataManager manager;

    auto healthParam = EntityDataManager::createKey<i32>();
    auto nameParam = EntityDataManager::createKey<String>();
    auto fireParam = EntityDataManager::createKey<bool>();

    manager.registerParam(healthParam, 20);
    manager.registerParam(nameParam, String("test"));
    manager.registerParam(fireParam, false);

    EXPECT_EQ(manager.get(healthParam), 20);
    EXPECT_EQ(manager.get(nameParam), "test");
    EXPECT_FALSE(manager.get(fireParam));
}

TEST(EntityDataManager, SetMarksDirty) {
    EntityDataManager manager;

    auto param = EntityDataManager::createKey<i32>();
    manager.registerParam(param, 100);

    EXPECT_FALSE(manager.hasDirtyData());

    manager.set(param, 50);
    EXPECT_TRUE(manager.hasDirtyData());
    EXPECT_TRUE(manager.hasParam(param.id()));
}

TEST(EntityDataManager, SetSameValueNotDirty) {
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

TEST(EntityDataManager, GetDirtyParams) {
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

TEST(EntityDataManager, ClearDirty) {
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

TEST(EntityDataManager, ClearDirtySingleParam) {
    EntityDataManager manager;

    auto param1 = EntityDataManager::createKey<i32>();
    auto param2 = EntityDataManager::createKey<i32>();

    manager.registerParam(param1, 1);
    manager.registerParam(param2, 2);

    manager.set(param1, 10);
    manager.set(param2, 20);

    // 只清除param1的脏标记
    manager.clearDirty(param1.id());

    EXPECT_TRUE(manager.hasDirtyData());  // param2仍然是脏的

    manager.clearDirty(param2.id());
    EXPECT_FALSE(manager.hasDirtyData());
}

TEST(EntityDataManager, CopyFrom) {
    EntityDataManager manager1;
    EntityDataManager manager2;

    auto param = EntityDataManager::createKey<i32>();

    manager1.registerParam(param, 100);
    manager2.registerParam(param, 50);

    manager2.copyFrom(manager1);

    EXPECT_EQ(manager2.get(param), 100);
}

TEST(EntityDataManager, DifferentTypes) {
    EntityDataManager manager;

    auto intParam = EntityDataManager::createKey<i32>();
    auto floatParam = EntityDataManager::createKey<f32>();
    auto stringParam = EntityDataManager::createKey<String>();
    auto boolParam = EntityDataManager::createKey<bool>();
    auto vecParam = EntityDataManager::createKey<Vector3i>();

    manager.registerParam(intParam, 42);
    manager.registerParam(floatParam, 3.14f);
    manager.registerParam(stringParam, String("hello"));
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

TEST(EntityDataManager, UniqueIds) {
    EntityDataManager manager;

    auto param1 = EntityDataManager::createKey<i32>();
    auto param2 = EntityDataManager::createKey<i32>();
    auto param3 = EntityDataManager::createKey<i32>();

    // 每个参数ID应该唯一
    EXPECT_NE(param1.id(), param2.id());
    EXPECT_NE(param2.id(), param3.id());
    EXPECT_NE(param1.id(), param3.id());
}
