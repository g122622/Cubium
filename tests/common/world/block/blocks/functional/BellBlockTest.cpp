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

#include <gtest/gtest.h>
#include "world/block/blocks/functional/BellBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "util/property/Properties.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== BellBlock 测试 ==========

class BellBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        bell_ = std::make_unique<BellBlock>(
            BlockProperties(Material::IRON)
                .hardness(5.0f)
                .resistance(5.0f)
        );
    }

    std::unique_ptr<BellBlock> bell_;
};

TEST_F(BellBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(bell_, nullptr);
}

TEST_F(BellBlockTest, DefaultState_FacingNorth) {
    const auto& state = bell_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(BellBlockTest, DefaultState_FloorAttachment) {
    const auto& state = bell_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::BELL_ATTACHMENT()), BlockStateProperties::BellAttachment::Floor);
}

TEST_F(BellBlockTest, DefaultState_NotPowered) {
    const auto& state = bell_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(BellBlockTest, GetShape_ReturnsValidShapeForAllAttachments) {
    const auto& state = bell_->defaultState();

    // 地面附着
    const auto& floorState = state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Floor);
    EXPECT_FALSE(bell_->getShape(floorState).isEmpty());

    // 天花板附着
    const auto& ceilingState = state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Ceiling);
    EXPECT_FALSE(bell_->getShape(ceilingState).isEmpty());

    // 单面墙附着
    const auto& singleWallState = state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::SingleWall);
    EXPECT_FALSE(bell_->getShape(singleWallState).isEmpty());

    // 双面墙附着
    const auto& doubleWallState = state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::DoubleWall);
    EXPECT_FALSE(bell_->getShape(doubleWallState).isEmpty());
}

TEST_F(BellBlockTest, IsOpaque_ReturnsFalse) {
    const auto& state = bell_->defaultState();
    EXPECT_FALSE(bell_->isOpaque(state));
}

TEST_F(BellBlockTest, Rotate_ChangesFacing) {
    const auto& state = bell_->defaultState();

    // 旋转 90 度
    const auto& rotated90 = bell_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated90.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // 旋转 180 度
    const auto& rotated180 = bell_->rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated180.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

TEST_F(BellBlockTest, Mirror_ChangesFacing) {
    const auto& state = bell_->defaultState();

    // 左右镜像（X轴）- 北方向会变成南方向
    const auto& mirroredLR = bell_->mirror(state, Mirror::LeftRight);
    EXPECT_EQ(mirroredLR.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // 前后镜像（Z轴）- 北方向保持不变
    const auto& mirroredFB = bell_->mirror(state, Mirror::FrontBack);
    EXPECT_EQ(mirroredFB.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(BellBlockTest, AttachmentType_AllValuesValid) {
    // 验证所有附着类型都可以设置
    const auto& state = bell_->defaultState();

    EXPECT_NO_THROW(state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Floor));
    EXPECT_NO_THROW(state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::Ceiling));
    EXPECT_NO_THROW(state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::SingleWall));
    EXPECT_NO_THROW(state.with(BlockStateProperties::BELL_ATTACHMENT(), BlockStateProperties::BellAttachment::DoubleWall));
}
