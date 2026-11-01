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

#include "RecipeBookCategories.hpp"

namespace mc {
namespace crafting {

const char* craftingBookCategoryToString(CraftingBookCategory category) noexcept
{
    switch (category) {
        case CraftingBookCategory::Building:
            return "building";
        case CraftingBookCategory::Redstone:
            return "redstone";
        case CraftingBookCategory::Equipment:
            return "equipment";
        case CraftingBookCategory::Misc:
        default:
            return "misc";
    }
}

CraftingBookCategory craftingBookCategoryFromString(const std::string& str) noexcept
{
    if (str == "building") {
        return CraftingBookCategory::Building;
    }
    if (str == "redstone") {
        return CraftingBookCategory::Redstone;
    }
    if (str == "equipment") {
        return CraftingBookCategory::Equipment;
    }
    // 原版 `CraftingBookCategory.CODEC` 是 `StringRepresentable.fromEnum`：取值不在集合内
    // 会让整条配方解析失败。但数据包里 `category` 是 optional 且缺省为 MISC，所以「字段
    // 缺失」与「取值非法」在实际数据上走的是同一条路径——这里把两者都折成 MISC，与
    // `CraftingBookCategory.CODEC.fieldOf("category").orElse(MISC)` 等价。
    return CraftingBookCategory::Misc;
}

const char* cookingBookCategoryToString(CookingBookCategory category) noexcept
{
    switch (category) {
        case CookingBookCategory::Food:
            return "food";
        case CookingBookCategory::Blocks:
            return "blocks";
        case CookingBookCategory::Misc:
        default:
            return "misc";
    }
}

CookingBookCategory cookingBookCategoryFromString(const std::string& str) noexcept
{
    if (str == "food") {
        return CookingBookCategory::Food;
    }
    if (str == "blocks") {
        return CookingBookCategory::Blocks;
    }
    return CookingBookCategory::Misc;
}

const char* recipeBookPageToPath(RecipeBookPage page) noexcept
{
    switch (page) {
        case RecipeBookPage::CraftingBuildingBlocks:
            return "crafting_building_blocks";
        case RecipeBookPage::CraftingRedstone:
            return "crafting_redstone";
        case RecipeBookPage::CraftingEquipment:
            return "crafting_equipment";
        case RecipeBookPage::CraftingMisc:
            return "crafting_misc";
        case RecipeBookPage::FurnaceFood:
            return "furnace_food";
        case RecipeBookPage::FurnaceBlocks:
            return "furnace_blocks";
        case RecipeBookPage::FurnaceMisc:
            return "furnace_misc";
        case RecipeBookPage::BlastFurnaceBlocks:
            return "blast_furnace_blocks";
        case RecipeBookPage::BlastFurnaceMisc:
            return "blast_furnace_misc";
        case RecipeBookPage::SmokerFood:
            return "smoker_food";
        case RecipeBookPage::Stonecutter:
            return "stonecutter";
        case RecipeBookPage::Smithing:
            return "smithing";
        case RecipeBookPage::Campfire:
            return "campfire";
        default:
            return "crafting_misc";
    }
}

} // namespace crafting
} // namespace mc
