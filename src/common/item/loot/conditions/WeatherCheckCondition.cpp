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

#include "common/item/loot/conditions/WeatherCheckCondition.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <optional>
#include <utility>

namespace mc {
namespace loot {

WeatherCheckCondition::WeatherCheckCondition(std::optional<bool> raining, std::optional<bool> thundering)
    : m_raining(std::move(raining))
    , m_thundering(std::move(thundering))
{}

bool WeatherCheckCondition::test(LootContext& context) const
{
    IWorld& world = context.getWorld();

    if (m_raining.has_value() && *m_raining != world.isRaining()) {
        return false;
    }

    if (m_thundering.has_value() && *m_thundering != world.isThundering()) {
        return false;
    }

    return true;
}

std::unique_ptr<LootCondition> WeatherCheckCondition::clone() const noexcept
{
    return std::make_unique<WeatherCheckCondition>(m_raining, m_thundering);
}

} // namespace loot
} // namespace mc
