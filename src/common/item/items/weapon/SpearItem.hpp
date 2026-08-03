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
#include "common/item/items/tool/TieredItem.hpp"
#include "common/item/tier/IItemTier.hpp"

namespace mc {

// 前向声明
class Player;
class ItemStack;
class LivingEntity;
class IWorld;
class BlockPos;
class BlockState;

namespace item {

/**
 * @brief 长矛物品
 *
 * 长矛是近战和远程相结合的武器，可按材质分层（木/石/铜/铁/金/钻石/下界合金）。
 *
 * 近战属性:
 * - 攻击伤害 = 基础值(3) + 层级加成（与剑一致）
 * - 攻击速度: -2.9（长杆武器，比剑慢）
 * - 攻击实体消耗 1 耐久，破坏方块消耗 2 耐久
 *
 * 投掷机制:
 * - 最小蓄力时间: 10 tick (0.5秒)
 * - 投掷速度: 2.5
 * - 投掷伤害: 8.0（固定，与三叉戟一致）
 * - 耐久消耗: 1 点/投掷
 * - 水中阻力极小，可拾回
 *
 * 与三叉戟的区别:
 * - 长矛按材质分层，属性随层级变化
 * - 长矛不支持忠诚/激流/引雷附魔（原版数据包未将 spears 加入 trident 可附魔标签）
 * - 长矛投掷伤害固定，不随层级变化
 */
class SpearItem : public tool::TieredItem {
public:
    /**
     * @brief 构造长矛
     * @param tier 工具层级（木、石、铜、铁、金、钻石、下界合金）
     * @param attackDamage 基础攻击伤害（通常为 3）
     * @param attackSpeed 攻击速度修正（通常为 -2.9）
     * @param properties 物品属性
     */
    SpearItem(const tier::IItemTier& tier, i32 attackDamage, f32 attackSpeed, ItemProperties properties);

    ~SpearItem() override = default;

    // ========== Item 接口重写 ==========

    /**
     * @brief 获取最大使用时间
     *
     * 返回 72000 tick（几乎无限制），与三叉戟一致。
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
     * 检查耐久度后开始蓄力。
     */
    [[nodiscard]] ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 停止使用物品（松开右键）
     *
     * 蓄力足够时投掷长矛。
     */
    void onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft) override;

    /**
     * @brief 攻击实体时调用
     *
     * 消耗 1 点耐久度（与剑一致）。
     */
    bool hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker) override;

    /**
     * @brief 破坏方块时调用
     *
     * 方块硬度 > 0 时消耗 2 点耐久度（与剑一致）。
     */
    bool onBlockDestroyed(
        ItemStack& stack, IWorld& world, const BlockState& state, const BlockPos& pos, LivingEntity& breaker) override;

    /**
     * @brief 获取总攻击伤害（近战）
     *
     * 基础伤害 + 层级加成
     */
    [[nodiscard]] f32 getAttackDamage() const { return m_attackDamage; }

    /**
     * @brief 获取攻击速度修正
     */
    [[nodiscard]] f32 getAttackSpeed() const { return m_attackSpeed; }

    /**
     * @brief 获取属性修饰符
     *
     * 长矛在主手时提供攻击伤害和攻击速度修饰符。
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 equipmentSlot) const override;

    // ========== 长矛特有常量 ==========

    /// 最小投掷蓄力时间（tick），低于此值松开右键不会投掷
    static constexpr i32 MIN_CHARGE_TICKS = 10;

    /// 最大使用时间（tick），几乎无限制
    static constexpr i32 MAX_USE_DURATION = 72000;

    /// 基础投掷速度
    static constexpr f32 THROW_VELOCITY = 2.5f;

    /// 投掷命中造成的固定伤害（与三叉戟一致）
    static constexpr f32 THROW_DAMAGE = 8.0f;

private:
    f32 m_attackDamage; // = attackDamage + tier.getAttackDamage()
    f32 m_attackSpeed;
};

} // namespace item
} // namespace mc
