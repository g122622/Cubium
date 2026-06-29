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
#include "common/world/gen/noise/Noises.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
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
        case DimensionKind::FloatingIslands:
            state->m_router = std::make_unique<density::NoiseRouter>(density::NoiseRouterData::end(worldSeed));
            break;
        case DimensionKind::Nether:
            state->m_router = std::make_unique<density::NoiseRouter>(density::NoiseRouterData::nether(worldSeed));
            break;
        case DimensionKind::Overworld:
        case DimensionKind::LargeBiomes:
        case DimensionKind::Amplified:
        case DimensionKind::Caves:
        case DimensionKind::Flat:
        default:
            state->m_router = std::make_unique<density::NoiseRouter>(
                density::NoiseRouterData::overworld(worldSeed, settings.largeBiomes));
            break;
    }

    // 创建 Climate::Sampler（从 NoiseRouter 的 6 个气候函数）
    state->m_sampler = std::make_unique<biome::climate::Sampler>(state->m_router->createClimateSampler());

    // 创建 PositionalRandomFactory（必须在 SurfaceSystem 之前，因为 getOrCreateNoise 需要它）
    // MC 1.21: RandomState 构造时从 worldSeed fork 出位置随机工厂
    ::mc::math::Xoroshiro128ppRandom mainRng(worldSeed);
    state->m_positionalRandom = std::make_unique<::mc::math::PositionalRandomFactory>(mainRng.forkPositional());

    // 创建 SurfaceSystem（根据维度选择表面规则）
    // MC 1.21: 表面规则不再需要 seed — 噪声名称通过 RandomState 查找
    std::unique_ptr<surface::SurfaceRule> surfaceRule;
    switch (settings.dimensionKind) {
        case DimensionKind::End:
        case DimensionKind::FloatingIslands:
            surfaceRule = surface::SurfaceRules::end();
            break;
        case DimensionKind::Nether:
            surfaceRule = surface::SurfaceRules::nether();
            break;
        case DimensionKind::Overworld:
        case DimensionKind::LargeBiomes:
        case DimensionKind::Amplified:
        case DimensionKind::Caves:
        case DimensionKind::Flat:
        default:
            surfaceRule = surface::SurfaceRules::overworld();
            break;
    }

    if (surfaceRule) {
        // MC 1.21: SurfaceSystem 构造接收 RandomState 和 PositionalRandomFactory
        // SurfaceSystem 从 RandomState 获取噪声实例，PositionalRandomFactory 用于 getSurfaceDepth 和 clayBands
        ::mc::math::Xoroshiro128ppRandom surfaceRng(worldSeed);
        auto surfacePositionalRandom = surfaceRng.forkPositional();

        state->m_surfaceSystem = std::make_unique<surface::SurfaceSystem>(std::move(surfaceRule),
            settings.defaultBlock,
            settings.defaultFluid,
            settings.seaLevel,
            settings.noise.minY,
            settings.noise.height,
            *state,
            surfacePositionalRandom);
    }

    // 含水层随机工厂
    // MC 1.21: RandomState 构造时从 noiseRandom.fromHashOf("minecraft:aquifer").forkPositional() 创建
    {
        auto aquiferRng = state->m_positionalRandom->fromHashOf("minecraft:aquifer");
        state->m_aquiferRandom = std::make_unique<::mc::math::PositionalRandomFactory>(aquiferRng->forkPositional());
    }

    // 矿石脉随机工厂
    // MC 1.21: RandomState 构造时从 noiseRandom.fromHashOf("minecraft:ore").forkPositional() 创建
    {
        auto oreRng = state->m_positionalRandom->fromHashOf("minecraft:ore");
        state->m_oreRandom = std::make_unique<::mc::math::PositionalRandomFactory>(oreRng->forkPositional());
    }

    return state;
}

density::NoiseRouter RandomState::createRouterCopy() const
{
    // 使用相同种子和设置重新创建路由器
    // 每个 NoiseChunk 需要自己的路由器副本，因为 mapAll() 会修改密度函数树
    switch (m_settings.dimensionKind) {
        case DimensionKind::End:
        case DimensionKind::FloatingIslands:
            return density::NoiseRouterData::end(m_worldSeed);
        case DimensionKind::Nether:
            return density::NoiseRouterData::nether(m_worldSeed);
        case DimensionKind::Overworld:
        case DimensionKind::LargeBiomes:
        case DimensionKind::Amplified:
        case DimensionKind::Caves:
        case DimensionKind::Flat:
        default:
            return density::NoiseRouterData::overworld(m_worldSeed, m_settings.largeBiomes);
    }
}

noise::NormalNoise& RandomState::getOrCreateNoise(const std::string& name)
{
    // 命中路径：shared_lock 并发读
    {
        std::shared_lock lock(m_noiseMutex);
        auto it = m_noiseCache.find(name);
        if (it != m_noiseCache.end()) {
            return *it->second;
        }
    }

    // miss 路径：unique_lock 写入（double-check，避免重复构造）
    std::unique_lock lock(m_noiseMutex);
    auto it = m_noiseCache.find(name);
    if (it != m_noiseCache.end()) {
        return *it->second;
    }

    // MC 1.21: RandomState.getOrCreateNoise()
    // 1. 从 Noises 注册表获取参数
    const noise::NoiseParameters& params = noise::Noises::get(name);

    // 2. 使用 fromHashOf(name) 创建随机源
    //    NormalNoise 构造函数会调用 rng.forkPositional() 两次来创建两个 PerlinNoise
    auto rng = m_positionalRandom->fromHashOf(name);

    // 3. 创建 NormalNoise
    auto noise = std::make_unique<noise::NormalNoise>(*rng, params.firstOctave, params.amplitudes);

    auto& ref = *noise;
    m_noiseCache.emplace(name, std::move(noise));
    return ref;
}

::mc::math::PositionalRandomFactory& RandomState::getOrCreateRandomFactory(const std::string& name)
{
    // 命中路径：shared_lock 并发读
    {
        std::shared_lock lock(m_randomFactoryMutex);
        auto it = m_randomFactoryCache.find(name);
        if (it != m_randomFactoryCache.end()) {
            return *it->second;
        }
    }

    // miss 路径：unique_lock 写入（double-check）
    std::unique_lock lock(m_randomFactoryMutex);
    auto it = m_randomFactoryCache.find(name);
    if (it != m_randomFactoryCache.end()) {
        return *it->second;
    }

    // MC 1.21: RandomState.getOrCreateRandomFactory()
    auto rng = m_positionalRandom->fromHashOf(name);
    auto factory = std::make_unique<::mc::math::PositionalRandomFactory>(rng->forkPositional());

    auto& ref = *factory;
    m_randomFactoryCache.emplace(name, std::move(factory));
    return ref;
}

} // namespace mc::world::gen
