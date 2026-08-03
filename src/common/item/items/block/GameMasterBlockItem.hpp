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
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/Item.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

/**
 * @brief 游戏管理员方块物品
 *
 * 限制命令方块、结构方块、拼图方块、屏障方块等的放置权限。
 * 只有拥有 canUseGameMasterBlocks() 权限（创造模式 + OP等级>=2）的玩家
 * 才能放置此类方块。
 *
 * 通过重写 getStateForPlacement() 方法，当玩家不具备权限时返回 nullptr，
 * 从而阻止放置。
 *
 * 参考: net.minecraft.item.GameMasterBlockItem
 */
class GameMasterBlockItem : public BlockItem {
public:
    /**
     * @brief 构造游戏管理员方块物品
     * @param block 关联的方块
     * @param properties 物品属性
     */
    GameMasterBlockItem(const Block& block, ItemProperties properties);

    ~GameMasterBlockItem() override = default;

    /**
     * @brief 获取放置时的方块状态
     *
     * 当玩家没有 canUseGameMasterBlocks() 权限时返回 nullptr，阻止放置。
     * 当玩家为 nullptr（如发射器放置）时，允许放置。
     *
     * @param context 放置上下文
     * @return 方块状态指针，如果不允许放置返回 nullptr
     */
    [[nodiscard]] const BlockState* getStateForPlacement(const BlockItemUseContext& context) const override;

private:
};

} // namespace mc
