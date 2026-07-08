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

#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/MultifaceBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 多面方块扩散器（MC MultifaceSpreader）
 *
 * 从某一面出发，按 SpreadType 优先级（SAME_POSITION > SAME_PLANE > WRAP_AROUND）
 * 向一个目标方向尝试扩散：原地加面 / 同平面相邻格 / 对角卷绕。
 *
 * 与 MC 默认 DefaultSpreaderConfig 行为一致：
 * - 同轴方向不扩散（spreadDir.getAxis() == fromFace.getAxis() 返回空）。
 * - 源方块需在 fromFace 有面（hasFace），且目标方向暂无面（避免重复）。
 * - 目标格可替换：空气、同种多面方块、或水源。
 */
class MultifaceSpreader {
public:
    explicit MultifaceSpreader(const MultifaceBlock& block)
        : m_block(block)
    {}

    /**
     * @brief MC spreadFromFaceTowardRandomDirection：
     *        从 fromFace 出发，对打乱后的所有方向逐个尝试扩散，命中即止。
     * @return 是否至少成功扩散一次。
     */
    bool spreadFromFaceTowardRandomDirection(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, math::IRandom& random) const;

private:
    /// MC getSpreadFromFaceTowardDirection：计算单方向的 SpreadPos 并放置。
    bool spreadFromFaceTowardDirection(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction fromFace, Direction spreadDir) const;

    /// MC SpreadType.getSpreadPos 的三种结果：(目标格, 新加面方向)。
    struct SpreadPos {
        BlockPos pos;
        Direction face;
    };

    /// 计算 SpreadPos 列表（SAME_POSITION / SAME_PLANE / WRAP_AROUND 优先级）。
    std::vector<SpreadPos> candidateSpreadPos(const BlockPos& pos, Direction spreadDir, Direction fromFace) const;

    /// MC DefaultSpreaderConfig.stateCanBeReplaced：空气 / 同种多面方块 / 水源。
    bool canSpreadInto(IWorld& world, const BlockPos& targetPos, Direction face) const;

    /// MC spreadToFace：在 targetPos 的 face 方向加一面。
    void spreadToFace(IWorld& world, const BlockPos& targetPos, Direction face) const;

    const MultifaceBlock& m_block;
};

} // namespace blocks
} // namespace mc
