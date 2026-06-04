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

#include "common/world/biome/source/EndBiomeSource.hpp"
#include "common/core/Constants.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"

namespace mc::world::biome::source {

EndBiomeSource::EndBiomeSource(u64 seed)
    : BiomeSource(seed)
    , m_islandNoise(std::make_unique<gen::density::EndIslands>(seed))
{
    m_possibleBiomes = {
        Biomes::TheEnd, Biomes::EndHighlands, Biomes::EndMidlands, Biomes::SmallEndIslands, Biomes::EndBarrens};
}

EndBiomeSource::~EndBiomeSource() = default;

EndBiomeSource::EndBiomeSource(EndBiomeSource&&) noexcept = default;
EndBiomeSource& EndBiomeSource::operator=(EndBiomeSource&&) noexcept = default;

BiomeId EndBiomeSource::getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const
{
    MC_UNUSED(quartY);

    // 转换为方块坐标
    const i32 blockX = quartX << 2;
    const i32 blockZ = quartZ << 2;

    // 中央岛屿检查（距原点64格内，即 x²+z² <= 4096）
    if (isInCentralIsland(blockX, blockZ)) {
        return Biomes::TheEnd;
    }

    // MC 1.21: 使用 EndIslands 密度函数判断外围岛屿生物群系
    const f64 islandNoise = m_islandNoise->compute(blockX, 0, blockZ);

    // MC 1.21 的生物群系映射逻辑
    // islandNoise > 0 表示在岛屿上，根据高度确定生物群系
    if (islandNoise > 0.0) {
        // 在岛屿上
        if (islandNoise > 0.25) {
            return Biomes::EndHighlands;
        }
        return Biomes::EndMidlands;
    }

    // 不在岛屿上
    if (islandNoise < -0.21875) {
        return Biomes::SmallEndIslands;
    }
    return Biomes::EndBarrens;
}

const std::vector<BiomeId>& EndBiomeSource::possibleBiomes() const
{
    return m_possibleBiomes;
}

void EndBiomeSource::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    constexpr i32 HORIZ_SIZE = 4;
    constexpr i32 VERT_SIZE = 4;
    constexpr i32 SECTION_COUNT = 24;

    for (i32 section = 0; section < SECTION_COUNT; ++section) {
        for (i32 y = 0; y < VERT_SIZE; ++y) {
            for (i32 z = 0; z < HORIZ_SIZE; ++z) {
                for (i32 x = 0; x < HORIZ_SIZE; ++x) {
                    const i32 quartX = (chunkX * HORIZ_SIZE) + x;
                    const i32 quartY = (section * VERT_SIZE) + y + (world::MIN_BUILD_HEIGHT >> 2);
                    const i32 quartZ = (chunkZ * HORIZ_SIZE) + z;

                    const BiomeId biome = getNoiseBiome(quartX, quartY, quartZ);
                    container.setBiome(section, x, y, z, biome);
                }
            }
        }
    }
}

bool EndBiomeSource::isInCentralIsland(i32 blockX, i32 blockZ)
{
    // MC 1.21: 中央岛屿范围 — 方块坐标 x² + z² <= 4096（64 格半径）
    const i64 bx = static_cast<i64>(blockX);
    const i64 bz = static_cast<i64>(blockZ);
    return bx * bx + bz * bz <= 4096LL;
}

} // namespace mc::world::biome::source
