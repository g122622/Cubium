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

#pragma once

#include <string>

namespace mc {
namespace crafting {

/**
 * @file RecipeBookCategories.hpp
 * @brief 配方在配方书里的**页签分类**（配方数据的属性）。
 *
 * 与「哪个界面有配方书」（{@code RecipeBookType}，见 RecipeBook.hpp）是两回事：
 *
 * - 本文件的两个枚举写在配方 JSON 的 `category` 字段里，决定配方归到界面的哪个页签；
 * - {@code RecipeBookType} 决定哪一类界面可以打开 / 过滤配方书。
 *
 * 单独成文件是因为 {@code IRecipe} 需要它（每条配方都带分类），而 {@code RecipeBook}
 * 依赖 {@code IRecipe}（要查「是不是特殊配方」）——放进 RecipeBook.hpp 会形成循环包含。
 */

/**
 * @brief 工作台配方书里的页签分类。
 *
 * 对齐 MC 1.21.11 的 `net.minecraft.world.item.crafting.CraftingBookCategory`。
 */
enum class CraftingBookCategory : unsigned char {
    Building,  ///< 建筑
    Redstone,  ///< 红石
    Equipment, ///< 装备
    Misc,      ///< 杂项（缺省）
};

/**
 * @brief 熔炉类配方书里的页签分类。
 *
 * 对齐 MC 1.21.11 的 `net.minecraft.world.item.crafting.CookingBookCategory`。
 */
enum class CookingBookCategory : unsigned char {
    Food,   ///< 食物
    Blocks, ///< 方块
    Misc,   ///< 杂项（缺省）
};

/// 工作台页签分类的字符串表示（`building` / `redstone` / `equipment` / `misc`）。
[[nodiscard]] const char* craftingBookCategoryToString(CraftingBookCategory category) noexcept;

/// 解析工作台页签分类；无法识别时返回 `Misc`（与原版 `orElse(MISC)` 一致）。
[[nodiscard]] CraftingBookCategory craftingBookCategoryFromString(const std::string& str) noexcept;

/// 熔炉页签分类的字符串表示（`food` / `blocks` / `misc`）。
[[nodiscard]] const char* cookingBookCategoryToString(CookingBookCategory category) noexcept;

/// 解析熔炉页签分类；无法识别时返回 `Misc`（与原版 `orElse(MISC)` 一致）。
[[nodiscard]] CookingBookCategory cookingBookCategoryFromString(const std::string& str) noexcept;

/**
 * @brief 配方在配方书里的**最终归属**（类型 + 页签分类合成的资源位置）。
 *
 * 对齐原版 `RecipeBookCategories` 注册表：`Recipe#recipeBookCategory()` 把「配方类型」
 * 与「页签分类」两个维度压成一个资源位置，例如有序合成的 `equipment` 页签得到
 * `minecraft:crafting_equipment`、烟熏炉得到 `minecraft:smoker_food`。
 *
 * 这是客户端渲染配方书时真正用的键，因此比「分类枚举取值」更接近玩家的可见结果：
 * 分类解析对了但类型映射错了（把烟熏炉算成熔炉），在这里才会暴露。
 */
enum class RecipeBookPage : unsigned char {
    CraftingBuildingBlocks,
    CraftingRedstone,
    CraftingEquipment,
    CraftingMisc,
    FurnaceFood,
    FurnaceBlocks,
    FurnaceMisc,
    BlastFurnaceBlocks,
    BlastFurnaceMisc,
    SmokerFood,
    Stonecutter,
    Smithing,
    Campfire,
};

/// 页签归属的资源路径（不含 `minecraft:` 前缀）。
[[nodiscard]] const char* recipeBookPageToPath(RecipeBookPage page) noexcept;


} // namespace crafting
} // namespace mc
