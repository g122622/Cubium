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

#include "SeagrassFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace mc {

// ============================================================================
// SeagrassFeature 实现
// ============================================================================

bool SeagrassFeature::place(
    WorldGenRegion& world, math::IRandom& random, const BlockPos& pos, const SeagrassFeatureConfig& config)
{
    if (!config.seagrassState) {
        return false;
    }

    // 对齐 MC SeagrassFeature.place：**只尝试一个随机偏移点**。
    //
    // 【曾循环 tries(默认 48) 次】那会让同一配置下的尝试次数是原版的 48 倍，
    // 表现为海草大面积超量（实测 seagrass 5.14x、tall_seagrass 3.62x）。
    // 偏移范围固定为 8 —— 原版写死 nextInt(8) - nextInt(8)，并非可配置字段。
    const i32 offsetX = random.nextInt(8) - random.nextInt(8);
    const i32 offsetZ = random.nextInt(8) - random.nextInt(8);

    // 原版用 OCEAN_FLOOR（而非生成期变体 OCEAN_FLOOR_WG），且取的正是"第一个可用高度"
    // （= 最高固体方块 Y + 1），直接作为放置点 Y。
    // 注意：WorldGenRegion::getHeight 返回的是 getFirstAvailable - 1（最高方块 Y），
    // 比原版 LevelReader.getHeight 少 1，故此处必须用 getHeightmapFirstAvailable。
    const i32 surfaceY = world.getHeightmapFirstAvailable(pos.x + offsetX, pos.z + offsetZ, HeightmapType::OceanFloor);
    const BlockPos placePos(pos.x + offsetX, surfaceY, pos.z + offsetZ);

    if (!_isWater(world, placePos)) {
        return false;
    }

    // 原版此处是 nextDouble() < probability（不是 nextFloat），随机数消耗量不同。
    const bool placeTall = random.nextDouble() < static_cast<f64>(config.tallSeagrassChance);
    // 原版先抽 nextDouble 再判 canSurvive，顺序不可颠倒。
    if (!_canPlaceAt(world, placePos, *config.seagrassState)) {
        return false;
    }

    if (placeTall) {
        const BlockPos abovePos(placePos.x, placePos.y + 1, placePos.z);
        if (_isWater(world, abovePos)) {
            _placeTallSeagrass(world, placePos, config);
        }
        // 注意：原版该分支**即便上方不是水也返回 true**（flag 置位在 if 之外）。
        return true;
    }

    world.setBlockState(placePos, config.seagrassState);
    return true;
}

bool SeagrassFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const BlockState& seagrassState) const
{
    MC_UNUSED(seagrassState);

    if (!_isWater(world, pos)) {
        return false;
    }

    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && belowState->isSolid();
}

bool SeagrassFeature::_isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    // 检查是否为水方块
    if (VanillaBlocks::WATER && state->blockId() == VanillaBlocks::WATER->blockId()) {
        return true;
    }

    return false;
}

bool SeagrassFeature::_placeTallSeagrass(
    WorldGenRegion& world, const BlockPos& pos, const SeagrassFeatureConfig& config) const
{
    // 放置下半部分
    world.setBlockState(pos, config.tallSeagrassLowerState);

    // 放置上半部分
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    world.setBlockState(abovePos, config.tallSeagrassUpperState);

    return true;
}

// ============================================================================
// ConfiguredSeagrassFeature 实现
// ============================================================================

ConfiguredSeagrassFeature::ConfiguredSeagrassFeature(
    std::unique_ptr<SeagrassFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredSeagrassFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::IRandom& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature.place(region, random, pos, *m_config);
}

} // namespace mc
