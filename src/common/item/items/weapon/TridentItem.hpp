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

namespace mc {

// 前向声明
class Player;
class World;
class ItemStack;
class LivingEntity;

namespace entity {
class AbstractArrowEntity;
}

namespace item {

/**
 * @brief 三叉戟物品
 *
 * 三叉戟是近战和远程相结合的武器，可以作为近战武器使用，也可以投掷。
 *
 * 近战属性:
 * - 伤害: 8 (与钻石剑相同)
 * - 攻击速度: -2.9
 *
 * 投掷机制:
 * - 最小蓄力时间: 10 tick (0.5秒)
 * - 投掷速度: 2.5 + 激流等级 * 0.5
 * - 耐久消耗: 1 点/投掷
 *
 * 附魔支持:
 * - 忠诚 (Loyalty): 投掷后自动返回，等级越高返回越快
 * - 激流 (Riptide): 在雨中或水中投掷时冲刺，不在水中则不能投掷
 * - 引雷 (Channeling): 雷暴天气投掷时召唤闪电
 */
class TridentItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit TridentItem(const ItemProperties& properties);

    ~TridentItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 获取最大使用时间
     *
     * 返回 72000 tick（几乎无限制）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作类型
     * @return UseAction::Spear
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 检查是否可以投掷（激流附魔需要玩家潮湿）。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 停止使用物品（松开右键）
     *
     * 投掷三叉戟或触发激流冲刺。
     */
    void onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft) override;

    /**
     * @brief 攻击实体时调用
     *
     * 消耗耐久度。
     */
    bool hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker) override;

    /**
     * @brief 破坏方块时调用
     *
     * 消耗耐久度。
     */
    bool onBlockDestroyed(
        ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& breaker) override;

    /**
     * @brief 获取附魔能力
     * @return 1
     */
    [[nodiscard]] i32 getItemEnchantability() const override { return 1; }

private:
    /**
     * @brief 检查玩家是否潮湿（在水中或雨中）
     */
    static bool _isWet(const Player& player) noexcept;

    /**
     * @brief 获取激流附魔等级
     */
    static i32 _getRiptideLevel(const ItemStack& stack) noexcept;
};

} // namespace item
} // namespace mc
