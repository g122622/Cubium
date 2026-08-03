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
 * @brief 深色橡树树干放置器
 *
 * 生成 2x2 的深色橡树树干。
 */
class DarkOakTrunkPlacer : public TrunkPlacer {
public:
    /**
     * @brief 构造函数
     * @param baseHeight 基础高度
     * @param heightRandA 高度随机值A
     * @param heightRandB 高度随机值B
     */
    DarkOakTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    std::vector<FoliagePosition> placeTrunk(WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock) override;

    [[nodiscard]] const char* name() const override { return "dark_oak"; }
    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override;
};

} // namespace mc
