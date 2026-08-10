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

/**
 * @file TraderLlamaIntegrationTest.cpp
 * @brief 商队羊驼（TraderLlamaEntity）集成测试
 *
 * 本测试覆盖 TraderLlamaEntity.hpp 中四个原 TODO 标注的私有方法行为：
 * - interactMob()：拴在流浪商人身上时阻止玩家骑乘
 * - maybeDespawn()：消失倒计时递减、与流浪商人同步、归零后 discard
 * - isLeashedToWanderingTrader()：区分拴在流浪商人 vs 其他实体/栅栏
 * - hasExactlyOnePlayerPassenger()：乘客数量与类型判断
 *
 * 这些方法本身为 private，通过公共接口（interactMob / canDespawn / tick）
 * 间接驱动并验证其行为，符合"测试公共契约而非私有实现"的原则。
 *
 * 测试世界 TraderLlamaIntegrationTestWorld 在 BaseTestWorld 基础上覆写：
 * - getEntity(EntityInstanceId)：支持乘客 EntityInstanceId 查找（hasExactlyOnePlayerPassenger 需要）
 * - getEntityByUuid(uuid)：支持拴绳持有者 UUID 查找（isLeashedToWanderingTrader 需要）
 * - spawnEntity(unique_ptr)：提供实体生成注册（部分子接口需要）
 * 并提供 registerEntity 辅助方法以建立索引。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

// ============================================================================
// 测试世界：支持实体查找（UUID + EntityInstanceId）与生成
// ============================================================================

class TraderLlamaIntegrationTestWorld : public mc::test::BaseTestWorld {
public:
    TraderLlamaIntegrationTestWorld()
    {
        Items::initialize();
        VanillaBlocks::initialize();
        // DO_ENTITY_DROPS 默认 true，clearLeash/dropLeash 会查询此规则
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    }

    // ========== EntityInstanceId 查找 ==========

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_idToEntity.find(id);
        return it != m_idToEntity.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_idToEntity.find(id);
        return it != m_idToEntity.end() ? it->second : nullptr;
    }

    // ========== UUID 查找 ==========

    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override
    {
        auto it = m_uuidToEntity.find(uuid);
        return it != m_uuidToEntity.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntityByUuid(const std::string& uuid) const override
    {
        auto it = m_uuidToEntity.find(uuid);
        return it != m_uuidToEntity.end() ? it->second : nullptr;
    }

    // ========== 生成（极简实现：仅登记到索引） ==========

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityInstanceId(0);
        }
        EntityInstanceId assignedId = entity->id();
        if (static_cast<u32>(assignedId) == 0u) {
            assignedId = EntityInstanceId(static_cast<u32>(m_nextAutoId++));
            entity->setId(assignedId);
        }
        Entity* raw = entity.get();
        raw->setWorld(this);
        m_ownedEntities.push_back(std::move(entity));
        registerEntity(*raw);
        return assignedId;
    }

    // ========== 测试辅助 ==========

    void registerEntity(Entity& entity)
    {
        m_idToEntity[entity.id()] = &entity;
        m_uuidToEntity[entity.uuid()] = &entity;
    }

    void unregisterEntity(EntityInstanceId id)
    {
        auto it = m_idToEntity.find(id);
        if (it == m_idToEntity.end()) {
            return;
        }
        Entity* entity = it->second;
        m_idToEntity.erase(it);
        if (entity != nullptr) {
            m_uuidToEntity.erase(entity->uuid());
        }
    }

private:
    std::unordered_map<EntityInstanceId, Entity*> m_idToEntity;
    std::unordered_map<std::string, Entity*> m_uuidToEntity;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
    u32 m_nextAutoId = 100; // 避免与测试中手动分配的 ID 冲突
};

// ============================================================================
// 测试夹具
// ============================================================================

class TraderLlamaIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<TraderLlamaIntegrationTestWorld>(); }

    std::unique_ptr<TraderLlamaIntegrationTestWorld> m_world;
};

// ============================================================================
// isLeashedToWanderingTrader 间接测试（通过 interactMob 返回值）
// ============================================================================
//
// 测试思路：interactMob 在 isLeashedToWanderingTrader() 返回 true 时立即返回 Pass，
// 不会调用 LlamaEntity::interactMob（后者会进入骑乘/装备流程并返回 Success）。
// 因此：
//   - 拴在流浪商人身上 → interactMob 返回 Pass
//   - 拴在其他实体/栅栏/未拴绳 → interactMob 委托给 LlamaEntity（返回非 Pass）
//
// 为避免 interactMob 委托路径中驯服/骑乘状态对断言造成干扰，
// 使用未驯服、未骑乘、空手的玩家，此时 LlamaEntity::interactMob 会走 doPlayerRide
// 返回 Success。

TEST_F(TraderLlamaIntegrationTest, InteractMob_LeashedToWanderingTrader_ReturnsPass)
{
    auto trader = std::make_unique<entity::WanderingTraderEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    trader->setWorld(m_world.get());
    trader->setUuid("wandering-trader-uuid-001");
    Entity* traderRaw = trader.get();
    m_world->registerEntity(*traderRaw);
    // 持有 trader 的所有权，避免在测试结束前被销毁
    std::unique_ptr<entity::WanderingTraderEntity> traderOwner(std::move(trader));

    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setLeashedToEntity(traderRaw->uuid());

    auto player = std::make_unique<Player>(EntityInstanceId(3), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    EXPECT_TRUE(llama->isLeashed());
    auto result = llama->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Pass);
}

TEST_F(TraderLlamaIntegrationTest, InteractMob_LeashedToFence_DelegatesToBase)
{
    // 拴在栅栏上不是拴在流浪商人身上，interactMob 应委托给 LlamaEntity::interactMob
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setLeashedToFence(BlockPos(0, 64, 0));

    auto player = std::make_unique<Player>(EntityInstanceId(3), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    EXPECT_TRUE(llama->isLeashed());
    auto result = llama->interactMob(*player, Hand::MainHand);
    // 委托给 LlamaEntity::interactMob（未驯服、空手、无骑乘者）→ doPlayerRide → Success
    EXPECT_EQ(result, ActionResultType::Success);
}

TEST_F(TraderLlamaIntegrationTest, InteractMob_LeashedToOtherEntity_DelegatesToBase)
{
    // 拴在非流浪商人实体（如另一只羊驼）身上，interactMob 应委托给基类
    auto otherLlama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    otherLlama->setWorld(m_world.get());
    otherLlama->setUuid("other-llama-uuid-002");
    m_world->registerEntity(*otherLlama);

    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setLeashedToEntity(otherLlama->uuid());

    auto player = std::make_unique<Player>(EntityInstanceId(3), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    EXPECT_TRUE(llama->isLeashed());
    auto result = llama->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
}

TEST_F(TraderLlamaIntegrationTest, InteractMob_NotLeashed_DelegatesToBase)
{
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityInstanceId(3), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    EXPECT_FALSE(llama->isLeashed());
    auto result = llama->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
}

TEST_F(TraderLlamaIntegrationTest, IsLeashedToWanderingTrader_HolderUuidNotInWorld_False)
{
    // 拴绳 UUID 指向不存在的实体：isLeashedToWanderingTrader 返回 false
    // （getLeashHolderEntity 返回 nullptr，dynamic_cast 失败）
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setLeashedToEntity("nonexistent-uuid-9999");

    auto player = std::make_unique<Player>(EntityInstanceId(3), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    EXPECT_TRUE(llama->isLeashed());
    // 因 holder 不存在，isLeashedToWanderingTrader 返回 false → 走 LlamaEntity 路径
    auto result = llama->interactMob(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
}

// ============================================================================
// hasExactlyOnePlayerPassenger 间接测试（通过 canDespawn）
// ============================================================================
//
// canDespawn() = !isTame() && !isLeashed() && !hasExactlyOnePlayerPassenger()
// 在未驯服、未拴绳的前提下：
//   - hasExactlyOnePlayerPassenger 返回 true  → canDespawn 返回 false
//   - hasExactlyOnePlayerPassenger 返回 false → canDespawn 返回 true

TEST_F(TraderLlamaIntegrationTest, CanDespawn_NoPassengers_True)
{
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());

    EXPECT_FALSE(llama->isLeashed());
    EXPECT_FALSE(llama->isTame());
    EXPECT_TRUE(llama->canDespawn(0.0));
}

TEST_F(TraderLlamaIntegrationTest, CanDespawn_ExactlyOnePlayerPassenger_False)
{
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    m_world->registerEntity(*llama);

    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    m_world->registerEntity(*player);

    // 玩家骑乘羊驼（startRiding 需要 world->getEntity 解析 vehicle）
    ASSERT_TRUE(player->startRiding(*llama));
    EXPECT_EQ(llama->getPassengers().size(), 1u);

    // 恰好一名玩家乘客 → hasExactlyOnePlayerPassenger 为 true → canDespawn 为 false
    EXPECT_FALSE(llama->canDespawn(0.0));
}

TEST_F(TraderLlamaIntegrationTest, CanDespawn_ExactlyOneNonPlayerPassenger_True)
{
    // 乘客是非 Player 实体（例如另一只羊驼）→ hasExactlyOnePlayerPassenger 为 false
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    m_world->registerEntity(*llama);

    auto otherLlama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    otherLlama->setWorld(m_world.get());
    m_world->registerEntity(*otherLlama);

    ASSERT_TRUE(otherLlama->startRiding(*llama));
    EXPECT_EQ(llama->getPassengers().size(), 1u);

    EXPECT_TRUE(llama->canDespawn(0.0));
}

TEST_F(TraderLlamaIntegrationTest, CanDespawn_ZeroPassengers_True)
{
    // 无乘客时 hasExactlyOnePlayerPassenger 为 false → canDespawn 为 true
    // （载具最多容纳 1 名乘客，无法测试多乘客场景，此处验证零乘客基线）
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());

    EXPECT_TRUE(llama->canDespawn(0.0));
}

// ============================================================================
// maybeDespawn 间接测试（通过 tick）
// ============================================================================
//
// tick() 在服务端会调用 maybeDespawn()，后者：
// 1. canDespawn 为 false 时不做任何事（倒计时不变）
// 2. 拴在流浪商人身上时同步商人倒计时（trader.despawnDelay - 1）
// 3. 否则自行递减
// 4. 倒计时 ≤ 0 时 clearLeash + discard
//
// 注意：tick() 会触发完整 LivingEntity/MobEntity tick 链，但 BaseTestWorld
// 的 isClientSide() 返回 false（服务端），m_world 非空，因此 maybeDespawn 会被调用。

TEST_F(TraderLlamaIntegrationTest, Tick_CanDespawnFalse_DespawnDelayUnchanged)
{
    // 驯服的羊驼 canDespawn 返回 false，tick 后倒计时不变
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setTame(true);
    llama->setDespawnDelay(100);

    llama->tick();

    EXPECT_EQ(llama->getDespawnDelay(), 100);
    EXPECT_FALSE(llama->isRemoved());
}

TEST_F(TraderLlamaIntegrationTest, Tick_NotLeashed_DecrementsDespawnDelay)
{
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setDespawnDelay(100);

    llama->tick();

    EXPECT_EQ(llama->getDespawnDelay(), 99);
    EXPECT_FALSE(llama->isRemoved());
}

TEST_F(TraderLlamaIntegrationTest, Tick_LeashedToWanderingTrader_SyncsDespawnDelay)
{
    auto trader = std::make_unique<entity::WanderingTraderEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    trader->setWorld(m_world.get());
    trader->setUuid("trader-uuid-sync-001");
    trader->setDespawnDelay(500);
    m_world->registerEntity(*trader);

    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setLeashedToEntity(trader->uuid());
    llama->setDespawnDelay(100); // 初始与商人不同步

    EXPECT_TRUE(llama->isLeashed());

    llama->tick();

    // 同步后 llama.despawnDelay = trader.despawnDelay - 1 = 499
    EXPECT_EQ(llama->getDespawnDelay(), 499);
    EXPECT_FALSE(llama->isRemoved());
}

TEST_F(TraderLlamaIntegrationTest, Tick_LeashedToWanderingTrader_TraderDespawnZero_LlamaDiscards)
{
    auto trader = std::make_unique<entity::WanderingTraderEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    trader->setWorld(m_world.get());
    trader->setUuid("trader-uuid-discard-001");
    trader->setDespawnDelay(1); // 商人倒计时为 1 → llama 同步为 0 → 触发 discard
    m_world->registerEntity(*trader);

    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setLeashedToEntity(trader->uuid());
    llama->setDespawnDelay(50);

    llama->tick();

    // despawnDelay 同步为 0，触发 discard
    EXPECT_TRUE(llama->isRemoved());
    // 拴绳状态应已清除
    EXPECT_FALSE(llama->isLeashed());
}

TEST_F(TraderLlamaIntegrationTest, Tick_NotLeashed_DelayReachesZero_Discards)
{
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setDespawnDelay(1);

    llama->tick();

    // 倒计时减为 0 → discard
    EXPECT_TRUE(llama->isRemoved());
}

TEST_F(TraderLlamaIntegrationTest, Tick_LeashedToOtherEntity_DespawnDelayUnchanged)
{
    // 拴在非流浪商人实体上：内部消失判定为 false（leashedToOther 为 true）
    // → maybeDespawn 直接 return，倒计时不变
    auto otherLlama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    otherLlama->setWorld(m_world.get());
    otherLlama->setUuid("other-llama-uuid-indep-001");
    m_world->registerEntity(*otherLlama);

    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setLeashedToEntity(otherLlama->uuid());
    llama->setDespawnDelay(100);

    EXPECT_TRUE(llama->isLeashed());

    llama->tick();

    // 拴在非流浪商人实体上 → maybeDespawn 直接返回，倒计时不变
    EXPECT_EQ(llama->getDespawnDelay(), 100);
    EXPECT_FALSE(llama->isRemoved());
}

TEST_F(TraderLlamaIntegrationTest, Tick_LeashedToFence_DespawnDelayUnchanged)
{
    // 拴在栅栏上：canDespawn 为 false（isLeashed 为 true）
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    // 将羊驼放在栅栏旁，避免 tickLeash 因距离超过 LEASH_SNAP_DISTANCE 而掉绳
    llama->setPosition(0.0f, 64.0f, 0.0f);
    llama->setLeashedToFence(BlockPos(0, 64, 0));
    llama->setDespawnDelay(100);

    llama->tick();

    EXPECT_EQ(llama->getDespawnDelay(), 100);
    EXPECT_FALSE(llama->isRemoved());
}

// ============================================================================
// 综合场景：消失前清除拴绳
// ============================================================================

TEST_F(TraderLlamaIntegrationTest, Tick_DespawnClearsLeashBeforeDiscard)
{
    auto trader = std::make_unique<entity::WanderingTraderEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    trader->setWorld(m_world.get());
    trader->setUuid("trader-uuid-clear-001");
    trader->setDespawnDelay(1);
    m_world->registerEntity(*trader);

    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(m_world.get());
    llama->setLeashedToEntity(trader->uuid());
    llama->setDespawnDelay(50);

    EXPECT_TRUE(llama->isLeashed());

    llama->tick();

    // 拴绳应在 discard 前被清除
    EXPECT_TRUE(llama->isRemoved());
    EXPECT_FALSE(llama->isLeashed());
    EXPECT_FALSE(llama->leashHolderUuid().has_value());
}

// ============================================================================
// 边界场景：tick 链不崩溃
// ============================================================================

TEST_F(TraderLlamaIntegrationTest, Tick_NoWorld_NoCrash)
{
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 不设置 world
    llama->setDespawnDelay(100);

    // 不应崩溃（tick 内部对 m_world == nullptr 做了保护）
    llama->tick();
    SUCCEED();
}

TEST_F(TraderLlamaIntegrationTest, Tick_ClientSide_NoDespawn)
{
    // 覆写 isClientSide 为 true，验证客户端不执行 maybeDespawn
    class ClientSideTestWorld : public TraderLlamaIntegrationTestWorld {
    public:
        [[nodiscard]] bool isClientSide() const override { return true; }
    };

    auto world = std::make_unique<ClientSideTestWorld>();
    auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(world.get());
    llama->setDespawnDelay(100);

    llama->tick();

    // 客户端不应递减倒计时
    EXPECT_EQ(llama->getDespawnDelay(), 100);
    EXPECT_FALSE(llama->isRemoved());
}

} // namespace
} // namespace mc
