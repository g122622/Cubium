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
#include "common/item/core/Item.hpp"

namespace mc {

// Forward declarations
class Player;
class LivingEntity;

namespace item::items {

/**
 * @brief 命名牌物品
 *
 * 命名牌可以给生物命名，被命名的生物将获得自定义名称并变为持久化（不会消失）。
 *
 * 主要功能：
 * 1. 对 MobEntity 右键使用：如果物品有自定义名称，设置实体的自定义名称并启用持久化
 * 2. 对玩家无效
 * 3. 消耗一个物品
 *
 * 参考: net.minecraft.item.NameTagItem
 */
class NameTagItem : public Item {
public:
    /**
     * @brief 构造命名牌物品
     * @param properties 物品属性
     */
    explicit NameTagItem(ItemProperties properties);

    ~NameTagItem() override = default;

    /**
     * @brief 与实体交互
     *
     * 当玩家右键点击实体时调用。
     * 如果物品有自定义名称且目标是 MobEntity，则：
     * 1. 设置实体的自定义名称
     * 2. 启用实体的持久化（不会消失）
     * 3. 消耗一个物品
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    bool itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand) override;
};

} // namespace item::items
} // namespace mc
