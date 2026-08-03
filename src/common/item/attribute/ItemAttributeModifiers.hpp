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
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace item {

/**
 * @brief 物品属性修饰符管理器
 *
 * 管理物品提供的属性修饰符。
 *
 * 用法示例:
 * @code
 * // 为剑添加攻击伤害和攻击速度修饰符
 * ItemAttributeModifiers modifiers;
 * modifiers.add(Attributes::ATTACK_DAMAGE, AttributeModifier(
 *     "sword_attack_damage", 4.0, Operation::Addition
 * ), EquipmentSlot::MainHand);
 * modifiers.add(Attributes::ATTACK_SPEED, AttributeModifier(
 *     "sword_attack_speed", -2.4, Operation::Addition
 * ), EquipmentSlot::MainHand);
 * @endcode
 */
class ItemAttributeModifiers {
public:
    /**
     * @brief 槽位修饰符条目
     *
     * 存储属性修饰符及其对应的属性注册名和装备槽位。
     * 使用属性注册名（如 "generic.armor"）而非 const Attribute* 指针，
     * 避免 Attribute 生命周期问题导致的悬挂指针。
     */
    struct Entry {
        std::string attributeName; ///< 属性注册名（如 "generic.armor"）
        entity::attribute::AttributeModifier modifier;
        i32 equipmentSlot; ///< 使用int代替EquipmentSlot避免循环依赖

        Entry(std::string attrName, const entity::attribute::AttributeModifier& mod, i32 slot) noexcept
            : attributeName(std::move(attrName))
            , modifier(mod)
            , equipmentSlot(slot)
        {}
    };

    /**
     * @brief 添加属性修饰符
     * @param attributeName 属性注册名（如 "generic.armor"）
     * @param modifier 修饰符
     * @param equipmentSlot 装备槽位
     */
    void add(const std::string& attributeName, const entity::attribute::AttributeModifier& modifier, i32 equipmentSlot);

    /**
     * @brief 获取所有修饰符
     */
    [[nodiscard]] const std::vector<Entry>& getEntries() const { return m_entries; }

    /**
     * @brief 获取指定槽位的修饰符
     * @param slot 装备槽位
     * @return 修饰符列表
     */
    [[nodiscard]] std::vector<Entry> getModifiersForSlot(i32 equipmentSlot) const;

    /**
     * @brief 检查是否有修饰符
     */
    [[nodiscard]] bool isEmpty() const { return m_entries.empty(); }

    /**
     * @brief 获取修饰符数量
     */
    [[nodiscard]] size_t size() const { return m_entries.size(); }

    /**
     * @brief 清空所有修饰符
     */
    void clear() { m_entries.clear(); }

    // ========== 静态辅助方法 ==========

    /**
     * @brief 生成物品修饰符UUID
     * @param itemId 物品ID
     * @param attributeId 属性ID
     * @return UUID
     */
    [[nodiscard]] static u64 generateModifierUUID(u32 itemId, const std::string& attributeId);

private:
    std::vector<Entry> m_entries;
};

/**
 * @brief 物品属性修饰符构建器
 *
 * 流畅接口构建物品属性修饰符。
 *
 * 用法示例:
 * @code
 * auto modifiers = ItemAttributeModifiersBuilder()
 *     .add(Attributes::ATTACK_DAMAGE, 4.0, Operation::Addition, EquipmentSlot::MainHand)
 *     .add(Attributes::ATTACK_SPEED, -2.4, Operation::Addition, EquipmentSlot::MainHand)
 *     .build();
 * @endcode
 */
class ItemAttributeModifiersBuilder {
public:
    /**
     * @brief 添加攻击伤害修饰符
     * @param amount 伤害值
     * @param slot 装备槽位
     * @return this引用
     */
    ItemAttributeModifiersBuilder& attackDamage(f64 amount, i32 equipmentSlot);

    /**
     * @brief 添加攻击速度修饰符
     * @param amount 速度值（通常为负数）
     * @param slot 装备槽位
     * @return this引用
     */
    ItemAttributeModifiersBuilder& attackSpeed(f64 amount, i32 equipmentSlot);

    /**
     * @brief 添加护甲修饰符
     * @param amount 护甲值
     * @param slot 装备槽位
     * @return this引用
     */
    ItemAttributeModifiersBuilder& armor(f64 amount, i32 equipmentSlot);

    /**
     * @brief 添加护甲韧性修饰符
     * @param amount 韧性值
     * @param slot 装备槽位
     * @return this引用
     */
    ItemAttributeModifiersBuilder& armorToughness(f64 amount, i32 equipmentSlot);

    /**
     * @brief 添加击退抗性修饰符
     * @param amount 击退抗性（0.0-1.0）
     * @param slot 装备槽位
     * @return this引用
     */
    ItemAttributeModifiersBuilder& knockbackResistance(f64 amount, i32 equipmentSlot);

    /**
     * @brief 添加移动速度修饰符
     * @param amount 速度值
     * @param slot 装备槽位
     * @return this引用
     */
    ItemAttributeModifiersBuilder& movementSpeed(f64 amount, i32 equipmentSlot);

    /**
     * @brief 构建修饰符
     * @return 物品属性修饰符
     */
    [[nodiscard]] ItemAttributeModifiers build() const { return m_modifiers; }

private:
    ItemAttributeModifiers m_modifiers;
};

} // namespace item
} // namespace mc
