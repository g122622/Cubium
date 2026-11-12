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

#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/blocks/decorative/SkullBlock.hpp"
#include "common/world/block/blocks/decorative/WallSkullBlock.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 玩家头颅方块
 *
 * 站立玩家头颅，放置时从物品的 SkullOwner 组件读取档案写入方块实体。
 *
 */
class PlayerHeadBlock : public SkullBlock {
public:
    explicit PlayerHeadBlock(const BlockProperties& properties);

    void onBlockPlacedBy(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const Entity* placer,
        const ItemStack& stack) override;
};

/**
 * @brief 贴墙玩家头颅方块
 */
class PlayerWallHeadBlock : public WallSkullBlock {
public:
    explicit PlayerWallHeadBlock(const BlockProperties& properties);

    void onBlockPlacedBy(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const Entity* placer,
        const ItemStack& stack) override;
};

} // namespace blocks
} // namespace mc
