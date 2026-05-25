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

#include "world/block/blocks/functional/GrindstoneBlock.hpp"
#include "util/Direction.hpp"
#include "util/property/Properties.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/Material.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

// ========== GrindstoneBlock 测试 ==========

class GrindstoneBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建砂轮方块
        grindstone_ = std::make_unique<GrindstoneBlock>(
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(2.0f));
    }

    std::unique_ptr<GrindstoneBlock> grindstone_;
};

TEST_F(GrindstoneBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(grindstone_, nullptr);
}

TEST_F(GrindstoneBlockTest, DefaultState_HasCorrectAttachFace)
{
    const auto& state = grindstone_->defaultState();
    auto attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    EXPECT_EQ(attachFace, BlockStateProperties::AttachFace::Wall);
}

TEST_F(GrindstoneBlockTest, DefaultState_HasCorrectFacing)
{
    const auto& state = grindstone_->defaultState();
    auto facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_EQ(facing, Direction::North);
}

TEST_F(GrindstoneBlockTest, IsOpaque_ReturnsFalse)
{
    const auto& state = grindstone_->defaultState();
    EXPECT_FALSE(grindstone_->isOpaque(state));
}

// ========== 形状测试 ==========

TEST_F(GrindstoneBlockTest, GetShape_FloorAttach_ReturnsValidShape)
{
    // 地面附着 + 北朝向
    auto state = grindstone_->defaultState()
                     .with(BlockStateProperties::ATTACH_FACE(), BlockStateProperties::AttachFace::Floor)
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const auto& shape = grindstone_->getShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Floor-North shape should not be empty";

    // 地面附着 + 南朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Floor-South shape should not be empty";

    // 地面附着 + 西朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Floor-West shape should not be empty";

    // 地面附着 + 东朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Floor-East shape should not be empty";
}

TEST_F(GrindstoneBlockTest, GetShape_WallAttach_ReturnsValidShape)
{
    // 墙面附着 + 北朝向
    auto state = grindstone_->defaultState()
                     .with(BlockStateProperties::ATTACH_FACE(), BlockStateProperties::AttachFace::Wall)
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const auto& shape = grindstone_->getShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Wall-North shape should not be empty";

    // 墙面附着 + 南朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Wall-South shape should not be empty";

    // 墙面附着 + 西朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Wall-West shape should not be empty";

    // 墙面附着 + 东朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Wall-East shape should not be empty";
}

TEST_F(GrindstoneBlockTest, GetShape_CeilingAttach_ReturnsValidShape)
{
    // 天花板附着 + 北朝向
    auto state = grindstone_->defaultState()
                     .with(BlockStateProperties::ATTACH_FACE(), BlockStateProperties::AttachFace::Ceiling)
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const auto& shape = grindstone_->getShape(state);
    EXPECT_FALSE(shape.isEmpty()) << "Ceiling-North shape should not be empty";

    // 天花板附着 + 南朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Ceiling-South shape should not be empty";

    // 天花板附着 + 西朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Ceiling-West shape should not be empty";

    // 天花板附着 + 东朝向
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    EXPECT_FALSE(grindstone_->getShape(state).isEmpty()) << "Ceiling-East shape should not be empty";
}

TEST_F(GrindstoneBlockTest, GetCollisionShape_ReturnsSameAsShape)
{
    // 碰撞形状应与渲染形状相同
    auto state = grindstone_->defaultState()
                     .with(BlockStateProperties::ATTACH_FACE(), BlockStateProperties::AttachFace::Floor)
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    const auto& shape = grindstone_->getShape(state);
    const auto& collisionShape = grindstone_->getCollisionShape(state);

    // 两者都应非空
    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(collisionShape.isEmpty());
}

// ========== 旋转测试 ==========

TEST_F(GrindstoneBlockTest, Rotate_RotatesFacing)
{
    auto state = grindstone_->defaultState()
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    // 顺时针旋转90度: North -> East
    const auto& rotated90 = grindstone_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated90.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // 顺时针旋转180度: North -> South
    const auto& rotated180 = grindstone_->rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated180.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // 逆时针旋转90度: North -> West
    const auto& rotated270 = grindstone_->rotate(state, Rotation::CounterClockwise90);
    EXPECT_EQ(rotated270.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
}

TEST_F(GrindstoneBlockTest, Rotate_PreservesAttachFace)
{
    auto state = grindstone_->defaultState()
                     .with(BlockStateProperties::ATTACH_FACE(), BlockStateProperties::AttachFace::Floor)
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    // 旋转应保持附着面类型
    const auto& rotated = grindstone_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::ATTACH_FACE()), BlockStateProperties::AttachFace::Floor);
}

// ========== 镜像测试 ==========

TEST_F(GrindstoneBlockTest, Mirror_PreservesAttachFace)
{
    auto state = grindstone_->defaultState()
                     .with(BlockStateProperties::ATTACH_FACE(), BlockStateProperties::AttachFace::Wall)
                     .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    // 镜像应保持附着面类型
    const auto& mirrored = grindstone_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirrored.get(BlockStateProperties::ATTACH_FACE()), BlockStateProperties::AttachFace::Wall);
}
