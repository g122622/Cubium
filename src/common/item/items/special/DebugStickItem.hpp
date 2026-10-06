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

namespace mc {
namespace item::items {

/**
 * @brief 调试棒
 *
 * 仅管理员（canUseGameMasterBlocks）可用的调试工具：
 * - 左键点击方块：循环切换"当前选中的属性"（在方块的所有属性间轮换），不破坏方块；
 * - 右键点击方块：循环切换"当前选中属性的值"，并立即应用到方块；
 * - 潜行（secondary use）反向循环。
 *
 * 每个方块的"当前选中属性"保存在物品的 NBT 中（键 `debug_stick_state`，
 * 形如 { "minecraft:oak_stairs": "facing" }），对齐 vanilla DebugStickState 组件。
 *
 * 参考: net.minecraft.world.item.DebugStickItem
 */
class DebugStickItem : public Item {
public:
    explicit DebugStickItem(ItemProperties properties);
    ~DebugStickItem() override = default;

    /**
     * @brief 右键使用：循环切换当前选中属性的值并应用
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 左键点击方块：循环切换当前选中的属性（不破坏方块）
     *
     * 返回 false 表示不允许破坏该方块（对齐 vanilla DebugStickItem#canDestroyBlock），
     * 调用方据此中止破坏流程，方块不被移除。
     */
    bool canDestroyBlock(
        ItemStack& stack, const BlockState& state, IWorld& world, const BlockPos& pos, LivingEntity& breaker) const override;

private:
    /**
     * @brief 处理一次交互
     * @param player 玩家
     * @param stack 手持物品堆（用于读写选中的属性）
     * @param state 目标方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param isRightClick true=右键（改值），false=左键（换属性）
     * @return 是否成功处理（false 表示无管理员权限或方块无属性）
     */
    static bool _handleInteraction(Player& player,
        ItemStack& stack,
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        bool isRightClick);
};

} // namespace item::items
} // namespace mc
