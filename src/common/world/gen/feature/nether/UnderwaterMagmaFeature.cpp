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

#include "UnderwaterMagmaFeature.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

namespace {

/// MC: isWaterOrAir
[[nodiscard]] bool isWaterOrAir(const BlockState* state)
{
    if (state == nullptr) {
        return true; // nullptr 视作空气
    }
    return state->is(VanillaBlocks::WATER) || state->isAir();
}

/// MC: Column.scanDirection（DOWN 方向）—— 起点为原点 Y，循环条件 i<range 且当前位置为水时
/// 向下移动；结束时若当前位置非水则返回其 Y，否则 empty。
/// MC: Column.scan 要求原点自身须满足 predicate(isWater)，否则整体返回 empty。
[[nodiscard]] std::optional<i32> scanDownForFloor(WorldGenRegion& world, const BlockPos& pos, i32 range)
{
    const BlockState* origin = world.getBlockState(pos);
    if (origin == nullptr || !origin->is(VanillaBlocks::WATER)) {
        return std::nullopt;
    }

    // MC: mutable.setY(originY); for (i=1; i<range && isStateAtPosition(mutable, isWater); i++) mutable.move(DOWN);
    BlockPos cursor = pos;
    for (i32 i = 1; i < range; ++i) {
        const BlockState* cur = world.getBlockState(cursor);
        if (cur == nullptr || !cur->is(VanillaBlocks::WATER)) {
            break;
        }
        cursor = cursor.down();
    }

    // MC: isStateAtPosition(cursor, isNotWater) ? cursor.y : empty
    const BlockState* stop = world.getBlockState(cursor);
    if (stop != nullptr && !stop->is(VanillaBlocks::WATER)) {
        return cursor.y;
    }
    return std::nullopt;
}

} // namespace

bool UnderwaterMagmaFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const UnderwaterMagmaConfig& config)
{
    const std::optional<i32> floorY = getFloorY(world, pos, config);
    if (!floorY.has_value()) {
        return false;
    }

    // MC: blockpos1 = blockpos.atY(floorY)；vec3i = (radius, radius, radius)；
    //     boundingbox = fromCorners(blockpos1 - vec3i, blockpos1 + vec3i)
    const BlockPos floorPos(pos.x, *floorY, pos.z);
    const i32 r = config.placementRadiusAroundFloor;

    bool placedAny = false;
    // MC: BlockPos.betweenClosedStream(boundingBox) —— 立方体全迭代
    for (i32 dx = -r; dx <= r; ++dx) {
        for (i32 dy = -r; dy <= r; ++dy) {
            for (i32 dz = -r; dz <= r; ++dz) {
                const BlockPos candidate(floorPos.x + dx, floorPos.y + dy, floorPos.z + dz);
                // MC: filter(nextFloat() < placementProbabilityPerValidPosition)
                if (random.nextFloat() >= config.placementProbabilityPerValidPosition) {
                    continue;
                }
                // MC: filter(isValidPlacement)
                if (!isValidPlacement(world, candidate)) {
                    continue;
                }
                // MC: setBlock(pos, MAGMA_BLOCK.defaultBlockState(), 2)
                const BlockState* magma =
                    VanillaBlocks::MAGMA != nullptr ? &VanillaBlocks::MAGMA->defaultState() : nullptr;
                if (magma != nullptr) {
                    world.setBlockState(candidate, magma);
                    placedAny = true;
                }
            }
        }
    }

    return placedAny;
}

std::optional<i32> UnderwaterMagmaFeature::getFloorY(
    WorldGenRegion& world, const BlockPos& pos, const UnderwaterMagmaConfig& config)
{
    // MC: predicate=isWater, predicate1=isNotWater；Column.scan(...).getFloor()
    // getFloor 取向下扫描结果（在水柱下方首个非水方块）
    return scanDownForFloor(world, pos, config.floorSearchRange);
}

bool UnderwaterMagmaFeature::isValidPlacement(WorldGenRegion& world, const BlockPos& pos)
{
    // MC: !isWaterOrAir(self) && !isVisibleFromOutside(below, UP)
    //     && 四水平邻居均 !isVisibleFromOutside(relative, opposite(dir))
    const BlockState* self = world.getBlockState(pos);
    if (isWaterOrAir(self)) {
        return false;
    }
    if (isVisibleFromOutside(world, pos.down(), Direction::Up)) {
        return false;
    }

    static const Direction horizontal[4] = {Direction::North, Direction::South, Direction::East, Direction::West};
    for (Direction dir : horizontal) {
        // MC: 任意水平邻居对外可见 → return false
        if (isVisibleFromOutside(world, pos.offset(dir), Directions::opposite(dir))) {
            return false;
        }
    }
    return true;
}

bool UnderwaterMagmaFeature::isVisibleFromOutside(WorldGenRegion& world, const BlockPos& pos, Direction dir)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        // 空气/越界：遮挡形状为空 → 对外可见
        return true;
    }
    // MC: voxelshape = state.getFaceOcclusionShape(dir);
    //     return voxelshape == Shapes.empty() || !Block.isShapeFullBlock(voxelshape);
    const CollisionShape shape = state->getFaceOcclusionShape(dir);
    return shape.isEmpty() || !shape.coversFullBlock();
}

// ============================================================================
// ConfiguredUnderwaterMagmaFeature 实现
// ============================================================================

ConfiguredUnderwaterMagmaFeature::ConfiguredUnderwaterMagmaFeature(
    UnderwaterMagmaConfig config, const char* featureName)
    : m_config(config)
    , m_name(featureName)
{}

bool ConfiguredUnderwaterMagmaFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, m_config);
}

} // namespace mc
