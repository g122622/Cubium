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

#include "MultiNoiseBiomeSource.hpp"
#include "NetherBiomeBuilder.hpp"
#include "OverworldBiomeBuilder.hpp"
#include "common/world/gen/RandomState.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace biome {
namespace source {

MultiNoiseBiomeSource::MultiNoiseBiomeSource(
    u64 worldSeed, climate::ParameterList<BiomeId> parameters, const climate::Sampler& sampler)
    : IBiomeSource(worldSeed)
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
    // 对齐 MC 1.21.11 MultiNoiseBiomeSource：无状态，气候采样复用 RandomState.sampler()。
    // 6 个气候密度函数由 RandomState 的 NoiseRouter 管理，本类只持 ParameterList + Sampler 引用。
    // largeBiomes/amplified 保留参数对齐原版签名；气候差异已在 RandomState（noise_settings JSON）体现。
    (void)largeBiomes;
    (void)amplified;

    OverworldBiomeBuilder builder;
    climate::ParameterList<BiomeId> parameters = builder.buildParameterList();
    return std::make_unique<MultiNoiseBiomeSource>(rs.worldSeed(), std::move(parameters), rs.sampler());
}

std::unique_ptr<MultiNoiseBiomeSource> MultiNoiseBiomeSource::createNether(const gen::RandomState& rs)
{
    auto parameters = NetherBiomeBuilder::buildParameterList();
    return std::make_unique<MultiNoiseBiomeSource>(rs.worldSeed(), std::move(parameters), rs.sampler());
}

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
