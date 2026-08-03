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

#include "FlowerFeature.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include <memory>
#include <utility>

namespace mc {

// ============================================================================
// FlowerFeatureConfig 实现
// ============================================================================

const BlockState* FlowerFeatureConfig::getRandomFlower(math::IRandom& random) const
{
    if (flowers.empty()) {
        return nullptr;
    }
    return flowers[random.nextInt(static_cast<i32>(flowers.size()))];
}

// ============================================================================
// FlowerFeature 实现
// ============================================================================

bool FlowerFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const FlowerFeatureConfig& config)
{
    if (config.flowers.empty()) {
        return false;
    }

    i32 placedCount = 0;
    // RandomPatchFeature 散布算法：j = xzSpread + 1, k = ySpread + 1
    i32 xzRange = config.xzSpread + 1;
    i32 yRange = config.ySpread + 1;

    // 使用世界表面高度作为起始位置
    BlockPos surfacePos(pos.x, world.getHeight(pos.x, pos.z), pos.z);

    for (i32 i = 0; i < config.tries; ++i) {
        // 三角形分布随机偏移
        i32 dx = random.nextInt(xzRange) - random.nextInt(xzRange);
        i32 dy = random.nextInt(yRange) - random.nextInt(yRange);
        i32 dz = random.nextInt(xzRange) - random.nextInt(xzRange);

        BlockPos placePos(surfacePos.x + dx, surfacePos.y + dy, surfacePos.z + dz);

        // 检查是否为空气或可替换
        const BlockState* currentState = world.getBlockState(placePos);
        if (currentState && !currentState->isAir()) {
            if (!config.isReplaceable || !currentState->canBeReplaced()) {
                continue;
            }
        }

        // 获取随机花卉
        const BlockState* flower = config.getRandomFlower(random);
        if (!flower) {
            continue;
        }

        // 检查下方方块是否在白名单中（如果有白名单）或不在黑名单中
        BlockPos groundPos = placePos.down();

        if (!_isValidGround(world, groundPos, config)) {
            continue;
        }

        // 检查是否需要水
        if (config.requiresWater) {
            if (!_hasAdjacentWater(world, groundPos)) {
                continue;
            }
        }

        world.setBlockState(placePos, flower);
        ++placedCount;
    }

    return placedCount > 0;
}

bool FlowerFeature::_isValidGround(WorldGenRegion& world, const BlockPos& pos, const FlowerFeatureConfig& config) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) return false;

    u32 blockId = state->blockId();

    // 检查白名单（如果有）
    if (!config.whitelist.empty()) {
        // 白名单检查：方块ID是否在白名单中
        bool inWhitelist = false;
        for (const auto* whitelistState : config.whitelist) {
            if (whitelistState && whitelistState->blockId() == blockId) {
                inWhitelist = true;
                break;
            }
        }
        if (!inWhitelist) return false;
    }

    // 检查黑名单
    for (const auto* blacklistState : config.blacklist) {
        if (blacklistState && *state == *blacklistState) {
            return false;
        }
    }

    // 花卉可以生长在草方块、泥土、灰化土和砂土上
    return blockId == VanillaBlocks::GRASS_BLOCK->blockId() || blockId == VanillaBlocks::DIRT->blockId() ||
        (VanillaBlocks::PODZOL && blockId == VanillaBlocks::PODZOL->blockId()) ||
        (VanillaBlocks::COARSE_DIRT && blockId == VanillaBlocks::COARSE_DIRT->blockId()) ||
        (VanillaBlocks::FARMLAND && blockId == VanillaBlocks::FARMLAND->blockId());
}

bool FlowerFeature::_hasAdjacentWater(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查四个水平方向是否有水
    constexpr i32 DX[] = {-1, 1, 0, 0};
    constexpr i32 DZ[] = {0, 0, -1, 1};

    for (i32 i = 0; i < 4; ++i) {
        BlockPos waterPos(pos.x + DX[i], pos.y, pos.z + DZ[i]);
        const BlockState* state = world.getBlockState(waterPos);
        if (state && state->blockId() == VanillaBlocks::WATER->blockId()) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// ConfiguredFlowerFeature 实现
// ============================================================================

ConfiguredFlowerFeature::ConfiguredFlowerFeature(std::unique_ptr<FlowerFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredFlowerFeature::place(WorldGenRegion& region,
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
