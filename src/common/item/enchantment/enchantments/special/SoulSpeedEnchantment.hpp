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
 * 在灵魂沙和灵魂土上增加移动速度并抵消减速效果。
 *
 * 效果:
 * - 通过 MOVEMENT_SPEED 属性修饰符增加移动速度：
 *   使用 Addition 操作，等级 I: +0.0405, II: +0.051, III: +0.0615
 *   公式: 0.0405 + 0.0105 * (level - 1)
 * - 通过 MOVEMENT_EFFICIENCY 属性修饰符抵消灵魂沙/土的减速：
 *   使用 Addition 操作，所有等级均为 +1.0
 *   （MOVEMENT_EFFICIENCY=1.0 时，LivingEntity.getBlockSpeedFactor() 在灵魂沙上
 *    将从默认的 0.4 插值到 1.0，即完全抵消减速）
 * - 最大 III 级
 * - 属于宝藏附魔
 * - 每次位置变化事件有 4% 概率消耗靴子 1 点耐久
 * - 每tick在满足条件时生成灵魂粒子效果和灵魂逃脱音效
 *
 * 位置依赖效果:
 * - 当实体站在灵魂沙/灵魂土上且在地面上、非骑乘、非飞行时激活
 * - 当条件不再满足时停用速度修饰符和效率修饰符
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
     * @brief 获取灵魂疾行的 MOVEMENT_SPEED 修饰符值
     * @param level 附魔等级
     * @return Addition 操作的修饰符值
     *
     * 公式: 0.0405 + 0.0105 * (level - 1)
     * Level I: +0.0405, Level II: +0.051, Level III: +0.0615
     */
    [[nodiscard]] static f32 getMovementSpeedBonus(i32 level)
    {
        return 0.0405f + 0.0105f * static_cast<f32>(level - 1);
    }

    /**
     * @brief 获取灵魂疾行的 MOVEMENT_EFFICIENCY 修饰符值
     * @return Addition 操作的修饰符值，固定为 1.0
     *
     * 所有等级均为 +1.0，配合 LivingEntity.getBlockSpeedFactor() 中的插值逻辑，
     * 当 MOVEMENT_EFFICIENCY=1.0 时，在灵魂沙上 (speedFactor=0.4) 将从 0.4 插值到 1.0，
     * 完全抵消减速效果。
     */
    [[nodiscard]] static f32 getMovementEfficiencyBonus() { return 1.0f; }

    /**
     * @brief 获取耐久消耗概率
     * @param level 附魔等级（未使用，概率固定）
     * @return 每次位置变化事件消耗耐久的概率（固定4%）
     */
    [[nodiscard]] static f32 getDurabilityConsumeChance(i32 level)
    {
        (void)level; // 概率固定为4%，与等级无关
        return 0.04f;
    }

    /**
     * @brief 位置依赖效果：在灵魂沙/土上激活速度加成和效率加成
     *
     * 当实体站在灵魂沙/灵魂土上且在地面上、非骑乘、非飞行时，激活属性修饰符。
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
     * @brief 停用位置依赖效果：移除灵魂疾行速度修饰符和效率修饰符
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
     * @brief 检查实体是否正在飞行（创造/旁观模式飞行或鞘翅滑翔）
     */
    [[nodiscard]] bool isFlying(const LivingEntity& entity) const;

    /**
     * @brief 应用灵魂疾行属性修饰符（MOVEMENT_SPEED 和 MOVEMENT_EFFICIENCY）
     */
    void applySoulSpeedModifiers(LivingEntity& entity, i32 level) const;

    /**
     * @brief 移除灵魂疾行属性修饰符
     */
    void removeSoulSpeedModifiers(LivingEntity& entity) const;

    /**
     * @brief 生成灵魂粒子效果
     */
    void spawnSoulParticles(LivingEntity& entity) const;

    /**
     * @brief 尝试消耗靴子耐久
     */
    void tryConsumeDurability(LivingEntity& entity, i32 slot) const;
};

} // namespace enchant
} // namespace item
} // namespace mc
