/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "common/item/loot/conditions/ReferenceCondition.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"

namespace mc {
namespace loot {

ReferenceCondition::ReferenceCondition(const std::string& name)
    : m_name(name)
{}

bool ReferenceCondition::test(LootContext& context) const
{
    MC_UNUSED(context);
    // TODO: 实现谓词查找和条件测试，当前总是返回 true
    return true;
}

std::unique_ptr<LootCondition> ReferenceCondition::clone() const
{
    return std::make_unique<ReferenceCondition>(m_name);
}

} // namespace loot
} // namespace mc
