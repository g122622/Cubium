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
 * @brief 爆炸保护附魔
 *
 * 减少爆炸伤害，并提供爆炸击退抗性。
 *
 * 效果:
 * - 对爆炸伤害有双倍保护效果
 * - 对其他伤害也有基础保护
 * - 通过 EXPLOSION_KNOCKBACK_RESISTANCE 属性衰减被爆炸推开时的击退力度（每级 +15%）
 * - 最大 IV 级
 */
class BlastProtectionEnchantment : public ProtectionEnchantment {
public:
    BlastProtectionEnchantment()
        : ProtectionEnchantment(Type::Explosion)
    {}

    [[nodiscard]] std::string id() const override { return "minecraft:blast_protection"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.blast_protection";
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    /**
     * @brief 爆炸保护附魔的属性修饰符
     *
     * 经 EnchantmentAttributeEffect（id="enchantment.blast_protection"，属性=EXPLOSION_KNOCKBACK_RESISTANCE，
     * LevelBasedValue.perLevel(0.15F)，Operation.ADD_VALUE，装备槽位组=ARMOR）把每级 +0.15 的加法修饰符
     * 加到 EXPLOSION_KNOCKBACK_RESISTANCE 属性。
     *
     * 装备槽位组 ARMOR 表示 4 个盔甲槽位任一激活即生效。Cubium 的附魔属性修饰符管线按单槽位过滤
     *（EnchantmentHelper::applyEnchantmentAttributeModifiers 按 entry.equipmentSlot==slot 匹配），
     * 无槽位组概念，故为 4 个盔甲槽位各注册一条同 id 修饰符。
     *
     * EXPLOSION_KNOCKBACK_RESISTANCE 默认 0.0，爆炸保护 IV 单件 → 0.0 + 4×0.15 = 0.6，
     * 被爆炸击退时力度衰减为 40%（爆炸击退计算消费 finalKnockback *= (1 - 抗性)）。
     *
     * 多件爆炸保护盔甲的叠加语义（已实测验证，见 BlastProtectionAttributeTest）：
     *   vanilla blast_protection.json 中 4 盔甲槽位共享同一 modifier id（"minecraft:enchantment.
     *   blast_protection"），vanilla AttributeInstance 按 id 去重（Map<UUID,Modifier>），全套 IV
     *   抗性 = 0.6（单条，非 4 件叠加的 2.4）。
     *   Cubium AttributeInstance 用 vector+push_back 不去重，但 EnchantmentHelper::
     *   applyEnchantmentAttributeModifiers 每 slot add 前调 removeModifier(id)（find_if 删第一条
     *   同 id）。4 槽位顺序处理时，后槽位的 remove 删前槽位刚加的同 id 修饰符，最终 m_modifiers
     *   只剩 1 条 0.6——恰好复现 vanilla 去重语义。故全套爆炸保护 IV 抗性 = 0.6，击退 ×0.4。
     *   此"remove-first + add" 复现去重依赖 4 同 id 槽位在同一次 detectEquipmentUpdates 内顺序
     *   处理，若未来 AttributeInstance 改为显式按 id 去重（add 时覆盖同 id），此行为不变。
     *
     * @param level 附魔等级
     * @return 4 个盔甲槽位的 EXPLOSION_KNOCKBACK_RESISTANCE 修饰符
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 level) const override
    {
        item::ItemAttributeModifiers modifiers;
        if (level > 0) {
            const f64 amount = static_cast<f64>(level) * 0.15;
            const entity::attribute::AttributeModifier modifier(
                "enchantment.blast_protection", "Blast Protection", amount, entity::attribute::Operation::Addition);
            modifiers.add(entity::attribute::Attributes::EXPLOSION_KNOCKBACK_RESISTANCE,
                modifier,
                static_cast<i32>(EquipmentSlot::Feet));
            modifiers.add(entity::attribute::Attributes::EXPLOSION_KNOCKBACK_RESISTANCE,
                modifier,
                static_cast<i32>(EquipmentSlot::Legs));
            modifiers.add(entity::attribute::Attributes::EXPLOSION_KNOCKBACK_RESISTANCE,
                modifier,
                static_cast<i32>(EquipmentSlot::Chest));
            modifiers.add(entity::attribute::Attributes::EXPLOSION_KNOCKBACK_RESISTANCE,
                modifier,
                static_cast<i32>(EquipmentSlot::Head));
        }
        return modifiers;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
