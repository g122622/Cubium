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
 * The above copyright notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "common/item/loot/conditions/ReferenceCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace mc {
namespace loot {

ReferenceCondition::ReferenceCondition(const std::string& name)
    : m_name(name)
{}

bool ReferenceCondition::test(LootContext& context) const
{
    // 通过谓词解析器查找命名谓词
    const LootCondition* predicate = context.getPredicate(m_name);
    if (predicate == nullptr) {
        // 谓词未找到，返回 false 并记录警告
        // 参考 MC: ConditionReference.test() 在找不到谓词时返回 false
        spdlog::warn("ReferenceCondition: unknown predicate '{}', returning false", m_name);
        return false;
    }

    // 循环引用检测：如果当前谓词已经在访问栈中，说明发生了无限循环
    if (!context.pushPredicate(predicate)) {
        spdlog::warn("ReferenceCondition: detected infinite loop in predicate reference '{}', returning false", m_name);
        return false;
    }

    // 执行引用的谓词条件
    bool result = predicate->test(context);

    // 从访问栈中移除
    context.popPredicate(predicate);

    return result;
}

std::unique_ptr<LootCondition> ReferenceCondition::clone() const noexcept
{
    return std::make_unique<ReferenceCondition>(m_name);
}

} // namespace loot
} // namespace mc
