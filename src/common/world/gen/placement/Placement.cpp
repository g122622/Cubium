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
#include "../../biome/BiomeClimate.hpp"
#include "../../block/Block.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../noise/PerlinSimplexNoise.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cmath>
#include <limits>

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
// IdentityPlacement 实现
// ============================================================================

std::vector<BlockPos> IdentityPlacement::getPositions(WorldGenRegion& /*region*/,
    math::Random& /*random*/,
    const IPlacementConfig& /*config*/,
    const BlockPos& basePos) const
{
    // 空 placement 链：直接在 origin 处放置。
    return {basePos};
}

// ============================================================================
// CountPlacement 实现
// ============================================================================

std::vector<BlockPos> CountPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)region;

    // 尝试使用 IntProvider 配置（MC 1.21 风格）
    const auto* providerConfig = dynamic_cast<const CountWithProviderConfig*>(&config);
    if (providerConfig && providerConfig->countProvider) {
        i32 count = providerConfig->countProvider->sample(random);
        std::vector<BlockPos> positions;
        positions.reserve(static_cast<size_t>(std::max(0, count)));
        for (i32 i = 0; i < count; ++i) {
            positions.push_back(basePos);
        }
        return positions;
    }

    // 回退到旧式 CountPlacementConfig
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
    // 尝试使用 HeightProvider 配置（MC 1.21 风格）
    const auto* providerConfig = dynamic_cast<const HeightProviderPlacementConfig*>(&config);
    if (providerConfig && providerConfig->heightProvider) {
        i32 y = providerConfig->heightProvider->sample(
            random, world::gen::valueprovider::WorldGenerationContext(world::MIN_BUILD_HEIGHT, world::CHUNK_HEIGHT));
        return {BlockPos(basePos.x, y, basePos.z)};
    }

    // 回退到旧式 HeightRangePlacementConfig
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

    // 尝试使用高度图
    i32 topY = region.getTopBlockY(basePos.x, basePos.z, HeightmapType::WorldSurfaceWG);
    if (topY > world::MIN_BUILD_HEIGHT) {
        // 高度图有效，使用它
        const BlockState* topState = region.getBlockState(basePos.x, topY, basePos.z);
        if (topState && !topState->isAir()) {
            // 检查水深
            if (surfaceConfig.maxWaterDepth >= 0) {
                i32 waterDepth = 0;
                for (i32 wy = topY + 1; wy <= topY + surfaceConfig.maxWaterDepth + 1; ++wy) {
                    const BlockState* waterState = region.getBlockState(basePos.x, wy, basePos.z);
                    if (waterState && waterState->isLiquid()) {
                        ++waterDepth;
                    } else {
                        break;
                    }
                }
                if (waterDepth > surfaceConfig.maxWaterDepth) {
                    return {};
                }
            }
            return {BlockPos(basePos.x, topY + 1, basePos.z)};
        }
    }

    // 回退：从顶部向下搜索
    constexpr i32 MIN_Y = world::MIN_BUILD_HEIGHT;
    constexpr i32 MAX_Y = world::MAX_BUILD_HEIGHT - 1;

    for (i32 y = MAX_Y; y >= MIN_Y; --y) {
        const BlockState* state = region.getBlockState(basePos.x, y, basePos.z);
        if (!state || state->isAir()) {
            continue;
        }

        if (state->is(VanillaBlocks::WATER)) {
            // 检查水深
            i32 waterDepth = 0;
            for (i32 wy = y; wy >= MIN_Y && waterDepth <= surfaceConfig.maxWaterDepth; --wy) {
                const BlockState* waterState = region.getBlockState(basePos.x, wy, basePos.z);
                if (!waterState || waterState->isAir()) {
                    break;
                }
                if (waterState->is(VanillaBlocks::WATER)) {
                    ++waterDepth;
                } else {
                    if (waterDepth <= surfaceConfig.maxWaterDepth) {
                        return {BlockPos(basePos.x, wy, basePos.z)};
                    }
                    break;
                }
            }
            return {};
        }

        return {BlockPos(basePos.x, y + 1, basePos.z)};
    }

    return {};
}

// ============================================================================
// HeightmapPlacement 实现
// ============================================================================

std::vector<BlockPos> HeightmapPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)random;
    const auto& heightmapConfig = static_cast<const HeightmapPlacementConfig&>(config);

    // MC 1.21.11: HeightmapPlacement.getPositions
    //   int k = ctx.getHeight(this.heightmap, x, z);
    //   return k > ctx.getMinY() ? Stream.of(new BlockPos(x, k, z)) : Stream.of();
    // 用 config 指定的高度图类型查列高，k > minY 才返回，无回退。
    const i32 topY = region.getTopBlockY(basePos.x, basePos.z, heightmapConfig.heightmap);
    if (topY <= region.getMinBuildHeight()) {
        return {};
    }
    return {BlockPos(basePos.x, topY, basePos.z)};
}

// ============================================================================
// RarityFilterPlacement 实现
// ============================================================================

std::vector<BlockPos> RarityFilterPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    (void)region;
    const auto& rarityConfig = static_cast<const RarityFilterConfig&>(config);

    // MC 1.21.11: RarityFilter.shouldPlace
    //   return random.nextFloat() < 1.0F / this.chance;
    if (random.nextFloat() < 1.0f / static_cast<f32>(rarityConfig.chance)) {
        return {basePos};
    }
    return {};
}

// ============================================================================
// FixedPlacement 实现
// ============================================================================

std::vector<BlockPos> FixedPlacement::getPositions(
    WorldGenRegion& /*region*/, math::Random& /*random*/, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto& fixedConfig = static_cast<const FixedPlacementConfig&>(config);

    // basePos 所在区块坐标（SectionPos.blockToSectionCoord = floor(x/16) = BlockPos.chunkX）。
    const ChunkCoord chunkX = basePos.chunkX();
    const ChunkCoord chunkZ = basePos.chunkZ();

    std::vector<BlockPos> result;
    for (const BlockPos& pos : fixedConfig.positions) {
        if (pos.chunkX() == chunkX && pos.chunkZ() == chunkZ) {
            result.push_back(pos);
        }
    }
    return result;
}

// ============================================================================
// CountOnEveryLayerPlacement 实现
// ============================================================================

namespace {

/// MC CountOnEveryLayerPlacement.isEmpty: air / water / lava。
bool isEmptyBlock(const BlockState* state)
{
    if (state == nullptr) {
        return true;
    }
    return state->isAir() || state->is(VanillaBlocks::WATER) || state->is(VanillaBlocks::LAVA);
}

/// MC CountOnEveryLayerPlacement.findOnGroundYPosition。
/// 从 startHeightInclusive 向下扫到 minY+1，找"下方非空且当前空且下方非基岩"的层，
/// 返回第 layer 个这样的层的 (下方方块 Y + 1)。找不到返回 INT_MAX。
i32 findOnGroundYPosition(WorldGenRegion& region, i32 x, i32 startHeightInclusive, i32 z, i32 layer)
{
    const i32 minY = region.getMinBuildHeight();
    // MC: blockstate = getBlockState(x, i1, z)；随后循环 j 从 i1 到 minY+1（含），
    //     每次取 blockstate1 = getBlockState(x, j-1, z)，判定 isEmpty(blockstate1) && isEmpty(blockstate) && !bedrock。
    const BlockState* current = region.getBlockState(x, startHeightInclusive, z);

    for (i32 j = startHeightInclusive; j >= minY + 1; --j) {
        const BlockState* below = region.getBlockState(x, j - 1, z);
        if (!isEmptyBlock(below) && isEmptyBlock(current) && !below->is(VanillaBlocks::BEDROCK)) {
            if (layer == 0) {
                return (j - 1) + 1; // 下方方块 Y + 1
            }
            --layer;
        }
        current = below;
    }
    return std::numeric_limits<i32>::max();
}

} // namespace

std::vector<BlockPos> CountOnEveryLayerPlacement::getPositions(
    WorldGenRegion& region, math::Random& random, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto& layerConfig = static_cast<const CountOnEveryLayerConfig&>(config);
    if (!layerConfig.count) {
        return {};
    }

    std::vector<BlockPos> result;
    i32 layer = 0;
    bool foundInLayer;

    // MC: do { flag=false; for j in 0..count.sample(): ...; if found flag=true; i++; } while(flag);
    do {
        foundInLayer = false;
        const i32 sampleCount = layerConfig.count->sample(random);
        for (i32 j = 0; j < sampleCount; ++j) {
            const i32 k = random.nextInt(world::CHUNK_WIDTH) + basePos.x;
            const i32 l = random.nextInt(world::CHUNK_WIDTH) + basePos.z;
            // MC getHeight(MOTION_BLOCKING,k,l) 返回 Y+1；项目 getTopBlockY 返回 Y。
            const i32 heightmapY = region.getTopBlockY(k, l, HeightmapType::MotionBlocking);
            // 空列（getTopBlockY 返回 minY）→ getHeight 原 = minY+1，findOnGround 仍会扫整列。
            const i32 startHeight = (heightmapY + 1);
            const i32 groundY = findOnGroundYPosition(region, k, startHeight, l, layer);
            if (groundY != std::numeric_limits<i32>::max()) {
                result.emplace_back(k, groundY, l);
                foundInLayer = true;
            }
        }
        ++layer;
    } while (foundInLayer);

    return result;
}

// ============================================================================
// NoiseThresholdCountPlacement 实现
// ============================================================================

std::vector<BlockPos> NoiseThresholdCountPlacement::getPositions(
    WorldGenRegion& /*region*/, math::Random& /*random*/, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto& noiseConfig = static_cast<const NoiseThresholdCountConfig&>(config);

    // MC: d0 = BIOME_INFO_NOISE.getValue(x/200.0, z/200.0, false); count = d0 < noiseLevel ? below : above.
    const f64 d0 = world::biome::biomeInfoNoise().getValue(basePos.x / 200.0, basePos.z / 200.0, false);
    const i32 count = d0 < noiseConfig.noiseLevel ? noiseConfig.belowNoise : noiseConfig.aboveNoise;

    std::vector<BlockPos> result;
    result.reserve(static_cast<size_t>(std::max(0, count)));
    for (i32 i = 0; i < count; ++i) {
        result.push_back(basePos);
    }
    return result;
}

// ============================================================================
// NoiseBasedCountPlacement 实现
// ============================================================================

std::vector<BlockPos> NoiseBasedCountPlacement::getPositions(
    WorldGenRegion& /*region*/, math::Random& /*random*/, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto& noiseConfig = static_cast<const NoiseBasedCountConfig&>(config);

    // MC: d0 = BIOME_INFO_NOISE.getValue(x/factor, z/factor, false);
    //     count = ceil((d0 + offset) * ratio).
    const f64 d0 = world::biome::biomeInfoNoise().getValue(
        basePos.x / noiseConfig.noiseFactor, basePos.z / noiseConfig.noiseFactor, false);
    const i32 count =
        static_cast<i32>(std::ceil((d0 + noiseConfig.noiseOffset) * static_cast<f64>(noiseConfig.noiseToCountRatio)));

    std::vector<BlockPos> result;
    const i32 clamped = std::max(0, count);
    result.reserve(static_cast<size_t>(clamped));
    for (i32 i = 0; i < clamped; ++i) {
        result.push_back(basePos);
    }
    return result;
}

// ============================================================================
// SurfaceRelativeThresholdFilterPlacement 实现
// ============================================================================

std::vector<BlockPos> SurfaceRelativeThresholdFilterPlacement::getPositions(
    WorldGenRegion& region, math::Random& /*random*/, const IPlacementConfig& config, const BlockPos& basePos) const
{
    const auto& filterConfig = static_cast<const SurfaceRelativeThresholdFilterConfig&>(config);

    // MC: i = getHeight(heightmap, x, z);  // Y+1 语义
    //     j = i + minInclusive; k = i + maxInclusive; return j <= y && y <= k.
    // 项目 getTopBlockY 返回最高方块 Y（非 Y+1），故基准 = getTopBlockY + 1。
    const i32 heightmapValue = region.getTopBlockY(basePos.x, basePos.z, filterConfig.heightmap) + 1;
    const i64 lo = static_cast<i64>(heightmapValue) + static_cast<i64>(filterConfig.minInclusive);
    const i64 hi = static_cast<i64>(heightmapValue) + static_cast<i64>(filterConfig.maxInclusive);
    const i64 y = static_cast<i64>(basePos.y);

    if (lo <= y && y <= hi) {
        return {basePos};
    }
    return {};
}

} // namespace mc
