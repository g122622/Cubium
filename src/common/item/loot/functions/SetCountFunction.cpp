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

#include "SetCountFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>

namespace mc {
namespace loot {

SetCountFunction::SetCountFunction(const RandomValueRange& count, bool add)
    : m_count(count)
    , m_add(add)
{}

ItemStack SetCountFunction::apply(ItemStack stack, LootContext& context) const
{
    // 空物品堆直接返回
    if (stack.isEmpty()) {
        return stack;
    }

    // 生成随机数量
    i32 newCount = m_count.generateInt(context.getRandom());

    // 根据 add 模式决定是叠加还是替换
    if (m_add) {
        stack.grow(newCount);
    } else {
        stack.setCount(newCount);
    }

    return stack;
}

std::unique_ptr<LootFunction> SetCountFunction::clone() const noexcept
{
    // 深拷贝函数及其条件
    auto func = std::make_unique<SetCountFunction>(m_count, m_add);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
