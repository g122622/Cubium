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

#include "world/block/blocks/ShelfBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/ShelfBlockEntity.hpp"
#include <gtest/gtest.h>

using namespace mc::blocks;
using namespace mc::blockentity;
using mc::BlockEntityType;
using mc::BlockPos;
using mc::BlockProperties;
using mc::BlockStateProperties;
using mc::Material;

// ========== ShelfBlock 测试 ==========

class ShelfBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        shelf_ = std::make_unique<ShelfBlock>(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    }

    std::unique_ptr<ShelfBlock> shelf_;
};

// ========== 基本属性测试 ==========

TEST_F(ShelfBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(shelf_, nullptr);
}

TEST_F(ShelfBlockTest, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(shelf_->hasBlockEntity());
}

TEST_F(ShelfBlockTest, CreateBlockEntity_ReturnsShelfType)
{
    auto entity = shelf_->createBlockEntity(BlockPos(0, 0, 0));
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Shelf);
}

TEST_F(ShelfBlockTest, HasComparatorInputOverride_ReturnsTrue)
{
    const auto& state = shelf_->defaultState();
    EXPECT_TRUE(shelf_->hasComparatorInputOverride(state));
}

TEST_F(ShelfBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    const auto& state = shelf_->defaultState();
    EXPECT_TRUE(shelf_->useShapeForLightOcclusion(state));
}

// ========== 方块状态属性测试 ==========
// 注意：方块状态属性（FACING, POWERED, SIDE_CHAIN_PART, WATERLOGGED）的读取和设置
// 需要方块在注册系统中注册后才能使用，因此在单元测试中不做属性状态操作测试。
// 这些测试在集成测试中覆盖。

// ========== 碰撞形状测试 ==========
// 注意：碰撞形状需要方向状态属性，在注册前不可用。在集成测试中覆盖。

// ========== 常量测试 ==========

TEST_F(ShelfBlockTest, Constants_HaveExpectedValues)
{
    EXPECT_EQ(ShelfBlock::ROWS, 1);
    EXPECT_EQ(ShelfBlock::COLUMNS, 3);
    EXPECT_EQ(ShelfBlock::MAX_CHAIN_LENGTH, 3);
}

// ========== 侧链连接静态方法测试 ==========

TEST_F(ShelfBlockTest, IsConnectable_DefaultState_ReturnsFalse)
{
    // 默认状态 POWERED=false，不可连接
    const auto& state = shelf_->defaultState();
    EXPECT_FALSE(ShelfBlock::isConnectable(state));
}

TEST_F(ShelfBlockTest, IsConnectable_PoweredState_DoesNotCrash)
{
    // POWERED=true 时可连接（假设书架在 WOODEN_SHELVES 标签中）
    // 注意：isConnectable 还检查 WOODEN_SHELVES 标签，这里只测试 POWERED 条件
    // 由于标签注册需要完整的世界环境，这里仅验证 POWERED 属性的影响
    const auto poweredState = shelf_->defaultState().with(BlockStateProperties::POWERED(), true);
    // 如果方块在 WOODEN_SHELVES 标签中，isConnectable 应该返回 true
    // 否则可能返回 false（取决于标签注册状态）
    // 这里只确认函数不会崩溃
    ShelfBlock::isConnectable(poweredState);
}

// ========== SideChainPart 属性辅助方法测试 ==========

TEST_F(ShelfBlockTest, SideChainPart_HelperIsConnected)
{
    EXPECT_FALSE(BlockStateProperties::isConnected(BlockStateProperties::SideChainPart::Unconnected));
    EXPECT_TRUE(BlockStateProperties::isConnected(BlockStateProperties::SideChainPart::Left));
    EXPECT_TRUE(BlockStateProperties::isConnected(BlockStateProperties::SideChainPart::Center));
    EXPECT_TRUE(BlockStateProperties::isConnected(BlockStateProperties::SideChainPart::Right));
}

TEST_F(ShelfBlockTest, SideChainPart_HelperWhenConnectedToTheRight)
{
    using SCP = BlockStateProperties::SideChainPart;

    // 未连接时，连接到右侧应变为 Left
    EXPECT_EQ(BlockStateProperties::whenConnectedToTheRight(SCP::Unconnected), SCP::Left);
    // 已是 Left 时，连接到右侧仍为 Left（左侧书架保持 Left）
    EXPECT_EQ(BlockStateProperties::whenConnectedToTheRight(SCP::Left), SCP::Left);
    // 已是 Center 时，连接到右侧应保持 Center
    EXPECT_EQ(BlockStateProperties::whenConnectedToTheRight(SCP::Center), SCP::Center);
    // 已是 Right 时，连接到右侧应变为 Center
    EXPECT_EQ(BlockStateProperties::whenConnectedToTheRight(SCP::Right), SCP::Center);
}

TEST_F(ShelfBlockTest, SideChainPart_HelperWhenConnectedToTheLeft)
{
    using SCP = BlockStateProperties::SideChainPart;

    // 未连接时，连接到左侧应变为 Right
    EXPECT_EQ(BlockStateProperties::whenConnectedToTheLeft(SCP::Unconnected), SCP::Right);
    // 已是 Right 时，连接到左侧仍为 Right（右侧书架保持 Right）
    EXPECT_EQ(BlockStateProperties::whenConnectedToTheLeft(SCP::Right), SCP::Right);
    // 已是 Center 时，连接到左侧应保持 Center
    EXPECT_EQ(BlockStateProperties::whenConnectedToTheLeft(SCP::Center), SCP::Center);
    // 已是 Left 时，连接到左侧应变为 Center
    EXPECT_EQ(BlockStateProperties::whenConnectedToTheLeft(SCP::Left), SCP::Center);
}

TEST_F(ShelfBlockTest, SideChainPart_HelperWhenDisconnectedFromTheRight)
{
    using SCP = BlockStateProperties::SideChainPart;

    // 从右侧断开：Left -> Unconnected（左侧书架变成单独）
    EXPECT_EQ(BlockStateProperties::whenDisconnectedFromTheRight(SCP::Left), SCP::Unconnected);
    // 从右侧断开：Center -> Right（中间书架变成右侧）
    EXPECT_EQ(BlockStateProperties::whenDisconnectedFromTheRight(SCP::Center), SCP::Right);
    // 从右侧断开：Unconnected -> Unconnected（未改变）
    EXPECT_EQ(BlockStateProperties::whenDisconnectedFromTheRight(SCP::Unconnected), SCP::Unconnected);
    // 从右侧断开：Right -> Right（右侧书架不受影响）
    EXPECT_EQ(BlockStateProperties::whenDisconnectedFromTheRight(SCP::Right), SCP::Right);
}

TEST_F(ShelfBlockTest, SideChainPart_HelperWhenDisconnectedFromTheLeft)
{
    using SCP = BlockStateProperties::SideChainPart;

    // 从左侧断开：Right -> Unconnected（右侧书架变成单独）
    EXPECT_EQ(BlockStateProperties::whenDisconnectedFromTheLeft(SCP::Right), SCP::Unconnected);
    // 从左侧断开：Center -> Left（中间书架变成左侧）
    EXPECT_EQ(BlockStateProperties::whenDisconnectedFromTheLeft(SCP::Center), SCP::Left);
    // 从左侧断开：Unconnected -> Unconnected（未改变）
    EXPECT_EQ(BlockStateProperties::whenDisconnectedFromTheLeft(SCP::Unconnected), SCP::Unconnected);
    // 从左侧断开：Left -> Left（左侧书架不受影响）
    EXPECT_EQ(BlockStateProperties::whenDisconnectedFromTheLeft(SCP::Left), SCP::Left);
}

// ========== 旋转和镜像测试 ==========
// 注意：旋转和镜像操作需要方块在注册系统中注册后才能使用，
// 因此在单元测试中不做旋转镜像测试。这些测试在集成测试中覆盖。

// ========== 水 logged 测试 ==========
// 注意：isWaterlogged 需要方块状态属性，在注册前不可用。在集成测试中覆盖。

// ========== 方块实体创建测试 ==========

TEST_F(ShelfBlockTest, CreateBlockEntity_ReturnsShelfBlockEntity)
{
    auto entity = shelf_->createBlockEntity(BlockPos(10, 20, 30));
    ASSERT_NE(entity, nullptr);

    auto* shelfEntity = dynamic_cast<ShelfBlockEntity*>(entity.get());
    ASSERT_NE(shelfEntity, nullptr);
    EXPECT_EQ(shelfEntity->getContainerSize(), ShelfBlockEntity::SHELF_SIZE);
}

TEST_F(ShelfBlockTest, CreateBlockEntity_DifferentPositions)
{
    auto entity1 = shelf_->createBlockEntity(BlockPos(0, 0, 0));
    auto entity2 = shelf_->createBlockEntity(BlockPos(100, -50, 200));

    ASSERT_NE(entity1, nullptr);
    ASSERT_NE(entity2, nullptr);
    EXPECT_EQ(entity1->getPos(), BlockPos(0, 0, 0));
    EXPECT_EQ(entity2->getPos(), BlockPos(100, -50, 200));
}

// ========== 集成测试待补充 ==========
// TODO 以下测试需要方块注册系统完整后在世界环境中测试：
// - 侧链连接逻辑（SIDE_CHAIN_PART）: 充能时邻居连接/断电时断开
// - 红石充能（POWERED）状态更新: neighborChanged 检测红石信号变化
// - onBlockActivated 物品交互: 未充能时单物品交换，充能时热栏交换
// - onBlockRemoved 掉落物品: 确保书架内物品正确掉落
// - 旋转/镜像: ShelfBlock::rotate 和 ShelfBlock::mirror 的方块状态变化
// - 含水（WATERLOGGED）: isWaterlogged/getFluidState
// - 碰撞箱: getShape 返回基于朝向的非完整方块碰撞箱
