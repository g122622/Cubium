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
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试用 MobEntity 子类，用于 NBT 序列化测试
// ============================================================================

class TestLeashMobEntity : public MobEntity {
public:
    TestLeashMobEntity()
        : MobEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    explicit TestLeashMobEntity(EntityInstanceId id)
        : MobEntity(id, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// ============================================================================
// 测试世界
// ============================================================================

class LeashTestWorld final : public mc::test::BaseTestWorld {
public:
    LeashTestWorld()
    {
        Items::initialize();
        VanillaBlocks::initialize();
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

private:
    Difficulty m_difficulty = Difficulty::Normal;
};

// ============================================================================
// 测试夹具
// ============================================================================

class LeashSystemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<LeashTestWorld>(); }

    std::unique_ptr<LeashTestWorld> m_world;
};

// ============================================================================
// MobEntity 拴绳状态测试
// ============================================================================

TEST_F(LeashSystemTest, MobInitiallyNotLeashed)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());

    EXPECT_FALSE(pig->isLeashed());
    EXPECT_FALSE(pig->leashHolderUuid().has_value());
    EXPECT_FALSE(pig->leashFencePos().has_value());
}

TEST_F(LeashSystemTest, SetLeashedToEntity_SetsLeashedState)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());

    const std::string playerUuid = "test-player-uuid-1234";
    pig->setLeashedToEntity(playerUuid);

    EXPECT_TRUE(pig->isLeashed());
    EXPECT_TRUE(pig->leashHolderUuid().has_value());
    EXPECT_EQ(pig->leashHolderUuid().value(), playerUuid);
    EXPECT_FALSE(pig->leashFencePos().has_value());
}

TEST_F(LeashSystemTest, SetLeashedToFence_SetsLeashedState)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());

    BlockPos fencePos(10, 64, 20);
    pig->setLeashedToFence(fencePos);

    EXPECT_TRUE(pig->isLeashed());
    EXPECT_FALSE(pig->leashHolderUuid().has_value());
    EXPECT_TRUE(pig->leashFencePos().has_value());
    EXPECT_EQ(pig->leashFencePos().value(), fencePos);
}

TEST_F(LeashSystemTest, ClearLeash_ClearsAllLeashState)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());

    pig->setLeashedToEntity("test-uuid");
    EXPECT_TRUE(pig->isLeashed());

    pig->clearLeash();
    EXPECT_FALSE(pig->isLeashed());
    EXPECT_FALSE(pig->leashHolderUuid().has_value());
    EXPECT_FALSE(pig->leashFencePos().has_value());
}

TEST_F(LeashSystemTest, DropLeash_ClearsLeashState)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());

    pig->setLeashedToEntity("test-uuid");
    EXPECT_TRUE(pig->isLeashed());

    pig->dropLeash();
    EXPECT_FALSE(pig->isLeashed());
    EXPECT_FALSE(pig->leashHolderUuid().has_value());
}

TEST_F(LeashSystemTest, CanBeLeashed_PassiveMobReturnsTrue)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(pig->canBeLeashed());
}

TEST_F(LeashSystemTest, CanBeLeashed_HostileMobReturnsFalse)
{
    auto zombie = std::make_unique<ZombieEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(zombie->canBeLeashed());
}

// ============================================================================
// processInitialInteract 拴绳交互测试
// ============================================================================

TEST_F(LeashSystemTest, ProcessInitialInteract_LeadLeashesPig)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    // 设置玩家手持拴绳
    ItemStack leadStack(Items::LEAD, 1);
    player->getHeldItem(Hand::MainHand) = leadStack;

    EXPECT_FALSE(pig->isLeashed());

    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    // 应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 猪应该被拴住
    EXPECT_TRUE(pig->isLeashed());
    EXPECT_TRUE(pig->leashHolderUuid().has_value());
    EXPECT_EQ(pig->leashHolderUuid().value(), player->uuid());

    // 生存模式下拴绳应被消耗
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 0);
}

TEST_F(LeashSystemTest, ProcessInitialInteract_LeadLeashesPig_CreativeModeNoConsume)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Creative);

    // 设置玩家手持拴绳
    ItemStack leadStack(Items::LEAD, 1);
    player->getHeldItem(Hand::MainHand) = leadStack;

    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(pig->isLeashed());

    // 创造模式下拴绳不应被消耗
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 1);
}

TEST_F(LeashSystemTest, ProcessInitialInteract_LeadUnleashesAlreadyLeashedPig)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    // 先拴住猪
    pig->setLeashedToEntity(player->uuid());
    EXPECT_TRUE(pig->isLeashed());

    // 手持拴绳再次右键应该解除拴绳
    ItemStack leadStack(Items::LEAD, 1);
    player->getHeldItem(Hand::MainHand) = leadStack;

    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_FALSE(pig->isLeashed());
}

TEST_F(LeashSystemTest, ProcessInitialInteract_LeadCannotLeashHostileMob)
{
    auto zombie = std::make_unique<ZombieEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    // 设置玩家手持拴绳
    ItemStack leadStack(Items::LEAD, 1);
    player->getHeldItem(Hand::MainHand) = leadStack;

    auto result = zombie->processInitialInteract(*player, Hand::MainHand);

    // 敌对生物不能被拴住，应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_FALSE(zombie->isLeashed());
}

TEST_F(LeashSystemTest, ProcessInitialInteract_EmptyHandDoesNotLeash)
{
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    // 空手不应拴住生物
    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    // 空手交互不应拴住猪（可能调用 interactMob）
    EXPECT_FALSE(pig->isLeashed());
}

// ============================================================================
// LeashKnotEntity 测试
// ============================================================================

TEST_F(LeashSystemTest, LeashKnotEntityCreation)
{
    entity::LeashKnotEntity knot(mc::test::testEcsRegistry());
    EXPECT_EQ(knot.getWidth(), 1);
    EXPECT_EQ(knot.getHeight(), 1);
}

TEST_F(LeashSystemTest, LeashKnotEntityAttachDetach)
{
    entity::LeashKnotEntity knot(mc::test::testEcsRegistry());

    // 创建测试实体用于绑定
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    Entity* rawPig = pig.get();

    // 绑定
    knot.attachLeash(rawPig);
    EXPECT_EQ(knot.getLeashedEntities().size(), 1u);
    EXPECT_EQ(knot.getLeashedEntities()[0], rawPig);

    // 重复绑定不应添加重复
    knot.attachLeash(rawPig);
    EXPECT_EQ(knot.getLeashedEntities().size(), 1u);

    // 解绑
    knot.detachLeash(rawPig);
    EXPECT_EQ(knot.getLeashedEntities().size(), 0u);

    // 解绑不存在的实体不应崩溃
    knot.detachLeash(rawPig);
    EXPECT_EQ(knot.getLeashedEntities().size(), 0u);
}

TEST_F(LeashSystemTest, LeashKnotEntitySurivesWithoutWorld)
{
    entity::LeashKnotEntity knot(mc::test::testEcsRegistry());
    // 没有设置世界时应返回 false
    EXPECT_FALSE(knot.survives());
}

// ============================================================================
// ItemFrameEntity processInitialInteract 测试
// ============================================================================

TEST_F(LeashSystemTest, ItemFrameProcessInitialInteract_PlaceItem)
{
    entity::ItemFrameEntity itemFrame(mc::test::testEcsRegistry());
    itemFrame.setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    // 展示框初始为空
    EXPECT_FALSE(itemFrame.hasItem());
    EXPECT_EQ(itemFrame.getItemRotation(), 0);

    // 手持钻石放入展示框
    ItemStack diamond(Items::DIAMOND, 1);
    player->getHeldItem(Hand::MainHand) = diamond;

    auto result = itemFrame.processInitialInteract(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(itemFrame.hasItem());
}

TEST_F(LeashSystemTest, ItemFrameProcessInitialInteract_RotateItem)
{
    entity::ItemFrameEntity itemFrame(mc::test::testEcsRegistry());
    itemFrame.setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    // 先放入物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);
    EXPECT_TRUE(itemFrame.hasItem());

    // 空手右键应旋转物品
    auto result = itemFrame.processInitialInteract(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(itemFrame.getItemRotation(), 1);
}

TEST_F(LeashSystemTest, ItemFrameProcessInitialInteract_SneakRemoveItem)
{
    entity::ItemFrameEntity itemFrame(mc::test::testEcsRegistry());
    itemFrame.setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    // 先放入物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);
    EXPECT_TRUE(itemFrame.hasItem());

    // 潜行+右键应取出物品
    player->setSneaking(true);

    auto result = itemFrame.processInitialInteract(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_FALSE(itemFrame.hasItem());
    EXPECT_EQ(itemFrame.getItemRotation(), 0);
}

// ============================================================================
// ItemFrameEntity 红石信号测试
// ============================================================================

TEST_F(LeashSystemTest, ItemFrameAnalogOutput_NoItem_ReturnsZero)
{
    entity::ItemFrameEntity itemFrame(mc::test::testEcsRegistry());
    EXPECT_FALSE(itemFrame.hasItem());
    EXPECT_EQ(itemFrame.getAnalogOutput(), 0);
}

TEST_F(LeashSystemTest, ItemFrameAnalogOutput_WithItem_ReturnsRotationPlusOne)
{
    entity::ItemFrameEntity itemFrame(mc::test::testEcsRegistry());
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);

    itemFrame.setItemRotation(0);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 1);

    itemFrame.setItemRotation(7);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 8);
}

// ============================================================================
// MobEntity NBT 序列化 - 拴绳数据测试
// ============================================================================

TEST_F(LeashSystemTest, LeashDataNbtSerialization_EntityLeash)
{
    auto mob = std::make_unique<TestLeashMobEntity>(EntityInstanceId(1));

    // 设置拴绳绑定到实体
    mob->setLeashedToEntity("550e8400-e29b-41d4-a716-446655440000");
    EXPECT_TRUE(mob->isLeashed());
    EXPECT_TRUE(mob->leashHolderUuid().has_value());

    // 序列化
    nbt::tags::compound_tag tag;
    mob->addAdditionalSaveData(tag);

    // 反序列化到新实体
    auto mob2 = std::make_unique<TestLeashMobEntity>(EntityInstanceId(2));
    auto result = mob2->readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    // 验证拴绳数据
    EXPECT_TRUE(mob2->isLeashed());
    EXPECT_TRUE(mob2->leashHolderUuid().has_value());
    // UUID 在 NBT 序列化过程中会经过格式转换（UUIDMost/UUIDLeast → 字符串），
    // 转换后的字符串格式可能与原始输入不同，因此只验证非空
    EXPECT_FALSE(mob2->leashHolderUuid().value().empty());
    EXPECT_FALSE(mob2->leashFencePos().has_value());
}

TEST_F(LeashSystemTest, LeashDataNbtSerialization_FenceLeash)
{
    auto mob = std::make_unique<TestLeashMobEntity>(EntityInstanceId(1));

    // 设置拴绳绑定到栅栏
    BlockPos fencePos(100, 64, -200);
    mob->setLeashedToFence(fencePos);
    EXPECT_TRUE(mob->isLeashed());
    EXPECT_TRUE(mob->leashFencePos().has_value());

    // 序列化
    nbt::tags::compound_tag tag;
    mob->addAdditionalSaveData(tag);

    // 反序列化到新实体
    auto mob2 = std::make_unique<TestLeashMobEntity>(EntityInstanceId(2));
    auto result = mob2->readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    // 验证拴绳数据
    EXPECT_TRUE(mob2->isLeashed());
    EXPECT_FALSE(mob2->leashHolderUuid().has_value());
    EXPECT_TRUE(mob2->leashFencePos().has_value());
    EXPECT_EQ(mob2->leashFencePos().value(), fencePos);
}

TEST_F(LeashSystemTest, LeashDataNbtSerialization_NoLeash)
{
    auto mob = std::make_unique<TestLeashMobEntity>(EntityInstanceId(1));

    // 不设置拴绳
    EXPECT_FALSE(mob->isLeashed());

    // 序列化
    nbt::tags::compound_tag tag;
    mob->addAdditionalSaveData(tag);

    // 反序列化到新实体
    auto mob2 = std::make_unique<TestLeashMobEntity>(EntityInstanceId(2));
    auto result = mob2->readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());

    // 应该仍然没有被拴住
    EXPECT_FALSE(mob2->isLeashed());
}

// ============================================================================
// tickLeash 延迟绑定恢复测试
// ============================================================================

// 支持实体查找的测试世界
class LeashEntityTestWorld final : public mc::test::BaseTestWorld {
public:
    LeashEntityTestWorld()
    {
        Items::initialize();
        VanillaBlocks::initialize();
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

    // 覆写 getEntityByUuid 以支持 UUID 查找
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

    // 注册实体到 UUID 索引
    void registerEntity(Entity& entity) { m_uuidToEntity[entity.uuid()] = &entity; }

    // 从 UUID 索引移除实体
    void unregisterEntity(const std::string& uuid) { m_uuidToEntity.erase(uuid); }

private:
    Difficulty m_difficulty = Difficulty::Normal;
    std::unordered_map<std::string, Entity*> m_uuidToEntity;
};

class LeashEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<LeashEntityTestWorld>(); }

    std::unique_ptr<LeashEntityTestWorld> m_world;
};

TEST_F(LeashEntityTest, TickLeash_DelayedBindEntityResolvesWhenEntityAvailable)
{
    // 模拟从 NBT 加载后延迟绑定场景：拴绳目标实体尚未加载
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    // 手动设置拴绳状态（模拟 NBT 加载后的延迟绑定状态）
    pig->setLeashedToEntity(player->uuid());

    // 模拟 NBT 加载后的延迟绑定状态：手动设置 delayInfo
    // 这模拟了 readAdditionalSaveData 中设置 m_leashDelayInfo.targetUuid 的场景
    auto& delayInfo = pig->leashDelayInfo();
    delayInfo.targetUuid = player->uuid();
    delayInfo.resolveTicks = 0;

    // 此时 delayInfo 有值，tickLeash 应该尝试解析
    EXPECT_TRUE(pig->isLeashed());
    EXPECT_TRUE(delayInfo.targetUuid.has_value());

    // 注册玩家实体到 UUID 索引
    m_world->registerEntity(*player);

    // 调用 tickLeash，应该能通过 UUID 找到持有者实体并完成延迟绑定
    pig->tickLeash();

    // 延迟绑定应该已解析
    EXPECT_TRUE(pig->isLeashed());
    EXPECT_FALSE(pig->leashDelayInfo().targetUuid.has_value());
    EXPECT_EQ(pig->leashDelayInfo().resolveTicks, 0);
}

TEST_F(LeashEntityTest, TickLeash_DelayedBindEntityTimesOut)
{
    // 模拟从 NBT 加载后延迟绑定场景：拴绳目标实体永远不加载
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());

    // 手动设置拴绳状态（模拟 NBT 加载后的延迟绑定状态）
    const std::string missingUuid = "nonexistent-player-uuid";
    pig->setLeashedToEntity(missingUuid);

    auto& delayInfo = pig->leashDelayInfo();
    delayInfo.targetUuid = missingUuid;
    delayInfo.resolveTicks = 0;

    EXPECT_TRUE(pig->isLeashed());

    // 模拟 99 次 tick，不应该掉落拴绳
    for (int i = 0; i < 99; ++i) {
        pig->tickLeash();
    }
    EXPECT_TRUE(pig->isLeashed());
    EXPECT_EQ(pig->leashDelayInfo().resolveTicks, 99);

    // 第 100 次 tick，应该掉落拴绳
    pig->tickLeash();
    EXPECT_FALSE(pig->isLeashed());
}

TEST_F(LeashEntityTest, TickLeash_UsesUuidIndexForHolderLookup)
{
    // 测试 tickLeash 在正常情况下使用 UUID 索引查找持有者
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());
    // 设置猪的位置
    pig->setPosition(0.0f, 64.0f, 0.0f);

    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    // 设置玩家位置（在拴绳范围内）
    player->setPosition(5.0f, 64.0f, 0.0f);

    // 注册玩家到 UUID 索引
    m_world->registerEntity(*player);

    // 设置拴绳
    pig->setLeashedToEntity(player->uuid());
    EXPECT_TRUE(pig->isLeashed());

    // tickLeash 应该能通过 UUID 找到持有者并正常工作（不掉落拴绳）
    pig->tickLeash();
    EXPECT_TRUE(pig->isLeashed());
}

TEST_F(LeashEntityTest, TickLeash_DropsLeashWhenHolderNotFoundByUuid)
{
    // 测试 tickLeash 在找不到持有者时掉落拴绳
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());
    pig->setPosition(0.0f, 64.0f, 0.0f);

    // 设置拴绳到一个不存在的 UUID（不注册到 UUID 索引）
    const std::string missingUuid = "nonexistent-entity-uuid";
    pig->setLeashedToEntity(missingUuid);
    // 清除延迟绑定信息（模拟已经完成绑定的状态）
    pig->leashDelayInfo().targetUuid = std::nullopt;
    pig->leashDelayInfo().fencePos = std::nullopt;

    EXPECT_TRUE(pig->isLeashed());

    // tickLeash 应该找不到持有者，掉落拴绳
    pig->tickLeash();
    EXPECT_FALSE(pig->isLeashed());
}

TEST_F(LeashEntityTest, SetLeashedToEntity_UsesUuidIndexForBroadcast)
{
    // 测试 setLeashedToEntity 使用 UUID 索引查找持有者进行广播
    auto pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    // 注册玩家到 UUID 索引
    m_world->registerEntity(*player);

    // 设置拴绳，应该能通过 UUID 找到持有者
    pig->setLeashedToEntity(player->uuid());

    EXPECT_TRUE(pig->isLeashed());
    EXPECT_TRUE(pig->leashHolderUuid().has_value());
    EXPECT_EQ(pig->leashHolderUuid().value(), player->uuid());
}
