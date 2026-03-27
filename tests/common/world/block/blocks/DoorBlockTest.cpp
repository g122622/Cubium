#include <gtest/gtest.h>
#include "world/block/blocks/DoorBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "util/property/Properties.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== DoorBlock 测试 ==========

class DoorBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建木门方块
        door_ = std::make_unique<DoorBlock>(
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
        );
    }

    std::unique_ptr<DoorBlock> door_;
};

TEST_F(DoorBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(door_, nullptr);
}

TEST_F(DoorBlockTest, IsOpen_UsesStaticMethod) {
    const auto& state = door_->defaultState();
    // 默认关闭
    EXPECT_FALSE(DoorBlock::isOpen(state));
}

TEST_F(DoorBlockTest, IsIronDoor_ReturnsFalse) {
    EXPECT_FALSE(door_->isIronDoor());
}

TEST_F(DoorBlockTest, GetPushReaction_ReturnsDestroy) {
    const auto& state = door_->defaultState();
    EXPECT_EQ(door_->getPushReaction(state), Material::PushReaction::Destroy);
}

TEST_F(DoorBlockTest, GetShape_ReturnsValidShape) {
    const auto& state = door_->defaultState();
    const auto& shape = door_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(DoorBlockTest, GetCollisionShape_ReturnsValidShape) {
    const auto& state = door_->defaultState();
    const auto& shape = door_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}
