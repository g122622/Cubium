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

#include "common/item/core/Item.hpp"
#include "common/util/color/DyeColor.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 欢乐诡鬼装备物品（16色变体）
 *
 * 用于装备欢乐诡鬼（HappyGhast）的装饰性装备，共 16 种颜色变体。
 * 每种颜色对应一个独立的 Item 实例，颜色为物品固有属性（非 NBT 染色）。
 *
 * 特性：
 * - 无护甲值、无耐久度、不可附魔
 * - 可通过合成（皮革+玻璃+对应颜色羊毛）或染色配方获得
 * - 右键成体快乐恶魂装备到 Body 装备槽，播放 HARNESS_EQUIP；剪刀（未潜行）可剪下，
 *   挽具掉回世界并播放 HARNESS_UNEQUIP（对齐 vanilla Equippable.harness 的
 *   canBeSheared/shearingSound 语义）
 *
 * 注：Cubium 无数据驱动的 Equippable 数据组件体系，允许装备 harness 的实体范围由
 *     EntityTypeTags::CAN_EQUIP_HARNESS 显式判定（该标签此前仅被注释引用，从未定义）。
 *
 * 参考: net.minecraft.world.item.equipment.Equippable#harness(DyeColor)
 */
class HarnessItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性（maxStackSize=1）
     * @param color 染料颜色（决定变体）
     */
    HarnessItem(ItemProperties properties, DyeColor color);

    /**
     * @brief 获取装备颜色
     * @return 染料颜色
     */
    [[nodiscard]] DyeColor getColor() const { return m_color; }

    /**
     * @brief 右键实体装备挽具（对齐 vanilla ItemStack.interactLivingEntity → Equippable 组件路径）
     *
     * 成功条件（全部满足才装备）：
     * - 目标存活且非幼年（vanilla HappyGhast.canUseSlot(BODY) = isAlive() && !isBaby()）
     * - 目标类型属于 EntityTypeTags::CAN_EQUIP_HARNESS（vanilla allowedEntities 标签）
     * - Body 装备槽当前为空（已装备时 vanilla 走 mobInteract 的骑乘分支，不替换）
     *
     * @param stack 调用方传入的物品堆（Player 路径为值拷贝，内部改以 player.getHeldItem(hand) 为准）
     * @param player 交互玩家
     * @param target 交互目标实体
     * @param hand 玩家使用的手
     * @return 是否成功装备
     */
    bool itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand) override;

private:
    DyeColor m_color;
};

} // namespace item::items
} // namespace mc
