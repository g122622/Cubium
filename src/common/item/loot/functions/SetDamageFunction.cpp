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

#include "SetDamageFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>

namespace mc {
namespace loot {

SetDamageFunction::SetDamageFunction(const RandomValueRange& durability, bool add)
    : m_durability(durability)
    , m_add(add)
{}

ItemStack SetDamageFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty() || !stack.isDamageable()) {
        return stack;
    }

    i32 maxDamage = stack.getMaxDamage();
    if (maxDamage <= 0) {
        return stack;
    }

    // 生成损坏程度 (0.0 = 完好, 1.0 = 完全损坏)
    f32 durabilityRatio = m_durability.generateFloat(context.getRandom());
    durabilityRatio = math::clamp(durabilityRatio, 0.0f, 1.0f);

    // 计算实际耐久度
    i32 damage = static_cast<i32>((1.0f - durabilityRatio) * static_cast<f32>(maxDamage));

    if (m_add) {
        stack.setDamage(stack.getDamage() + damage);
    } else {
        stack.setDamage(damage);
    }

    // 确保不超过最大耐久度
    if (stack.getDamage() >= maxDamage) {
        stack.setDamage(maxDamage - 1);
    }

    return stack;
}

std::unique_ptr<LootFunction> SetDamageFunction::clone() const noexcept
{
    auto func = std::make_unique<SetDamageFunction>(m_durability, m_add);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
