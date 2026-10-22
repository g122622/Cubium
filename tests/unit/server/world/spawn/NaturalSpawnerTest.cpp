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

#include "server/world/spawn/NaturalSpawner.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <array>
#include <cmath>
#include <utility>
#include <gtest/gtest.h>

namespace {

mc::world::spawn::EntityDensityManager createTemporaryDensityManager(
    mc::world::spawn::MobDensityTracker& densityTracker)
{
    std::unordered_map<mc::entity::EntityClassification, mc::i32> entityCounts;
    entityCounts[mc::entity::EntityClassification::Monster] = 200;
    entityCounts[mc::entity::EntityClassification::Creature] = 5;
    return mc::world::spawn::EntityDensityManager(289, std::move(entityCounts), densityTracker);
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
    f64 chargeAtOrigin = tracker.getTotalCharge(Vector3(0.0f, 0.0f, 0.0f), 1.0);
    EXPECT_GT(chargeAtOrigin, 0.0);

    // 在远处应该得到较低的密度值
    f64 chargeAtFar = tracker.getTotalCharge(Vector3(100.0f, 0.0f, 0.0f), 1.0);
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
    f64 charge = tracker.getTotalCharge(Vector3(10.0f, 0.0f, 0.0f), 1.0);
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

    // 密度采用逆衰减公式：sum(charge / sqrt(distSq)) * multiplier。
    // 在与点电荷重合的位置查询时返回无穷大（任何有限预算都会被超出），
    // 这样可以阻止在已存在实体的精确位置上堆叠生成。
    f64 chargeAtOrigin = tracker.getTotalCharge(Vector3(0.0f, 0.0f, 0.0f), 1.0);
    EXPECT_TRUE(std::isinf(chargeAtOrigin));

    // 在距离 32 格处按 1/32 衰减
    f64 chargeAt32 = tracker.getTotalCharge(Vector3(32.0f, 0.0f, 0.0f), 1.0);
    EXPECT_GT(chargeAt32, 0.0);
    EXPECT_LT(chargeAt32, chargeAtOrigin);
    EXPECT_NEAR(chargeAt32, 1.0 / 32.0, 0.001);

    // 逆衰减永远不会真正归零；距离 64 格处按 1/64 衰减
    f64 chargeAt64 = tracker.getTotalCharge(Vector3(64.0f, 0.0f, 0.0f), 1.0);
    EXPECT_GT(chargeAt64, 0.0);
    EXPECT_NEAR(chargeAt64, 1.0 / 64.0, 0.001);
}

// ========== EntityDensityManager 测试 ==========

TEST_F(NaturalSpawnerTest, EntityDensityManager_CanSpawn)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    // 初始状态应该可以生成
    world::spawn::EntityDensityManager manager(289, entityCounts, densityTracker);

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

    // 满载刷怪范围（spawnableChunkCount=289）时，怪物上限 = 70*289/289 = 70
    entityCounts[entity::EntityClassification::Monster] = 107;

    world::spawn::EntityDensityManager manager(289, entityCounts, densityTracker);

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

    world::spawn::EntityDensityManager manager(289, entityCounts, densityTracker);

    // 怪物应该可以生成
    EXPECT_TRUE(manager.canSpawn(entity::EntityClassification::Monster));
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_CanSpawnWithDensity)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    world::spawn::EntityDensityManager manager(289, entityCounts, densityTracker);

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

    world::spawn::EntityDensityManager manager(289, entityCounts, densityTracker);

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

    world::spawn::EntityDensityManager manager(289, entityCounts, densityTracker);

    // 生成实体后更新密度
    world::spawn::SpawnCosts costs(1.0, 0.5);
    manager.onSpawn("minecraft:zombie", entity::EntityClassification::Monster, Vector3(0, 0, 0), costs);

    // 密度追踪器应该记录了这个点
    EXPECT_EQ(densityTracker.size(), 1);
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_OnSpawnWithoutCosts)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    world::spawn::EntityDensityManager manager(289, entityCounts, densityTracker);

    // 生成没有成本的实体不应该添加密度
    world::spawn::SpawnCosts noCosts;
    manager.onSpawn("minecraft:zombie", entity::EntityClassification::Monster, Vector3(0, 0, 0), noCosts);

    // 密度追踪器不应该记录
    EXPECT_EQ(densityTracker.size(), 0);
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_GetCount)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    entityCounts[entity::EntityClassification::Monster] = 10;
    entityCounts[entity::EntityClassification::Creature] = 5;

    world::spawn::EntityDensityManager manager(289, entityCounts, densityTracker);

    EXPECT_EQ(manager.getCount(entity::EntityClassification::Monster), 10);
    EXPECT_EQ(manager.getCount(entity::EntityClassification::Creature), 5);
    EXPECT_EQ(manager.getCount(entity::EntityClassification::Ambient), 0);
}

TEST_F(NaturalSpawnerTest, EntityDensityManager_SpawnableChunkCount)
{
    world::spawn::MobDensityTracker densityTracker;
    std::unordered_map<entity::EntityClassification, i32> entityCounts;

    world::spawn::EntityDensityManager manager(16, entityCounts, densityTracker);
    EXPECT_EQ(manager.spawnableChunkCount(), 16);
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

// 各分类每区块最大实例数由 EntityClassification::getMaxCount 统一提供（cap 公式据此计算）。
TEST_F(NaturalSpawnerTest, EntityLimits_FromClassification)
{
    using ec = entity::EntityClassification;
    EXPECT_EQ(entity::getMaxCount(ec::Monster), 70);
    EXPECT_EQ(entity::getMaxCount(ec::Creature), 10);
    EXPECT_EQ(entity::getMaxCount(ec::Ambient), 15);
    EXPECT_EQ(entity::getMaxCount(ec::WaterCreature), 5);
    EXPECT_EQ(entity::getMaxCount(ec::WaterAmbient), 20);
}

TEST_F(NaturalSpawnerTest, Constants_SpawnDistanceChunk)
{
    // 固定刷怪距离 8 区块
    EXPECT_EQ(world::spawn::NaturalSpawner::SPAWN_DISTANCE_CHUNK, 8);
}

TEST_F(NaturalSpawnerTest, Constants_MagicNumber)
{
    // 刷怪区块计数基准 17^2 = 289 = (2*SPAWN_DISTANCE_CHUNK+1)^2
    constexpr i32 side = 2 * world::spawn::NaturalSpawner::SPAWN_DISTANCE_CHUNK + 1;
    EXPECT_EQ(side * side, 289);
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

// ========== MobSpawnInfo 工厂方法测试 ==========
//
// 期望值逐项对应 MC Java 1.21.11 net/minecraft/data/worldgen/BiomeDefaultFeatures.java 与
// net/minecraft/data/worldgen/biome/{OverworldBiomes,NetherBiomes}.java。
// 尚未实现的实体不在内部刷怪表中出现（当前均已实现，列表为空）。

namespace {

/// 在实体的指定分类列表中查找条目；未找到返回 nullptr。
const world::spawn::SpawnEntry* findSpawn(
    const std::vector<world::spawn::SpawnEntry>& entries, const std::string& entityTypeId)
{
    for (const auto& entry : entries) {
        if (entry.entityTypeId == entityTypeId) {
            return &entry;
        }
    }
    return nullptr;
}

/// 断言条目存在且权重/群组范围符合原版数据。
void expectSpawn(const std::vector<world::spawn::SpawnEntry>& entries,
    const std::string& entityTypeId,
    i32 weight,
    i32 minCount,
    i32 maxCount)
{
    const auto* entry = findSpawn(entries, entityTypeId);
    ASSERT_NE(entry, nullptr) << "missing spawn entry " << entityTypeId;
    EXPECT_EQ(entry->weight, weight) << entityTypeId;
    EXPECT_EQ(entry->minCount, minCount) << entityTypeId;
    EXPECT_EQ(entry->maxCount, maxCount) << entityTypeId;
}

/// 标准 commonSpawns 的怪物部分：monsters(95, 5, 0, 100)，共 8 条；
/// 部分生物群系（丛林追加豹猫、海洋/河流追加溺尸）在 commonSpawns 之后还有额外怪物。
void expectCommonMonsters(const std::vector<world::spawn::SpawnEntry>& monsters, i32 extraCount = 0)
{
    EXPECT_EQ(monsters.size(), 8u + static_cast<size_t>(extraCount));
    expectSpawn(monsters, "minecraft:spider", 100, 4, 4);
    expectSpawn(monsters, "minecraft:zombie", 95, 4, 4);
    expectSpawn(monsters, "minecraft:zombie_villager", 5, 1, 1);
    expectSpawn(monsters, "minecraft:skeleton", 100, 4, 4);
    expectSpawn(monsters, "minecraft:creeper", 100, 4, 4);
    expectSpawn(monsters, "minecraft:slime", 100, 4, 4);
    expectSpawn(monsters, "minecraft:enderman", 10, 1, 4);
    expectSpawn(monsters, "minecraft:witch", 5, 1, 1);
    EXPECT_EQ(findSpawn(monsters, "minecraft:zombie_horse"), nullptr);
}

/// commonSpawns 的洞穴部分：蝙蝠(10,8,8) + 发光鱿鱼(10,4,6)。
void expectCaveSpawns(const world::spawn::MobSpawnInfo& info)
{
    ASSERT_EQ(info.getAmbientSpawns().size(), 1u);
    expectSpawn(info.getAmbientSpawns(), "minecraft:bat", 10, 8, 8);
    ASSERT_EQ(info.getUndergroundWaterCreatureSpawns().size(), 1u);
    expectSpawn(info.getUndergroundWaterCreatureSpawns(), "minecraft:glow_squid", 10, 4, 6);
}

/// commonSpawnWithZombieHorse 的怪物部分：monsters(90, 5, 5, 100)，共 9 条。
void expectCommonMonstersWithZombieHorse(const std::vector<world::spawn::SpawnEntry>& monsters)
{
    EXPECT_EQ(monsters.size(), 9u);
    expectSpawn(monsters, "minecraft:zombie", 90, 4, 4);
    expectSpawn(monsters, "minecraft:zombie_villager", 5, 1, 1);
    expectSpawn(monsters, "minecraft:zombie_horse", 5, 1, 1);
    expectSpawn(monsters, "minecraft:skeleton", 100, 4, 4);
    expectSpawn(monsters, "minecraft:witch", 5, 1, 1);
}

} // namespace

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Plains)
{
    // OverworldBiomes.plains → plainsSpawns：farmAnimals + horse + donkey + commonSpawnWithZombieHorse
    auto info = world::spawn::MobSpawnInfo::createPlains();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);
    EXPECT_TRUE(info.isPlayerSpawnFriendly());

    const auto& creatures = info.getCreatureSpawns();
    expectSpawn(creatures, "minecraft:sheep", 12, 4, 4);
    expectSpawn(creatures, "minecraft:pig", 10, 4, 4);
    expectSpawn(creatures, "minecraft:chicken", 10, 4, 4);
    expectSpawn(creatures, "minecraft:cow", 8, 4, 4);
    expectSpawn(creatures, "minecraft:horse", 5, 2, 6);
    expectSpawn(creatures, "minecraft:donkey", 1, 1, 3);
    EXPECT_EQ(creatures.size(), 6u);

    expectCommonMonstersWithZombieHorse(info.getMonsterSpawns());
    expectCaveSpawns(info);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Forest)
{
    // OverworldBiomes.forest(birch=false, flower=false)：farmAnimals + commonSpawns + wolf(5,4,4)
    auto info = world::spawn::MobSpawnInfo::createForest();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());

    const auto& creatures = info.getCreatureSpawns();
    int chickenCount = 0;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:chicken") {
            ++chickenCount;
        }
    }
    EXPECT_EQ(chickenCount, 1); // 普通森林没有 baseJungleSpawns 的额外鸡
    expectSpawn(creatures, "minecraft:wolf", 5, 4, 4);
    EXPECT_EQ(creatures.size(), 5u);

    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
    expectCaveSpawns(info);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_BirchForest_HasNoWolfOrRabbit)
{
    // 桦木森林（含原始桦木森林、桦木森林丘陵）：farmAnimals + commonSpawns，无 wolf、无 rabbit
    auto info = world::spawn::MobSpawnInfo::createBirchForest();
    EXPECT_EQ(info.getCreatureSpawns().size(), 4u);
    EXPECT_EQ(findSpawn(info.getCreatureSpawns(), "minecraft:wolf"), nullptr);
    EXPECT_EQ(findSpawn(info.getCreatureSpawns(), "minecraft:rabbit"), nullptr);
    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_FlowerForest_UsesRabbitInsteadOfWolf)
{
    // 花森林：farmAnimals + commonSpawns + rabbit(4,2,3)
    auto info = world::spawn::MobSpawnInfo::createFlowerForest();
    expectSpawn(info.getCreatureSpawns(), "minecraft:rabbit", 4, 2, 3);
    EXPECT_EQ(info.getCreatureSpawns().size(), 5u);
    EXPECT_EQ(findSpawn(info.getCreatureSpawns(), "minecraft:wolf"), nullptr);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_DarkForest_AndPaleGarden)
{
    // 深色森林：farmAnimals + commonSpawns（无 wolf）
    auto darkForest = world::spawn::MobSpawnInfo::createDarkForest();
    EXPECT_EQ(darkForest.getCreatureSpawns().size(), 4u);
    EXPECT_EQ(findSpawn(darkForest.getCreatureSpawns(), "minecraft:wolf"), nullptr);
    expectCommonMonsters(darkForest.getMonsterSpawns());

    // 苍园：仅 commonSpawns，没有动物
    auto paleGarden = world::spawn::MobSpawnInfo::createPaleGarden();
    EXPECT_TRUE(paleGarden.getCreatureSpawns().empty());
    expectCommonMonsters(paleGarden.getMonsterSpawns());
    expectCaveSpawns(paleGarden);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Badlands)
{
    // 恶地：creatureGenerationProbability 0.03；farmAnimals + commonSpawns + armadillo(6,1,2)
    auto badlands = world::spawn::MobSpawnInfo::createBadlands();
    EXPECT_FLOAT_EQ(badlands.getCreatureSpawnProbability(), 0.03f);
    EXPECT_EQ(badlands.getCreatureSpawns().size(), 5u);
    expectSpawn(badlands.getCreatureSpawns(), "minecraft:armadillo", 6, 1, 2);
    expectCommonMonsters(badlands.getMonsterSpawns());

    // 疏林恶地：概率 0.04，追加 wolf(2,4,8)
    auto wooded = world::spawn::MobSpawnInfo::createWoodedBadlands();
    EXPECT_FLOAT_EQ(wooded.getCreatureSpawnProbability(), 0.04f);
    expectSpawn(wooded.getCreatureSpawns(), "minecraft:wolf", 2, 4, 8);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_SoulSandValley_SpawnCosts)
{
    // 原版 NetherBiomes.soulSandValley：addMobCharge(0.7, 0.15) → energyBudget=0.7、charge=0.15
    auto info = world::spawn::MobSpawnInfo::createSoulSandValley();

    for (const char* entityTypeId :
        {"minecraft:skeleton", "minecraft:ghast", "minecraft:enderman", "minecraft:strider"}) {
        const auto* costs = info.getSpawnCost(entityTypeId);
        ASSERT_NE(costs, nullptr) << entityTypeId;
        EXPECT_DOUBLE_EQ(costs->energyBudget, 0.7) << entityTypeId;
        EXPECT_DOUBLE_EQ(costs->charge, 0.15) << entityTypeId;
    }

    expectSpawn(info.getMonsterSpawns(), "minecraft:skeleton", 20, 5, 5);
    expectSpawn(info.getMonsterSpawns(), "minecraft:ghast", 50, 4, 4);
    expectSpawn(info.getMonsterSpawns(), "minecraft:enderman", 1, 4, 4);
    expectSpawn(info.getCreatureSpawns(), "minecraft:strider", 60, 1, 2);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_WarpedForest_SpawnCosts)
{
    // 原版 NetherBiomes.warpedForest：addMobCharge(ENDERNAN, 1.0, 0.12)
    auto info = world::spawn::MobSpawnInfo::createWarpedForest();
    const auto* costs = info.getSpawnCost("minecraft:enderman");
    ASSERT_NE(costs, nullptr);
    EXPECT_DOUBLE_EQ(costs->energyBudget, 1.0);
    EXPECT_DOUBLE_EQ(costs->charge, 0.12);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Ocean)
{
    // OverworldBiomes.ocean：oceanSpawns(1,4,10) + dolphin(1,1,2) + nautilus(5,1,1)
    auto info = world::spawn::MobSpawnInfo::createOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物 + drowned
    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
    expectSpawn(info.getMonsterSpawns(), "minecraft:drowned", 5, 1, 1);
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);

    expectCaveSpawns(info);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:squid", 1, 1, 4);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:dolphin", 1, 1, 2);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:nautilus", 5, 1, 1);
    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:cod", 10, 3, 6);
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 1u);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_LukewarmOcean)
{
    // OverworldBiomes.lukeWarmOcean：pufferfish(5,1,3) + tropical_fish(25,8,8) + dolphin(2,1,2)
    //   + nautilus(5,1,1) + oceanSpawns(10,2,15)
    auto info = world::spawn::MobSpawnInfo::createLukewarmOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
    expectSpawn(info.getMonsterSpawns(), "minecraft:drowned", 5, 1, 1);

    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:squid", 10, 1, 2);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:dolphin", 2, 1, 2);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:nautilus", 5, 1, 1);
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u);

    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:cod", 15, 3, 6);
    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:pufferfish", 5, 1, 3);
    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:tropical_fish", 25, 8, 8);
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 3u);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_DeepLukewarmOcean)
{
    // 深水版本：仅 oceanSpawns 的 squid(8,1,4) 与 cod(8,3,6) 参数不同
    auto info = world::spawn::MobSpawnInfo::createDeepLukewarmOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:squid", 8, 1, 4);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:nautilus", 5, 1, 1);
    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:cod", 8, 3, 6);

    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);       // 8 标准怪物 + drowned
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u); // squid + dolphin + nautilus
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 3u);  // cod + pufferfish + tropical_fish
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_ColdOcean)
{
    // OverworldBiomes.coldOcean：oceanSpawns(3,4,15) + salmon(15,1,5) + nautilus(2,1,1)
    auto info = world::spawn::MobSpawnInfo::createColdOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
    expectSpawn(info.getMonsterSpawns(), "minecraft:drowned", 5, 1, 1);

    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:squid", 3, 1, 4);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:nautilus", 2, 1, 1);
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 2u); // 冷水海洋无 dolphin

    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:cod", 15, 3, 6);
    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:salmon", 15, 1, 5);
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 2u);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_FrozenOcean)
{
    // OverworldBiomes.frozenOcean：squid(1,1,4) + salmon(15,1,5) + polar_bear(1,1,2)
    //   + nautilus(2,1,1) + commonSpawns + drowned(5,1,1)
    auto info = world::spawn::MobSpawnInfo::createFrozenOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
    expectSpawn(info.getMonsterSpawns(), "minecraft:drowned", 5, 1, 1);
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);

    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:squid", 1, 1, 4);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:nautilus", 2, 1, 1);
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 2u);

    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:salmon", 15, 1, 5);
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 1u);

    expectSpawn(info.getCreatureSpawns(), "minecraft:polar_bear", 1, 1, 2);
    EXPECT_EQ(info.getCreatureSpawns().size(), 1u);

    EXPECT_EQ(findSpawn(info.getMonsterSpawns(), "minecraft:stray"), nullptr);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_DeepOcean)
{
    // 深海：spawn list 与普通海洋完全一致
    auto info = world::spawn::MobSpawnInfo::createDeepOcean();
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:nautilus", 5, 1, 1);
    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:cod", 10, 3, 6);
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u);
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);
    EXPECT_EQ(findSpawn(info.getMonsterSpawns(), "minecraft:guardian"), nullptr);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_WarmOcean)
{
    // OverworldBiomes.warmOcean：pufferfish(15,1,3) + nautilus(5,1,1) + warmOceanSpawns(10,4)
    auto info = world::spawn::MobSpawnInfo::createWarmOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
    expectSpawn(info.getMonsterSpawns(), "minecraft:drowned", 5, 1, 1);
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);

    expectCaveSpawns(info);

    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:squid", 10, 4, 4);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:dolphin", 2, 1, 2);
    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:nautilus", 5, 1, 1);
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u);

    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:pufferfish", 15, 1, 3);
    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:tropical_fish", 25, 8, 8);
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 2u);

    // 暖水海洋不含 cod / salmon
    EXPECT_EQ(findSpawn(info.getWaterCreatureSpawns(), "minecraft:cod"), nullptr);
    EXPECT_EQ(findSpawn(info.getWaterAmbientSpawns(), "minecraft:cod"), nullptr);
    EXPECT_EQ(findSpawn(info.getWaterAmbientSpawns(), "minecraft:salmon"), nullptr);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_DeepWarmOcean)
{
    // 深水暖水海洋：spawn list 与暖水海洋一致
    auto warm = world::spawn::MobSpawnInfo::createWarmOcean();
    auto deepWarm = world::spawn::MobSpawnInfo::createDeepWarmOcean();
    EXPECT_EQ(deepWarm.getMonsterSpawns().size(), warm.getMonsterSpawns().size());
    EXPECT_EQ(deepWarm.getWaterCreatureSpawns().size(), warm.getWaterCreatureSpawns().size());
    EXPECT_EQ(deepWarm.getWaterAmbientSpawns().size(), warm.getWaterAmbientSpawns().size());
    const auto* deepDrowned = findSpawn(deepWarm.getMonsterSpawns(), "minecraft:drowned");
    ASSERT_NE(deepDrowned, nullptr);
    expectSpawn(warm.getMonsterSpawns(),
        "minecraft:drowned",
        deepDrowned->weight,
        deepDrowned->minCount,
        deepDrowned->maxCount);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_River)
{
    // OverworldBiomes.river：squid(2,1,4) + salmon(5,1,5) + commonSpawns + drowned(100,1,1)
    auto info = world::spawn::MobSpawnInfo::createRiver();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    expectSpawn(info.getWaterCreatureSpawns(), "minecraft:squid", 2, 1, 4);
    expectSpawn(info.getWaterAmbientSpawns(), "minecraft:salmon", 5, 1, 5);
    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
    expectSpawn(info.getMonsterSpawns(), "minecraft:drowned", 100, 1, 1);
    EXPECT_TRUE(info.getCreatureSpawns().empty());

    // 冻河：溺尸权重降为 1
    auto frozen = world::spawn::MobSpawnInfo::createFrozenRiver();
    expectSpawn(frozen.getMonsterSpawns(), "minecraft:drowned", 1, 1, 1);
    expectSpawn(frozen.getWaterAmbientSpawns(), "minecraft:salmon", 5, 1, 5);
    expectCommonMonsters(frozen.getMonsterSpawns(), 1);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Beach)
{
    // 沙滩：turtle(5,2,5) + commonSpawns；积雪沙滩与石岸都没有海龟
    auto beach = world::spawn::MobSpawnInfo::createBeach();
    expectSpawn(beach.getCreatureSpawns(), "minecraft:turtle", 5, 2, 5);
    EXPECT_EQ(beach.getCreatureSpawns().size(), 1u);
    expectCommonMonsters(beach.getMonsterSpawns());

    auto snowyBeach = world::spawn::MobSpawnInfo::createSnowyBeach();
    EXPECT_TRUE(snowyBeach.getCreatureSpawns().empty());
    expectCommonMonsters(snowyBeach.getMonsterSpawns());

    auto stoneShore = world::spawn::MobSpawnInfo::createStoneShore();
    EXPECT_TRUE(stoneShore.getCreatureSpawns().empty());
    expectCommonMonsters(stoneShore.getMonsterSpawns());
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Swamp)
{
    // 沼泽：farmAnimals + swampSpawns(70) = commonSpawns(70) + slime(1,1,1) + bogged(30,4,4) + frog(10,2,5)
    auto info = world::spawn::MobSpawnInfo::createSwamp();
    EXPECT_EQ(info.getCreatureSpawns().size(), 5u); // farmAnimals + 青蛙
    expectSpawn(info.getCreatureSpawns(), "minecraft:frog", 10, 2, 5);

    expectCaveSpawns(info);
    expectSpawn(info.getMonsterSpawns(), "minecraft:skeleton", 70, 4, 4);
    expectSpawn(info.getMonsterSpawns(), "minecraft:bogged", 30, 4, 4);
    // slime 出现两条：commonSpawns 的 100(4,4) 与沼泽专属的 1(1,1)
    i32 slimeCount = 0;
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:slime") {
            ++slimeCount;
        }
    }
    EXPECT_EQ(slimeCount, 2);
    EXPECT_EQ(info.getMonsterSpawns().size(), 10u);

    // 红树林沼泽：无 farmAnimals，额外热带鱼与青蛙
    auto mangrove = world::spawn::MobSpawnInfo::createMangroveSwamp();
    EXPECT_EQ(mangrove.getCreatureSpawns().size(), 1u);
    expectSpawn(mangrove.getCreatureSpawns(), "minecraft:frog", 10, 2, 5);
    expectSpawn(mangrove.getWaterAmbientSpawns(), "minecraft:tropical_fish", 25, 8, 8);
    expectSpawn(mangrove.getMonsterSpawns(), "minecraft:bogged", 30, 4, 4);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Snowy)
{
    // OverworldBiomes.plains(snowy=true) → snowySpawns(true)：rabbit + polar_bear + caveSpawns
    //   + monsters(90,5,5,20) + stray(80,4,4)
    auto info = world::spawn::MobSpawnInfo::createSnowy();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.07f);

    expectSpawn(info.getCreatureSpawns(), "minecraft:rabbit", 10, 2, 3);
    expectSpawn(info.getCreatureSpawns(), "minecraft:polar_bear", 1, 1, 2);
    EXPECT_EQ(info.getCreatureSpawns().size(), 2u);

    const auto& monsters = info.getMonsterSpawns();
    expectSpawn(monsters, "minecraft:zombie", 90, 4, 4);
    expectSpawn(monsters, "minecraft:zombie_horse", 5, 1, 1);
    expectSpawn(monsters, "minecraft:skeleton", 20, 4, 4);
    expectSpawn(monsters, "minecraft:stray", 80, 4, 4);
    EXPECT_EQ(monsters.size(), 10u);

    expectCaveSpawns(info);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_MushroomFields)
{
    // 蘑菇岛：mooshroom(CREATURE 8,4,8) + caveSpawns，没有怪物
    auto info = world::spawn::MobSpawnInfo::createMushroomFields();
    expectSpawn(info.getCreatureSpawns(), "minecraft:mooshroom", 8, 4, 8);
    EXPECT_EQ(info.getCreatureSpawns().size(), 1u);
    EXPECT_TRUE(info.getMonsterSpawns().empty());
    expectCaveSpawns(info);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_MeadowAndCherryGrove)
{
    // meadowOrCherryGrove：草甸 donkey(1,1,2)、樱花树林 pig(1,1,2)；两者都有 rabbit(2,2,6) + sheep(2,2,4)
    auto meadow = world::spawn::MobSpawnInfo::createMeadow();
    expectSpawn(meadow.getCreatureSpawns(), "minecraft:donkey", 1, 1, 2);
    expectSpawn(meadow.getCreatureSpawns(), "minecraft:rabbit", 2, 2, 6);
    expectSpawn(meadow.getCreatureSpawns(), "minecraft:sheep", 2, 2, 4);
    EXPECT_EQ(meadow.getCreatureSpawns().size(), 3u);
    expectCommonMonsters(meadow.getMonsterSpawns());

    auto cherry = world::spawn::MobSpawnInfo::createCherryGrove();
    expectSpawn(cherry.getCreatureSpawns(), "minecraft:pig", 1, 1, 2);
    EXPECT_EQ(findSpawn(cherry.getCreatureSpawns(), "minecraft:donkey"), nullptr);
    EXPECT_EQ(cherry.getCreatureSpawns().size(), 3u);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_GroveAndSlopes)
{
    // 雪林：wolf(1,1,1) + rabbit(8,2,3) + fox(4,2,4) + commonSpawns
    auto grove = world::spawn::MobSpawnInfo::createGrove();
    expectSpawn(grove.getCreatureSpawns(), "minecraft:wolf", 1, 1, 1);
    expectSpawn(grove.getCreatureSpawns(), "minecraft:rabbit", 8, 2, 3);
    expectSpawn(grove.getCreatureSpawns(), "minecraft:fox", 4, 2, 4);
    EXPECT_EQ(grove.getCreatureSpawns().size(), 3u);

    // 雪坡：rabbit(4,2,3) + goat(5,1,3)
    auto slopes = world::spawn::MobSpawnInfo::createSnowySlopes();
    expectSpawn(slopes.getCreatureSpawns(), "minecraft:rabbit", 4, 2, 3);
    expectSpawn(slopes.getCreatureSpawns(), "minecraft:goat", 5, 1, 3);
    EXPECT_EQ(slopes.getCreatureSpawns().size(), 2u);
    // 山峰：goat(5,1,3) + commonSpawns
    auto peaks = world::spawn::MobSpawnInfo::createGoatPeaks();
    expectSpawn(peaks.getCreatureSpawns(), "minecraft:goat", 5, 1, 3);
    EXPECT_EQ(peaks.getCreatureSpawns().size(), 1u);
    expectCommonMonsters(peaks.getMonsterSpawns());
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_WindsweptHills)
{
    // 疾风丘陵：farmAnimals + llama(5,4,6) + commonSpawns
    auto info = world::spawn::MobSpawnInfo::createMountains();
    expectSpawn(info.getCreatureSpawns(), "minecraft:llama", 5, 4, 6);
    EXPECT_EQ(info.getCreatureSpawns().size(), 5u);
    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);

    // 带森林变体：刷怪表一致
    auto forest = world::spawn::MobSpawnInfo::createWindsweptForest();
    EXPECT_EQ(forest.getCreatureSpawns().size(), info.getCreatureSpawns().size());
    EXPECT_EQ(forest.getMonsterSpawns().size(), info.getMonsterSpawns().size());
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Desert)
{
    // desertSpawns：rabbit(12,2,3) + camel(1,1,1) + caveSpawns + monsters(19,1,0,50)
    //   + husk(80,4,4) + parched(50,4,4)
    auto info = world::spawn::MobSpawnInfo::createDesert();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    expectSpawn(info.getCreatureSpawns(), "minecraft:rabbit", 12, 2, 3);
    expectSpawn(info.getCreatureSpawns(), "minecraft:camel", 1, 1, 1);
    EXPECT_EQ(info.getCreatureSpawns().size(), 2u);

    expectCaveSpawns(info);
    expectSpawn(info.getMonsterSpawns(), "minecraft:zombie", 19, 4, 4);
    expectSpawn(info.getMonsterSpawns(), "minecraft:zombie_villager", 1, 1, 1);
    expectSpawn(info.getMonsterSpawns(), "minecraft:skeleton", 50, 4, 4);
    expectSpawn(info.getMonsterSpawns(), "minecraft:husk", 80, 4, 4);
    expectSpawn(info.getMonsterSpawns(), "minecraft:parched", 50, 4, 4);
    EXPECT_EQ(info.getMonsterSpawns().size(), 10u);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Savanna)
{
    // 普通/破碎热带草原：horse(1,2,6) + donkey(1,1,1) + armadillo(10,2,3) + commonSpawnWithZombieHorse；
    //   1.21.11 不再调用 farmAnimals，也没有 llama / wolf
    auto info = world::spawn::MobSpawnInfo::createSavanna();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    expectSpawn(info.getCreatureSpawns(), "minecraft:horse", 1, 2, 6);
    expectSpawn(info.getCreatureSpawns(), "minecraft:donkey", 1, 1, 1);
    expectSpawn(info.getCreatureSpawns(), "minecraft:armadillo", 10, 2, 3);
    EXPECT_EQ(info.getCreatureSpawns().size(), 3u);

    expectCommonMonstersWithZombieHorse(info.getMonsterSpawns());

    auto shattered = world::spawn::MobSpawnInfo::createShatteredSavanna();
    EXPECT_EQ(shattered.getCreatureSpawns().size(), 3u);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_SavannaPlateau)
{
    // savanna(llamaAndWolves=true)：在普通热带草原基础上加 llama(8,4,4) 与 wolf(8,4,8)
    auto info = world::spawn::MobSpawnInfo::createSavannaPlateau();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());

    expectSpawn(info.getCreatureSpawns(), "minecraft:llama", 8, 4, 4);
    expectSpawn(info.getCreatureSpawns(), "minecraft:wolf", 8, 4, 8);
    expectSpawn(info.getCreatureSpawns(), "minecraft:armadillo", 10, 2, 3);
    EXPECT_EQ(info.getCreatureSpawns().size(), 5u);
    expectCommonMonstersWithZombieHorse(info.getMonsterSpawns());
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Jungle)
{
    // baseJungleSpawns 在 farmAnimals 之外再追加一条 chicken(10,4,4)
    auto info = world::spawn::MobSpawnInfo::createJungle();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());

    int chickenCount = 0;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:chicken") {
            ++chickenCount;
        }
    }
    EXPECT_EQ(chickenCount, 2);
    expectSpawn(info.getCreatureSpawns(), "minecraft:parrot", 40, 1, 2);
    expectSpawn(info.getCreatureSpawns(), "minecraft:panda", 1, 1, 2);
    expectSpawn(info.getMonsterSpawns(), "minecraft:ocelot", 2, 1, 3);
    expectCommonMonsters(info.getMonsterSpawns(), 1);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_SparseJungle)
{
    // sparseJungle：baseJungleSpawns + wolf(8,2,4)
    auto info = world::spawn::MobSpawnInfo::createSparseJungle();
    expectSpawn(info.getCreatureSpawns(), "minecraft:wolf", 8, 2, 4);
    EXPECT_EQ(findSpawn(info.getCreatureSpawns(), "minecraft:parrot"), nullptr);
    EXPECT_EQ(findSpawn(info.getCreatureSpawns(), "minecraft:panda"), nullptr);
    EXPECT_EQ(findSpawn(info.getMonsterSpawns(), "minecraft:ocelot"), nullptr);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_BambooJungle)
{
    // bambooJungle：baseJungleSpawns + parrot(40,1,2) + panda(80,1,2) + ocelot(2,1,1)
    auto info = world::spawn::MobSpawnInfo::createBambooJungle();
    expectSpawn(info.getCreatureSpawns(), "minecraft:parrot", 40, 1, 2);
    expectSpawn(info.getCreatureSpawns(), "minecraft:panda", 80, 1, 2);
    expectSpawn(info.getMonsterSpawns(), "minecraft:ocelot", 2, 1, 1);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Taiga)
{
    // taiga：farmAnimals + wolf(8,4,4) + rabbit(4,2,3) + fox(8,2,4) + commonSpawns
    auto info = world::spawn::MobSpawnInfo::createTaiga();
    expectSpawn(info.getCreatureSpawns(), "minecraft:wolf", 8, 4, 4);
    expectSpawn(info.getCreatureSpawns(), "minecraft:rabbit", 4, 2, 3);
    expectSpawn(info.getCreatureSpawns(), "minecraft:fox", 8, 2, 4);
    EXPECT_EQ(info.getCreatureSpawns().size(), 7u);
    expectCommonMonsters(
        info.getMonsterSpawns(), findSpawn(info.getMonsterSpawns(), "minecraft:drowned") != nullptr ? 1 : 0);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Caves)
{
    // 繁茂洞穴：axolotl(10,4,6) + tropical_fish(25,8,8) + commonSpawns
    auto lush = world::spawn::MobSpawnInfo::createLushCaves();
    expectSpawn(lush.getAxolotlSpawns(), "minecraft:axolotl", 10, 4, 6);
    expectSpawn(lush.getWaterAmbientSpawns(), "minecraft:tropical_fish", 25, 8, 8);
    expectCommonMonsters(lush.getMonsterSpawns());

    // 溶洞：caveSpawns + monsters(95,5,0,100) + drowned(95,4,4) + commonSpawns
    auto dripstone = world::spawn::MobSpawnInfo::createDripstoneCaves();
    expectCaveSpawns(dripstone);
    expectSpawn(dripstone.getMonsterSpawns(), "minecraft:drowned", 95, 4, 4);
    expectCommonMonsters(dripstone.getMonsterSpawns(), 1);
    EXPECT_EQ(dripstone.getMonsterSpawns().size(), 9u);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_NetherAndEnd)
{
    // 下界荒地
    auto wastes = world::spawn::MobSpawnInfo::createNetherWastes();
    expectSpawn(wastes.getMonsterSpawns(), "minecraft:ghast", 50, 4, 4);
    expectSpawn(wastes.getMonsterSpawns(), "minecraft:zombified_piglin", 100, 4, 4);
    expectSpawn(wastes.getMonsterSpawns(), "minecraft:magma_cube", 2, 4, 4);
    expectSpawn(wastes.getMonsterSpawns(), "minecraft:enderman", 1, 4, 4);
    expectSpawn(wastes.getMonsterSpawns(), "minecraft:piglin", 15, 4, 4);
    expectSpawn(wastes.getCreatureSpawns(), "minecraft:strider", 60, 1, 2);

    // 绯红森林
    auto crimson = world::spawn::MobSpawnInfo::createCrimsonForest();
    expectSpawn(crimson.getMonsterSpawns(), "minecraft:zombified_piglin", 1, 2, 4);
    expectSpawn(crimson.getMonsterSpawns(), "minecraft:hoglin", 9, 3, 4);
    expectSpawn(crimson.getMonsterSpawns(), "minecraft:piglin", 5, 3, 4);

    // 玄武岩三角洲
    auto basalt = world::spawn::MobSpawnInfo::createBasaltDeltas();
    expectSpawn(basalt.getMonsterSpawns(), "minecraft:ghast", 40, 1, 1);
    expectSpawn(basalt.getMonsterSpawns(), "minecraft:magma_cube", 100, 2, 5);

    // 末地：仅末影人
    auto end = world::spawn::MobSpawnInfo::createTheEnd();
    expectSpawn(end.getMonsterSpawns(), "minecraft:enderman", 10, 4, 4);
    EXPECT_EQ(end.getMonsterSpawns().size(), 1u);
    EXPECT_TRUE(end.getCreatureSpawns().empty());
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Empty)
{
    auto info = world::spawn::MobSpawnInfo::createEmpty();
    EXPECT_EQ(info.getMonsterSpawns().size(), 0u);
    EXPECT_EQ(info.getCreatureSpawns().size(), 0u);
    EXPECT_EQ(info.getAmbientSpawns().size(), 0u);
}

} // namespace test
} // namespace mc
