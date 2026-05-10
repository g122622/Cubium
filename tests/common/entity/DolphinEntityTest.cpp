/**
 * @file DolphinEntityTest.cpp
 * @brief 海豚实体单元测试
 *
 * 测试 DolphinEntity 的关键方法：
 * - isFoodItem: 检查是否是食物（鱼类）
 * - 维度和属性测试
 */

#include <gtest/gtest.h>
#include "entity/entities/passive/water/DolphinEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"

using namespace mc;

// ==================== DolphinEntity Test Fixture ====================

class DolphinEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建海豚实体
        dolphin = std::make_unique<DolphinEntity>(LegacyEntityType::Unknown, 0);
    }

    void TearDown() override {
        dolphin.reset();
    }

    std::unique_ptr<DolphinEntity> dolphin;
};

// ==================== isFoodItem Tests ====================

TEST_F(DolphinEntityTest, IsFoodItem_Cod_ReturnsTrue) {
    // 鳕鱼是海豚的食物
    if (Items::COD != nullptr) {
        ItemStack codStack(Items::COD, 1);
        EXPECT_TRUE(dolphin->isFoodItem(codStack));
    }
}

TEST_F(DolphinEntityTest, IsFoodItem_Salmon_ReturnsTrue) {
    // 鲑鱼是海豚的食物
    if (Items::SALMON != nullptr) {
        ItemStack salmonStack(Items::SALMON, 1);
        EXPECT_TRUE(dolphin->isFoodItem(salmonStack));
    }
}

TEST_F(DolphinEntityTest, IsFoodItem_Pufferfish_ReturnsTrue) {
    // 河豚是海豚的食物
    if (Items::PUFFERFISH != nullptr) {
        ItemStack pufferfishStack(Items::PUFFERFISH, 1);
        EXPECT_TRUE(dolphin->isFoodItem(pufferfishStack));
    }
}

TEST_F(DolphinEntityTest, IsFoodItem_TropicalFish_ReturnsTrue) {
    // 热带鱼是海豚的食物
    if (Items::TROPICAL_FISH != nullptr) {
        ItemStack tropicalFishStack(Items::TROPICAL_FISH, 1);
        EXPECT_TRUE(dolphin->isFoodItem(tropicalFishStack));
    }
}

TEST_F(DolphinEntityTest, IsFoodItem_NonFish_ReturnsFalse) {
    // 非鱼类物品不是海豚的食物
    if (Items::DIAMOND != nullptr) {
        ItemStack diamondStack(Items::DIAMOND, 1);
        EXPECT_FALSE(dolphin->isFoodItem(diamondStack));
    }
}

// ==================== Attribute Tests ====================

TEST_F(DolphinEntityTest, Attributes_HasCorrectValues) {
    // 海豚应该有正确的属性值
    // MC 1.16.5: 最大生命值 10，游泳速度 0.6
    EXPECT_FLOAT_EQ(dolphin->maxHealth(), 10.0f);
}

TEST_F(DolphinEntityTest, Dimensions_HasCorrectValues) {
    // MC 1.16.5 海豚尺寸
    EXPECT_FLOAT_EQ(dolphin->width(), 0.9f);
    EXPECT_FLOAT_EQ(dolphin->height(), 0.6f);
}

// ==================== Treasure Finding Tests ====================

TEST_F(DolphinEntityTest, TreasurePos_SetAndGet) {
    BlockPos pos(100, 64, 200);
    dolphin->setTreasurePos(pos);

    EXPECT_TRUE(dolphin->hasTreasureTarget());
    EXPECT_EQ(dolphin->getTreasurePos(), pos);
}

TEST_F(DolphinEntityTest, TreasurePos_Clear) {
    BlockPos pos(100, 64, 200);
    dolphin->setTreasurePos(pos);
    EXPECT_TRUE(dolphin->hasTreasureTarget());

    dolphin->clearTreasureTarget();
    EXPECT_FALSE(dolphin->hasTreasureTarget());
}

TEST_F(DolphinEntityTest, GuidingPlayer_SetAndGet) {
    dolphin->setGuidingPlayer(true, 12345ULL);

    EXPECT_TRUE(dolphin->isGuidingPlayer());
    EXPECT_EQ(dolphin->getGuidedPlayerId(), 12345ULL);
}

TEST_F(DolphinEntityTest, GuidingPlayer_Clear) {
    dolphin->setGuidingPlayer(true, 12345ULL);
    EXPECT_TRUE(dolphin->isGuidingPlayer());

    dolphin->clearTreasureTarget();
    EXPECT_FALSE(dolphin->isGuidingPlayer());
}

// ==================== Jump State Tests ====================

TEST_F(DolphinEntityTest, Jumping_SetAndGet) {
    EXPECT_FALSE(dolphin->isJumping());

    dolphin->setJumping(true);
    EXPECT_TRUE(dolphin->isJumping());

    dolphin->setJumping(false);
    EXPECT_FALSE(dolphin->isJumping());
}

// ==================== Eye Height Tests ====================

TEST_F(DolphinEntityTest, EyeHeight_CorrectValue) {
    // MC 1.16.5 海豚眼睛高度
    EXPECT_FLOAT_EQ(dolphin->eyeHeight(), 0.3f);
}
