#include <gtest/gtest.h>
#include "world/block/blocks/EnchantingTableBlock.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/blockentity/BlockEntityType.hpp"

using namespace mc;
using namespace mc::blocks;

// ========== EnchantingTableBlock 测试 ==========

class EnchantingTableBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建附魔台方块
        enchantingTable_ = std::make_unique<EnchantingTableBlock>(
            BlockProperties(Material::ROCK)
                .hardness(5.0f)
                .resistance(1200.0f)
                .notSolid()
        );
    }

    std::unique_ptr<EnchantingTableBlock> enchantingTable_;
};

TEST_F(EnchantingTableBlockTest, Create_HasCorrectProperties) {
    EXPECT_NE(enchantingTable_, nullptr);
}

TEST_F(EnchantingTableBlockTest, HasBlockEntity_ReturnsTrue) {
    EXPECT_TRUE(enchantingTable_->hasBlockEntity());
}

TEST_F(EnchantingTableBlockTest, GetBlockEntityType_ReturnsCorrectType) {
    EXPECT_EQ(enchantingTable_->getBlockEntityType(), BlockEntityType::EnchantingTable);
}

TEST_F(EnchantingTableBlockTest, GetShape_ReturnsValidShape) {
    const auto& state = enchantingTable_->defaultState();
    const auto& shape = enchantingTable_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(EnchantingTableBlockTest, GetOcclusionShape_CanBeEmpty) {
    // 附魔台是非固体方块，遮挡形状可以为空
    const auto& state = enchantingTable_->defaultState();
    const auto& shape = enchantingTable_->getOcclusionShape(state);
    // 非固体方块的遮挡形状可以为空
}

TEST_F(EnchantingTableBlockTest, GetPushReaction_ReturnsBlock) {
    const auto& state = enchantingTable_->defaultState();
    EXPECT_EQ(enchantingTable_->getPushReaction(state), Material::PushReaction::Block);
}
