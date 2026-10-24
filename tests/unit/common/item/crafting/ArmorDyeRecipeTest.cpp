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

// 盔甲染色配方（ArmorDyeRecipe）与混色算法（DyedItemColor::applyDyes）的测试。
//
// 覆盖：
// - DyedItemColor::applyDyes 的一次性多染料归一化混色（数值锚定 vanilla 算法）；
// - 顺序无关性（同一组染料无论输入顺序结果一致）；
// - 已有颜色参与平均（二次染色）；
// - ArmorDyeRecipe 使用 ItemTags::DYEABLE 判定（含皮革马铠，非 DyeableArmorItem 子类）；
// - 匹配条件：恰好一个可染色物品 + 至少一个染料；
// - 染料物品被消耗（getRemainingItems 全空）。

#include <gtest/gtest.h>

#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/special/ArmorDyeRecipe.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/item/items/armor/HorseArmorItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyedItemColor.hpp"

using namespace mc;
using namespace mc::crafting;

namespace {

/// 3x3 合成网格
CraftingInventory _grid()
{
    return CraftingInventory(3, 3);
}

/// 皮革胸甲物品栈
ItemStack _leatherChestplate()
{
    return ItemStack(Items::LEATHER_CHESTPLATE, 1);
}

/// 读取物品 display.color（无颜色返回 std::nullopt）
std::optional<u32> _storedColor(const ItemStack& stack)
{
    return item::items::DyeableArmorItem::getStoredColorOrNone(stack);
}

} // namespace

// ============================================================================
// DyedItemColor::applyDyes —— 混色算法
// ============================================================================

class DyedItemColorTest : public ::testing::Test {};

TEST_F(DyedItemColorTest, SingleDye_YieldsDyeColor)
{
    // 单染料且物品未染色：结果应等于该染料的纹理漫反射色本身
    // （avg = 该色，avgMax = max(r,g,b)，maxAvg = 同值，缩放系数为 1）。
    const u32 result = DyedItemColor::applyDyes(std::nullopt, {dyeTextureDiffuseColor(DyeColor::Red)});
    EXPECT_EQ(result, 0xB02E26u);
}

TEST_F(DyedItemColorTest, MultipleDyes_OneShotNormalization)
{
    // 红 + 蓝一次性混色。vanilla 的亮度归一化会明显提亮结果（0xAD5398），
    // 而非逐次两两平均得到的暗淡 0x763968——此断言可作为"是否一次性归一化"的判别器。
    const u32 result = DyedItemColor::applyDyes(
        std::nullopt, {dyeTextureDiffuseColor(DyeColor::Red), dyeTextureDiffuseColor(DyeColor::Blue)});
    EXPECT_EQ(result, 0xAD5398u);
    EXPECT_NE(result, 0x763968u) << "退化为逐次两两平均（旧实现）";
}

TEST_F(DyedItemColorTest, MultipleDyes_OrderIndependent)
{
    // 一次性累加求平均 => 与染料顺序无关
    const u32 redBlue = DyedItemColor::applyDyes(
        std::nullopt, {dyeTextureDiffuseColor(DyeColor::Red), dyeTextureDiffuseColor(DyeColor::Blue)});
    const u32 blueRed = DyedItemColor::applyDyes(
        std::nullopt, {dyeTextureDiffuseColor(DyeColor::Blue), dyeTextureDiffuseColor(DyeColor::Red)});
    EXPECT_EQ(redBlue, blueRed);
}

TEST_F(DyedItemColorTest, MultipleDyes_BrightnessNormalization)
{
    // 白 + 黑：等量的极端明暗按 vanilla 归一化后是中亮灰（0x8B8E90），
    // 而非朴素平均得到的暗灰——验证 avgMax / maxAvg 亮度缩放确实生效。
    const u32 result = DyedItemColor::applyDyes(
        std::nullopt, {dyeTextureDiffuseColor(DyeColor::White), dyeTextureDiffuseColor(DyeColor::Black)});
    EXPECT_EQ(result, 0x8B8E90u);
}

TEST_F(DyedItemColorTest, ExistingColorParticipatesInAverage)
{
    // 二次染色：物品已有颜色（皮革默认 0xA06540）与染料一同参与平均。
    const u32 result = DyedItemColor::applyDyes(0xA06540u, {dyeTextureDiffuseColor(DyeColor::Red)});
    // (0xA06540 + 0xB02E26) 各通道平均后归一化
    EXPECT_EQ(result, 0xA84933u);
}

TEST_F(DyedItemColorTest, NoDye_ReturnsCurrentColor)
{
    // 无染料：返回当前颜色（未染色时为 0）
    EXPECT_EQ(DyedItemColor::applyDyes(0xA06540u, {}), 0xA06540u);
    EXPECT_EQ(DyedItemColor::applyDyes(std::nullopt, {}), 0u);
}

// ============================================================================
// ArmorDyeRecipe
// ============================================================================

class ArmorDyeRecipeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        Items::ensureInitialized();
        // DYEABLE 标签的成员集在 initialize 内部构建，供 _isDyeableArmor 查询
        item::tag::ItemTags::initialize();
    }

    void SetUp() override
    {
        m_recipe = std::make_unique<ArmorDyeRecipe>(ResourceLocation("minecraft", "armor_dye"));
    }

    std::unique_ptr<ArmorDyeRecipe> m_recipe;
};

TEST_F(ArmorDyeRecipeTest, BasicProperties)
{
    EXPECT_EQ(m_recipe->getId().toString(), "minecraft:armor_dye");
    EXPECT_TRUE(m_recipe->isSpecial());
}

TEST_F(ArmorDyeRecipeTest, Matches_ArmorAndDye)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _leatherChestplate());
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(ArmorDyeRecipeTest, Matches_MultipleDyes)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _leatherChestplate());
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::BLUE_DYE, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(ArmorDyeRecipeTest, DoesNotMatch_NoDye)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _leatherChestplate());

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(ArmorDyeRecipeTest, DoesNotMatch_NonDyeableItem)
{
    // 石头既非 DYEABLE 也非染料 => 不匹配
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::STONE, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(ArmorDyeRecipeTest, Matches_LeatherHorseArmor)
{
    // 皮革马铠是 DYEABLE 标签成员（HorseArmorItem，非 DyeableArmorItem 子类），
    // 旧实现用 dynamic_cast<DyeableArmorItem> 判定会漏掉它。
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::LEATHER_HORSE_ARMOR, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(ArmorDyeRecipeTest, Assemble_MixesMultipleDyes)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _leatherChestplate());
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::BLUE_DYE, 1));

    const ItemStack result = m_recipe->assemble(grid);
    ASSERT_FALSE(result.isEmpty());
    const auto color = _storedColor(result);
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(*color, 0xAD5398u) << "结果色调应与一次性归一化混色一致（旧逐次平均为 0x763968）";
}

TEST_F(ArmorDyeRecipeTest, Assemble_LeatherHorseArmor_GetsColored)
{
    // 皮革马铠染色：颜色写回 display.color
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::LEATHER_HORSE_ARMOR, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    const ItemStack result = m_recipe->assemble(grid);
    ASSERT_FALSE(result.isEmpty());
    const auto color = _storedColor(result);
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(*color, 0xB02E26u);
}

TEST_F(ArmorDyeRecipeTest, Assemble_ReDyeIncludesExistingColor)
{
    // 二次染色：已有颜色参与平均
    ItemStack armor = _leatherChestplate();
    item::items::DyeableArmorItem::setColor(armor, 0xA06540u);

    auto grid = _grid();
    grid.setItemAt(0, 0, armor);
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    const ItemStack result = m_recipe->assemble(grid);
    ASSERT_FALSE(result.isEmpty());
    const auto color = _storedColor(result);
    ASSERT_TRUE(color.has_value());
    EXPECT_EQ(*color, 0xA84933u);
}

TEST_F(ArmorDyeRecipeTest, GetRemainingItems_ConsumesAllDyes)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _leatherChestplate());
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    const auto remaining = m_recipe->getRemainingItems(grid);
    EXPECT_EQ(remaining.size(), static_cast<std::size_t>(grid.getContainerSize()));
    for (const auto& stack : remaining) {
        EXPECT_TRUE(stack.isEmpty());
    }
}