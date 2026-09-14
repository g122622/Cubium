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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CarvingContext.hpp"
#include "common/core/Types.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

const BlockState* CarvingContext::topMaterial(
    const world::biome::IBiomeSource& biomeSource, i32 worldX, i32 worldY, i32 worldZ, bool hasFluid) const
{
    // 将方块坐标转换为 quart 坐标（1 quart = 4 blocks）
    const i32 quartX = worldX >> 2;
    const i32 quartY = worldY >> 2;
    const i32 quartZ = worldZ >> 2;

    // 查询该位置的生物群系
    const BiomeId biomeId = biomeSource.getNoiseBiome(quartX, quartY, quartZ);
    const world::biome::Biome& biome = biomeSource.getBiomeDefinition(biomeId);

    // MC 1.21: 当雕刻后该位置含流体时，使用水下地表方块（如砾石、沙子）
    // 否则使用常规地表方块（如草地、沙子、菌丝等）
    if (hasFluid) {
        if (const BlockState* underWaterBlock = biome.underWaterBlock()) {
            return underWaterBlock;
        }
    }

    if (const BlockState* surfaceBlock = biome.surfaceBlock()) {
        return surfaceBlock;
    }

    return nullptr;
}

} // namespace mc
