/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following
 * conditions:
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

namespace mc {
namespace entity {

/**
 * @brief 裂纹程度追踪器
 *
 * 根据物品耐久度比例计算裂纹等级，用于狼铠和铁傀儡的裂纹渲染。
 * 参考: net.minecraft.world.entity.Crackiness
 */
class Crackiness {
public:
    /**
     * @brief 裂纹等级
     */
    enum class Level : u8 {
        None = 0,   ///< 无裂纹
        Low = 1,    ///< 轻微裂纹
        Medium = 2, ///< 中等裂纹
        High = 3    ///< 严重裂纹
    };

    /**
     * @brief 构造裂纹程度追踪器
     *
     * @param fractionLow 剩余耐久比例低于此值时出现轻微裂纹
     * @param fractionMedium 剩余耐久比例低于此值时出现中等裂纹
     * @param fractionHigh 剩余耐久比例低于此值时出现严重裂纹
     */
    constexpr Crackiness(f32 fractionLow, f32 fractionMedium, f32 fractionHigh) noexcept
        : m_fractionLow(fractionLow)
        , m_fractionMedium(fractionMedium)
        , m_fractionHigh(fractionHigh)
    {}

    /**
     * @brief 根据剩余耐久比例获取裂纹等级
     *
     * @param fractionRemaining 剩余耐久比例 (0.0~1.0)，即 (maxDamage - damage) / maxDamage
     * @return 裂纹等级
     */
    [[nodiscard]] Level byFraction(f32 fractionRemaining) const noexcept
    {
        if (fractionRemaining < m_fractionHigh) {
            return Level::High;
        }
        if (fractionRemaining < m_fractionMedium) {
            return Level::Medium;
        }
        if (fractionRemaining < m_fractionLow) {
            return Level::Low;
        }
        return Level::None;
    }

    /**
     * @brief 根据物品耐久值获取裂纹等级
     *
     * @param damage 当前耐久损伤值
     * @param maxDamage 最大耐久值
     * @return 裂纹等级
     */
    [[nodiscard]] Level byDamage(i32 damage, i32 maxDamage) const noexcept
    {
        if (maxDamage <= 0) {
            return Level::None;
        }
        f32 fractionRemaining = static_cast<f32>(maxDamage - damage) / static_cast<f32>(maxDamage);
        return byFraction(fractionRemaining);
    }

    /// 铁傀儡裂纹程度阈值
    static const Crackiness GOLEM;

    /// 狼铠裂纹程度阈值
    /// 剩余 < 95% → Low, < 69% → Medium, < 32% → High
    static const Crackiness WOLF_ARMOR;

private:
    f32 m_fractionLow;
    f32 m_fractionMedium;
    f32 m_fractionHigh;
};

} // namespace entity
} // namespace mc
