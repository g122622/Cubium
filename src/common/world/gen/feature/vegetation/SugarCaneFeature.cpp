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

#include "SugarCaneFeature.hpp"
#include "../../../../core/Constants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"

namespace mc {

// ============================================================================
// SugarCaneFeature 实现
// ============================================================================

bool SugarCaneFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const SugarCaneFeatureConfig& config)
{
    if (!config.state) {
        return false;
    }

    i32 placedCount = 0;

    for (i32 i = 0; i < config.tries; ++i) {
        // 随机偏移
        i32 dx = random.nextInt(config.xzSpread) - random.nextInt(config.xzSpread);
        i32 dz = random.nextInt(config.xzSpread) - random.nextInt(config.xzSpread);

        BlockPos placePos(pos.x + dx, pos.y, pos.z + dz);

        // 向下寻找地面
        // 从世界中间高度向下搜索到最小建筑高度+1（避开最底层）
        constexpr i32 searchStartY = world::MAX_BUILD_HEIGHT / 2;
        constexpr i32 searchEndY = world::MIN_BUILD_HEIGHT + 1;
        for (i32 y = searchStartY; y >= searchEndY; --y) {
            BlockPos checkPos(placePos.x, y, placePos.z);
            const BlockState* state = world.getBlockState(checkPos);

            if (state && !state->isAir()) {
                placePos = BlockPos(placePos.x, y + 1, placePos.z);
                break;
            }
        }

        // 检查是否可以放置
        if (!_canPlaceAt(world, placePos)) {
            continue;
        }

        // 随机高度（1-3）
        i32 height = random.nextInt(config.maxHeight) + 1;

        // 检查是否有足够空间
        bool hasSpace = true;
        for (i32 y = 0; y < height; ++y) {
            BlockPos checkPos(placePos.x, placePos.y + y, placePos.z);
            const BlockState* state = world.getBlockState(checkPos);
            if (state && !state->isAir()) {
                height = y;
                break;
            }
        }

        if (height <= 0 || !hasSpace) {
            continue;
        }

        // 放置甘蔗
        for (i32 y = 0; y < height; ++y) {
            BlockPos canePos(placePos.x, placePos.y + y, placePos.z);
            world.setBlockState(canePos, config.state);
        }

        ++placedCount;
    }

    return placedCount > 0;
}

bool SugarCaneFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查位置是否为空气
    const BlockState* state = world.getBlockState(pos);
    if (state && !state->isAir()) {
        return false;
    }

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    if (!_isValidGround(world, belowPos)) {
        return false;
    }

    // 检查周围是否有水
    return _hasWaterNearby(world, belowPos);
}

bool SugarCaneFeature::_hasWaterNearby(WorldGenRegion& world, const BlockPos& pos) const
{
    // 甘蔗需要周围有水（4个方向相邻）
    static const i32 directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (i32 i = 0; i < 4; ++i) {
        BlockPos waterPos(pos.x + directions[i][0], pos.y, pos.z + directions[i][1]);
        const BlockState* state = world.getBlockState(waterPos);

        if (state && state->blockId() == VanillaBlocks::WATER->blockId()) {
            return true;
        }
    }

    return false;
}

bool SugarCaneFeature::_isValidGround(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    u32 blockId = state->blockId();

    // 甘蔗可以生长在草地、泥土、沙子、红沙、甘蔗上
    return blockId == VanillaBlocks::GRASS_BLOCK->blockId() || blockId == VanillaBlocks::DIRT->blockId() ||
        blockId == VanillaBlocks::SAND->blockId() || blockId == VanillaBlocks::PODZOL->blockId() ||
        blockId == VanillaBlocks::MYCELIUM->blockId() ||
        (VanillaBlocks::SUGAR_CANE && blockId == VanillaBlocks::SUGAR_CANE->blockId());
}

// ============================================================================
// ConfiguredSugarCaneFeature 实现
// ============================================================================

ConfiguredSugarCaneFeature::ConfiguredSugarCaneFeature(
    std::unique_ptr<SugarCaneFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredSugarCaneFeature::place(WorldGenRegion& region,
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
