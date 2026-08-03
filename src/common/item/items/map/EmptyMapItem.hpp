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
 * IMPLIED, NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "AbstractMapItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "item/core/ActionResult.hpp"
#include "item/core/ItemStack.hpp"

namespace mc {

class Player;
class IWorld;

namespace item::items {

/**
 * @brief 空地图物品
 *
 * 右键使用时创建一张新的已填充地图（FilledMapItem）。
 */
class EmptyMapItem : public AbstractMapItem {
public:
    explicit EmptyMapItem(const ItemProperties& properties);

    /**
     * @brief 右键使用 - 创建新地图
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 创建一张已填充地图
     *
     * @param world 世界
     * @param x 地图中心X坐标
     * @param z 地图中心Z坐标
     * @param scale 缩放级别(0-4)
     * @param trackingPosition 是否追踪玩家位置
     * @param unlimitedTracking 是否无限追踪
     * @return 包含已填充地图的ItemStack
     */
    static ItemStack createFilledMap(
        IWorld& world, i32 x, i32 z, i32 scale, bool trackingPosition, bool unlimitedTracking);
};

} // namespace item::items
} // namespace mc
