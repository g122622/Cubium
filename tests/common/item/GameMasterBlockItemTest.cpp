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

#include "common/item/items/block/GameMasterBlockItem.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/GameMasterBlock.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/special/BarrierBlock.hpp"
#include "common/world/block/blocks/special/CommandBlock.hpp"
#include "common/world/block/blocks/special/JigsawBlock.hpp"
#include "common/world/block/blocks/special/StructureBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "physics/collision/CollisionShape.hpp"
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== 测试辅助：支持方块读写的 IWorld 实现 ==========

class GameMasterTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(key(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[key(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("GameMasterTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("GameMasterTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    static i64 key(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 40) ^ (static_cast<i64>(y) << 20) ^ static_cast<i64>(z & 0xFFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
};

// ========== 测试用方块 ==========

// 测试用管理员方块，用于测试 GameMasterBlockItem
class TestGameMasterBlock : public Block, public GameMasterBlock {
public:
    TestGameMasterBlock()
        : Block(BlockProperties(Material::ROCK).hardness(-1.0f))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, auto values, auto layouts, auto allStates, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), layouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] bool isGameMaster() const noexcept override { return true; }

    void fillStateContainer(StateContainer<Block, BlockState>& /*container*/) override {}
};

// 普通方块（非 GameMaster）
class TestNormalBlock : public Block {
public:
    TestNormalBlock()
        : Block(BlockProperties(Material::ROCK).hardness(1.0f))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, auto values, auto layouts, auto allStates, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), layouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    void fillStateContainer(StateContainer<Block, BlockState>& /*container*/) override {}
};

// ========== Block::isGameMaster 测试 ==========

TEST(GameMasterBlockTest, GameMasterBlockReturnsTrue)
{
    TestGameMasterBlock block;
    EXPECT_TRUE(block.isGameMaster());
}

TEST(GameMasterBlockTest, NormalBlockReturnsFalse)
{
    TestNormalBlock block;
    EXPECT_FALSE(block.isGameMaster());
}

TEST(GameMasterBlockTest, GameMasterBlockIsGameMasterBlockInterface)
{
    TestGameMasterBlock block;
    auto* gameMasterBlock = dynamic_cast<const GameMasterBlock*>(&block);
    EXPECT_NE(gameMasterBlock, nullptr);
}

TEST(GameMasterBlockTest, NormalBlockIsNotGameMasterBlockInterface)
{
    TestNormalBlock block;
    auto* gameMasterBlock = dynamic_cast<const GameMasterBlock*>(&block);
    EXPECT_EQ(gameMasterBlock, nullptr);
}

// ========== 特殊方块 isGameMaster 测试 ==========

TEST(GameMasterBlockTest, CommandBlockIsGameMaster)
{
    CommandBlock block(BlockProperties(Material::ROCK).hardness(-1.0f));
    EXPECT_TRUE(block.isGameMaster());
}

TEST(GameMasterBlockTest, StructureBlockIsGameMaster)
{
    StructureBlock block(BlockProperties(Material::ROCK).hardness(-1.0f).noLootTable());
    EXPECT_TRUE(block.isGameMaster());
}

TEST(GameMasterBlockTest, JigsawBlockIsGameMaster)
{
    JigsawBlock block(BlockProperties(Material::ROCK).hardness(-1.0f).noLootTable());
    EXPECT_TRUE(block.isGameMaster());
}

TEST(GameMasterBlockTest, BarrierBlockIsNotGameMaster)
{
    // BarrierBlock 不实现 GameMasterBlock 接口
    // 它通过 hardness=-1 防止破坏，而非通过权限检查
    BarrierBlock block(BlockProperties(Material::BARRIER).hardness(-1.0f).resistance(3600000.0f).noLootTable());
    EXPECT_FALSE(block.isGameMaster());
}

// ========== GameMasterBlockItem 类层次结构测试 ==========

TEST(GameMasterBlockItemTest, InheritsFromBlockItem)
{
    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));

    // GameMasterBlockItem 应该是 BlockItem 的子类
    auto* blockItem = dynamic_cast<const BlockItem*>(&item);
    EXPECT_NE(blockItem, nullptr);
}

TEST(GameMasterBlockItemTest, ReturnsAssociatedBlock)
{
    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));

    EXPECT_EQ(&item.block(), &gameMasterBlock);
}

TEST(GameMasterBlockItemTest, MaxStackSizeFromProperties)
{
    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(1));

    EXPECT_EQ(item.maxStackSize(), 1);
}

TEST(GameMasterBlockItemTest, GameMasterBlockItemCreation)
{
    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));

    // 验证基本属性
    EXPECT_EQ(&item.block(), &gameMasterBlock);
    EXPECT_EQ(item.maxStackSize(), 64);
}

// ========== GameMasterBlockItem::getStateForPlacement 权限检查测试 ==========

class GameMasterBlockItemPlacementTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }

    void TearDown() override
    {
        // 清理注册表状态
        BlockItemRegistry::instance().clear();
    }
};

TEST_F(GameMasterBlockItemPlacementTest, AllowsPlacementWhenPlayerIsNull)
{
    // 玩家为 nullptr（如发射器放置），应允许放置
    GameMasterTestWorld world;
    // 设置地面方块
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, nullptr, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, &gameMasterBlock.defaultState());
}

TEST_F(GameMasterBlockItemPlacementTest, AllowsPlacementWhenPlayerHasPermission)
{
    // 创造模式 + OP等级>=2 → 应允许放置
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(2);

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, &gameMasterBlock.defaultState());
}

TEST_F(GameMasterBlockItemPlacementTest, AllowsPlacementWhenPlayerHasOwnerPermission)
{
    // 创造模式 + OP等级4 → 应允许放置
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(4);

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, &gameMasterBlock.defaultState());
}

TEST_F(GameMasterBlockItemPlacementTest, DeniesPlacementWhenSurvivalModeWithPermission)
{
    // 生存模式 + OP等级2 → 不能放置（需要创造模式）
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Survival);
    player.setPermissionLevel(2);

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(GameMasterBlockItemPlacementTest, DeniesPlacementWhenCreativeModeWithoutPermission)
{
    // 创造模式 + OP等级0 → 不能放置（需要OP等级>=2）
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(0);

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(GameMasterBlockItemPlacementTest, DeniesPlacementWhenCreativeModeWithModeratorPermission)
{
    // 创造模式 + OP等级1（版主）→ 不能放置（需要OP等级>=2）
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(1);

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(GameMasterBlockItemPlacementTest, DeniesPlacementWhenAdventureModeWithPermission)
{
    // 冒险模式 + OP等级2 → 不能放置
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Adventure);
    player.setPermissionLevel(2);

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(GameMasterBlockItemPlacementTest, DeniesPlacementWhenSpectatorModeWithPermission)
{
    // 旁观者模式 + OP等级2 → 不能放置
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Spectator);
    player.setPermissionLevel(2);

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(GameMasterBlockItemPlacementTest, DeniesPlacementWhenDefaultPlayer)
{
    // 默认玩家（生存模式 + OP等级0）→ 不能放置
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    // 默认：生存模式，OP等级0

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    EXPECT_EQ(result, nullptr);
}

// ========== 参数化测试：GameMasterBlockItem 放置权限组合 ==========

class GameMasterBlockItemPermissionComboTest : public ::testing::TestWithParam<std::tuple<GameMode, i32, bool>> {};

TEST_P(GameMasterBlockItemPermissionComboTest, ParameterizedPlacementCheck)
{
    auto [gameMode, permLevel, expectedAllowed] = GetParam();

    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(gameMode);
    player.setPermissionLevel(permLevel);

    TestGameMasterBlock gameMasterBlock;
    GameMasterBlockItem item(gameMasterBlock, ItemProperties().maxStackSize(64));
    ItemStack stack(item, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    const BlockState* result = item.getStateForPlacement(context);
    if (expectedAllowed) {
        ASSERT_NE(result, nullptr) << "gameMode=" << static_cast<int>(gameMode) << " permLevel=" << permLevel;
        EXPECT_EQ(result, &gameMasterBlock.defaultState());
    } else {
        EXPECT_EQ(result, nullptr) << "gameMode=" << static_cast<int>(gameMode) << " permLevel=" << permLevel;
    }
}

INSTANTIATE_TEST_SUITE_P(GameMasterBlockItemPlacementPermissions,
    GameMasterBlockItemPermissionComboTest,
    ::testing::Values(
        // Survival + 任何权限等级 = 不允许
        std::make_tuple(GameMode::Survival, 0, false),
        std::make_tuple(GameMode::Survival, 1, false),
        std::make_tuple(GameMode::Survival, 2, false),
        std::make_tuple(GameMode::Survival, 3, false),
        std::make_tuple(GameMode::Survival, 4, false),
        // Creative + < 2 = 不允许
        std::make_tuple(GameMode::Creative, 0, false),
        std::make_tuple(GameMode::Creative, 1, false),
        // Creative + >= 2 = 允许
        std::make_tuple(GameMode::Creative, 2, true),
        std::make_tuple(GameMode::Creative, 3, true),
        std::make_tuple(GameMode::Creative, 4, true),
        // Adventure = 始终不允许
        std::make_tuple(GameMode::Adventure, 0, false),
        std::make_tuple(GameMode::Adventure, 2, false),
        std::make_tuple(GameMode::Adventure, 4, false),
        // Spectator = 始终不允许
        std::make_tuple(GameMode::Spectator, 0, false),
        std::make_tuple(GameMode::Spectator, 2, false),
        std::make_tuple(GameMode::Spectator, 4, false)));

// ========== BlockItem 基类不受 GameMaster 权限限制 ==========

TEST_F(GameMasterBlockItemPlacementTest, NormalBlockItemAlwaysAllowsPlacement)
{
    // 普通 BlockItem 不受 GameMaster 权限限制
    GameMasterTestWorld world;
    world.setBlockState(0, 63, 0, &VanillaBlocks::STONE->defaultState());

    // 生存模式、无OP权限的玩家
    Player player(EntityInstanceId(1), "TestPlayer");
    // 默认：生存模式，OP等级0

    const BlockItem* stoneItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STONE->blockId());
    ASSERT_NE(stoneItem, nullptr);
    ItemStack stack(*stoneItem, 1);

    BlockItemUseContext context(
        world, &player, stack, Vector3(0.5f, 63.99f, 0.5f), BlockPos(0, 63, 0), Direction::Up, 0.0f, 0.0f);

    // 普通 BlockItem 不检查权限，应正常返回
    const BlockState* result = stoneItem->getStateForPlacement(context);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result, &VanillaBlocks::STONE->defaultState());
}

// ========== 原版方块 isGameMaster 注册验证 ==========

TEST_F(GameMasterBlockItemPlacementTest, VanillaStructureBlockIsGameMaster)
{
    // 验证原版结构方块通过 VanillaBlocks 注册后 isGameMaster() 返回 true
    ASSERT_NE(VanillaBlocks::STRUCTURE_BLOCK, nullptr);
    EXPECT_TRUE(VanillaBlocks::STRUCTURE_BLOCK->isGameMaster());
}

TEST_F(GameMasterBlockItemPlacementTest, VanillaJigsawBlockIsGameMaster)
{
    // 验证原版拼图方块通过 VanillaBlocks 注册后 isGameMaster() 返回 true
    ASSERT_NE(VanillaBlocks::JIGSAW, nullptr);
    EXPECT_TRUE(VanillaBlocks::JIGSAW->isGameMaster());
}

TEST_F(GameMasterBlockItemPlacementTest, VanillaBarrierBlockIsNotGameMaster)
{
    // 验证原版屏障方块 isGameMaster() 返回 false
    ASSERT_NE(VanillaBlocks::BARRIER, nullptr);
    EXPECT_FALSE(VanillaBlocks::BARRIER->isGameMaster());
}

TEST_F(GameMasterBlockItemPlacementTest, VanillaStoneBlockIsNotGameMaster)
{
    // 验证普通方块 isGameMaster() 返回 false
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_FALSE(VanillaBlocks::STONE->isGameMaster());
}

// ========== GameMasterBlockItem 注册验证 ==========

TEST_F(GameMasterBlockItemPlacementTest, StructureBlockUsesGameMasterBlockItem)
{
    // 结构方块应使用 GameMasterBlockItem 注册
    ASSERT_NE(VanillaBlocks::STRUCTURE_BLOCK, nullptr);
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STRUCTURE_BLOCK->blockId());
    ASSERT_NE(item, nullptr);

    // GameMasterBlockItem 应该可以通过 dynamic_cast 识别
    const auto* gameMasterItem = dynamic_cast<const GameMasterBlockItem*>(item);
    EXPECT_NE(gameMasterItem, nullptr);
}

TEST_F(GameMasterBlockItemPlacementTest, JigsawBlockUsesGameMasterBlockItem)
{
    // 拼图方块应使用 GameMasterBlockItem 注册
    ASSERT_NE(VanillaBlocks::JIGSAW, nullptr);
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::JIGSAW->blockId());
    ASSERT_NE(item, nullptr);

    const auto* gameMasterItem = dynamic_cast<const GameMasterBlockItem*>(item);
    EXPECT_NE(gameMasterItem, nullptr);
}

TEST_F(GameMasterBlockItemPlacementTest, BarrierBlockUsesRegularBlockItem)
{
    // 屏障方块应使用普通 BlockItem 注册（不是 GameMasterBlockItem）
    ASSERT_NE(VanillaBlocks::BARRIER, nullptr);
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BARRIER->blockId());
    ASSERT_NE(item, nullptr);

    // 不应该是 GameMasterBlockItem
    const auto* gameMasterItem = dynamic_cast<const GameMasterBlockItem*>(item);
    EXPECT_EQ(gameMasterItem, nullptr);
}

TEST_F(GameMasterBlockItemPlacementTest, StoneBlockUsesRegularBlockItem)
{
    // 石头方块应使用普通 BlockItem 注册
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STONE->blockId());
    ASSERT_NE(item, nullptr);

    // 不应该是 GameMasterBlockItem
    const auto* gameMasterItem = dynamic_cast<const GameMasterBlockItem*>(item);
    EXPECT_EQ(gameMasterItem, nullptr);
}
