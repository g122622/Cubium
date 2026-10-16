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
 * 判定点复用（盔甲染色配方、烟花之星配方、烟花之星渐变色配方）。
 *
 * 覆盖的物品集合与 vanilla 的 DyeItem 实例一致：16 种 *_dye，外加 ink_sac（黑）与
 * cocoa_beans（棕）——后两者在 vanilla 中同样是 DyeItem 实例。
 *
 * @note 项目同时注册了旧名 `minecraft:lapis_lazuli_dye` 与 1.21.11 的 `minecraft:blue_dye`，
 * 二者都是蓝色染料（见 .cpp 的 TODO(命名迁移)）。
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
