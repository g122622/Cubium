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

#include "common/world/biome/BiomeProvider.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// BiomeProvider 实现
// ============================================================================

BiomeProvider::BiomeProvider(u64 seed)
    : m_seed(seed)
{}

const Biome& BiomeProvider::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

std::optional<BlockPos> BiomeProvider::findBiome(i32 centerX,
    i32 centerY,
    i32 centerZ,
    i32 radius,
    i32 step,
    const std::function<bool(BiomeId)>& predicate,
    math::Random& random,
    bool stopOnFirst) const
{
    // 噪声坐标 = 方块坐标 / 4（每个噪声单元是 4x4 方块）
    i32 noiseX = centerX >> 2;
    i32 noiseZ = centerZ >> 2;
    i32 noiseRadius = radius >> 2;
    i32 noiseY = centerY >> 2;

    BlockPos result(0, 0, 0);
    i32 matchCount = 0;

    // 从内向外搜索
    // stopOnFirst 模式：从 0 开始；否则从最外圈开始向内搜索
    i32 startRadius = stopOnFirst ? 0 : noiseRadius;

    for (i32 r = startRadius; r <= noiseRadius; r += (step >> 2)) {
        // 在半径为 r 的正方形环上搜索
        for (i32 dz = -r; dz <= r; dz += (step >> 2)) {
            bool isEdgeZ = std::abs(dz) == r;

            for (i32 dx = -r; dx <= r; dx += (step >> 2)) {
                // stopOnFirst 模式：搜索整个区域
                // 非stopOnFirst模式：只搜索当前环的边缘
                if (!stopOnFirst) {
                    bool isEdgeX = std::abs(dx) == r;
                    if (!isEdgeX && !isEdgeZ) {
                        continue;
                    }
                }

                i32 checkNoiseX = noiseX + dx;
                i32 checkNoiseZ = noiseZ + dz;

                // 检查生物群系
                BiomeId biome = getNoiseBiome(checkNoiseX, noiseY, checkNoiseZ);
                if (predicate(biome)) {
                    // 转换回世界坐标
                    BlockPos foundPos(checkNoiseX << 2, centerY, checkNoiseZ << 2);

                    if (stopOnFirst) {
                        // 找到第一个即返回
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

} // namespace mc
