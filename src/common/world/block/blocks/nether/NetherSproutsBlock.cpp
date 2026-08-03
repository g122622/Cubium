/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "NetherSproutsBlock.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// 下界苗的碰撞箱形状：宽12像素（从2到14），高3像素
static const CollisionShape s_sproutsShape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.1875f, 0.875f);

NetherSproutsBlock::NetherSproutsBlock(const BlockProperties& properties)
    : BushBlock(properties)
{}

const CollisionShape& NetherSproutsBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return s_sproutsShape;
}

bool NetherSproutsBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 下界苗可放置在菌岩（绯红/诡异）、灵魂土上
    if (VanillaBlocks::CRIMSON_NYLIUM != nullptr && groundState.is(VanillaBlocks::CRIMSON_NYLIUM)) {
        return true;
    }
    if (VanillaBlocks::WARPED_NYLIUM != nullptr && groundState.is(VanillaBlocks::WARPED_NYLIUM)) {
        return true;
    }
    if (VanillaBlocks::SOUL_SOIL != nullptr && groundState.is(VanillaBlocks::SOUL_SOIL)) {
        return true;
    }

    // 默认支撑面（草方块、泥土、砂土、灰化土、耕地等）
    return BushBlock::canSustain(groundState, world, groundPos);
}

PlantType NetherSproutsBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Nether;
}

} // namespace blocks
} // namespace mc
