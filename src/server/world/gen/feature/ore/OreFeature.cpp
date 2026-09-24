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
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

namespace {

/// 判断 Y 是否在可建造高度之外（对齐 MC LevelReader.isOutsideBuildHeight）
[[nodiscard]] bool _isOutsideBuildHeight(i32 y) noexcept
{
    return y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT;
}

/**
 * @brief 是否跳过"邻接空气"检查（对齐 MC OreFeature.shouldSkipAirCheck）
 *
 * 返回 true 表示不做空气暴露判定，直接允许放置。注意 RNG 消耗时机：
 * 只要 discardChance 落在 (0, 1) 开区间就会消耗一次 nextFloat，
 * 与被测方块是否邻接空气无关——顺序错位会让后续所有随机量整体偏移。
 */
bool _shouldSkipAirCheck(math::IRandom& random, f32 discardChance)
{
    if (discardChance <= 0.0F) {
        return true;
    }
    if (discardChance >= 1.0F) {
        return false;
    }
    return random.nextFloat() >= discardChance;
}

/// 六邻域中是否存在空气（对齐 MC Feature.isAdjacentToAir → checkNeighbors）
bool _isAdjacentToAir(WorldGenRegion& region, i32 x, i32 y, i32 z)
{
    static constexpr i32 kOffsets[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const auto& offset : kOffsets) {
        const BlockState* neighbor = region.getBlockState(x + offset[0], y + offset[1], z + offset[2]);
        if (neighbor != nullptr && neighbor->isAir()) {
            return true;
        }
    }
    return false;
}

} // namespace

// ============================================================================
// OreFeature 实现
// ============================================================================

bool OreFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    math::IRandom& random,
    const BlockPos& origin,
    const OreFeatureConfig& config)
{
    (void)chunk; // 方块读写统一走 region（对齐原版 WorldGenLevel 接口）
    // 生成椭圆体状矿脉（对齐 MC OreFeature.place）。
    // 轴线方向：原版用 java.lang.Math.sin/cos（精确 double 三角函数），不是 Mth 查表版；
    // 只有半径包络那处才用 Mth.sin。两者混用会整体偏移，须按原版区分。
    const f32 angle = random.nextFloat() * static_cast<f32>(math::PI);
    const f32 sizeFactor = static_cast<f32>(config.size) / 8.0F;
    const i32 halfSize = math::ceilTo<i32>((static_cast<f32>(config.size) / 16.0F * 2.0F + 1.0F) / 2.0F);

    const f64 x1 = static_cast<f64>(origin.x) + std::sin(static_cast<f64>(angle)) * static_cast<f64>(sizeFactor);
    const f64 x2 = static_cast<f64>(origin.x) - std::sin(static_cast<f64>(angle)) * static_cast<f64>(sizeFactor);
    const f64 z1 = static_cast<f64>(origin.z) + std::cos(static_cast<f64>(angle)) * static_cast<f64>(sizeFactor);
    const f64 z2 = static_cast<f64>(origin.z) - std::cos(static_cast<f64>(angle)) * static_cast<f64>(sizeFactor);

    // Y 方向的随机抖动是 nextInt(3) - 2（取值 -2..0），不是 nextInt(-2, 2)（取值 -2..2）。
    // 两者取值集合不同，会改变轴线倾角与后续所有随机量的消耗。
    const f64 y1 = static_cast<f64>(origin.y + random.nextInt(3) - 2);
    const f64 y2 = static_cast<f64>(origin.y + random.nextInt(3) - 2);

    const i32 minX = origin.x - math::ceilTo<i32>(sizeFactor) - halfSize;
    const i32 minY = origin.y - 2 - halfSize;
    const i32 minZ = origin.z - math::ceilTo<i32>(sizeFactor) - halfSize;
    const i32 sizeX = 2 * (math::ceilTo<i32>(sizeFactor) + halfSize);
    const i32 sizeY = 2 * (2 + halfSize);
    const i32 sizeZ = sizeX;

    // 放置前提：包围盒内只要有一列的地表不低于包围盒底，就尝试放置。
    // 高度图取 OCEAN_FLOOR_WG（对齐原版），它不计入水，因此海底列不会把包围盒底"抬高"。
    for (i32 checkX = minX; checkX <= minX + sizeX; ++checkX) {
        for (i32 checkZ = minZ; checkZ <= minZ + sizeZ; ++checkZ) {
            const i32 topY = region.getHeight(checkX, checkZ, HeightmapType::OceanFloorWG);
            if (minY <= topY) {
                return _doPlace(region, random, config, x1, y1, z1, x2, y2, z2, minX, minY, minZ, sizeX, sizeY, sizeZ) >
                    0;
            }
        }
    }

    return false;
}

i32 OreFeature::_doPlace(WorldGenRegion& region,
    math::IRandom& random,
    const OreFeatureConfig& config,
    f64 x1,
    f64 y1,
    f64 z1,
    f64 x2,
    f64 y2,
    f64 z2,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 sizeX,
    i32 sizeY,
    i32 sizeZ)
{
    i32 placedCount = 0;
    const i32 veinSize = config.size;

    // 用位图记录已处理过的方块位置，避免多个球体重复判定同一格。
    std::vector<bool> processed(
        static_cast<size_t>(sizeX) * static_cast<size_t>(sizeY) * static_cast<size_t>(sizeZ), false);

    // 沿轴线等距布置 veinSize 个球心，每项 4 个分量：[x, y, z, radius]
    std::vector<f64> spheres(static_cast<size_t>(veinSize) * 4, 0.0);
    for (i32 i = 0; i < veinSize; ++i) {
        const f32 progress = static_cast<f32>(i) / static_cast<f32>(veinSize);

        // 注意 Mth.lerp 的参数顺序与项目的 math::lerp 相反：
        //   Java:   Mth.lerp(delta, start, end) = end + delta * (start - end)
        //   Cubium: math::lerp(a, b, t)         = a + t * (b - a)
        // 即 math::lerp(end, start, delta)。写反会让球心落到与轴线无关的位置，
        // 球体判定随即全部落空，矿脉一个方块都放不出来。
        spheres[static_cast<size_t>(i) * 4 + 0] = math::lerp(x2, x1, static_cast<f64>(progress));
        spheres[static_cast<size_t>(i) * 4 + 1] = math::lerp(y2, y1, static_cast<f64>(progress));
        spheres[static_cast<size_t>(i) * 4 + 2] = math::lerp(z2, z1, static_cast<f64>(progress));

        const f64 radiusScale = random.nextDouble() * static_cast<f64>(veinSize) / 16.0;
        // 半径包络用 Mth.sin 的量化查表值（f32），再与 f64 的 radiusScale 相乘后取半。
        // 末尾的 /2.0 是原版就有的一步：不做的话半径整体翻倍，矿脉体积膨胀约 8 倍。
        const f32 sinEnvelope = math::mthSin(static_cast<f64>(static_cast<f32>(math::PI) * progress)) + 1.0F;
        spheres[static_cast<size_t>(i) * 4 + 3] = (static_cast<f64>(sinEnvelope) * radiusScale + 1.0) / 2.0;
    }

    // 球体重叠消除：两个球心距离小于**半径之差的绝对值**时，保留半径较大的那个。
    // 判据是 |r1 - r2| 而非 r1 + r2——后者会在球体只是相邻时就误删，把矿脉打断。
    for (i32 i = 0; i < veinSize - 1; ++i) {
        if (spheres[static_cast<size_t>(i) * 4 + 3] <= 0.0) {
            continue;
        }
        for (i32 j = i + 1; j < veinSize; ++j) {
            if (spheres[static_cast<size_t>(j) * 4 + 3] <= 0.0) {
                continue;
            }

            const f64 dx = spheres[static_cast<size_t>(i) * 4 + 0] - spheres[static_cast<size_t>(j) * 4 + 0];
            const f64 dy = spheres[static_cast<size_t>(i) * 4 + 1] - spheres[static_cast<size_t>(j) * 4 + 1];
            const f64 dz = spheres[static_cast<size_t>(i) * 4 + 2] - spheres[static_cast<size_t>(j) * 4 + 2];
            const f64 dr = spheres[static_cast<size_t>(i) * 4 + 3] - spheres[static_cast<size_t>(j) * 4 + 3];

            if (dr * dr > dx * dx + dy * dy + dz * dz) {
                if (dr > 0.0) {
                    spheres[static_cast<size_t>(j) * 4 + 3] = -1.0;
                } else {
                    spheres[static_cast<size_t>(i) * 4 + 3] = -1.0;
                }
            }
        }
    }

    if (config.targets.empty()) {
        return 0;
    }

    for (i32 i = 0; i < veinSize; ++i) {
        const f64 radius = spheres[static_cast<size_t>(i) * 4 + 3];
        if (radius < 0.0) {
            continue;
        }

        const f64 cx = spheres[static_cast<size_t>(i) * 4 + 0];
        const f64 cy = spheres[static_cast<size_t>(i) * 4 + 1];
        const f64 cz = spheres[static_cast<size_t>(i) * 4 + 2];

        // 球体扫描范围：先按球心 ± 半径取整，再夹到包围盒内。
        const i32 scanMinX = std::max(math::floorTo<i32>(cx - radius), minX);
        const i32 scanMinY = std::max(math::floorTo<i32>(cy - radius), minY);
        const i32 scanMinZ = std::max(math::floorTo<i32>(cz - radius), minZ);
        const i32 scanMaxX = std::max(math::floorTo<i32>(cx + radius), scanMinX);
        const i32 scanMaxY = std::max(math::floorTo<i32>(cy + radius), scanMinY);
        const i32 scanMaxZ = std::max(math::floorTo<i32>(cz + radius), scanMinZ);

        for (i32 bx = scanMinX; bx <= scanMaxX; ++bx) {
            const f64 nx = (static_cast<f64>(bx) + 0.5 - cx) / radius;
            if (nx * nx >= 1.0) {
                continue;
            }

            for (i32 by = scanMinY; by <= scanMaxY; ++by) {
                const f64 ny = (static_cast<f64>(by) + 0.5 - cy) / radius;
                if (nx * nx + ny * ny >= 1.0) {
                    continue;
                }

                for (i32 bz = scanMinZ; bz <= scanMaxZ; ++bz) {
                    const f64 nz = (static_cast<f64>(bz) + 0.5 - cz) / radius;
                    if (nx * nx + ny * ny + nz * nz >= 1.0 || _isOutsideBuildHeight(by)) {
                        continue;
                    }

                    const i32 index = (bx - minX) + (by - minY) * sizeX + (bz - minZ) * sizeX * sizeY;
                    if (index < 0 || index >= static_cast<i32>(processed.size())) {
                        continue;
                    }
                    if (processed[static_cast<size_t>(index)]) {
                        continue;
                    }
                    // 先标记后判定（对齐原版）：即便本次因写入范围或匹配失败而未放置，
                    // 该格也不会被后续球体重新判定。
                    processed[static_cast<size_t>(index)] = true;

                    if (!region.ensureCanWrite(bx, by, bz)) {
                        continue;
                    }

                    const BlockState* currentState = region.getBlockState(bx, by, bz);
                    if (currentState == nullptr) {
                        continue;
                    }

                    for (const auto& target : config.targets) {
                        if (target.state == nullptr) {
                            continue;
                        }
                        if (!_canPlaceOre(region, random, config, target, bx, by, bz)) {
                            continue;
                        }
                        if (region.setBlockState(bx, by, bz, target.state)) {
                            ++placedCount;
                        }
                        break;
                    }
                }
            }
        }
    }

    return placedCount;
}

bool OreFeature::_canPlaceOre(WorldGenRegion& region,
    math::IRandom& random,
    const OreFeatureConfig& config,
    const OreTarget& target,
    i32 x,
    i32 y,
    i32 z)
{
    const BlockState* state = region.getBlockState(x, y, z);
    if (state == nullptr || !target.target) {
        return false;
    }
    if (!target.target->test(*state, random)) {
        return false;
    }
    if (_shouldSkipAirCheck(random, config.discardChanceOnAirExposure)) {
        return true;
    }
    return !_isAdjacentToAir(region, x, y, z);
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
    math::IRandom& random,
    const BlockPos& pos) const
{
    (void)generator;

    if (!m_config) {
        return false;
    }

    // 数据驱动下 pos 已是 placement 链处理后的最终位置，直接放置矿脉
    OreFeature feature;
    return feature.place(region, chunk, random, pos, *m_config);
}

} // namespace mc
