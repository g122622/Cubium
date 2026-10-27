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

#include "common/world/village/trade/ItemCost.hpp"

#include <utility>

namespace mc {
namespace world {
namespace village {
namespace trade {

ItemCost::ItemCost(ItemStack stack, i32 count)
    : m_stack(std::move(stack))
    , m_count(count)
{
    m_stack.setCount(count);
}

ItemCost::ItemCost(ItemStack stack)
    : m_stack(std::move(stack))
    , m_count(m_stack.getCount())
{}

ItemStack ItemCost::toStack() const
{
    return m_stack.copy();
}

bool ItemCost::test(const ItemStack& stack) const
{
    if (stack.isEmpty() || stack.getCount() < m_count) {
        return false;
    }
    if (!stack.isSameItem(m_stack)) {
        return false;
    }
    return _testComponents(stack);
}

bool ItemCost::testComponentwise(const ItemStack& stack) const
{
    if (stack.isEmpty() || !stack.isSameItem(m_stack)) {
        return false;
    }
    return _testComponents(stack);
}

bool ItemCost::_testComponents(const ItemStack& stack) const
{
    // potion_contents 组件：空串表示"无药水内容"，与期望值不符
    if (m_potionId.has_value() && stack.getPotionId() != *m_potionId) {
        return false;
    }

    // custom_data 组件：根标签必须包含全部期望键（深度相等）
    if (m_customData.is_object() && !m_customData.empty()) {
        const nlohmann::json* tag = stack.getTag();
        if (tag == nullptr || !tag->is_object()) {
            return false;
        }
        for (auto iter = m_customData.begin(); iter != m_customData.end(); ++iter) {
            auto found = tag->find(iter.key());
            if (found == tag->end() || *found != iter.value()) {
                return false;
            }
        }
    }

    return true;
}

ItemCost& ItemCost::withPotion(const std::string& potionId)
{
    m_potionId = potionId;
    m_stack.setPotionId(potionId);
    return *this;
}

ItemCost& ItemCost::withCustomData(nlohmann::json data)
{
    m_customData = std::move(data);
    return *this;
}

} // namespace trade
} // namespace village
} // namespace world
} // namespace mc
