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
 */

#pragma once

#include "BlockPredicate.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include <memory>

namespace mc::world::gen::feature::predicate {

/**
 * @brief 环境扫描谓词
 *
 * 从起始位置沿指定方向扫描，寻找满足条件的方块面。
 * 用于洞穴植被放置中寻找天花板/地面。
 */
class EnvironmentScanPredicate {
public:
    /**
     * @brief 构造环境扫描谓词
     * @param direction 扫描方向
     * @param targetCondition 目标条件（找到此条件时停止）
     * @param abortCondition 终止条件（遇到此条件时停止，表示找不到）
     * @param maxSteps 最大扫描步数
     */
    EnvironmentScanPredicate(Direction direction,
        std::unique_ptr<BlockPredicate> targetCondition,
        std::unique_ptr<BlockPredicate> abortCondition,
        i32 maxSteps)
        : m_direction(direction)
        , m_targetCondition(std::move(targetCondition))
        , m_abortCondition(std::move(abortCondition))
        , m_maxSteps(maxSteps)
    {}

    /**
     * @brief 执行环境扫描
     * @param world 世界读取接口
     * @param startPos 起始位置（输出：扫描结果位置）
     * @return 是否找到满足条件的位置
     */
    [[nodiscard]] bool scan(const IWorld& world, BlockPos& startPos) const;

private:
    Direction m_direction;
    std::unique_ptr<BlockPredicate> m_targetCondition;
    std::unique_ptr<BlockPredicate> m_abortCondition;
    i32 m_maxSteps;
};

} // namespace mc::world::gen::feature::predicate
