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
 * @file BellBlockTest.cpp
 * @brief 钟方块单元测试
 *
 * 测试 BellBlock 的功能：
 * - 状态属性
 * - 形状获取
 * - 附着类型
 * - 旋转和镜像
 */

#include "world/block/blocks/functional/BellBlock.hpp"
#include "util/property/Properties.hpp"
#include "world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== BellBlock 测试 ==========

class BellBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        bell_ = std::make_unique<BellBlock>(BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f));
    }

    std::unique_ptr<BellBlock> bell_;
};

TEST_F(BellBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(bell_, nullptr);
}

TEST_F(BellBlockTest, DefaultState_FacingNorth)
{
    const auto& state = bell_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(BellBlockTest, DefaultState_FloorAttachment)
{
    const auto& state = bell_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::BELL_ATTACHMENT()), BlockStateProperties::BellAttachment::Floor);
}

TEST_F(BellBlockTest, DefaultState_NotPowered)
{
    const auto& state = bell_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(BellBlockTest, GetShape_ReturnsValidShapeForAllAttachments)
{
    const auto& state = bell_->defaultState();

    // 地面附着
    const auto& floorState =
        state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Floor);
    EXPECT_FALSE(bell_->getShape(floorState).isEmpty());

    // 天花板附着
    const auto& ceilingState =
        state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Ceiling);
    EXPECT_FALSE(bell_->getShape(ceilingState).isEmpty());

    // 单面墙附着
    const auto& singleWallState =
        state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::SingleWall);
    EXPECT_FALSE(bell_->getShape(singleWallState).isEmpty());

    // 双面墙附着
    const auto& doubleWallState =
        state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::DoubleWall);
    EXPECT_FALSE(bell_->getShape(doubleWallState).isEmpty());
}

TEST_F(BellBlockTest, IsOpaque_ReturnsFalse)
{
    const auto& state = bell_->defaultState();
    EXPECT_FALSE(bell_->isOpaque(state));
}

TEST_F(BellBlockTest, Rotate_ChangesFacing)
{
    const auto& state = bell_->defaultState();

    // 旋转 90 度
    const auto& rotated90 = bell_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated90.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // 旋转 180 度
    const auto& rotated180 = bell_->rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated180.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(BellBlockTest, Mirror_ChangesFacing)
{
    const auto& state = bell_->defaultState();

    // 左右镜像（X轴）- 北方向会变成南方向
    const auto& mirroredLR = bell_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirroredLR.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // 前后镜像（Z轴）- 北方向保持不变
    const auto& mirroredFB = bell_->mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirroredFB.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(BellBlockTest, AttachmentType_AllValuesValid)
{
    // 验证所有附着类型都可以设置
    const auto& state = bell_->defaultState();

    EXPECT_NO_THROW(state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Floor));
    EXPECT_NO_THROW(state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Ceiling));
    EXPECT_NO_THROW(
        state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::SingleWall));
    EXPECT_NO_THROW(
        state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::DoubleWall));
}
