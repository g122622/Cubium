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

#include "common/item/enchantment/Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 灵魂疾行附魔
 *
 * 在灵魂沙和灵魂土上增加移动速度。
 *
 * 效果:
 * - 每级增加灵魂沙/土上的移动速度
 * - I: +40%, II: +60%, III: +80%
 * - 最大 III 级
 * - 属于宝藏附魔
 *
 * 位置依赖效果:
 * - 当实体站在灵魂沙/灵魂土上时激活速度修饰符
 * - 当实体离开灵魂沙/灵魂土时停用速度修饰符
 * - 每次位置变化有 4% 概率消耗靴子耐久
 * - 灵魂粒子效果和音效
 */
class SoulSpeedEnchantment : public Enchantment {
public:
    SoulSpeedEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:soul_speed"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.soul_speed";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::ArmorFeet; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::VeryRare; }

    [[nodiscard]] bool isTreasure() const noexcept override
    {
        return true; // 只能从猪灵交易获得
    }

    [[nodiscard]] bool canVillagerTrade() const noexcept override
    {
        return false; // 不能通过村民交易获得
    }

    [[nodiscard]] bool canGenerateInLoot() const noexcept override
    {
        return false; // 不能在战利品表中生成
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 10 + (level - 1) * 10; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 15; }

    /**
     * @brief 获取灵魂沙/土上的移动速度乘数
     * @param level 附魔等级
     * @return 速度乘数
     *
     * TODO: MC 1.21.11 使用 AddValue 操作（perLevel(0.0405, 0.0105)），
     *       当前实现使用 MultiplyTotal（I: +40%, II: +60%, III: +80%），
     *       与 MC 1.16.5 风格一致，待属性系统完善后对齐。
     */
    [[nodiscard]] static f32 getSoulSpeedMultiplier(i32 level)
    {
        // I: 1.4 (+40%), II: 1.6 (+60%), III: 1.8 (+80%)
        // 修饰符值 = 0.2 * (level + 1): I=0.4, II=0.6, III=0.8
        return 1.0f + 0.2f * static_cast<f32>(level + 1);
    }

    /**
     * @brief 获取耐久消耗概率
     * @param level 附魔等级（未使用，概率固定）
     * @return 每tick消耗耐久的概率（固定4%）
     */
    [[nodiscard]] static f32 getDurabilityConsumeChance(i32 level)
    {
        (void)level;  // 概率固定为4%，与等级无关
        return 0.04f; // 固定4%概率
    }

    /**
     * @brief 位置依赖效果：在灵魂沙/土上激活速度加成
     *
     * 当实体站在灵魂沙/灵魂土上且在地面上、非骑乘时，激活速度修饰符。
     * 当条件不再满足时，由 onLocationEffectDeactivated 移除修饰符。
     *
     * @param entity 持有附魔物品的实体
     * @param stack 附魔物品堆
     * @param slot 装备槽位
     * @param level 附魔等级
     * @param isActive 当前该附魔是否已经处于活跃状态
     * @return 是否应该处于活跃状态
     */
    [[nodiscard]] bool onLocationChanged(
        LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level, bool isActive) const override;

    /**
     * @brief 停用位置依赖效果：移除灵魂疾行速度修饰符
     *
     * @param entity 持有附魔物品的实体
     * @param stack 附魔物品堆
     * @param slot 装备槽位
     * @param level 附魔等级
     */
    void onLocationEffectDeactivated(LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level) const override;

private:
    /**
     * @brief 检查实体是否站在灵魂沙/灵魂土上
     */
    [[nodiscard]] bool isOnSoulSpeedBlock(LivingEntity& entity) const;

    /**
     * @brief 应用灵魂疾行速度修饰符
     */
    void applySoulSpeedModifier(LivingEntity& entity, i32 level) const;

    /**
     * @brief 移除灵魂疾行速度修饰符
     */
    void removeSoulSpeedModifier(LivingEntity& entity) const;
};

} // namespace enchant
} // namespace item
} // namespace mc
