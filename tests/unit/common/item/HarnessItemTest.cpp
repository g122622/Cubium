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

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/HarnessItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::item::items;
using namespace mc::item::tag;

// ============================================================================
// 测试夹具
// ============================================================================

class HarnessItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 → 物品 → 物品标签
        VanillaBlocks::initialize();
        Items::initialize();
        ItemTags::initialize();
    }
};

// ============================================================================
// 注册与基础属性测试
// ============================================================================

TEST_F(HarnessItemTest, All16Colors_Registered_NotNull)
{
    // 16 色马铠全部应注册成功
    ASSERT_NE(Items::WHITE_HARNESS, nullptr);
    ASSERT_NE(Items::ORANGE_HARNESS, nullptr);
    ASSERT_NE(Items::MAGENTA_HARNESS, nullptr);
    ASSERT_NE(Items::LIGHT_BLUE_HARNESS, nullptr);
    ASSERT_NE(Items::YELLOW_HARNESS, nullptr);
    ASSERT_NE(Items::LIME_HARNESS, nullptr);
    ASSERT_NE(Items::PINK_HARNESS, nullptr);
    ASSERT_NE(Items::GRAY_HARNESS, nullptr);
    ASSERT_NE(Items::LIGHT_GRAY_HARNESS, nullptr);
    ASSERT_NE(Items::CYAN_HARNESS, nullptr);
    ASSERT_NE(Items::PURPLE_HARNESS, nullptr);
    ASSERT_NE(Items::BLUE_HARNESS, nullptr);
    ASSERT_NE(Items::BROWN_HARNESS, nullptr);
    ASSERT_NE(Items::GREEN_HARNESS, nullptr);
    ASSERT_NE(Items::RED_HARNESS, nullptr);
    ASSERT_NE(Items::BLACK_HARNESS, nullptr);
}

TEST_F(HarnessItemTest, MaxStackSize_Is1)
{
    // 装备类物品堆叠上限为 1
    EXPECT_EQ(Items::WHITE_HARNESS->maxStackSize(), 1);
    EXPECT_EQ(Items::BLACK_HARNESS->maxStackSize(), 1);
    EXPECT_EQ(Items::CYAN_HARNESS->maxStackSize(), 1);
}

TEST_F(HarnessItemTest, IsDamageable_False)
{
    // 马铠无耐久度，不可损坏
    EXPECT_FALSE(Items::WHITE_HARNESS->isDamageable());
    EXPECT_FALSE(Items::RED_HARNESS->maxDamage() > 0);
}

// ============================================================================
// 颜色映射测试
// ============================================================================

TEST_F(HarnessItemTest, GetColor_ReturnsCorrectDyeColor)
{
    // 每种颜色的 harness 应返回对应的 DyeColor
    auto* white = dynamic_cast<HarnessItem*>(Items::WHITE_HARNESS);
    ASSERT_NE(white, nullptr);
    EXPECT_EQ(white->getColor(), DyeColor::White);

    auto* orange = dynamic_cast<HarnessItem*>(Items::ORANGE_HARNESS);
    ASSERT_NE(orange, nullptr);
    EXPECT_EQ(orange->getColor(), DyeColor::Orange);

    auto* magenta = dynamic_cast<HarnessItem*>(Items::MAGENTA_HARNESS);
    ASSERT_NE(magenta, nullptr);
    EXPECT_EQ(magenta->getColor(), DyeColor::Magenta);

    auto* lightBlue = dynamic_cast<HarnessItem*>(Items::LIGHT_BLUE_HARNESS);
    ASSERT_NE(lightBlue, nullptr);
    EXPECT_EQ(lightBlue->getColor(), DyeColor::LightBlue);

    auto* yellow = dynamic_cast<HarnessItem*>(Items::YELLOW_HARNESS);
    ASSERT_NE(yellow, nullptr);
    EXPECT_EQ(yellow->getColor(), DyeColor::Yellow);

    auto* lime = dynamic_cast<HarnessItem*>(Items::LIME_HARNESS);
    ASSERT_NE(lime, nullptr);
    EXPECT_EQ(lime->getColor(), DyeColor::Lime);

    auto* pink = dynamic_cast<HarnessItem*>(Items::PINK_HARNESS);
    ASSERT_NE(pink, nullptr);
    EXPECT_EQ(pink->getColor(), DyeColor::Pink);

    auto* gray = dynamic_cast<HarnessItem*>(Items::GRAY_HARNESS);
    ASSERT_NE(gray, nullptr);
    EXPECT_EQ(gray->getColor(), DyeColor::Gray);

    auto* lightGray = dynamic_cast<HarnessItem*>(Items::LIGHT_GRAY_HARNESS);
    ASSERT_NE(lightGray, nullptr);
    EXPECT_EQ(lightGray->getColor(), DyeColor::LightGray);

    auto* cyan = dynamic_cast<HarnessItem*>(Items::CYAN_HARNESS);
    ASSERT_NE(cyan, nullptr);
    EXPECT_EQ(cyan->getColor(), DyeColor::Cyan);

    auto* purple = dynamic_cast<HarnessItem*>(Items::PURPLE_HARNESS);
    ASSERT_NE(purple, nullptr);
    EXPECT_EQ(purple->getColor(), DyeColor::Purple);

    auto* blue = dynamic_cast<HarnessItem*>(Items::BLUE_HARNESS);
    ASSERT_NE(blue, nullptr);
    EXPECT_EQ(blue->getColor(), DyeColor::Blue);

    auto* brown = dynamic_cast<HarnessItem*>(Items::BROWN_HARNESS);
    ASSERT_NE(brown, nullptr);
    EXPECT_EQ(brown->getColor(), DyeColor::Brown);

    auto* green = dynamic_cast<HarnessItem*>(Items::GREEN_HARNESS);
    ASSERT_NE(green, nullptr);
    EXPECT_EQ(green->getColor(), DyeColor::Green);

    auto* red = dynamic_cast<HarnessItem*>(Items::RED_HARNESS);
    ASSERT_NE(red, nullptr);
    EXPECT_EQ(red->getColor(), DyeColor::Red);

    auto* black = dynamic_cast<HarnessItem*>(Items::BLACK_HARNESS);
    ASSERT_NE(black, nullptr);
    EXPECT_EQ(black->getColor(), DyeColor::Black);
}

// ============================================================================
// itemLocation 测试
// ============================================================================

TEST_F(HarnessItemTest, ItemLocation_MatchesDyeColor)
{
    // 物品 id 应包含颜色名
    EXPECT_EQ(Items::WHITE_HARNESS->itemLocation().path(), "white_harness");
    EXPECT_EQ(Items::ORANGE_HARNESS->itemLocation().path(), "orange_harness");
    EXPECT_EQ(Items::MAGENTA_HARNESS->itemLocation().path(), "magenta_harness");
    EXPECT_EQ(Items::LIGHT_BLUE_HARNESS->itemLocation().path(), "light_blue_harness");
    EXPECT_EQ(Items::YELLOW_HARNESS->itemLocation().path(), "yellow_harness");
    EXPECT_EQ(Items::LIME_HARNESS->itemLocation().path(), "lime_harness");
    EXPECT_EQ(Items::PINK_HARNESS->itemLocation().path(), "pink_harness");
    EXPECT_EQ(Items::GRAY_HARNESS->itemLocation().path(), "gray_harness");
    EXPECT_EQ(Items::LIGHT_GRAY_HARNESS->itemLocation().path(), "light_gray_harness");
    EXPECT_EQ(Items::CYAN_HARNESS->itemLocation().path(), "cyan_harness");
    EXPECT_EQ(Items::PURPLE_HARNESS->itemLocation().path(), "purple_harness");
    EXPECT_EQ(Items::BLUE_HARNESS->itemLocation().path(), "blue_harness");
    EXPECT_EQ(Items::BROWN_HARNESS->itemLocation().path(), "brown_harness");
    EXPECT_EQ(Items::GREEN_HARNESS->itemLocation().path(), "green_harness");
    EXPECT_EQ(Items::RED_HARNESS->itemLocation().path(), "red_harness");
    EXPECT_EQ(Items::BLACK_HARNESS->itemLocation().path(), "black_harness");
}

// ============================================================================
// 标签集成测试
// ============================================================================

TEST_F(HarnessItemTest, HARNESSES_TagContainsAll16Colors)
{
    // HARNESSES 标签应包含全部 16 色马铠
    const auto& tag = ItemTags::HARNESSES();
    EXPECT_TRUE(tag.contains(Items::WHITE_HARNESS));
    EXPECT_TRUE(tag.contains(Items::ORANGE_HARNESS));
    EXPECT_TRUE(tag.contains(Items::MAGENTA_HARNESS));
    EXPECT_TRUE(tag.contains(Items::LIGHT_BLUE_HARNESS));
    EXPECT_TRUE(tag.contains(Items::YELLOW_HARNESS));
    EXPECT_TRUE(tag.contains(Items::LIME_HARNESS));
    EXPECT_TRUE(tag.contains(Items::PINK_HARNESS));
    EXPECT_TRUE(tag.contains(Items::GRAY_HARNESS));
    EXPECT_TRUE(tag.contains(Items::LIGHT_GRAY_HARNESS));
    EXPECT_TRUE(tag.contains(Items::CYAN_HARNESS));
    EXPECT_TRUE(tag.contains(Items::PURPLE_HARNESS));
    EXPECT_TRUE(tag.contains(Items::BLUE_HARNESS));
    EXPECT_TRUE(tag.contains(Items::BROWN_HARNESS));
    EXPECT_TRUE(tag.contains(Items::GREEN_HARNESS));
    EXPECT_TRUE(tag.contains(Items::RED_HARNESS));
    EXPECT_TRUE(tag.contains(Items::BLACK_HARNESS));
}

TEST_F(HarnessItemTest, HARNESSES_Tag_ExcludesOtherItems)
{
    // HARNESSES 标签不应包含其他物品（如马铠、狼铠）
    const auto& tag = ItemTags::HARNESSES();
    EXPECT_FALSE(tag.contains(Items::IRON_HORSE_ARMOR));
    EXPECT_FALSE(tag.contains(Items::WOLF_ARMOR));
}

TEST_F(HarnessItemTest, HARNESSES_Tag_HasExactly16Items)
{
    // HARNESSES 标签应恰好包含 16 项
    const auto& tag = ItemTags::HARNESSES();
    const auto items = tag.getItemsList();
    EXPECT_EQ(items.size(), 16u);
}

TEST_F(HarnessItemTest, IsIn_HARNESSES_Tag_ReturnsTrue)
{
    // Item::isIn 应正确识别标签归属
    EXPECT_TRUE(Items::WHITE_HARNESS->isIn(ItemTags::HARNESSES()));
    EXPECT_TRUE(Items::BLACK_HARNESS->isIn(ItemTags::HARNESSES()));
}

// ============================================================================
// ItemStack 标签集成测试
// ============================================================================

TEST_F(HarnessItemTest, ItemStack_Contains_TagCheckWorks)
{
    // ItemStack 形式的标签检查也应通过
    const ItemStack whiteHarnessStack(Items::WHITE_HARNESS, 1);
    EXPECT_TRUE(ItemTags::HARNESSES().contains(whiteHarnessStack));
}
