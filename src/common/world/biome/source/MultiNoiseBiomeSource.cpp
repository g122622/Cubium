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

#include "MultiNoiseBiomeSource.hpp"
#include "NetherBiomeBuilder.hpp"
#include "OverworldBiomeBuilder.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/NoiseRouterData.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace biome {
namespace source {

MultiNoiseBiomeSource::MultiNoiseBiomeSource(const gen::RandomState& rs,
    climate::ParameterList<BiomeId> parameters,
    std::unique_ptr<gen::density::NoiseRouter> router)
    : IBiomeSource(rs.worldSeed())
    , m_parameters(std::move(parameters))
    , m_router(std::move(router))
    , m_sampler(m_router->createClimateSampler())
{
    MC_ASSERT_RELEASE(m_router != nullptr);

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
    const climate::TargetPoint target = m_sampler.sample(quartX, quartY, quartZ);
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

std::unique_ptr<MultiNoiseBiomeSource> MultiNoiseBiomeSource::createOverworld(
    const gen::RandomState& rs, bool largeBiomes, bool amplified)
{
    // 创建主世界噪声路由器：从 rs 的派生种子缓存获取 NormalNoise，与生成器共享
    auto router = std::make_unique<gen::density::NoiseRouter>(
        gen::density::NoiseRouterData::overworld(rs, rs.worldSeed(), largeBiomes, amplified));

    // 构建主世界生物群系参数列表
    OverworldBiomeBuilder builder;
    climate::ParameterList<BiomeId> parameters = builder.buildParameterList();

    // NoiseRouter 由 MultiNoiseBiomeSource 持有，Sampler 引用 Router 中的 DensityFunction
    return std::make_unique<MultiNoiseBiomeSource>(rs, std::move(parameters), std::move(router));
}

std::unique_ptr<MultiNoiseBiomeSource> MultiNoiseBiomeSource::createNether(const gen::RandomState& rs)
{
    // 创建下界噪声路由器：从 rs 的派生种子缓存获取 NormalNoise，与生成器共享
    auto router =
        std::make_unique<gen::density::NoiseRouter>(gen::density::NoiseRouterData::nether(rs, rs.worldSeed()));

    // 构建下界生物群系参数列表
    auto parameters = NetherBiomeBuilder::buildParameterList();

    return std::make_unique<MultiNoiseBiomeSource>(rs, std::move(parameters), std::move(router));
}

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
