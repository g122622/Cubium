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

#include "BasaltFeature.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <cmath>

namespace mc {

namespace {

/// MC 1.21.11: BasaltColumnsFeature.CANNOT_PLACE_ON
/// 这些方块上不能放置玄武岩柱
bool isCannotPlaceOnBlock(const Block& block)
{
    return &block == VanillaBlocks::LAVA || &block == VanillaBlocks::BEDROCK || &block == VanillaBlocks::MAGMA ||
        &block == VanillaBlocks::SOUL_SAND;
}

/// 检查位置是否为空气或熔岩海洋（海平面及以下的熔岩）
bool isAirOrLavaOcean(WorldGenRegion& world, const BlockPos& pos, i32 seaLevel)
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || state->isAir()) {
        return true;
    }
    // 海平面及以下的熔岩视为"海洋"
    if (state->is(VanillaBlocks::LAVA) && pos.y <= seaLevel) {
        return true;
    }
    return false;
}

/// MC 1.21.11: 聚类模式参数
constexpr i32 CLUSTERED_REACH = 5;
constexpr i32 CLUSTERED_SIZE = 50;
constexpr i32 UNCLUSTERED_REACH = 8;
constexpr i32 UNCLUSTERED_SIZE = 15;

} // namespace

// ============================================================================
// BasaltColumnFeature 实现
// ============================================================================

bool BasaltColumnFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BasaltColumnFeatureConfig& config)
{
    // MC 1.21.11: 从配置采样柱高度
    const i32 height = config.minHeight + random.nextInt(config.maxHeight - config.minHeight + 1);
    if (height <= 0) {
        return false;
    }

    // MC 1.21.11: 90% 聚类模式，10% 非聚类模式
    const bool clustered = random.nextFloat() < 0.9f;
    const i32 reach = std::min(height, clustered ? CLUSTERED_REACH : UNCLUSTERED_REACH);
    const i32 size = clustered ? CLUSTERED_SIZE : UNCLUSTERED_SIZE;

    // MC 1.21.11: 生成最多 size 个随机位置
    for (i32 i = 0; i < size; ++i) {
        i32 dx = random.nextInt(reach * 2 + 1) - reach;
        i32 dz = random.nextInt(reach * 2 + 1) - reach;
        BlockPos columnPos(pos.x + dx, pos.y, pos.z + dz);

        // 计算到中心的曼哈顿距离
        i32 manhattanDist = std::abs(dx) + std::abs(dz);
        i32 columnHeight = height - manhattanDist / 2;
        if (columnHeight <= 0) {
            continue;
        }

        _placeColumn(world, columnPos, columnHeight);
    }

    return true;
}

void BasaltColumnFeature::_placeColumn(WorldGenRegion& world, const BlockPos& pos, i32 height)
{
    const BlockState* basalt = VanillaBlocks::getState(VanillaBlocks::BASALT);
    if (!basalt) return;

    const i32 seaLevel = world::SEA_LEVEL; // 下界海平面高度

    // 确定起始放置位置
    BlockPos start = _findSurface(world, pos, seaLevel);

    if (start.y < world::MIN_BUILD_HEIGHT) {
        // 当前位置是实心方块，尝试向上找空气
        start = _findAir(world, pos, seaLevel);
        if (start.y < world::MIN_BUILD_HEIGHT) {
            return;
        }
    }

    // 检查起始位置是否可以放置
    if (!_canPlaceAt(world, start, seaLevel)) {
        return;
    }

    // 向上放置玄武岩柱
    for (i32 y = 0; y < height; ++y) {
        BlockPos placePos(start.x, start.y + y, start.z);
        const BlockState* currentState = world.getBlockState(placePos);

        if (isAirOrLavaOcean(world, placePos, seaLevel)) {
            world.setBlockState(placePos, basalt);
        } else if (currentState && currentState->is(VanillaBlocks::BASALT)) {
            // 已有玄武岩块，跳过但继续向上
            continue;
        } else {
            // 遇到非空气、非熔岩、非玄武岩的方块，停止
            break;
        }
    }
}

bool BasaltColumnFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos, i32 seaLevel) const
{
    // MC 1.21.11: 当前位置必须为空气或熔岩海洋
    if (!isAirOrLavaOcean(world, pos, seaLevel)) {
        return false;
    }

    // MC 1.21.11: 下方方块不能是空气也不能在 CANNOT_PLACE_ON 列表中
    BlockPos below(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(below);
    if (!belowState || belowState->isAir()) {
        return false;
    }
    if (isCannotPlaceOnBlock(belowState->getBlock())) {
        return false;
    }

    return true;
}

BlockPos BasaltColumnFeature::_findSurface(WorldGenRegion& world, const BlockPos& pos, i32 seaLevel) const
{
    // MC 1.21.11: 向下搜索，寻找可以放置玄武岩的表面
    for (i32 y = pos.y; y >= world::MIN_BUILD_HEIGHT; --y) {
        BlockPos checkPos(pos.x, y, pos.z);
        if (!isAirOrLavaOcean(world, checkPos, seaLevel)) {
            // 找到固体方块，在其上方放置
            return BlockPos(pos.x, y + 1, pos.z);
        }
    }
    return BlockPos(pos.x, world::MIN_BUILD_HEIGHT, pos.z);
}

BlockPos BasaltColumnFeature::_findAir(WorldGenRegion& world, const BlockPos& pos, i32 seaLevel) const
{
    // MC 1.21.11: 向上搜索，寻找空气位置
    for (i32 y = pos.y; y < world::MAX_BUILD_HEIGHT; ++y) {
        BlockPos checkPos(pos.x, y, pos.z);
        const BlockState* state = world.getBlockState(checkPos);
        if (state && isCannotPlaceOnBlock(state->getBlock())) {
            // 遇到 CANNOT_PLACE_ON 中的方块，放弃
            return BlockPos(pos.x, world::MIN_BUILD_HEIGHT, pos.z);
        }
        if (isAirOrLavaOcean(world, checkPos, seaLevel)) {
            return checkPos;
        }
    }
    return BlockPos(pos.x, world::MIN_BUILD_HEIGHT, pos.z);
}

// ============================================================================
// ConfiguredBasaltColumnFeature 实现
// ============================================================================

ConfiguredBasaltColumnFeature::ConfiguredBasaltColumnFeature(
    std::unique_ptr<BasaltColumnFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredBasaltColumnFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// BasaltColumnFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>> BasaltColumnFeatures::s_features;

void BasaltColumnFeatures::initialize()
{
    if (!s_features.empty()) return;

    s_features.push_back(createNormal());
    s_features.push_back(createLarge());
}

const std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>>& BasaltColumnFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredBasaltColumnFeature>> BasaltColumnFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredBasaltColumnFeature> BasaltColumnFeatures::createNormal()
{
    auto config = std::make_unique<BasaltColumnFeatureConfig>(0, // minHeight
        5,                                                       // maxHeight
        false);
    return std::make_unique<ConfiguredBasaltColumnFeature>(std::move(config), "basalt_column");
}

std::unique_ptr<ConfiguredBasaltColumnFeature> BasaltColumnFeatures::createLarge()
{
    auto config = std::make_unique<BasaltColumnFeatureConfig>(3, // minHeight
        10,                                                      // maxHeight
        true);
    return std::make_unique<ConfiguredBasaltColumnFeature>(std::move(config), "basalt_column_large");
}

// ============================================================================
// BasaltDeltaFeature 实现
// ============================================================================

bool BasaltDeltaFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BasaltDeltaFeatureConfig& config)
{
    // 获取方块状态
    const BlockState* basalt = VanillaBlocks::getState(VanillaBlocks::BASALT);
    const BlockState* magma = VanillaBlocks::getState(VanillaBlocks::MAGMA);
    const BlockState* netherrack = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);

    if (!basalt || !netherrack) {
        return false;
    }

    // 在区域内生成玄武岩地面
    i32 halfSize = config.size / 2;

    for (i32 dx = -halfSize; dx <= halfSize; ++dx) {
        for (i32 dz = -halfSize; dz <= halfSize; ++dz) {
            // 使用圆形掩码
            f32 distSq = static_cast<f32>(dx * dx + dz * dz);
            f32 radiusSq = static_cast<f32>(halfSize * halfSize);
            if (distSq > radiusSq) {
                continue;
            }

            // 边缘渐变
            f32 edgeFactor = 1.0f - (distSq / radiusSq);
            if (random.nextFloat() > edgeFactor) {
                continue;
            }

            BlockPos placePos(pos.x + dx, pos.y, pos.z + dz);

            // 检查当前位置
            const BlockState* currentState = world.getBlockState(placePos);
            if (!currentState || !currentState->is(VanillaBlocks::NETHERRACK)) {
                continue;
            }

            // 决定放置什么方块
            const BlockState* toPlace = basalt;
            if (magma && random.nextFloat() < config.magmaChance) {
                toPlace = magma;
            }

            world.setBlockState(placePos, toPlace);

            // 有时候向下替换一层
            if (random.nextFloat() < 0.3f) {
                BlockPos belowPos(placePos.x, placePos.y - 1, placePos.z);
                const BlockState* belowState = world.getBlockState(belowPos);
                if (belowState && belowState->is(VanillaBlocks::NETHERRACK)) {
                    world.setBlockState(belowPos, toPlace);
                }
            }
        }
    }

    return true;
}

// ============================================================================
// ConfiguredBasaltDeltaFeature 实现
// ============================================================================

ConfiguredBasaltDeltaFeature::ConfiguredBasaltDeltaFeature(
    std::unique_ptr<BasaltDeltaFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredBasaltDeltaFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// BasaltDeltaFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>> BasaltDeltaFeatures::s_features;

void BasaltDeltaFeatures::initialize()
{
    if (!s_features.empty()) return;
    s_features.push_back(createNormal());
}

const std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>>& BasaltDeltaFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>> BasaltDeltaFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredBasaltDeltaFeature> BasaltDeltaFeatures::createNormal()
{
    auto config = std::make_unique<BasaltDeltaFeatureConfig>(8, // size
        0.2f,                                                   // magmaChance
        true                                                    // useBasalt
    );
    return std::make_unique<ConfiguredBasaltDeltaFeature>(std::move(config), "basalt_delta");
}

} // namespace mc
