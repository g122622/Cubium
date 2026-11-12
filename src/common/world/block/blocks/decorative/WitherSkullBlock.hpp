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
 * @brief 凋灵骷髅头颅方块
 *
 * 放置时检测凋灵生成结构（4 个灵魂沙/灵魂土 T 形 + 3 个凋灵骷髅头颅）。
 *
 */
class WitherSkullBlock : public SkullBlock {
public:
    explicit WitherSkullBlock(const BlockProperties& properties);

    void onBlockPlacedBy(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const Entity* placer,
        const ItemStack& stack) override;

    /**
     * @brief 检测并生成凋灵
     *
     * 对应 MC Java 1.21.11 WitherSkullBlock.checkSpawn(Level, BlockPos)。
     */
    static void checkSpawn(IWorld& world, const BlockPos& pos);

    /**
     * @brief 判断在指定位置放下该物品是否具备召唤凋灵的条件
     *
     * 对应原版 `WitherSkullBlock.canSpawnMob(Level, BlockPos, ItemStack)`：物品须是凋灵骷髅头颅、
     * 位置至少比世界最低高度高 2 格、难度非和平、服务端侧，且「基座图案」在 pos 处匹配
     * （顶层任意方块、中层 3 个基座、底层 空气-基座-空气）。
     *
     * 发射器放置头颅前用它做前置判定：不满足时原版不会放置头颅，而是走装备穿戴分支。
     *
     * @param world 世界
     * @param pos 头颅将要放置的位置（图案顶行中间）
     * @param stack 待放置的物品堆
     * @return 是否具备召唤条件
     */
    [[nodiscard]] static bool canSpawnMob(IWorld& world, const BlockPos& pos, const ItemStack& stack);
};

/**
 * @brief 贴墙凋灵骷髅头颅方块
 */
class WitherWallSkullBlock : public WallSkullBlock {
public:
    explicit WitherWallSkullBlock(const BlockProperties& properties);

    void onBlockPlacedBy(IWorld& world,
        const BlockPos& pos,
        const BlockState& state,
        const Entity* placer,
        const ItemStack& stack) override;
};

} // namespace blocks
} // namespace mc
