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

#include "../../Enchantment.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 击退附魔
 *
 * 增加击退敌人的力度。
 *
 * 效果:
 * - 每级增加 0.5 击退强度（实际击退距离受目标击退抗性影响）
 * - 击退强度与玩家基础击退叠加计算
 * - 最大 II 级
 *
 * 注意：实际击退计算在 PlayerAttackHelper 中进行，
 * 使用 KNOCKBACK_ENCHANT_BONUS = 0.5f 作为每级加成。
 */
class KnockbackEnchantment : public Enchantment {
public:
    KnockbackEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:knockback"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.knockback";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Weapon; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 2; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Uncommon; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 5 + (level - 1) * 20; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 获取击退强度加成
     *
     * 每级增加 0.5 击退强度。
     * 实际击退距离 = 基础击退 + (level * 0.5)
     *
     * @param level 附魔等级
     * @return 击退强度加成
     */
    [[nodiscard]] static f32 getKnockbackBonus(i32 level) { return static_cast<f32>(level) * 0.5f; }
};

} // namespace enchant
} // namespace item
} // namespace mc
