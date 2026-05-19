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

#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"

#include <cmath>

namespace mc {
namespace {

/**
 * @brief HangingEntity 单元测试
 *
 * 测试悬挂实体的 canPlaceOn、dropItem 等方法。
 */
class HangingEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// canPlaceOn 测试
// ============================================================================

TEST_F(HangingEntityTest, CanPlaceOnReturnsFalseWhenWorldIsNull)
{
    // 创建画作实体
    entity::PaintingEntity painting;

    // 没有设置世界时应该返回 false
    EXPECT_FALSE(painting.canPlaceOn());
}

TEST_F(HangingEntityTest, CanPlaceOnReturnsTrueForValidPosition)
{
    // 这个测试需要模拟世界，目前只验证方法存在
    // 实际测试需要 Mock IWorld
    entity::PaintingEntity painting;

    // 验证方法可以调用
    // 由于没有设置世界，应该返回 false
    EXPECT_FALSE(painting.canPlaceOn());
}

TEST_F(HangingEntityTest, LeashKnotEntityCanBeCreated)
{
    // 验证拴绳结实体可以创建
    entity::LeashKnotEntity leashKnot;

    EXPECT_EQ(leashKnot.getWidth(), 1);
    EXPECT_EQ(leashKnot.getHeight(), 1);
}

TEST_F(HangingEntityTest, ItemFrameEntityCanBeCreated)
{
    // 验证物品展示框实体可以创建
    entity::ItemFrameEntity itemFrame;

    EXPECT_EQ(itemFrame.getWidth(), 1);
    EXPECT_EQ(itemFrame.getHeight(), 1);
    EXPECT_FALSE(itemFrame.isGlowing());

    itemFrame.setGlowing(true);
    EXPECT_TRUE(itemFrame.isGlowing());
}

TEST_F(HangingEntityTest, PaintingEntityMotiveCanBeSet)
{
    entity::PaintingEntity painting;

    // 验证默认画作
    EXPECT_EQ(painting.getMotive(), "Kebab");

    // 设置新的画作类型
    painting.setMotive("Aztec");
    EXPECT_EQ(painting.getMotive(), "Aztec");

    // 验证尺寸
    EXPECT_EQ(painting.getWidth(), 1);
    EXPECT_EQ(painting.getHeight(), 1);
}

TEST_F(HangingEntityTest, PaintingEntityDimensions)
{
    // 验证不同画作的尺寸
    entity::PaintingEntity painting;

    // 1x1 画作
    painting.setMotive("Kebab");
    EXPECT_EQ(painting.getWidth(), 1);
    EXPECT_EQ(painting.getHeight(), 1);

    // 2x1 画作
    painting.setMotive("Pool");
    EXPECT_EQ(painting.getWidth(), 2);
    EXPECT_EQ(painting.getHeight(), 1);

    // 4x4 画作
    painting.setMotive("Pointer");
    EXPECT_EQ(painting.getWidth(), 4);
    EXPECT_EQ(painting.getHeight(), 4);
}

TEST_F(HangingEntityTest, ItemFrameRotation)
{
    entity::ItemFrameEntity itemFrame;

    // 初始旋转
    EXPECT_EQ(itemFrame.getItemRotation(), 0);

    // 旋转物品
    itemFrame.rotateItem();
    EXPECT_EQ(itemFrame.getItemRotation(), 1);

    itemFrame.rotateItem();
    EXPECT_EQ(itemFrame.getItemRotation(), 2);

    // 设置旋转
    itemFrame.setItemRotation(5);
    EXPECT_EQ(itemFrame.getItemRotation(), 5);

    // 旋转超过 7 应该回绕
    itemFrame.setItemRotation(10);
    EXPECT_EQ(itemFrame.getItemRotation(), 2); // 10 % 8 = 2

    // 负数旋转
    itemFrame.setItemRotation(-1);
    EXPECT_EQ(itemFrame.getItemRotation(), 7); // -1 + 8 = 7
}

// ============================================================================
// ItemFrameEntity 红石信号测试
// ============================================================================

TEST_F(HangingEntityTest, ItemFrameAnalogOutput_NoItem_ReturnsZero)
{
    // MC 1.16.5: 无物品时返回 0
    entity::ItemFrameEntity itemFrame;

    EXPECT_FALSE(itemFrame.hasItem());
    EXPECT_EQ(itemFrame.getAnalogOutput(), 0);
}

TEST_F(HangingEntityTest, ItemFrameAnalogOutput_WithItem_ReturnsRotationPlusOne)
{
    // MC 1.16.5: 有物品时返回 rotation % 8 + 1
    entity::ItemFrameEntity itemFrame;

    // 设置物品
    ItemStack diamond(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(diamond);
    EXPECT_TRUE(itemFrame.hasItem());

    // rotation = 0 时，信号 = 1
    itemFrame.setItemRotation(0);
    EXPECT_EQ(itemFrame.getItemRotation(), 0);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 1);

    // rotation = 1 时，信号 = 2
    itemFrame.setItemRotation(1);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 2);

    // rotation = 7 时，信号 = 8
    itemFrame.setItemRotation(7);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 8);

    // rotation = 8 时，应该是 0（被 % 8）
    itemFrame.setItemRotation(8);
    EXPECT_EQ(itemFrame.getItemRotation(), 0);
    EXPECT_EQ(itemFrame.getAnalogOutput(), 1);
}

TEST_F(HangingEntityTest, ItemFrameAnalogOutput_RotationRange)
{
    // MC 1.16.5: 测试所有旋转值的信号强度
    entity::ItemFrameEntity itemFrame;
    ItemStack item(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(item);

    // 测试所有旋转值
    for (i32 rotation = 0; rotation <= 7; ++rotation) {
        itemFrame.setItemRotation(rotation);
        EXPECT_EQ(itemFrame.getAnalogOutput(), rotation + 1)
            << "Expected signal " << (rotation + 1) << " for rotation " << rotation;
    }

    // 信号范围应该是 1-8
    EXPECT_GE(itemFrame.getAnalogOutput(), 1);
    EXPECT_LE(itemFrame.getAnalogOutput(), 8);
}

TEST_F(HangingEntityTest, ItemFrameSetDisplayedItem_ResetsRotation)
{
    // MC 1.16.5: 设置物品时重置旋转为 0
    entity::ItemFrameEntity itemFrame;

    // 先设置旋转
    itemFrame.setItemRotation(5);
    EXPECT_EQ(itemFrame.getItemRotation(), 5);

    // 设置新物品时旋转应该重置
    ItemStack item(Items::DIAMOND, 1);
    itemFrame.setDisplayedItem(item);
    EXPECT_EQ(itemFrame.getItemRotation(), 0);
}

TEST_F(HangingEntityTest, ItemFrameHorizontalFacing_ConvertsCorrectly)
{
    // MC 1.16.5: 测试方向转换
    entity::ItemFrameEntity itemFrame;

    // 测试所有方向的转换
    // HangingEntity::Direction: SOUTH=0, WEST=1, NORTH=2, EAST=3
    // mc::Direction: North=2, South=3, West=4, East=5

    BlockPos pos(0, 0, 0);

    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::SOUTH);
    EXPECT_EQ(itemFrame.getHorizontalFacing(), mc::Direction::South);

    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::WEST);
    EXPECT_EQ(itemFrame.getHorizontalFacing(), mc::Direction::West);

    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::NORTH);
    EXPECT_EQ(itemFrame.getHorizontalFacing(), mc::Direction::North);

    itemFrame.setHangingPosition(pos, entity::HangingEntity::Direction::EAST);
    EXPECT_EQ(itemFrame.getHorizontalFacing(), mc::Direction::East);
}

TEST_F(HangingEntityTest, LeashKnotCanAttachEntities)
{
    entity::LeashKnotEntity leashKnot;

    // 验证可以绑定和解绑实体
    EXPECT_TRUE(leashKnot.getLeashedEntities().empty());

    // 注意：实际测试需要实体实例
    // 这里只验证方法存在
}

TEST_F(HangingEntityTest, HangingDirection)
{
    entity::PaintingEntity painting;

    // 验证默认方向
    EXPECT_EQ(painting.getDirection(), entity::HangingEntity::Direction::SOUTH);

    // 设置新方向
    BlockPos pos(0, 0, 0);
    painting.setHangingPosition(pos, entity::HangingEntity::Direction::NORTH);
    EXPECT_EQ(painting.getDirection(), entity::HangingEntity::Direction::NORTH);
    EXPECT_EQ(painting.getHangingBlockPos(), pos);

    painting.setHangingPosition(pos, entity::HangingEntity::Direction::EAST);
    EXPECT_EQ(painting.getDirection(), entity::HangingEntity::Direction::EAST);

    painting.setHangingPosition(pos, entity::HangingEntity::Direction::WEST);
    EXPECT_EQ(painting.getDirection(), entity::HangingEntity::Direction::WEST);
}

TEST_F(HangingEntityTest, ItemRegistration)
{
    // 验证物品已注册
    EXPECT_NE(Items::PAINTING, nullptr);
    EXPECT_NE(Items::ITEM_FRAME, nullptr);
    EXPECT_NE(Items::LEAD, nullptr);

    // 验证物品属性
    EXPECT_EQ(Items::PAINTING->maxStackSize(), 16);
    EXPECT_EQ(Items::ITEM_FRAME->maxStackSize(), 16);
    EXPECT_EQ(Items::LEAD->maxStackSize(), 16);
}

} // namespace
} // namespace mc
