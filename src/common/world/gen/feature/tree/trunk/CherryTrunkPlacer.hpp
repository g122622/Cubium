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
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <set>
#include <vector>

namespace mc {

/**
 * @brief 樱花树干放置器
 *
 * 生成带有弯曲分支的樱花树干。树干高度7-8格，
 * 随机产生1-3个分支，分支沿水平方向延伸后弯向目标高度。
 */
class CherryTrunkPlacer : public TrunkPlacer {
public:
    /**
     * @brief 构造樱花树干放置器
     * @param baseHeight 基础高度（7）
     * @param heightRandA 高度随机值A（1）
     * @param heightRandB 高度随机值B（0）
     * @param branchCountMin 最小分支数（1）
     * @param branchCountMax 最大分支数（3）
     * @param branchHorizontalLengthMin 分支水平长度最小值（2）
     * @param branchHorizontalLengthMax 分支水平长度最大值（4）
     * @param branchStartOffsetFromTopMin 分支起始偏移最小值（-4）
     * @param branchStartOffsetFromTopMax 分支起始偏移最大值（-3）
     * @param branchEndOffsetFromTopMin 分支末端偏移最小值（-1）
     * @param branchEndOffsetFromTopMax 分支末端偏移最大值（0）
     */
    CherryTrunkPlacer(i32 baseHeight,
        i32 heightRandA,
        i32 heightRandB,
        i32 branchCountMin,
        i32 branchCountMax,
        i32 branchHorizontalLengthMin,
        i32 branchHorizontalLengthMax,
        i32 branchStartOffsetFromTopMin,
        i32 branchStartOffsetFromTopMax,
        i32 branchEndOffsetFromTopMin,
        i32 branchEndOffsetFromTopMax);

    std::vector<FoliagePosition> placeTrunk(WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock) override;

    [[nodiscard]] const char* name() const override { return "cherry"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;

private:
    /**
     * @brief 生成单个分支
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param treeHeight 树总高度
     * @param startPos 树干起始位置
     * @param trunkBlock 树干方块
     * @param direction 分支水平方向
     * @param branchStartHeight 分支起始高度
     * @param canExtend 是否可以延伸（树干高度 > 分支起始高度+1）
     * @param trunkBlocks 树干方块集合
     * @return 分支末端的树叶位置
     */
    FoliagePosition generateBranch(WorldGenRegion& world,
        math::Random& random,
        i32 treeHeight,
        const BlockPos& startPos,
        const BlockState* trunkBlock,
        Direction direction,
        i32 branchStartHeight,
        bool canExtend,
        std::set<BlockPos>& trunkBlocks);

    i32 m_branchCountMin;
    i32 m_branchCountMax;
    i32 m_branchHorizontalLengthMin;
    i32 m_branchHorizontalLengthMax;
    i32 m_branchStartOffsetFromTopMin;
    i32 m_branchStartOffsetFromTopMax;
    i32 m_branchEndOffsetFromTopMin;
    i32 m_branchEndOffsetFromTopMax;
};

} // namespace mc
