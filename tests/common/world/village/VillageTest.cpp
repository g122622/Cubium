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

#include "../../TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/village/Village.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace world::village::test {
namespace {

/**
 * @brief 测试用 IWorld 实现
 *
 * 为 Village 测试提供最小化的 World 接口实现。
 */
class VillageTestWorld final : public ::mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entities.find(static_cast<u64>(id));
        return it != m_entities.end() ? it->second : nullptr;
    }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entities.find(static_cast<u64>(id));
        return it != m_entities.end() ? it->second : nullptr;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("VillageTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("VillageTestWorld::tickManager not implemented");
    }

    // 实体管理
    void addTestEntity(Entity* entity, u64 id) { m_entities[id] = entity; }

    void removeTestEntity(u64 id) { m_entities.erase(id); }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<u64, Entity*> m_entities;
    u64 m_currentTick = 0;
};

/**
 * @brief 测试用 Entity 实现
 *
 * 提供最小化的实体接口用于测试。
 */
class TestVillagerEntity : public Entity {
public:
    TestVillagerEntity(EntityInstanceId id, IWorld* world, ecs::EntityRegistry& registry)
        : Entity(id, world, registry)
    {
        // 设置实体类型为村民
        setTypeId(entity::EntityTypeKeys::VILLAGER);
    }

    void tick() override { /* 空实现 */ }
};

/**
 * @brief Village::tick() 测试类
 */
class VillageTickTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        m_world.setCurrentTick(0);
    }

    VillageTestWorld m_world;
    poi::PointOfInterestStorage m_poiStorage;
};

// ========== 村民范围检查测试 ==========

TEST_F(VillageTickTest, TickVillagerCheck_VillagerWithinRange_UpdatesLastSeenTime)
{
    // 创建村庄，中心在 (0, 64, 0)
    Village village(BlockPos(0, 64, 0));

    // 创建测试村民并添加到村庄
    auto villager = std::make_unique<TestVillagerEntity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    villager->setPosition(10.0f, 64.0f, 10.0f); // 在村庄范围内
    m_world.addTestEntity(villager.get(), 1);
    village.addVillager(1);

    // 第一次 tick，设置最后出现时间
    m_world.setCurrentTick(100);
    village.tick(m_world, 100, &m_poiStorage);

    // 验证村民仍在村庄中
    EXPECT_TRUE(village.hasVillager(1));
    EXPECT_EQ(village.getPopulation(), 1);

    m_world.removeTestEntity(1);
}

TEST_F(VillageTickTest, TickVillagerCheck_VillagerOutOfRange_RemovedAfterTimeout)
{
    // 创建村庄，中心在 (0, 64, 0)，默认半径 64
    Village village(BlockPos(0, 64, 0));

    // 创建测试村民
    auto villager = std::make_unique<TestVillagerEntity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    villager->setPosition(10.0f, 64.0f, 10.0f); // 初始在范围内
    m_world.addTestEntity(villager.get(), 1);
    village.addVillager(1);

    // 第一次 tick，记录最后出现时间
    m_world.setCurrentTick(100);
    village.tick(m_world, 100, &m_poiStorage);
    EXPECT_TRUE(village.hasVillager(1));

    // 将村民移出村庄范围
    villager->setPosition(200.0f, 64.0f, 200.0f); // 超出村庄半径

    // 等待超时时间的一半，村民应该仍在列表中
    m_world.setCurrentTick(100 + Village::VILLAGER_TIMEOUT / 2);
    village.tick(m_world, 100 + Village::VILLAGER_TIMEOUT / 2, &m_poiStorage);
    EXPECT_TRUE(village.hasVillager(1));

    // 等待超过超时时间
    m_world.setCurrentTick(100 + Village::VILLAGER_TIMEOUT + 100);
    village.tick(m_world, 100 + Village::VILLAGER_TIMEOUT + 100, &m_poiStorage);

    // 村民应该被移除
    EXPECT_FALSE(village.hasVillager(1));
    EXPECT_EQ(village.getPopulation(), 0);

    m_world.removeTestEntity(1);
}

TEST_F(VillageTickTest, TickVillagerCheck_EntityRemoved_RemovedFromVillage)
{
    Village village(BlockPos(0, 64, 0));

    // 创建村民并添加到村庄
    auto villager = std::make_unique<TestVillagerEntity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    villager->setPosition(10.0f, 64.0f, 10.0f);
    m_world.addTestEntity(villager.get(), 1);
    village.addVillager(1);

    // 验证村民存在
    m_world.setCurrentTick(100);
    village.tick(m_world, 100, &m_poiStorage);
    EXPECT_TRUE(village.hasVillager(1));

    // 移除实体（模拟死亡或卸载）
    m_world.removeTestEntity(1);
    villager.reset();

    // Tick 后村民应该从村庄移除
    m_world.setCurrentTick(200);
    village.tick(m_world, 200, &m_poiStorage);
    EXPECT_FALSE(village.hasVillager(1));
    EXPECT_EQ(village.getPopulation(), 0);
}

TEST_F(VillageTickTest, TickVillagerCheck_NonVillagerEntity_RemovedFromVillage)
{
    Village village(BlockPos(0, 64, 0));

    // 创建一个非村民实体（使用 Unknown 类型）
    auto nonVillager = std::make_unique<Entity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    nonVillager->setPosition(10.0f, 64.0f, 10.0f);
    m_world.addTestEntity(nonVillager.get(), 1);

    // 手动添加到村庄（模拟数据错误）
    village.addVillager(1);
    EXPECT_TRUE(village.hasVillager(1));

    // Tick 后应该被移除
    m_world.setCurrentTick(100);
    village.tick(m_world, 100, &m_poiStorage);
    EXPECT_FALSE(village.hasVillager(1));

    m_world.removeTestEntity(1);
}

TEST_F(VillageTickTest, TickVillagerCheck_RemovedEntity_RemovedFromVillage)
{
    // 测试 isRemoved() 为 true 的实体应从村庄中移除
    Village village(BlockPos(0, 64, 0));

    auto villager = std::make_unique<TestVillagerEntity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    villager->setPosition(10.0f, 64.0f, 10.0f); // 在村庄范围内
    m_world.addTestEntity(villager.get(), 1);
    village.addVillager(1);

    // 第一次 tick，确认村民在村庄中
    m_world.setCurrentTick(100);
    village.tick(m_world, 100, &m_poiStorage);
    EXPECT_TRUE(village.hasVillager(1));

    // 标记实体为已移除（模拟死亡动画结束等场景）
    villager->remove(); // 设置 m_removed = true
    EXPECT_TRUE(villager->isRemoved());

    // Tick 后村民应该从村庄移除
    m_world.setCurrentTick(200);
    village.tick(m_world, 200, &m_poiStorage);
    EXPECT_FALSE(village.hasVillager(1));
    EXPECT_EQ(village.getPopulation(), 0);
}

TEST_F(VillageTickTest, TickVillagerCheck_OutOfRangeNewVillager_GetsGracePeriod)
{
    // 测试新加入的村民如果在村庄范围外，会获得超时宽限期而不是立即被移除
    Village village(BlockPos(0, 64, 0));

    auto villager = std::make_unique<TestVillagerEntity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    villager->setPosition(200.0f, 64.0f, 200.0f); // 初始就在村庄范围外
    m_world.addTestEntity(villager.get(), 1);
    village.addVillager(1);

    // 第一次 tick，村民在范围外但没有 lastSeenTime 记录
    // 应给予宽限期，不应该立即被移除
    m_world.setCurrentTick(100);
    village.tick(m_world, 100, &m_poiStorage);
    EXPECT_TRUE(village.hasVillager(1)); // 仍然在村庄中，获得宽限期

    // 等待部分超时时间，村民应该仍在列表中
    m_world.setCurrentTick(100 + Village::VILLAGER_TIMEOUT / 2);
    village.tick(m_world, 100 + Village::VILLAGER_TIMEOUT / 2, &m_poiStorage);
    EXPECT_TRUE(village.hasVillager(1));

    // 超过超时时间后，村民应该被移除
    m_world.setCurrentTick(100 + Village::VILLAGER_TIMEOUT + 100);
    village.tick(m_world, 100 + Village::VILLAGER_TIMEOUT + 100, &m_poiStorage);
    EXPECT_FALSE(village.hasVillager(1));

    m_world.removeTestEntity(1);
}

// ========== POI 释放测试 ==========

TEST_F(VillageTickTest, TickVillagerCheck_VillagerRemoved_ReleasesPOI)
{
    Village village(BlockPos(0, 64, 0));

    // 注册一个床位 POI
    BlockPos bedPos(5, 64, 5);
    m_poiStorage.registerPOI(bedPos, poi::PointOfInterestType::BedRed);

    // 创建村民并占用床位
    auto villager = std::make_unique<TestVillagerEntity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    villager->setPosition(5.0f, 64.0f, 5.0f);
    m_world.addTestEntity(villager.get(), 1);
    village.addVillager(1);

    // 村民占用 POI
    m_poiStorage.acquirePOI(bedPos, 1, 100);
    EXPECT_TRUE(m_poiStorage.getPOI(bedPos)->isOccupied());

    // 验证村民在村庄中
    m_world.setCurrentTick(100);
    village.tick(m_world, 100, &m_poiStorage);
    EXPECT_TRUE(village.hasVillager(1));

    // 将村民移出范围并等待超时
    villager->setPosition(500.0f, 64.0f, 500.0f); // 远离村庄

    // 超时后移除
    m_world.setCurrentTick(100 + Village::VILLAGER_TIMEOUT + 100);
    village.tick(m_world, 100 + Village::VILLAGER_TIMEOUT + 100, &m_poiStorage);

    // 村民应该被移除
    EXPECT_FALSE(village.hasVillager(1));

    // POI 应该被释放
    const auto* poi = m_poiStorage.getPOI(bedPos);
    ASSERT_NE(poi, nullptr);
    EXPECT_FALSE(poi->isOccupied());

    m_world.removeTestEntity(1);
}

TEST_F(VillageTickTest, TickVillagerCheck_MultipleVillagers_OnlyRemovesTimedOut)
{
    Village village(BlockPos(0, 64, 0));

    // 创建多个村民
    auto villager1 = std::make_unique<TestVillagerEntity>(EntityInstanceId(1), &m_world, mc::test::testEcsRegistry());
    villager1->setPosition(10.0f, 64.0f, 10.0f);
    m_world.addTestEntity(villager1.get(), 1);
    village.addVillager(1);

    auto villager2 = std::make_unique<TestVillagerEntity>(EntityInstanceId(2), &m_world, mc::test::testEcsRegistry());
    villager2->setPosition(20.0f, 64.0f, 20.0f);
    m_world.addTestEntity(villager2.get(), 2);
    village.addVillager(2);

    auto villager3 = std::make_unique<TestVillagerEntity>(EntityInstanceId(3), &m_world, mc::test::testEcsRegistry());
    villager3->setPosition(30.0f, 64.0f, 30.0f);
    m_world.addTestEntity(villager3.get(), 3);
    village.addVillager(3);

    // 初始 tick
    m_world.setCurrentTick(100);
    village.tick(m_world, 100, &m_poiStorage);
    EXPECT_EQ(village.getPopulation(), 3);

    // villager1 移出范围，villager2 和 villager3 保持在范围内
    villager1->setPosition(500.0f, 64.0f, 500.0f);
    villager2->setPosition(15.0f, 64.0f, 15.0f);
    villager3->setPosition(25.0f, 64.0f, 25.0f);

    // 超时后只有 villager1 被移除
    m_world.setCurrentTick(100 + Village::VILLAGER_TIMEOUT + 100);
    village.tick(m_world, 100 + Village::VILLAGER_TIMEOUT + 100, &m_poiStorage);

    EXPECT_FALSE(village.hasVillager(1));
    EXPECT_TRUE(village.hasVillager(2));
    EXPECT_TRUE(village.hasVillager(3));
    EXPECT_EQ(village.getPopulation(), 2);

    m_world.removeTestEntity(1);
    m_world.removeTestEntity(2);
    m_world.removeTestEntity(3);
}

// ========== POI 统计更新测试 ==========

TEST_F(VillageTickTest, TickPOIStats_UpdatesBedCount)
{
    Village village(BlockPos(0, 64, 0));

    // 添加多个床位 POI
    m_poiStorage.registerPOI(BlockPos(10, 64, 10), poi::PointOfInterestType::BedRed);
    m_poiStorage.registerPOI(BlockPos(-10, 64, 10), poi::PointOfInterestType::BedBlue);
    m_poiStorage.registerPOI(BlockPos(10, 64, -10), poi::PointOfInterestType::BedGreen);

    // 初始状态床位计数为 0
    EXPECT_EQ(village.getBedCount(), 0);

    // 手动触发 POI 统计更新
    village.recalculateBounds(m_poiStorage);
    EXPECT_EQ(village.getBedCount(), 3);
}

TEST_F(VillageTickTest, TickPOIStats_UpdatesWorkstationCount)
{
    Village village(BlockPos(0, 64, 0));

    // 添加床位（用于村庄边界计算）- 放在中心
    ASSERT_TRUE(m_poiStorage.registerPOI(BlockPos(0, 64, 0), poi::PointOfInterestType::BedRed));

    // 添加工作站 POI - 放在靠近中心的位置
    ASSERT_TRUE(m_poiStorage.registerPOI(BlockPos(5, 64, 5), poi::PointOfInterestType::Smoker));
    ASSERT_TRUE(m_poiStorage.registerPOI(BlockPos(-5, 64, 5), poi::PointOfInterestType::BlastFurnace));
    ASSERT_TRUE(m_poiStorage.registerPOI(BlockPos(5, 64, -5), poi::PointOfInterestType::Lectern));

    // 验证 POI 是否正确注册
    EXPECT_TRUE(m_poiStorage.hasPOI(BlockPos(0, 64, 0)));
    EXPECT_TRUE(m_poiStorage.hasPOI(BlockPos(5, 64, 5)));
    EXPECT_TRUE(m_poiStorage.hasPOI(BlockPos(-5, 64, 5)));
    EXPECT_TRUE(m_poiStorage.hasPOI(BlockPos(5, 64, -5)));

    // 手动触发边界重计算
    village.recalculateBounds(m_poiStorage);
    EXPECT_EQ(village.getRadius(), VillageConfig::BASE_RADIUS + VillageConfig::RADIUS_PER_BED);

    // Tick 触发 POI 统计更新（需要在 POI_STAT_UPDATE_INTERVAL 之后）
    for (i64 t = 0; t <= Village::POI_STAT_UPDATE_INTERVAL; ++t) {
        village.tick(m_world, t, &m_poiStorage);
    }

    EXPECT_EQ(village.getBedCount(), 1);
    EXPECT_EQ(village.getWorkstationCount(), 3);
}

TEST_F(VillageTickTest, TickPOIStats_UpdatesMeetingPoint)
{
    Village village(BlockPos(0, 64, 0));

    // 添加床位
    ASSERT_TRUE(m_poiStorage.registerPOI(BlockPos(0, 64, 0), poi::PointOfInterestType::BedRed));

    // 添加钟（聚集点）
    BlockPos bellPos(5, 65, 5);
    ASSERT_TRUE(m_poiStorage.registerPOI(bellPos, poi::PointOfInterestType::Bell));

    // 手动触发边界重计算
    village.recalculateBounds(m_poiStorage);

    // Tick 触发 POI 统计更新
    for (i64 t = 0; t <= Village::POI_STAT_UPDATE_INTERVAL; ++t) {
        village.tick(m_world, t, &m_poiStorage);
    }

    // 验证聚集点
    EXPECT_TRUE(village.hasMeetingPoint());
    EXPECT_EQ(village.getMeetingPoint(), bellPos);
}

TEST_F(VillageTickTest, TickPOIStats_NoMeetingPointWhenNoBell)
{
    Village village(BlockPos(0, 64, 0));

    // 只添加床位，没有钟
    m_poiStorage.registerPOI(BlockPos(0, 64, 0), poi::PointOfInterestType::BedRed);

    village.recalculateBounds(m_poiStorage);

    // Tick 触发 POI 统计更新
    for (i64 t = 0; t < Village::POI_STAT_UPDATE_INTERVAL + 10; ++t) {
        village.tick(m_world, t, &m_poiStorage);
    }

    // 验证没有聚集点
    EXPECT_FALSE(village.hasMeetingPoint());
}

// ========== 序列化测试 ==========

TEST_F(VillageTickTest, SerializeDeserialize_PreservesVillagers)
{
    Village original(BlockPos(100, 64, 200));

    // 添加村民
    original.addVillager(1);
    original.addVillager(2);
    original.addVillager(3);

    // 设置属性
    original.setBedCount(5);
    original.setWorkstationCount(3);
    original.setUnderRaid(true);
    original.setLastRaidTime(1000);

    // 序列化
    nbt::tags::compound_tag tag;
    original.serialize(tag);

    // 反序列化
    Village deserialized = Village::deserialize(tag);

    // 验证
    EXPECT_EQ(deserialized.getCenter().x, 100);
    EXPECT_EQ(deserialized.getCenter().y, 64);
    EXPECT_EQ(deserialized.getCenter().z, 200);
    EXPECT_TRUE(deserialized.hasVillager(1));
    EXPECT_TRUE(deserialized.hasVillager(2));
    EXPECT_TRUE(deserialized.hasVillager(3));
    EXPECT_EQ(deserialized.getPopulation(), 3);
    EXPECT_EQ(deserialized.getBedCount(), 5);
    EXPECT_EQ(deserialized.getWorkstationCount(), 3);
    EXPECT_TRUE(deserialized.isUnderRaid());
    EXPECT_EQ(deserialized.getLastRaidTime(), 1000);
}

TEST_F(VillageTickTest, SerializeDeserialize_PreservesMeetingPoint)
{
    Village original(BlockPos(0, 64, 0));
    original.setMeetingPoint(BlockPos(50, 70, 50));

    nbt::tags::compound_tag tag;
    original.serialize(tag);

    Village deserialized = Village::deserialize(tag);

    EXPECT_TRUE(deserialized.hasMeetingPoint());
    EXPECT_EQ(deserialized.getMeetingPoint()->x, 50);
    EXPECT_EQ(deserialized.getMeetingPoint()->y, 70);
    EXPECT_EQ(deserialized.getMeetingPoint()->z, 50);
}

// ========== 边界测试 ==========

TEST_F(VillageTickTest, IsWithinVillage_PositionCheck)
{
    Village village(BlockPos(0, 64, 0));

    // 默认半径是 64 格
    // 在半径内的点（距离中心 < 64）
    EXPECT_TRUE(village.isWithinVillage(BlockPos(0, 64, 0)));   // 距离 0
    EXPECT_TRUE(village.isWithinVillage(BlockPos(40, 64, 40))); // 距离 sqrt(3200) ≈ 56.6
    EXPECT_TRUE(village.isWithinVillage(BlockPos(0, 100, 0)));  // 高度也算在内，距离 36

    // 刚好在边界上（距离中心 = 64）
    EXPECT_TRUE(village.isWithinVillage(BlockPos(64, 64, 0))); // 距离 64

    // 超出半径的点（距离中心 > 64）
    EXPECT_FALSE(village.isWithinVillage(BlockPos(65, 64, 0)));  // 距离 65
    EXPECT_FALSE(village.isWithinVillage(BlockPos(50, 64, 50))); // 距离 sqrt(5000) ≈ 70.7
    EXPECT_FALSE(village.isWithinVillage(BlockPos(0, 64, 100))); // 距离 100
}

TEST_F(VillageTickTest, IsWithinRaidTrigger_PositionCheck)
{
    Village village(BlockPos(0, 64, 0));

    // 袭击触发范围是 96 格
    EXPECT_TRUE(village.isWithinRaidTrigger(BlockPos(50, 64, 50)));
    EXPECT_TRUE(village.isWithinRaidTrigger(BlockPos(90, 64, 0)));
    EXPECT_FALSE(village.isWithinRaidTrigger(BlockPos(100, 64, 0))); // 超出触发范围
}

// ========== 繁殖测试 ==========

TEST_F(VillageTickTest, CanBreed_HasAvailableBeds_ReturnsTrue)
{
    Village village(BlockPos(0, 64, 0));

    village.addVillager(1);
    village.addVillager(2);
    village.setBedCount(5); // 5 张床，2 个村民

    EXPECT_TRUE(village.canBreed());
    EXPECT_EQ(village.getAvailableBeds(), 3);
}

TEST_F(VillageTickTest, CanBreed_NoAvailableBeds_ReturnsFalse)
{
    Village village(BlockPos(0, 64, 0));

    village.addVillager(1);
    village.addVillager(2);
    village.setBedCount(2); // 2 张床，2 个村民

    EXPECT_FALSE(village.canBreed());
    EXPECT_EQ(village.getAvailableBeds(), 0);
}

TEST_F(VillageTickTest, CanBreed_NoVillagers_ReturnsFalse)
{
    Village village(BlockPos(0, 64, 0));

    village.setBedCount(5); // 有床但没有村民

    EXPECT_FALSE(village.canBreed());
}

// ========== Village ID 测试 ==========

TEST_F(VillageTickTest, VillageId_InitiallyZero)
{
    Village village(BlockPos(0, 64, 0));
    EXPECT_EQ(village.getId(), 0);
}

TEST_F(VillageTickTest, VillageId_CanBeSetAndRetrieved)
{
    Village village(BlockPos(0, 64, 0));
    village.setId(12345);
    EXPECT_EQ(village.getId(), 12345);
}

TEST_F(VillageTickTest, VillageId_SerializeDeserialize)
{
    Village original(BlockPos(100, 64, 200));
    original.setId(42);

    nbt::tags::compound_tag tag;
    original.serialize(tag);

    Village deserialized = Village::deserialize(tag);
    EXPECT_EQ(deserialized.getId(), 42);
}

TEST_F(VillageTickTest, VillageId_DeserializeBackwardCompatibility_NoIdField)
{
    // 测试没有 ID 字段的旧数据（向后兼容）
    nbt::tags::compound_tag tag;
    tag.put("CenterX", 0);
    tag.put("CenterY", 64);
    tag.put("CenterZ", 0);
    tag.put("Radius", 64.0f);
    tag.put("BedCount", 0);
    tag.put("WorkstationCount", 0);
    tag.put("UnderRaid", static_cast<i8>(0));
    tag.put("LastRaidTime", static_cast<i64>(0));
    tag.put("CreatedTime", static_cast<i64>(0));
    tag.put("LastPOIStatUpdateTime", static_cast<i64>(0));
    // 不设置 Id 字段

    Village deserialized = Village::deserialize(tag);
    EXPECT_EQ(deserialized.getId(), 0); // 默认值
}

TEST_F(VillageTickTest, VillageId_LargeIdValue)
{
    Village village(BlockPos(0, 64, 0));
    village.setId(UINT64_MAX);
    EXPECT_EQ(village.getId(), UINT64_MAX);
}

TEST_F(VillageTickTest, VillageId_MultipleVillagesHaveUniqueIds)
{
    Village village1(BlockPos(0, 64, 0));
    Village village2(BlockPos(100, 64, 100));

    village1.setId(1);
    village2.setId(2);

    EXPECT_NE(village1.getId(), village2.getId());
}

TEST_F(VillageTickTest, VillageId_SerializePreservesAllFields)
{
    Village original(BlockPos(50, 70, 100));
    original.setId(999);
    original.setBedCount(10);
    original.setWorkstationCount(5);
    original.addVillager(123);

    nbt::tags::compound_tag tag;
    original.serialize(tag);

    Village deserialized = Village::deserialize(tag);
    EXPECT_EQ(deserialized.getId(), 999);
    EXPECT_EQ(deserialized.getBedCount(), 10);
    EXPECT_EQ(deserialized.getWorkstationCount(), 5);
    EXPECT_TRUE(deserialized.hasVillager(123));
}

} // namespace
} // namespace world::village::test
} // namespace mc
