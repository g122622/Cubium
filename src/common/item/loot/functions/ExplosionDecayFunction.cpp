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

#include "ExplosionDecayFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>

namespace mc {
namespace loot {

ItemStack ExplosionDecayFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 获取爆炸半径参数，如果没有则默认为 1.0（100% 保留）
    f32 explosionRadius = 1.0f;
    auto* radiusParam = context.get<f32>(LootParams::EXPLOSION_RADIUS);
    if (radiusParam != nullptr && *radiusParam > 0.0f) {
        explosionRadius = *radiusParam;
    }

    // 使用二项分布计算保留数量
    f32 keepChance = math::clamp(1.0f / explosionRadius, 0.0f, 1.0f);
    BinomialRange binomial(stack.getCount(), keepChance);
    i32 keptCount = binomial.generateInt(context.getRandom());

    if (keptCount <= 0) {
        return ItemStack();
    }

    stack.setCount(keptCount);
    return stack;
}

std::unique_ptr<LootFunction> ExplosionDecayFunction::clone() const noexcept
{
    auto func = std::make_unique<ExplosionDecayFunction>();
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
