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

#include "TreeDecorator.hpp"
#include "common/core/Types.hpp"
#include "server/world/gen/feature/state/BlockStateProvider.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

/**
 * @brief 在地面散布方块装饰器（MC PlaceOnGroundDecorator，1.21.5+ 新增类型）
 *
 * 以"最低树干或树根"所在层为基准，取该层树干/树根的水平张成矩形，再按
 * radius/height 向四周与上下扩张成包围盒；在盒内随机取点 tries 次，每次尝试
 * 在该点**正上方**放置方块，需同时满足：
 *   1. 上方格为空气或藤蔓（`isAir() || is(VINE)`）
 *   2. 该点自身满足 checkBlock(isSolidRender)——完整固体，即"站在实地上"
 *   3. 该列 MOTION_BLOCKING_NO_LEAVES 高度不高于上方格（避免把方块埋进地表以下）
 *
 * 树叶散落（`minecraft:leaf_litter`）即由它产生：birch/oak 的
 * `birch_bees_0002_leaf_litter` 等树型各挂两个该装饰器（tries 96/150）。
 * 未实现该类型时，这些树照常生成但一株落叶都不落，表现为 leaf_litter 完全缺失。
 *
 * 配置字段：tries(正整数，默认 128) / radius(非负，默认 2) / height(非负，默认 1) /
 * block_state_provider(BlockStateProvider)。
 */
class PlaceOnGroundDecorator final : public TreeDecorator {
public:
    PlaceOnGroundDecorator(
        i32 tries, i32 radius, i32 height, std::unique_ptr<state::BlockStateProvider> blockStateProvider);

    void place(const TreeDecoratorContext& context) const override;

private:
    /// MC PlaceOnGroundDecorator.attemptToPlaceBlockAbove。
    void _attemptToPlaceBlockAbove(const TreeDecoratorContext& context, const BlockPos& pos) const;

    i32 m_tries;
    i32 m_radius;
    i32 m_height;
    std::unique_ptr<state::BlockStateProvider> m_blockStateProvider;
};

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
