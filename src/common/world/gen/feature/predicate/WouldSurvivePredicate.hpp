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
#include "common/world/block/BlockState.hpp"
#include <memory>

namespace mc::world::gen::feature::predicate {

/**
 * @brief 检查方块能否在指定位置存活的谓词
 *
 * 调用 Block::isValidPosition 判断方块是否可以放置在目标位置。
 */
class WouldSurvivePredicate : public BlockPredicate {
public:
    /**
     * @brief 构造谓词
     * @param state 要检查的方块状态
     * @param offset 相对偏移量（默认为零偏移）
     */
    WouldSurvivePredicate(const BlockState* state, BlockPos offset = BlockPos())
        : m_state(state)
        , m_offset(offset)
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<WouldSurvivePredicate>(m_state, m_offset);
    }

private:
    const BlockState* m_state;
    BlockPos m_offset;
};

} // namespace mc::world::gen::feature::predicate
