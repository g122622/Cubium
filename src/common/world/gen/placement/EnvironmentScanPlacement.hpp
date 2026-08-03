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

#include "../../../util/Direction.hpp"
#include "../feature/predicate/BlockPredicate.hpp"
#include "Placement.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

namespace predicate = world::gen::feature::predicate;

/**
 * @brief 环境扫描放置配置
 *
 * 参考 MC 1.21.11: EnvironmentScanPlacement
 * 沿指定方向扫描，寻找满足目标条件的位置。
 */
struct EnvironmentScanConfig : public IPlacementConfig {
    /// 扫描方向
    Direction directionOfSearch;

    /// 目标条件（找到满足此条件的位置时返回）
    std::unique_ptr<predicate::BlockPredicate> targetCondition;

    /// 允许搜索条件（扫描路径上的方块必须满足此条件，否则终止扫描）
    std::unique_ptr<predicate::BlockPredicate> allowedSearchCondition;

    /// 最大扫描步数（1~32）
    i32 maxSteps;

    EnvironmentScanConfig(Direction direction,
        std::unique_ptr<predicate::BlockPredicate> target,
        std::unique_ptr<predicate::BlockPredicate> allowed,
        i32 steps)
        : directionOfSearch(direction)
        , targetCondition(std::move(target))
        , allowedSearchCondition(std::move(allowed))
        , maxSteps(std::clamp(steps, 1, 32))
    {}
};

/**
 * @brief 环境扫描放置器
 *
 * 参考 MC 1.21.11: EnvironmentScanPlacement
 * 从当前位置沿指定方向逐步扫描，寻找满足目标条件的位置。
 *
 * 算法：
 * 1. 检查当前位置是否满足 allowedSearchCondition，不满足则返回空
 * 2. 循环 maxSteps 次：
 *    - 若当前位置满足 targetCondition，返回 {currentPos}
 *    - 沿 directionOfSearch 移动一格
 *    - 若越界或不满足 allowedSearchCondition，跳出循环
 * 3. 循环结束后再检查一次 targetCondition
 */
class EnvironmentScanPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "environment_scan"; }
};

} // namespace mc
