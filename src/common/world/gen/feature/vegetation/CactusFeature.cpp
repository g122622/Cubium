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

#include "CactusFeature.hpp"
#include "../../../../core/Constants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"

namespace mc {

// ============================================================================
// CactusFeature 实现
// ============================================================================

bool CactusFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CactusFeatureConfig& config)
{
    if (!config.state) {
        return false;
    }

    // 寻找有效的放置位置（向下找到地面）
    BlockPos placePos = pos;
    for (i32 y = pos.y; y >= world::MIN_BUILD_HEIGHT + 1; --y) {
        BlockPos checkPos(pos.x, y, pos.z);
        const BlockState* state = world.getBlockState(checkPos);

        if (state && !state->isAir()) {
            placePos = BlockPos(pos.x, y + 1, pos.z);
            break;
        }
    }

    // 检查是否可以放置
    if (!_canPlaceAt(world, placePos)) {
        return false;
    }

    // 随机高度
    i32 height = random.nextInt(config.maxHeight) + 1;

    // 检查是否有足够空间
    for (i32 y = 0; y < height; ++y) {
        BlockPos checkPos(placePos.x, placePos.y + y, placePos.z);
        const BlockState* state = world.getBlockState(checkPos);
        if (state && !state->isAir()) {
            height = y;
            break;
        }

        // 检查周围空间
        if (!_hasValidSpace(world, checkPos)) {
            height = y;
            break;
        }
    }

    if (height <= 0) {
        return false;
    }

    // 放置仙人掌
    for (i32 y = 0; y < height; ++y) {
        BlockPos cactusPos(placePos.x, placePos.y + y, placePos.z);
        world.setBlockState(cactusPos, config.state);
    }

    return true;
}

bool CactusFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
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

    // 检查周围空间
    return _hasValidSpace(world, pos);
}

bool CactusFeature::_hasValidSpace(WorldGenRegion& world, const BlockPos& pos) const
{
    // 仙人掌需要周围4个方向都是空气或水
    static const i32 directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (i32 i = 0; i < 4; ++i) {
        BlockPos neighborPos(pos.x + directions[i][0], pos.y, pos.z + directions[i][1]);
        const BlockState* neighborState = world.getBlockState(neighborPos);

        if (neighborState && !neighborState->isAir()) {
            // 仙人掌旁边可以有水
            if (VanillaBlocks::WATER && neighborState->blockId() == VanillaBlocks::WATER->blockId()) {
                continue;
            }
            return false;
        }
    }

    return true;
}

bool CactusFeature::_isValidGround(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    u32 blockId = state->blockId();

    // 仙人掌可以生长在沙子、红沙、仙人掌上
    if (blockId == VanillaBlocks::SAND->blockId()) {
        return true;
    }

    // 检查是否是仙人掌本身（用于叠加生长）
    if (VanillaBlocks::CACTUS && blockId == VanillaBlocks::CACTUS->blockId()) {
        return true;
    }

    // 红沙支持
    if (VanillaBlocks::RED_SAND && blockId == VanillaBlocks::RED_SAND->blockId()) {
        return true;
    }

    return false;
}

// ============================================================================
// ConfiguredCactusFeature 实现
// ============================================================================

ConfiguredCactusFeature::ConfiguredCactusFeature(std::unique_ptr<CactusFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredCactusFeature::place(WorldGenRegion& region,
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
