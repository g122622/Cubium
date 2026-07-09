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

#include "BasaltFeature.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

namespace {

/// MC 1.21.11: BasaltColumnsFeature.CANNOT_PLACE_ON（10 个方块）
[[nodiscard]] bool isCannotPlaceOnBlock(const Block& block)
{
    return &block == VanillaBlocks::LAVA || &block == VanillaBlocks::BEDROCK || &block == VanillaBlocks::MAGMA ||
        &block == VanillaBlocks::SOUL_SAND || &block == VanillaBlocks::NETHER_BRICKS ||
        &block == VanillaBlocks::NETHER_BRICK_FENCE || &block == VanillaBlocks::NETHER_BRICK_STAIRS ||
        &block == VanillaBlocks::NETHER_WART || &block == VanillaBlocks::CHEST || &block == VanillaBlocks::SPAWNER;
}

/// MC 1.21.11: isAirOrLavaOcean —— 空气，或海平面及以下的熔岩
[[nodiscard]] bool isAirOrLavaOcean(WorldGenRegion& world, i32 seaLevel, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return true;
    }
    if (state->isAir()) {
        return true;
    }
    return state->is(VanillaBlocks::LAVA) && pos.y <= seaLevel;
}

/// MC 1.21.11: canPlaceAt —— 当前格为空气/熔岩海，且下方非空且不在 CANNOT_PLACE_ON
[[nodiscard]] bool canPlaceAt(WorldGenRegion& world, i32 seaLevel, const BlockPos& pos)
{
    if (!isAirOrLavaOcean(world, seaLevel, pos)) {
        return false;
    }
    const BlockState* below = world.getBlockState(pos.down());
    return below != nullptr && !below->isAir() && !isCannotPlaceOnBlock(below->getBlock());
}

/// MC 1.21.11: findSurface —— 向下搜索可放置表面（返回表面位置或无效哨兵）
/// 与 MC 不同的是用 BlockPos 是否合法区分；这里用 minY+1 作为「未找到」哨兵。
[[nodiscard]] BlockPos findSurface(WorldGenRegion& world, i32 seaLevel, BlockPos pos, i32 budget)
{
    while (pos.y > world::MIN_BUILD_HEIGHT + 1 && budget > 0) {
        --budget;
        if (canPlaceAt(world, seaLevel, pos)) {
            return pos;
        }
        pos = pos.down();
    }
    return BlockPos(pos.x, world::MIN_BUILD_HEIGHT, pos.z);
}

/// MC 1.21.11: findAir —— 向上搜索空气（遇 CANNOT_PLACE_ON 则放弃，返回 minY 哨兵）
[[nodiscard]] BlockPos findAir(WorldGenRegion& world, BlockPos pos, i32 budget)
{
    while (pos.y <= world::MAX_BUILD_HEIGHT && budget > 0) {
        --budget;
        const BlockState* state = world.getBlockState(pos);
        if (state != nullptr && isCannotPlaceOnBlock(state->getBlock())) {
            return BlockPos(pos.x, world::MIN_BUILD_HEIGHT, pos.z);
        }
        if (state != nullptr && state->isAir()) {
            return pos;
        }
        pos = pos.up();
    }
    return BlockPos(pos.x, world::MIN_BUILD_HEIGHT, pos.z);
}

/// 哨兵判定：findSurface/findAir 返回的「未找到」位置
[[nodiscard]] inline bool isSentinel(const BlockPos& p)
{
    return p.y <= world::MIN_BUILD_HEIGHT;
}

/// MC 1.21.11: 聚类/非聚类半径与柱数
constexpr i32 CLUSTERED_REACH = 5;
constexpr i32 CLUSTERED_SIZE = 50;
constexpr i32 UNCLUSTERED_REACH = 8;
constexpr i32 UNCLUSTERED_SIZE = 15;

} // namespace

// ============================================================================
// BasaltColumnFeature 实现
// ============================================================================

bool BasaltColumnFeature::place(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    i32 seaLevel,
    const BasaltColumnFeatureConfig& config)
{
    // MC: !canPlaceAt → return false
    if (!canPlaceAt(world, seaLevel, pos)) {
        return false;
    }

    // MC: height = config.height().sample(rng)
    const i32 height = config.height != nullptr ? config.height->sample(random) : 0;
    // MC: flag = nextFloat() < 0.9；k = min(height, flag?5:8)；l = flag?50:15
    const bool clustered = random.nextFloat() < 0.9f;
    const i32 reach = std::min(height, clustered ? CLUSTERED_REACH : UNCLUSTERED_REACH);
    const i32 sampleCount = clustered ? CLUSTERED_SIZE : UNCLUSTERED_SIZE;

    bool placedAny = false;
    // MC: BlockPos.randomBetweenClosed(rng, l, x-k..x+k, y, z-k..z+k)
    for (i32 i = 0; i < sampleCount; ++i) {
        const i32 dx = random.nextInt(reach * 2 + 1) - reach;
        const i32 dz = random.nextInt(reach * 2 + 1) - reach;
        BlockPos samplePos(pos.x + dx, pos.y, pos.z + dz);
        // MC: i1 = j - distManhattan；if (i1 >= 0) placeColumn(..., i1, reach.sample)
        const i32 manhattan = samplePos.manhattanDistance(pos);
        const i32 columnHeight = height - manhattan;
        if (columnHeight < 0) {
            continue;
        }
        const i32 columnReach = config.reach != nullptr ? config.reach->sample(random) : 0;
        if (placeColumn(world, seaLevel, samplePos, columnHeight, columnReach)) {
            placedAny = true;
        }
    }
    return placedAny;
}

bool BasaltColumnFeature::placeColumn(
    WorldGenRegion& world, i32 seaLevel, const BlockPos& center, i32 columnHeight, i32 reach)
{
    const BlockState* const basalt = VanillaBlocks::getState(VanillaBlocks::BASALT);
    if (basalt == nullptr) {
        return false;
    }

    bool placedAny = false;
    // MC: BlockPos.betweenClosed(cx-r, y, cz-r .. cx+r, y, cz+r)
    for (i32 dx = -reach; dx <= reach; ++dx) {
        for (i32 dz = -reach; dz <= reach; ++dz) {
            const BlockPos columnPos(center.x + dx, center.y, center.z + dz);
            const i32 manhattan = std::abs(dx) + std::abs(dz);
            // MC: isAirOrLavaOcean ? findSurface(budget=manhattan) : findAir(budget=manhattan)
            BlockPos start;
            if (isAirOrLavaOcean(world, seaLevel, columnPos)) {
                start = findSurface(world, seaLevel, columnPos, manhattan);
            } else {
                start = findAir(world, columnPos, manhattan);
            }
            if (isSentinel(start)) {
                continue;
            }
            // MC: j = columnHeight - manhattan/2；向上 j+1 格放置
            i32 remaining = columnHeight - manhattan / 2;
            BlockPos cursor = start;
            while (remaining >= 0) {
                if (isAirOrLavaOcean(world, seaLevel, cursor)) {
                    world.setBlockState(cursor, basalt);
                    cursor = cursor.up();
                    placedAny = true;
                } else {
                    const BlockState* s = world.getBlockState(cursor);
                    if (s == nullptr || !s->is(VanillaBlocks::BASALT)) {
                        break;
                    }
                    cursor = cursor.up();
                }
                --remaining;
            }
        }
    }
    return placedAny;
}

// ============================================================================
// ConfiguredBasaltColumnFeature 实现
// ============================================================================

ConfiguredBasaltColumnFeature::ConfiguredBasaltColumnFeature(
    std::unique_ptr<BasaltColumnFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredBasaltColumnFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, generator.seaLevel(), *m_config);
}

} // namespace mc
