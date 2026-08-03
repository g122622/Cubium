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
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mc {

class LivingEntity;
class IWorld;
class ItemStack;

namespace item {
namespace enchant {

class Enchantment;

} // namespace enchant
} // namespace item

namespace entity {

/**
 * @brief 位置依赖附魔效果跟踪器
 *
 * 追踪当前活跃的位置依赖附魔效果（如冰霜行者、灵魂疾行）。
 * 按装备槽位和附魔ID记录活跃的效果集合，当实体移动到新的方块位置时
 * 评估效果是否应该激活/停用。
 *
 * 对应 LivingEntity.activeLocationDependentEnchantments。
 */
class LocationEnchantmentTracker {
public:
    /**
     * @brief 检查指定槽位是否有活跃的位置依赖附魔
     *
     * @param slot 装备槽位
     * @param enchantmentId 附魔ID
     * @return 是否活跃
     */
    [[nodiscard]] bool isActive(i32 slot, const std::string& enchantmentId) const;

    /**
     * @brief 设置指定槽位的附魔为活跃状态
     *
     * @param slot 装备槽位
     * @param enchantmentId 附魔ID
     */
    void setActive(i32 slot, const std::string& enchantmentId);

    /**
     * @brief 设置指定槽位的附魔为非活跃状态
     *
     * @param slot 装备槽位
     * @param enchantmentId 附魔ID
     * @return 之前是否为活跃状态
     */
    bool setInactive(i32 slot, const std::string& enchantmentId);

    /**
     * @brief 清除指定槽位的所有活跃附魔
     *
     * @param slot 装备槽位
     * @return 被清除的附魔ID集合
     */
    std::unordered_set<std::string> clearSlot(i32 slot);

    /**
     * @brief 清除所有槽位的所有活跃附魔
     */
    void clearAll();

    /**
     * @brief 检查是否存在任何活跃的位置依赖附魔
     *
     * 当实体有活跃的位置依赖附魔但未移动时，需要周期性重新评估
     * 附魔效果（例如实体站在灵魂沙上，灵魂沙被挖走后需要移除灵魂疾行
     * 的速度修饰符）。
     *
     * @return 是否存在至少一个活跃的位置依赖附魔
     */
    [[nodiscard]] bool hasActiveEnchantments() const;

    /**
     * @brief 获取指定槽位的活跃附魔集合
     *
     * @param slot 装备槽位
     * @return 活跃附魔ID集合（可能为空）
     */
    [[nodiscard]] const std::unordered_set<std::string>& getActiveEnchantments(i32 slot) const;

private:
    /**
     * @brief 槽位 -> (附魔ID集合) 的映射
     *
     * 使用 i32 作为键，对应 EquipmentSlot 的数值。
     */
    std::unordered_map<i32, std::unordered_set<std::string>> m_activeEnchantments;
};

} // namespace entity
} // namespace mc
