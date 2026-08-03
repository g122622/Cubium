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

#include "BlockItem.hpp"
#include "common/item/core/Item.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

/**
 * @brief 墙壁或地板放置物品
 *
 * 用于可以在地板上放置或在墙上放置的物品（如告示牌、旗帜、头颅等）。
 * 根据点击位置自动选择放置在地板上还是墙上。
 *
 * 参考: net.minecraft.item.WallOrFloorItem
 */
class WallOrFloorItem : public BlockItem {
public:
    /**
     * @brief 构造墙壁或地板放置物品
     * @param floorBlock 地板方块（放在地上）
     * @param wallBlock 墙壁方块（贴在墙上）
     * @param properties 物品属性
     */
    WallOrFloorItem(const Block& floorBlock, const Block& wallBlock, ItemProperties properties);

    ~WallOrFloorItem() override = default;

    /**
     * @brief 获取墙壁方块
     * @return 墙壁方块引用
     */
    [[nodiscard]] const Block& wallBlock() const { return *m_wallBlock; }

protected:
    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家视线的最近方向，决定放置地板方块还是墙壁方块。
     * 如果方向是 DOWN，尝试放置地板方块。
     * 如果方向是水平方向，尝试放置墙壁方块。
     *
     * @param context 放置上下文
     * @return 方块状态指针，如果不能放置返回 nullptr
     */
    [[nodiscard]] const BlockState* getStateForPlacement(const BlockItemUseContext& context) const override;

private:
    const Block* m_wallBlock;
};

} // namespace mc
