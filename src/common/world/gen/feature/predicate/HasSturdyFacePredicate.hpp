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
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc::world::gen::feature::predicate {

/**
 * @brief 检查相邻方块是否有指定方向的坚固面的谓词
 *
 * 扫描指定位置，判断某个方向上是否有实心方块的坚固面。
 * 使用 isSolidSide 判断方块的指定面是否坚固，检查方向取反
 * （即检查 pos 处方块在 direction 方向的面，需要查询该面相邻方块的反方向）。
 */
class HasSturdyFacePredicate : public BlockPredicate {
public:
    /**
     * @brief 构造谓词
     * @param direction 检查的方向（检查pos处方块在direction方向的面）
     * @param offset 相对偏移量（默认为零偏移）
     */
    explicit HasSturdyFacePredicate(Direction direction, BlockPos offset = BlockPos())
        : m_direction(direction)
        , m_offset(offset)
    {}

    [[nodiscard]] bool test(const IWorld& world, const BlockPos& pos) const override;
    [[nodiscard]] std::unique_ptr<BlockPredicate> clone() const override
    {
        return std::make_unique<HasSturdyFacePredicate>(m_direction, m_offset);
    }

    [[nodiscard]] Direction getDirection() const { return m_direction; }
    [[nodiscard]] const BlockPos& getOffset() const { return m_offset; }

private:
    Direction m_direction;
    BlockPos m_offset;
};

} // namespace mc::world::gen::feature::predicate
