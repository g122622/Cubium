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
 * @brief 望远镜物品
 *
 * 右键使用进入望远镜视角（最长 1200 tick），松开或超时退出。使用期间服务端
 * 通过 UseAction::Spyglass 通知客户端播放缩放视角动画，并由
 * LivingEntity::isScoping 消费（影响 FOV 与移速）。
 *
 * 参考: net.minecraft.world.item.SpyglassItem
 */
class SpyglassItem : public Item {
public:
    /// 最长使用时长（tick），对齐 vanilla SpyglassItem.USE_DURATION = 1200
    static constexpr i32 USE_DURATION = 1200;

    explicit SpyglassItem(ItemProperties properties);
    ~SpyglassItem() override = default;

    /**
     * @brief 获取使用时长
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作（望远镜缩放）
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用：进入望远镜视角
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 使用完成（超时）：退出望远镜视角
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;

    /**
     * @brief 提前松开：退出望远镜视角
     */
    void onPlayerStoppedUsing(ItemStack& stack, IWorld& world, LivingEntity& entity, i32 timeLeft) override;
};

} // namespace item::items
} // namespace mc
