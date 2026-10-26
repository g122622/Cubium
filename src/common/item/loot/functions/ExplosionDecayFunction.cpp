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

ItemStack ExplosionDecayFunction::run(ItemStack stack, LootContext& context) const
{
    // 对齐原版 `ApplyExplosionDecay#run`：
    //   Float f = ctx.getOptionalParameter(EXPLOSION_RADIUS);
    //   if (f != null) { ...每个物品按 1/f 的概率保留... }
    // **没有爆炸半径时函数整体不生效**（既不消耗随机数、也不改动数量）。
    // 早先的实现把缺省半径当成 1.0 再跑一遍二项分布，虽然数量不变，
    // 却会额外消耗 count 次随机数，导致后续所有抽取与原版错位。
    const f32* radiusParam = context.get<f32>(LootParams::EXPLOSION_RADIUS);
    if (radiusParam == nullptr || stack.isEmpty()) {
        return stack;
    }

    const f32 keepChance = 1.0f / *radiusParam;
    const i32 count = stack.getCount();
    i32 kept = 0;
    for (i32 i = 0; i < count; ++i) {
        if (context.getRandom().nextFloat() <= keepChance) {
            ++kept;
        }
    }
    stack.setCount(kept);
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
