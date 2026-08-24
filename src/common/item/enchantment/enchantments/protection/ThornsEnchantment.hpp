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
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/util/math/random/Random.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 荆棘附魔
 *
 * 对齐 vanilla 1.21.11 THORNS（Enchantments.java:337-346）。
 *
 * 效果（POST_ATTACK VICTIM→ATTACKER，概率 perLevel 0.15）：
 * - DamageEntity(constant 1.0, constant 5.0, THORNS)：触发时对攻击者造成 [1.0, 5.0) 随机荆棘伤害，
 *   与等级无关（仅概率随等级线性增长）。
 * - ChangeItemDamage(constant 2.0)：触发时使触发荆棘的护甲扣 2 耐久。
 * - 最大 III 级（每级 +15% 触发概率：I=15%, II=30%, III=45%）。
 */
class ThornsEnchantment : public Enchantment {
public:
    ThornsEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:thorns"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.thorns";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::ArmorChest; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::VeryRare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 10 + (level - 1) * 20; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 检查是否触发荆棘效果
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return true 如果触发
     */
    [[nodiscard]] static bool shouldTrigger(i32 level, math::Random& random);

    /**
     * @brief 获取荆棘反伤
     *
     * 对齐 vanilla 1.21.11 Enchantments.java:342 THORNS 的 DamageEntity(constant 1.0, constant 5.0)：
     * Mth.randomBetween(random, 1.0F, 5.0F) 返回 [1.0, 5.0) 随机浮点，与等级无关（无老版本
     * level>10 分支）。Cubium IRandom::nextFloat(1.0f, 5.0f) 语义等价（min + nextFloat()*(max-min)）。
     *
     * @param random 随机数生成器
     * @return 反伤点数 [1.0, 5.0)
     */
    [[nodiscard]] static f32 getThornsDamage(math::Random& random);

    /**
     * @brief 获取触发概率
     * @param level 附魔等级
     * @return 触发概率 (0.0-1.0)
     */
    [[nodiscard]] static f32 getTriggerChance(i32 level)
    {
        // 对齐 vanilla Enchantments.java:345 perLevel 0.15（每级 15%）
        return static_cast<f32>(level) * 0.15f;
    }

    /**
     * @brief 当持有者受到伤害时调用
     *
     * 对齐 vanilla 1.21.11 THORNS（Enchantments.java:337-346）的 POST_ATTACK(VICTIM→ATTACKER)
     * AllOf.entityEffects(DamageEntity, ChangeItemDamage)，概率 perLevel 0.15。触发时：
     *   1. DamageEntity(constant 1.0, constant 5.0, THORNS)：对攻击者造成 [1.0,5.0) 随机荆棘伤害。
     *   2. ChangeItemDamage(constant 2.0)：触发荆棘的护甲扣 2 耐久（hurtAndBreak）。
     * 耐久消耗作用于 enchantedItem（触发荆棘的那件护甲，对齐 vanilla EnchantedItemInUse.itemStack），
     * 由本方法内部处理（对齐 ChangeItemDamage.apply 直接调 itemstack.hurtAndBreak），不在调用方处理。
     *
     * @param user 受伤者（持有荆棘附魔装备的实体）
     * @param attacker 攻击者
     * @param enchantedItem 触发荆棘的护甲物品（耐久消耗作用对象）
     * @param slot 触发荆棘的护甲所在装备槽
     * @param level 附魔等级
     */
    void onUserHurt(
        LivingEntity& user, Entity& attacker, ItemStack& enchantedItem, EquipmentSlot slot, i32 level) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
