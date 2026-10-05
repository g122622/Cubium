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

#include "common/item/component/AttackRange.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"

#include <cmath>

namespace mc {
namespace item {
namespace component {

f32 AttackRange::effectiveMinRange(const Entity& attacker) const noexcept
{
    // 玩家按创造/旁观模式取 creative 值（对应 vanilla instanceof Player 分支）
    const auto* player = dynamic_cast<const Player*>(&attacker);
    if (player != nullptr) {
        if (player->isSpectator()) {
            return 0.0f;
        }
        return player->isCreative() ? minCreativeRange : minRange;
    }
    return minRange * mobFactor;
}

f32 AttackRange::effectiveMaxRange(const Entity& attacker) const noexcept
{
    const auto* player = dynamic_cast<const Player*>(&attacker);
    if (player != nullptr) {
        return player->isCreative() ? maxCreativeRange : maxRange;
    }
    return maxRange * mobFactor;
}

bool AttackRange::isInRange(const LivingEntity& attacker, const AxisAlignedBB& target, f64 padding) const noexcept
{
    // 眼睛位置 = 实体位置 + 眼睛高度（与 Player::getEyePosition 一致，对所有生物通用）
    const Vector3 eye(attacker.x(), static_cast<f32>(attacker.getEyeY()), attacker.z());
    // 眼睛到目标碰撞箱的最近距离（AABB.distanceToSqr 语义：点在盒内返回 0）
    const f64 distSq = static_cast<f64>(target.distanceToSqr(eye));
    const f64 dist = std::sqrt(distSq);

    const f64 minEffective = static_cast<f64>(effectiveMinRange(attacker)) - static_cast<f64>(hitboxMargin) - padding;
    const f64 maxEffective = static_cast<f64>(effectiveMaxRange(attacker)) + static_cast<f64>(hitboxMargin) + padding;
    return dist >= minEffective && dist <= maxEffective;
}

} // namespace component
} // namespace item
} // namespace mc
