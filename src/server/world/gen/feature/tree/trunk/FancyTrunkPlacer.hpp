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
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <set>
#include <vector>

namespace mc {

/**
 * @brief 精美树干放置器
 *
 * 生成弯曲的树干，用于精美橡树。
 */
class FancyTrunkPlacer : public TrunkPlacer {
public:
    FancyTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    std::vector<FoliagePosition> placeTrunk(WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock) override;

    [[nodiscard]] const char* name() const override { return "fancy"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;

private:
    /**
     * @brief 计算指定相对高度处的分支长度
     *
     * @param trunkHeight 树干总高
     * @param relY 相对起始位置的高度偏移，必须落在 [0, trunkHeight] 区间内
     * @return 分支长度；该高度不生成分支时返回负值
     */
    [[nodiscard]] f32 _getBranchLength(i32 trunkHeight, i32 relY) const;

    /**
     * @brief 沿两点连线逐格推进，探测或放置方块
     *
     * @param place 为 true 时逐格放置方块，为 false 时仅探测路径是否通畅
     */
    bool _makeLimb(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& start,
        const BlockPos& end,
        bool place,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock);

    /**
     * @brief 获取步数
     */
    [[nodiscard]] i32 _getSteps(const BlockPos& delta) const;

    /**
     * @brief 判断是否保留树叶
     */
    [[nodiscard]] bool _shouldKeepFoliage(i32 trunkHeight, i32 relY) const;
};

} // namespace mc
