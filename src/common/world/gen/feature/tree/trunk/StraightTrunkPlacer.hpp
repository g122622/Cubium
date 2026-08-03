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
 * @brief 直树干放置器
 *
 * 用于橡树、白桦、丛林木等简单的垂直树干。
 * 生成一条从底部到顶部的直线树干。
 */
class StraightTrunkPlacer : public TrunkPlacer {
public:
    /**
     * @brief 构造直树干放置器
     * @param baseHeight 基础高度
     * @param heightRandA 高度随机值A
     * @param heightRandB 高度随机值B
     */
    StraightTrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB);

    /**
     * @brief 放置直树干
     *
     * 在起始位置向上生成一条垂直的树干。
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param height 树干高度
     * @param startPos 起始位置
     * @param trunkBlocks 树干方块集合
     * @param trunkBlock 树干方块状态
     * @return 树叶位置列表（只有一个树叶位置在树干顶部）
     */
    std::vector<FoliagePosition> placeTrunk(WorldGenRegion& world,
        math::Random& random,
        i32 height,
        const BlockPos& startPos,
        std::set<BlockPos>& trunkBlocks,
        const BlockState* trunkBlock) override;

    [[nodiscard]] const char* name() const override { return "StraightTrunkPlacer"; }

    [[nodiscard]] std::unique_ptr<TrunkPlacer> clone() const override
    {
        return std::make_unique<StraightTrunkPlacer>(m_baseHeight, m_heightRandA, m_heightRandB);
    }
};

} // namespace mc
