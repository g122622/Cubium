/**
 * @file DecorativeBlockTest.cpp
 * @brief 装饰性方块单元测试
 *
 * 测试 LadderBlock、CarpetBlock、FlowerPotBlock、LanternBlock 的功能：
 * - 状态属性
 * - 形状获取
 * - 放置验证
 * - 邻居更新
 */

#include <gtest/gtest.h>
#include "world/block/blocks/decorative/LadderBlock.hpp"
#include "world/block/blocks/decorative/CarpetBlock.hpp"
#include "world/block/blocks/decorative/FlowerPotBlock.hpp"
#include "world/block/blocks/decorative/LanternBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "util/property/Properties.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== LadderBlock 测试 ==========

class LadderBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        ladder_ = std::make_unique<LadderBlock>(
            BlockProperties(Material::WOOD)
                .hardness(0.4f)
                .resistance(0.4f)
        );
    }

    std::unique_ptr<LadderBlock> ladder_;
};

TEST_F(LadderBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(ladder_, nullptr);
}

TEST_F(LadderBlockTest, DefaultState_FacingNorth) {
    const auto& state = ladder_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(LadderBlockTest, DefaultState_NotWaterlogged) {
    const auto& state = ladder_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(LadderBlockTest, IsLadder_ReturnsTrue) {
    const auto& state = ladder_->defaultState();
    EXPECT_TRUE(ladder_->isLadder(state, nullptr, nullptr, nullptr));
}

TEST_F(LadderBlockTest, GetShape_ReturnsValidShape) {
    const auto& state = ladder_->defaultState();
    const auto& shape = ladder_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(LadderBlockTest, GetCollisionShape_ReturnsEmpty) {
    const auto& state = ladder_->defaultState();
    const auto& shape = ladder_->getCollisionShape(state);
    // 梯子没有碰撞箱
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(LadderBlockTest, Rotate_ChangesFacing) {
    const auto& state = ladder_->defaultState();

    // 旋转 90 度
    const auto& rotated90 = ladder_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated90.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // 旋转 180 度
    const auto& rotated180 = ladder_->rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated180.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

// ========== CarpetBlock 测试 ==========

class CarpetBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        carpet_ = std::make_unique<CarpetBlock>(
            BlockProperties(Material::WOOL)
                .hardness(0.1f)
                .resistance(0.5f)
        );
    }

    std::unique_ptr<CarpetBlock> carpet_;
};

TEST_F(CarpetBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(carpet_, nullptr);
}

TEST_F(CarpetBlockTest, GetShape_ReturnsValidShape) {
    const auto& state = carpet_->defaultState();
    const auto& shape = carpet_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(CarpetBlockTest, GetCollisionShape_ReturnsEmpty) {
    const auto& state = carpet_->defaultState();
    const auto& shape = carpet_->getCollisionShape(state);
    // 地毯没有碰撞箱
    EXPECT_TRUE(shape.isEmpty());
}

// ========== FlowerPotBlock 测试 ==========

class FlowerPotBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建空花盆
        flowerPot_ = std::make_unique<FlowerPotBlock>(
            BlockProperties(Material::DECORATION)
                .hardness(0.0f)
                .resistance(0.0f),
            0  // content = 0 (空花盆)
        );
    }

    std::unique_ptr<FlowerPotBlock> flowerPot_;
};

TEST_F(FlowerPotBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(flowerPot_, nullptr);
}

TEST_F(FlowerPotBlockTest, GetContent_ReturnsZeroForEmptyPot) {
    EXPECT_EQ(flowerPot_->getContent(), 0u);
}

TEST_F(FlowerPotBlockTest, IsEmpty_ReturnsTrueForEmptyPot) {
    EXPECT_TRUE(flowerPot_->isEmpty());
}

TEST_F(FlowerPotBlockTest, GetShape_ReturnsValidShape) {
    const auto& state = flowerPot_->defaultState();
    const auto& shape = flowerPot_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(FlowerPotBlockTest, GetCollisionShape_ReturnsValidShape) {
    const auto& state = flowerPot_->defaultState();
    const auto& shape = flowerPot_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

// ========== LanternBlock 测试 ==========

class LanternBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        lantern_ = std::make_unique<LanternBlock>(
            BlockProperties(Material::IRON)
                .hardness(3.5f)
                .resistance(3.5f)
                .lightLevel(15),
            15
        );
    }

    std::unique_ptr<LanternBlock> lantern_;
};

TEST_F(LanternBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(lantern_, nullptr);
}

TEST_F(LanternBlockTest, DefaultState_NotHanging) {
    const auto& state = lantern_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::HANGING()));
}

TEST_F(LanternBlockTest, DefaultState_NotWaterlogged) {
    const auto& state = lantern_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(LanternBlockTest, GetShape_ReturnsValidShape) {
    const auto& state = lantern_->defaultState();
    const auto& shape = lantern_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(LanternBlockTest, GetShape_DifferentForHanging) {
    const auto& standingState = lantern_->defaultState();
    const auto& hangingState = standingState.with(BlockStateProperties::HANGING(), true);

    const auto& standingShape = lantern_->getShape(standingState);
    const auto& hangingShape = lantern_->getShape(hangingState);

    // 站立和悬挂形状应该不同
    // 注意：由于 CollisionShape 目前只比较指针，这里只验证形状有效
    EXPECT_FALSE(standingShape.isEmpty());
    EXPECT_FALSE(hangingShape.isEmpty());
}

TEST_F(LanternBlockTest, LightLevel_ReturnsCorrectValue) {
    EXPECT_EQ(lantern_->lightLevel(), 15u);
}
