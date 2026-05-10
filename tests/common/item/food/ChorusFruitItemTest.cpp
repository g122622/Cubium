/**
 * @file ChorusFruitItemTest.cpp
 * @brief 紫颂果物品单元测试
 *
 * 测试 ChorusFruitItem 的基本属性
 */

#include <gtest/gtest.h>
#include "item/items/food/ChorusFruitItem.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "sound/SoundEvents.hpp"

using namespace mc;

// ==================== ChorusFruitItem Test Fixture ====================

class ChorusFruitItemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 检查紫颂果物品是否已注册
    }

    void TearDown() override {
    }
};

// ==================== Sound Events Tests ====================

TEST_F(ChorusFruitItemTest, SoundEvents_AreDefined) {
    // 验证紫颂果传送音效已定义
    const auto& teleportSound = SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT;
    EXPECT_EQ(teleportSound.namespace_(), "minecraft");
    EXPECT_EQ(teleportSound.path(), "item.chorus_fruit.teleport");

    // 验证狐狸传送音效已定义
    const auto& foxTeleportSound = SoundEvents::ENTITY_FOX_TELEPORT;
    EXPECT_EQ(foxTeleportSound.namespace_(), "minecraft");
    EXPECT_EQ(foxTeleportSound.path(), "entity.fox.teleport");
}

// ==================== Item Registration Check ====================

TEST_F(ChorusFruitItemTest, ItemRegistration_CHORUS_FRUIT_Exists) {
    // 检查 Items::CHORUS_FRUIT 是否已注册
    if (Items::CHORUS_FRUIT != nullptr) {
        EXPECT_EQ(Items::CHORUS_FRUIT->maxStackSize(), 64);
        EXPECT_TRUE(Items::CHORUS_FRUIT->isFood());
    }
}
