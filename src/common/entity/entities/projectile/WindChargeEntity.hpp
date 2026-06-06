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

#include "ThrowableEntity.hpp"

namespace mc {
namespace entity {

/**
 * @brief 风弹弹射物实体
 *
 * 由旋风人投掷或玩家使用风弹物品投掷。
 * 命中实体或方块时产生风爆效果，推开周围实体和弹射物。
 *
 * 属性：
 * - 伤害：1（无论来源）
 * - 重力加速度：0.03
 * - 风爆范围：内圈3.5格/外圈5.5格（玩家投掷），3.0/5.0（旋风人投掷）
 * - 风爆推力：0.4（玩家投掷），0.2（旋风人投掷）
 *
 * 命名空间ID: minecraft:wind_charge
 */
class WindChargeEntity final : public ThrowableEntity {
public:
    /// 玩家投掷风弹的基础伤害
    static constexpr f32 PLAYER_DAMAGE = 1.0f;

    /// 旋风人投掷风弹的基础伤害
    static constexpr f32 BREEZE_DAMAGE = 1.0f;

    /// 风爆内圈半径
    static constexpr f32 WIND_BURST_INNER_RADIUS = 3.5f;

    /// 风爆外圈半径
    static constexpr f32 WIND_BURST_OUTER_RADIUS = 5.5f;

    /// 风爆推力（玩家投掷）
    static constexpr f32 WIND_BURST_POWER = 0.4f;

    /**
     * @brief 构造风弹弹射物
     * @param id 实体ID
     */
    explicit WindChargeEntity(EntityId id);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 获取重力加速度
     */
    f32 getGravity() const override { return 0.03f; }

protected:
    /**
     * @brief 命中实体时处理
     */
    void onEntityHit(const RayTraceResult& result) override;

    /**
     * @brief 命中方块时处理
     */
    void onBlockHit(const RayTraceResult& result) override;

    /**
     * @brief 冲击处理（风爆效果）
     */
    void onImpact(const RayTraceResult& result) override;

private:
    /**
     * @brief 产生风爆效果
     *
     * 推开范围内的实体和弹射物。
     * 内圈推力较大，外圈推力衰减。
     */
    void applyWindBurst();

    /// 是否已产生风爆（防止重复触发）
    bool m_hasBurst = false;
};

} // namespace entity
} // namespace mc
