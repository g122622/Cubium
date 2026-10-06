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
 * @brief 末影水晶物品
 *
 * 右键黑曜石或基岩顶面放置一个末影水晶实体（不显示底座）。目标位置上方必须为空气，
 * 且该位置无其他实体占据。放置后尝试唤醒末影龙战斗（tryRespawn）。
 *
 * 参考: net.minecraft.world.item.EndCrystalItem
 */
class EndCrystalItem : public Item {
public:
    explicit EndCrystalItem(ItemProperties properties);
    ~EndCrystalItem() override = default;

    /**
     * @brief 在方块上使用：在黑曜石/基岩顶面放置末影水晶
     */
    ActionResultType onItemUse(ItemUseContext& context) override;
};

} // namespace item::items
} // namespace mc
