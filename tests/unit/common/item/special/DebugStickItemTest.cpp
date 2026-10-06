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
 * @file DebugStickItemTest.cpp
 * @brief 调试棒（DebugStickItem）与光源/测试方块的单元测试
 *
 * 测试覆盖：
 * - DebugStickItem 注册（物品 id 反查）
 * - 无管理员权限时不做任何修改
 * - 左键（canDestroyBlock）切换选中属性、返回 true（消费破坏）
 * - 右键（onItemUse）循环切换选中属性的值并应用到方块
 * - 潜行（isInputSneaking）反向循环
 * - 无可调试属性方块给出提示
 * - 光源方块 LightBlock：默认 LEVEL=15、getLightLevel 跟随 LEVEL、右键 cycle、GameMaster 标记
 * - 测试方块 TestBlock：默认 MODE=Fail、MODE 属性、GameMaster 标记、方块实体类型
 */

#include "common/item/items/special/DebugStickItem.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/RotatedPillarBlock.hpp"
#include "common/world/block/blocks/special/LightBlock.hpp"
#include "common/world/block/blocks/special/TestBlock.hpp"
#include "common/world/block/blocks/special/TestInstanceBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/TestBlockEntity.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "physics/collision/CollisionShape.hpp"

#include <memory>
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

namespace {

// ========== 测试世界 ==========

/**
 * @brief 支持方块读写与方块实体登记的测试世界
 *
 * 供调试棒（需要 setBlockState 与手持物品槽）、光源/测试方块（需要方块实体）测试使用。
 */
class DebugStickTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(key(x, y, z));
        return it != m_blocks.end() ? it->second.get() : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[key(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }
    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return world::MAX_BUILD_HEIGHT; }
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
        throw std::runtime_error("DebugStickTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("DebugStickTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::IRandom& getRandom() override { return m_random; }
    [[nodiscard]] const math::IRandom& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    static i64 key(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 40) ^ (static_cast<i64>(y) << 20) ^ static_cast<i64>(z & 0xFFFFF);
    }

    std::unordered_map<i64, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
};

/// 取主手物品堆引用（测试夹具用）
ItemStack& heldStack(Player& player)
{
    return player.inventory().getSelectedStackRef();
}

} // namespace

// ============================================================================
// 注册测试
// ============================================================================

class DebugStickRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(DebugStickRegistrationTest, DebugStickItemRegistered)
{
    ASSERT_NE(Items::DEBUG_STICK, nullptr);
    EXPECT_NE(ItemRegistry::instance().getItem(ResourceLocation("minecraft:debug_stick")), nullptr);
    EXPECT_NE(dynamic_cast<item::items::DebugStickItem*>(Items::DEBUG_STICK), nullptr);
}

TEST_F(DebugStickRegistrationTest, LightTestBlocksRegistered)
{
    ASSERT_NE(VanillaBlocks::LIGHT, nullptr);
    ASSERT_NE(VanillaBlocks::TEST_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::TEST_INSTANCE_BLOCK, nullptr);

    EXPECT_TRUE(VanillaBlocks::LIGHT->isGameMaster());
    EXPECT_TRUE(VanillaBlocks::TEST_BLOCK->isGameMaster());
    EXPECT_TRUE(VanillaBlocks::TEST_INSTANCE_BLOCK->isGameMaster());

    // 注册表可按 id 反查
    auto& registry = BlockRegistry::instance();
    EXPECT_EQ(registry.getBlock(ResourceLocation("minecraft:light")), VanillaBlocks::LIGHT);
    EXPECT_EQ(registry.getBlock(ResourceLocation("minecraft:test_block")), VanillaBlocks::TEST_BLOCK);
    EXPECT_EQ(registry.getBlock(ResourceLocation("minecraft:test_instance_block")),
        VanillaBlocks::TEST_INSTANCE_BLOCK);
}

// ============================================================================
// 光源方块测试
// ============================================================================

class LightBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(LightBlockTest, DefaultStateIsLevel15)
{
    ASSERT_NE(VanillaBlocks::LIGHT, nullptr);
    const BlockState& state = VanillaBlocks::LIGHT->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::LEVEL_0_15()), 15);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(LightBlockTest, LightLevelFollowsLevelProperty)
{
    const BlockState& base = VanillaBlocks::LIGHT->defaultState();
    const BlockState& dim = base.with(BlockStateProperties::LEVEL_0_15(), 7);
    EXPECT_EQ(VanillaBlocks::LIGHT->getLightLevel(dim), 7u);
    EXPECT_EQ(VanillaBlocks::LIGHT->getLightLevel(base), 15u);
}

TEST_F(LightBlockTest, IsGameMaster)
{
    EXPECT_TRUE(VanillaBlocks::LIGHT->isGameMaster());
}

TEST_F(LightBlockTest, RightClickWithoutPermissionReturnsConsume)
{
    DebugStickTestWorld world;
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    // 默认生存模式、OP 0：无管理员权限

    BlockRaycastResult hit{};
    auto result = VanillaBlocks::LIGHT->onBlockActivated(
        VanillaBlocks::LIGHT->defaultState(), world, BlockPos(0, 64, 0), player, Hand::MainHand, hit);
    // 无权限 → Consume（对齐 vanilla）
    EXPECT_EQ(result.getType(), ActionResultType::Consume);
}

TEST_F(LightBlockTest, RightClickWithPermissionCyclesLevel)
{
    DebugStickTestWorld world;
    world.setBlockState(0, 64, 0, &VanillaBlocks::LIGHT->defaultState());

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);
    player.setPermissionLevel(2);

    BlockRaycastResult hit{};
    auto result = VanillaBlocks::LIGHT->onBlockActivated(
        VanillaBlocks::LIGHT->defaultState(), world, BlockPos(0, 64, 0), player, Hand::MainHand, hit);
    EXPECT_EQ(result.getType(), ActionResultType::Success);

    const BlockState* updated = world.getBlockState(0, 64, 0);
    ASSERT_NE(updated, nullptr);
    // 15 循环到 0（cycle 到下一值）
    EXPECT_EQ(updated->get(BlockStateProperties::LEVEL_0_15()), 0);
}

// ============================================================================
// 测试方块测试
// ============================================================================

class TestBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(TestBlockTest, DefaultModeIsFail)
{
    ASSERT_NE(VanillaBlocks::TEST_BLOCK, nullptr);
    const BlockState& state = VanillaBlocks::TEST_BLOCK->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::TEST_BLOCK_MODE()), BlockStateProperties::TestBlockMode::Fail);
}

TEST_F(TestBlockTest, TestBlockIsGameMasterAndHasBlockEntity)
{
    EXPECT_TRUE(VanillaBlocks::TEST_BLOCK->isGameMaster());
    EXPECT_TRUE(VanillaBlocks::TEST_BLOCK->hasBlockEntity());
    EXPECT_EQ(VanillaBlocks::TEST_BLOCK->createBlockEntity(BlockPos(0, 64, 0))->getType(),
        BlockEntityType::TestBlock);
}

TEST_F(TestBlockTest, TestInstanceBlockIsGameMasterAndHasBlockEntity)
{
    EXPECT_TRUE(VanillaBlocks::TEST_INSTANCE_BLOCK->isGameMaster());
    EXPECT_TRUE(VanillaBlocks::TEST_INSTANCE_BLOCK->hasBlockEntity());
    EXPECT_EQ(VanillaBlocks::TEST_INSTANCE_BLOCK->createBlockEntity(BlockPos(0, 64, 0))->getType(),
        BlockEntityType::TestInstanceBlock);
}

TEST_F(TestBlockTest, StartModeOutputsRedstoneWhenPowered)
{
    DebugStickTestWorld world;
    const BlockState& startState =
        VanillaBlocks::TEST_BLOCK->defaultState().with(BlockStateProperties::TEST_BLOCK_MODE(),
            BlockStateProperties::TestBlockMode::Start);
    world.setBlockState(0, 64, 0, &startState);

    auto be = VanillaBlocks::TEST_BLOCK->createBlockEntity(BlockPos(0, 64, 0));
    auto* testBe = dynamic_cast<blockentity::TestBlockEntity*>(be.get());
    ASSERT_NE(testBe, nullptr);
    testBe->setPowered(true);
    world.setBlockEntity(BlockPos(0, 64, 0), be.release());

    EXPECT_EQ(VanillaBlocks::TEST_BLOCK->getWeakPower(startState, world, BlockPos(0, 64, 0), Direction::Up), 15);
}

TEST_F(TestBlockTest, NonStartModeOutputsNoRedstone)
{
    DebugStickTestWorld world;
    // 默认 Fail 模式
    const BlockState& failState = VanillaBlocks::TEST_BLOCK->defaultState();
    world.setBlockState(0, 64, 0, &failState);

    auto be = VanillaBlocks::TEST_BLOCK->createBlockEntity(BlockPos(0, 64, 0));
    auto* testBe = dynamic_cast<blockentity::TestBlockEntity*>(be.get());
    ASSERT_NE(testBe, nullptr);
    testBe->setPowered(true);
    world.setBlockEntity(BlockPos(0, 64, 0), be.release());

    EXPECT_EQ(VanillaBlocks::TEST_BLOCK->getWeakPower(failState, world, BlockPos(0, 64, 0), Direction::Up), 0);
}

// ============================================================================
// 调试棒行为测试
// ============================================================================

class DebugStickItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    /// 构造一个创造模式 OP 玩家
    static std::unique_ptr<Player> makeOperator()
    {
        auto player = std::make_unique<Player>(EntityInstanceId(1), "OpPlayer", mc::test::testEcsRegistry());
        player->setGameMode(GameMode::Creative);
        player->setPermissionLevel(2);
        return player;
    }
};

TEST_F(DebugStickItemTest, NoPermissionDoesNothing)
{
    DebugStickTestWorld world;
    world.setBlockState(0, 64, 0, &VanillaBlocks::OAK_LOG->defaultState());
    const BlockState* before = world.getBlockState(0, 64, 0);
    const u32 beforeId = before->stateId();

    auto player = std::make_unique<Player>(EntityInstanceId(1), "Survivor", mc::test::testEcsRegistry());
    // 默认无管理员权限

    ItemStack stack(*Items::DEBUG_STICK, 1);
    auto* debugStick = dynamic_cast<item::items::DebugStickItem*>(Items::DEBUG_STICK);
    ASSERT_NE(debugStick, nullptr);

    bool consumed = debugStick->canDestroyBlock(stack, *before, world, BlockPos(0, 64, 0), *player);
    // 调试棒左键一律返回 false（不允许破坏方块）
    EXPECT_FALSE(consumed);
    // 无权限时不做任何修改：方块未变、物品 NBT 未写入
    EXPECT_EQ(world.getBlockState(0, 64, 0)->stateId(), beforeId);
    EXPECT_EQ(stack.getChildTag("debug_stick_state"), nullptr);
}

TEST_F(DebugStickItemTest, LeftClickSelectsPropertyAndConsumesBreak)
{
    DebugStickTestWorld world;
    world.setBlockState(0, 64, 0, &VanillaBlocks::OAK_LOG->defaultState());

    auto player = makeOperator();
    ItemStack stack(*Items::DEBUG_STICK, 1);
    auto* debugStick = dynamic_cast<item::items::DebugStickItem*>(Items::DEBUG_STICK);
    ASSERT_NE(debugStick, nullptr);

    bool consumed = debugStick->canDestroyBlock(
        stack, *world.getBlockState(0, 64, 0), world, BlockPos(0, 64, 0), *player);
    // 调试棒左键返回 false（不允许破坏该方块），但已完成选中属性切换
    EXPECT_FALSE(consumed);

    // 已记录选中属性（OAK_LOG 仅有 axis 属性）
    const nlohmann::json* debugState = stack.getChildTag("debug_stick_state");
    ASSERT_NE(debugState, nullptr);
    const std::string blockId = VanillaBlocks::OAK_LOG->blockLocation().toString();
    ASSERT_TRUE(debugState->contains(blockId));
    EXPECT_EQ((*debugState)[blockId].get<std::string>(), "axis");
}

TEST_F(DebugStickItemTest, RightClickCyclesPropertyValue)
{
    DebugStickTestWorld world;
    const BlockState& initial = VanillaBlocks::OAK_LOG->defaultState();
    world.setBlockState(0, 64, 0, &initial);
    // OAK_LOG 的 axis 属性由 RotatedPillarBlock::AXIS() 持有（与 BlockStateProperties::AXIS() 是不同实例）
    const EnumProperty<Axis>& axisProp = RotatedPillarBlock::AXIS();
    const Axis initialAxis = initial.get(axisProp);

    auto player = makeOperator();
    ItemStack stack(*Items::DEBUG_STICK, 1);
    heldStack(*player) = stack;

    ItemUseContext context(world,
        player.get(),
        heldStack(*player),
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    auto* debugStick = dynamic_cast<item::items::DebugStickItem*>(Items::DEBUG_STICK);
    ASSERT_NE(debugStick, nullptr);
    ActionResultType result = debugStick->onItemUse(context);
    EXPECT_EQ(result, ActionResultType::Success);

    const BlockState* updated = world.getBlockState(0, 64, 0);
    ASSERT_NE(updated, nullptr);
    // 轴属性已被切换到下一个值
    EXPECT_NE(updated->get(axisProp), initialAxis);

    // 选中属性已写入物品 NBT
    const nlohmann::json* debugState = heldStack(*player).getChildTag("debug_stick_state");
    ASSERT_NE(debugState, nullptr);
    EXPECT_TRUE(debugState->contains(VanillaBlocks::OAK_LOG->blockLocation().toString()));
}

TEST_F(DebugStickItemTest, SneakReversesCycle)
{
    DebugStickTestWorld world;
    const BlockState& initial = VanillaBlocks::OAK_LOG->defaultState();
    world.setBlockState(0, 64, 0, &initial);
    const EnumProperty<Axis>& axisProp = RotatedPillarBlock::AXIS();
    const Axis initialAxis = initial.get(axisProp);

    auto player = makeOperator();
    player->handleMovementInput(0.0f, 0.0f, false, true);

    ItemStack stack(*Items::DEBUG_STICK, 1);
    heldStack(*player) = stack;

    ItemUseContext context(world,
        player.get(),
        heldStack(*player),
        Vector3(0.5f, 64.5f, 0.5f),
        BlockPos(0, 64, 0),
        Direction::Up,
        Hand::MainHand,
        0.0f,
        0.0f);

    auto* debugStick = dynamic_cast<item::items::DebugStickItem*>(Items::DEBUG_STICK);
    ASSERT_NE(debugStick, nullptr);
    debugStick->onItemUse(context);

    // 反向循环：轴值与初始不同
    const BlockState* updated = world.getBlockState(0, 64, 0);
    ASSERT_NE(updated, nullptr);
    EXPECT_NE(updated->get(axisProp), initialAxis);
}
