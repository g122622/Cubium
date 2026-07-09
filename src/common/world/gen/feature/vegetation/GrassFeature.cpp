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

#include "GrassFeature.hpp"

#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

// ============================================================================
// GrassFeatureConfig 实现
// ============================================================================

const BlockState* GrassFeatureConfig::getRandomState(math::Random& random) const
{
    if (states.empty()) {
        return nullptr;
    }
    return states[random.nextInt(static_cast<i32>(states.size()))];
}

// ============================================================================
// GrassFeature 实现
// ============================================================================

bool GrassFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const GrassFeatureConfig& config)
{
    if (config.states.empty()) {
        return false;
    }

    i32 placedCount = 0;
    BlockPos basePos = pos;

    // 如果需要投影到地面，从高度图获取Y坐标
    if (config.project) {
        // 向下寻找第一个非空气方块
        for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT + 1; --y) {
            BlockPos checkPos(pos.x, y, pos.z);
            const BlockState* state = world.getBlockState(checkPos);
            if (state && !state->isAir()) {
                basePos = BlockPos(pos.x, y + 1, pos.z);
                break;
            }
        }
    }

    for (i32 i = 0; i < config.tries; ++i) {
        // 在指定范围内随机选择位置
        i32 dx = random.nextInt(config.xSpread + 1) - random.nextInt(config.xSpread + 1);
        i32 dy = random.nextInt(config.ySpread + 1) - random.nextInt(config.ySpread + 1);
        i32 dz = random.nextInt(config.zSpread + 1) - random.nextInt(config.zSpread + 1);

        BlockPos placePos(basePos.x + dx, basePos.y + dy, basePos.z + dz);

        // 检查是否可以放置
        if (_canPlaceAt(world, placePos, config)) {
            const BlockState* state = config.getRandomState(random);
            if (state) {
                world.setBlockState(placePos, state);
                ++placedCount;
            }
        }
    }

    return placedCount > 0;
}

bool GrassFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const GrassFeatureConfig& config) const
{
    const BlockState* state = world.getBlockState(pos);

    // 检查位置是否为空或可替换
    if (state) {
        if (!state->isAir() && !config.canReplace) {
            return false;
        }
    }

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    if (!_isValidGround(world, belowPos)) {
        return false;
    }

    // 检查是否需要水
    if (config.requiresWater) {
        bool hasWater = false;
        for (i32 dx = -4; dx <= 4; ++dx) {
            for (i32 dz = -4; dz <= 4; ++dz) {
                BlockPos waterPos(belowPos.x + dx, belowPos.y, belowPos.z + dz);
                const BlockState* waterState = world.getBlockState(waterPos);
                if (waterState && waterState->blockId() == VanillaBlocks::WATER->blockId()) {
                    hasWater = true;
                    break;
                }
            }
            if (hasWater) break;
        }
        if (!hasWater) return false;
    }

    return true;
}

bool GrassFeature::_isValidGround(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) return false;

    u32 blockId = state->blockId();

    // 草丛可以生长在草方块、泥土、砂土、灰化土、菌丝上
    return blockId == VanillaBlocks::GRASS_BLOCK->blockId() || blockId == VanillaBlocks::DIRT->blockId() ||
        blockId == VanillaBlocks::COARSE_DIRT->blockId() || blockId == VanillaBlocks::PODZOL->blockId() ||
        blockId == VanillaBlocks::MYCELIUM->blockId() ||
        (VanillaBlocks::FARMLAND && blockId == VanillaBlocks::FARMLAND->blockId());
}

// ============================================================================
// ConfiguredGrassFeature 实现
// ============================================================================

ConfiguredGrassFeature::ConfiguredGrassFeature(std::unique_ptr<GrassFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredGrassFeature::place(WorldGenRegion& region,
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
