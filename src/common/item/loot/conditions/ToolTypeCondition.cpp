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

#include "common/item/loot/conditions/ToolTypeCondition.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/tool/ToolItem.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include <memory>

namespace mc {
namespace loot {

ToolTypeCondition::ToolTypeCondition(u8 toolType)
    : m_toolType(toolType)
{}

bool ToolTypeCondition::test(LootContext& context) const
{
    // 从上下文获取工具参数
    auto* tool = context.get<ItemStack>(LootParams::TOOL);
    if (!tool || tool->isEmpty()) {
        // 空手不满足任何工具类型条件
        return false;
    }

    // 获取物品
    const Item* item = tool->getItem();
    if (!item) {
        return false;
    }

    // 检查是否为工具物品，并获取工具类型
    const item::tool::ToolItem* toolItem = dynamic_cast<const item::tool::ToolItem*>(item);
    if (toolItem) {
        return static_cast<u8>(toolItem->getToolType()) == m_toolType;
    }

    // 非工具物品不满足工具类型条件
    return false;
}

std::unique_ptr<LootCondition> ToolTypeCondition::clone() const noexcept
{
    return std::make_unique<ToolTypeCondition>(m_toolType);
}

} // namespace loot
} // namespace mc
