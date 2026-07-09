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

#include "DripstoneUtils.hpp"

#include "common/util/Direction.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <cmath>

namespace mc {

double DripstoneUtils::getDripstoneHeight(double radius, double scale, double heightScale, double bluntness)
{
    if (radius < bluntness) {
        radius = bluntness;
    }

    constexpr double d0 = 0.384;
    const double d1 = radius / scale * 0.384;
    const double d2 = 0.75 * std::pow(d1, 1.3333333333333333);
    const double d3 = std::pow(d1, 0.6666666666666666);
    const double d4 = 0.3333333333333333 * std::log(d1);
    double d5 = heightScale * (d2 - d3 - d4);
    d5 = std::max(d5, 0.0);
    return d5 / 0.384 * scale;
}

bool DripstoneUtils::isCircleMostlyEmbeddedInStone(IWorld& world, const BlockPos& pos, i32 radius)
{
    if (isEmptyOrWaterOrLava(world, pos)) {
        return false;
    }

    const float step = 6.0F / static_cast<float>(radius);
    for (float angle = 0.0F; angle < math::TWO_PI; angle += step) {
        const i32 dx = static_cast<i32>(std::cos(angle) * radius);
        const i32 dz = static_cast<i32>(std::sin(angle) * radius);
        if (isEmptyOrWaterOrLava(world, BlockPos(pos.x + dx, pos.y, pos.z + dz))) {
            return false;
        }
    }
    return true;
}

bool DripstoneUtils::isEmptyOrWater(IWorld& world, const BlockPos& pos)
{
    return isEmptyOrWater(world.getBlockState(pos));
}

bool DripstoneUtils::isEmptyOrWaterOrLava(IWorld& world, const BlockPos& pos)
{
    return isEmptyOrWaterOrLava(world.getBlockState(pos));
}

bool DripstoneUtils::isDripstoneBaseOrLava(const BlockState* state)
{
    return isDripstoneBase(state) || (state != nullptr && state->is(VanillaBlocks::LAVA));
}

bool DripstoneUtils::isDripstoneBase(const BlockState* state)
{
    if (state == nullptr) {
        return false;
    }
    return state->is(VanillaBlocks::DRIPSTONE_BLOCK) || BlockTags::DRIPSTONE_REPLACEABLE().contains(*state);
}

bool DripstoneUtils::isEmptyOrWater(const BlockState* state)
{
    // getBlockState 对空气返回 nullptr（项目约定：nullptr 即空气）。
    return state == nullptr || state->isAir() || state->is(VanillaBlocks::WATER);
}

bool DripstoneUtils::isNeitherEmptyNorWater(const BlockState* state)
{
    return !isEmptyOrWater(state);
}

bool DripstoneUtils::isEmptyOrWaterOrLava(const BlockState* state)
{
    return isEmptyOrWater(state) || (state != nullptr && state->is(VanillaBlocks::LAVA));
}

void DripstoneUtils::buildBaseToTipColumn(
    Direction direction, i32 height, bool merge, std::function<void(const BlockState&)> emitter)
{
    if (height >= 3) {
        emitter(*createPointedDripstone(direction, BlockStateProperties::DripstoneThickness::Base));
        for (i32 i = 0; i < height - 3; ++i) {
            emitter(*createPointedDripstone(direction, BlockStateProperties::DripstoneThickness::Middle));
        }
    }

    if (height >= 2) {
        emitter(*createPointedDripstone(direction, BlockStateProperties::DripstoneThickness::Frustum));
    }

    if (height >= 1) {
        const auto tip =
            merge ? BlockStateProperties::DripstoneThickness::TipMerge : BlockStateProperties::DripstoneThickness::Tip;
        emitter(*createPointedDripstone(direction, tip));
    }
}

void DripstoneUtils::growPointedDripstone(
    IWorld& world, const BlockPos& pos, Direction direction, i32 height, bool merge)
{
    if (isDripstoneBase(world.getBlockState(pos.offset(Directions::opposite(direction))))) {
        BlockPosMutable cursor(pos);
        buildBaseToTipColumn(direction, height, merge, [&](const BlockState& state) {
            const BlockState* toPlace = &state;
            // 尖端滴石需要按当前格是否含水设置 WATERLOGGED。
            if (state.is(VanillaBlocks::POINTED_DRIPSTONE)) {
                toPlace = &state.with(BlockStateProperties::WATERLOGGED(), world.isWaterAt(cursor));
            }
            world.setBlockState(cursor.x, cursor.y, cursor.z, toPlace);
            cursor.move(direction);
        });
    }
}

bool DripstoneUtils::placeDripstoneBlockIfPossible(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state != nullptr && BlockTags::DRIPSTONE_REPLACEABLE().contains(*state)) {
        world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::DRIPSTONE_BLOCK->defaultState());
        return true;
    }
    return false;
}

const BlockState* DripstoneUtils::createPointedDripstone(
    Direction direction, BlockStateProperties::DripstoneThickness thickness)
{
    return &VanillaBlocks::POINTED_DRIPSTONE->defaultState()
                .with(BlockStateProperties::VERTICAL_DIRECTION(), direction)
                .with(BlockStateProperties::DRIPSTONE_THICKNESS(), thickness);
}

} // namespace mc
