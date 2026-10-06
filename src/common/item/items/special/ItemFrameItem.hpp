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
 * @brief 悬挂实体物品（物品展示框 / 荧光物品展示框）
 *
 * 右键方块侧面时在该面外侧放置一个物品展示框实体。仅可放置在水平面上
 * （不可贴地/贴顶）。荧光变体额外设置 glowing 标记。
 *
 * 参考: net.minecraft.world.item.HangingEntityItem / ItemFrameItem
 */
class ItemFrameItem : public Item {
public:
    /**
     * @brief 构造物品展示框物品
     * @param glowing true=荧光物品展示框，false=普通物品展示框
     * @param properties 物品属性
     */
    ItemFrameItem(bool glowing, ItemProperties properties);
    ~ItemFrameItem() override = default;

    /**
     * @brief 在方块上使用：在侧面放置物品展示框
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

private:
    bool m_glowing; ///< 是否为荧光物品展示框
};

} // namespace item::items
} // namespace mc
