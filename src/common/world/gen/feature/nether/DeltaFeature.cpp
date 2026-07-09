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

#include "DeltaFeature.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <algorithm>

namespace mc {

namespace {

/// MC 1.21.11: DeltaFeature.CANNOT_REPLACE
[[nodiscard]] bool isCannotReplace(const Block& block)
{
    return &block == VanillaBlocks::BEDROCK || &block == VanillaBlocks::NETHER_BRICKS ||
        &block == VanillaBlocks::NETHER_BRICK_FENCE || &block == VanillaBlocks::NETHER_BRICK_STAIRS ||
        &block == VanillaBlocks::NETHER_WART || &block == VanillaBlocks::CHEST || &block == VanillaBlocks::SPAWNER;
}

/// MC 1.21.11: DeltaFeature.isClear
/// 位置已为 contents 方块 → false；CANNOT_REPLACE → false；
/// 否则六向邻居：水平/下方必须非空气，上方必须为空气。
[[nodiscard]] bool isClear(WorldGenRegion& world, const BlockPos& pos, const DeltaFeatureConfig& config)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }
    if (config.contents != nullptr && state->is(&config.contents->getBlock())) {
        return false;
    }
    if (isCannotReplace(state->getBlock())) {
        return false;
    }

    // MC: for direction in DIRECTIONS: flag = relative.isAir();
    //     if (flag && dir != UP) || (!flag && dir == UP) return false;
    static const Direction dirs[6] = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};
    for (Direction dir : dirs) {
        const BlockState* rel = world.getBlockState(pos.offset(dir));
        const bool air = rel != nullptr && rel->isAir();
        const bool fail = (air && dir != Direction::Up) || (!air && dir == Direction::Up);
        if (fail) {
            return false;
        }
    }
    return true;
}

/// BlockPos.offset(Direction) 取邻居
} // namespace

bool DeltaFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const DeltaFeatureConfig& config)
{
    if (config.contents == nullptr || config.rim == nullptr) {
        return false;
    }

    bool placedAny = false;
    // MC: flag1 = nextDouble() < 0.9；i/j = flag1 ? rimSize.sample : 0；flag2 = flag1 && i!=0 && j!=0
    const bool rimOffsetActive = random.nextDouble() < 0.9;
    const i32 rimDx = rimOffsetActive && config.rimSize != nullptr ? config.rimSize->sample(random) : 0;
    const i32 rimDz = rimOffsetActive && config.rimSize != nullptr ? config.rimSize->sample(random) : 0;
    const bool placeRim = rimOffsetActive && rimDx != 0 && rimDz != 0;

    // MC: k/l = size.sample；i1 = max(k,l)
    const i32 k = config.size != nullptr ? config.size->sample(random) : 0;
    const i32 l = config.size != nullptr ? config.size->sample(random) : 0;
    const i32 maxReach = std::max(k, l);

    // MC: BlockPos.withinManhattan(blockpos, k, 0, l) —— 曼哈顿菱形
    for (i32 dx = -k; dx <= k; ++dx) {
        for (i32 dz = -l; dz <= l; ++dz) {
            // withinManhattan 菱形条件：|dx|/k + |dz|/l <= 1（k/l 为 0 时该轴只能取 0）
            const bool inRhombus = (k == 0 ? dx == 0 : std::abs(dx) <= k) && (l == 0 ? dz == 0 : std::abs(dz) <= l) &&
                (k == 0 || l == 0 || std::abs(dx) * l + std::abs(dz) * k <= k * l);
            if (!inRhombus) {
                continue;
            }
            const BlockPos blockpos1(pos.x + dx, pos.y, pos.z + dz);
            // MC: if (blockpos1.distManhattan(blockpos) > i1) break;（菱形内 distManhattan 恒 <= i1，等价跳过）
            if (blockpos1.manhattanDistance(pos) > maxReach) {
                continue;
            }

            if (isClear(world, blockpos1, config)) {
                if (placeRim) {
                    placedAny = true;
                    world.setBlockState(blockpos1, config.rim);
                }
                // MC: blockpos2 = blockpos1.offset(i, 0, j)
                const BlockPos blockpos2(blockpos1.x + rimDx, blockpos1.y, blockpos1.z + rimDz);
                if (isClear(world, blockpos2, config)) {
                    placedAny = true;
                    world.setBlockState(blockpos2, config.contents);
                }
            }
        }
    }

    return placedAny;
}

// ============================================================================
// ConfiguredDeltaFeature 实现
// ============================================================================

ConfiguredDeltaFeature::ConfiguredDeltaFeature(std::unique_ptr<DeltaFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredDeltaFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

} // namespace mc
