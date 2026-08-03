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

#include "KelpFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace mc {

namespace {

// 获取海底高度，如果无法找到则返回 -1。
[[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z)
{
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    // 某些测试场景会直接写方块而不更新高度图，回退到显式扫描。
    // 正常情况下不会走到下面的代码路径。
    for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT + 1; --y) {
        const BlockState* state = world.getBlockState(x, y, z);
        if (state == nullptr || state->isAir()) {
            continue;
        }

        if (VanillaBlocks::WATER != nullptr && state->is(VanillaBlocks::WATER)) {
            continue;
        }

        return y;
    }

    return -1;
}

} // namespace

// ============================================================================
// KelpFeature 实现
// ============================================================================

bool KelpFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const KelpFeatureConfig& config)
{
    if (!config.kelpState || !config.kelpTopState) {
        return false;
    }

    // 单次放置会尝试多次随机点，并在顶端设置 AGE 20-23。
    const i32 tries = std::max(1, config.tries);
    const i32 maxHeight = std::max(1, config.maxHeight);

    bool placedAny = false;
    for (i32 attempt = 0; attempt < tries; ++attempt) {
        const i32 placeX = pos.x + random.nextInt(world::CHUNK_WIDTH);
        const i32 placeZ = pos.z + random.nextInt(world::CHUNK_WIDTH);
        const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
        if (oceanFloorY <= 0) {
            continue;
        }

        BlockPos currentPos(placeX, oceanFloorY + 1, placeZ);
        if (!_isWater(world, currentPos)) {
            continue;
        }

        // 从海床平面开始向上生长海带，直到达到随机高度或无法继续生长。
        const i32 height = 1 + random.nextInt(maxHeight);
        for (i32 y = 0; y <= height; ++y) {
            const BlockPos abovePos = currentPos.up();
            const bool canGrowHere =
                _isWater(world, currentPos) && _isWater(world, abovePos) && _canPlaceAt(world, currentPos);

            if (canGrowHere) {
                if (y == height) {
                    // 顶端海带设置 AGE 属性，值为 20-23
                    const i32 age = random.nextInt(4) + 20;
                    world.setBlockState(currentPos, &config.kelpTopState->with(BlockStateProperties::AGE_0_25(), age));
                    placedAny = true;
                } else {
                    world.setBlockState(currentPos, config.kelpState);
                }
            } else if (y > 0) {
                const BlockPos belowPos = currentPos.down();
                const BlockPos belowBelowPos = belowPos.down();
                const BlockState* belowBelowState = world.getBlockState(belowBelowPos);

                const bool belowHasKelp = (VanillaBlocks::KELP != nullptr && belowBelowState != nullptr &&
                    belowBelowState->is(VanillaBlocks::KELP));

                if (_canPlaceAt(world, belowPos) && !belowHasKelp) {
                    // 顶端海带设置 AGE 属性，值为 20-23
                    const i32 age = random.nextInt(4) + 20;
                    world.setBlockState(belowPos, &config.kelpTopState->with(BlockStateProperties::AGE_0_25(), age));
                    placedAny = true;
                }
                break;
            }

            currentPos = currentPos.up();
        }
    }

    return placedAny;
}

bool KelpFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* belowState = world.getBlockState(pos.down());

    MC_ASSERT_RELEASE(belowState);

    if (VanillaBlocks::KELP != nullptr && belowState->is(VanillaBlocks::KELP)) {
        return true;
    }
    if (VanillaBlocks::KELP_PLANT != nullptr && belowState->is(VanillaBlocks::KELP_PLANT)) {
        return true;
    }

    return belowState->owner().isSolid(*belowState);
}

bool KelpFeature::_isWater(WorldGenRegion& world, const BlockPos& pos) const
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

// ============================================================================
// ConfiguredKelpFeature 实现
// ============================================================================

ConfiguredKelpFeature::ConfiguredKelpFeature(std::unique_ptr<KelpFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredKelpFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature.place(region, random, pos, *m_config);
}

} // namespace mc
