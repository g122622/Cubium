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

#include "ToggleTooltipsFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <algorithm>
#include <memory>

namespace mc {
namespace loot {

namespace {
/// 隐藏提示组件集合在 custom_data 中的键
constexpr const char* HIDDEN_COMPONENTS_KEY = "minecraft:tooltip_display.hidden_components";
} // namespace

ToggleTooltipsFunction::ToggleTooltipsFunction(std::vector<std::pair<item::component::DataComponentType, bool>> toggles)
    : m_toggles(std::move(toggles))
{}

ItemStack ToggleTooltipsFunction::apply(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);
    if (stack.isEmpty()) {
        return stack;
    }
    // hidden = 互斥集合：value=false 表示隐藏该组件的提示，true 表示恢复显示
    std::vector<std::string> hidden;
    auto& tag = stack.getOrCreateTag();
    if (tag.contains(HIDDEN_COMPONENTS_KEY) && tag[HIDDEN_COMPONENTS_KEY].is_array()) {
        hidden = tag[HIDDEN_COMPONENTS_KEY].get<std::vector<std::string>>();
    }
    for (const auto& [type, visible] : m_toggles) {
        auto name = item::component::componentName(type);
        if (!name.has_value()) {
            continue;
        }
        const std::string key(*name);
        auto it = std::find(hidden.begin(), hidden.end(), key);
        if (!visible && it == hidden.end()) {
            hidden.push_back(key);
        } else if (visible && it != hidden.end()) {
            hidden.erase(it);
        }
    }
    tag[HIDDEN_COMPONENTS_KEY] = hidden;
    return stack;
}

std::unique_ptr<LootFunction> ToggleTooltipsFunction::clone() const noexcept
{
    auto func = std::make_unique<ToggleTooltipsFunction>(m_toggles);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
