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
#include "item/enchantment/Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 伤害加成附魔基类
 *
 * 锋利、亡灵杀手、节肢杀手的共同基类。
 * 这三类附魔互斥，不能同时存在于同一物品上。
 *
 * @note 锋利对所有生物造成额外伤害，亡灵杀手对亡灵生物有效，
 *       节肢杀手对节肢生物有效。
 */
class DamageEnchantment : public Enchantment {
public:
    /**
     * @brief 伤害类型枚举
     */
    enum class Type : u8 {
        All,       ///< 锋利 - 对所有生物有效
        Undead,    ///< 亡灵杀手 - 对亡灵生物有效
        Arthropods ///< 节肢杀手 - 对节肢生物有效
    };

    explicit DamageEnchantment(Type damageType) noexcept;

    // ========== Enchantment 接口实现 ==========

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Weapon; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 5; }

    [[nodiscard]] i32 getMinCost(i32 level) const override;

    [[nodiscard]] i32 getMaxCost(i32 level) const override;

    [[nodiscard]] f32 getDamageBonus(i32 level, const LivingEntity* target = nullptr) const noexcept override;

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;

    // ========== 伤害类特有方法 ==========

    /**
     * @brief 获取伤害类型
     */
    [[nodiscard]] Type getDamageType() const { return m_damageType; }

protected:
    Type m_damageType;
};

} // namespace enchant
} // namespace item
} // namespace mc
