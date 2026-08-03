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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
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
 * @brief 破甲附魔（重锤专属）
 *
 * 重锤专属附魔，降低目标护甲的减伤效果。
 *
 * 效果：
 * - 每级降低 15% 护甲有效率
 * - 最大 IV 级
 * - 与锋利、亡灵杀手、节肢杀手、穿刺、致密互斥
 *
 * 护甲有效率修改：
 * - I: -0.15 (护甲减伤效果降低 15%)
 * - II: -0.30 (护甲减伤效果降低 30%)
 * - III: -0.45 (护甲减伤效果降低 45%)
 * - IV: -0.60 (护甲减伤效果降低 60%)
 *
 * 例：目标 20 护甲(80%减伤) + 破甲 IV:
 *   有效护甲率 = 0.8 - 0.6 = 0.2 → 最终减伤 20%（原本 80%）
 *
 * 命名空间ID: minecraft:breach
 */
class BreachEnchantment : public Enchantment {
public:
    /// 附魔ID
    static constexpr const char* ENCHANTMENT_ID = "minecraft:breach";

    BreachEnchantment() = default;

    [[nodiscard]] std::string id() const override { return ENCHANTMENT_ID; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.breach";
    }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }
    [[nodiscard]] i32 maxLevel() const noexcept override { return 4; }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Weapon; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Uncommon; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 15 + (level - 1) * 9; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return 65 + (level - 1) * 9; }

    [[nodiscard]] bool isTreasure() const noexcept override { return false; }

    /**
     * @brief 破甲与伤害类附魔（锋利、亡灵杀手、节肢杀手）和致密互斥
     *
     * 致密和破甲属于 DAMAGE_EXCLUSIVE 组。
     */
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;

    /**
     * @brief 计算护甲有效率修正值
     *
     * 每级 -0.15，叠加到原始护甲有效率上。
     * 结果应被 clamp 到 [0.0, 1.0]。
     *
     * @param level 附魔等级
     * @return 护甲有效率修正值（负值表示降低护甲效果）
     */
    [[nodiscard]] static f32 getArmorEffectivenessModifier(i32 level) noexcept
    {
        return -0.15f * static_cast<f32>(level);
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
