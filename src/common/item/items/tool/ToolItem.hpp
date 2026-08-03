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

#include "TieredItem.hpp"
#include "ToolType.hpp"
#include "common/core/Types.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/tier/IItemTier.hpp"
#include "common/world/block/Material.hpp"
#include <unordered_set>

namespace mc {

// Forward declarations
class Block;
class BlockState;
class IWorld;
class BlockPos;
class LivingEntity;

namespace item {
namespace tool {

/**
 * @brief 挖掘工具基类
 *
 * 所有挖掘工具（镐、斧、锹、锄）的基类。
 * 提供挖掘速度计算、耐久度消耗、有效方块判断等功能。
 *
 * 关键功能：
 * - getDestroySpeed(): 计算对特定方块的挖掘速度
 * - canHarvestBlock(): 判断是否能采集特定方块
 * - hitEntity(): 攻击实体时消耗耐久（2点）
 * - onBlockDestroyed(): 破坏方块时消耗耐久（1点）
 */
class ToolItem : public TieredItem {
public:
    /**
     * @brief 构造挖掘工具
     * @param attackDamage 基础攻击伤害（层级加成会自动添加）
     * @param attackSpeed 攻击速度修正（负值表示减速）
     * @param tier 工具层级
     * @param effectiveBlocks 有效方块集合（挖掘速度为效率值）
     * @param toolType 工具类型（用于采集判断）
     * @param properties 物品属性
     */
    ToolItem(f32 attackDamage,
        f32 attackSpeed,
        const tier::IItemTier& tier,
        std::unordered_set<const Block*> effectiveBlocks,
        ToolType toolType,
        ItemProperties properties);

    ~ToolItem() override = default;

    /**
     * @brief 获取挖掘速度
     *
     * 对有效方块返回层级的效率值，否则返回 1.0。
     * 有效方块包括：
     * 1. 材质有效的方块（isEffectiveMaterial）
     * 2. 特定方块集合中的方块
     *
     * @param stack 物品堆
     * @param state 目标方块状态
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack, const BlockState& state) const override;

    /**
     * @brief 检查是否能采集方块
     *
     * 默认实现检查：
     * 1. 工具类型是否匹配
     * 2. 工具等级是否足够
     *
     * 子类可重写以添加特殊逻辑（如镐检查材质）。
     *
     * @param state 目标方块状态
     * @return 如果可以采集返回 true
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const override;

    /**
     * @brief 攻击实体时调用
     *
     * 消耗 2 点耐久度。
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
     * 如果方块硬度 > 0，消耗 1 点耐久度。
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
     * @brief 获取工具类型
     */
    [[nodiscard]] ToolType getToolType() const noexcept { return m_toolType; }

    /**
     * @brief 获取总攻击伤害
     *
     * 基础伤害 + 层级加成
     *
     * @return 攻击伤害值
     */
    [[nodiscard]] f32 getAttackDamage() const noexcept { return m_attackDamage; }

    /**
     * @brief 获取攻击速度修正
     * @return 攻击速度修正值
     */
    [[nodiscard]] f32 getAttackSpeed() const noexcept { return m_attackSpeed; }

    /**
     * @brief 获取有效方块集合
     */
    [[nodiscard]] const std::unordered_set<const Block*>& getEffectiveBlocks() const noexcept
    {
        return m_effectiveBlocks;
    }

    /**
     * @brief 获取属性修饰符
     *
     * 工具在主手时提供攻击伤害和攻击速度修饰符。
     *
     * @param equipmentSlot 装备槽位
     * @return 属性修饰符
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 equipmentSlot) const override;

protected:
    /**
     * @brief 检查方块是否在有效集合中
     * @param block 方块引用
     * @return 如果有效返回 true
     */
    [[nodiscard]] bool isEffectiveBlock(const Block& block) const;

    /**
     * @brief 检查材质是否有效
     *
     * 子类应重写此方法以定义材质有效性。
     * 例如：
     * - 镐：ROCK, IRON, ANVIL
     * - 斧：WOOD, PLANT, GOURD, NETHER_WOOD
     * - 锹：EARTH, SAND, SNOW
     *
     * @param material 材质引用
     * @return 如果材质有效返回 true
     */
    [[nodiscard]] virtual bool isEffectiveMaterial(const Material& material) const;

    std::unordered_set<const Block*> m_effectiveBlocks;
    ToolType m_toolType;
    f32 m_attackDamage;
    f32 m_attackSpeed;
    f32 m_efficiency;
};

} // namespace tool
} // namespace item
} // namespace mc
