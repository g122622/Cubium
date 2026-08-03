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
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace {

[[nodiscard]] bool isVegetationGround(const mc::BlockState& groundState)
{
    return (mc::VanillaBlocks::GRASS_BLOCK != nullptr && groundState.is(mc::VanillaBlocks::GRASS_BLOCK)) ||
        (mc::VanillaBlocks::DIRT != nullptr && groundState.is(mc::VanillaBlocks::DIRT)) ||
        (mc::VanillaBlocks::COARSE_DIRT != nullptr && groundState.is(mc::VanillaBlocks::COARSE_DIRT)) ||
        (mc::VanillaBlocks::PODZOL != nullptr && groundState.is(mc::VanillaBlocks::PODZOL)) ||
        (mc::VanillaBlocks::FARMLAND != nullptr && groundState.is(mc::VanillaBlocks::FARMLAND));
}

} // namespace

namespace mc {
namespace blocks {

// ========== TallGrassBlock ==========

TallGrassBlock::TallGrassBlock(const BlockProperties& properties)
    : BushBlock(properties)
{
    // 高草形状：薄的一层
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
}

const CollisionShape& TallGrassBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool TallGrassBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    return isVegetationGround(groundState);
}

// ========== FernBlock ==========

FernBlock::FernBlock(const BlockProperties& properties)
    : TallGrassBlock(properties)
{
    // 蕨类使用与高草相同的形状
}

} // namespace blocks
} // namespace mc
