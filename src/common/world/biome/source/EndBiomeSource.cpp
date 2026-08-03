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

#include "EndBiomeSource.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace biome {
namespace source {

EndBiomeSource::EndBiomeSource(const gen::RandomState& rs)
    : IBiomeSource(rs.worldSeed())
    , m_islandNoise(std::make_unique<gen::density::EndIslands>(rs.worldSeed()))
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

    // 中央岛屿检查：使用区块坐标判断
    // blockToSectionCoord = blockX >> 4，4096 = 64^2（64格区块半径）
    if (isInCentralIsland(blockX, blockZ)) {
        return Biomes::TheEnd;
    }

    // 外围岛屿：噪声采样使用区块坐标缩放
    const i32 chunkX = blockX >> 4;
    const i32 chunkZ = blockZ >> 4;
    const i32 sampleX = (chunkX * 2 + 1) * 8;
    const i32 sampleZ = (chunkZ * 2 + 1) * 8;
    const i32 blockY = quartY << 2;
    const f64 islandNoise = m_islandNoise->compute(sampleX, blockY, sampleZ);

    // 生物群系映射阈值
    if (islandNoise > 0.25) {
        return Biomes::EndHighlands;
    }
    if (islandNoise >= -0.0625) {
        return Biomes::EndMidlands;
    }
    if (islandNoise < -0.21875) {
        return Biomes::SmallEndIslands;
    }
    return Biomes::EndBarrens;
}

const std::vector<BiomeId>& EndBiomeSource::possibleBiomes() const
{
    return m_possibleBiomes;
}

bool EndBiomeSource::isInCentralIsland(i32 blockX, i32 blockZ)
{
    // 使用区块坐标判断中央岛屿
    // SectionPos.blockToSectionCoord(blockX) = blockX >> 4
    // 4096 = 64^2，即距原点64个区块半径（= 64*16 = 1024 格方块半径）
    const i32 chunkX = blockX >> 4;
    const i32 chunkZ = blockZ >> 4;
    const i64 cx = static_cast<i64>(chunkX);
    const i64 cz = static_cast<i64>(chunkZ);
    return cx * cx + cz * cz <= 4096LL;
}

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
