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

#include "MagmaPatchFeature.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// MagmaPatchFeature 实现
// ============================================================================

bool MagmaPatchFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const MagmaPatchFeatureConfig& config)
{
    // 检查起始位置是否有效
    if (!_isValidLocation(world, pos)) {
        return false;
    }

    // 获取方块状态
    const BlockState* magma = VanillaBlocks::getState(VanillaBlocks::MAGMA);
    const BlockState* lava = VanillaBlocks::getState(VanillaBlocks::LAVA);
    const BlockState* fire = VanillaBlocks::getState(VanillaBlocks::FIRE);

    if (!magma) {
        return false;
    }

    // 计算深度
    i32 depth = config.minDepth + random.nextInt(config.maxDepth - config.minDepth + 1);

    // 生成圆形岩浆池
    i32 radiusSq = config.radius * config.radius;

    for (i32 dx = -config.radius; dx <= config.radius; ++dx) {
        for (i32 dz = -config.radius; dz <= config.radius; ++dz) {
            i32 distSq = dx * dx + dz * dz;
            if (distSq > radiusSq) {
                continue;
            }

            // 边缘渐变
            f32 edgeFactor = 1.0f - static_cast<f32>(distSq) / static_cast<f32>(radiusSq);
            if (random.nextFloat() > edgeFactor * 0.8f) {
                continue;
            }

            BlockPos placePos(pos.x + dx, pos.y, pos.z + dz);

            // 替换为岩浆块
            if (random.nextFloat() < config.magmaChance) {
                world.setBlockState(placePos, magma);

                // 在岩浆块上生成火焰
                if (fire && random.nextFloat() < config.fireChance) {
                    BlockPos firePos(placePos.x, placePos.y + 1, placePos.z);
                    const BlockState* aboveState = world.getBlockState(firePos);
                    if (!aboveState || aboveState->isAir()) {
                        world.setBlockState(firePos, fire);
                    }
                }
            }

            // 向下挖掘
            for (i32 d = 1; d <= depth; ++d) {
                BlockPos deepPos(placePos.x, placePos.y - d, placePos.z);
                const BlockState* deepState = world.getBlockState(deepPos);
                if (deepState && deepState->is(VanillaBlocks::NETHERRACK)) {
                    // 底部有时有熔岩
                    if (d == depth && lava && random.nextFloat() < 0.3f) {
                        world.setBlockState(deepPos, lava);
                    } else if (random.nextFloat() < config.magmaChance * 0.5f) {
                        world.setBlockState(deepPos, magma);
                    }
                }
            }
        }
    }

    return true;
}

bool MagmaPatchFeature::_isValidLocation(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查当前位置是否为下界岩
    const BlockState* state = world.getBlockState(pos);
    if (!state || !state->is(VanillaBlocks::NETHERRACK)) {
        return false;
    }

    // 检查上方是否有空间
    const BlockState* aboveState = world.getBlockState(pos.up());
    return !aboveState || aboveState->isAir();
}

// ============================================================================
// ConfiguredMagmaPatchFeature 实现
// ============================================================================

ConfiguredMagmaPatchFeature::ConfiguredMagmaPatchFeature(
    std::unique_ptr<MagmaPatchFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredMagmaPatchFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// MagmaPatchFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredMagmaPatchFeature>> MagmaPatchFeatures::s_features;

void MagmaPatchFeatures::initialize()
{
    if (!s_features.empty()) return;

    s_features.push_back(createNormal());
    s_features.push_back(createDense());
}

const std::vector<std::unique_ptr<ConfiguredMagmaPatchFeature>>& MagmaPatchFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredMagmaPatchFeature>> MagmaPatchFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredMagmaPatchFeature> MagmaPatchFeatures::createNormal()
{
    auto config = std::make_unique<MagmaPatchFeatureConfig>(4, // radius
        0.3f,                                                  // magmaChance
        0.1f,                                                  // fireChance
        1,                                                     // minDepth
        3                                                      // maxDepth
    );
    return std::make_unique<ConfiguredMagmaPatchFeature>(std::move(config), "magma_patch");
}

std::unique_ptr<ConfiguredMagmaPatchFeature> MagmaPatchFeatures::createDense()
{
    auto config = std::make_unique<MagmaPatchFeatureConfig>(6, // radius
        0.5f,                                                  // magmaChance
        0.2f,                                                  // fireChance
        2,                                                     // minDepth
        4                                                      // maxDepth
    );
    return std::make_unique<ConfiguredMagmaPatchFeature>(std::move(config), "magma_patch_dense");
}

// ============================================================================
// NetherFireFeature 实现
// ============================================================================

bool NetherFireFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const NetherFireFeatureConfig& config)
{
    const BlockState* fire = VanillaBlocks::getState(VanillaBlocks::FIRE);
    if (!fire) {
        return false;
    }

    bool placed = false;

    // 在范围内随机放置火焰
    for (i32 i = 0; i < config.spread * config.spread; ++i) {
        i32 dx = random.nextInt(config.spread * 2 + 1) - config.spread;
        i32 dz = random.nextInt(config.spread * 2 + 1) - config.spread;

        BlockPos firePos(pos.x + dx, pos.y, pos.z + dz);

        // 检查位置是否有效（在下界岩上）
        const BlockState* belowState = world.getBlockState(firePos.x, firePos.y - 1, firePos.z);
        if (!belowState || !belowState->is(VanillaBlocks::NETHERRACK)) {
            continue;
        }

        // 检查上方是否有空间
        const BlockState* current = world.getBlockState(firePos);
        if (current && !current->isAir()) {
            continue;
        }

        // 放置火焰
        world.setBlockState(firePos, fire);
        placed = true;
    }

    return placed;
}

// ============================================================================
// ConfiguredNetherFireFeature 实现
// ============================================================================

ConfiguredNetherFireFeature::ConfiguredNetherFireFeature(
    std::unique_ptr<NetherFireFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredNetherFireFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// NetherFireFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredNetherFireFeature>> NetherFireFeatures::s_features;

void NetherFireFeatures::initialize()
{
    if (!s_features.empty()) return;
    s_features.push_back(createNormal());
}

const std::vector<std::unique_ptr<ConfiguredNetherFireFeature>>& NetherFireFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredNetherFireFeature>> NetherFireFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredNetherFireFeature> NetherFireFeatures::createNormal()
{
    auto config = std::make_unique<NetherFireFeatureConfig>(4, // spread
        1,                                                     // minHeight
        3                                                      // maxHeight
    );
    return std::make_unique<ConfiguredNetherFireFeature>(std::move(config), "nether_fire");
}

} // namespace mc
