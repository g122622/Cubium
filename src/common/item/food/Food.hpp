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
#include "common/entity/effect/EffectType.hpp"
#include <memory>
#include <vector>

namespace mc {

// Forward declarations
namespace entity::effect {
class EffectInstance;
}

namespace item::food {

/**
 * @brief 食物效果条目
 *
 * 描述食物可能给予的药水效果。
 */
struct FoodEffect {
    entity::effect::EffectType type; ///< 效果类型
    i32 duration = 0;                ///< 持续时间（ticks）
    i32 amplifier = 0;               ///< 效果等级（0 = I, 1 = II, 等）
    f32 probability = 1.0f;          ///< 触发概率 (0.0 - 1.0)
};

/**
 * @brief 食物属性类
 *
 * 定义食物的属性，如饥饿值、饱和度、是否可以快速食用等。
 *
 * 用法示例:
 * @code
 * Food apple(4, 0.3f);  // 恢复4点饥饿值，0.3饱和度修正
 * Food goldenApple(4, 1.2f).setAlwaysEdible(true);
 * Food pufferfish(1, 0.1f).addEffect(EffectType::Poison, 1200, 1, 1.0f);
 * @endcode
 *
 * 饱和度计算公式：saturation = food * saturationModifier * 2.0
 * 例如苹果 (4, 0.3F)：saturation = 4 * 0.3 * 2.0 = 2.4
 */
class Food {
public:
    /**
     * @brief 构造食物属性
     * @param hunger 恢复的饥饿值 (0-20)
     * @param saturationModifier 饱和度修正值（不是直接饱和度）
     */
    Food(i32 hunger, f32 saturationModifier);

    // ========== 构建器方法 ==========

    /**
     * @brief 设置是否为肉类
     * @param isMeat 是否为肉类
     * @note 肉类食物可以喂给狼
     */
    Food& setMeat(bool isMeat = true) noexcept
    {
        m_isMeat = isMeat;
        return *this;
    }

    /**
     * @brief 设置是否可以快速食用
     * @param fastEat 是否快速食用
     * @note 快速食用时间为16ticks，普通为32ticks
     */
    Food& setFastEat(bool fastEat = true) noexcept
    {
        m_fastEat = fastEat;
        return *this;
    }

    /**
     * @brief 设置是否可以在饱食时食用
     * @param alwaysEdible 是否总是可食用
     * @note 金苹果等特殊食物需要此属性
     */
    Food& setAlwaysEdible(bool alwaysEdible = true) noexcept
    {
        m_alwaysEdible = alwaysEdible;
        return *this;
    }

    /**
     * @brief 添加药水效果
     * @param type 效果类型
     * @param duration 持续时间（ticks）
     * @param amplifier 效果等级（0 = I, 1 = II, 等）
     * @param probability 触发概率 (0.0 - 1.0)
     * @note 可以添加多个效果
     */
    Food& addEffect(entity::effect::EffectType type, i32 duration, i32 amplifier, f32 probability)
    {
        m_effects.push_back({type, duration, amplifier, probability});
        return *this;
    }

    /**
     * @brief 添加必定触发的药水效果
     * @param type 效果类型
     * @param duration 持续时间（ticks）
     * @param amplifier 效果等级（0 = I, 1 = II, 等）
     */
    Food& addEffect(entity::effect::EffectType type, i32 duration, i32 amplifier)
    {
        return addEffect(type, duration, amplifier, 1.0f);
    }

    // ========== 获取属性 ==========

    /**
     * @brief 获取恢复的饥饿值
     */
    [[nodiscard]] i32 getHunger() const noexcept { return m_hunger; }

    /**
     * @brief 获取饱和度修正值
     * @note 实际饱和度 = food * saturationModifier * 2.0
     */
    [[nodiscard]] f32 getSaturationModifier() const noexcept { return m_saturationModifier; }

    /**
     * @brief 是否为肉类
     */
    [[nodiscard]] bool isMeat() const noexcept { return m_isMeat; }

    /**
     * @brief 是否可以快速食用
     */
    [[nodiscard]] bool isFastEat() const noexcept { return m_fastEat; }

    /**
     * @brief 是否可以在饱食时食用
     */
    [[nodiscard]] bool canAlwaysEat() const noexcept { return m_alwaysEdible; }

    /**
     * @brief 获取所有药水效果
     */
    [[nodiscard]] const std::vector<FoodEffect>& getEffects() const noexcept { return m_effects; }

    /**
     * @brief 是否有药水效果
     */
    [[nodiscard]] bool hasEffects() const noexcept { return !m_effects.empty(); }

private:
    i32 m_hunger;                      ///< 恢复的饥饿值
    f32 m_saturationModifier;          ///< 饱和度修正值
    bool m_isMeat = false;             ///< 是否为肉类
    bool m_fastEat = false;            ///< 是否快速食用
    bool m_alwaysEdible = false;       ///< 是否总是可食用
    std::vector<FoodEffect> m_effects; ///< 药水效果列表
};

} // namespace item::food
} // namespace mc
