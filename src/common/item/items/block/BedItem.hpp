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

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

/**
 * @brief 床物品
 *
 * 床的物品形式，重写 getStateForPlacement 以委托给 BedBlock::getStateForPlacement，
 * 实现根据玩家朝向设置床的朝向状态，并在头部位置不可替换时阻止放置。
 */
class BedItem : public BlockItem {
public:
    /**
     * @brief 构造函数
     * @param block 床方块引用
     * @param properties 物品属性
     */
    BedItem(const Block& block, ItemProperties properties);

    ~BedItem() override = default;

    /**
     * @brief 获取放置状态
     *
     * 委托给 BedBlock::getStateForPlacement 以根据玩家朝向设置
     * HORIZONTAL_FACING 属性，并在头部位置不可替换时返回 nullptr。
     *
     * @param context 放置上下文
     * @return 方块状态指针，如果不能放置返回 nullptr
     */
    [[nodiscard]] const BlockState* getStateForPlacement(const BlockItemUseContext& context) const override;
};

} // namespace mc
