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

#include "../../core/Item.hpp"
#include "MaceMath.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace item {

/**
 * @brief 重锤物品
 *
 * 重型近战武器，具有下落攻击加成。
 *
 * 属性：
 * - 攻击伤害：5
 * - 攻击速度：-3.4（基础DPS约1.6）
 * - 最大耐久：250
 *
 * 特殊机制：
 * - 下落攻击(Smash Attack)：当下落距离 > 1.5 格且不在滑翔时触发
 * - 下落攻击伤害加成：分段函数（见 MaceMath::calculateSmashAttackDamage）
 * - 下落攻击时产生击退效果，对周围3.5格内实体施加击退
 * - 重击(下落距离>5格)时击退力翻倍
 * - 下落攻击后重置攻击者的下落距离
 *
 * 支持魔咒：破甲（Breach）、致密（Density）、风爆（Wind Burst）
 *
 * 命名空间ID: minecraft:mace
 */
class MaceItem final : public Item {
public:
    /// 基础攻击伤害（委托给 MaceMath）
    static constexpr f32 DEFAULT_ATTACK_DAMAGE = MaceMath::DEFAULT_ATTACK_DAMAGE;

    /// 攻击速度修正（委托给 MaceMath）
    static constexpr f32 DEFAULT_ATTACK_SPEED = MaceMath::DEFAULT_ATTACK_SPEED;

    /// 最大耐久度（委托给 MaceMath）
    static constexpr i32 MAX_DURABILITY = MaceMath::MAX_DURABILITY;

    /// 触发砸地攻击的最低下落距离（委托给 MaceMath）
    static constexpr f32 SMASH_ATTACK_FALL_THRESHOLD = MaceMath::SMASH_ATTACK_FALL_THRESHOLD;

    /// 重砸判定阈值（委托给 MaceMath）
    static constexpr f32 SMASH_ATTACK_HEAVY_THRESHOLD = MaceMath::SMASH_ATTACK_HEAVY_THRESHOLD;

    /// 击退范围（格）（委托给 MaceMath）
    static constexpr f32 SMASH_ATTACK_KNOCKBACK_RADIUS = MaceMath::SMASH_ATTACK_KNOCKBACK_RADIUS;

    /// 击退基础力度（委托给 MaceMath）
    static constexpr f32 SMASH_ATTACK_KNOCKBACK_POWER = MaceMath::SMASH_ATTACK_KNOCKBACK_POWER;

    /**
     * @brief 构造重锤
     * @param properties 物品属性
     */
    explicit MaceItem(const ItemProperties& properties);

    /**
     * @brief 重锤主手属性修饰符（ATTACK_DAMAGE +5.0、ATTACK_SPEED -3.4）
     *
     * 对齐 vanilla 重锤物品组件 component_item_properties（attack_damage=5、attack_speed=-3.4）。
     * 重锤近战伤害 = 玩家基础 1.0 + ATTACK_DAMAGE modifier 5.0 = 6.0（满冷却）。
     * 攻击速度 = 基础 4.0 + ATTACK_SPEED modifier -3.4 = 0.6（对齐 vanilla）。
     *
     * 修复前缺陷：MaceItem 未重写 getAttributeModifiers，重锤无 ATTACK_DAMAGE/ATTACK_SPEED
     * modifier，致近战 baseDamage 仅玩家基础 1.0（vanilla 6.0，严重偏低），攻击速度 4.0
     * （vanilla 0.6，过快）。破甲/致密/风爆等附魔虽定义但基数错误致伤害全错。
     *
     * @param equipmentSlot 装备槽（仅 MainHand 提供修饰符）
     * @return 该槽位的属性修饰符集合
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 equipmentSlot) const override;

    /**
     * @brief 检查实体是否处于砸地攻击状态
     *
     * 条件：下落距离 > 1.5 且不在滑翔（鞘翅飞行）
     *
     * @param entity 实体
     * @return 是否处于砸地攻击状态
     */
    [[nodiscard]] static bool canSmashAttack(const class LivingEntity& entity);

    /**
     * @brief 计算下落攻击的基础伤害加成（不含魔咒）
     *
     * 委托给 MaceMath::calculateSmashAttackDamage()。
     *
     * @param fallDistance 下落距离
     * @return 基础伤害加成
     */
    [[nodiscard]] static f32 calculateSmashAttackDamage(f32 fallDistance)
    {
        return MaceMath::calculateSmashAttackDamage(fallDistance);
    }

    /**
     * @brief 攻击命中时回调
     *
     * 砸地攻击触发时：
     * 1. 将攻击者Y轴速度设为0.01（停止下落）
     * 2. 播放音效
     * 3. 对目标周围实体施加击退
     * 4. 消耗耐久
     *
     * @param stack 物品堆
     * @param target 被攻击的目标
     * @param attacker 攻击者
     * @return true（消耗耐久）
     */
    bool hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker) override;

    /**
     * @brief 攻击伤害结算后回调
     *
     * 砸地攻击触发时重置攻击者的下落距离为0。
     * 对应 MC 的 postHurtEnemy。
     *
     * @param stack 物品堆
     * @param target 被攻击的目标
     * @param attacker 攻击者
     */
    void postHitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker) override;

    /**
     * @brief 计算下落攻击的额外伤害（包含致密魔咒）
     *
     * 对应 MC 的 getAttackDamageBonus。
     * 基础伤害加成 + 致密魔咒加成 * fallDistance。
     *
     * @param attacker 攻击者
     * @param fallDistance 下落距离
     * @param weapon 武器物品堆
     * @return 额外伤害值
     */
    [[nodiscard]] static f32 getSmashAttackDamageBonus(
        const class LivingEntity& attacker, f32 fallDistance, const ItemStack& weapon);

    /**
     * @brief 对目标周围实体施加砸地攻击击退
     *
     * 击退力度 = (3.5 - 距离) * 0.7 * (重击? 2 : 1) * (1 - 击退抗性)
     * 排除：攻击者自身、目标自身、旁观者、创造飞行玩家、可驯服且属于目标的实体、标记盔甲架
     *
     * @param world 世界
     * @param attacker 攻击者
     * @param target 被攻击的目标
     */
    static void applySmashAttackKnockback(class IWorld& world, class Entity& attacker, class Entity& target);
};

} // namespace item
} // namespace mc
