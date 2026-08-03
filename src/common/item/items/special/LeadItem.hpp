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

#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"

namespace mc::item::items {

/**
 * @brief 拴绳物品
 *
 * 拴绳可以用于：
 * 1. 右键栅栏方块：将玩家手中拴住的生物绑定到栅栏柱上的拴绳结
 * 2. 右键生物（由 MobEntity::processInitialInteract 处理）：将拴绳拴在生物身上
 *
 * 参考 MC Java: net.minecraft.world.item.LeadItem
 */
class LeadItem : public Item {
public:
    explicit LeadItem(ItemProperties properties);

    /**
     * @brief 在方块上使用物品
     *
     * 当玩家右键点击栅栏方块时，将玩家手中拴住的生物绑定到栅栏柱上的拴绳结。
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    [[nodiscard]] ActionResultType onItemUse(ItemUseContext& context) override;
};

} // namespace mc::item::items
