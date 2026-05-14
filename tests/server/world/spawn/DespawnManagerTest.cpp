#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

// 测试用的 MobEntity 子类
class TestMob : public MobEntity {
public:
    TestMob(LegacyEntityType type, EntityId id)
        : MobEntity(type, id)
    {}

    void tick() override { MobEntity::tick(); }
    void registerGoals() override {}
};

// 测试用的 MonsterEntity 子类
class TestMonster : public MonsterEntity {
public:
    TestMonster(LegacyEntityType type, EntityId id)
        : MonsterEntity(type, id)
    {}

    void registerGoals() override {}
};

// 测试用的 AnimalEntity 子类
class TestAnimal : public AnimalEntity {
public:
    TestAnimal(LegacyEntityType type, EntityId id)
        : AnimalEntity(type, id)
    {}

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
    TestMob mob(LegacyEntityType::Zombie, 1);
    EXPECT_FALSE(mob.isNoDespawnRequired());
}

TEST(MobEntityPersistenceTest, EnablePersistenceSetsFlag)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
    mob.enablePersistence();
    EXPECT_TRUE(mob.isNoDespawnRequired());
}

TEST(MobEntityPersistenceTest, PreventDespawnReturnsIsRiding)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
    // 默认没有骑乘任何实体
    EXPECT_FALSE(mob.preventDespawn());
}

TEST(MobEntityPersistenceTest, CanDespawnDefaultReturnsTrue)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
    EXPECT_TRUE(mob.canDespawn(100.0));
}

// ========== AnimalEntity 消失测试 ==========

TEST(AnimalEntityDespawnTest, CanDespawnReturnsFalse)
{
    TestAnimal animal(LegacyEntityType::Pig, 1);
    // 动物不会消失
    EXPECT_FALSE(animal.canDespawn(100.0));
    EXPECT_FALSE(animal.canDespawn(200.0));
}

// ========== MonsterEntity 消失测试 ==========

TEST(MonsterEntityDespawnTest, IsDespawnPeacefulReturnsTrue)
{
    TestMonster monster(LegacyEntityType::Zombie, 1);
    // 怪物在和平模式下会消失
    EXPECT_TRUE(monster.isDespawnPeaceful());
}

// ========== 家范围系统测试 ==========

TEST(MobEntityHomeTest, DefaultNoHome)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
    EXPECT_FALSE(mob.hasHome());
    EXPECT_LT(mob.maximumHomeDistance(), 0.0f);
}

TEST(MobEntityHomeTest, SetHome)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
    mob.setHomePosAndDistance(BlockPos(100, 64, 200), 50);

    EXPECT_TRUE(mob.hasHome());
    EXPECT_EQ(mob.homePosition(), BlockPos(100, 64, 200));
    EXPECT_FLOAT_EQ(mob.maximumHomeDistance(), 50.0f);
}

TEST(MobEntityHomeTest, IsWithinHomeDistance)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
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
    TestMob mob(LegacyEntityType::Zombie, 1);
    mob.setHomePosAndDistance(BlockPos(100, 64, 200), 50);
    EXPECT_TRUE(mob.hasHome());

    mob.clearHome();
    EXPECT_FALSE(mob.hasHome());
    EXPECT_LT(mob.maximumHomeDistance(), 0.0f);
}

TEST(MobEntityHomeTest, NoHomeAllowsAllPositions)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
    // 没有设置家范围时，任何位置都允许
    EXPECT_TRUE(mob.isWithinHomeDistanceFromPosition(BlockPos(100000, 0, 0)));
    EXPECT_TRUE(mob.isWithinHomeDistanceFromPosition(BlockPos(0, 0, 0)));
}

// ========== 空闲时间测试 ==========

TEST(MobEntityIdleTimeTest, DefaultIdleTimeIsZero)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
    EXPECT_EQ(mob.idleTime(), 0);
}

TEST(MobEntityIdleTimeTest, SetIdleTime)
{
    TestMob mob(LegacyEntityType::Zombie, 1);
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
