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
#include "common/world/block/BlockPos.hpp"

namespace mc::world::gen::feature::predicate {

/**
 * @brief 检查位置是否在世界高度范围内的谓词
 *
 * 忠实复刻 MC 1.21.11 InsideWorldBoundsPredicate：
 *   test = !world.isOutsideBuildHeight(pos + offset)
 * 其中 offset 默认为 (0,0,0)。isOutsideBuildHeight 等价于
 * y < MIN_BUILD_HEIGHT || y >= MAX_BUILD_HEIGHT。
 */
class InsideWorldBoundsPredicate : public BlockPredicate {
public:
    InsideWorldBoundsPredicate() = default;
    explicit InsideWorldBoundsPredicate(BlockPos offset) noexcept
        : m_offset(offset)
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<InsideWorldBoundsPredicate>(m_offset);
    }

private:
    BlockPos m_offset{0, 0, 0};
};

} // namespace mc::world::gen::feature::predicate
