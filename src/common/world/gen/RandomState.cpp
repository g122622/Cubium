/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "RandomState.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include "density/NoiseRouterData.hpp"

namespace mc::world::gen {

RandomState::RandomState(u64 worldSeed, const DimensionSettings& settings)
    : m_worldSeed(worldSeed)
    , m_settings(settings)
{}

std::unique_ptr<RandomState> RandomState::create(const DimensionSettings& settings, u64 worldSeed)
{
    auto state = std::unique_ptr<RandomState>(new RandomState(worldSeed, settings));

    // 创建 NoiseRouter（根据维度类型选择预设）
    switch (settings.dimensionKind) {
        case DimensionKind::End:
            state->m_router = std::make_unique<density::NoiseRouter>(density::NoiseRouterData::end(worldSeed));
            break;
        case DimensionKind::Nether:
            state->m_router = std::make_unique<density::NoiseRouter>(density::NoiseRouterData::nether(worldSeed));
            break;
        case DimensionKind::Overworld:
        default:
            state->m_router = std::make_unique<density::NoiseRouter>(
                density::NoiseRouterData::overworld(worldSeed, settings.largeBiomes));
            break;
    }

    // 创建 Climate::Sampler（从 NoiseRouter 的 6 个气候函数）
    state->m_sampler = std::make_unique<biome::climate::Sampler>(state->m_router->createClimateSampler());

    // 创建 SurfaceSystem（根据维度选择表面规则）
    std::unique_ptr<surface::SurfaceRule> surfaceRule;
    switch (settings.dimensionKind) {
        case DimensionKind::End:
            surfaceRule = surface::SurfaceRules::end();
            break;
        case DimensionKind::Nether:
            surfaceRule = surface::SurfaceRules::nether(worldSeed);
            break;
        case DimensionKind::Overworld:
        default:
            surfaceRule = surface::SurfaceRules::overworld(worldSeed);
            break;
    }

    if (surfaceRule) {
        state->m_surfaceSystem = std::make_unique<surface::SurfaceSystem>(std::move(surfaceRule),
            settings.defaultBlock,
            settings.defaultFluid,
            settings.seaLevel,
            settings.noise.minY,
            settings.noise.height,
            worldSeed);
    }

    // 创建 PositionalRandomFactory（含水层随机源）
    // MC 1.21: RandomState 构造时从 worldSeed fork 出位置随机工厂
    ::mc::math::Xoroshiro128ppRandom mainRng(worldSeed);
    state->m_positionalRandom = std::make_unique<::mc::math::PositionalRandomFactory>(mainRng.forkPositional());

    // 含水层随机工厂（独立种子偏移）
    ::mc::math::Xoroshiro128ppRandom aquiferRng(worldSeed ^ 0x9E3779B97F4A7C15ULL);
    state->m_aquiferRandom = std::make_unique<::mc::math::PositionalRandomFactory>(aquiferRng.forkPositional());

    // 矿石脉随机工厂（独立种子偏移）
    ::mc::math::Xoroshiro128ppRandom oreRng(worldSeed ^ 0x6A09E667F3BCC909ULL);
    state->m_oreRandom = std::make_unique<::mc::math::PositionalRandomFactory>(oreRng.forkPositional());

    return state;
}

density::NoiseRouter RandomState::createRouterCopy() const
{
    // 使用相同种子和设置重新创建路由器
    // 每个 NoiseChunk 需要自己的路由器副本，因为 mapAll() 会修改密度函数树
    switch (m_settings.dimensionKind) {
        case DimensionKind::End:
            return density::NoiseRouterData::end(m_worldSeed);
        case DimensionKind::Nether:
            return density::NoiseRouterData::nether(m_worldSeed);
        case DimensionKind::Overworld:
        default:
            return density::NoiseRouterData::overworld(m_worldSeed, m_settings.largeBiomes);
    }
}

} // namespace mc::world::gen
