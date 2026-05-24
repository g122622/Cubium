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

#include "EndSpikeFeature.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/util/math/MathConstants.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace mc {

namespace {

[[nodiscard]] bool intersectsWorldGenRegion(
    const EndSpike& spike, const WorldGenRegion& world, i32 centerX, i32 centerZ)
{
    const i32 minX = world.minChunkX() * 16;
    const i32 maxX = (world.maxChunkX() + 1) * 16 - 1;
    const i32 minZ = world.minChunkZ() * 16;
    const i32 maxZ = (world.maxChunkZ() + 1) * 16 - 1;

    if (centerX + spike.radius < minX || centerX - spike.radius > maxX) {
        return false;
    }

    if (centerZ + spike.radius < minZ || centerZ - spike.radius > maxZ) {
        return false;
    }

    return true;
}

} // namespace

// ============================================================================
// EndSpikeFeatureConfig 实现
// ============================================================================

std::vector<EndSpike> EndSpikeFeatureConfig::generateSpikes(u64 worldSeed)
{
    std::vector<EndSpike> spikes;

    // 参考 MC 1.16.5: SpikeFeature.EndSpikeCacheLoader.load()
    // 10根柱子，使用种子随机打乱高度/半径索引

    math::Random rng(worldSeed);
    // MC: long i = random.nextLong() & 65535L;
    u64 cacheKey = rng.nextLong() & 65535ULL;
    math::Random shuffleRng(static_cast<u64>(cacheKey));

    // 创建索引 0-9 并打乱
    std::vector<i32> indices = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::shuffle(indices.begin(), indices.end(), std::mt19937(static_cast<u32>(cacheKey)));

    // 生成10根柱子
    for (i32 i = 0; i < 10; ++i) {
        // MC 1.16.5: angle = 2 * (-PI + (PI/10) * i)
        f64 angle = 2.0 * (-mc::math::PI_DOUBLE + (mc::math::PI_DOUBLE / 10.0) * static_cast<f64>(i));

        // MC 1.16.5: 使用半径 42 (不是 43)
        i32 x = static_cast<i32>(std::floor(42.0 * std::cos(angle)));
        i32 z = static_cast<i32>(std::floor(42.0 * std::sin(angle)));

        i32 idx = indices[i];
        // MC 1.16.5: radius = 2 + l / 3 (l 是打乱后的索引 0-9)
        i32 radius = 2 + idx / 3;
        // MC 1.16.5: height = 76 + l * 3
        i32 height = 76 + idx * 3;
        // MC 1.16.5: guarded = l == 1 || l == 2
        bool guarded = (idx == 1 || idx == 2);

        spikes.emplace_back(x, z, radius, height, guarded);
    }

    return spikes;
}

// ============================================================================
// EndSpikeFeature 实现
// ============================================================================

bool EndSpikeFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const EndSpikeFeatureConfig& config)
{
    // 获取黑曜石柱列表
    const std::vector<EndSpike>& spikes = config.spikes;

    // 如果配置为销毁模式，先销毁所有柱子
    if (config.destroying) {
        const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
        for (const auto& spike : spikes) {
            if (!intersectsWorldGenRegion(spike, world, pos.x + spike.centerX, pos.z + spike.centerZ)) {
                continue;
            }

            // 销毁柱子区域的所有方块
            for (i32 y = 0; y < spike.height; ++y) {
                for (i32 x = -spike.radius; x <= spike.radius; ++x) {
                    for (i32 z = -spike.radius; z <= spike.radius; ++z) {
                        // 圆形截面
                        if (x * x + z * z <= spike.radius * spike.radius) {
                            world.setBlockState(pos.x + spike.centerX + x, y, pos.z + spike.centerZ + z, air);
                        }
                    }
                }
            }
        }
        return true;
    }

    // 生成每根柱子
    for (const auto& spike : spikes) {
        if (!intersectsWorldGenRegion(spike, world, spike.centerX, spike.centerZ)) {
            continue;
        }

        generateSpike(world, random, spike);
    }

    return true;
}

bool EndSpikeFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查位置是否在末地石上
    const BlockState* state = world.getBlockState(pos.x, pos.y - 1, pos.z);
    return state && &state->getBlock() == VanillaBlocks::END_STONE;
}

void EndSpikeFeature::generateSpike(WorldGenRegion& world, math::Random& random, const EndSpike& spike)
{
    (void)random;

    const BlockState* obsidian = VanillaBlocks::getState(VanillaBlocks::OBSIDIAN);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    // 柱子中心坐标
    i32 baseX = spike.centerX;
    i32 baseZ = spike.centerZ;
    i32 radius = spike.radius;
    i32 height = spike.height;

    // MC 1.16.5 EndSpikeFeature.placeSpike():
    // 遍历整个柱子区域（包括上方10格）
    for (i32 x = -radius; x <= radius; ++x) {
        for (i32 z = -radius; z <= radius; ++z) {
            for (i32 y = 0; y <= height + 10; ++y) {
                // 计算到中心的距离平方
                f64 distSq = static_cast<f64>(x * x + z * z);

                // MC: blockpos.distanceSq(centerX, y, centerZ, false) <= radius*radius + 1
                if (distSq <= static_cast<f64>(radius * radius + 1) && y < height) {
                    // 在柱子范围内且低于高度：放置黑曜石
                    world.setBlockState(baseX + x, y, baseZ + z, obsidian);
                } else if (y > 65) {
                    // Y > 65 的区域清除为空气
                    world.setBlockState(baseX + x, y, baseZ + z, air);
                }
            }
        }
    }

    // 如果需要笼子，生成铁栏杆
    if (spike.guarded) {
        BlockPos topPos(baseX, height, baseZ);
        generateCage(world, topPos, radius);
    }
}

void EndSpikeFeature::generateCage(WorldGenRegion& world, const BlockPos& topPos, i32 radius)
{
    // MC 1.16.5: 使用铁栏杆作为笼子材料
    const BlockState* cageBlock = VanillaBlocks::getState(VanillaBlocks::IRON_BARS);

    if (!cageBlock) {
        return;
    }

    // MC 1.16.5 EndSpikeFeature.placeSpike(): 生成铁栏杆笼子
    // 循环范围: k, l 从 -2 到 2, i1 从 0 到 3
    for (i32 k = -2; k <= 2; ++k) {
        for (i32 l = -2; l <= 2; ++l) {
            for (i32 y = 0; y <= 3; ++y) {
                bool isOuterK = std::abs(k) == 2;
                bool isOuterL = std::abs(l) == 2;
                bool isTop = (y == 3);

                // MC: if (flag || flag1 || flag2)
                if (isOuterK || isOuterL || isTop) {
                    // 计算铁栏杆连接方向
                    // MC: flag3 = k == -2 || k == 2 || flag2 (north/south)
                    // MC: flag4 = l == -2 || l == 2 || flag2 (west/east)
                    bool connectNS = isOuterK || isTop;
                    bool connectWE = isOuterL || isTop;

                    // 设置方向属性
                    const BlockState* barState = cageBlock;
                    barState = &barState->with(BlockStateProperties::NORTH(), connectNS && l != -2);
                    barState = &barState->with(BlockStateProperties::SOUTH(), connectNS && l != 2);
                    barState = &barState->with(BlockStateProperties::WEST(), connectWE && k != -2);
                    barState = &barState->with(BlockStateProperties::EAST(), connectWE && k != 2);

                    world.setBlockState(topPos.x + k, topPos.y + y, topPos.z + l, barState);
                }
            }
        }
    }

    // 在柱子顶部放置基岩作为水晶底座
    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    if (bedrock) {
        world.setBlockState(topPos.x, topPos.y, topPos.z, bedrock);
    }
}

// ============================================================================
// ConfiguredEndSpikeFeature 实现
// ============================================================================

ConfiguredEndSpikeFeature::ConfiguredEndSpikeFeature(
    std::unique_ptr<EndSpikeFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredEndSpikeFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    MC_UNUSED(chunk);

    EndSpikeFeatureConfig runtimeConfig = *m_config;
    if (!runtimeConfig.destroying) {
        runtimeConfig.spikes = EndSpikeFeatureConfig::generateSpikes(generator.seed());
    }

    return m_feature.place(region, random, pos, runtimeConfig);
}

// ============================================================================
// EndSpikeFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>> EndSpikeFeatures::s_features;

void EndSpikeFeatures::initialize()
{
    if (!s_features.empty()) return;

    // 创建标准黑曜石柱配置（使用默认种子）
    // 实际使用时应该传入世界种子
    s_features.push_back(createStandard(0));
}

const std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>>& EndSpikeFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>> EndSpikeFeatures::getAllFeaturesAndClear()
{
    auto extracted = std::move(s_features);
    s_features.clear();
    return extracted;
}

std::unique_ptr<ConfiguredEndSpikeFeature> EndSpikeFeatures::createStandard(u64 worldSeed)
{
    auto config = std::make_unique<EndSpikeFeatureConfig>(EndSpikeFeatureConfig::generateSpikes(worldSeed), false);
    return std::make_unique<ConfiguredEndSpikeFeature>(std::move(config), "end_spike");
}

} // namespace mc
