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

#include "ItemTooltipBuilder.hpp"

#include "common/item/core/Item.hpp"
#include <algorithm>
#include <string>

namespace mc::client::ui::kagero::widget {

Tooltip ItemTooltipBuilder::build(const mc::ItemStack& stack, mc::IWorld* world)
{
    if (stack.isEmpty()) {
        return Tooltip{};
    }

    std::vector<std::string> lines;

    // 显示名（第一行）
    auto displayName = stack.getDisplayName();
    lines.emplace_back(displayName ? displayName->getUnformattedText() : "");

    // 数量
    if (stack.getCount() > 1) {
        lines.emplace_back("Count: " + std::to_string(stack.getCount()));
    }

    // 耐久度
    if (stack.isDamageable() && stack.getMaxDamage() > 0) {
        const i32 remainingDurability = std::max(0, stack.getMaxDamage() - stack.getDamage());
        lines.emplace_back(
            "Durability: " + std::to_string(remainingDurability) + "/" + std::to_string(stack.getMaxDamage()));
    }

    // 物品自定义 tooltip（对应 MC Item#appendHoverText）
    // world 为 null 时对应 MC 的 EMPTY TooltipContext，子类按需跳过依赖世界的逻辑。
    if (stack.getItem() != nullptr) {
        stack.getItem()->addInformation(stack, world, lines, false);
    }

    return Tooltip::create(std::move(lines));
}

} // namespace mc::client::ui::kagero::widget
