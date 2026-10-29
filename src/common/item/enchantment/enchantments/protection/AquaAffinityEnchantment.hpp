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
 * @brief 水下速掘附魔
 *
 * 水下挖掘速度不减慢。
 *
 * 效果:
 * - 水下挖掘速度与陆地相同
 * - 只有 I 级
 */
class AquaAffinityEnchantment : public Enchantment {
public:
    AquaAffinityEnchantment()
    {
        setAnvilCost(4);
    }

    [[nodiscard]] std::string id() const noexcept override
    {
        return "minecraft:aqua_affinity";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const noexcept override
    {
        (void)level;
        return "enchantment.minecraft.aqua_affinity";
    }

    [[nodiscard]] EnchantmentType type() const noexcept override
    {
        return EnchantmentType::ArmorHead;
    }

    [[nodiscard]] i32 minLevel() const noexcept override
    {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const noexcept override
    {
        return 1;
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override
    {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const noexcept override
    {
        (void)level;
        return 1;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const noexcept override
    {
        (void)level;
        return 41;
    }

    /**
     * @brief 水下速掘的属性修饰符
     *
     * 对齐 vanilla `data/minecraft/enchantment/aqua_affinity.json`：
     * 属性 `minecraft:submerged_mining_speed`、id `minecraft:enchantment.aqua_affinity`、
     * `add_multiplied_total`、amount = linear(base 4.0) = 4.0，作用槽位 = HEAD。
     *
     * 该属性的默认值是 0.2（水下挖掘为陆地的 1/5），+400% 后为 1.0，即水下与陆地同速。
     *
     * @param level 附魔等级（本附魔只有 1 级）
     * @return 头部槽位的 submerged_mining_speed 修饰符
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 level) const override
    {
        item::ItemAttributeModifiers modifiers;
        if (level > 0) {
            modifiers.add(entity::attribute::Attributes::SUBMERGED_MINING_SPEED,
                entity::attribute::AttributeModifier("minecraft:enchantment.aqua_affinity",
                    "Aqua Affinity",
                    4.0,
                    entity::attribute::Operation::MultiplyTotal),
                static_cast<i32>(EquipmentSlot::Head));
        }
        return modifiers;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
