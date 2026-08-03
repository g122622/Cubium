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

namespace mc {

class Player;

namespace item::items {

/**
 * @brief 知识之书物品
 *
 * 右键使用时，从物品NBT中读取配方列表并解锁给玩家。
 * 解锁成功后消耗一个物品。
 *
 * NBT结构：
 * - "recipes" 列表，每个元素是配方ID字符串（如 "minecraft:recovery_compass"）
 *
 * 参考: net.minecraft.world.item.KnowledgeBookItem
 */
class KnowledgeBookItem : public Item {
public:
    /**
     * @brief 构造知识之书物品
     * @param properties 物品属性
     */
    explicit KnowledgeBookItem(ItemProperties properties);

    ~KnowledgeBookItem() override = default;

    /**
     * @brief 右键使用
     *
     * 读取物品NBT中的配方列表，逐个验证配方是否存在，
     * 成功时解锁所有配方给玩家并消耗一个物品。
     *
     * @param world 世界引用
     * @param player 玩家引用
     * @param hand 使用的手
     * @return 动作结果
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;
};

} // namespace item::items
} // namespace mc
