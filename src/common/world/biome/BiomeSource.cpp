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

#include "common/world/biome/BiomeSource.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/core/Constants.hpp"
#include <cmath>

namespace mc::world::biome {

// ============================================================================
// BiomeSource 实现
// ============================================================================

BiomeSource::BiomeSource(u64 seed)
    : m_seed(seed)
{
}

const Biome& BiomeSource::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

std::optional<BlockPos> BiomeSource::findBiome(i32 centerX,
    i32 centerY,
    i32 centerZ,
    i32 radius,
    i32 step,
    const std::function<bool(BiomeId)>& predicate,
    math::Random& random,
    bool stopOnFirst) const
{
    // quart 坐标 = 方块坐标 / 4（每个 quart 单元是 4x4 方块）
    const i32 quartX = centerX >> 2;
    const i32 quartZ = centerZ >> 2;
    const i32 quartRadius = radius >> 2;
    const i32 quartY = centerY >> 2;
    const i32 quartStep = step >> 2;

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

} // namespace mc::world::biome
