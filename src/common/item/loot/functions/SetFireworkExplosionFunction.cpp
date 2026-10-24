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

#include "SetFireworkExplosionFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>

namespace mc {
namespace loot {

SetFireworkExplosionFunction::SetFireworkExplosionFunction(std::optional<item::component::FireworkShape> shape,
    std::optional<std::vector<i32>> colors,
    std::optional<std::vector<i32>> fadeColors,
    std::optional<bool> trail,
    std::optional<bool> twinkle)
    : m_shape(shape)
    , m_colors(std::move(colors))
    , m_fadeColors(std::move(fadeColors))
    , m_trail(trail)
    , m_twinkle(twinkle)
{}

ItemStack SetFireworkExplosionFunction::apply(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);
    if (stack.isEmpty()) {
        return stack;
    }
    item::component::FireworkExplosion explosion =
        stack.getFireworkExplosion() != nullptr ? *stack.getFireworkExplosion() : item::component::FireworkExplosion{};
    if (m_shape.has_value()) {
        explosion.shape = *m_shape;
    }
    if (m_colors.has_value()) {
        explosion.colors = *m_colors;
    }
    if (m_fadeColors.has_value()) {
        explosion.fadeColors = *m_fadeColors;
    }
    if (m_trail.has_value()) {
        explosion.hasTrail = *m_trail;
    }
    if (m_twinkle.has_value()) {
        explosion.hasTwinkle = *m_twinkle;
    }
    stack.setFireworkExplosion(explosion);
    return stack;
}

std::unique_ptr<LootFunction> SetFireworkExplosionFunction::clone() const noexcept
{
    auto func = std::make_unique<SetFireworkExplosionFunction>(m_shape, m_colors, m_fadeColors, m_trail, m_twinkle);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
