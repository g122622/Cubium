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

#include "common/world/block/blocks/vegetation/FlowerBlock.hpp"

#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {
namespace blocks {

// ========== FlowerBlock ==========

FlowerBlock::FlowerBlock(const BlockProperties& properties, u32 suspiciousStewEffect, i32 effectDuration)
    : BushBlock(properties)
    , m_suspiciousStewEffect(suspiciousStewEffect)
    , m_effectDuration(effectDuration)
{

    // 花朵形状：小型，偏移中心
    // 原版 FlowerBlock.SHAPE = Block.column(6.0, 0.0, 10.0)
    //   = box(5, 0, 5, 11, 10, 11) -> X/Z 0.3125..0.6875，Y 上限 10/16 = 0.625。
    // 此前 Y 上限写成 0.375（6/16），矮了 4 像素，选中框明显偏小。
    m_shape = CollisionShape::box(0.3125f, 0.0f, 0.3125f, 0.6875f, 0.625f, 0.6875f);
}

const CollisionShape& FlowerBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

CollisionShape FlowerBlock::getShapeAt(const BlockState& state, const BlockPos& pos) const
{
    const Vector3 offset = getOffset(pos);
    return getShape(state).offset(offset.x, offset.y, offset.z);
}

} // namespace blocks
} // namespace mc
