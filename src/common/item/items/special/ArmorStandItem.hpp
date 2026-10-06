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
 * @brief 盔甲架物品
 *
 * 右键方块顶面放置一个盔甲架实体，朝向按玩家 yaw 对齐 45 度取整。
 * 放置前检查目标位置的实体碰撞（有实体占据则不放置）。
 *
 * 参考: net.minecraft.world.item.ArmorStandItem
 */
class ArmorStandItem : public Item {
public:
    explicit ArmorStandItem(ItemProperties properties);
    ~ArmorStandItem() override = default;

    /**
     * @brief 在方块上使用：放置盔甲架实体
     */
    ActionResultType onItemUse(ItemUseContext& context) override;
};

} // namespace item::items
} // namespace mc
