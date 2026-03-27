#include <gtest/gtest.h>
#include "world/block/blocks/ChestBlock.hpp"
#include "world/block/blocks/TrappedChestBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "util/property/Properties.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== ChestBlock 测试 ==========

class ChestBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建箱子方块
        chest_ = std::make_unique<ChestBlock>(
            BlockProperties(Material::WOOD)
                .hardness(2.5f)
                .resistance(2.5f)
        );
    }

    std::unique_ptr<ChestBlock> chest_;
};

TEST_F(ChestBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(chest_, nullptr);
}

TEST_F(ChestBlockTest, HasBlockEntity_ReturnsTrue) {
    EXPECT_TRUE(chest_->hasBlockEntity());
}

TEST_F(ChestBlockTest, GetBlockEntityType_ReturnsChest) {
    EXPECT_EQ(chest_->getBlockEntityType(), BlockEntityType::Chest);
}

TEST_F(ChestBlockTest, CanProvidePower_ReturnsFalse) {
    const auto& state = chest_->defaultState();
    EXPECT_FALSE(chest_->canProvidePower(state));
}

TEST_F(ChestBlockTest, GetShape_ReturnsValidShape) {
    const auto& state = chest_->defaultState();
    const auto& shape = chest_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

// ========== TrappedChestBlock 测试 ==========

class TrappedChestBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建陷阱箱方块
        trappedChest_ = std::make_unique<TrappedChestBlock>(
            BlockProperties(Material::WOOD)
                .hardness(2.5f)
                .resistance(2.5f)
        );
    }

    std::unique_ptr<TrappedChestBlock> trappedChest_;
};

TEST_F(TrappedChestBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(trappedChest_, nullptr);
}

TEST_F(TrappedChestBlockTest, HasBlockEntity_ReturnsTrue) {
    EXPECT_TRUE(trappedChest_->hasBlockEntity());
}

TEST_F(TrappedChestBlockTest, GetBlockEntityType_ReturnsTrappedChest) {
    EXPECT_EQ(trappedChest_->getBlockEntityType(), BlockEntityType::TrappedChest);
}
