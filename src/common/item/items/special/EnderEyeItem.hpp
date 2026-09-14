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

#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 末影之眼物品
 *
 * 末影之眼有两种主要用途：
 * 1. 右键使用：发射末影之眼投掷物（指引要塞方向）
 * 2. 对末地传送门框架使用：嵌入末影之眼
 *
 * 当 12 个末地传送门框架全部嵌入末影之眼后，中央 3×3 区域会生成末地传送门方块，
 * 并触发全服广播的末地传送门激活音效（事件 1038，globalLevelEvent）。
 *
 * 参考: net.minecraft.item.EnderEyeItem
 */
class EnderEyeItem : public Item {
public:
    /**
     * @brief 构造末影之眼物品
     * @param properties 物品属性
     */
    explicit EnderEyeItem(ItemProperties properties);

    ~EnderEyeItem() override = default;

    /**
     * @brief 在方块上使用物品
     *
     * 对末地传送门框架方块使用末影之眼：
     * - 检查目标方块是否为 END_PORTAL_FRAME 且 !hasEye
     * - 若是：设置 EYE=true，播放填充音效（事件 1503）
     * - 消耗物品（非创造模式）
     * - 检测 12 框架是否全部含眼，若是则放置 3×3 END_PORTAL 方块并广播 globalLevelEvent(1038)
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    ActionResultType onItemUse(ItemUseContext& context) override;
};

} // namespace item::items
} // namespace mc
