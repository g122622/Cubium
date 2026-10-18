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

#include "LootFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 切换组件提示显示（minecraft:toggle_tooltips）
 *
 * 对应原版 ToggleTooltips：把组件写进 tooltip_display 的 hidden_components 列表。
 * 本实现以 custom_data 中的内部标记承载提示隐藏集合（项目暂无国际化提示渲染，
 * 组件本身仍正常存在于物品上）。
 *
 * JSON：{ "toggles": { "minecraft:enchantments": false } }
 */
class ToggleTooltipsFunction : public LootFunction {
public:
    explicit ToggleTooltipsFunction(std::vector<std::pair<item::component::DataComponentType, bool>> toggles);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const noexcept override { return "toggle_tooltips"; }

    [[nodiscard]] const std::vector<std::pair<item::component::DataComponentType, bool>>& getToggles() const
    {
        return m_toggles;
    }

private:
    std::vector<std::pair<item::component::DataComponentType, bool>> m_toggles;
};

} // namespace loot
} // namespace mc
