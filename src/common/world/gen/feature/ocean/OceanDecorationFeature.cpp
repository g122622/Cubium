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

#include "OceanDecorationFeature.hpp"

#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

#include <algorithm>

namespace mc {

bool OceanDecorationFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const OceanDecorationFeatureConfig& config)
{
    if (config.tries <= 0) {
        return false;
    }

    bool placedAny = false;
    for (i32 i = 0; i < config.tries; ++i) {
        // 在区块内随机选择位置
        const i32 placeX = pos.x + random.nextInt(world::CHUNK_WIDTH);
        const i32 placeZ = pos.z + random.nextInt(world::CHUNK_WIDTH);
        const i32 oceanFloorY = _findOceanFloorY(world, placeX, placeZ);

        // 检查是否找到有效的海洋底部
        if (oceanFloorY <= world::MIN_BUILD_HEIGHT) {
            continue;
        }

        const BlockPos centerPos(placeX, oceanFloorY + 1, placeZ);
        if (!_isWater(world, centerPos) || !_hasSolidSupport(world, centerPos.down())) {
            continue;
        }

        if (_placeSingleDecoration(world, random, centerPos, config)) {
            placedAny = true;
        }
    }

    return placedAny;
}

bool OceanDecorationFeature::_isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || VanillaBlocks::WATER == nullptr) {
        return false;
    }

    return state->is(VanillaBlocks::WATER);
}

bool OceanDecorationFeature::_hasSolidSupport(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr && !state->isAir() && state->owner().isSolid(*state);
}

i32 OceanDecorationFeature::_findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) const
{
    // 首先尝试使用高度图快速获取海洋底部
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > world::MIN_BUILD_HEIGHT) {
        return oceanFloorY;
    }

    // 回退方案：从上往下扫描查找海洋底部
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

bool OceanDecorationFeature::_placeSingleDecoration(
    WorldGenRegion& world, math::Random& random, const BlockPos& centerPos, const OceanDecorationFeatureConfig& config)
{
    bool placed = false;

    // 在中心位置放置海晶石作为基座
    const BlockPos floorPos = centerPos.down();
    if (config.prismarineState != nullptr) {
        world.setBlockState(floorPos, config.prismarineState);
        placed = true;
    }

    // 放置潮涌核心（如果位置仍在水中）
    if (config.conduitState != nullptr && _isWater(world, centerPos)) {
        world.setBlockState(centerPos, config.conduitState);
        placed = true;
    }

    // 在四周放置海晶石楼梯和台阶
    const auto directions = Directions::horizontal();
    for (Direction direction : directions) {
        const BlockPos ringPos = centerPos.offset(direction);
        if (!_isWater(world, ringPos) || !_hasSolidSupport(world, ringPos.down())) {
            continue;
        }

        if (config.prismarineStairsState != nullptr && random.nextBoolean()) {
            world.setBlockState(ringPos, config.prismarineStairsState);
            placed = true;
            continue;
        }

        if (config.prismarineSlabState != nullptr) {
            world.setBlockState(ringPos, config.prismarineSlabState);
            placed = true;
        }
    }

    // 在周围放置干海带块
    const i32 driedKelpCount = std::max(0, config.driedKelpCount);
    for (i32 i = 0; i < driedKelpCount; ++i) {
        const i32 dx = random.nextInt(5) - 2;
        const i32 dz = random.nextInt(5) - 2;
        const BlockPos kelpPos(centerPos.x + dx, centerPos.y, centerPos.z + dz);

        if (config.driedKelpBlockState == nullptr || !_isWater(world, kelpPos) ||
            !_hasSolidSupport(world, kelpPos.down())) {
            continue;
        }

        world.setBlockState(kelpPos, config.driedKelpBlockState);
        placed = true;
    }

    // 放置海龟蛋巢穴
    if (config.turtleEggState != nullptr) {
        const Direction nestDirection = directions[static_cast<size_t>(random.nextInt(4))];
        const BlockPos nestPos = centerPos.offset(nestDirection, 2);

        if (_hasSolidSupport(world, nestPos.down())) {
            if (config.sandState != nullptr) {
                world.setBlockState(nestPos.down(), config.sandState);
            }

            const i32 eggs = random.nextInt(4) + 1;
            const BlockState* eggState = &config.turtleEggState->with(BlockStateProperties::EGGS_1_4(), eggs);
            world.setBlockState(nestPos, eggState);
            placed = true;
        }
    }

    // 放置气泡柱（岩浆块上方）
    if (config.bubbleColumnState != nullptr && config.magmaState != nullptr) {
        const Direction ventDirection = directions[static_cast<size_t>(random.nextInt(4))];
        const BlockPos ventSourcePos = centerPos.offset(ventDirection, 2).down();
        if (_hasSolidSupport(world, ventSourcePos)) {
            world.setBlockState(ventSourcePos, config.magmaState);

            // 向上生成气泡柱
            const i32 maxColumnHeight = std::max(1, config.bubbleColumnMaxHeight);
            for (i32 yOffset = 1; yOffset <= maxColumnHeight; ++yOffset) {
                const BlockPos bubblePos(ventSourcePos.x, ventSourcePos.y + yOffset, ventSourcePos.z);
                if (!_isWater(world, bubblePos)) {
                    break;
                }

                world.setBlockState(bubblePos, config.bubbleColumnState);
                placed = true;
            }
        }
    }

    return placed;
}

ConfiguredOceanDecorationFeature::ConfiguredOceanDecorationFeature(
    std::unique_ptr<OceanDecorationFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredOceanDecorationFeature::place(WorldGenRegion& region,
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
