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

#include "ProtectionEnchantment.hpp"
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
 * @brief 火焰保护附魔
 *
 * 减少火焰伤害。
 *
 * 效果:
 * - 对火焰伤害（燃烧、岩浆、火）有双倍保护效果
 * - 对其他伤害也有基础保护
 * - 通过 BURNING_TIME 属性缩减被点燃后的燃烧时间（每级 -15%）
 * - 最大 IV 级
 */
class FireProtectionEnchantment : public ProtectionEnchantment {
public:
    FireProtectionEnchantment() noexcept
        : ProtectionEnchantment(Type::Fire)
    {}

    [[nodiscard]] std::string id() const noexcept override { return "minecraft:fire_protection"; }

    [[nodiscard]] std::string getNameKey(i32 level) const noexcept override
    {
        (void)level;
        return "enchantment.minecraft.fire_protection";
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Uncommon; }

    /**
     * @brief 火焰保护附魔的属性修饰符
     *
     * 经 EnchantmentAttributeEffect（id="enchantment.fire_protection"，属性=BURNING_TIME，
     * LevelBasedValue.perLevel(-0.15F)，Operation.ADD_MULTIPLIED_BASE，装备槽位组=ARMOR）把每级 -0.15
     * 的基础乘法修饰符加到 BURNING_TIME 属性。
     *
     * 装备槽位组 ARMOR 表示 4 个盔甲槽位任一激活即生效。Cubium 的附魔属性修饰符管线按单槽位过滤
     *（EnchantmentHelper::applyEnchantmentAttributeModifiers 按 entry.equipmentSlot==slot 匹配），
     * 无槽位组概念，故为 4 个盔甲槽位各注册一条同 id 修饰符。
     *
     * BURNING_TIME 默认 1.0，火焰保护 IV 单件 → 1.0 + 1.0×(4×-0.15) = 0.4，
     * 被点燃时燃烧时间缩减为 40%（LivingEntity::igniteForTicks override 消费）。
     *
     * 多件火焰保护盔甲的叠加语义（已实测验证，见 FireProtectionBurningTimeTests 集成测试通过
     * BURNING_TIME=0.4 即单条 -0.6 修饰符，非 4 件叠加的 -2.4）：
     *   vanilla fire_protection.json 中 4 盔甲槽位共享同一 modifier id，vanilla AttributeInstance
     *   按 id 去重。Cubium 虽用 vector+push_back 不去重，但 EnchantmentHelper::
     *   applyEnchantmentAttributeModifiers 每 slot add 前调 removeModifier(id)（删第一条同 id），
     *   4 槽位顺序处理时后槽位 remove 删前槽位刚加的同 id 修饰符，最终只剩 1 条 -0.6——
     *   恰好复现 vanilla 去重语义。故全套火焰保护 IV BURNING_TIME = 0.4（同单件）。
     *
     * @param level 附魔等级
     * @return 4 个盔甲槽位的 BURNING_TIME 修饰符
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 level) const override
    {
        item::ItemAttributeModifiers modifiers;
        if (level > 0) {
            const f64 amount = static_cast<f64>(level) * -0.15;
            const entity::attribute::AttributeModifier modifier(
                "enchantment.fire_protection", "Fire Protection", amount, entity::attribute::Operation::MultiplyBase);
            modifiers.add(entity::attribute::Attributes::BURNING_TIME, modifier, static_cast<i32>(EquipmentSlot::Feet));
            modifiers.add(entity::attribute::Attributes::BURNING_TIME, modifier, static_cast<i32>(EquipmentSlot::Legs));
            modifiers.add(
                entity::attribute::Attributes::BURNING_TIME, modifier, static_cast<i32>(EquipmentSlot::Chest));
            modifiers.add(entity::attribute::Attributes::BURNING_TIME, modifier, static_cast<i32>(EquipmentSlot::Head));
        }
        return modifiers;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
