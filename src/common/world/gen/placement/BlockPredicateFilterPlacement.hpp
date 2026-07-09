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

#include "../../../core/Types.hpp"
#include "Placement.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include <memory>

namespace mc {

/**
 * @brief 方块谓词过滤放置配置
 *
 * 对应 MC 1.21.11 BlockPredicateFilterPlacement{predicate}。
 * 持有一个 BlockPredicate，getPositions 时对 basePos 测试谓词，
 * 通过则返回 [basePos]，否则返回空列表。
 */
struct BlockPredicateFilterConfig : public IPlacementConfig {
    /// 谓词（拥有所有权）
    std::unique_ptr<world::gen::feature::predicate::BlockPredicate> predicate;

    explicit BlockPredicateFilterConfig(std::unique_ptr<world::gen::feature::predicate::BlockPredicate> pred)
        : predicate(std::move(pred))
    {}
};

/**
 * @brief 方块谓词过滤放置器
 *
 * 仅当 basePos 处方块满足谓词时才保留该位置。
 */
class BlockPredicateFilterPlacement : public Placement {
public:
    [[nodiscard]] std::vector<BlockPos> getPositions(WorldGenRegion& region,
        math::Random& random,
        const IPlacementConfig& config,
        const BlockPos& basePos) const override;

    [[nodiscard]] const char* name() const noexcept override { return "block_predicate_filter"; }
};

} // namespace mc
