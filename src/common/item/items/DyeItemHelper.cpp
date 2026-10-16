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

#include "common/item/items/DyeItemHelper.hpp"

#include "common/item/Items.hpp"
#include <unordered_map>

namespace mc {
namespace item {

namespace {

/// 物品 → 染料颜色映射（与 vanilla 的 DyeItem 实例集合一一对应：仅 16 种 *_dye）
///
/// 注意：ink_sac / cocoa_beans / bone_meal 在 vanilla 中**不是** DyeItem 实例
/// （分别是 InkSacItem / BlockItem / BoneMealItem），故不在此集合内。它们虽各自对应
/// 一种染料色，但只在合成配方（如 black_dye ← ink_sac）里作为原料，不能直接用于
/// 项圈染色、盔甲染色、织布机染色等 `instanceof DyeItem` 判定点。
const std::unordered_map<const Item*, DyeColor>& _dyeColorMap()
{
    static const std::unordered_map<const Item*, DyeColor> map = {
        {Items::WHITE_DYE, DyeColor::White},
        {Items::ORANGE_DYE, DyeColor::Orange},
        {Items::MAGENTA_DYE, DyeColor::Magenta},
        {Items::LIGHT_BLUE_DYE, DyeColor::LightBlue},
        {Items::YELLOW_DYE, DyeColor::Yellow},
        {Items::LIME_DYE, DyeColor::Lime},
        {Items::PINK_DYE, DyeColor::Pink},
        {Items::GRAY_DYE, DyeColor::Gray},
        {Items::LIGHT_GRAY_DYE, DyeColor::LightGray},
        {Items::CYAN_DYE, DyeColor::Cyan},
        {Items::PURPLE_DYE, DyeColor::Purple},
        {Items::BLUE_DYE, DyeColor::Blue},
        {Items::BROWN_DYE, DyeColor::Brown},
        {Items::GREEN_DYE, DyeColor::Green},
        {Items::RED_DYE, DyeColor::Red},
        {Items::BLACK_DYE, DyeColor::Black},
    };
    return map;
}

} // namespace

bool DyeItemHelper::isDyeItem(const Item* item) noexcept
{
    return item != nullptr && _dyeColorMap().find(item) != _dyeColorMap().end();
}

std::optional<DyeColor> DyeItemHelper::dyeColorOfItem(const Item* item) noexcept
{
    if (item == nullptr) {
        return std::nullopt;
    }
    const auto it = _dyeColorMap().find(item);
    if (it == _dyeColorMap().end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace item
} // namespace mc
