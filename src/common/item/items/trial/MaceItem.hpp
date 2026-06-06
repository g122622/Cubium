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
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../core/Item.hpp"

namespace mc {
namespace item {

/**
 * @brief 重锤物品
 *
 * MC 1.21 新增的重型近战武器，具有下落攻击加成。
 *
 * 属性：
 * - 攻击伤害：5
 * - 攻击速度：-2.4（基础DPS约2.1）
 * - 最大耐久：250
 *
 * 特殊机制：
 * - 下落攻击加成：每下落1格增加3点伤害，最大额外伤害40（约14格下落）
 * - 下落攻击时产生风爆效果，推开周围实体
 * - 支持魔咒：破甲（Breach）、致密（Density）、风爆（Wind Burst）
 *
 * 命名空间ID: minecraft:mace
 */
class MaceItem final : public Item {
public:
    /// 基础攻击伤害
    static constexpr f32 ATTACK_DAMAGE = 5.0f;

    /// 攻击速度修正
    static constexpr f32 ATTACK_SPEED = -2.4f;

    /// 最大耐久度
    static constexpr i32 MAX_DURABILITY = 250;

    /// 每格下落增加的伤害
    static constexpr f32 DAMAGE_PER_BLOCK_FALLEN = 3.0f;

    /// 最大额外伤害上限
    static constexpr f32 MAX_EXTRA_DAMAGE = 40.0f;

    /// 风爆推开力度
    static constexpr f32 SMASH_ATTACK_KNOCKBACK_POWER = 3.0f;

    /**
     * @brief 构造重锤
     * @param properties 物品属性
     */
    explicit MaceItem(const ItemProperties& properties);

    /**
     * @brief 计算下落攻击伤害
     * @param fallDistance 下落距离（格）
     * @return 额外伤害值
     */
    [[nodiscard]] static f32 calculateSmashAttackDamage(f32 fallDistance);

    // TODO(trial_chambers): 实现下落攻击检测和伤害计算逻辑
    // TODO(trial_chambers): 实现风爆魔咒效果
    // TODO(trial_chambers): 实现破甲魔咒效果
    // TODO(trial_chambers): 实现致密魔咒效果
};

} // namespace item
} // namespace mc
