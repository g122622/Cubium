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
#include "../weapon/DamageEnchantment.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 穿刺附魔
 *
 * 增加三叉戟对水生生物的伤害。
 *
 * 效果:
 * - 每级增加 2.5 点伤害对水生生物
 * - 最大 V 级
 *
 * 穿刺属于 DAMAGE_EXCLUSIVE 组，与锋利、亡灵杀手、节肢杀手、致密、破甲互斥。
 */
class ImpalingEnchantment : public Enchantment {
public:
    ImpalingEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:impaling"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.impaling";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Trident; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 5; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 1 + (level - 1) * 8; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 20; }

    /**
     * @brief 穿刺与伤害类附魔（锋利、亡灵杀手、节肢杀手）和重锤附魔（致密、破甲）互斥
     *
     * 穿刺属于 DAMAGE_EXCLUSIVE 组。
     */
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override
    {
        // 与锋利、亡灵杀手、节肢杀手互斥（DamageEnchantment 类型）
        // 通过类型检查实现（dynamic_cast）
        if (dynamic_cast<const DamageEnchantment*>(&other) != nullptr) {
            return false;
        }
        // 与致密、破甲互斥（DAMAGE_EXCLUSIVE 组）
        if (other.id() == "minecraft:density" || other.id() == "minecraft:breach") {
            return false;
        }
        return Enchantment::isCompatibleWith(other);
    }

    /**
     * @brief 获取对水生生物的伤害加成
     * @param level 附魔等级
     * @param target 受击目标实体（ nullptr 时返回 0）
     * @return 额外伤害（命中水生生物标签时 level * 2.5，否则 0）
     *
     * 对齐 MC Java 1.21.11 Enchantments.java:989-996：穿刺额外伤害通过
     * EnchantmentEffectComponents.DAMAGE + AddValue(perLevel(2.5F)) +
     * EntityPredicate.entityType(EntityTypeTags.SENSITIVE_TO_IMPALING) 判定目标。
     * 即用 SENSITIVE_TO_IMPALING（= AQUATIC，12 成员：turtle/axolotl/guardian/
     * elder_guardian/cod/pufferfish/salmon/tropical_fish/dolphin/squid/glow_squid/
     * tadpole）标签判定水生生物，而非旧的 getMobType()==WATER（仅覆盖 guardian）。
     * 实现在 ImpalingEnchantment.cpp（需 LivingEntity 完整定义查 getTypeId）。
     */
    [[nodiscard]] f32 getDamageBonus(i32 level, const LivingEntity* target = nullptr) const noexcept override;
};

} // namespace enchant
} // namespace item
} // namespace mc
