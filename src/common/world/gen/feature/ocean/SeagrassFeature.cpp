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
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../WorldConstants.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../chunk/IChunkGenerator.hpp"

#include <algorithm>

namespace mc {

namespace {

[[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z)
{
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    // 某些测试会绕过高度图更新，回退到显式扫描。
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

std::unique_ptr<ConfiguredSeagrassFeature> createSeagrassFeature(
    const char* featureName, f32 tallChance, i32 tries, i32 spread)
{
    auto config = std::make_unique<SeagrassFeatureConfig>();
    if (VanillaBlocks::SEAGRASS != nullptr) {
        config->seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    }
    if (VanillaBlocks::TALL_SEAGRASS != nullptr) {
        config->tallSeagrassLowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
            BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
        config->tallSeagrassUpperState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
            BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    }

    config->tallSeagrassChance = tallChance;
    config->tries = tries;
    config->horizontalSpread = spread;

    return std::make_unique<ConfiguredSeagrassFeature>(std::move(config), featureName);
}

} // namespace

// ============================================================================
// SeagrassFeature 实现
// ============================================================================

bool SeagrassFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const SeagrassFeatureConfig& config)
{
    if (!config.seagrassState) {
        return false;
    }

    // 参考原版 SeaGrassFeature：每次调用尝试多个随机偏移点。
    const i32 tries = std::max(1, config.tries);
    const i32 spread = std::max(1, config.horizontalSpread);

    bool placedAny = false;
    for (i32 attempt = 0; attempt < tries; ++attempt) {
        const i32 dx = random.nextInt(spread) - random.nextInt(spread);
        const i32 dz = random.nextInt(spread) - random.nextInt(spread);

        const i32 placeX = pos.x + dx;
        const i32 placeZ = pos.z + dz;
        const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
        if (oceanFloorY <= 0) {
            continue;
        }

        const BlockPos placePos(placeX, oceanFloorY + 1, placeZ);
        if (!canPlaceAt(world, placePos, *config.seagrassState)) {
            continue;
        }

        const bool shouldPlaceTall = config.tallSeagrassChance > 0.0f && config.tallSeagrassLowerState != nullptr &&
            config.tallSeagrassUpperState != nullptr && random.nextFloat() < config.tallSeagrassChance;

        if (shouldPlaceTall) {
            const BlockPos abovePos(placePos.x, placePos.y + 1, placePos.z);
            if (isWater(world, abovePos)) {
                placeTallSeagrass(world, placePos, config);
                placedAny = true;
                continue;
            }
        }

        world.setBlockState(placePos, config.seagrassState);
        placedAny = true;
    }

    return placedAny;
}

bool SeagrassFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const BlockState& seagrassState) const
{
    MC_UNUSED(seagrassState);

    if (!isWater(world, pos)) {
        return false;
    }

    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && belowState->isSolid();
}

bool SeagrassFeature::isWater(WorldGenRegion& world, const BlockPos& pos) const
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

bool SeagrassFeature::placeTallSeagrass(
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

bool ConfiguredSeagrassFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// SeagrassFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredSeagrassFeature>> SeagrassFeatures::s_features;

void SeagrassFeatures::initialize()
{
    s_features.clear();
    s_features.push_back(createSimpleSeagrass());
    s_features.push_back(createMixedSeagrass());
    s_features.push_back(createColdSeagrass());
    s_features.push_back(createDeepColdSeagrass());
    s_features.push_back(createNormalSeagrass());
    s_features.push_back(createRiverSeagrass());
    s_features.push_back(createDeepSeagrass());
    s_features.push_back(createSwampSeagrass());
    s_features.push_back(createWarmSeagrass());
    s_features.push_back(createDeepWarmSeagrass());
}

const std::vector<std::unique_ptr<ConfiguredSeagrassFeature>>& SeagrassFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredSeagrassFeature>> SeagrassFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createSimpleSeagrass()
{
    return createSeagrassFeature("seagrass_simple", 0.0f, 64, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createMixedSeagrass()
{
    return createSeagrassFeature("seagrass_mixed", 0.3f, 48, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createColdSeagrass()
{
    return createSeagrassFeature("seagrass_cold", 0.3f, 32, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createDeepColdSeagrass()
{
    return createSeagrassFeature("seagrass_deep_cold", 0.8f, 40, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createNormalSeagrass()
{
    return createSeagrassFeature("seagrass_normal", 0.3f, 48, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createRiverSeagrass()
{
    return createSeagrassFeature("seagrass_river", 0.4f, 48, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createDeepSeagrass()
{
    return createSeagrassFeature("seagrass_deep", 0.8f, 48, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createSwampSeagrass()
{
    return createSeagrassFeature("seagrass_swamp", 0.6f, 64, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createWarmSeagrass()
{
    return createSeagrassFeature("seagrass_warm", 0.3f, 80, 8);
}

std::unique_ptr<ConfiguredSeagrassFeature> SeagrassFeatures::createDeepWarmSeagrass()
{
    return createSeagrassFeature("seagrass_deep_warm", 0.8f, 80, 8);
}

} // namespace mc
