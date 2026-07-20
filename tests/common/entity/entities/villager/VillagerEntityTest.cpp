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

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/village/Village.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace entity {
namespace test {
namespace {

/**
 * @brief 支持VillageManager的测试用世界实现
 */
class VillagerTestWorld final : public mc::test::BaseTestWorld {
public:
    VillagerTestWorld()
        : m_villageManager(*this)
    {}

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

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
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

    [[nodiscard]] world::village::VillageManager* villageManager() override { return &m_villageManager; }
    [[nodiscard]] const world::village::VillageManager* villageManager() const override { return &m_villageManager; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("VillagerTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("VillagerTestWorld::tickManager not implemented");
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityInstanceId id = entity->id();
        m_spawnedEntities.push_back(std::move(entity));
        return id;
    }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    void addTestEntity(Entity* entity, u64 id) { m_entities[id] = entity; }
    void removeTestEntity(u64 id) { m_entities.erase(id); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<u64, Entity*> m_entities;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    world::village::VillageManager m_villageManager;
};

/**
 * @brief VillagerEntity 测试夹具
 */
class VillagerEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        VanillaEntities::registerAll();
        m_world = std::make_unique<VillagerTestWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityInstanceId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<VillagerTestWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

// ========== releaseAllPois 测试 ==========

TEST_F(VillagerEntityTest, ReleaseAllPois_NoWorld_EarlyReturn)
{
    // 村民没有世界时应安全返回
    VillagerEntity orphanVillager(EntityInstanceId(2));
    // 不调用 setWorld，m_world 为 nullptr
    orphanVillager.releaseAllPois();
    // 不崩溃即可
}

TEST_F(VillagerEntityTest, ReleaseAllPois_ReleasesPOI)
{
    // 注册一个床位 POI 并让村民占用
    auto& poiStorage = m_world->villageManager()->getPOIStorage();
    BlockPos bedPos(5, 64, 5);
    ASSERT_TRUE(poiStorage.registerPOI(bedPos, world::village::poi::PointOfInterestType::BedRed));
    ASSERT_TRUE(poiStorage.acquirePOI(bedPos, 1, 100));
    EXPECT_TRUE(poiStorage.getPOI(bedPos)->isOccupied());

    // 释放所有 POI
    m_villager->releaseAllPois();

    // POI 应该被释放
    const auto* poi = poiStorage.getPOI(bedPos);
    ASSERT_NE(poi, nullptr);
    EXPECT_FALSE(poi->isOccupied());
}

TEST_F(VillagerEntityTest, ReleaseAllPois_NotifiesVillageManager)
{
    // 注册一个床位 POI，这样 onVillagerJoin 才能创建或找到村庄
    auto& poiStorage = m_world->villageManager()->getPOIStorage();
    ASSERT_TRUE(poiStorage.registerPOI(BlockPos(0, 64, 0), world::village::poi::PointOfInterestType::BedRed));

    // 让村民加入村庄
    auto* villageManager = m_world->villageManager();
    villageManager->onVillagerJoin(1, BlockPos(0, 64, 0));

    // 验证村民已关联到村庄
    auto* village = villageManager->getVillageForVillager(1);
    ASSERT_NE(village, nullptr);
    EXPECT_TRUE(village->hasVillager(1));

    // 释放所有 POI
    m_villager->releaseAllPois();

    // 村民应该已从村庄移除
    auto* villageAfter = villageManager->getVillageForVillager(1);
    EXPECT_EQ(villageAfter, nullptr);
}

TEST_F(VillagerEntityTest, ReleaseAllPois_Idempotent_DoubleCallSafe)
{
    // 注册 POI 并让村民占用
    auto& poiStorage = m_world->villageManager()->getPOIStorage();
    BlockPos bedPos(5, 64, 5);
    ASSERT_TRUE(poiStorage.registerPOI(bedPos, world::village::poi::PointOfInterestType::BedRed));
    ASSERT_TRUE(poiStorage.acquirePOI(bedPos, 1, 100));

    // 让村民加入村庄
    auto* villageManager = m_world->villageManager();
    villageManager->onVillagerJoin(1, BlockPos(0, 64, 0));

    // 第一次释放
    m_villager->releaseAllPois();
    EXPECT_FALSE(poiStorage.getPOI(bedPos)->isOccupied());

    // 第二次释放应安全（不会崩溃或导致错误状态）
    m_villager->releaseAllPois();
    EXPECT_FALSE(poiStorage.getPOI(bedPos)->isOccupied());
}

// ========== die() 测试 ==========

TEST_F(VillagerEntityTest, Die_ReleasesPOI)
{
    // 注册 POI 并让村民占用
    auto& poiStorage = m_world->villageManager()->getPOIStorage();
    BlockPos bedPos(5, 64, 5);
    ASSERT_TRUE(poiStorage.registerPOI(bedPos, world::village::poi::PointOfInterestType::BedRed));
    ASSERT_TRUE(poiStorage.acquirePOI(bedPos, 1, 100));
    EXPECT_TRUE(poiStorage.getPOI(bedPos)->isOccupied());

    // 让村民加入村庄
    auto* villageManager = m_world->villageManager();
    villageManager->onVillagerJoin(1, BlockPos(0, 64, 0));

    // 设置村民生命值为0使其死亡
    m_villager->setHealth(0.0f);
    auto damageSource = mc::DamageSources::generic();
    m_villager->die(damageSource);

    // POI 应该被释放
    const auto* poi = poiStorage.getPOI(bedPos);
    ASSERT_NE(poi, nullptr);
    EXPECT_FALSE(poi->isOccupied());

    // 村民应该已从村庄管理器移除
    auto* villageAfter = villageManager->getVillageForVillager(1);
    EXPECT_EQ(villageAfter, nullptr);
}

TEST_F(VillagerEntityTest, Die_ThenRemove_DoesNotDoubleRelease)
{
    // 注册 POI 并让村民占用
    auto& poiStorage = m_world->villageManager()->getPOIStorage();
    BlockPos bedPos(5, 64, 5);
    ASSERT_TRUE(poiStorage.registerPOI(bedPos, world::village::poi::PointOfInterestType::BedRed));
    ASSERT_TRUE(poiStorage.acquirePOI(bedPos, 1, 100));

    // 让村民加入村庄
    auto* villageManager = m_world->villageManager();
    villageManager->onVillagerJoin(1, BlockPos(0, 64, 0));

    // 死亡
    m_villager->setHealth(0.0f);
    auto damageSource = mc::DamageSources::generic();
    m_villager->die(damageSource);

    // POI 已释放
    EXPECT_FALSE(poiStorage.getPOI(bedPos)->isOccupied());

    // 再次调用 remove()（模拟 tickDeath 20 tick 后调用 remove）
    // 由于 releaseAllPois 有幂等守卫，不会重复释放
    m_villager->remove();

    // POI 状态应保持不变，不应崩溃
    const auto* poi = poiStorage.getPOI(bedPos);
    ASSERT_NE(poi, nullptr);
    EXPECT_FALSE(poi->isOccupied());
}

// ========== remove() 测试 ==========

TEST_F(VillagerEntityTest, Remove_ReleasesPOI)
{
    // 注册 POI 并让村民占用
    auto& poiStorage = m_world->villageManager()->getPOIStorage();
    BlockPos bedPos(5, 64, 5);
    ASSERT_TRUE(poiStorage.registerPOI(bedPos, world::village::poi::PointOfInterestType::BedRed));
    ASSERT_TRUE(poiStorage.acquirePOI(bedPos, 1, 100));

    // 让村民加入村庄
    auto* villageManager = m_world->villageManager();
    villageManager->onVillagerJoin(1, BlockPos(0, 64, 0));

    // 移除村民（模拟区块卸载）
    m_villager->remove();

    // POI 应该被释放
    const auto* poi = poiStorage.getPOI(bedPos);
    ASSERT_NE(poi, nullptr);
    EXPECT_FALSE(poi->isOccupied());

    // 村民应该已从村庄管理器移除
    auto* villageAfter = villageManager->getVillageForVillager(1);
    EXPECT_EQ(villageAfter, nullptr);

    // 实体应标记为已移除
    EXPECT_TRUE(m_villager->isRemoved());
}

TEST_F(VillagerEntityTest, Remove_Idempotent_DoubleCallSafe)
{
    // 注册 POI 并让村民占用
    auto& poiStorage = m_world->villageManager()->getPOIStorage();
    BlockPos bedPos(5, 64, 5);
    ASSERT_TRUE(poiStorage.registerPOI(bedPos, world::village::poi::PointOfInterestType::BedRed));
    ASSERT_TRUE(poiStorage.acquirePOI(bedPos, 1, 100));

    // 第一次 remove
    m_villager->remove();
    EXPECT_TRUE(m_villager->isRemoved());

    // 第二次 remove 应安全（releaseAllPois 有幂等守卫）
    m_villager->remove();
    EXPECT_TRUE(m_villager->isRemoved());
}

// ========== 多 POI 释放测试 ==========

TEST_F(VillagerEntityTest, ReleaseAllPois_ReleasesMultiplePOIs)
{
    auto& poiStorage = m_world->villageManager()->getPOIStorage();

    // 注册多个 POI 并让村民占用
    BlockPos bedPos(5, 64, 5);
    BlockPos bedPos2(-5, 64, 5);
    BlockPos workstationPos(10, 64, 10);

    ASSERT_TRUE(poiStorage.registerPOI(bedPos, world::village::poi::PointOfInterestType::BedRed));
    ASSERT_TRUE(poiStorage.registerPOI(bedPos2, world::village::poi::PointOfInterestType::BedBlue));
    ASSERT_TRUE(poiStorage.registerPOI(workstationPos, world::village::poi::PointOfInterestType::Smoker));

    ASSERT_TRUE(poiStorage.acquirePOI(bedPos, 1, 100));
    ASSERT_TRUE(poiStorage.acquirePOI(bedPos2, 1, 100));
    ASSERT_TRUE(poiStorage.acquirePOI(workstationPos, 1, 100));

    // 验证所有 POI 被占用
    EXPECT_TRUE(poiStorage.getPOI(bedPos)->isOccupied());
    EXPECT_TRUE(poiStorage.getPOI(bedPos2)->isOccupied());
    EXPECT_TRUE(poiStorage.getPOI(workstationPos)->isOccupied());

    // 释放所有 POI
    m_villager->releaseAllPois();

    // 所有 POI 应被释放
    EXPECT_FALSE(poiStorage.getPOI(bedPos)->isOccupied());
    EXPECT_FALSE(poiStorage.getPOI(bedPos2)->isOccupied());
    EXPECT_FALSE(poiStorage.getPOI(workstationPos)->isOccupied());
}

} // namespace
} // namespace test
} // namespace entity
} // namespace mc
