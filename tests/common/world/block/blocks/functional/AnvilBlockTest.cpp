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
 * @file AnvilBlockTest.cpp
 * @brief AnvilBlock 单元测试
 *
 * 测试 AnvilBlock 的核心功能：状态属性、损坏状态转换、朝向旋转、碰撞箱、交互等。
 */

#include "common/world/block/blocks/functional/AnvilBlock.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <memory>
#include <unordered_map>
#include <gtest/gtest.h>

// 前向声明
namespace mc::server {
class ServerWorld;
}

namespace mc {
namespace blocks {
namespace test {

// ========== 测试固件和辅助类 ==========

/**
 * @brief AnvilBlock 测试固件
 */
class AnvilBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    void TearDown() override {}
};

/**
 * @brief 测试用 Mock World，用于测试 AnvilBlock 的交互和放置逻辑
 *
 * 实现了 IWorld 接口的关键方法，特别是：
 * - isClientSide() / asServerWorld() 用于区分客户端/服务端
 * - openContainer() 用于测试容器打开
 */
class AnvilTestWorld final : public ::mc::test::BaseTestWorld {
public:
    explicit AnvilTestWorld(bool isClient = false)
        : m_isClient(isClient)
        , m_openContainerCalled(false)
        , m_lastContainerType(ContainerType::Player)
        , m_lastContainerPos(0, 0, 0)
    {}

    // ========== IWorld 核心接口 ==========

    [[nodiscard]] bool isClientSide() const override { return m_isClient; }

    [[nodiscard]] ::mc::server::ServerWorld* asServerWorld() override
    {
        return m_isClient ? nullptr : reinterpret_cast<::mc::server::ServerWorld*>(0x1); // 非 null 表示服务端
    }

    bool openContainer(ContainerType type, const BlockPos& pos, Player& player) override
    {
        m_openContainerCalled = true;
        m_lastContainerType = type;
        m_lastContainerPos = pos;
        return !m_isClient; // 客户端返回 false，服务端返回 true
    }

    // ========== 测试验证方法 ==========

    [[nodiscard]] bool wasOpenContainerCalled() const { return m_openContainerCalled; }
    [[nodiscard]] ContainerType getLastContainerType() const { return m_lastContainerType; }
    [[nodiscard]] BlockPos getLastContainerPos() const { return m_lastContainerPos; }

private:
    bool m_isClient;
    bool m_openContainerCalled;
    ContainerType m_lastContainerType;
    BlockPos m_lastContainerPos;
};

// ========== 状态属性测试 ==========

/**
 * @brief 测试铁砧方块默认朝向
 *
 * MC 原版：铁砧默认朝向北
 */
TEST_F(AnvilBlockTest, DefaultFacingIsNorth)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& defaultState = anvil->defaultState();
    EXPECT_TRUE(defaultState.hasProperty(BlockStateProperties::HORIZONTAL_FACING()));
    Direction facing = defaultState.get(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_EQ(facing, Direction::North);
}

/**
 * @brief 测试铁砧三种变体都注册成功
 */
TEST_F(AnvilBlockTest, AllAnvilVariantsRegistered)
{
    EXPECT_NE(block_registry::BuildingBlocks::ANVIL, nullptr);
    EXPECT_NE(block_registry::BuildingBlocks::CHIPPED_ANVIL, nullptr);
    EXPECT_NE(block_registry::BuildingBlocks::DAMAGED_ANVIL, nullptr);
}

/**
 * @brief 测试铁砧变体的方块ID互不相同
 */
TEST_F(AnvilBlockTest, AnvilVariantsHaveDifferentBlockIds)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    const Block* chipped = block_registry::BuildingBlocks::CHIPPED_ANVIL;
    const Block* damaged = block_registry::BuildingBlocks::DAMAGED_ANVIL;

    EXPECT_NE(anvil->blockId(), chipped->blockId());
    EXPECT_NE(chipped->blockId(), damaged->blockId());
    EXPECT_NE(anvil->blockId(), damaged->blockId());
}

// ========== 损坏状态转换测试 ==========

/**
 * @brief 测试铁砧损坏状态转换：anvil → chipped_anvil
 */
TEST_F(AnvilBlockTest, DamageAnvilToIntact)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& state = anvil->defaultState();
    const BlockState* damaged = AnvilBlock::damageAnvil(state);

    ASSERT_NE(damaged, nullptr);
    EXPECT_EQ(damaged->getBlock().blockLocation(), ResourceLocation("minecraft", "chipped_anvil"));
}

/**
 * @brief 测试铁砧损坏状态转换：chipped_anvil → damaged_anvil
 */
TEST_F(AnvilBlockTest, DamageAnvilToChipped)
{
    const Block* chipped = block_registry::BuildingBlocks::CHIPPED_ANVIL;
    ASSERT_NE(chipped, nullptr);

    const BlockState& state = chipped->defaultState();
    const BlockState* damaged = AnvilBlock::damageAnvil(state);

    ASSERT_NE(damaged, nullptr);
    EXPECT_EQ(damaged->getBlock().blockLocation(), ResourceLocation("minecraft", "damaged_anvil"));
}

/**
 * @brief 测试铁砧损坏状态转换：damaged_anvil → nullptr（完全摧毁）
 */
TEST_F(AnvilBlockTest, DamageAnvilToDestroyed)
{
    const Block* damaged = block_registry::BuildingBlocks::DAMAGED_ANVIL;
    ASSERT_NE(damaged, nullptr);

    const BlockState& state = damaged->defaultState();
    const BlockState* result = AnvilBlock::damageAnvil(state);

    EXPECT_EQ(result, nullptr);
}

/**
 * @brief 测试铁砧损坏时保留朝向属性
 *
 * 损坏后的铁砧应保留原始的 HORIZONTAL_FACING
 */
TEST_F(AnvilBlockTest, DamageAnvilPreservesFacing)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    // 设置朝向为东
    const BlockState& facingEast =
        anvil->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState* damaged = AnvilBlock::damageAnvil(facingEast);

    ASSERT_NE(damaged, nullptr);
    Direction facing = damaged->get(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_EQ(facing, Direction::East);
}

/**
 * @brief 测试铁砧损坏链完整性：anvil → chipped → damaged → destroyed
 */
TEST_F(AnvilBlockTest, DamageChainComplete)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    // 第一级损坏
    const BlockState* state1 = &anvil->defaultState();
    const BlockState* state2 = AnvilBlock::damageAnvil(*state1);
    ASSERT_NE(state2, nullptr);
    EXPECT_EQ(state2->getBlock().blockLocation(), ResourceLocation("minecraft", "chipped_anvil"));

    // 第二级损坏
    const BlockState* state3 = AnvilBlock::damageAnvil(*state2);
    ASSERT_NE(state3, nullptr);
    EXPECT_EQ(state3->getBlock().blockLocation(), ResourceLocation("minecraft", "damaged_anvil"));

    // 第三级损坏（完全摧毁）
    const BlockState* state4 = AnvilBlock::damageAnvil(*state3);
    EXPECT_EQ(state4, nullptr);
}

/**
 * @brief 测试非铁砧方块调用 damageAnvil 返回 nullptr
 */
TEST_F(AnvilBlockTest, DamageAnvilOnNonAnvilReturnsNull)
{
    const Block* sand = VanillaBlocks::SAND;
    ASSERT_NE(sand, nullptr);

    const BlockState& state = sand->defaultState();
    const BlockState* result = AnvilBlock::damageAnvil(state);

    EXPECT_EQ(result, nullptr);
}

/**
 * @brief 测试铁砧方块在 ANVIL 标签中
 */
TEST_F(AnvilBlockTest, AnvilInAnvilBlockTag)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    const Block* chipped = block_registry::BuildingBlocks::CHIPPED_ANVIL;
    const Block* damaged = block_registry::BuildingBlocks::DAMAGED_ANVIL;

    ASSERT_NE(anvil, nullptr);
    ASSERT_NE(chipped, nullptr);
    ASSERT_NE(damaged, nullptr);

    EXPECT_TRUE(BlockTags::ANVIL().contains(*anvil));
    EXPECT_TRUE(BlockTags::ANVIL().contains(*chipped));
    EXPECT_TRUE(BlockTags::ANVIL().contains(*damaged));

    // 非铁砧方块不在标签中
    const Block* sand = VanillaBlocks::SAND;
    ASSERT_NE(sand, nullptr);
    EXPECT_FALSE(BlockTags::ANVIL().contains(*sand));
}

// ========== 旋转和镜像测试 ==========

/**
 * @brief 测试铁砧旋转
 */
TEST_F(AnvilBlockTest, Rotation)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& north = anvil->defaultState();
    EXPECT_EQ(north.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);

    // 顺时针旋转 90 度
    const BlockState& east = anvil->rotate(north, Rotation::Clockwise90);
    EXPECT_EQ(east.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // 旋转 180 度
    const BlockState& south = anvil->rotate(north, Rotation::Clockwise180);
    EXPECT_EQ(south.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

/**
 * @brief 测试铁砧镜像
 */
TEST_F(AnvilBlockTest, Mirror)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    // 北朝向
    const BlockState& north = anvil->defaultState();

    // 前后镜像：南北互换
    const BlockState& mirroredFB = anvil->mirror(north, Mirror::FrontBack);
    EXPECT_EQ(mirroredFB.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // 左右镜像：北不变
    const BlockState& mirroredLR = anvil->mirror(north, Mirror::LeftRight);
    EXPECT_EQ(mirroredLR.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

// ========== 铁砧碰撞箱测试 ==========

/**
 * @brief 测试铁砧碰撞箱非空
 *
 * 铁砧的碰撞箱应非空，且由多个子盒组成
 */
TEST_F(AnvilBlockTest, GetShape_NotEmpty)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& state = anvil->defaultState();
    const CollisionShape& shape = anvil->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

/**
 * @brief 测试铁砧碰撞箱非满方块
 *
 * 铁砧的碰撞箱不应覆盖整个方块
 */
TEST_F(AnvilBlockTest, GetShape_NotFullBlock)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& state = anvil->defaultState();
    const CollisionShape& shape = anvil->getShape(state);
    EXPECT_FALSE(shape.coversFullBlock());
}

/**
 * @brief 测试铁砧碰撞箱包含多个子盒
 *
 * 铁砧碰撞箱由4个子盒组成（底座、中段、窄颈、顶面）
 */
TEST_F(AnvilBlockTest, GetShape_HasMultipleBoxes)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& state = anvil->defaultState();
    const CollisionShape& shape = anvil->getShape(state);
    // 铁砧由4个子盒组成
    EXPECT_GE(shape.boxCount(), 4u);
}

/**
 * @brief 测试铁砧碰撞箱根据朝向轴返回不同形状
 *
 * MC 原版铁砧形状 X/Z 不对称：顶面沿朝向方向满16像素宽。
 * North/South (Z轴) 和 East/West (X轴) 应返回不同形状。
 */
TEST_F(AnvilBlockTest, GetShape_DifferentShapeForDifferentAxes)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    // North/South 共用 Z 轴形状
    const BlockState& northState =
        anvil->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& southState =
        anvil->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);

    // East/West 共用 X 轴形状
    const BlockState& eastState =
        anvil->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const BlockState& westState =
        anvil->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);

    const CollisionShape& northShape = anvil->getShape(northState);
    const CollisionShape& southShape = anvil->getShape(southState);
    const CollisionShape& eastShape = anvil->getShape(eastState);
    const CollisionShape& westShape = anvil->getShape(westState);

    // North/South 应返回同一形状引用（Z轴）
    EXPECT_EQ(&northShape, &southShape);

    // East/West 应返回同一形状引用（X轴）
    EXPECT_EQ(&eastShape, &westShape);

    // Z轴和X轴形状应不同（铁砧是X/Z不对称的）
    EXPECT_NE(&northShape, &eastShape);
}

/**
 * @brief 测试铁砧碰撞箱与视觉形状一致
 *
 * getCollisionShape 应返回与 getShape 相同的形状
 */
TEST_F(AnvilBlockTest, GetCollisionShape_SameAsShape)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    const BlockState& state = anvil->defaultState();
    const CollisionShape& shape = anvil->getShape(state);
    const CollisionShape& collisionShape = anvil->getCollisionShape(state);

    // 碰撞箱应与视觉形状一致
    EXPECT_EQ(&shape, &collisionShape);
}

// ========== 寻路测试 ==========

/**
 * @brief 测试铁砧不允许寻路通过
 *
 * MC 原版：AnvilBlock.isPathfindable() 返回 false，
 * 铁砧不应作为实体寻路的目标方块。
 */
TEST_F(AnvilBlockTest, AllowsMovement_ReturnsFalse)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvil, nullptr);

    AnvilTestWorld world;
    BlockPos pos(10, 64, 20);

    const BlockState& northState =
        anvil->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState& eastState =
        anvil->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);

    // 所有朝向的铁砧都不允许寻路
    EXPECT_FALSE(anvil->allowsMovement(northState, world, pos));
    EXPECT_FALSE(anvil->allowsMovement(eastState, world, pos));
}

/**
 * @brief 测试三种铁砧变体都不允许寻路
 */
TEST_F(AnvilBlockTest, AllAnvilVariants_AllowMovementFalse)
{
    const Block* anvil = block_registry::BuildingBlocks::ANVIL;
    const Block* chipped = block_registry::BuildingBlocks::CHIPPED_ANVIL;
    const Block* damaged = block_registry::BuildingBlocks::DAMAGED_ANVIL;

    ASSERT_NE(anvil, nullptr);
    ASSERT_NE(chipped, nullptr);
    ASSERT_NE(damaged, nullptr);

    AnvilTestWorld world;
    BlockPos pos(10, 64, 20);

    EXPECT_FALSE(anvil->allowsMovement(anvil->defaultState(), world, pos));
    EXPECT_FALSE(chipped->allowsMovement(chipped->defaultState(), world, pos));
    EXPECT_FALSE(damaged->allowsMovement(damaged->defaultState(), world, pos));
}

// ========== 铁砧放置朝向测试 ==========

/**
 * @brief 测试铁砧放置朝向为顺时针旋转90度
 *
 * MC 原版：铁砧的 HORIZONTAL_FACING 为玩家朝向的顺时针旋转90度。
 * 例如：玩家朝北 → 铁砧朝东，玩家朝东 → 铁砧朝南。
 */
TEST_F(AnvilBlockTest, GetStateForPlacement_ClockwiseRotation)
{
    Block* anvilBlock = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvilBlock, nullptr);

    AnvilTestWorld world;

    BlockPos pos(10, 64, 20);
    ItemStack stack; // 空 ItemStack

    // 玩家朝南（yaw=0）→ 顺时针90度 → 西
    {
        Vector3 hitPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
        BlockItemUseContext context(world, nullptr, stack, hitPos, pos, Direction::Up, 0.0f, 0.0f);
        BlockState state = anvilBlock->getStateForPlacement(context);
        // yaw=0 → horizontalDirection=South → rotateY(South) = West
        EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
    }

    // 玩家朝西（yaw=90）→ 顺时针90度 → 北
    {
        Vector3 hitPos(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f);
        BlockItemUseContext context(world, nullptr, stack, hitPos, pos, Direction::Up, 90.0f, 0.0f);
        BlockState state = anvilBlock->getStateForPlacement(context);
        // yaw=90 → horizontalDirection=West → rotateY(West) = North
        EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    }
}

// ========== 铁砧交互测试 ==========

/**
 * @brief 测试铁砧客户端交互返回 Success
 *
 * 客户端调用 onBlockActivated 应返回 Success，不打开容器。
 */
TEST_F(AnvilBlockTest, OnBlockActivated_ClientSide_ReturnsSuccess)
{
    Block* anvilBlock = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvilBlock, nullptr);

    AnvilTestWorld world(true); // 客户端
    BlockPos pos(10, 64, 20);
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    const auto& state = anvilBlock->defaultState();
    BlockRaycastResult hit;
    auto result = anvilBlock->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    // 客户端应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 客户端不应调用 openContainer
    EXPECT_FALSE(world.wasOpenContainerCalled());
}

/**
 * @brief 测试铁砧服务端交互打开 Anvil 容器
 *
 * 服务端调用 onBlockActivated 应返回 Consume，并打开 Anvil 类型容器。
 */
TEST_F(AnvilBlockTest, OnBlockActivated_ServerSide_OpensAnvilContainer)
{
    Block* anvilBlock = block_registry::BuildingBlocks::ANVIL;
    ASSERT_NE(anvilBlock, nullptr);

    AnvilTestWorld world(false); // 服务端
    BlockPos pos(10, 64, 20);
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

    const auto& state = anvilBlock->defaultState();
    BlockRaycastResult hit;
    auto result = anvilBlock->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    // 服务端应返回 Consume
    EXPECT_EQ(result, ActionResultType::Consume);

    // 服务端应调用 openContainer
    EXPECT_TRUE(world.wasOpenContainerCalled());

    // 容器类型应为 Anvil
    EXPECT_EQ(world.getLastContainerType(), ContainerType::Anvil);

    // 容器位置应正确
    EXPECT_EQ(world.getLastContainerPos(), pos);
}

} // namespace test
} // namespace blocks
} // namespace mc
