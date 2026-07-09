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

#include "TrunkPlacer.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>

namespace mc {

class BlockTag;

/**
 * @brief 向上分支树干放置器
 *
 * 忠实复刻 MC 1.21.11 UpwardsBranchingTrunkPlacer：主干垂直生长，每段原木按概率
 * 在水平方向长出分支；分支末端追加树叶附着点。用于红树林（mangrove）。
 *
 * 字段：
 * - extraBranchSteps(IntProvider)：每条分支额外的步数采样
 * - placeBranchPerLogProbability(float)：每段原木生成分支的概率
 * - extraBranchLength(IntProvider)：分支长度采样
 * - canGrowThrough(BlockTag)：可穿透方块标签（validTreePos 额外放行）
 */
class UpwardsBranchingTrunkPlacer : public TrunkPlacer {
public:
    UpwardsBranchingTrunkPlacer(i32 baseHeight,
        i32 heightRandA,
        i32 heightRandB,
        std::unique_ptr<world::gen::valueprovider::IntProvider> extraBranchSteps,
        f32 placeBranchPerLogProbability,
        std::unique_ptr<world::gen::valueprovider::IntProvider> extraBranchLength,
        const BlockTag* canGrowThrough);

    std::vector<FoliagePosition> placeTrunk(WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock) override;

    [[nodiscard]] const char* name() const override { return "upwards_branching"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;

private:
    /// MC: placeLog —— 可放置则放置并返回 true，否则 false。
    bool placeLog(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock);

    /// MC: validTreePos —— 基类放行 + canGrowThrough 标签放行
    [[nodiscard]] bool validTreePos(WorldGenRegion& world, const BlockPos& pos) const;

    /// MC: placeBranch —— 从 (trunkX,trunkZ,branchStartY) 沿 direction 水平延伸分支
    void placeBranch(WorldGenRegion& world,
        math::Random& random,
        i32 height,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock,
        std::vector<FoliagePosition>& foliagePositions,
        i32 branchStartY,
        i32 trunkX,
        i32 trunkZ,
        Direction direction,
        i32 branchLengthOffset,
        i32 branchSteps);

    std::unique_ptr<world::gen::valueprovider::IntProvider> m_extraBranchSteps;
    f32 m_placeBranchPerLogProbability;
    std::unique_ptr<world::gen::valueprovider::IntProvider> m_extraBranchLength;
    const BlockTag* m_canGrowThrough;
};

} // namespace mc
