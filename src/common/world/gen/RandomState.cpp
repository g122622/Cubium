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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include "common/world/gen/density/NoiseBindingVisitor.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/world/gen/noise/Noises.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/gen/settings/NoiseSettingsRegistry.hpp"

#include <fmt/format.h>

namespace mc::world::gen {

RandomState::RandomState(u64 worldSeed, const DimensionSettings& settings)
    : m_worldSeed(worldSeed)
    , m_settings(settings)
{}

namespace {

/// 从 m_routerDfs 模板（15 槽 UnboundNoiseLeaf 占位）经 NoiseBindingVisitor 绑定后构造 NoiseRouter。
/// mapAll 对每个槽 DF 递归深拷贝 + 替换占位（一次完成深拷贝 + 绑定），每区块得到独立绑定树。
/// const RandomState&：create（非 const）与 createRouterCopy（const）共用此路径；NoiseBindingVisitor
/// 仅调用 getOrCreateNoiseShared（const，mutable 噪声缓存）+ positionalRandom() const 重载。
density::NoiseRouter buildRouterFromTemplate(const DimensionSettings& settings, const RandomState& rs, u64 worldSeed)
{
    MC_ASSERT_MSG(settings.m_noiseSettingsId.isValid(),
        "RandomState::create: DimensionSettings has no noise_settings id (flat worlds must not reach "
        "NoiseChunkGenerator path)");

    density::NoiseBindingVisitor visitor(rs, worldSeed);
    auto barrier = settings.m_routerDfs[static_cast<size_t>(RouterSlot::Barrier)]->mapAll(visitor);
    auto fluidLevelFloodedness =
        settings.m_routerDfs[static_cast<size_t>(RouterSlot::FluidLevelFloodedness)]->mapAll(visitor);
    auto fluidLevelSpread = settings.m_routerDfs[static_cast<size_t>(RouterSlot::FluidLevelSpread)]->mapAll(visitor);
    auto lava = settings.m_routerDfs[static_cast<size_t>(RouterSlot::Lava)]->mapAll(visitor);
    auto temperature = settings.m_routerDfs[static_cast<size_t>(RouterSlot::Temperature)]->mapAll(visitor);
    auto vegetation = settings.m_routerDfs[static_cast<size_t>(RouterSlot::Vegetation)]->mapAll(visitor);
    auto continents = settings.m_routerDfs[static_cast<size_t>(RouterSlot::Continents)]->mapAll(visitor);
    auto erosion = settings.m_routerDfs[static_cast<size_t>(RouterSlot::Erosion)]->mapAll(visitor);
    auto depth = settings.m_routerDfs[static_cast<size_t>(RouterSlot::Depth)]->mapAll(visitor);
    auto ridges = settings.m_routerDfs[static_cast<size_t>(RouterSlot::Ridges)]->mapAll(visitor);
    auto preliminarySurfaceLevel =
        settings.m_routerDfs[static_cast<size_t>(RouterSlot::PreliminarySurfaceLevel)]->mapAll(visitor);
    auto finalDensity = settings.m_routerDfs[static_cast<size_t>(RouterSlot::FinalDensity)]->mapAll(visitor);
    auto veinToggle = settings.m_routerDfs[static_cast<size_t>(RouterSlot::VeinToggle)]->mapAll(visitor);
    auto veinRidged = settings.m_routerDfs[static_cast<size_t>(RouterSlot::VeinRidged)]->mapAll(visitor);
    auto veinGap = settings.m_routerDfs[static_cast<size_t>(RouterSlot::VeinGap)]->mapAll(visitor);

    return density::NoiseRouter(std::move(barrier),
        std::move(fluidLevelFloodedness),
        std::move(fluidLevelSpread),
        std::move(lava),
        std::move(temperature),
        std::move(vegetation),
        std::move(continents),
        std::move(erosion),
        std::move(depth),
        std::move(ridges),
        std::move(preliminarySurfaceLevel),
        std::move(finalDensity),
        std::move(veinToggle),
        std::move(veinRidged),
        std::move(veinGap));
}

} // namespace

std::unique_ptr<RandomState> RandomState::create(const DimensionSettings& settings, u64 worldSeed)
{
    // 数据驱动唯一路径：从 NoiseSettingsRegistry 查完整 DimensionSettings（含 m_routerDfs 模板）。
    // 传入的 settings 可能是 C++ 预设（仅带 m_noiseSettingsId）或已加载模板；统一用 registry 解析到的版本。
    const DimensionSettings* resolved = nullptr;
    if (settings.m_noiseSettingsId.isValid()) {
        resolved = settings::NoiseSettingsRegistry::instance().get(settings.m_noiseSettingsId);
    }
    // resolved 为空 = registry 未加载该 id（数据包缺失或未接入）→ 断言，数据驱动为唯一路径无兜底。
    MC_ASSERT_RELEASE_MSG(resolved != nullptr,
        fmt::format("RandomState::create: noise_settings '{}' not in NoiseSettingsRegistry (datapacks not loaded?)",
            settings.m_noiseSettingsId.toString())
            .c_str());

    auto state = std::unique_ptr<RandomState>(new RandomState(worldSeed, *resolved));

    // 1. PositionalRandomFactory（必须在 NoiseRouter 之前初始化：buildRouterFromTemplate 经
    //    NoiseBindingVisitor 绑定 old_blended_noise 叶子时调 positionalRandom().fromHashOf("terrain") 派生种子）。
    ::mc::math::Xoroshiro128ppRandom mainRng(worldSeed);
    state->m_positionalRandom = std::make_unique<::mc::math::PositionalRandomFactory>(mainRng.forkPositional());

    // 2. NoiseRouter：m_routerDfs 模板经 NoiseBindingVisitor mapAll 绑定（深拷贝 + 占位替换）。
    state->m_router = std::make_unique<density::NoiseRouter>(buildRouterFromTemplate(*resolved, *state, worldSeed));

    // 3. Climate.Sampler：从绑定后的 NoiseRouter 6 个气候函数构造，并设置 spawnTarget。
    state->m_sampler = std::make_unique<biome::climate::Sampler>(state->m_router->createClimateSampler());
    state->m_sampler->setSpawnTarget(resolved->spawnTarget);

    // 4. SurfaceSystem：surface_rule 必须由 noise_settings JSON 提供（数据驱动唯一路径，无兜底）。
    //    原版 7 个 noise_settings JSON 均含 surface_rule；flat 不经 create（无此断言风险）。
    //    数据包残缺缺 surface_rule 属数据错误，断言合理。
    MC_ASSERT_RELEASE_MSG(resolved->m_surfaceRule,
        fmt::format("RandomState::create: noise_settings '{}' has no surface_rule (datadriven requires all "
                    "noise_settings to provide surface_rule)",
            resolved->m_noiseSettingsId.toString())
            .c_str());
    std::shared_ptr<surface::SurfaceRule> surfaceRule = resolved->m_surfaceRule;

    ::mc::math::Xoroshiro128ppRandom surfaceRng(worldSeed);
    auto surfacePositionalRandom = surfaceRng.forkPositional();
    state->m_surfaceSystem = std::make_unique<surface::SurfaceSystem>(surfaceRule,
        resolved->defaultBlock,
        resolved->defaultFluid,
        resolved->seaLevel,
        resolved->noise.minY,
        resolved->noise.height,
        *state,
        surfacePositionalRandom);

    // 5. 含水层 / 矿石脉随机工厂（name-hash 派生，依赖 m_positionalRandom）。
    {
        auto aquiferRng = state->m_positionalRandom->fromHashOf("minecraft:aquifer");
        state->m_aquiferRandom = std::make_unique<::mc::math::PositionalRandomFactory>(aquiferRng->forkPositional());
    }
    {
        auto oreRng = state->m_positionalRandom->fromHashOf("minecraft:ore");
        state->m_oreRandom = std::make_unique<::mc::math::PositionalRandomFactory>(oreRng->forkPositional());
    }

    return state;
}

density::NoiseRouter RandomState::createRouterCopy() const
{
    // 每区块需独立路由器副本：m_routerDfs 模板经 NoiseBindingVisitor mapAll 重新绑定（深拷贝 + 占位替换）。
    // 底层 NormalNoise 经 getOrCreateNoiseShared 缓存共享，不每区块重建 PerlinNoise 倍频置换表。
    return buildRouterFromTemplate(m_settings, *this, m_worldSeed);
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

    // 3. 创建 NormalNoise（shared_ptr 存储，供数据驱动噪声叶子共享所有权）
    auto noise = std::make_shared<noise::NormalNoise>(*rng, params.firstOctave, params.amplitudes);

    auto& ref = *noise;
    m_noiseCache.emplace(name, std::move(noise));
    return ref;
}

std::shared_ptr<const noise::NormalNoise> RandomState::getOrCreateNoiseShared(const std::string& name) const
{
    // 命中路径：shared_lock 并发读
    {
        std::shared_lock lock(m_noiseMutex);
        auto it = m_noiseCache.find(name);
        if (it != m_noiseCache.end()) {
            return it->second;
        }
    }

    // miss 路径：unique_lock 写入（double-check）
    std::unique_lock lock(m_noiseMutex);
    auto it = m_noiseCache.find(name);
    if (it != m_noiseCache.end()) {
        return it->second;
    }

    const noise::NoiseParameters& params = noise::Noises::get(name);
    auto rng = m_positionalRandom->fromHashOf(name);
    auto noise = std::make_shared<noise::NormalNoise>(*rng, params.firstOctave, params.amplitudes);
    m_noiseCache.emplace(name, noise);
    return noise;
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
