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

#include "common/item/loot/conditions/LocationCheckCondition.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace loot {

LocationCheckCondition::LocationCheckCondition(
    advancement::LocationPredicate predicate, i32 offsetX, i32 offsetY, i32 offsetZ)
    : m_predicate(std::move(predicate))
    , m_offsetX(offsetX)
    , m_offsetY(offsetY)
    , m_offsetZ(offsetZ)
    , m_isAny(m_predicate.isAny())
{}

bool LocationCheckCondition::test(LootContext& context) const
{
    // 从 BLOCK_POS 获取位置
    auto* blockPos = context.get<BlockPos>(LootParams::BLOCK_POS);
    if (blockPos) {
        f64 x = static_cast<f64>(blockPos->x + m_offsetX);
        f64 y = static_cast<f64>(blockPos->y + m_offsetY);
        f64 z = static_cast<f64>(blockPos->z + m_offsetZ);

        if (m_isAny) {
            return true;
        }
        return m_predicate.test(context.getWorld(), x, y, z);
    }

    // 从实体获取位置
    auto* entity = context.get<Entity>(LootParams::THIS_ENTITY);
    if (entity) {
        f64 x = static_cast<f64>(entity->x()) + static_cast<f64>(m_offsetX);
        f64 y = static_cast<f64>(entity->y()) + static_cast<f64>(m_offsetY);
        f64 z = static_cast<f64>(entity->z()) + static_cast<f64>(m_offsetZ);

        if (m_isAny) {
            return true;
        }
        return m_predicate.test(context.getWorld(), x, y, z);
    }

    return false;
}

std::unique_ptr<LootCondition> LocationCheckCondition::clone() const noexcept
{
    return std::make_unique<LocationCheckCondition>(m_predicate, m_offsetX, m_offsetY, m_offsetZ);
}

} // namespace loot
} // namespace mc
