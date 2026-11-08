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

#include "common/world/block/blocks/vegetation/TallGrassBlock.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {
namespace blocks {

// ========== TallGrassBlock ==========

TallGrassBlock::TallGrassBlock(const BlockProperties& properties)
    : BushBlock(properties)
{
    // 高草形状：薄的一层
    // 原版 TallGrassBlock.SHAPE = Block.column(12.0, 0.0, 13.0)
    //   = box(2, 0, 2, 14, 13, 14) -> X/Z 0.125..0.875，Y 上限 13/16 = 0.8125。
    // 此前 Y 上限写成 1.0（满高），草叶覆盖整个方块高度。
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.8125f, 0.875f);
}

const CollisionShape& TallGrassBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

// ========== FernBlock ==========

FernBlock::FernBlock(const BlockProperties& properties)
    : TallGrassBlock(properties)
{
    // 蕨类使用与高草相同的形状
}

} // namespace blocks
} // namespace mc
