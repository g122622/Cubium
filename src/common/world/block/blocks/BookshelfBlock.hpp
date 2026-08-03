/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 书架方块
 *
 * 书架是附魔台附魔力量的来源。当书架被放置或移除时，
 * 需要通知周围2格范围内的附魔台重新计算附魔力量。
 *
 * 由于ServerWorld::setBlockState只会通知直接邻居（1格距离），
 * 而书架可以在2格距离内影响附魔台，因此书架需要主动通知
 * 范围内的附魔台方块实体。
 */
class BookshelfBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BookshelfBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~BookshelfBlock() override = default;

    // ========== 放置和移除 ==========

    /**
     * @brief 书架被放置后，通知附近附魔台重算附魔力量
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 书架被移除后，通知附近附魔台重算附魔力量
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

private:
    /**
     * @brief 通知附近附魔台重新计算附魔力量
     *
     * 遍历以书架为中心5x3x5范围内的所有附魔台方块实体，
     * 调用其recalculateEnchantPower方法。
     * 附魔台有效书架范围是水平2格、垂直0-1格，
     * 因此书架最多影响水平2格、垂直0-1格内的附魔台，
     * 即以书架为中心的5x2x5范围。额外增加垂直范围以覆盖偏移。
     *
     * @param world 世界
     * @param pos 书架位置
     */
    static void _notifyNearbyEnchantingTables(IWorld& world, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
