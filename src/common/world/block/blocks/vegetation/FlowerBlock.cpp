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

#include "FlowerBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../Material.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/blocks/vegetation/DoublePlantBlock.hpp"

namespace mc {
namespace blocks {

// ========== FlowerBlock ==========

FlowerBlock::FlowerBlock(const BlockProperties& properties, u32 suspiciousStewEffect, i32 effectDuration)
    : BushBlock(properties)
    , m_suspiciousStewEffect(suspiciousStewEffect)
    , m_effectDuration(effectDuration)
{

    // 花朵形状：小型，偏移中心
    m_shape = CollisionShape::box(0.3125f, 0.0f, 0.3125f, 0.6875f, 0.375f, 0.6875f);
}

const CollisionShape& FlowerBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool FlowerBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 花朵可以放置在草方块、泥土、耕地等上
    const Material& material = groundState.getMaterial();
    return material.isSolid();
}

// ========== LilacBlock ==========

LilacBlock::LilacBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties)
{}

// ========== RoseBushBlock ==========

RoseBushBlock::RoseBushBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties)
{}

// ========== PeonyBlock ==========

PeonyBlock::PeonyBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties)
{}

// ========== LargeFernBlock ==========

LargeFernBlock::LargeFernBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties)
{}

// ========== SunflowerBlock ==========

SunflowerBlock::SunflowerBlock(const BlockProperties& properties)
    : DoublePlantBlock(properties)
{}

} // namespace blocks
} // namespace mc
