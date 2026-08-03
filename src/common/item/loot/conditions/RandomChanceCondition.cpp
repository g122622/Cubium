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

#include "common/item/loot/conditions/RandomChanceCondition.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include <memory>

namespace mc {
namespace loot {

RandomChanceCondition::RandomChanceCondition(f32 chance, bool affectedByLuck)
    : m_chance(chance)
    , m_affectedByLuck(affectedByLuck)
{}

bool RandomChanceCondition::test(LootContext& context) const
{
    f32 actualChance = m_chance;

    if (m_affectedByLuck) {
        // 幸运值增加概率
        actualChance += context.getLuck();
    }

    return context.getRandom().nextFloat() < actualChance;
}

std::unique_ptr<LootCondition> RandomChanceCondition::clone() const noexcept
{
    return std::make_unique<RandomChanceCondition>(m_chance, m_affectedByLuck);
}

} // namespace loot
} // namespace mc
