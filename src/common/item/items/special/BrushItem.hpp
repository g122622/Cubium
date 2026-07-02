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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, IN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"

namespace mc {

class IWorld;
class Player;
class LivingEntity;

namespace item {

/**
 * @brief 刷子物品
 *
 * 刷子用于考古挖掘可疑方块（可疑沙、可疑沙砾）中的物品，
 * 也可以刷犰狳获取犰狳鳞甲。
 *
 * 属性：
 * - 最大耐久：64
 * - 附魔能力：1（仅支持耐久、经验修补、消失诅咒）
 * - 使用时长：200 ticks（10秒）
 * - 动画周期：10 ticks
 *
 * 使用机制：
 * - 玩家右键方块开始持续使用刷子
 * - 每10 ticks触发一次刷扫（动画周期的第5 tick）
 * - 每次成功刷扫消耗1耐久
 * - 刷扫犰狳时消耗16耐久
 * - 非玩家或未对准方块时取消使用
 *
 * 参考: net.minecraft.world.item.BrushItem
 */
class BrushItem final : public Item {
public:
    /// 最大耐久度
    static constexpr i32 MAX_DURABILITY = 64;

    /// 使用持续时长（ticks）
    static constexpr i32 USE_DURATION = 200;

    /// 动画周期（ticks），每10 ticks触发一次刷扫
    static constexpr i32 ANIMATION_DURATION = 10;

    /// 刷扫触发时机（动画周期内的第几个tick，0-based 为第5 tick）
    static constexpr i32 BRUSH_TICK_IN_CYCLE = 4;

    /// 刷犰狳时的耐久消耗量
    static constexpr i32 ARMADILLO_DURABILITY_COST = 16;

    /**
     * @brief 构造刷子
     * @param properties 物品属性
     */
    explicit BrushItem(ItemProperties properties);

    ~BrushItem() override = default;

    // ========== 物品使用 ==========

    /**
     * @brief 在方块上使用物品
     *
     * 当玩家对准一个方块右键时调用。
     * 检查玩家视线是否对准方块，如果是则开始持续使用。
     *
     * @param context 物品使用上下文
     * @return Consume（开始使用）或 Pass（不对准方块时不使用）
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 右键使用物品
     *
     * 当玩家右键（不对准方块）时调用。
     * 开始持续使用刷子。
     *
     * @param world 世界引用
     * @param player 玩家引用
     * @param hand 使用的手
     * @return 动作结果
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 物品使用过程中每tick调用
     *
     * 每 ANIMATION_DURATION (10) ticks 的第 BRUSH_TICK_IN_CYCLE+1 (5th) tick 触发刷扫逻辑：
     * 1. 检查玩家是否仍对准方块
     * 2. 对可刷方块执行刷扫逻辑
     * 3. 播放音效和粒子
     * 4. 消耗耐久
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用实体
     * @param elapsedTicks 已使用的tick数（从1开始）
     */
    void onUseTick(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 elapsedTicks) override;

    /**
     * @brief 与实体交互
     *
     * 刷犰狳时掉落犰狳鳞甲并消耗16耐久。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    bool itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand) override;

    // ========== 属性 ==========

    /**
     * @brief 获取使用时长
     * @return USE_DURATION (200 ticks)
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作类型
     * @return UseAction::Brush
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& /*stack*/) const override { return UseAction::Brush; }

    /**
     * @brief 获取附魔能力值
     *
     * 刷子的附魔能力为1，仅支持耐久、经验修补和消失诅咒。
     *
     * @return 1
     */
    [[nodiscard]] i32 getItemEnchantability() const override { return 1; }
};

} // namespace item
} // namespace mc
