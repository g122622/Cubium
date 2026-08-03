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
 */

#include "RootSystemFeature.hpp"
#include "CaveSurface.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include <memory>
#include <utility>

namespace mc::world::gen::feature::cave {

// ============================================================================
// RootSystemFeature
// ============================================================================

bool RootSystemFeature::spaceForTree(WorldGenRegion& region, const BlockPos& pos, i32 requiredSpace, i32 allowedWater)
{
    i32 waterCount = 0;
    for (i32 i = 1; i <= requiredSpace; ++i) {
        BlockPos checkPos = pos.offset(Direction::Up, i);
        const BlockState* state = region.getBlockState(checkPos);
        if (state == nullptr) {
            return false;
        }
        if (state->isAir()) {
            continue;
        }
        // 检查是否为水
        if (state->getMaterial().isLiquid()) {
            waterCount++;
            if (waterCount > allowedWater) {
                return false;
            }
            continue;
        }
        // 非空气非水，空间不足
        return false;
    }
    return true;
}

void RootSystemFeature::placeRootedDirtColumn(
    WorldGenRegion& region, math::Random& random, const BlockPos& origin, i32 targetY, const RootSystemConfig& config)
{
    MC_UNUSED(random);

    // 从origin向上填充到targetY
    for (i32 y = origin.y; y <= targetY; ++y) {
        BlockPos dirtPos(origin.x, y, origin.z);
        const BlockState* existing = region.getBlockState(dirtPos);
        if (existing != nullptr && matchesTag(*existing, config.rootReplaceableTag)) {
            region.setBlockState(dirtPos, config.rootState, 3);
        }
    }

    // 在rootRadius范围内随机放置缠根泥土
    for (i32 attempt = 0; attempt < config.rootPlacementAttempts; ++attempt) {
        i32 dx = random.nextInt(config.rootRadius * 2 + 1) - config.rootRadius;
        i32 dz = random.nextInt(config.rootRadius * 2 + 1) - config.rootRadius;
        BlockPos rootPos(origin.x + dx, random.nextInt(targetY - origin.y + 1) + origin.y, origin.z + dz);

        const BlockState* existing = region.getBlockState(rootPos);
        if (existing != nullptr && matchesTag(*existing, config.rootReplaceableTag)) {
            region.setBlockState(rootPos, config.rootState, 3);
        }
    }
}

void RootSystemFeature::placeHangingRoots(
    WorldGenRegion& region, math::Random& random, const BlockPos& rootCenter, const RootSystemConfig& config)
{
    if (config.hangingRootState == nullptr) {
        return;
    }

    for (i32 attempt = 0; attempt < config.hangingRootPlacementAttempts; ++attempt) {
        i32 dx = random.nextInt(config.hangingRootRadius * 2 + 1) - config.hangingRootRadius;
        i32 dy = -random.nextInt(config.hangingRootsVerticalSpan + 1);
        i32 dz = random.nextInt(config.hangingRootRadius * 2 + 1) - config.hangingRootRadius;

        BlockPos rootPos(rootCenter.x + dx, rootCenter.y + dy, rootCenter.z + dz);
        const BlockState* existing = region.getBlockState(rootPos);

        // 只在空气位置放置垂根
        if (existing == nullptr || !existing->isAir()) {
            continue;
        }

        // 检查上方是否有坚固面
        BlockPos abovePos = rootPos.offset(Direction::Up);
        const BlockState* aboveState = region.getBlockState(abovePos);
        if (aboveState != nullptr && aboveState->isSolid()) {
            region.setBlockState(rootPos, config.hangingRootState, 3);
        }
    }
}

bool RootSystemFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos,
    const RootSystemConfig& config)
{
    // 检查起始位置是否为空气
    const BlockState* startState = region.getBlockState(pos);
    if (startState != nullptr && !startState->isAir()) {
        return false;
    }

    // 向上搜索有效树木位置
    for (i32 i = 0; i < config.rootColumnMaxHeight; ++i) {
        BlockPos treePos = pos.offset(Direction::Up, i + 1);

        // 检查是否为空气
        const BlockState* treeState = region.getBlockState(treePos);
        if (treeState == nullptr || !treeState->isAir()) {
            continue;
        }

        // 检查树木是否有足够空间
        if (!spaceForTree(region, treePos, config.requiredVerticalSpaceForTree, config.allowedVerticalWaterForTree)) {
            continue;
        }

        // 检查下方是否有实心支撑
        BlockPos belowTreePos = treePos.offset(Direction::Down);
        const BlockState* belowState = region.getBlockState(belowTreePos);
        if (belowState == nullptr || belowState->isAir()) {
            continue;
        }

        // 检查下方不是熔岩
        if (belowState->getMaterial().isLiquid()) {
            continue;
        }

        // 尝试放置树木
        const ConfiguredFeatureBase* treeFeature = ConfiguredFeatureRegistry::instance().get(config.treeFeatureId);

        if (treeFeature != nullptr) {
            bool treePlaced = treeFeature->place(region, chunk, generator, random, treePos);
            if (treePlaced) {
                // 放置缠根泥土柱
                placeRootedDirtColumn(region, random, pos, treePos.y, config);

                // 放置垂根
                placeHangingRoots(region, random, pos, config);

                return true;
            }
        }
    }

    return false;
}

// ============================================================================
// ConfiguredRootSystemFeature
// ============================================================================

ConfiguredRootSystemFeature::ConfiguredRootSystemFeature(
    std::unique_ptr<RootSystemConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredRootSystemFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return RootSystemFeature::place(region, chunk, generator, random, pos, *m_config);
}

} // namespace mc::world::gen::feature::cave
