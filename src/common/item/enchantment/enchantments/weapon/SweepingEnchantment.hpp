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
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 横扫之刃附魔（原版注册名 sweeping_edge）
 *
 * 增加横扫攻击的伤害。
 *
 * 效果:
 * - I: 50% 伤害传递
 * - II: 67% 伤害传递
 * - III: 75% 伤害传递
 * - 最大 III 级
 */
class SweepingEnchantment : public Enchantment {
public:
    SweepingEnchantment()
    {
        setAnvilCost(4);
    }

    [[nodiscard]] std::string id() const override
    {
        return "minecraft:sweeping_edge";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.sweeping_edge";
    }

    [[nodiscard]] EnchantmentType type() const override
    {
        return EnchantmentType::SweepingWeapon;
    }

    [[nodiscard]] i32 minLevel() const noexcept override
    {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const noexcept override
    {
        return 3;
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override
    {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override
    {
        return 5 + (level - 1) * 9;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const override
    {
        return getMinCost(level) + 15;
    }

    /**
     * @brief 获取横扫伤害比例
     *
     * TODO: 该函数与 getAttributeModifiers 提供的 sweeping_damage_ratio 重复（后者才是权威来源），
     * 目前仅 EnchantmentHelper::getSweepingDamageRatio 使用，而横扫攻击已改为读属性。
     * 待 EnchantmentHelper::getSweepingDamageRatio 一并删除。
     *
     * @param level 附魔等级
     * @return 伤害传递比例 (0.0-1.0)
     */
    [[nodiscard]] static f32 getSweepingDamageRatio(i32 level) noexcept
    {
        // I: 50%, II: 67%, III: 75%
        // 公式: 1 / (level + 1) 的补数
        return 1.0f - (1.0f / static_cast<f32>(level + 1));
    }

    /**
     * @brief 横扫之刃的属性修饰符
     *
     * 对齐 vanilla `data/minecraft/enchantment/sweeping_edge.json`：
     * 属性 `minecraft:sweeping_damage_ratio`、id `minecraft:enchantment.sweeping_edge`、
     * `add_value`、amount = fraction(linear(1,1) / linear(2,1)) = level / (level + 1)，
     * 作用槽位 = MAINHAND。
     *
     * 与 {@link getSweepingDamageRatio} 的关系：两者算的是同一个比例，但表达方式不同——
     * 原版的 fraction 是 float 除法 level/(level+1)，旧辅助函数算的是 1 - 1/(level+1)，
     * 两者在 float 下可能差 1ulp。属性修饰符是权威数值（多件装备按原版规则合并），
     * 因此消费方应当读属性而不再用旧公式。
     *
     * @param level 附魔等级
     * @return 主手槽位的 sweeping_damage_ratio 修饰符
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 level) const override
    {
        item::ItemAttributeModifiers modifiers;
        if (level > 0) {
            const f32 base = static_cast<f32>(level);
            const f32 denominator = static_cast<f32>(level + 1);
            const f64 amount = static_cast<f64>(base / denominator);
            modifiers.add(entity::attribute::Attributes::SWEEPING_DAMAGE_RATIO,
                entity::attribute::AttributeModifier("minecraft:enchantment.sweeping_edge",
                    "Sweeping Edge",
                    amount,
                    entity::attribute::Operation::Addition),
                static_cast<i32>(EquipmentSlot::MainHand));
        }
        return modifiers;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
