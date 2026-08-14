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

// 容量上限（cap）与生成循环的核心对齐测试。
//
// 原版 NaturalSpawner 的全局容量上限公式为：
//   maxInstancesPerChunk * spawnableChunkCount / MAGIC_NUMBER(289)
// 其中 spawnableChunkCount 是"玩家 8 区块刷怪范围内的区块数"（固定刷怪距离，
// 与视距 viewDistance 无关），满载时 ≈ 289。本项目曾错误地用
// (2*viewDistance+1)^2 作为区块计数，且加了 max(...,1) 下限保护，导致上限被
// 放大且永不为 0，是实体无限累积的放大器之一。

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/world/spawn/NaturalSpawner.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;
using namespace mc::world::spawn;

namespace {

constexpr i32 MAGIC_NUMBER = 289; // 17^2，原版刷怪区块计数基准

class NaturalSpawnerCapTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaEntities::registerAll(); }

    // 构造一个使用指定刷怪区块计数的密度管理器
    EntityDensityManager makeManager(
        i32 spawnableChunkCount, std::unordered_map<EntityClassification, i32> counts, MobDensityTracker& tracker)
    {
        return EntityDensityManager(spawnableChunkCount, std::move(counts), tracker);
    }
};

} // namespace

// ========== cap 公式：使用固定刷怪区块计数而非视距 ==========

TEST_F(NaturalSpawnerCapTest, CapFormulaUsesSpawnableChunkCount)
{
    // 满载刷怪范围：spawnableChunkCount = 289
    // WaterAmbient max = 20，cap = 20 * 289 / 289 = 20
    MobDensityTracker tracker;

    // 计数 19 应可生成
    {
        std::unordered_map<EntityClassification, i32> counts;
        counts[EntityClassification::WaterAmbient] = 19;
        auto manager = makeManager(289, counts, tracker);
        EXPECT_TRUE(manager.canSpawn(EntityClassification::WaterAmbient));
    }

    // 计数 20 应不可生成
    {
        std::unordered_map<EntityClassification, i32> counts;
        counts[EntityClassification::WaterAmbient] = 20;
        auto manager = makeManager(289, counts, tracker);
        EXPECT_FALSE(manager.canSpawn(EntityClassification::WaterAmbient));
    }
}

TEST_F(NaturalSpawnerCapTest, CapScalesWithSpawnableChunkCount)
{
    // 半载刷怪范围：spawnableChunkCount = 144
    // Monster max = 70，cap = 70 * 144 / 289 = 34（整数除法）
    MobDensityTracker tracker;

    // 计数 34 应不可生成
    {
        std::unordered_map<EntityClassification, i32> counts;
        counts[EntityClassification::Monster] = 34;
        auto manager = makeManager(144, counts, tracker);
        EXPECT_FALSE(manager.canSpawn(EntityClassification::Monster));
    }

    // 计数 33 应可生成
    {
        std::unordered_map<EntityClassification, i32> counts;
        counts[EntityClassification::Monster] = 33;
        auto manager = makeManager(144, counts, tracker);
        EXPECT_TRUE(manager.canSpawn(EntityClassification::Monster));
    }
}

// ========== cap 不应有 max(...,1) 下限保护 ==========
// 原版 NaturalSpawner.canSpawnForCategoryGlobal 直接 count < i，无下限。
// 当刷怪区块很少导致 cap 计算为 0 时，count>=0 应直接阻止生成。

TEST_F(NaturalSpawnerCapTest, CapHasNoForcedLowerBoundOfOne)
{
    // spawnableChunkCount 极小：Monster cap = 70 * 1 / 289 = 0
    MobDensityTracker tracker;
    std::unordered_map<EntityClassification, i32> counts;
    counts[EntityClassification::Monster] = 0;
    auto manager = makeManager(1, counts, tracker);

    // cap=0 时，即使计数为 0 也不应生成（原版 count < 0 为 false）
    EXPECT_FALSE(manager.canSpawn(EntityClassification::Monster));
}

// ========== 持久化实体不应计入 cap 计数 ==========
// 原版 createState 跳过 isPersistenceRequired || requiresCustomPersistence 的实体。
// 这里通过 EntityManager.countEntitiesByClassification 验证：命名/持久化 mob 不计入。

TEST_F(NaturalSpawnerCapTest, PersistentEntitiesExcludedFromCapCount)
{
    EntityManager manager{mc::test::testEcsRegistry()};
    const EntityType* zombieType = EntityRegistry::instance().getType("minecraft:zombie");
    ASSERT_NE(zombieType, nullptr);

    // 创建 5 个普通僵尸 + 5 个持久化僵尸
    for (i32 i = 0; i < 5; ++i) {
        auto z = zombieType->create(nullptr, mc::test::testEcsRegistry());
        manager.addEntity(std::move(z));
    }
    for (i32 i = 0; i < 5; ++i) {
        auto z = zombieType->create(nullptr, mc::test::testEcsRegistry());
        auto* mob = dynamic_cast<MobEntity*>(z.get());
        ASSERT_NE(mob, nullptr);
        mob->enablePersistence(); // 标记为持久化，不应计入 cap
        manager.addEntity(std::move(z));
    }

    auto counts = manager.countEntitiesByClassification();
    // 只有 5 个非持久化僵尸计入 Monster
    EXPECT_EQ(counts[EntityClassification::Monster], 5);
}

// ========== MobDensityTracker 应每 tick 清理（不跨 tick 无限累积） ==========
// 这一点通过 NaturalSpawner 的 tick 行为间接验证；此处直接验证 tracker 清理语义。

TEST_F(NaturalSpawnerCapTest, MobDensityTrackerClearResetsAccumulatedCharges)
{
    MobDensityTracker tracker;
    tracker.addCharge(Vector3(1.0f, 0.0f, 0.0f), 1.0);
    tracker.addCharge(Vector3(2.0f, 0.0f, 0.0f), 1.0);
    ASSERT_EQ(tracker.size(), 2);

    tracker.clear();
    EXPECT_EQ(tracker.size(), 0);

    // 清理后查询应返回 0 密度
    EXPECT_DOUBLE_EQ(tracker.getTotalCharge(Vector3(0.0f, 0.0f, 0.0f), 1.0), 0.0);
}

// ========== onSpawn 递增计数后 canSpawn 应反映新计数（cap 翻转） ==========

TEST_F(NaturalSpawnerCapTest, OnSpawnFlipsCanSpawnAtCapBoundary)
{
    MobDensityTracker tracker;
    std::unordered_map<EntityClassification, i32> counts;
    // WaterAmbient cap=20（spawnableChunkCount=289），当前 19
    counts[EntityClassification::WaterAmbient] = 19;
    auto manager = makeManager(289, counts, tracker);

    EXPECT_TRUE(manager.canSpawn(EntityClassification::WaterAmbient));

    // 生成一个后计数应到 20
    manager.onSpawn("minecraft:salmon", EntityClassification::WaterAmbient, Vector3(0.0f, 0.0f, 0.0f), SpawnCosts());

    EXPECT_EQ(manager.getCount(EntityClassification::WaterAmbient), 20);
    EXPECT_FALSE(manager.canSpawn(EntityClassification::WaterAmbient));
}

// ========== LocalMobCapCalculator：每玩家每分类本地 cap ==========

TEST_F(NaturalSpawnerCapTest, LocalMobCapBlocksLocalPileup)
{
    // 本地 cap = getMaxInstancesPerChunk(classification)。
    // WaterCreature max=5。在单一玩家附近区块内，生成 5 个后本地 cap 应阻止继续生成。
    LocalMobCapCalculator localCap;

    // 模拟 createState：把已有 mob 加入玩家附近区块
    ChunkPos chunkPos(0, 0);
    // 玩家关联到该区块（addMob 会为该区块附近的所有玩家计数）
    localCap.addPlayerChunk(0, chunkPos); // player 0 near chunk
    for (i32 i = 0; i < 5; ++i) {
        localCap.addMob(chunkPos, EntityClassification::WaterCreature);
    }

    // 第 6 个应被本地 cap 阻止
    EXPECT_FALSE(localCap.canSpawn(EntityClassification::WaterCreature, chunkPos));
}
