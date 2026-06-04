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

#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/core/Constants.hpp"
#include "common/world/biome/source/NetherBiomeSource.hpp"
#include "common/world/biome/source/OverworldBiomeBuilder.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/gen/density/NoiseRouterData.hpp"

namespace mc::world::biome::source {

MultiNoiseBiomeSource::MultiNoiseBiomeSource(
    u64 seed, climate::ParameterList<BiomeId> parameters, const climate::Sampler* sampler)
    : BiomeSource(seed)
    , m_parameters(std::move(parameters))
    , m_sampler(sampler)
{
    // 收集所有可能的生物群系
    for (const auto& entry : m_parameters.entries()) {
        const BiomeId id = entry.second;
        if (std::find(m_possibleBiomes.begin(), m_possibleBiomes.end(), id) == m_possibleBiomes.end()) {
            m_possibleBiomes.push_back(id);
        }
    }
}

BiomeId MultiNoiseBiomeSource::getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const
{
    MC_ASSERT_RELEASE(m_sampler != nullptr);
    const climate::TargetPoint target = m_sampler->sample(quartX, quartY, quartZ);
    return m_parameters.findValue(target);
}

BiomeId MultiNoiseBiomeSource::getNoiseBiome(const climate::TargetPoint& target) const
{
    return m_parameters.findValue(target);
}

const std::vector<BiomeId>& MultiNoiseBiomeSource::possibleBiomes() const
{
    return m_possibleBiomes;
}

void MultiNoiseBiomeSource::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    // 区块内 4x4x4 采样网格遍历所有 section
    // 每个区块有 24 个 section（-64 ~ 320）
    constexpr i32 HORIZ_SIZE = 4;
    constexpr i32 VERT_SIZE = 4;
    constexpr i32 SECTION_COUNT = 24;

    for (i32 section = 0; section < SECTION_COUNT; ++section) {
        for (i32 y = 0; y < VERT_SIZE; ++y) {
            for (i32 z = 0; z < HORIZ_SIZE; ++z) {
                for (i32 x = 0; x < HORIZ_SIZE; ++x) {
                    // quart 坐标 = (区块坐标 * 4 + 采样偏移) / 4
                    // 因为 1 quart = 4 blocks，1 section = 16 blocks = 4 quart
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

std::unique_ptr<MultiNoiseBiomeSource> MultiNoiseBiomeSource::createOverworld(u64 seed, bool largeBiomes)
{
    // 创建主世界噪声路由器
    auto router =
        std::make_unique<gen::density::NoiseRouter>(gen::density::NoiseRouterData::overworld(seed, largeBiomes));

    // 创建气候采样器
    const climate::Sampler sampler = router->createClimateSampler();

    // 构建主世界生物群系参数列表
    OverworldBiomeBuilder builder;
    climate::ParameterList<BiomeId> parameters = builder.buildParameterList();

    // 注意：sampler 引用了 router 中的密度函数，需要保持 router 活着
    // 这里有一个生命周期问题：sampler 持有 DensityFunction 的裸指针，
    // 而 DensityFunction 由 router 持有。
    // 解决方案：将 router 和 sampler 一起管理。
    // 当前简化实现：直接在 MultiNoiseBiomeSource 内部持有 router。

    auto source = std::make_unique<MultiNoiseBiomeSource>(seed, std::move(parameters), nullptr);
    // TODO: 解决 sampler 生命周期问题，需要让 MultiNoiseBiomeSource 持有 NoiseRouter
    return source;
}

std::unique_ptr<MultiNoiseBiomeSource> MultiNoiseBiomeSource::createNether(u64 seed)
{
    // 创建下界噪声路由器
    auto router = std::make_unique<gen::density::NoiseRouter>(gen::density::NoiseRouterData::nether(seed));

    // 构建下界生物群系参数列表
    auto parameters = NetherBiomeSource::buildParameterList();

    auto source = std::make_unique<MultiNoiseBiomeSource>(seed, std::move(parameters), nullptr);
    return source;
}

} // namespace mc::world::biome::source
