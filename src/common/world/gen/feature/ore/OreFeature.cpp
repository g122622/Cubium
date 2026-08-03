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

#include "OreFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// OreFeature 实现
// ============================================================================

bool OreFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    math::Random& random,
    const BlockPos& origin,
    const OreFeatureConfig& config)
{
    // 生成椭圆形矿脉
    using namespace mc::world;

    // 计算矿脉参数
    f32 angle = random.nextFloat() * math::PI;
    f32 sizeFactor = static_cast<f32>(config.size) / 8.0f;
    i32 halfSize = static_cast<i32>(std::ceil((static_cast<f32>(config.size) / 16.0f * 2.0f + 1.0f) / 2.0f));

    // 计算矿脉两端位置
    f32 x1 = static_cast<f32>(origin.x) + std::sin(static_cast<f32>(angle)) * static_cast<f32>(sizeFactor);
    f32 x2 = static_cast<f32>(origin.x) - std::sin(static_cast<f32>(angle)) * static_cast<f32>(sizeFactor);
    f32 z1 = static_cast<f32>(origin.z) + std::cos(static_cast<f32>(angle)) * static_cast<f32>(sizeFactor);
    f32 z2 = static_cast<f32>(origin.z) - std::cos(static_cast<f32>(angle)) * static_cast<f32>(sizeFactor);

    f32 y1 = static_cast<f32>(origin.y + random.nextInt(-2, 2));
    f32 y2 = static_cast<f32>(origin.y + random.nextInt(-2, 2));

    // 计算边界框
    i32 minX = origin.x - static_cast<i32>(std::ceil(sizeFactor)) - halfSize;
    i32 minY = origin.y - 2 - halfSize;
    i32 minZ = origin.z - static_cast<i32>(std::ceil(sizeFactor)) - halfSize;
    i32 sizeX = 2 * (static_cast<i32>(std::ceil(sizeFactor)) + halfSize);
    i32 sizeY = 2 * (2 + halfSize);
    i32 sizeZ = sizeX;

    // 检查是否在有效范围内
    for (i32 checkX = minX; checkX <= minX + sizeX; ++checkX) {
        for (i32 checkZ = minZ; checkZ <= minZ + sizeZ; ++checkZ) {
            i32 topY = region.getTopBlockY(checkX, checkZ, HeightmapType::WorldSurfaceWG);
            if (minY <= topY) {
                i32 placedCount = 0;
                _generateSphere(
                    region, random, config, x1, y1, z1, x2, y2, z2, minX, minY, minZ, sizeX, sizeY, sizeZ, placedCount);
                return placedCount > 0;
            }
        }
    }

    return false;
}

void OreFeature::_generateSphere(WorldGenRegion& region,
    math::Random& random,
    const OreFeatureConfig& config,
    f32 x1,
    f32 y1,
    f32 z1,
    f32 x2,
    f32 y2,
    f32 z2,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 sizeX,
    i32 sizeY,
    i32 sizeZ,
    i32& placedCount)
{
    // 使用球形采样算法生成矿脉
    using namespace mc::world;

    placedCount = 0;
    i32 totalSize = sizeX * sizeY * sizeZ;

    // 使用位数组跟踪已处理的方块
    std::vector<bool> processed(totalSize, false);

    // 计算每个"球心"的位置
    std::vector<f32> sphereCenters;
    sphereCenters.resize(static_cast<size_t>(config.size) * 4);

    for (i32 i = 0; i < config.size; ++i) {
        f32 progress = static_cast<f32>(i) / static_cast<f32>(config.size);

        f32 cx = x1 + (x2 - x1) * progress;
        f32 cy = y1 + (y2 - y1) * progress;
        f32 cz = z1 + (z2 - z1) * progress;

        f32 radiusFactor = static_cast<f32>(random.nextDouble() * static_cast<f64>(config.size) / 16.0);
        f32 radius = (std::sin(math::PI * progress) + 1.0f) * radiusFactor + 1.0f;

        sphereCenters[static_cast<size_t>(i) * 4 + 0] = cx;
        sphereCenters[static_cast<size_t>(i) * 4 + 1] = cy;
        sphereCenters[static_cast<size_t>(i) * 4 + 2] = cz;
        sphereCenters[static_cast<size_t>(i) * 4 + 3] = radius;
    }

    // 处理每个球心
    for (i32 i = 0; i < config.size - 1; ++i) {
        f32 radius1 = sphereCenters[static_cast<size_t>(i) * 4 + 3];

        if (radius1 <= 0.0) {
            continue;
        }

        for (i32 j = i + 1; j < config.size; ++j) {
            f32 radius2 = sphereCenters[static_cast<size_t>(j) * 4 + 3];

            if (radius2 <= 0.0) {
                continue;
            }

            // 计算两球心之间的距离
            f32 dx = sphereCenters[static_cast<size_t>(i) * 4 + 0] - sphereCenters[static_cast<size_t>(j) * 4 + 0];
            f32 dy = sphereCenters[static_cast<size_t>(i) * 4 + 1] - sphereCenters[static_cast<size_t>(j) * 4 + 1];
            f32 dz = sphereCenters[static_cast<size_t>(i) * 4 + 2] - sphereCenters[static_cast<size_t>(j) * 4 + 2];

            f32 distSq = dx * dx + dy * dy + dz * dz;
            f32 radiusSum = radius1 + radius2;

            // 如果两球重叠太多，移除较小的那个
            if (radiusSum * radiusSum > distSq) {
                if (radius1 > radius2) {
                    sphereCenters[static_cast<size_t>(j) * 4 + 3] = -1.0;
                } else {
                    sphereCenters[static_cast<size_t>(i) * 4 + 3] = -1.0;
                    break;
                }
            }
        }
    }

    // 如果没有目标则直接返回
    if (config.targets.empty()) {
        return;
    }

    // 在每个球心周围放置矿石
    for (i32 i = 0; i < config.size; ++i) {
        f32 radius = sphereCenters[static_cast<size_t>(i) * 4 + 3];

        if (radius < 0.0) {
            continue;
        }

        f32 cx = sphereCenters[static_cast<size_t>(i) * 4 + 0];
        f32 cy = sphereCenters[static_cast<size_t>(i) * 4 + 1];
        f32 cz = sphereCenters[static_cast<size_t>(i) * 4 + 2];

        // 计算边界
        i32 localMinX = std::max(static_cast<i32>(std::floor(cx - radius)), minX);
        i32 localMinY = std::max(static_cast<i32>(std::floor(cy - radius)), minY);
        i32 localMinZ = std::max(static_cast<i32>(std::floor(cz - radius)), minZ);
        i32 localMaxX = std::max(static_cast<i32>(std::floor(cx + radius)), localMinX);
        i32 localMaxY = std::max(static_cast<i32>(std::floor(cy + radius)), localMinY);
        i32 localMaxZ = std::max(static_cast<i32>(std::floor(cz + radius)), localMinZ);

        // 遍历边界内的每个方块
        for (i32 bx = localMinX; bx <= localMaxX; ++bx) {
            f32 dx = (static_cast<f32>(bx) + 0.5f - cx) / radius;

            if (dx * dx >= 1.0f) {
                continue;
            }

            for (i32 by = localMinY; by <= localMaxY; ++by) {
                f32 dy = (static_cast<f32>(by) + 0.5f - cy) / radius;

                if (dx * dx + dy * dy >= 1.0f) {
                    continue;
                }

                for (i32 bz = localMinZ; bz <= localMaxZ; ++bz) {
                    f32 dz = (static_cast<f32>(bz) + 0.5f - cz) / radius;

                    if (dx * dx + dy * dy + dz * dz >= 1.0f) {
                        continue;
                    }

                    // 计算位数组索引
                    i32 index = (bx - minX) + (by - minY) * sizeX + (bz - minZ) * sizeX * sizeY;

                    if (index < 0 || index >= totalSize) {
                        continue;
                    }

                    if (processed[static_cast<size_t>(index)]) {
                        continue;
                    }

                    processed[static_cast<size_t>(index)] = true;

                    // 获取当前方块
                    const BlockState* currentState = region.getBlockState(bx, by, bz);

                    // MC 1.21: 遍历所有目标，使用第一个匹配的目标放置对应矿石
                    const BlockState* oreState = nullptr;
                    for (const auto& target : config.targets) {
                        if (currentState && target.target && target.target->test(*currentState, random)) {
                            oreState = target.state;
                            break;
                        }
                    }

                    if (oreState == nullptr) {
                        continue;
                    }

                    // MC 1.21: 检查空气暴露丢弃概率
                    if (config.discardChanceOnAirExposure > 0.0f) {
                        bool exposedToAir = false;
                        static constexpr i32 offsets[6][3] = {
                            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
                        for (i32 d = 0; d < 6; ++d) {
                            const BlockState* neighbor =
                                region.getBlockState(bx + offsets[d][0], by + offsets[d][1], bz + offsets[d][2]);
                            if (neighbor == nullptr || neighbor->isAir()) {
                                exposedToAir = true;
                                break;
                            }
                        }
                        if (exposedToAir && random.nextFloat() < config.discardChanceOnAirExposure) {
                            continue;
                        }
                    }

                    if (region.setBlockState(bx, by, bz, oreState)) {
                        ++placedCount;
                    }
                }
            }
        }
    }
}

// ============================================================================
// ConfiguredOreFeature 实现
// ============================================================================

ConfiguredOreFeature::ConfiguredOreFeature(std::unique_ptr<OreFeatureConfig> featureConfig, const char* featureName)
    : m_config(std::move(featureConfig))
    , m_name(featureName)
{}

bool ConfiguredOreFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    (void)chunk;
    (void)generator;

    if (!m_config) {
        return false;
    }

    // 数据驱动下 pos 已是 placement 链处理后的最终位置，直接放置矿脉
    OreFeature feature;
    return feature.place(region, chunk, random, pos, *m_config);
}

} // namespace mc
