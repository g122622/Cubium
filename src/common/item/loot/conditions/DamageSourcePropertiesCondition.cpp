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

#include "common/item/loot/conditions/DamageSourcePropertiesCondition.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace loot {

DamageSourcePropertiesCondition::DamageSourcePropertiesCondition(advancement::DamageSourcePredicate predicate)
    : m_predicate(std::move(predicate))
    , m_isAny(m_predicate.isAny())
{}

bool DamageSourcePropertiesCondition::test(LootContext& context) const
{
    // 从上下文获取伤害源
    auto* damageSource = context.get<DamageSource>(LootParams::DAMAGE_SOURCE);
    if (!damageSource) {
        return false;
    }

    // 如果谓词匹配任何伤害源，直接返回 true
    if (m_isAny) {
        return true;
    }

    // 使用谓词检查伤害源属性
    return m_predicate.test(*damageSource);
}

std::unique_ptr<LootCondition> DamageSourcePropertiesCondition::clone() const noexcept
{
    return std::make_unique<DamageSourcePropertiesCondition>(m_predicate);
}

} // namespace loot
} // namespace mc
