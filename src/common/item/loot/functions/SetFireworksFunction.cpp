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

#include "SetFireworksFunction.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <algorithm>
#include <memory>

namespace mc {
namespace loot {

SetFireworksFunction::SetFireworksFunction(
    std::optional<ExplosionListOperation> explosions, std::optional<i32> flightDuration)
    : m_explosions(std::move(explosions))
    , m_flightDuration(flightDuration)
{}

ItemStack SetFireworksFunction::apply(ItemStack stack, LootContext& context) const
{
    MC_UNUSED(context);
    if (stack.isEmpty()) {
        return stack;
    }

    // vanilla：以物品当前 fireworks（或 DEFAULT_VALUE）为底，仅覆盖显式给出的字段
    const auto* existing = stack.getFireworks();
    item::component::Fireworks fireworks = existing != nullptr ? *existing : item::component::Fireworks{};
    if (m_flightDuration.has_value()) {
        fireworks.flightDuration = static_cast<u8>(std::clamp(*m_flightDuration, 0, 255));
    }
    if (m_explosions.has_value()) {
        auto& list = fireworks.explosions;
        switch (m_explosions->kind) {
            case ListOperationKind::ReplaceAll:
                list = m_explosions->explosions;
                break;
            case ListOperationKind::Append:
                list.insert(list.end(), m_explosions->explosions.begin(), m_explosions->explosions.end());
                break;
            case ListOperationKind::Insert: {
                const auto position =
                    std::min<std::size_t>(static_cast<std::size_t>(std::max(m_explosions->offset, 0)), list.size());
                list.insert(list.begin() + static_cast<std::ptrdiff_t>(position),
                    m_explosions->explosions.begin(),
                    m_explosions->explosions.end());
                break;
            }
        }
        if (list.size() > static_cast<std::size_t>(item::component::Fireworks::MAX_EXPLOSIONS)) {
            list.resize(static_cast<std::size_t>(item::component::Fireworks::MAX_EXPLOSIONS));
        }
    }
    stack.setFireworks(fireworks);
    return stack;
}

std::unique_ptr<LootFunction> SetFireworksFunction::clone() const noexcept
{
    auto func = std::make_unique<SetFireworksFunction>(m_explosions, m_flightDuration);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

} // namespace loot
} // namespace mc
