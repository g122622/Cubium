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
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 水下呼吸附魔
 *
 * 通过 oxygen_bonus 属性降低水下氧气消耗概率，延长水下停留时间。
 *
 * 对齐 vanilla 1.21.11：水下呼吸魔咒经 EnchantmentAttributeEffect
 *（id="enchantment.respiration"，属性=OXYGEN_BONUS，LevelBasedValue.perLevel(1.0F)，
 *  Operation.ADD_VALUE，槽位组=HEAD）把每级 +1.0 的修饰符加到 oxygen_bonus 属性
 *（Enchantments.java:285-296）。LivingEntity.decreaseAirSupply 读 oxygen_bonus 值 d0，
 * d0>0 时仅 1/(d0+1) 概率消耗氧气（I级 50%、II级 66.7%、III级 75% 不消耗）。
 *
 * 注意：vanilla 水下呼吸不改变 maxAir（最大氧气值仍 300），而是降低消耗概率。
 * 此前实现的"每级延长 15 秒呼吸时间"是基于 breath_max 的误解，已废弃。
 */
class RespirationEnchantment : public Enchantment {
public:
    RespirationEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:respiration"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.respiration";
    }

    [[nodiscard]] EnchantmentType type() const noexcept override { return EnchantmentType::ArmorHead; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const noexcept override { return 10 * level; }

    [[nodiscard]] i32 getMaxCost(i32 level) const noexcept override { return getMinCost(level) + 30; }

    /**
     * @brief 水下呼吸附魔的属性修饰符
     *
     * 对齐 vanilla Enchantments.java:285-296：每级给 oxygen_bonus +1.0（ADD_VALUE），
     * 仅 HEAD 槽位生效。decreaseAirSupply 据此降低氧气消耗概率。
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 level) const override
    {
        item::ItemAttributeModifiers modifiers;
        if (level > 0) {
            modifiers.add(entity::attribute::Attributes::OXYGEN_BONUS,
                entity::attribute::AttributeModifier("enchantment.respiration",
                    "Respiration",
                    static_cast<f64>(level),
                    entity::attribute::Operation::Addition),
                static_cast<i32>(EquipmentSlot::Head));
        }
        return modifiers;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
