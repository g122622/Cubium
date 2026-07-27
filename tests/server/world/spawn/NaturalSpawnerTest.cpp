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

    // 原版 Forest chicken 仅一条 weight=10 条目；此前源码误加两条，已收敛
    int chickenCount = 0;
    bool hasWolf = false;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:chicken") ++chickenCount;
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
    }
    EXPECT_EQ(chickenCount, 1);
    EXPECT_TRUE(hasWolf);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_SoulSandValley_SpawnCosts)
{
    // 原版 1.16.5：灵魂沙谷 energyBudget=0.15，charge=0.7
    // 此前源码误用 0.12（与 WarpedForest 混淆），已收敛为 0.15
    auto info = world::spawn::MobSpawnInfo::createSoulSandValley();

    const auto checkCost = [&](const std::string& entityTypeId) {
        const auto* costs = info.getSpawnCost(entityTypeId);
        ASSERT_NE(costs, nullptr);
        EXPECT_DOUBLE_EQ(costs->energyBudget, 0.15);
        EXPECT_DOUBLE_EQ(costs->charge, 0.7);
    };
    checkCost("minecraft:skeleton");
    checkCost("minecraft:ghast");
    checkCost("minecraft:enderman");
    checkCost("minecraft:strider");
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Ocean)
{
    // 对应 MC 1.21.11 BiomeMaker.func_244234_c(false)（浅水版本）
    auto info = world::spawn::MobSpawnInfo::createOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物 + drowned
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);
    bool hasDrowned = false;
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:drowned") {
            hasDrowned = true;
            EXPECT_EQ(entry.weight, 5);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    EXPECT_TRUE(hasDrowned);

    // 水生生物：squid + dolphin + nautilus（1.21.11 原版归 WaterCreature）
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u);
    bool hasSquid = false, hasDolphin = false, hasNautilus = false;
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:squid") {
            hasSquid = true;
            EXPECT_EQ(entry.weight, 1);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 4);
        }
        if (entry.entityTypeId == "minecraft:dolphin") {
            hasDolphin = true;
            EXPECT_EQ(entry.weight, 1);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 2);
        }
        if (entry.entityTypeId == "minecraft:nautilus") {
            hasNautilus = true;
            EXPECT_EQ(entry.weight, 5);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    EXPECT_TRUE(hasSquid);
    EXPECT_TRUE(hasDolphin);
    EXPECT_TRUE(hasNautilus);

    // 水生环境生物：cod（原版归 WaterAmbient，非 WaterCreature）
    ASSERT_EQ(info.getWaterAmbientSpawns().size(), 1u);
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].entityTypeId, "minecraft:cod");
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].weight, 10);
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].minCount, 3);
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].maxCount, 6);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_LukewarmOcean)
{
    // 对应 MC 1.21.11 BiomeMaker.func_244237_d(false)（浅水版本）
    auto info = world::spawn::MobSpawnInfo::createLukewarmOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物 + drowned
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);

    // 水生生物：squid(10,1,2) + dolphin(2,1,2) + nautilus(5,1,1)（1.21.11 原版归 WaterCreature）
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u);
    bool hasSquid = false, hasDolphin = false, hasNautilus = false;
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:squid") {
            hasSquid = true;
            EXPECT_EQ(entry.weight, 10);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 2);
        }
        if (entry.entityTypeId == "minecraft:dolphin") {
            hasDolphin = true;
            EXPECT_EQ(entry.weight, 2);
        }
        if (entry.entityTypeId == "minecraft:nautilus") {
            hasNautilus = true;
            EXPECT_EQ(entry.weight, 5);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    EXPECT_TRUE(hasSquid);
    EXPECT_TRUE(hasDolphin);
    EXPECT_TRUE(hasNautilus);

    // 水生环境生物：cod(15,3,6) + pufferfish(5,1,3) + tropical_fish(25,8,8)
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 3u);
    bool hasCod = false, hasPufferfish = false, hasTropicalFish = false;
    for (const auto& entry : info.getWaterAmbientSpawns()) {
        if (entry.entityTypeId == "minecraft:cod") {
            hasCod = true;
            EXPECT_EQ(entry.weight, 15);
        }
        if (entry.entityTypeId == "minecraft:pufferfish") {
            hasPufferfish = true;
            EXPECT_EQ(entry.weight, 5);
        }
        if (entry.entityTypeId == "minecraft:tropical_fish") {
            hasTropicalFish = true;
            EXPECT_EQ(entry.weight, 25);
        }
    }
    EXPECT_TRUE(hasCod);
    EXPECT_TRUE(hasPufferfish);
    EXPECT_TRUE(hasTropicalFish);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_DeepLukewarmOcean)
{
    // 对应 MC 1.21.11 BiomeMaker.func_244237_d(true)（深水版本）
    auto info = world::spawn::MobSpawnInfo::createDeepLukewarmOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 与浅水版本差异：squid 权重 8（非 10）、minCount 4（非 2）；cod 权重 8（非 15）
    bool hasSquid = false, hasCod = false, hasNautilus = false;
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:squid") {
            hasSquid = true;
            EXPECT_EQ(entry.weight, 8);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 4);
        }
        if (entry.entityTypeId == "minecraft:nautilus") {
            hasNautilus = true;
            EXPECT_EQ(entry.weight, 5);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    for (const auto& entry : info.getWaterAmbientSpawns()) {
        if (entry.entityTypeId == "minecraft:cod") {
            hasCod = true;
            EXPECT_EQ(entry.weight, 8);
            EXPECT_EQ(entry.minCount, 3);
            EXPECT_EQ(entry.maxCount, 6);
        }
    }
    EXPECT_TRUE(hasSquid);
    EXPECT_TRUE(hasCod);
    EXPECT_TRUE(hasNautilus);

    // 其余条目与浅水版本相同
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);       // 8 标准怪物 + drowned
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u); // squid + dolphin + nautilus
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 3u);  // cod + pufferfish + tropical_fish
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_ColdOcean)
{
    // 对应 MC 1.21.11 BiomeMaker.func_244230_b(false)（浅水版本）
    auto info = world::spawn::MobSpawnInfo::createColdOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物 + drowned
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);

    // 水生生物：squid(3,1,4) + nautilus(2,1,1)（1.21.11 无 dolphin）
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 2u);
    bool hasSquid = false, hasNautilus = false;
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:squid") {
            hasSquid = true;
            EXPECT_EQ(entry.weight, 3);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 4);
        }
        if (entry.entityTypeId == "minecraft:nautilus") {
            hasNautilus = true;
            EXPECT_EQ(entry.weight, 2);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    EXPECT_TRUE(hasSquid);
    EXPECT_TRUE(hasNautilus);

    // 水生环境生物：cod(15,3,6) + salmon(15,1,5)
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 2u);
    bool hasCod = false, hasSalmon = false;
    for (const auto& entry : info.getWaterAmbientSpawns()) {
        if (entry.entityTypeId == "minecraft:cod") {
            hasCod = true;
            EXPECT_EQ(entry.weight, 15);
        }
        if (entry.entityTypeId == "minecraft:salmon") {
            hasSalmon = true;
            EXPECT_EQ(entry.weight, 15);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 5);
        }
    }
    EXPECT_TRUE(hasCod);
    EXPECT_TRUE(hasSalmon);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_FrozenOcean)
{
    // 对应 MC 1.21.11 BiomeMaker.func_244239_e(false)（浅水版本）
    auto info = world::spawn::MobSpawnInfo::createFrozenOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物 + drowned
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);

    // 水生生物：squid + nautilus（1.21.11 无 dolphin）
    ASSERT_EQ(info.getWaterCreatureSpawns().size(), 2u);
    bool hasSquid = false, hasNautilus = false;
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:squid") {
            hasSquid = true;
            EXPECT_EQ(entry.weight, 1);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 4);
        }
        if (entry.entityTypeId == "minecraft:nautilus") {
            hasNautilus = true;
            EXPECT_EQ(entry.weight, 2);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    EXPECT_TRUE(hasSquid);
    EXPECT_TRUE(hasNautilus);

    // 水生环境生物：仅 salmon(15,1,5)（无 cod、无 tropical_fish、无 pufferfish）
    ASSERT_EQ(info.getWaterAmbientSpawns().size(), 1u);
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].entityTypeId, "minecraft:salmon");

    // 动物：polar_bear(1,1,2)
    ASSERT_EQ(info.getCreatureSpawns().size(), 1u);
    EXPECT_EQ(info.getCreatureSpawns()[0].entityTypeId, "minecraft:polar_bear");
    EXPECT_EQ(info.getCreatureSpawns()[0].weight, 1);
    EXPECT_EQ(info.getCreatureSpawns()[0].minCount, 1);
    EXPECT_EQ(info.getCreatureSpawns()[0].maxCount, 2);

    // 确认无 stray（1.16.5 FrozenOcean 不调用 snowySpawns）
    for (const auto& entry : info.getMonsterSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:stray") << "FrozenOcean 不应含 stray";
    }
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_DeepOcean)
{
    // 对应 MC 1.21.11 BiomeMaker.func_244234_c(true)（深水版本）
    //   spawn list 与普通 Ocean 完全一致
    auto info = world::spawn::MobSpawnInfo::createDeepOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物 + drowned
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);

    // 水生生物：squid + dolphin + nautilus（与 Ocean 一致）
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u);
    bool hasNautilus = false;
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:nautilus") {
            hasNautilus = true;
            EXPECT_EQ(entry.weight, 5);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    EXPECT_TRUE(hasNautilus);

    // 水生环境生物：cod（与 Ocean 一致）
    ASSERT_EQ(info.getWaterAmbientSpawns().size(), 1u);
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].entityTypeId, "minecraft:cod");
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].weight, 10);

    // 确认无 guardian（guardian 只在 OceanMonument 周围生成，由结构生成器处理）
    for (const auto& entry : info.getMonsterSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:guardian") << "DeepOcean 不应含 guardian";
    }
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_WarmOcean)
{
    // 对应 MC 1.21.11 BiomeMaker.func_244249_o()（warmOcean 浅水版本）
    auto info = world::spawn::MobSpawnInfo::createWarmOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物列表（commonSpawns → func_243735_b(_, 95, 5, 100)）
    // 浅水 warmOcean 不含 drowned（drowned 是 deepWarmOcean 才有）
    const std::array<std::pair<std::string, std::array<i32, 3>>, 8> expectedMonsters = {{
        {"minecraft:spider", {100, 4, 4}},
        {"minecraft:zombie", {95, 4, 4}},
        {"minecraft:zombie_villager", {5, 1, 1}},
        {"minecraft:skeleton", {100, 4, 4}},
        {"minecraft:creeper", {100, 4, 4}},
        {"minecraft:slime", {100, 4, 4}},
        {"minecraft:enderman", {10, 1, 4}},
        {"minecraft:witch", {5, 1, 1}},
    }};
    EXPECT_EQ(info.getMonsterSpawns().size(), expectedMonsters.size());
    for (const auto& [entityId, params] : expectedMonsters) {
        bool found = false;
        for (const auto& entry : info.getMonsterSpawns()) {
            if (entry.entityTypeId == entityId && entry.weight == params[0] && entry.minCount == params[1] &&
                entry.maxCount == params[2]) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Missing or wrong monster entry: " << entityId;
    }
    // 确认浅水版本不含 drowned
    for (const auto& entry : info.getMonsterSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:drowned") << "浅水 warmOcean 不应含 drowned";
    }

    // 环境生物：bat（commonSpawns → caveSpawns）
    ASSERT_EQ(info.getAmbientSpawns().size(), 1u);
    EXPECT_EQ(info.getAmbientSpawns()[0].entityTypeId, "minecraft:bat");
    EXPECT_EQ(info.getAmbientSpawns()[0].weight, 10);
    EXPECT_EQ(info.getAmbientSpawns()[0].minCount, 8);
    EXPECT_EQ(info.getAmbientSpawns()[0].maxCount, 8);

    // 水生生物：squid + dolphin + nautilus（1.21.11 原版归 WaterCreature）
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 3u);
    bool hasSquid = false, hasDolphin = false, hasNautilus = false;
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:squid") {
            hasSquid = true;
            EXPECT_EQ(entry.weight, 10);
            EXPECT_EQ(entry.minCount, 4);
            EXPECT_EQ(entry.maxCount, 4);
        }
        if (entry.entityTypeId == "minecraft:dolphin") {
            hasDolphin = true;
            EXPECT_EQ(entry.weight, 2);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 2);
        }
        if (entry.entityTypeId == "minecraft:nautilus") {
            hasNautilus = true;
            EXPECT_EQ(entry.weight, 5);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    EXPECT_TRUE(hasSquid);
    EXPECT_TRUE(hasDolphin);
    EXPECT_TRUE(hasNautilus);

    // 水生环境生物：pufferfish + tropical_fish（原版归 WaterAmbient）
    EXPECT_EQ(info.getWaterAmbientSpawns().size(), 2u);
    bool hasPufferfish = false, hasTropicalFish = false;
    for (const auto& entry : info.getWaterAmbientSpawns()) {
        if (entry.entityTypeId == "minecraft:pufferfish") {
            hasPufferfish = true;
            EXPECT_EQ(entry.weight, 15);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 3);
        }
        if (entry.entityTypeId == "minecraft:tropical_fish") {
            hasTropicalFish = true;
            EXPECT_EQ(entry.weight, 25);
            EXPECT_EQ(entry.minCount, 8);
            EXPECT_EQ(entry.maxCount, 8);
        }
    }
    EXPECT_TRUE(hasPufferfish);
    EXPECT_TRUE(hasTropicalFish);

    // 确认不含 cod（暖水太暖）和 salmon
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:cod") << "warmOcean 不应含 cod";
        EXPECT_NE(entry.entityTypeId, "minecraft:salmon") << "warmOcean 不应含 salmon";
    }
    for (const auto& entry : info.getWaterAmbientSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:cod") << "warmOcean 不应含 cod";
        EXPECT_NE(entry.entityTypeId, "minecraft:salmon") << "warmOcean 不应含 salmon";
    }
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_DeepWarmOcean)
{
    // 对应 MC 1.16.5 BiomeMaker.func_244250_p()（deepWarmOcean 深水版本）
    auto info = world::spawn::MobSpawnInfo::createDeepWarmOcean();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物列表 + drowned（深水版本额外添加）
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);
    bool hasDrowned = false;
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:drowned") {
            hasDrowned = true;
            EXPECT_EQ(entry.weight, 5);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
    }
    EXPECT_TRUE(hasDrowned) << "deepWarmOcean 应含 drowned";

    // 环境生物：bat
    ASSERT_EQ(info.getAmbientSpawns().size(), 1u);
    EXPECT_EQ(info.getAmbientSpawns()[0].entityTypeId, "minecraft:bat");

    // 水生生物：squid + dolphin（深水 squid weight=5, minCount=1）
    EXPECT_EQ(info.getWaterCreatureSpawns().size(), 2u);
    bool hasSquid = false, hasDolphin = false;
    for (const auto& entry : info.getWaterCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:squid") {
            hasSquid = true;
            EXPECT_EQ(entry.weight, 5);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 4);
        }
        if (entry.entityTypeId == "minecraft:dolphin") {
            hasDolphin = true;
            EXPECT_EQ(entry.weight, 2);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 2);
        }
    }
    EXPECT_TRUE(hasSquid);
    EXPECT_TRUE(hasDolphin);

    // 水生环境生物：仅 tropical_fish（深水版本无 pufferfish）
    ASSERT_EQ(info.getWaterAmbientSpawns().size(), 1u);
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].entityTypeId, "minecraft:tropical_fish");
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].weight, 25);
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].minCount, 8);
    EXPECT_EQ(info.getWaterAmbientSpawns()[0].maxCount, 8);

    // 确认深水版本不含 pufferfish
    for (const auto& entry : info.getWaterAmbientSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:pufferfish") << "deepWarmOcean 不应含 pufferfish";
    }
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Desert)
{
    // 对应 MC 1.16.5 BiomeMaker.func_244220_a(...) → DefaultBiomeFeatures.func_243743_f()（desertSpawns）
    auto info = world::spawn::MobSpawnInfo::createDesert();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物（zombie weight=19, zombie_villager weight=1, skeleton weight=100）+ husk(80,4,4)
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);
    bool hasHusk = false;
    int zombieWeight = 0, skeletonWeight = 0;
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:husk") {
            hasHusk = true;
            EXPECT_EQ(entry.weight, 80);
            EXPECT_EQ(entry.minCount, 4);
            EXPECT_EQ(entry.maxCount, 4);
        }
        if (entry.entityTypeId == "minecraft:zombie") zombieWeight = entry.weight;
        if (entry.entityTypeId == "minecraft:skeleton") skeletonWeight = entry.weight;
    }
    EXPECT_TRUE(hasHusk);
    EXPECT_EQ(zombieWeight, 19);    // desertSpawns 中 zombie 权重 19（非标准 95）
    EXPECT_EQ(skeletonWeight, 100); // desertSpawns 中 skeleton 权重 100（与标准一致）

    // 动物：仅 rabbit(4,2,3)（desertSpawns 中 rabbit 权重 4，非 12）
    ASSERT_EQ(info.getCreatureSpawns().size(), 1u);
    EXPECT_EQ(info.getCreatureSpawns()[0].entityTypeId, "minecraft:rabbit");
    EXPECT_EQ(info.getCreatureSpawns()[0].weight, 4);
    EXPECT_EQ(info.getCreatureSpawns()[0].minCount, 2);
    EXPECT_EQ(info.getCreatureSpawns()[0].maxCount, 3);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Snowy)
{
    // 对应 MC 1.16.5 DefaultBiomeFeatures.func_243741_e()（snowySpawns）
    auto info = world::spawn::MobSpawnInfo::createSnowy();
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.07f); // 雪原更稀疏

    // 怪物：8 条标准陆地怪物（zombie weight=95, skeleton weight=20）+ stray(80,4,4)
    EXPECT_EQ(info.getMonsterSpawns().size(), 9u);
    bool hasStray = false;
    int zombieWeight = 0, skeletonWeight = 0;
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:stray") {
            hasStray = true;
            EXPECT_EQ(entry.weight, 80);
        }
        if (entry.entityTypeId == "minecraft:zombie") zombieWeight = entry.weight;
        if (entry.entityTypeId == "minecraft:skeleton") skeletonWeight = entry.weight;
    }
    EXPECT_TRUE(hasStray);
    EXPECT_EQ(zombieWeight, 95);   // snowySpawns 中 zombie 权重 95（与标准一致）
    EXPECT_EQ(skeletonWeight, 20); // snowySpawns 中 skeleton 权重 20（非标准 100）

    // 动物：rabbit(10,2,3) + polar_bear(1,1,2)
    EXPECT_EQ(info.getCreatureSpawns().size(), 2u);
    bool hasRabbit = false, hasPolarBear = false;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:rabbit") {
            hasRabbit = true;
            EXPECT_EQ(entry.weight, 10);
        }
        if (entry.entityTypeId == "minecraft:polar_bear") {
            hasPolarBear = true;
            EXPECT_EQ(entry.weight, 1);
        }
    }
    EXPECT_TRUE(hasRabbit);
    EXPECT_TRUE(hasPolarBear);

    // 确认无 zombie_horse（1.16.5 中无任何生物群系 spawn list 含 zombie_horse）
    for (const auto& entry : info.getMonsterSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:zombie_horse") << "Snowy 不应含 zombie_horse";
    }
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_SnowyBeach)
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(26, SNOWY_BEACH,
    //   BiomeMaker.func_244208_a(..., true, false, false))
    //   → commonSpawns（无 TURTLE、无 creature 分类）
    auto info = world::spawn::MobSpawnInfo::createSnowyBeach();
    // creatureSpawnProbability 使用默认 0.1f（commonSpawns 不修改概率）
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物（skeleton weight=100），无 stray
    EXPECT_EQ(info.getMonsterSpawns().size(), 8u);
    bool hasStray = false;
    int zombieWeight = 0, skeletonWeight = 0;
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:stray") {
            hasStray = true;
        }
        if (entry.entityTypeId == "minecraft:zombie") zombieWeight = entry.weight;
        if (entry.entityTypeId == "minecraft:skeleton") skeletonWeight = entry.weight;
    }
    EXPECT_FALSE(hasStray);         // SnowyBeach 无 stray（与 SnowyTundra 差异）
    EXPECT_EQ(zombieWeight, 95);    // commonSpawns 中 zombie 权重 95（与标准一致）
    EXPECT_EQ(skeletonWeight, 100); // commonSpawns 中 skeleton 权重 100（与标准一致，非 SnowyTundra 的 20）

    // 动物：无（SnowyBeach 无 creature 分类，与 Beach 一致但 Beach 有 turtle，SnowyBeach 因雪地无 turtle）
    EXPECT_EQ(info.getCreatureSpawns().size(), 0u);

    // 环境生物：bat(10,8,8)
    ASSERT_EQ(info.getAmbientSpawns().size(), 1u);
    EXPECT_EQ(info.getAmbientSpawns()[0].entityTypeId, "minecraft:bat");
    EXPECT_EQ(info.getAmbientSpawns()[0].weight, 10);
    EXPECT_EQ(info.getAmbientSpawns()[0].minCount, 8);
    EXPECT_EQ(info.getAmbientSpawns()[0].maxCount, 8);

    // 确认无 turtle（雪地沙滩不加 turtle）
    for (const auto& entry : info.getCreatureSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:turtle") << "SnowyBeach 不应含 turtle";
    }
    // 确认无 zombie_horse
    for (const auto& entry : info.getMonsterSpawns()) {
        EXPECT_NE(entry.entityTypeId, "minecraft:zombie_horse") << "SnowyBeach 不应含 zombie_horse";
    }
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Savanna)
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(35, SAVANNA, BiomeMaker.func_244211_a(..., false, false))
    auto info = world::spawn::MobSpawnInfo::createSavanna();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // 怪物：8 条标准陆地怪物
    EXPECT_EQ(info.getMonsterSpawns().size(), 8u);

    // 动物：farmAnimals + horse(1,2,6) + donkey(1,1,1)（无 llama、无 wolf、无 armadillo）
    bool hasHorse = false, hasDonkey = false, hasLlama = false, hasWolf = false;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:horse") {
            hasHorse = true;
            EXPECT_EQ(entry.weight, 1);
            EXPECT_EQ(entry.minCount, 2);
            EXPECT_EQ(entry.maxCount, 6);
        }
        if (entry.entityTypeId == "minecraft:donkey") {
            hasDonkey = true;
            EXPECT_EQ(entry.weight, 1);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 1);
        }
        if (entry.entityTypeId == "minecraft:llama") hasLlama = true;
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
    }
    EXPECT_TRUE(hasHorse);
    EXPECT_TRUE(hasDonkey);
    EXPECT_FALSE(hasLlama); // 普通 Savanna 不含 llama
    EXPECT_FALSE(hasWolf);  // 1.16.5 Savanna 不含 wolf
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_SavannaPlateau)
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(36, SAVANNA_PLATEAU, BiomeMaker.func_244247_m())
    //   func_244247_m 在 func_244258_x 基础上额外添加 llama(8,4,4)（无 wolf）
    auto info = world::spawn::MobSpawnInfo::createSavannaPlateau();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());

    // 怪物：8 条标准陆地怪物
    EXPECT_EQ(info.getMonsterSpawns().size(), 8u);

    // 动物：farmAnimals + horse + donkey + llama(8,4,4)
    bool hasLlama = false, hasWolf = false;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:llama") {
            hasLlama = true;
            EXPECT_EQ(entry.weight, 8);
            EXPECT_EQ(entry.minCount, 4);
            EXPECT_EQ(entry.maxCount, 4);
        }
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
    }
    EXPECT_TRUE(hasLlama); // SavannaPlateau 含 llama
    EXPECT_FALSE(hasWolf); // 1.16.5 SavannaPlateau 不含 wolf
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_ShatteredSavanna)
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(163, SHATTERED_SAVANNA, BiomeMaker.func_244211_a(..., true, true))
    //   func_244211_a 内部仅调用 func_244258_x，spawn list 与普通 Savanna 相同
    auto info = world::spawn::MobSpawnInfo::createShatteredSavanna();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());

    // 怪物：8 条标准陆地怪物
    EXPECT_EQ(info.getMonsterSpawns().size(), 8u);

    // 动物：farmAnimals + horse + donkey（无 llama、无 wolf）
    bool hasLlama = false, hasWolf = false;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:llama") hasLlama = true;
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
    }
    EXPECT_FALSE(hasLlama); // ShatteredSavanna 不含 llama
    EXPECT_FALSE(hasWolf);  // ShatteredSavanna 不含 wolf
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_Jungle)
{
    // 对应 MC 1.16.5 OverworldBiomes.jungle()
    auto info = world::spawn::MobSpawnInfo::createJungle();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());
    EXPECT_FLOAT_EQ(info.getCreatureSpawnProbability(), 0.1f);

    // baseJungleSpawns 在标准 farmAnimals（含 1 条 chicken）之外额外再加一条
    // weight=10 的 chicken，故 Jungle 应有两条 chicken 条目
    int chickenCount = 0;
    bool hasCow = false, hasParrot = false, hasPanda = false, hasOcelot = false;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:chicken") ++chickenCount;
        if (entry.entityTypeId == "minecraft:cow") hasCow = true;
        if (entry.entityTypeId == "minecraft:parrot") hasParrot = true;
        if (entry.entityTypeId == "minecraft:panda") hasPanda = true;
    }
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:ocelot") hasOcelot = true;
    }
    EXPECT_EQ(chickenCount, 2); // farmAnimals 的 chicken + baseJungleSpawns 额外 chicken
    EXPECT_TRUE(hasCow);        // farmAnimals 含 cow（旧实现误删，已收敛）
    EXPECT_TRUE(hasParrot);
    EXPECT_TRUE(hasPanda);
    EXPECT_TRUE(hasOcelot); // MC 1.16.5 jungle() 将 ocelot 加入 MONSTER 分类

    // 验证 ocelot pack size 为 (1,3)，parrot 为 (1,2)，panda 为 (1,2)
    bool ocelotPackOk = false, parrotPackOk = false, pandaPackOk = false;
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:ocelot" && entry.minCount == 1 && entry.maxCount == 3) {
            ocelotPackOk = true;
        }
    }
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:parrot" && entry.minCount == 1 && entry.maxCount == 2) {
            parrotPackOk = true;
        }
        if (entry.entityTypeId == "minecraft:panda" && entry.minCount == 1 && entry.maxCount == 2 &&
            entry.weight == 1) {
            pandaPackOk = true;
        }
    }
    EXPECT_TRUE(ocelotPackOk);
    EXPECT_TRUE(parrotPackOk);
    EXPECT_TRUE(pandaPackOk);
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_SparseJungle)
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(23, JUNGLE_EDGE, BiomeMaker.func_244227_b())
    //   func_244227_b 内部仅调用 baseJungleSpawns，不额外添加 wolf/parrot/ocelot/panda。
    //   注：1.21.11 sparseJungle() 额外添加了 wolf(8,2,4)，但 1.16.5 中无 wolf。本项目对齐 1.16.5。
    auto info = world::spawn::MobSpawnInfo::createSparseJungle();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());

    // sparseJungle = baseJungleSpawns（farmAnimals + 额外 chicken + commonSpawns）
    int chickenCount = 0;
    bool hasWolf = false, hasOcelot = false, hasParrot = false, hasPanda = false;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:chicken") ++chickenCount;
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
        if (entry.entityTypeId == "minecraft:parrot") hasParrot = true;
        if (entry.entityTypeId == "minecraft:panda") hasPanda = true;
    }
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:ocelot") hasOcelot = true;
    }
    EXPECT_EQ(chickenCount, 2); // baseJungleSpawns 额外鸡规则
    EXPECT_FALSE(hasWolf);      // 1.16.5 JungleEdge 不含 wolf（1.21.11 才加）
    EXPECT_FALSE(hasOcelot);    // sparseJungle 没有 ocelot
    EXPECT_FALSE(hasParrot);    // sparseJungle 没有 parrot
    EXPECT_FALSE(hasPanda);     // sparseJungle 没有 panda
}

TEST_F(NaturalSpawnerTest, MobSpawnInfo_BambooJungle)
{
    // 对应 MC 1.16.5 OverworldBiomes.bambooJungle()
    auto info = world::spawn::MobSpawnInfo::createBambooJungle();
    EXPECT_TRUE(info.isPlayerSpawnFriendly());

    // bambooJungle = baseJungleSpawns + parrot(40,1,2) + panda(80,1,2) + ocelot(2,1,1) MONSTER
    int chickenCount = 0;
    bool hasParrot = false, hasPanda = false, hasOcelot = false;
    int pandaWeight = 0;
    for (const auto& entry : info.getCreatureSpawns()) {
        if (entry.entityTypeId == "minecraft:chicken") ++chickenCount;
        if (entry.entityTypeId == "minecraft:parrot") hasParrot = true;
        if (entry.entityTypeId == "minecraft:panda") {
            hasPanda = true;
            pandaWeight = entry.weight;
        }
    }
    int ocelotMin = 0, ocelotMax = 0;
    for (const auto& entry : info.getMonsterSpawns()) {
        if (entry.entityTypeId == "minecraft:ocelot") {
            hasOcelot = true;
            ocelotMin = entry.minCount;
            ocelotMax = entry.maxCount;
        }
    }
    EXPECT_EQ(chickenCount, 2); // baseJungleSpawns 额外鸡规则
    EXPECT_TRUE(hasParrot);
    EXPECT_TRUE(hasPanda);
    EXPECT_EQ(pandaWeight, 80); // bambooJungle panda weight=80（与普通 jungle 的 1 不同）
    EXPECT_TRUE(hasOcelot);
    EXPECT_EQ(ocelotMin, 1); // bambooJungle ocelot pack=1（与普通 jungle 的 3 不同）
    EXPECT_EQ(ocelotMax, 1);
}

} // namespace test
} // namespace mc
