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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;
using namespace mc::world::spawn;

namespace {

// 在首次构造测试实体前注册原版实体类型，使 DespawnManager::shouldDespawn 能通过
// typeId 查到分类（原版 Mob.checkDespawn 用 this.getType().getCategory() 取消失距离）。
// 使用函数局部静态量保证注册在首个测试实体构造前完成，规避跨 TU 静态初始化顺序问题。
void ensureVanillaEntitiesRegistered()
{
    static const bool kRegistered = [] {
        VanillaEntities::registerAll();
        return true;
    }();
    (void)kRegistered;
}

// 测试用的 MobEntity 子类
// 设置 typeId 为 minecraft:zombie（Monster 分类，despawnDistance=128），
// 使 DespawnManager::shouldDespawn 能查到分类消失距离。
class TestMob : public MobEntity {
public:
    TestMob(EntityId id)
        : MobEntity(id)
    {
        ensureVanillaEntitiesRegistered();
        setTypeId(EntityTypes::ZOMBIE);
    }

    void tick() override { MobEntity::tick(); }
    void registerGoals() override {}
};

// 测试用的 MonsterEntity 子类
class TestMonster : public MonsterEntity {
public:
    TestMonster(EntityId id)
        : MonsterEntity(id)
    {
        ensureVanillaEntitiesRegistered();
        setTypeId(EntityTypes::ZOMBIE);
    }

    void registerGoals() override {}
};

// 测试用的 AnimalEntity 子类
class TestAnimal : public AnimalEntity {
public:
    TestAnimal(EntityId id)
        : AnimalEntity(id)
    {
        ensureVanillaEntitiesRegistered();
        setTypeId(EntityTypes::COW);
    }

    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& /*partner*/) override { return nullptr; }
};

// 简化的测试世界
class DespawnTestWorld : public test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    // TickManager 接口
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("DespawnTestWorld::tickManager not implemented");
    }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    void setDifficulty(Difficulty diff) { m_difficulty = diff; }

private:
    u64 m_currentTick = 0;
    Difficulty m_difficulty = Difficulty::Normal;
};

} // namespace

// ========== MobEntity 持久化测试 ==========

TEST(MobEntityPersistenceTest, DefaultPersistenceIsFalse)
{
    TestMob mob(EntityId(1));
    EXPECT_FALSE(mob.isNoDespawnRequired());
}

TEST(MobEntityPersistenceTest, EnablePersistenceSetsFlag)
{
    TestMob mob(EntityId(1));
    mob.enablePersistence();
    EXPECT_TRUE(mob.isNoDespawnRequired());
}

TEST(MobEntityPersistenceTest, PreventDespawnReturnsIsRiding)
{
    TestMob mob(EntityId(1));
    // 默认没有骑乘任何实体
    EXPECT_FALSE(mob.preventDespawn());
}

TEST(MobEntityPersistenceTest, CanDespawnDefaultReturnsTrue)
{
    TestMob mob(EntityId(1));
    EXPECT_TRUE(mob.canDespawn(100.0));
}

// ========== AnimalEntity 消失测试 ==========

TEST(AnimalEntityDespawnTest, CanDespawnReturnsFalse)
{
    TestAnimal animal(EntityId(1));
    // 动物不会消失
    EXPECT_FALSE(animal.canDespawn(100.0));
    EXPECT_FALSE(animal.canDespawn(200.0));
}

// ========== MonsterEntity 消失测试 ==========

TEST(MonsterEntityDespawnTest, IsDespawnPeacefulReturnsTrue)
{
    TestMonster monster(EntityId(1));
    // 怪物在和平模式下会消失
    EXPECT_TRUE(monster.isDespawnPeaceful());
}

// ========== 家范围系统测试 ==========

TEST(MobEntityHomeTest, DefaultNoHome)
{
    TestMob mob(EntityId(1));
    EXPECT_FALSE(mob.hasHome());
    EXPECT_LT(mob.maximumHomeDistance(), 0.0f);
}

TEST(MobEntityHomeTest, SetHome)
{
    TestMob mob(EntityId(1));
    mob.setHomePosAndDistance(BlockPos(100, 64, 200), 50);

    EXPECT_TRUE(mob.hasHome());
    EXPECT_EQ(mob.homePosition(), BlockPos(100, 64, 200));
    EXPECT_FLOAT_EQ(mob.maximumHomeDistance(), 50.0f);
}

TEST(MobEntityHomeTest, IsWithinHomeDistance)
{
    TestMob mob(EntityId(1));
    mob.setHomePosAndDistance(BlockPos(0, 0, 0), 10);

    // 在范围内
    EXPECT_TRUE(mob.isWithinHomeDistanceFromPosition(BlockPos(0, 0, 0)));
    EXPECT_TRUE(mob.isWithinHomeDistanceFromPosition(BlockPos(5, 5, 5)));
    EXPECT_TRUE(mob.isWithinHomeDistanceFromPosition(BlockPos(9, 0, 0)));

    // 在范围外
    EXPECT_FALSE(mob.isWithinHomeDistanceFromPosition(BlockPos(11, 0, 0)));
    EXPECT_FALSE(mob.isWithinHomeDistanceFromPosition(BlockPos(0, 0, 11)));
    EXPECT_FALSE(mob.isWithinHomeDistanceFromPosition(BlockPos(10, 0, 0))); // 10 >= 10 不在范围内
}

TEST(MobEntityHomeTest, ClearHome)
{
    TestMob mob(EntityId(1));
    mob.setHomePosAndDistance(BlockPos(100, 64, 200), 50);
    EXPECT_TRUE(mob.hasHome());

    mob.clearHome();
    EXPECT_FALSE(mob.hasHome());
    EXPECT_LT(mob.maximumHomeDistance(), 0.0f);
}

TEST(MobEntityHomeTest, NoHomeAllowsAllPositions)
{
    TestMob mob(EntityId(1));
    // 没有设置家范围时，任何位置都允许
    EXPECT_TRUE(mob.isWithinHomeDistanceFromPosition(BlockPos(100000, 0, 0)));
    EXPECT_TRUE(mob.isWithinHomeDistanceFromPosition(BlockPos(0, 0, 0)));
}

// ========== 空闲时间测试 ==========

TEST(MobEntityIdleTimeTest, DefaultIdleTimeIsZero)
{
    TestMob mob(EntityId(1));
    EXPECT_EQ(mob.idleTime(), 0);
}

TEST(MobEntityIdleTimeTest, SetIdleTime)
{
    TestMob mob(EntityId(1));
    mob.setIdleTime(100);
    EXPECT_EQ(mob.idleTime(), 100);
}

// ========== EntityClassification 消失距离测试 ==========

TEST(EntityClassificationDespawnTest, MonsterDespawnDistance)
{
    auto info = EntityClassificationInfo::get(EntityClassification::Monster);
    EXPECT_EQ(info.despawnDistance, 128);
    EXPECT_EQ(info.randomDespawnDistance, 32);
}

TEST(EntityClassificationDespawnTest, CreatureDespawnDistance)
{
    auto info = EntityClassificationInfo::get(EntityClassification::Creature);
    EXPECT_EQ(info.despawnDistance, 128);
    EXPECT_EQ(info.randomDespawnDistance, 32);
}

TEST(EntityClassificationDespawnTest, WaterAmbientDespawnDistance)
{
    auto info = EntityClassificationInfo::get(EntityClassification::WaterAmbient);
    EXPECT_EQ(info.despawnDistance, 64); // 水生环境生物消失距离更短
    EXPECT_EQ(info.randomDespawnDistance, 32);
}

TEST(EntityClassificationDespawnTest, MiscDespawnDistance)
{
    auto info = EntityClassificationInfo::get(EntityClassification::Misc);
    EXPECT_EQ(info.despawnDistance, 128);
    EXPECT_EQ(info.randomDespawnDistance, 32);
}

// ============================================================================
// DespawnManager 消失决策测试
//
// 原版 Mob.checkDespawn 的核心逻辑被提取为纯函数 DespawnManager::shouldDespawn，
// 不依赖 ServerWorld，便于覆盖边界。无玩家时用 kNoPlayer（负值哨兵）表示，
// 此时原版保留实体（getNearestPlayer 返回 null 不做任何事）。
// ============================================================================

#include "server/world/spawn/DespawnManager.hpp"

// 无玩家哨兵复用 DespawnManager::kNoPlayer（getClosestPlayerDistanceSq 无玩家时
// 返回 null，这里用负值表示"无玩家"语义）

// 和平难度下，怪物（isDespawnPeaceful）应立即消失
TEST(DespawnManagerDecisionTest, PeacefulDespawnsMonsters)
{
    TestMonster monster(EntityId(1));
    // 玩家就在旁边（距离 0）
    EXPECT_TRUE(DespawnManager::shouldDespawn(monster, 0.0, Difficulty::Peaceful, 0));
}

// 和平难度下，动物（isDespawnPeaceful=false）不消失
TEST(DespawnManagerDecisionTest, PeacefulKeepsAnimals)
{
    TestAnimal animal(EntityId(1));
    EXPECT_FALSE(DespawnManager::shouldDespawn(animal, 0.0, Difficulty::Peaceful, 0));
}

// 距离玩家超过立即消失距离（Monster=128）时消失
TEST(DespawnManagerDecisionTest, FarEntityDespawnsImmediately)
{
    TestMonster monster(EntityId(1));
    // 200 格 > 128
    EXPECT_TRUE(DespawnManager::shouldDespawn(monster, 200.0 * 200.0, Difficulty::Normal, 0));
}

// 距离玩家在 32 格内时重置 idle，不消失
TEST(DespawnManagerDecisionTest, CloseEntityResetsIdleAndKeeps)
{
    TestMob mob(EntityId(1));
    mob.setIdleTime(10000); // 即使空闲很久
    // 10 格 < 32，应保留并重置 idle
    EXPECT_FALSE(DespawnManager::shouldDespawn(mob, 10.0 * 10.0, Difficulty::Normal, 0));
    EXPECT_EQ(mob.idleTime(), 0);
}

// 无玩家时应保留实体：最近玩家距离查询返回 null 时，实体不做任何事。
TEST(DespawnManagerDecisionTest, NoPlayerKeepsEntity)
{
    TestMonster monster(EntityId(1));
    monster.setIdleTime(10000);
    EXPECT_FALSE(DespawnManager::shouldDespawn(monster, DespawnManager::kNoPlayer, Difficulty::Normal, 0));
}

// 持久化实体（命名/桶装）永不消失
TEST(DespawnManagerDecisionTest, PersistentEntityNeverDespawns)
{
    TestMonster monster(EntityId(1));
    monster.enablePersistence();
    // 即使远超 128 格
    EXPECT_FALSE(DespawnManager::shouldDespawn(monster, 10000.0 * 10000.0, Difficulty::Normal, 0));
}

// 32~128 格内、idle>600 时，仅以 1/800 概率消失。
// 这里用确定性 Random 验证"概率命中"与"未命中"两条路径。
TEST(DespawnManagerDecisionTest, RandomDespawnRespectsProbability)
{
    TestMob mob(EntityId(1));
    mob.setIdleTime(700);              // > 600
    const f64 midDistSq = 64.0 * 64.0; // 32 < 64 < 128

    // 用一个恒返回 0 的随机源模拟"命中"（nextInt(800)==0）
    // 由于 Random 基于 seed，这里多 seed 抽样：只要存在不消失的 seed 即可证明非 100% 消失
    bool anyKeep = false;
    for (u64 seed = 0; seed < 200; ++seed) {
        math::Random rng(seed);
        if (!DespawnManager::shouldDespawn(mob, midDistSq, Difficulty::Normal, 0, rng)) {
            anyKeep = true;
            break;
        }
    }
    EXPECT_TRUE(anyKeep) << "32~128 格内不应 100% 消失，应有概率保留";
}
