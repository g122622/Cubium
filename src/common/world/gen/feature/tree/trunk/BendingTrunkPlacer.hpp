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
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <set>
#include <vector>

namespace mc {

/**
 * @brief 弯曲树干放置器
 *
 * 生成带有水平弯曲的树干，用于杜鹃树和红树林等。
 * 树干先垂直生长，在接近顶部时开始向随机水平方向弯曲，
 * 弯曲部分延伸 bendLength 格水平原木，形成"弯腰"效果。
 * 弯曲部分的每一格都会产生树叶附着点，形成沿弯曲分布的树冠。
 */
class BendingTrunkPlacer : public TrunkPlacer {
public:
    /**
     * @brief 构造弯曲树干放置器
     * @param baseHeight 基础高度
     * @param heightRandA 高度随机值A
     * @param heightRandB 高度随机值B
     * @param minHeightForLeaves 开始产生树叶附着点的最低高度
     * @param bendLength 弯曲部分的水平长度（IntProvider 采样）
     */
    BendingTrunkPlacer(i32 baseHeight,
        i32 heightRandA,
        i32 heightRandB,
        i32 minHeightForLeaves,
        std::unique_ptr<world::gen::valueprovider::IntProvider> bendLength);

    std::vector<FoliagePosition> placeTrunk(WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock) override;

    [[nodiscard]] const char* name() const override { return "bending"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;

private:
    i32 m_minHeightForLeaves;                                             ///< 开始产生树叶附着点的最低高度
    std::unique_ptr<world::gen::valueprovider::IntProvider> m_bendLength; ///< 弯曲部分水平长度
};

} // namespace mc
