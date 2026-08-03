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
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/tool/TieredItem.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/world/block/Material.hpp"

namespace mc {

// Forward declarations
class LivingEntity;
class IWorld;
class BlockPos;

namespace item {
namespace tool {

/**
 * @brief 剑类武器
 *
 * 剑是 TieredItem 但不是 ToolItem。
 * 它们有不同的行为：
 * - 对蜘蛛网有极高的挖掘效率 (15.0)
 * - 对植物有轻微效率 (1.5)
 * - 攻击伤害高于其他工具
 * - 攻击敌人消耗 1 耐久，破坏方块消耗 2 耐久
 *
 * 攻击伤害 = 基础值 + 层级加成
 * 攻击速度 = 基础值（通常为 -2.4）
 */
class SwordItem : public TieredItem {
public:
    /**
     * @brief 构造剑
     * @param tier 工具层级
     * @param attackDamage 基础攻击伤害（通常为 3）
     * @param attackSpeed 攻击速度修正（通常为 -2.4）
     * @param properties 物品属性
     */
    SwordItem(const tier::IItemTier& tier, i32 attackDamage, f32 attackSpeed, ItemProperties properties);

    ~SwordItem() override = default;

    /**
     * @brief 获取挖掘速度
     *
     * 对蜘蛛网返回 15.0，对植物返回 1.5，其他返回 1.0。
     *
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack, const BlockState& state) const override;

    /**
     * @brief 检查是否能采集方块
     *
     * 剑只能采集蜘蛛网。
     *
     * @param state 目标方块状态
     * @return 如果可以采集返回 true
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const override;

    /**
     * @brief 攻击实体时调用
     *
     * 消耗 1 点耐久度（其他工具消耗 2 点）。
     *
     * @param stack 物品堆
     * @param target 目标实体
     * @param attacker 攻击者实体
     * @return 是否成功
     */
    bool hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker) override;

    /**
     * @brief 破坏方块时调用
     *
     * 如果方块硬度 > 0，消耗 2 点耐久度（其他工具消耗 1 点）。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param state 被破坏的方块状态
     * @param pos 方块位置
     * @param entity 破坏者实体
     * @return 是否成功
     */
    bool onBlockDestroyed(
        ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& entity) override;

    /**
     * @brief 获取总攻击伤害
     *
     * 基础伤害 + 层级加成
     *
     * @return 攻击伤害值
     */
    [[nodiscard]] f32 getAttackDamage() const { return m_attackDamage; }

    /**
     * @brief 获取攻击速度修正
     * @return 攻击速度修正值
     */
    [[nodiscard]] f32 getAttackSpeed() const { return m_attackSpeed; }

    /**
     * @brief 获取属性修饰符
     *
     * 剑在主手时提供攻击伤害和攻击速度修饰符。
     *
     * @param equipmentSlot 装备槽位
     * @return 属性修饰符
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 equipmentSlot) const override;

private:
    f32 m_attackDamage;
    f32 m_attackSpeed;
};

} // namespace tool
} // namespace item
} // namespace mc
