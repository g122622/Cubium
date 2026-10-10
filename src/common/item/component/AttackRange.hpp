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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {

class Entity;
class LivingEntity;

namespace item {
namespace component {

/**
 * @brief 攻击范围组件（1.21.11 net.minecraft.world.item.component.AttackRange）
 *
 * 描述持有该物品的生物的近战攻击有效距离区间。语义：目标与攻击者眼睛的距离
 * d（取目标碰撞箱到眼睛的最近距离）须落在
 *   [effectiveMinRange - hitboxMargin, effectiveMaxRange + hitboxMargin]
 * 区间内才算命中。玩家与生物的有效范围不同：玩家按创造/旁观模式取 creative 值，
 * 生物按 mobFactor 缩放。
 *
 * 仅 spear 等物品显式携带该组件；未携带时由 LivingEntity::entityAttackRange 回退
 * 到基于 generic.entity_interaction_range 属性的默认值（见 defaultFor）。
 *
 * 字段默认值对齐 vanilla CODEC：
 *   min_reach=0.0 max_reach=3.0 min_creative_reach=0.0 max_creative_reach=5.0
 *   hitbox_margin=0.3 mob_factor=1.0
 */
struct AttackRange {
    f32 minRange = 0.0f;
    f32 maxRange = 3.0f;
    f32 minCreativeRange = 0.0f;
    f32 maxCreativeRange = 5.0f;
    f32 hitboxMargin = 0.3f;
    f32 mobFactor = 1.0f;

    /**
     * @brief 基于实体交互距离属性构造默认攻击范围
     *
     * 对应 vanilla AttackRange.defaultFor(LivingEntity)：min/max 与
     * min/maxCreative 均取 generic.entity_interaction_range 的计算值，hitboxMargin=0，
     * mobFactor=1。
     *
     * @param entityInteractionRange generic.entity_interaction_range 的计算值
     * @return 默认攻击范围
     */
    [[nodiscard]] static AttackRange defaultFor(f32 entityInteractionRange) noexcept
    {
        return AttackRange{
            /*minRange=*/0.0f,
            /*maxRange=*/entityInteractionRange,
            /*minCreativeRange=*/0.0f,
            /*maxCreativeRange=*/entityInteractionRange,
            /*hitboxMargin=*/0.0f,
            /*mobFactor=*/1.0f,
        };
    }

    /**
     * @brief 计算指定攻击者的有效最小范围
     *
     * 对应 vanilla AttackRange.effectiveMinRange(Entity)：玩家旁观者=0、创造=minCreative、
     * 否则=min；非玩家=mobFactor*min。
     *
     * @param attacker 攻击者实体
     * @return 有效最小范围
     */
    [[nodiscard]] f32 effectiveMinRange(const Entity& attacker) const noexcept;

    /**
     * @brief 计算指定攻击者的有效最大范围
     *
     * 对应 vanilla AttackRange.effectiveMaxRange(Entity)：玩家创造=maxCreative、否则=max；
     * 非玩家=mobFactor*max。
     *
     * @param attacker 攻击者实体
     * @return 有效最大范围
     */
    [[nodiscard]] f32 effectiveMaxRange(const Entity& attacker) const noexcept;

    /**
     * @brief 目标 AABB 是否落在攻击范围内
     *
     * 对应 vanilla AttackRange.isInRange(LivingEntity, AABB, f64)：以攻击者眼睛到
     * 目标 AABB 的最近距离与有效区间（含 hitboxMargin 与额外 padding）比较。
     *
     * @param attacker 攻击者（用于取眼睛位置与有效范围）
     * @param target 目标碰撞箱
     * @param padding 额外容差
     * @return 在范围内返回 true
     */
    [[nodiscard]] bool isInRange(const LivingEntity& attacker, const AxisAlignedBB& target, f64 padding) const noexcept;

    [[nodiscard]] bool operator==(const AttackRange& other) const noexcept
    {
        return minRange == other.minRange && maxRange == other.maxRange && minCreativeRange == other.minCreativeRange &&
            maxCreativeRange == other.maxCreativeRange && hitboxMargin == other.hitboxMargin &&
            mobFactor == other.mobFactor;
    }
};

} // namespace component
} // namespace item
} // namespace mc
