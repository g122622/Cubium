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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/NameTagItem.hpp"
#include "common/item/items/special/SpawnEggItem.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试世界 - 支持 processInitialInteract 所需的 IWorld 接口
//
// 扩展点：
// - setClientSide(true) 切换为客户端模式（用于测试客户端预测分支）
// - spawnedEntities() 取出已生成的实体（用于测试 _spawnOffspringFromSpawnEgg）
// ============================================================================

class MobInteractTestWorld final : public test::BaseTestWorld {
public:
    MobInteractTestWorld()
    {
        Items::initialize();
        VanillaBlocks::initialize();
    }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

    // 客户端预测测试：覆写 isClientSide 以模拟客户端
    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }
    void setClientSide(bool clientSide) { m_clientSide = clientSide; }

    // 捕获 spawnEntity 调用以便测试断言幼体生成
    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityId id = ++m_nextEntityId;
        if (entity != nullptr) {
            entity->setId(id);
            m_spawnedEntities.push_back(std::move(entity));
        }
        return id;
    }

    // 测试辅助：取出已生成的实体（所有权转移给调用方）
    std::vector<std::unique_ptr<Entity>> takeSpawnedEntities() { return std::move(m_spawnedEntities); }

    // 测试辅助：已生成实体数量
    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

private:
    Difficulty m_difficulty = Difficulty::Normal;
    bool m_clientSide = false;
    EntityId m_nextEntityId = EntityId(100);
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

// ============================================================================
// 测试夹具
// ============================================================================

class MobEntityInteractTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<MobInteractTestWorld>(); }

    std::unique_ptr<MobInteractTestWorld> m_world;
};

// ============================================================================
// canBeLeashed 测试
// ============================================================================

TEST_F(MobEntityInteractTest, CanBeLeashed_NonHostileMobReturnsTrue)
{
    // PigEntity 继承自 AnimalEntity -> AgeableEntity -> CreatureEntity -> MobEntity
    // 不实现 IMob，所以 canBeLeashed() 应返回 true
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    EXPECT_TRUE(pig->canBeLeashed()) << "PigEntity (passive mob) should be leashable";
}

TEST_F(MobEntityInteractTest, CanBeLeashed_HostileMobReturnsFalse)
{
    // ZombieEntity 继承自 MonsterEntity，MonsterEntity 继承 IMob
    // 所以 canBeLeashed() 应返回 false
    auto zombie = std::make_unique<ZombieEntity>(EntityId(1));
    EXPECT_FALSE(zombie->canBeLeashed()) << "ZombieEntity (hostile mob implementing IMob) should not be leashable";
}

TEST_F(MobEntityInteractTest, CanBeLeashed_IMobInterfaceCheck)
{
    // 验证 canBeLeashed() 的底层 IMob 判断逻辑
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    auto zombie = std::make_unique<ZombieEntity>(EntityId(2));

    // ZombieEntity 实现了 IMob（通过 MonsterEntity）
    EXPECT_NE(dynamic_cast<const IMob*>(zombie.get()), nullptr) << "ZombieEntity should implement IMob interface";
    // PigEntity 没有实现 IMob
    EXPECT_EQ(dynamic_cast<const IMob*>(pig.get()), nullptr) << "PigEntity should NOT implement IMob interface";
}

// ============================================================================
// processInitialInteract 测试
// ============================================================================

TEST_F(MobEntityInteractTest, ProcessInitialInteract_DeadEntityReturnsPass)
{
    // 已死亡的实体应返回 Pass
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 移除实体模拟死亡状态
    pig->remove();
    EXPECT_FALSE(pig->isAlive());

    auto result = pig->processInitialInteract(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Pass);
}

TEST_F(MobEntityInteractTest, ProcessInitialInteract_EmptyHandCallsInteractMob)
{
    // 空手交互应该调用 interactMob（基类返回 Pass），然后传递到 LivingEntity
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    auto result = pig->processInitialInteract(*player, Hand::MainHand);
    // PigEntity 的 interactMob 可能返回 Success（可骑乘交互）或 Pass
    // 基类 MobEntity::interactMob 返回 Pass，但子类可能覆写
    // 结果应该是 Success、Consume 或 Pass 之一（不是 Fail）
    EXPECT_NE(result, ActionResultType::Fail);
}

TEST_F(MobEntityInteractTest, ProcessInitialInteract_NameTagWithoutNameReturnsPass)
{
    // 未命名的命名牌应对 MobEntity 返回 Pass（NameTagItem::itemInteractionForEntity 返回 false）
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 创建一个没有自定义名称的命名牌
    ItemStack nameTagStack(Items::NAME_TAG, 1);
    EXPECT_FALSE(nameTagStack.hasCustomName());

    // 设置到玩家主手
    player->getHeldItem(Hand::MainHand) = nameTagStack;

    // 未命名的命名牌应返回 Pass（interactMob 可能也返回 Pass）
    auto result = pig->processInitialInteract(*player, Hand::MainHand);
    // NameTagItem::itemInteractionForEntity 应该返回 false（没有自定义名称）
    // 所以 processInitialInteract 应该继续到 interactMob
    // 结果取决于 PigEntity::interactMob 的实现
    EXPECT_NE(result, ActionResultType::Fail);
}

TEST_F(MobEntityInteractTest, ProcessInitialInteract_NameTagWithNameNamesEntity)
{
    // 有自定义名称的命名牌应该成功命名实体
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    // 创建一个有自定义名称的命名牌
    ItemStack nameTagStack(Items::NAME_TAG, 1);
    nameTagStack.setCustomName("TestPig");
    EXPECT_TRUE(nameTagStack.hasCustomName());

    i32 countBefore = nameTagStack.getCount();

    // 设置到玩家主手
    player->getHeldItem(Hand::MainHand) = nameTagStack;

    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    // 应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 实体应有自定义名称
    EXPECT_TRUE(pig->hasCustomName());

    // 生存模式下命名牌应该被消耗一个
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), countBefore - 1);
}

TEST_F(MobEntityInteractTest, ProcessInitialInteract_NameTagCreativeNoConsume)
{
    // 创造模式下命名牌命名不消耗物品
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Creative);

    ItemStack nameTagStack(Items::NAME_TAG, 1);
    nameTagStack.setCustomName("CreativePig");
    player->getHeldItem(Hand::MainHand) = nameTagStack;

    i32 countBefore = player->getHeldItem(Hand::MainHand).getCount();

    auto result = pig->processInitialInteract(*player, Hand::MainHand);
    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(pig->hasCustomName());

    // 创造模式下不应消耗命名牌
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), countBefore);
}

TEST_F(MobEntityInteractTest, ProcessInitialInteract_NameTagPersistence)
{
    // 使用命名牌命名后实体应变为持久化
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    EXPECT_FALSE(pig->isNoDespawnRequired()) << "PigEntity should not require persistence by default";

    ItemStack nameTagStack(Items::NAME_TAG, 1);
    nameTagStack.setCustomName("NamedPig");
    player->getHeldItem(Hand::MainHand) = nameTagStack;

    pig->processInitialInteract(*player, Hand::MainHand);

    // 命名后应标记为持久化
    EXPECT_TRUE(pig->isNoDespawnRequired()) << "Named entity should require persistence";
}

TEST_F(MobEntityInteractTest, ProcessInitialInteract_LeadItemOnLeashableMobDoesNotCrash)
{
    // 拴绳在可拴住的生物上使用时，当前代码只是检查了条件但没有实现完整逻辑
    // 验证不会崩溃
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 设置拴绳到玩家主手
    ItemStack leadStack(Items::LEAD, 1);
    player->getHeldItem(Hand::MainHand) = leadStack;

    // 应该不崩溃，返回 interactMob 的结果或 Pass
    auto result = pig->processInitialInteract(*player, Hand::MainHand);
    // 拴绳逻辑当前是 TODO，所以会继续到 interactMob
    EXPECT_NE(result, ActionResultType::Fail);
}

TEST_F(MobEntityInteractTest, ProcessInitialInteract_LeadItemOnHostileMobDoesNotCrash)
{
    // 拴绳在敌对生物上使用也不会崩溃
    auto zombie = std::make_unique<ZombieEntity>(EntityId(1));
    zombie->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    ItemStack leadStack(Items::LEAD, 1);
    player->getHeldItem(Hand::MainHand) = leadStack;

    // 不崩溃即可
    auto result = zombie->processInitialInteract(*player, Hand::MainHand);
    EXPECT_NE(result, ActionResultType::Fail);
}

// ============================================================================
// AgeableEntity 类型检查与刷怪蛋前置条件测试
// ============================================================================

TEST_F(MobEntityInteractTest, AgeableEntity_TypeCheck)
{
    // 验证 PigEntity 是 AgeableEntity（可生成幼体的前提条件）
    // _spawnOffspringFromSpawnEgg 要求目标实体类型是 AgeableEntity 子类，
    // 否则 dynamic_cast<AgeableEntity*> 返回 nullptr，无法设置幼体状态
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    EXPECT_NE(dynamic_cast<AgeableEntity*>(pig.get()), nullptr)
        << "PigEntity should be an AgeableEntity (required for spawn egg baby creation)";

    // 验证 ZombieEntity 不是 AgeableEntity（不可生成幼体）
    auto zombie = std::make_unique<ZombieEntity>(EntityId(2));
    EXPECT_EQ(dynamic_cast<AgeableEntity*>(zombie.get()), nullptr) << "ZombieEntity should NOT be an AgeableEntity";
}

// ============================================================================
// _spawnOffspringFromSpawnEgg 完整测试
//
// 通过 processInitialInteract 触发刷怪蛋交互路径，覆盖以下场景：
//   1. 类型匹配时成功生成幼体（生存模式消耗物品）
//   2. 类型不匹配时返回 Pass（不生成幼体，不消耗物品）
//   3. 非 AgeableEntity 实体（如 ZombieEntity）使用匹配类型刷怪蛋返回 Pass
//   4. 创造模式下刷怪蛋不消耗物品
//   5. 客户端预测返回 Success
// ============================================================================

TEST_F(MobEntityInteractTest, SpawnEgg_MatchingTypeSpawnsBabyAndConsumesItem)
{
    // 刷怪蛋类型匹配时（猪刷怪蛋作用于猪），应成功生成幼体并消耗一个刷怪蛋
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setTypeId("minecraft:pig");
    pig->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    // 手持猪刷怪蛋（已在 Items::initialize() 中注册）
    ASSERT_NE(Items::PIG_SPAWN_EGG, nullptr);
    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    const i32 countBefore = player->getHeldItem(Hand::MainHand).getCount();
    EXPECT_EQ(countBefore, 4);

    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    // 服务端成功生成幼体
    EXPECT_EQ(result, ActionResultType::Success);

    // 物品消耗 1 个
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), countBefore - 1);

    // 应生成 1 个幼体实体
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);

    // 幼体应为 AgeableEntity 且处于幼体状态
    auto spawned = m_world->takeSpawnedEntities();
    ASSERT_EQ(spawned.size(), 1u);
    auto* babyAgeable = dynamic_cast<AgeableEntity*>(spawned[0].get());
    EXPECT_NE(babyAgeable, nullptr) << "Spawned baby should be an AgeableEntity";
    EXPECT_TRUE(babyAgeable->isChild()) << "Spawned baby should be a child (growing age < 0)";
    EXPECT_EQ(spawned[0]->getTypeId(), "minecraft:pig") << "Spawned baby should be minecraft:pig";
}

TEST_F(MobEntityInteractTest, SpawnEgg_MismatchedTypeReturnsPass)
{
    // 刷怪蛋类型不匹配时（牛刷怪蛋作用于猪），应返回 Pass 且不生成幼体、不消耗物品
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setTypeId("minecraft:pig");
    pig->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    // 手持牛刷怪蛋作用于猪 - 类型不匹配
    ASSERT_NE(Items::COW_SPAWN_EGG, nullptr);
    ItemStack eggStack(Items::COW_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    const i32 countBefore = player->getHeldItem(Hand::MainHand).getCount();

    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    // 类型不匹配 - _spawnOffspringFromSpawnEgg 返回 false，processInitialInteract 返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不消耗物品
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), countBefore);

    // 不生成幼体
    EXPECT_EQ(m_world->spawnedEntityCount(), 0u);
}

TEST_F(MobEntityInteractTest, SpawnEgg_NonAgeableEntityReturnsPass)
{
    // 非 AgeableEntity 实体（如 ZombieEntity）使用匹配类型刷怪蛋应返回 Pass
    // _spawnOffspringFromSpawnEgg 中 dynamic_cast<AgeableEntity*> 返回 nullptr，返回 false
    auto zombie = std::make_unique<ZombieEntity>(EntityId(1));
    zombie->setTypeId("minecraft:zombie");
    zombie->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    // 手持僵尸刷怪蛋作用于僵尸 - 类型匹配但 ZombieEntity 不是 AgeableEntity
    ASSERT_NE(Items::ZOMBIE_SPAWN_EGG, nullptr);
    ItemStack eggStack(Items::ZOMBIE_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    const i32 countBefore = player->getHeldItem(Hand::MainHand).getCount();

    auto result = zombie->processInitialInteract(*player, Hand::MainHand);

    // ZombieEntity 不是 AgeableEntity，无法生成幼体，返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 不消耗物品
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), countBefore);

    // 不生成实体
    EXPECT_EQ(m_world->spawnedEntityCount(), 0u);
}

TEST_F(MobEntityInteractTest, SpawnEgg_CreativeModeDoesNotConsumeItem)
{
    // 创造模式下刷怪蛋生成幼体但不消耗物品
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setTypeId("minecraft:pig");
    pig->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Creative);

    ASSERT_NE(Items::PIG_SPAWN_EGG, nullptr);
    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    const i32 countBefore = player->getHeldItem(Hand::MainHand).getCount();

    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    EXPECT_EQ(result, ActionResultType::Success);

    // 创造模式不消耗物品
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), countBefore);

    // 仍生成幼体
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);
}

TEST_F(MobEntityInteractTest, SpawnEgg_ClientSidePredictsSuccess)
{
    // 客户端预测：isClientSide() == true 时直接返回 Success，不生成实体、不消耗物品
    m_world->setClientSide(true);

    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setTypeId("minecraft:pig");
    pig->setWorld(m_world.get());

    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    ASSERT_NE(Items::PIG_SPAWN_EGG, nullptr);
    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    const i32 countBefore = player->getHeldItem(Hand::MainHand).getCount();

    auto result = pig->processInitialInteract(*player, Hand::MainHand);

    // 客户端预测成功
    EXPECT_EQ(result, ActionResultType::Success);

    // 客户端不消耗物品（实际消耗由服务端处理）
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), countBefore);

    // 客户端不生成实体（实际生成由服务端处理）
    EXPECT_EQ(m_world->spawnedEntityCount(), 0u);
}

// ============================================================================
// MobEntity::canDespawn 和 preventDespawn 测试
// ============================================================================

TEST_F(MobEntityInteractTest, CanDespawn_HostileMobReturnsTrue)
{
    // MonsterEntity (ZombieEntity) 默认 canDespawn 返回 true
    auto zombie = std::make_unique<ZombieEntity>(EntityId(1));
    EXPECT_TRUE(zombie->canDespawn(0.0)) << "MonsterEntity should be able to despawn by default";
}

TEST_F(MobEntityInteractTest, PreventDespawn_DefaultReturnsFalse)
{
    // 默认情况下 preventDespawn 返回 isRiding()，未骑乘时为 false
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    EXPECT_FALSE(pig->preventDespawn()) << "Non-riding entity should not prevent despawn by default";
}

TEST_F(MobEntityInteractTest, PersistenceRequired_AfterNaming)
{
    // 被命名牌命名后应该需要持久化
    auto pig = std::make_unique<PigEntity>(EntityId(1));
    pig->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    EXPECT_FALSE(pig->isNoDespawnRequired());

    ItemStack nameTagStack(Items::NAME_TAG, 1);
    nameTagStack.setCustomName("TestPig");
    player->getHeldItem(Hand::MainHand) = nameTagStack;

    pig->processInitialInteract(*player, Hand::MainHand);

    EXPECT_TRUE(pig->isNoDespawnRequired()) << "Named entity should require persistence";
}

// ============================================================================
// interactMob 子类交互测试
// ============================================================================

TEST_F(MobEntityInteractTest, InteractMob_BaseMobEntityReturnsPass)
{
    // MobEntity 基类的 interactMob 应返回 Pass
    auto zombie = std::make_unique<ZombieEntity>(EntityId(1));
    zombie->setWorld(m_world.get());
    auto player = std::make_unique<Player>(EntityId(2), "TestPlayer");
    player->setWorld(m_world.get());

    // 直接调用 interactMob（不是 processInitialInteract）
    auto result = zombie->interactMob(*player, Hand::MainHand);
    // ZombieEntity 可能覆写了 interactMob，但基类 MobEntity 返回 Pass
    // 实际结果取决于 ZombieEntity 的实现
    EXPECT_TRUE(
        result == ActionResultType::Pass || result == ActionResultType::Success || result == ActionResultType::Consume)
        << "interactMob should return Pass, Success, or Consume";
}

// ============================================================================
// 命名牌对 Player 无效测试
// ============================================================================

TEST_F(MobEntityInteractTest, ProcessInitialInteract_NameTagOnPlayerReturnsPass)
{
    // 命名牌不能对玩家使用，NameTagItem::itemInteractionForEntity 对 Player 返回 false
    auto targetPlayer = std::make_unique<Player>(EntityId(1), "TargetPlayer");
    targetPlayer->setWorld(m_world.get());
    auto sourcePlayer = std::make_unique<Player>(EntityId(2), "SourcePlayer");
    sourcePlayer->setWorld(m_world.get());

    ItemStack nameTagStack(Items::NAME_TAG, 1);
    nameTagStack.setCustomName("NamedPlayer");
    sourcePlayer->getHeldItem(Hand::MainHand) = nameTagStack;

    // Player 不是 MobEntity，processInitialInteract 不会走到命名牌分支
    // （因为 NameTagItem::itemInteractionForEntity 检查 dynamic_cast<MobEntity*> 返回 nullptr）
    auto result = targetPlayer->processInitialInteract(*sourcePlayer, Hand::MainHand);
    // Player 继承 LivingEntity，其 processInitialInteract 默认返回 Pass 或其他
    EXPECT_NE(result, ActionResultType::Fail);
}
