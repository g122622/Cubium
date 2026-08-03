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

#include "common/item/loot/conditions/SurvivesExplosionCondition.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include <memory>

namespace mc {
namespace loot {

bool SurvivesExplosionCondition::test(LootContext& context) const
{
    // 检查上下文中是否有爆炸半径参数
    auto* radius = context.get<f32>(LootParams::EXPLOSION_RADIUS);
    if (!radius) {
        // 非爆炸破坏，物品总是存活
        return true;
    }

    // 爆炸半径为0或负数，物品存活
    if (*radius <= 0.0f) {
        return true;
    }

    // 以 1/radius 的概率存活
    const f32 chance = 1.0f / *radius;
    const f32 roll = context.getRandom().nextFloat();
    return roll < chance;
}

std::unique_ptr<LootCondition> SurvivesExplosionCondition::clone() const noexcept
{
    return std::make_unique<SurvivesExplosionCondition>();
}

} // namespace loot
} // namespace mc
