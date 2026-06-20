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
 * LIABILITY, IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"

namespace mc {

class IWorld;
class Player;

namespace item {

/**
 * @brief 风弹物品
 *
 * 右键投掷风弹弹射物实体。风弹命中实体或方块时产生风爆效果，
 * 推开周围的实体和弹射物。
 *
 * 属性：
 * - 最大堆叠：64
 * - 冷却时间：10 tick（玩家投掷）
 * - 伤害：1
 * - 风爆范围：内圈4格/中圈8格/外圈24格
 * - 投掷速度：1.5
 * - 投掷散布：1.0
 *
 * 获取方式：
 * - 旋风人掉落 (0-1，受抢夺影响)
 * - 试炼刷怪笼补给
 *
 * 命名空间ID: minecraft:wind_charge
 */
class WindChargeItem final : public Item {
public:
    /// 玩家投掷冷却时间（ticks）
    static constexpr i32 COOLDOWN_TICKS = 10;

    /// 风弹造成的伤害
    static constexpr f32 DAMAGE = 1.0f;

    /// 风爆内圈半径
    static constexpr f32 WIND_BURST_INNER_RADIUS = 4.0f;

    /// 风爆中圈半径
    static constexpr f32 WIND_BURST_MIDDLE_RADIUS = 8.0f;

    /// 风爆外圈半径
    static constexpr f32 WIND_BURST_OUTER_RADIUS = 24.0f;

    /// 投掷速度因子
    static constexpr f32 THROW_VELOCITY = 1.5f;

    /// 投掷散布精度
    static constexpr f32 THROW_INACCURACY = 1.0f;

    /**
     * @brief 构造风弹物品
     * @param properties 物品属性
     */
    explicit WindChargeItem(const ItemProperties& properties);

    /**
     * @brief 右键使用风弹（投掷弹射物）
     *
     * 创建 WindChargeEntity 弹射物实体并投掷。
     * 非创造模式消耗1个风弹，使用后进入 10 tick 冷却。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;
};

} // namespace item
} // namespace mc
