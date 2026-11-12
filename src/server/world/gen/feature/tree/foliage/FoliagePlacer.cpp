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

#include "FoliagePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/FeatureSpread.hpp"
#include "server/world/gen/feature/state/BlockStateProvider.hpp"
#include "server/world/gen/feature/tree/TreeFeature.hpp"
#include "server/world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

namespace mc {

namespace {

bool _canReplaceFoliage(WorldGenRegion& world, const BlockPos& pos, const ISet* leafPositionSet)
{
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 已由本棵树落过叶的位置（例如多根树干共用的树冠）不再重复落位。
    if (leafPositionSet != nullptr && leafPositionSet->isSet(pos)) {
        return false;
    }

    const BlockState* state = world.getBlockState(pos);
    const bool persistent = state != nullptr && state->hasProperty(BlockStateProperties::PERSISTENT()) &&
        state->get(BlockStateProperties::PERSISTENT());
    return !persistent && TreeFeature::isReplaceableAt(world, pos);
}

void _placeLeaf(WorldGenRegion& world, const BlockPos& pos, const BlockState& state)
{
    const BlockState* placedState = &state;
    if (state.hasProperty(BlockStateProperties::WATERLOGGED())) {
        const bool sourceWater = waterloggable::isWaterSourceFluidState(world.getFluidState(pos.x, pos.y, pos.z));
        placedState = &state.with(BlockStateProperties::WATERLOGGED(), sourceWater);
    }
    world.setBlockState(pos, placedState);
}

} // namespace

FoliagePlacer::FoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset)
    : m_radius(radius)
    , m_offset(offset)
{}

bool FoliagePlacer::shouldSkip(math::IRandom& /*random*/, i32 dx, i32 /*dy*/, i32 dz, i32 radius, bool trunkTop) const
{
    // 基类默认实现：跳过角落
    if (trunkTop) {
        i32 absDx = std::abs(dx);
        i32 absDz = std::abs(dz);
        absDx = std::min(absDx, std::abs(dx - 1));
        absDz = std::min(absDz, std::abs(dz - 1));
        return absDx == radius && absDz == radius;
    }
    return std::abs(dx) == radius && std::abs(dz) == radius;
}

FoliageShape FoliagePlacer::sampleShape(math::IRandom& random, i32 trunkHeight) const
{
    const i32 height = getFoliageHeight(random, trunkHeight);
    const i32 radius = m_radius.get(random);
    return {height, radius};
}

void FoliagePlacer::placeFoliage(WorldGenRegion& world,
    math::IRandom& random,
    i32 trunkHeight,
    const std::vector<FoliagePosition>& foliagePositions,
    const FoliageShape& shape,
    const BlockState* foliageBlock,
    const world::gen::feature::state::BlockStateProvider* foliageProvider,
    std::set<BlockPos>& outFoliageBlocks,
    ISet* outLeafPositionSet)
{
    if (foliageProvider == nullptr && foliageBlock == nullptr) {
        return;
    }

    // 每个附着点先采样偏移，再在形状遍历中逐个候选位置落位（状态采样也在遍历中发生）。
    for (const auto& foliagePos : foliagePositions) {
        const i32 offset = m_offset.get(random);
        placeFoliageInternal(world,
            random,
            trunkHeight,
            foliagePos,
            shape.height,
            shape.radius,
            offset,
            outFoliageBlocks,
            foliageBlock,
            foliageProvider,
            outLeafPositionSet);
    }
}

bool FoliagePlacer::tryPlaceLeaf(WorldGenRegion& world,
    math::IRandom& random,
    const BlockPos& pos,
    const BlockState* foliageBlock,
    const world::gen::feature::state::BlockStateProvider* foliageProvider,
    std::set<BlockPos>& foliageBlocks,
    ISet* leafPositionSet)
{
    if (!_canReplaceFoliage(world, pos, leafPositionSet)) {
        return false;
    }

    // 状态提供者必须在这里采样，且只在真正落位时采样：加权提供者会消耗随机数，
    // 采样次数与时机直接决定后续附着点/装饰器的随机流。
    const BlockState* state =
        foliageProvider != nullptr ? foliageProvider->getState(world, random, pos.x, pos.y, pos.z) : foliageBlock;
    if (state == nullptr) {
        return false;
    }

    _placeLeaf(world, pos, *state);
    foliageBlocks.insert(pos);
    if (leafPositionSet != nullptr) {
        leafPositionSet->set(pos, *state);
    }
    return true;
}

void FoliagePlacer::placeFoliageLayer(WorldGenRegion& world,
    math::IRandom& random,
    const BlockPos& centerPos,
    i32 radius,
    std::set<BlockPos>& foliageBlocks,
    i32 y,
    bool trunkTop,
    const BlockState* foliageBlock,
    const world::gen::feature::state::BlockStateProvider* foliageProvider,
    ISet* leafPositionSet)
{
    // 遍历半径范围内的所有方块
    i32 radiusOffset = trunkTop ? 1 : 0;
    BlockPos pos;

    for (i32 dx = -radius; dx <= radius + radiusOffset; ++dx) {
        for (i32 dz = -radius; dz <= radius + radiusOffset; ++dz) {
            // 检查是否跳过该位置
            if (shouldSkip(random, dx, y - centerPos.y, dz, radius, trunkTop)) {
                continue;
            }

            pos.x = centerPos.x + dx;
            pos.y = y;
            pos.z = centerPos.z + dz;

            // 越界、持久树叶与已落位检查、状态采样均在 tryPlaceLeaf 内完成
            tryPlaceLeaf(world, random, pos, foliageBlock, foliageProvider, foliageBlocks, leafPositionSet);
        }
    }
}

} // namespace mc
