#include "server/world/spawn/NaturalSpawner.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <array>
#include <utility>
#include <gtest/gtest.h>

namespace {

mc::world::spawn::EntityDensityManager createTemporaryDensityManager(
    mc::world::spawn::MobDensityTracker& densityTracker)
{
    std::unordered_map<mc::entity::EntityClassification, mc::i32> entityCounts;
    entityCounts[mc::entity::EntityClassification::Monster] = 200;
    entityCounts[mc::entity::EntityClassification::Creature] = 5;
    return mc::world::spawn::EntityDensityManager(10, std::move(entityCounts), densityTracker);
}

void clobberStack()
{
    std::array<mc::u8, 4096> scratch{};
    for (std::size_t i = 0; i < scratch.size(); ++i) {
        scratch[i] = static_cast<mc::u8>(i);
    }

    volatile mc::u8 sink = scratch[0];
    (void)sink;
}

} // namespace

namespace mc {
namespace test {

/**
 * @brief NaturalSpawner 测试套件
 *
 * 测试自然生成器的核心功能。
 */
class NaturalSpawnerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化实体放置注册表
        world::spawn::EntitySpawnPlacementRegistry::initializeDefaults();
    }
};

// ========== MobDensityTracker 测试 ==========

TEST_F(NaturalSpawnerTest, MobDensityTracker_InitialState)
{
    world::spawn::MobDensityTracker tracker;
    EXPECT_EQ(tracker.size(), 0);
}

TEST_F(NaturalSpawnerTest, MobDensityTracker_AddCharge)
{
    world::spawn::MobDensityTracker tracker;
    tracker.addCharge(Vector3(0.0f, 0.0f, 0.0f), 1.0);
    EXPECT_EQ(tracker.size(), 1);
}

TEST_F(NaturalSpawnerTest, MobDensityTracker_GetTotalCharge)
{
    world::spawn::MobDensityTracker tracker;

    // 在原点添加密度
    tracker.addCharge(Vector3(0.0f, 0.0f, 0.0f), 1.0);

    // 在原点应该得到完全的密度值
    f64 chargeAtOrigin = tracker.getTotalCharge(Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_GT(chargeAtOrigin, 0.0);

    // 在远处应该得到较低的密度值
    f64 chargeAtFar = tracker.getTotalCharge(Vector3(100.0f, 0.0f, 0.0f));
    EXPECT_LT(chargeAtFar, chargeAtOrigin);
}

TEST_F(NaturalSpawnerTest, MobDensityTracker_MultipleCharges)
{
    world::spawn::MobDensityTracker tracker;

    // 添加多个密度点
    tracker.addCharge(Vector3(0.0f, 0.0f, 0.0f), 1.0);
    tracker.addCharge(Vector3(10.0f, 0.0f, 0.0f), 1.0);
    tracker.addCharge(Vector3(20.0f, 0.0f, 0.0f), 1.0);

    EXPECT_EQ(tracker.size(), 3);

    // 中间位置应该得到累计密度
    f64 charge = tracker.getTotalCharge(Vector3(10.0f, 0.0f, 0.0f));
    EXPECT_GT(charge, 1.0);
}

TEST_F(NaturalSpawnerTest, MobDensityTracker_Clear)
{
    world::spawn::MobDensityTracker tracker;
    tracker.addCharge(Vector3(0.0f, 0.0f, 0.0f), 1.0);
    EXPECT_EQ(tracker.size(), 1);

    tracker.clear();
    EXPECT_EQ(tracker.size(), 0);
}

TEST_F(NaturalSpawnerTest, MobDensityTracker_DistanceFalloff)
{
    world::spawn::MobDensityTracker tracker;

    // 在原点添加密度
    tracker.addCharge(Vector3(0.0f, 0.0f, 0.0f), 1.0);

    // 在同一位置应该得到完整的密度（无衰减）
    f64 chargeAtOrigin = tracker.getTotalCharge(Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_NEAR(chargeAtOrigin, 1.0, 0.01);

    // 在距离 32 格处应该有衰减
    f64 chargeAt32 = tracker.getTotalCharge(Vector3(32.0f, 0.0f, 0.0f));
    EXPECT_GT(chargeAt32, 0.0);
    EXPECT_LT(chargeAt32, chargeAtOrigin);

    // 在距离 64 格处应该完全衰减
    f64 chargeAt64 = tracker.getTotalCharge(Vector3(64.0f, 0.0f, 0.0f));
    EXPECT_NEAR(chargeAt64, 0.0, 0.01);
}

// ========== EntityDensityManager 测试 ==========

TEST_F(NaturalSpawnerTest, EntityDensityManager_CanSpawn)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    // 初始状态应该可以生成
    world::spawn::EntityDensityManager manager(10, entityCounts, densityTracker);

    // 怪物应该可以生成（数量为0，低于限制70）
    EXPECT_TRUE(manager.canSpawn(entity::EntityClassification::Monster));

    // 动物应该可以生成（数量为0，低于限制10）
    EXPECT_TRUE(manager.canSpawn(entity::EntityClassification::Creature));

    // 环境生物应该可以生成（数量为0，低于限制15）
    EXPECT_TRUE(manager.canSpawn(entity::EntityClassification::Ambient));

    // MISC 分类不应该生成
    EXPECT_FALSE(manager.canSpawn(entity::EntityClassification::Misc));
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_CanSpawnWithLimit)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    // 视距为 10 时，怪物上限会按区块数量缩放到 106
    entityCounts[entity::EntityClassification::Monster] = 107;

    world::spawn::EntityDensityManager manager(10, entityCounts, densityTracker);

    // 超过缩放后的上限时不应该可以生成
    EXPECT_FALSE(manager.canSpawn(entity::EntityClassification::Monster));

    // 动物仍然可以生成
    EXPECT_TRUE(manager.canSpawn(entity::EntityClassification::Creature));
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_CanSpawnBelowLimit)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    // 设置怪物数量接近但未达到上限
    entityCounts[entity::EntityClassification::Monster] = 69;

    world::spawn::EntityDensityManager manager(10, entityCounts, densityTracker);

    // 怪物应该可以生成
    EXPECT_TRUE(manager.canSpawn(entity::EntityClassification::Monster));
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_CanSpawnWithDensity)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    world::spawn::EntityDensityManager manager(10, entityCounts, densityTracker);

    // 无效的 SpawnCosts 应该允许生成
    world::spawn::SpawnCosts invalidCosts(0.0, 0.0);
    EXPECT_TRUE(manager.canSpawnWithDensity("minecraft:zombie", Vector3(0, 0, 0), invalidCosts));

    // 有效的 SpawnCosts 应该允许生成（初始密度为0）
    world::spawn::SpawnCosts validCosts(1.0, 0.5);
    EXPECT_TRUE(manager.canSpawnWithDensity("minecraft:zombie", Vector3(0, 0, 0), validCosts));
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_CanSpawnWithDensityExceeded)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    // 预先添加密度
    densityTracker.addCharge(Vector3(0.0f, 0.0f, 0.0f), 2.0);

    world::spawn::EntityDensityManager manager(10, entityCounts, densityTracker);

    // 在相同位置，如果能量预算为 1.0 且已有 2.0 的密度，应该不允许生成
    world::spawn::SpawnCosts costs(1.0, 0.5);
    EXPECT_FALSE(manager.canSpawnWithDensity("minecraft:zombie", Vector3(0, 0, 0), costs));
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_CountSnapshotSurvivesTemporarySource)
{
    world::spawn::MobDensityTracker densityTracker;
    auto manager = createTemporaryDensityManager(densityTracker);

    clobberStack();

    EXPECT_EQ(manager.getCount(entity::EntityClassification::Monster), 200);
    EXPECT_EQ(manager.getCount(entity::EntityClassification::Creature), 5);
    EXPECT_EQ(manager.getCount(entity::EntityClassification::Ambient), 0);
    EXPECT_FALSE(manager.canSpawn(entity::EntityClassification::Monster));
    EXPECT_TRUE(manager.canSpawn(entity::EntityClassification::Creature));
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_OnSpawn)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    world::spawn::EntityDensityManager manager(10, entityCounts, densityTracker);

    // 生成实体后更新密度
    world::spawn::SpawnCosts costs(1.0, 0.5);
    manager.onSpawn("minecraft:zombie", Vector3(0, 0, 0), costs);

    // 密度追踪器应该记录了这个点
    EXPECT_EQ(densityTracker.size(), 1);
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_OnSpawnWithoutCosts)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    world::spawn::EntityDensityManager manager(10, entityCounts, densityTracker);

    // 生成没有成本的实体不应该添加密度
    world::spawn::SpawnCosts noCosts;
    manager.onSpawn("minecraft:zombie", Vector3(0, 0, 0), noCosts);

    // 密度追踪器不应该记录
    EXPECT_EQ(densityTracker.size(), 0);
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_GetCount)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    entityCounts[entity::EntityClassification::Monster] = 10;
    entityCounts[entity::EntityClassification::Creature] = 5;

    world::spawn::EntityDensityManager manager(10, entityCounts, densityTracker);

    EXPECT_EQ(manager.getCount(entity::EntityClassification::Monster), 10);
    EXPECT_EQ(manager.getCount(entity::EntityClassification::Creature), 5);
    EXPECT_EQ(manager.getCount(entity::EntityClassification::Ambient), 0);
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_ViewDistance)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    world::spawn::EntityDensityManager manager(16, entityCounts, densityTracker);
    EXPECT_EQ(manager.viewDistance(), 16);
}

// ========== NaturalSpawner 基本功能测试 ==========

TEST_F(NaturalSpawnerTest, CreateSpawner)
{
    world::spawn::NaturalSpawner spawner;
    EXPECT_EQ(spawner.getSpawnDistance(), 8);
    EXPECT_EQ(spawner.getSpawnRange(), 20);
    EXPECT_EQ(spawner.getMaxEntities(), 200);
}

TEST_F(NaturalSpawnerTest, SetSpawnDistance)
{
    world::spawn::NaturalSpawner spawner;
    spawner.setSpawnDistance(16);
    EXPECT_EQ(spawner.getSpawnDistance(), 16);
}

TEST_F(NaturalSpawnerTest, SetSpawnRange)
{
    world::spawn::NaturalSpawner spawner;
    spawner.setSpawnRange(32);
    EXPECT_EQ(spawner.getSpawnRange(), 32);
}

TEST_F(NaturalSpawnerTest, SetMaxEntities)
{
    world::spawn::NaturalSpawner spawner;
    spawner.setMaxEntities(100);
    EXPECT_EQ(spawner.getMaxEntities(), 100);
}

// ========== 常量测试 ==========

TEST_F(NaturalSpawnerTest, Constants_MinSpawnDistance)
{
    // 最小生成距离平方：24^2 = 576
    EXPECT_DOUBLE_EQ(world::spawn::NaturalSpawner::MIN_SPAWN_DISTANCE_SQ, 576.0);
}

TEST_F(NaturalSpawnerTest, Constants_MaxSpawnDistance)
{
    // 最大生成距离平方：128^2 = 16384
    EXPECT_DOUBLE_EQ(world::spawn::NaturalSpawner::MAX_SPAWN_DISTANCE_SQ, 16384.0);
}

TEST_F(NaturalSpawnerTest, Constants_MaxMonsters)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_MONSTERS, 70);
}

TEST_F(NaturalSpawnerTest, Constants_MaxCreatures)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_CREATURES, 10);
}

TEST_F(NaturalSpawnerTest, Constants_MaxAmbient)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_AMBIENT, 15);
}

TEST_F(NaturalSpawnerTest, Constants_MaxWaterCreatures)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_WATER_CREATURES, 5);
}

TEST_F(NaturalSpawnerTest, Constants_MaxGroupSize)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_GROUP_SIZE, 4);
}

// ========== SpawnCosts 测试 ==========

TEST_F(NaturalSpawnerTest, SpawnCosts_DefaultValues)
{
    world::spawn::SpawnCosts costs;
    EXPECT_DOUBLE_EQ(costs.energyBudget, 0.0);
    EXPECT_DOUBLE_EQ(costs.charge, 0.0);
    EXPECT_FALSE(costs.isValid());
}

TEST_F(NaturalSpawnerTest, SpawnCosts_ValidValues)
{
    world::spawn::SpawnCosts costs(1.0, 0.5);
    EXPECT_DOUBLE_EQ(costs.energyBudget, 1.0);
    EXPECT_DOUBLE_EQ(costs.charge, 0.5);
    EXPECT_TRUE(costs.isValid());
}

TEST_F(NaturalSpawnerTest, SpawnCosts_ZeroBudget)
{
    world::spawn::SpawnCosts costs(0.0, 0.5);
    EXPECT_FALSE(costs.isValid());
}

TEST_F(NaturalSpawnerTest, SpawnCosts_ZeroCharge)
{
    world::spawn::SpawnCosts costs(1.0, 0.0);
    EXPECT_FALSE(costs.isValid());
}

// ========== 实体分类限制测试 ==========

TEST_F(NaturalSpawnerTest, EntityLimits_Monster)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_MONSTERS, 70);
}

TEST_F(NaturalSpawnerTest, EntityLimits_Creature)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_CREATURES, 10);
}

TEST_F(NaturalSpawnerTest, EntityLimits_Ambient)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_AMBIENT, 15);
}

TEST_F(NaturalSpawnerTest, EntityLimits_WaterCreature)
{
    EXPECT_EQ(world::spawn::NaturalSpawner::MAX_WATER_CREATURES, 5);
}

// ========== MobSpawnInfo 工厂方法测试 ==========

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Plains)
{
    auto info = world::spawn::MobSpawnInfo::createPlains();
    EXPECT_GT(info.getCreatureSpawns().size(), 0);
    EXPECT_GT(info.getMonsterSpawns().size(), 0);
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Forest)
{
    auto info = world::spawn::MobSpawnInfo::createForest();
    EXPECT_GT(info.getCreatureSpawns().size(), 0);
    EXPECT_TRUE(info.isPlayerSpawnFriendly());
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Ocean)
{
    auto info = world::spawn::MobSpawnInfo::createOcean();
    EXPECT_GT(info.getWaterCreatureSpawns().size(), 0);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Desert)
{
    auto info = world::spawn::MobSpawnInfo::createDesert();
    EXPECT_GT(info.getMonsterSpawns().size(), 0);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Snowy)
{
    auto info = world::spawn::MobSpawnInfo::createSnowy();
    EXPECT_GT(info.getMonsterSpawns().size(), 0);
}

} // namespace test
} // namespace mc
