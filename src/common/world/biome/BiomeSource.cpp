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
 */

#include "BiomeSource.hpp"
#include "BiomeRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include <cmath>
#include <functional>
#include <optional>
#include <unordered_set>

namespace mc {
namespace world {
namespace biome {

// ============================================================================
// IBiomeSource 实现
// ============================================================================

IBiomeSource::IBiomeSource(u64 seed)
    : m_seed(seed)
{}

const Biome& IBiomeSource::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

void IBiomeSource::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    constexpr i32 HORIZ_SIZE = 4;
    constexpr i32 VERT_SIZE = 4;
    constexpr i32 SECTION_COUNT = world::CHUNK_SECTIONS;

    for (i32 section = 0; section < SECTION_COUNT; ++section) {
        for (i32 y = 0; y < VERT_SIZE; ++y) {
            for (i32 z = 0; z < HORIZ_SIZE; ++z) {
                for (i32 x = 0; x < HORIZ_SIZE; ++x) {
                    const i32 quartX = (chunkX * HORIZ_SIZE) + x;
                    const i32 quartY = (section * VERT_SIZE) + y + math::floorDiv(world::MIN_BUILD_HEIGHT, 4);
                    const i32 quartZ = (chunkZ * HORIZ_SIZE) + z;

                    const BiomeId biome = getNoiseBiome(quartX, quartY, quartZ);
                    container.setBiome(section, x, y, z, biome);
                }
            }
        }
    }
}

std::optional<BlockPos> IBiomeSource::findBiome(i32 centerX,
    i32 centerY,
    i32 centerZ,
    i32 radius,
    i32 step,
    const std::function<bool(BiomeId)>& predicate,
    math::Random& random,
    bool stopOnFirst) const
{
    // quart 坐标 = floorDiv(方块坐标, 4)，负坐标下 >> 2 不是向下取整
    const i32 quartX = math::floorDiv(centerX, 4);
    const i32 quartZ = math::floorDiv(centerZ, 4);
    const i32 quartRadius = math::floorDiv(radius, 4);
    const i32 quartY = math::floorDiv(centerY, 4);
    const i32 quartStep = math::floorDiv(step, 4);

    BlockPos result(0, 0, 0);
    i32 matchCount = 0;

    // 从内向外搜索
    // stopOnFirst 模式：从 0 开始；否则从最外圈开始向内搜索
    const i32 startRadius = stopOnFirst ? 0 : quartRadius;

    for (i32 r = startRadius; r <= quartRadius; r += quartStep) {
        // 在半径为 r 的正方形环上搜索
        for (i32 dz = -r; dz <= r; dz += quartStep) {
            const bool isEdgeZ = std::abs(dz) == r;

            for (i32 dx = -r; dx <= r; dx += quartStep) {
                // stopOnFirst 模式：搜索整个区域
                // 非 stopOnFirst 模式：只搜索当前环的边缘
                if (!stopOnFirst) {
                    const bool isEdgeX = std::abs(dx) == r;
                    if (!isEdgeX && !isEdgeZ) {
                        continue;
                    }
                }

                const i32 checkQuartX = quartX + dx;
                const i32 checkQuartZ = quartZ + dz;

                // 检查生物群系
                const BiomeId biome = getNoiseBiome(checkQuartX, quartY, checkQuartZ);
                if (predicate(biome)) {
                    // 转换回世界坐标
                    const BlockPos foundPos(checkQuartX << 2, centerY, checkQuartZ << 2);

                    if (stopOnFirst) {
                        return foundPos;
                    }

                    // 随机选择（蓄水池采样）
                    ++matchCount;
                    if (random.nextInt(matchCount) == 0) {
                        result = foundPos;
                    }
                }
            }
        }
    }

    if (matchCount > 0) {
        return result;
    }

    return std::nullopt;
}

std::unordered_set<BiomeId> IBiomeSource::getBiomesWithin(i32 x, i32 y, i32 z, i32 radius) const
{
    const i32 minQuartX = math::floorDiv(x - radius, 4);
    const i32 minQuartY = math::floorDiv(y - radius, 4);
    const i32 minQuartZ = math::floorDiv(z - radius, 4);
    const i32 maxQuartX = math::floorDiv(x + radius, 4);
    const i32 maxQuartY = math::floorDiv(y + radius, 4);
    const i32 maxQuartZ = math::floorDiv(z + radius, 4);

    std::unordered_set<BiomeId> result;

    for (i32 qz = minQuartZ; qz <= maxQuartZ; ++qz) {
        for (i32 qx = minQuartX; qx <= maxQuartX; ++qx) {
            for (i32 qy = minQuartY; qy <= maxQuartY; ++qy) {
                result.insert(getNoiseBiome(qx, qy, qz));
            }
        }
    }

    return result;
}

} // namespace biome
} // namespace world
} // namespace mc
