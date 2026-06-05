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

#include "Placement.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../WorldConstants.hpp"
#include "../../block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// HeightRangePlacementConfig 实现
// ============================================================================

i32 HeightRangePlacementConfig::getRandomY(math::Random& random) const noexcept
{
    // 在 [bottomOffset, maximum - topOffset) 范围内随机选择
    i32 range = maximum - topOffset - bottomOffset;
    if (range <= 0) {
        return bottomOffset;
    }
    return bottomOffset + random.nextInt(0, range - 1);
}

// ============================================================================
// BiomePlacementConfig 实现
// ============================================================================

bool BiomePlacementConfig::isAllowed(u32 biomeId) const noexcept
{
    for (u32 id : allowedBiomes) {
        if (id == biomeId) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// ConfiguredPlacement 实现
// ============================================================================

ConfiguredPlacement::ConfiguredPlacement(std::unique_ptr<Placement> placement, std::unique_ptr<IPlacementConfig> config)
    : m_placement(std::move(placement))
    , m_config(std::move(config))
    , m_next(nullptr)
{}

std::vector<BlockPos> ConfiguredPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const BlockPos& basePos) const
{
    std::vector<BlockPos> positions = m_placement->getPositions(region, random, *m_config, basePos);

    // 链式处理
    if (m_next) {
        std::vector<BlockPos> result;
        for (const BlockPos& pos : positions) {
            auto nextPositions = m_next->getPositions(region, random, pos);
            result.insert(result.end(), nextPositions.begin(), nextPositions.end());
        }
        return result;
    }

    return positions;
}

std::unique_ptr<ConfiguredPlacement> ConfiguredPlacement::then(
    std::unique_ptr<Placement> placement, std::unique_ptr<IPlacementConfig> config) const
{
    auto next = std::make_unique<ConfiguredPlacement>(std::move(placement), std::move(config));

    // 当前实现采用显式链式构建（通过 setNext）
    // 此处返回新节点，供调用方挂接到已有链路
    return next;
}

// ============================================================================
// CountPlacement 实现
// ============================================================================

std::vector<BlockPos> CountPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)region;
    (void)random;
    const auto& countConfig = static_cast<const CountPlacementConfig&>(config);
    std::vector<BlockPos> positions;

    for (i32 i = 0; i < countConfig.count; ++i) {
        positions.push_back(basePos);
    }

    return positions;
}

// ============================================================================
// HeightRangePlacement 实现
// ============================================================================

std::vector<BlockPos> HeightRangePlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)region;
    const auto& heightConfig = static_cast<const HeightRangePlacementConfig&>(config);

    i32 y = heightConfig.getRandomY(random);
    return {BlockPos(basePos.x, y, basePos.z)};
}

// ============================================================================
// SquarePlacement 实现
// ============================================================================

std::vector<BlockPos> SquarePlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)region;
    (void)config;

    // 在XZ平面内随机分散（区块范围内）
    i32 x = basePos.x + random.nextInt(0, world::CHUNK_WIDTH - 1);
    i32 z = basePos.z + random.nextInt(0, world::CHUNK_WIDTH - 1);

    return {BlockPos(x, basePos.y, z)};
}

// ============================================================================
// BiomePlacement 实现
// ============================================================================

std::vector<BlockPos> BiomePlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)random;
    const auto& biomeConfig = static_cast<const BiomePlacementConfig&>(config);

    const BiomeId biomeId = region.getBiome(basePos.x, basePos.y, basePos.z);
    if (!biomeConfig.isAllowed(biomeId)) {
        return {};
    }

    return {basePos};
}

// ============================================================================
// ChancePlacement 实现
// ============================================================================

std::vector<BlockPos> ChancePlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)region;
    const auto& chanceConfig = static_cast<const ChancePlacementConfig&>(config);

    if (random.nextFloat() < chanceConfig.chance) {
        return {basePos};
    }
    return {};
}

// ============================================================================
// SurfacePlacement 实现
// ============================================================================

std::vector<BlockPos> SurfacePlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)random;
    const auto& surfaceConfig = static_cast<const SurfacePlacementConfig&>(config);

    // 从顶部向下搜索第一个固体方块
    constexpr i32 MIN_Y = world::MIN_BUILD_HEIGHT;
    constexpr i32 MAX_Y = world::MAX_BUILD_HEIGHT - 1;

    for (i32 y = MAX_Y; y >= MIN_Y; --y) {
        const BlockState* state = region.getBlockState(basePos.x, y, basePos.z);
        if (state == nullptr) {
            continue;
        }

        // 如果是空气，继续向下
        if (state->isAir()) {
            continue;
        }

        // 找到固体方块，检查是否是水
        if (state->is(VanillaBlocks::WATER)) {
            // 检查水深
            i32 waterDepth = 0;
            for (i32 wy = y; wy >= MIN_Y && waterDepth <= surfaceConfig.maxWaterDepth; --wy) {
                const BlockState* waterState = region.getBlockState(basePos.x, wy, basePos.z);
                if (waterState == nullptr || waterState->isAir()) {
                    break;
                }
                if (waterState->is(VanillaBlocks::WATER)) {
                    waterDepth++;
                } else {
                    // 找到水下的固体方块
                    if (waterDepth <= surfaceConfig.maxWaterDepth) {
                        return {BlockPos(basePos.x, wy, basePos.z)};
                    }
                    break;
                }
            }
            // 水太深，不能放置
            return {};
        }

        // 找到固体地表，返回上方一格的位置（种植位置）
        return {BlockPos(basePos.x, y + 1, basePos.z)};
    }

    // 没找到地表
    return {};
}

} // namespace mc
