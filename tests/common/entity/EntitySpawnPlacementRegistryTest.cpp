#include <gtest/gtest.h>
#include "entity/core/EntitySpawnPlacementRegistry.hpp"
#include "world/spawn/MobSpawnInfo.hpp"
#include "world/chunk/IChunk.hpp"
#include "world/block/Block.hpp"

namespace mc {
namespace test {

/**
 * @brief EntitySpawnPlacementRegistry 测试套件
 *
 * 测试实体生成位置规则的注册和查询。
 */
class EntitySpawnPlacementRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化注册表
        world::spawn::EntitySpawnPlacementRegistry::initializeDefaults();
    }
};

// ========== 基本功能测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, IsInitialized) {
    EXPECT_TRUE(world::spawn::EntitySpawnPlacementRegistry::isInitialized());
}

TEST_F(EntitySpawnPlacementRegistryTest, GetPlacementTypeForKnownEntity) {
    // 测试已知实体的放置类型
    auto pigType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:pig");
    EXPECT_EQ(pigType, world::spawn::PlacementType::OnGround);

    auto codType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:cod");
    EXPECT_EQ(codType, world::spawn::PlacementType::InWater);

    auto striderType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:strider");
    EXPECT_EQ(striderType, world::spawn::PlacementType::InLava);

    auto phantomType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:phantom");
    EXPECT_EQ(phantomType, world::spawn::PlacementType::NoRestrictions);
}

TEST_F(EntitySpawnPlacementRegistryTest, GetPlacementTypeForUnknownEntity) {
    // 未知实体应该返回 NoRestrictions
    auto unknownType = world::spawn::EntitySpawnPlacementRegistry::getPlacementType("minecraft:unknown_entity");
    EXPECT_EQ(unknownType, world::spawn::PlacementType::NoRestrictions);
}

TEST_F(EntitySpawnPlacementRegistryTest, GetHeightmapTypeForKnownEntity) {
    // 测试已知实体的高度图类型
    auto pigHeightmap = world::spawn::EntitySpawnPlacementRegistry::getHeightmapType("minecraft:pig");
    EXPECT_EQ(pigHeightmap, HeightmapType::MotionBlockingNoLeaves);

    auto ocelotHeightmap = world::spawn::EntitySpawnPlacementRegistry::getHeightmapType("minecraft:ocelot");
    EXPECT_EQ(ocelotHeightmap, HeightmapType::MotionBlocking);
}

TEST_F(EntitySpawnPlacementRegistryTest, GetHeightmapTypeForUnknownEntity) {
    // 未知实体应该返回默认高度图类型
    auto unknownHeightmap = world::spawn::EntitySpawnPlacementRegistry::getHeightmapType("minecraft:unknown_entity");
    EXPECT_EQ(unknownHeightmap, HeightmapType::MotionBlockingNoLeaves);
}

// ========== SpawnReason 枚举测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, SpawnReasonValues) {
    // 验证 SpawnReason 枚举值
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Natural), 0);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::ChunkGeneration), 1);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::SpawnEgg), 2);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Spawner), 3);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Structure), 4);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Breeding), 5);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Jockey), 6);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::MobSummons), 7);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Conversion), 8);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Reinforcement), 9);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Trigger), 10);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Bucket), 11);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Dispenser), 12);
    EXPECT_EQ(static_cast<int>(world::spawn::SpawnReason::Event), 13);
}

// ========== PlacementType 枚举测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, PlacementTypeValues) {
    // 验证 PlacementType 枚举值
    EXPECT_EQ(static_cast<int>(world::spawn::PlacementType::OnGround), 0);
    EXPECT_EQ(static_cast<int>(world::spawn::PlacementType::InWater), 1);
    EXPECT_EQ(static_cast<int>(world::spawn::PlacementType::InLava), 2);
    EXPECT_EQ(static_cast<int>(world::spawn::PlacementType::NoRestrictions), 3);
}

// ========== 注册陆生动物测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, LandAnimalsRegistered) {
    // 验证陆生动物已注册
    const auto* pigEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:pig");
    ASSERT_NE(pigEntry, nullptr);
    EXPECT_EQ(pigEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* cowEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:cow");
    ASSERT_NE(cowEntry, nullptr);
    EXPECT_EQ(cowEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* sheepEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:sheep");
    ASSERT_NE(sheepEntry, nullptr);
    EXPECT_EQ(sheepEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* chickenEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:chicken");
    ASSERT_NE(chickenEntry, nullptr);
    EXPECT_EQ(chickenEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* horseEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:horse");
    ASSERT_NE(horseEntry, nullptr);
    EXPECT_EQ(horseEntry->placementType, world::spawn::PlacementType::OnGround);
}

// ========== 注册水生生物测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, WaterCreaturesRegistered) {
    // 验证水生生物已注册
    const auto* codEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:cod");
    ASSERT_NE(codEntry, nullptr);
    EXPECT_EQ(codEntry->placementType, world::spawn::PlacementType::InWater);

    const auto* salmonEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:salmon");
    ASSERT_NE(salmonEntry, nullptr);
    EXPECT_EQ(salmonEntry->placementType, world::spawn::PlacementType::InWater);

    const auto* squidEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:squid");
    ASSERT_NE(squidEntry, nullptr);
    EXPECT_EQ(squidEntry->placementType, world::spawn::PlacementType::InWater);

    const auto* dolphinEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:dolphin");
    ASSERT_NE(dolphinEntry, nullptr);
    EXPECT_EQ(dolphinEntry->placementType, world::spawn::PlacementType::InWater);
}

// ========== 注册怪物测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, MonstersRegistered) {
    // 验证怪物已注册
    const auto* zombieEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:zombie");
    ASSERT_NE(zombieEntry, nullptr);
    EXPECT_EQ(zombieEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* skeletonEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:skeleton");
    ASSERT_NE(skeletonEntry, nullptr);
    EXPECT_EQ(skeletonEntry->placementType, world::spawn::PlacementType::OnGround);

    const auto* creeperEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:creeper");
    ASSERT_NE(creeperEntry, nullptr);
    EXPECT_EQ(creeperEntry->placementType, world::spawn::PlacementType::OnGround);
}

// ========== 注册岩浆生物测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, LavaCreaturesRegistered) {
    // 验证岩浆生物已注册
    const auto* striderEntry = world::spawn::EntitySpawnPlacementRegistry::getPlacementEntry("minecraft:strider");
    ASSERT_NE(striderEntry, nullptr);
    EXPECT_EQ(striderEntry->placementType, world::spawn::PlacementType::InLava);
}

// ========== SpawnCosts 测试 ==========

TEST_F(EntitySpawnPlacementRegistryTest, SpawnCostsDefaultValues) {
    world::spawn::SpawnCosts costs;
    EXPECT_DOUBLE_EQ(costs.energyBudget, 0.0);
    EXPECT_DOUBLE_EQ(costs.charge, 0.0);
    EXPECT_FALSE(costs.isValid());
}

TEST_F(EntitySpawnPlacementRegistryTest, SpawnCostsValidValues) {
    world::spawn::SpawnCosts costs(1.0, 0.5);
    EXPECT_DOUBLE_EQ(costs.energyBudget, 1.0);
    EXPECT_DOUBLE_EQ(costs.charge, 0.5);
    EXPECT_TRUE(costs.isValid());
}

TEST_F(EntitySpawnPlacementRegistryTest, SpawnCostsInvalidValues) {
    world::spawn::SpawnCosts costs1(0.0, 0.5);
    EXPECT_FALSE(costs1.isValid());

    world::spawn::SpawnCosts costs2(1.0, 0.0);
    EXPECT_FALSE(costs2.isValid());

    world::spawn::SpawnCosts costs3(0.0, 0.0);
    EXPECT_FALSE(costs3.isValid());
}

} // namespace test
} // namespace mc
