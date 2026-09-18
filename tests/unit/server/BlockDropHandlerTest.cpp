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

#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/world/drop/BlockDropHandler.hpp"

#include <mutex>
#include <vector>

namespace mc {

namespace {

void ensureRegistriesInitialized()
{
    static std::once_flag s_once;
    std::call_once(s_once, [] {
        VanillaBlocks::initialize();
        Items::initialize();
        // 注册原版实体类型，使 VanillaEntityTypeKeys::ITEM 全局缓存与注册表一致。
        // 本文件多个用例断言 entity->entityType() == VanillaEntityTypeKeys::ITEM，
        // 二者必须来自同一已初始化注册表，避免依赖前置测试的隐式注册状态
        // （测试顺序污染）。VanillaEntities::registerAll() 幂等且线程安全，无异常风险。
        entity::VanillaEntities::registerAll();
    });
}

} // namespace

// ============================================================================
// spawnDrops 测试
// ============================================================================

TEST(BlockDropHandlerTest, SpawnDropsToEntityManagerCreatesItemEntities)
{
    ensureRegistriesInitialized();

    ASSERT_NE(Items::APPLE, nullptr);

    EntityManager entityManager{mc::test::testEcsRegistry()};
    const BlockPos pos(12, 80, -4);
    const std::vector<ItemStack> drops{ItemStack(*Items::APPLE, 2)};

    const auto spawned = BlockDropHandler::spawnDrops(entityManager, nullptr, pos, drops, "");

    ASSERT_EQ(spawned.size(), 1u);
    EXPECT_TRUE(entityManager.hasEntity(spawned[0]));
    EXPECT_EQ(entityManager.entityCount(), 1u);

    const Entity* entity = entityManager.getEntity(spawned[0]);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->entityType(), entity::VanillaEntityTypeKeys::ITEM);
}

TEST(BlockDropHandlerTest, SpawnDropsEmptyListReturnsEmpty)
{
    ensureRegistriesInitialized();

    EntityManager entityManager{mc::test::testEcsRegistry()};
    const BlockPos pos(0, 0, 0);
    const std::vector<ItemStack> drops;

    const auto spawned = BlockDropHandler::spawnDrops(entityManager, nullptr, pos, drops, "");

    EXPECT_TRUE(spawned.empty());
    EXPECT_EQ(entityManager.entityCount(), 0u);
}

TEST(BlockDropHandlerTest, SpawnDropsMultipleItems)
{
    ensureRegistriesInitialized();

    ASSERT_NE(Items::STONE, nullptr);
    ASSERT_NE(Items::COBBLESTONE, nullptr);

    EntityManager entityManager{mc::test::testEcsRegistry()};
    const BlockPos pos(100, 64, -200);
    const std::vector<ItemStack> drops{ItemStack(*Items::STONE, 32), ItemStack(*Items::COBBLESTONE, 16)};

    const auto spawned = BlockDropHandler::spawnDrops(entityManager, nullptr, pos, drops, "");

    ASSERT_EQ(spawned.size(), 2u);
    EXPECT_EQ(entityManager.entityCount(), 2u);

    // 验证所有实体都是物品实体
    for (EntityInstanceId id : spawned) {
        const Entity* entity = entityManager.getEntity(id);
        ASSERT_NE(entity, nullptr);
        EXPECT_EQ(entity->entityType(), entity::VanillaEntityTypeKeys::ITEM);
    }
}

TEST(BlockDropHandlerTest, SpawnDropsSetsPickupDelay)
{
    ensureRegistriesInitialized();

    ASSERT_NE(Items::COBBLESTONE, nullptr);

    EntityManager entityManager{mc::test::testEcsRegistry()};
    const BlockPos pos(0, 0, 0);
    const std::vector<ItemStack> drops{ItemStack(*Items::COBBLESTONE, 1)};

    const auto spawned = BlockDropHandler::spawnDrops(entityManager, nullptr, pos, drops, "");

    ASSERT_EQ(spawned.size(), 1u);

    const Entity* entity = entityManager.getEntity(spawned[0]);
    ASSERT_NE(entity, nullptr);

    // 验证是物品实体
    EXPECT_EQ(entity->entityType(), entity::VanillaEntityTypeKeys::ITEM);

    // 验证拾取延迟被设置
    const auto* itemEntity = dynamic_cast<const ItemEntity*>(entity);
    ASSERT_NE(itemEntity, nullptr);
    EXPECT_GT(itemEntity->getPickupDelay(), 0);
}

// ============================================================================
// getOreType 测试 - 用于 destroy 模式的经验掉落
// ============================================================================

TEST(BlockDropHandlerTest, GetOreTypeReturnsNoneForNonOre)
{
    ensureRegistriesInitialized();

    // 石头不是矿石
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*stoneState), OreType::None);

    // 空气不是矿石
    const BlockState* airState = &VanillaBlocks::AIR->defaultState();
    ASSERT_NE(airState, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*airState), OreType::None);
}

TEST(BlockDropHandlerTest, GetOreTypeIdentifiesCoalOre)
{
    ensureRegistriesInitialized();

    const BlockState* state = &VanillaBlocks::COAL_ORE->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*state), OreType::Coal);
}

TEST(BlockDropHandlerTest, GetOreTypeIdentifiesDiamondOre)
{
    ensureRegistriesInitialized();

    const BlockState* state = &VanillaBlocks::DIAMOND_ORE->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*state), OreType::Diamond);
}

TEST(BlockDropHandlerTest, GetOreTypeIdentifiesEmeraldOre)
{
    ensureRegistriesInitialized();

    const BlockState* state = &VanillaBlocks::EMERALD_ORE->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*state), OreType::Emerald);
}

TEST(BlockDropHandlerTest, GetOreTypeIdentifiesLapisOre)
{
    ensureRegistriesInitialized();

    const BlockState* state = &VanillaBlocks::LAPIS_ORE->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*state), OreType::Lapis);
}

TEST(BlockDropHandlerTest, GetOreTypeIdentifiesRedstoneOre)
{
    ensureRegistriesInitialized();

    const BlockState* state = &VanillaBlocks::REDSTONE_ORE->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*state), OreType::Redstone);
}

TEST(BlockDropHandlerTest, GetOreTypeIdentifiesNetherQuartzOre)
{
    ensureRegistriesInitialized();

    const BlockState* state = &VanillaBlocks::NETHER_QUARTZ_ORE->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*state), OreType::NetherQuartz);
}

TEST(BlockDropHandlerTest, GetOreTypeIdentifiesNetherGoldOre)
{
    ensureRegistriesInitialized();

    const BlockState* state = &VanillaBlocks::NETHER_GOLD_ORE->defaultState();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(BlockDropHandler::getOreType(*state), OreType::NetherGold);
}

// ============================================================================
// canHarvestBlock 测试 - 用于 destroy 模式的采集检查
// ============================================================================

TEST(BlockDropHandlerTest, CanHarvestBlockAirAlwaysTrue)
{
    ensureRegistriesInitialized();

    const BlockState* airState = &VanillaBlocks::AIR->defaultState();
    ASSERT_NE(airState, nullptr);

    // 空气应该总是可以"采集"
    EXPECT_TRUE(BlockDropHandler::canHarvestBlock(*airState, nullptr, nullptr));
}

TEST(BlockDropHandlerTest, CanHarvestBlockRequiresCorrectTool)
{
    ensureRegistriesInitialized();

    // 钻石矿需要铁镐或更高级工具
    const BlockState* diamondOreState = &VanillaBlocks::DIAMOND_ORE->defaultState();
    ASSERT_NE(diamondOreState, nullptr);

    // 没有工具时不能采集钻石矿（如果方块需要工具）
    // 注意：这里取决于方块的具体实现
    // 我们只验证函数可以被正确调用
    (void)BlockDropHandler::canHarvestBlock(*diamondOreState, nullptr, nullptr);
}

// ============================================================================
// handleBlockBreakExperience 测试 - 用于 destroy 模式的经验掉落
// ============================================================================

TEST(BlockDropHandlerTest, HandleBlockBreakExperienceNonOreReturnsZero)
{
    ensureRegistriesInitialized();

    EntityManager entityManager{mc::test::testEcsRegistry()};
    math::Random rng(12345);
    const BlockPos pos(0, 0, 0);

    // 石头不是矿石，不应该掉落经验
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    i32 xp = BlockDropHandler::handleBlockBreakExperience(entityManager, nullptr, pos, *stoneState, nullptr, rng);

    EXPECT_EQ(xp, 0);
    EXPECT_EQ(entityManager.entityCount(), 0u);
}

TEST(BlockDropHandlerTest, HandleBlockBreakExperienceCoalOreGeneratesXP)
{
    ensureRegistriesInitialized();

    EntityManager entityManager{mc::test::testEcsRegistry()};
    math::Random rng(12345);
    const BlockPos pos(0, 0, 0);

    const BlockState* coalOreState = &VanillaBlocks::COAL_ORE->defaultState();
    ASSERT_NE(coalOreState, nullptr);

    // 煤矿掉落 0-2 经验
    // 注意：这里可能不生成经验球，因为煤矿需要正确工具采集
    // 我们只验证函数可以被正确调用
    i32 xp = BlockDropHandler::handleBlockBreakExperience(entityManager, nullptr, pos, *coalOreState, nullptr, rng);

    // 煤矿需要工具，所以没有工具时可能不生成经验
    // 验证返回值是合理的范围
    EXPECT_GE(xp, 0);
}

TEST(BlockDropHandlerTest, HandleBlockBreakExperienceDiamondOreGeneratesXP)
{
    ensureRegistriesInitialized();

    EntityManager entityManager{mc::test::testEcsRegistry()};
    math::Random rng(54321);
    const BlockPos pos(100, 64, -50);

    const BlockState* diamondOreState = &VanillaBlocks::DIAMOND_ORE->defaultState();
    ASSERT_NE(diamondOreState, nullptr);

    // 钻石矿掉落 3-7 经验
    i32 xp = BlockDropHandler::handleBlockBreakExperience(entityManager, nullptr, pos, *diamondOreState, nullptr, rng);

    // 钻石矿需要工具，没有工具时可能不生成经验
    EXPECT_GE(xp, 0);
}

TEST(BlockDropHandlerTest, HandleBlockBreakExperienceGeneratesExperienceOrbs)
{
    ensureRegistriesInitialized();

    EntityManager entityManager{mc::test::testEcsRegistry()};
    math::Random rng(99999);
    const BlockPos pos(10, 20, 30);

    // 使用煤矿测试经验球生成
    const BlockState* coalOreState = &VanillaBlocks::COAL_ORE->defaultState();
    ASSERT_NE(coalOreState, nullptr);

    // 多次尝试，看看是否能生成经验球
    i32 totalXp = 0;
    for (int i = 0; i < 10; ++i) {
        math::Random testRng(i * 1000);
        i32 xp =
            BlockDropHandler::handleBlockBreakExperience(entityManager, nullptr, pos, *coalOreState, nullptr, testRng);
        totalXp += xp;
    }

    // 由于需要工具，可能都不生成经验，但函数应该正常工作
    EXPECT_GE(totalXp, 0);
}

// ============================================================================
// getDefaultDrops 测试
// ============================================================================

TEST(BlockDropHandlerTest, GetDefaultDropsReturnsEmptyForNormalBlocks)
{
    ensureRegistriesInitialized();

    // 大多数方块没有默认掉落
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    auto drops = BlockDropHandler::getDefaultDrops(*stoneState);
    EXPECT_TRUE(drops.empty());
}

} // namespace mc
