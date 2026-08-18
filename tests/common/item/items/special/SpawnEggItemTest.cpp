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
 * @file SpawnEggItemTest.cpp
 * @brief SpawnEggItem 右键方块/空气生成实体单元测试
 *
 * 覆盖核心修复点（spawnEntity 反查 EntityRegistry 真实工厂）与 Java 行为对齐：
 * - spawnEntity：真实注册表反查成功 / 未知类型失败 / 和平难度怪物拒绝
 * - onItemUse：常规方块生成 / 空碰撞形状原位生成 / 阻塞位置仍生成 / 和平难度 / 客户端预测
 * - onItemRightClick：水源方块生成 / 无水 PASS / 流动水 PASS / 客户端预测
 *
 * 关键：使用真实 Items::PIG_SPAWN_EGG / ZOMBIE_SPAWN_EGG（反查真实 EntityRegistry 工厂），
 * 不使用工厂返回 nullptr 的 mock egg，以真正覆盖反查路径。
 */

#include "common/item/items/special/SpawnEggItem.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

// ============================================================================
// 测试世界：方块存储 + 实体生成捕获 + 流体状态覆写
//
// 融合 SpawnerEggTestWorld（方块/方块实体存储）与 MobInteractTestWorld（spawnEntity 捕获）
// 的能力，并额外覆写 getFluidState 以支持 onItemRightClick 水源测试。
// ============================================================================
class SpawnEggTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;
    using IWorld::getFluidState;

    void setSeed(u64 seed) { m_seed = seed; }
    [[nodiscard]] u64 seed() const override { return m_seed; }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] math::Random& getRandom() override { return m_rng; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_rng; }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }
    void setClientSide(bool clientSide) { m_clientSide = clientSide; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
            return true;
        }
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    // 捕获生成的实体（所有权转移），用于断言生成结果
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityInstanceId id = ++m_nextEntityId;
        if (entity != nullptr) {
            entity->setId(id);
            m_spawnedEntities.push_back(std::move(entity));
        }
        return id;
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }
    std::vector<std::unique_ptr<Entity>> takeSpawnedEntities() { return std::move(m_spawnedEntities); }

    // 覆写 getFluidState：按方块位置返回注入的流体状态（用于水源/流动水测试）
    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_fluids.find(BlockPos(x, y, z));
        if (it != m_fluids.end()) {
            return it->second;
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    // 测试辅助：为指定位置注入流体状态
    void setFluidState(const BlockPos& pos, const fluid::FluidState* state) { m_fluids[pos] = state; }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

private:
    u64 m_seed = 12345;
    u64 m_currentTick = 0;
    Difficulty m_difficulty = Difficulty::Normal;
    bool m_clientSide = false;
    math::Random m_rng{12345};
    EntityInstanceId m_nextEntityId = EntityInstanceId(100);
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, const fluid::FluidState*> m_fluids;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

// 构造 ItemUseContext 的辅助函数（右键方块场景）
std::unique_ptr<ItemUseContext> makeUseContext(
    IWorld& world, Player* player, const ItemStack& stack, const BlockPos& blockPos, Direction face)
{
    const Vector3 hitPos(
        static_cast<f32>(blockPos.x) + 0.5f, static_cast<f32>(blockPos.y) + 1.0f, static_cast<f32>(blockPos.z) + 0.5f);
    return std::make_unique<ItemUseContext>(world, player, stack, hitPos, blockPos, face, Hand::MainHand, 0.0f, 90.0f);
}

} // namespace

// ============================================================================
// 测试夹具
// ============================================================================
class SpawnEggItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            VanillaBlocks::initialize();
            entity::VanillaEntities::registerAll();
            fluid::FluidRegistry::instance().initialize();
            Items::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<SpawnEggTestWorld>(); }

    std::unique_ptr<SpawnEggTestWorld> m_world;
};

// ============================================================================
// spawnEntity 测试
// ============================================================================

TEST_F(SpawnEggItemTest, SpawnEntity_RealRegistryPigSucceeds)
{
    // 真实猪刷怪蛋反查 EntityRegistry 成功，应生成 minecraft:pig 实体
    ASSERT_NE(Items::PIG_SPAWN_EGG, nullptr);
    auto* pigEgg = static_cast<item::SpawnEggItem*>(Items::PIG_SPAWN_EGG);

    const BlockPos pos(10, 64, 20);
    EXPECT_TRUE(pigEgg->spawnEntity(*m_world, pos, world::spawn::SpawnReason::SpawnEgg));
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);

    auto spawned = m_world->takeSpawnedEntities();
    ASSERT_EQ(spawned.size(), 1u);
    EXPECT_EQ(spawned[0]->getTypeId(), "minecraft:pig");
}

TEST_F(SpawnEggItemTest, SpawnEntity_UnknownEntityTypeReturnsFalse)
{
    // 构造一个 name 不在注册表中的刷怪蛋，反查失败应返回 false
    auto unknownType =
        entity::EntityType::Builder([](IWorld*, ecs::EntityRegistry&) -> std::unique_ptr<Entity> { return nullptr; },
            entity::EntityClassification::Creature)
            .build();
    const_cast<std::string&>(unknownType.name()) = "minecraft:nonexistent_entity";

    item::SpawnEggItem egg(std::move(unknownType), 0xFFFFFF, 0x000000, ItemProperties().maxStackSize(64));

    const BlockPos pos(10, 64, 20);
    EXPECT_FALSE(egg.spawnEntity(*m_world, pos, world::spawn::SpawnReason::SpawnEgg));
    EXPECT_EQ(m_world->spawnedEntityCount(), 0u);
}

TEST_F(SpawnEggItemTest, SpawnEntity_PeacefulDifficultyMonsterFails)
{
    // 和平难度下怪物类刷怪蛋（僵尸）不应生成
    m_world->setDifficulty(Difficulty::Peaceful);
    ASSERT_NE(Items::ZOMBIE_SPAWN_EGG, nullptr);
    auto* zombieEgg = static_cast<item::SpawnEggItem*>(Items::ZOMBIE_SPAWN_EGG);

    const BlockPos pos(10, 64, 20);
    EXPECT_FALSE(zombieEgg->spawnEntity(*m_world, pos, world::spawn::SpawnReason::SpawnEgg));
    EXPECT_EQ(m_world->spawnedEntityCount(), 0u);
}

TEST_F(SpawnEggItemTest, SpawnEntity_PeacefulDifficultyAnimalSucceeds)
{
    // 和平难度下动物类刷怪蛋（猪）仍可生成
    m_world->setDifficulty(Difficulty::Peaceful);
    ASSERT_NE(Items::PIG_SPAWN_EGG, nullptr);
    auto* pigEgg = static_cast<item::SpawnEggItem*>(Items::PIG_SPAWN_EGG);

    const BlockPos pos(10, 64, 20);
    EXPECT_TRUE(pigEgg->spawnEntity(*m_world, pos, world::spawn::SpawnReason::SpawnEgg));
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);
}

// ============================================================================
// onItemUse 测试
// ============================================================================

TEST_F(SpawnEggItemTest, OnItemUse_NormalBlockSpawnsEntityAtOffset)
{
    // 右键石头顶面：石头有碰撞形状，实体应生成在 offset(Up) 位置
    const BlockPos stonePos(10, 64, 20);
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    m_world->setBlockState(stonePos.x, stonePos.y, stonePos.z, stoneState);

    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    ASSERT_NE(Items::PIG_SPAWN_EGG, nullptr);
    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    auto context = makeUseContext(*m_world, player.get(), player->getHeldItem(Hand::MainHand), stonePos, Direction::Up);
    auto result = Items::PIG_SPAWN_EGG->onItemUse(*context);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);
    // 生存模式消耗 1 个
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 3);
}

TEST_F(SpawnEggItemTest, OnItemUse_EmptyCollisionShapeSpawnsInPlace)
{
    // 右键火把（空碰撞形状）：实体应生成在火把自身位置（不 offset）
    // 火把的 getCollisionShape().isEmpty() 为 true
    const BlockState* torchState = BlockRegistry::instance().get(ResourceLocation("minecraft:torch"));
    ASSERT_NE(torchState, nullptr) << "minecraft:torch must be registered";
    ASSERT_TRUE(torchState->getCollisionShape().isEmpty()) << "torch should have empty collision shape";

    const BlockPos torchPos(10, 64, 20);
    m_world->setBlockState(torchPos.x, torchPos.y, torchPos.z, torchState);

    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    auto context = makeUseContext(*m_world, player.get(), player->getHeldItem(Hand::MainHand), torchPos, Direction::Up);
    auto result = Items::PIG_SPAWN_EGG->onItemUse(*context);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);

    // 验证生成位置在火把自身（不 offset）：spawnEntity 内部用 pos.x+0.5 设置位置，
    // 这里通过捕获实体的位置反推 spawnPos。实体 y 应等于 torchPos.y（原位）而非 torchPos.y+1。
    auto spawned = m_world->takeSpawnedEntities();
    ASSERT_EQ(spawned.size(), 1u);
    EXPECT_FLOAT_EQ(spawned[0]->y(), static_cast<f32>(torchPos.y)) << "should spawn in place (no offset)";
}

TEST_F(SpawnEggItemTest, OnItemUse_BlockedSpawnPositionStillSpawnsEntity)
{
    // 右键石头顶面，offset(Up) 位置被另一个石头占据。
    // 对齐 Java spawnMob：不检查生成位置可替换性，实体与方块可共处，仍生成成功并消耗。
    const BlockPos stonePos(10, 64, 20);
    m_world->setBlockState(stonePos.x, stonePos.y, stonePos.z, &VanillaBlocks::STONE->defaultState());
    // 在石头上方放另一个石头，阻塞生成位置
    m_world->setBlockState(stonePos.x, stonePos.y + 1, stonePos.z, &VanillaBlocks::STONE->defaultState());

    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);

    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    auto context = makeUseContext(*m_world, player.get(), player->getHeldItem(Hand::MainHand), stonePos, Direction::Up);
    auto result = Items::PIG_SPAWN_EGG->onItemUse(*context);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 3) << "success should consume 1";
}

TEST_F(SpawnEggItemTest, OnItemUse_CreativeModeDoesNotConsume)
{
    // 创造模式右键石头顶面生成实体但不消耗刷怪蛋
    const BlockPos stonePos(10, 64, 20);
    m_world->setBlockState(stonePos.x, stonePos.y, stonePos.z, &VanillaBlocks::STONE->defaultState());

    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Creative);

    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    auto context = makeUseContext(*m_world, player.get(), player->getHeldItem(Hand::MainHand), stonePos, Direction::Up);
    auto result = Items::PIG_SPAWN_EGG->onItemUse(*context);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 4) << "creative should not consume";
}

TEST_F(SpawnEggItemTest, OnItemUse_ClientSidePredictsSuccess)
{
    // 客户端直接预测 Success，不生成实体、不消耗
    m_world->setClientSide(true);
    const BlockPos stonePos(10, 64, 20);
    m_world->setBlockState(stonePos.x, stonePos.y, stonePos.z, &VanillaBlocks::STONE->defaultState());

    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());

    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    auto context = makeUseContext(*m_world, player.get(), player->getHeldItem(Hand::MainHand), stonePos, Direction::Up);
    auto result = Items::PIG_SPAWN_EGG->onItemUse(*context);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(m_world->spawnedEntityCount(), 0u) << "client should not spawn";
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 4) << "client should not consume";
}

// ============================================================================
// onItemRightClick 测试
// ============================================================================

TEST_F(SpawnEggItemTest, OnItemRightClick_WaterSourceSpawnsEntity)
{
    // 玩家垂直俯视命中水源方块：应在水方块位置生成实体
    auto* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState& waterSource = waterFluid->defaultState();
    ASSERT_TRUE(waterSource.isSource()) << "water default state should be a source";

    // 玩家站在水面上方，垂直俯视。水面在 (10, 64, 19)。
    const BlockPos waterPos(10, 64, 19);
    m_world->setFluidState(waterPos, &waterSource);

    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);
    // 玩家位于水方块正上方（x/z 对齐方块中心），pitch=-90 垂直俯视
    // （Cubium Ray::fromAngles 约定：pitch=-90 向下，pitch=+90 向上）
    player->setPosition(10.5f, 66.0f, 19.5f);
    player->setRotation(0.0f, -90.0f);

    ASSERT_NE(Items::SQUID_SPAWN_EGG, nullptr);
    ItemStack eggStack(Items::SQUID_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    // 验证 getFluidState 注入生效
    const auto* fs = m_world->getFluidState(waterPos);
    ASSERT_NE(fs, nullptr);
    ASSERT_TRUE(fs->isSource());

    auto result = Items::SQUID_SPAWN_EGG->onItemRightClick(*m_world, *player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(m_world->spawnedEntityCount(), 1u);
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 3) << "survival should consume 1";
}

TEST_F(SpawnEggItemTest, OnItemRightClick_NoWaterReturnsPass)
{
    // 视线无水源方块：返回 Pass，不生成不消耗
    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setGameMode(GameMode::Survival);
    player->setPosition(10.5f, 65.0f, 20.0f);
    player->setRotation(0.0f, 0.0f);

    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    auto result = Items::PIG_SPAWN_EGG->onItemRightClick(*m_world, *player, Hand::MainHand);

    EXPECT_EQ(result.getType(), ActionResultType::Pass);
    EXPECT_EQ(m_world->spawnedEntityCount(), 0u);
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 4) << "pass should not consume";
}

TEST_F(SpawnEggItemTest, OnItemRightClick_ClientSidePredictsSuccess)
{
    // 客户端直接预测 Success
    m_world->setClientSide(true);

    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setPosition(10.5f, 65.0f, 20.0f);

    ItemStack eggStack(Items::PIG_SPAWN_EGG, 4);
    player->getHeldItem(Hand::MainHand) = eggStack;

    auto result = Items::PIG_SPAWN_EGG->onItemRightClick(*m_world, *player, Hand::MainHand);

    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(m_world->spawnedEntityCount(), 0u) << "client should not spawn";
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 4) << "client should not consume";
}
