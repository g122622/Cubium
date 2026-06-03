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
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/UseAction.hpp"
#include <functional>

namespace mc {

// 前向声明
class Player;
class World;
class ItemStack;
class LivingEntity;

namespace item {

/**
 * @brief 盾牌物品
 *
 * 盾牌可以格挡攻击和投射物。
 *
 * 格挡机制:
 * - 右键按住时进入格挡状态
 * - 可以格挡近战攻击、箭矢、火球等
 * - 格挡伤害会消耗耐久度
 * - 被斧攻击会使盾牌失效 5 秒
 *
 * 附魔支持:
 * - 耐久 (Unbreaking): 减少耐久消耗
 * - 经验修补 (Mending): 用经验修复
 */
class ShieldItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit ShieldItem(const ItemProperties& properties);

    ~ShieldItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 获取最大使用时间
     *
     * 返回 72000 tick（几乎无限制）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作类型
     * @return UseAction::Block
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 进入格挡状态。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 获取防御值
     * @return 防御值
     */
    [[nodiscard]] i32 getShieldDefenseValue() const { return SHIELD_DEFENSE_VALUE; }

    /**
     * @brief 获取盾牌冷却时间（被斧攻击后）
     * @return 冷却时间 (tick)
     */
    [[nodiscard]] static i32 getShieldDisableTime() { return SHIELD_DISABLE_TIME; }

    /**
     * @brief 检查物品是否可以作为盾牌使用
     */
    [[nodiscard]] static bool isShield(const ItemStack& stack);

private:
    static constexpr i32 MAX_USE_DURATION = 72000;
    static constexpr i32 SHIELD_DEFENSE_VALUE = 5;  // 盾牌防御值
    static constexpr i32 SHIELD_DISABLE_TIME = 100; // 被斧攻击后失效时间 (5秒 = 100 tick)
};

} // namespace item
} // namespace mc
