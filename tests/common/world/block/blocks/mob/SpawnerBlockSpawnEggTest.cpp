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
 * LIABILITY,WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file SpawnerBlockSpawnEggTest.cpp
 * @brief SpawnerBlock + SpawnEggItem 交互测试
 *
 * 测试刷怪蛋与刷怪笼方块的交互：
 * - 持有刷怪蛋右键刷怪笼时设置实体类型
 * - 非刷怪蛋物品右键刷怪笼返回 Pass
 * - 空手右键刷怪笼返回 Pass
 * - 创造模式下不消耗刷怪蛋
 * - 生存模式下消耗刷怪蛋
 * - 客户端返回 Success
 * - SpawnEggItem::onItemUse 在点击刷怪笼时设置实体类型
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/SpawnEggItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/mob/SpawnerBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace blocks {
namespace test {

// ============================================================================
// 测试用 Mock World
// ============================================================================

class SpawnerEggTestWorld final : public ::mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    void setSeed(u64 seed) { m_seed = seed; }
    [[nodiscard]] u64 seed() const override { return m_seed; }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] math::Random& getRandom() override { return m_rng; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_rng; }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }
    void setClientSide(bool clientSide) { m_clientSide = clientSide; }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void addBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity)
    {
        m_blockEntities[pos] = std::move(entity);
    }

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

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SpawnerEggTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SpawnerEggTestWorld::tickManager not implemented");
    }

private:
    u64 m_seed = 12345;
    u64 m_currentTick = 0;
    Difficulty m_difficulty = Difficulty::Easy;
    bool m_clientSide = false;
    math::Random m_rng{12345};
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
};

// ============================================================================
// 辅助函数：创建测试用的 SpawnEggItem
// ============================================================================

/**
 * @brief 创建一个测试用 SpawnEggItem
 *
 * EntityType 的 m_name 会被 const_cast 设置为指定名称，
 * 以模拟注册表中已注册的实体类型。
 */
std::unique_ptr<item::SpawnEggItem> makeSpawnEgg(const std::string& entityName, u32 primaryColor, u32 secondaryColor)
{
    auto entityType = entity::EntityType::Builder(
        [](IWorld*, ecs::EntityRegistry& registry) -> std::unique_ptr<Entity> { return nullptr; }, entity::EntityClassification::Creature)
                          .size(0.9f, 0.9f)
                          .trackingRange(10)
                          .updateInterval(3)
                          .canSummon(true)
                          .build();

    // 手动设置实体类型名称（模拟注册表的 const_cast 行为）
    const_cast<std::string&>(entityType.name()) = entityName;

    return std::make_unique<item::SpawnEggItem>(
        std::move(entityType), primaryColor, secondaryColor, ItemProperties().maxStackSize(64));
}

// ============================================================================
// SpawnerBlock + SpawnEgg 交互测试
// ============================================================================

class SpawnerBlockSpawnEggTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        Items::initialize();

        // 创建测试用刷怪蛋
        pigEgg_ = makeSpawnEgg("minecraft:pig", 0xF0A0A0, 0xA05050);
        zombieEgg_ = makeSpawnEgg("minecraft:zombie", 0x00A800, 0x7A0000);
        skeletonEgg_ = makeSpawnEgg("minecraft:skeleton", 0xC1C1C1, 0x494949);

        // 创建刷怪笼方块
        spawnerBlock_ =
            std::make_unique<SpawnerBlock>(BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f));
        spawnerState_ = &spawnerBlock_->defaultState();
    }

    void TearDown() override
    {
        // 注册表清理由各自的单例自动管理
    }

    /**
     * @brief 创建一个放置了刷怪笼的测试世界
     */
    void setupSpawnerWorld(const BlockPos& spawnerPos)
    {
        world_.setBlockState(spawnerPos.x, spawnerPos.y, spawnerPos.z, spawnerState_);

        auto spawnerEntity = std::make_unique<blockentity::MobSpawnerBlockEntity>(spawnerPos);
        spawnerEntity_ = spawnerEntity.get();
        world_.addBlockEntity(spawnerPos, std::move(spawnerEntity));

        spawnerPos_ = spawnerPos;
    }

    SpawnerEggTestWorld world_;
    std::unique_ptr<item::SpawnEggItem> pigEgg_;
    std::unique_ptr<item::SpawnEggItem> zombieEgg_;
    std::unique_ptr<item::SpawnEggItem> skeletonEgg_;
    std::unique_ptr<SpawnerBlock> spawnerBlock_;
    const BlockState* spawnerState_ = nullptr;
    blockentity::MobSpawnerBlockEntity* spawnerEntity_ = nullptr;
    BlockPos spawnerPos_{10, 64, 20};
};

// ============================================================================
// SpawnerBlock::onBlockActivated 测试
// ============================================================================

TEST_F(SpawnerBlockSpawnEggTest, OnBlockActivated_SpawnEggSetsEntityId)
{
    setupSpawnerWorld({10, 64, 20});

    ItemStack eggStack(pigEgg_.get(), 1);

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);
    player.getHeldItem(Hand::MainHand) = eggStack;

    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), spawnerPos_, Direction::Up, 0.0f);
    auto result = spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Consume);
    EXPECT_EQ(spawnerEntity_->getNextEntityId(), ResourceLocation("minecraft:pig"));
}

TEST_F(SpawnerBlockSpawnEggTest, OnBlockActivated_EmptyHandReturnsPass)
{
    setupSpawnerWorld({10, 64, 20});

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);

    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), spawnerPos_, Direction::Up, 0.0f);
    auto result = spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(spawnerEntity_->getNextEntityId(), ResourceLocation());
}

TEST_F(SpawnerBlockSpawnEggTest, OnBlockActivated_NonSpawnEggItemReturnsPass)
{
    setupSpawnerWorld({10, 64, 20});

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);

    if (Items::STICK != nullptr) {
        ItemStack stickStack(Items::STICK, 1);
        player.getHeldItem(Hand::MainHand) = stickStack;

        BlockRaycastResult hit =
            BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), spawnerPos_, Direction::Up, 0.0f);
        auto result = spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

        EXPECT_EQ(result, ActionResultType::Pass);
    }
}

TEST_F(SpawnerBlockSpawnEggTest, OnBlockActivated_CreativeModeDoesNotConsumeEgg)
{
    setupSpawnerWorld({10, 64, 20});

    ItemStack eggStack(pigEgg_.get(), 5);

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);
    player.setGameMode(GameMode::Creative);
    player.getHeldItem(Hand::MainHand) = eggStack;

    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), spawnerPos_, Direction::Up, 0.0f);
    spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 5);
}

TEST_F(SpawnerBlockSpawnEggTest, OnBlockActivated_SurvivalModeConsumesEgg)
{
    setupSpawnerWorld({10, 64, 20});

    ItemStack eggStack(pigEgg_.get(), 5);

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);
    player.setGameMode(GameMode::Survival);
    player.getHeldItem(Hand::MainHand) = eggStack;

    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), spawnerPos_, Direction::Up, 0.0f);
    auto result = spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Consume);
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 4);
}

TEST_F(SpawnerBlockSpawnEggTest, OnBlockActivated_ClientSideReturnsSuccess)
{
    setupSpawnerWorld({10, 64, 20});

    ItemStack eggStack(pigEgg_.get(), 5);

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);
    player.getHeldItem(Hand::MainHand) = eggStack;

    world_.setClientSide(true);

    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), spawnerPos_, Direction::Up, 0.0f);
    auto result = spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(spawnerEntity_->getNextEntityId(), ResourceLocation());
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 5);
}

TEST_F(SpawnerBlockSpawnEggTest, OnBlockActivated_NoBlockEntityReturnsPass)
{
    // 只放置方块状态，不添加方块实体
    world_.setBlockState(10, 64, 20, spawnerState_);
    spawnerPos_ = BlockPos(10, 64, 20);

    ItemStack eggStack(pigEgg_.get(), 1);

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);
    player.getHeldItem(Hand::MainHand) = eggStack;

    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), spawnerPos_, Direction::Up, 0.0f);
    auto result = spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Pass);
}

TEST_F(SpawnerBlockSpawnEggTest, OnBlockActivated_DifferentEntityTypes)
{
    setupSpawnerWorld({10, 64, 20});

    // 先使用僵尸刷怪蛋
    ItemStack zombieStack(zombieEgg_.get(), 2);
    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);
    player.getHeldItem(Hand::MainHand) = zombieStack;

    BlockRaycastResult hit = BlockRaycastResult::hit(Vector3(10.5f, 64.5f, 20.5f), spawnerPos_, Direction::Up, 0.0f);
    auto result = spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Consume);
    EXPECT_EQ(spawnerEntity_->getNextEntityId(), ResourceLocation("minecraft:zombie"));

    // 再使用骷髅刷怪蛋覆盖
    ItemStack skeletonStack(skeletonEgg_.get(), 2);
    player.getHeldItem(Hand::MainHand) = skeletonStack;

    result = spawnerBlock_->onBlockActivated(*spawnerState_, world_, spawnerPos_, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Consume);
    EXPECT_EQ(spawnerEntity_->getNextEntityId(), ResourceLocation("minecraft:skeleton"));
}

// ============================================================================
// SpawnEggItem::onItemUse 刷怪笼分支测试
// ============================================================================

TEST_F(SpawnerBlockSpawnEggTest, OnItemUse_SpawnerBlockSetsEntityId)
{
    setupSpawnerWorld({10, 64, 20});

    ItemStack eggStack(pigEgg_.get(), 5);

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);
    player.getHeldItem(Hand::MainHand) = eggStack;

    ItemUseContext context(world_,
        &player,
        player.getHeldItem(Hand::MainHand),
        Vector3(10.5f, 65.0f, 20.5f),
        spawnerPos_,
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = pigEgg_->onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(spawnerEntity_->getNextEntityId(), ResourceLocation("minecraft:pig"));
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 4);
}

TEST_F(SpawnerBlockSpawnEggTest, OnItemUse_SpawnerBlockCreativeModeNoConsume)
{
    setupSpawnerWorld({10, 64, 20});

    ItemStack eggStack(pigEgg_.get(), 5);

    Player player(PlayerId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setWorld(&world_);
    player.setGameMode(GameMode::Creative);
    player.getHeldItem(Hand::MainHand) = eggStack;

    ItemUseContext context(world_,
        &player,
        player.getHeldItem(Hand::MainHand),
        Vector3(10.5f, 65.0f, 20.5f),
        spawnerPos_,
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    ActionResultType result = pigEgg_->onItemUse(context);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_EQ(spawnerEntity_->getNextEntityId(), ResourceLocation("minecraft:pig"));
    EXPECT_EQ(player.getHeldItem(Hand::MainHand).getCount(), 5);
}

} // namespace test
} // namespace blocks
} // namespace mc
