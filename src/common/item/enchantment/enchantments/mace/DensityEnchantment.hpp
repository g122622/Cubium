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
 * @brief 致密附魔（重锤专属）
 *
 * 重锤专属附魔，增加下落攻击每格伤害。
 *
 * 效果：
 * - 每级增加 0.5 点每格下落伤害
 * - 最大 V 级
 * - 与锋利、亡灵杀手、节肢杀手、穿刺、破甲互斥
 *
 * 伤害计算：
 * - I: 0.5/格
 * - II: 1.0/格
 * - III: 1.5/格
 * - IV: 2.0/格
 * - V: 2.5/格
 *
 * 总加成 = level * 0.5 * fallDistance
 *
 * 命名空间ID: minecraft:density
 */
class DensityEnchantment : public Enchantment {
public:
    /// 附魔ID
    static constexpr const char* ENCHANTMENT_ID = "minecraft:density";

    DensityEnchantment() = default;

    [[nodiscard]] std::string id() const override { return ENCHANTMENT_ID; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.density";
    }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }
    [[nodiscard]] i32 maxLevel() const noexcept override { return 5; }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Weapon; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Common; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 5 + (level - 1) * 8; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return 25 + (level - 1) * 8; }

    [[nodiscard]] bool isTreasure() const noexcept override { return false; }

    /**
     * @brief 致密与伤害类附魔（锋利、亡灵杀手、节肢杀手）和破甲互斥
     *
     * 致密和破甲属于 DAMAGE_EXCLUSIVE 组。
     */
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;

    /**
     * @brief 计算每格下落伤害加成
     * @param level 附魔等级
     * @return 每格伤害加成值
     */
    [[nodiscard]] static f32 getDamagePerFallenBlock(i32 level) noexcept { return 0.5f * static_cast<f32>(level); }
};

} // namespace enchant
} // namespace item
} // namespace mc
