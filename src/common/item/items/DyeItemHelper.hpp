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

#include "common/core/Types.hpp"
#include "common/util/color/DyeColor.hpp"

#include <optional>

namespace mc {

class Item;

namespace item {

/**
 * @brief 染料物品判定与颜色查询
 *
 * 1.21.11 的染料是 DyeItem 子类（`item instanceof DyeItem`）；本项目未建模 DyeItem 类，
 * 染料在注册表里是普通 Item，故此处以物品指针集合表达同一语义，供所有"物品→染料"
 * 判定点复用（盔甲染色配方、烟花之星配方、烟花之星渐变色配方、织布机染色槽、
 * 狼/猫项圈染色、狼铠染色）。
 *
 * 覆盖的物品集合与 vanilla 的 DyeItem 实例完全一致：仅 16 种 `*_dye`。
 * ink_sac / cocoa_beans / bone_meal 在 vanilla 中**不是** DyeItem（分别是 InkSacItem /
 * BlockItem / BoneMealItem），它们只在合成配方里作为对应染料的原料，不能直接用于
 * `instanceof DyeItem` 判定点（项圈染色、盔甲染色、织布机等）。
 */
class DyeItemHelper {
public:
    /**
     * @brief 判断物品是否为染料
     *
     * @param item 物品指针（可为 nullptr）
     * @return 是染料返回 true
     */
    [[nodiscard]] static bool isDyeItem(const Item* item) noexcept;

    /**
     * @brief 查询染料物品对应的颜色
     *
     * @param item 物品指针（可为 nullptr）
     * @return 对应染料颜色；非染料返回 std::nullopt
     */
    [[nodiscard]] static std::optional<DyeColor> dyeColorOfItem(const Item* item) noexcept;
};

} // namespace item
} // namespace mc
