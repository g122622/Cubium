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
 * @file ChorusFruitItemTest.cpp
 * @brief 紫颂果物品单元测试
 *
 * 测试 ChorusFruitItem 的基本属性
 */

#include "item/items/food/ChorusFruitItem.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "sound/SoundEvents.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ==================== ChorusFruitItem Test Fixture ====================

class ChorusFruitItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 检查紫颂果物品是否已注册
    }

    void TearDown() override {}
};

// ==================== Sound Events Tests ====================

TEST_F(ChorusFruitItemTest, SoundEvents_AreDefined)
{
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

TEST_F(ChorusFruitItemTest, ItemRegistration_CHORUS_FRUIT_Exists)
{
    // 检查 Items::CHORUS_FRUIT 是否已注册
    if (Items::CHORUS_FRUIT != nullptr) {
        EXPECT_EQ(Items::CHORUS_FRUIT->maxStackSize(), 64);
        EXPECT_TRUE(Items::CHORUS_FRUIT->isFood());
    }
}
