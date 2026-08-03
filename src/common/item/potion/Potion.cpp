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

#include "Potion.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {
namespace potion {

// ========== Potion 实现 ==========

Potion::Potion()
    : m_baseName("")
    , m_effects()
{}

Potion::Potion(std::string_view baseName)
    : m_baseName(baseName)
    , m_effects()
{}

Potion::Potion(std::string_view baseName, std::vector<entity::effect::EffectInstance> effects)
    : m_baseName(baseName)
    , m_effects(std::move(effects))
{}

Potion::Potion(const entity::effect::EffectInstance& effect)
    : m_baseName("")
    , m_effects({effect})
{}

bool Potion::hasInstantEffect() const
{
    for (const auto& effect : m_effects) {
        // 瞬间治疗和瞬间伤害是瞬间效果
        if (effect.type() == entity::effect::EffectType::InstantHealth ||
            effect.type() == entity::effect::EffectType::InstantDamage) {
            return true;
        }
    }
    return false;
}

std::string Potion::getNamePrefixed(std::string_view prefix) const
{
    if (m_baseName.empty()) {
        return std::string(prefix);
    }
    return std::string(prefix) + ".effect.minecraft." + m_baseName;
}

} // namespace potion
} // namespace mc
