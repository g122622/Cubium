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
 * LIABILITY, WHETHER IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/biome/climate/Sampler.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/surface/SurfaceRules.hpp"
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::world::gen::noise {
class NormalNoise;
} // namespace mc::world::gen::noise

namespace mc::world::gen {

/**
 * @brief 世界生成随机状态
 *
 * MC 1.21: 对应 net.minecraft.world.level.levelgen.RandomState
 *
 * 集中持有世界生成所需的全部随机源和子系统，确保所有生成组件
 * 使用一致的种子和噪声参数。NoiseChunkGenerator 应只消费 RandomState
 * 中的组件，不自行拼装子系统。
 *
 * 生命周期：每个维度每个种子创建一个 RandomState 实例，
 * 在 NoiseChunkGenerator 构造时初始化，整个世界生成过程共享。
 */
class RandomState {
public:
    /**
     * @brief 从维度设置和世界种子创建 RandomState
     * @param settings 维度设置（包含 DimensionKind、NoiseSettings 等）
     * @param worldSeed 世界种子
     * @return 创建的 RandomState 实例
     */
    [[nodiscard]] static std::unique_ptr<RandomState> create(const DimensionSettings& settings, u64 worldSeed);

    // === 核心访问器 ===

    /** 噪声路由器（持有 15 个密度函数） */
    [[nodiscard]] density::NoiseRouter& router() { return *m_router; }
    [[nodiscard]] const density::NoiseRouter& router() const { return *m_router; }

    /**
     * @brief 创建噪声路由器的独立副本
     *
     * NoiseChunk 构造时需要拥有自己的路由器副本，
     * 以便 mapAll() 可以将 Marker 替换为区块特定实现。
     * 每个区块生成任务调用一次。
     */
    [[nodiscard]] density::NoiseRouter createRouterCopy() const;

    /** 气候采样器（6 个气候密度函数的封装） */
    [[nodiscard]] biome::climate::Sampler& sampler() { return *m_sampler; }
    [[nodiscard]] const biome::climate::Sampler& sampler() const { return *m_sampler; }

    /** 表面规则系统 */
    [[nodiscard]] surface::SurfaceSystem& surfaceSystem() { return *m_surfaceSystem; }
    [[nodiscard]] const surface::SurfaceSystem& surfaceSystem() const { return *m_surfaceSystem; }

    /** 含水层随机工厂 */
    [[nodiscard]] ::mc::math::PositionalRandomFactory& aquiferRandom() { return *m_aquiferRandom; }
    [[nodiscard]] const ::mc::math::PositionalRandomFactory& aquiferRandom() const { return *m_aquiferRandom; }

    /** 矿石脉随机工厂 */
    [[nodiscard]] ::mc::math::PositionalRandomFactory& oreRandom() { return *m_oreRandom; }
    [[nodiscard]] const ::mc::math::PositionalRandomFactory& oreRandom() const { return *m_oreRandom; }

    /** 通用位置随机工厂 */
    [[nodiscard]] ::mc::math::PositionalRandomFactory& positionalRandom() { return *m_positionalRandom; }
    [[nodiscard]] const ::mc::math::PositionalRandomFactory& positionalRandom() const { return *m_positionalRandom; }

    /** 维度设置 */
    [[nodiscard]] const DimensionSettings& settings() const { return m_settings; }

    /** 世界种子 */
    [[nodiscard]] u64 worldSeed() const { return m_worldSeed; }

    // === MC 1.21 噪声/随机工厂缓存 ===

    /**
     * @brief 获取或创建命名噪声实例
     *
     * MC 1.21: RandomState.getOrCreateNoise(Holder<NormalNoise.NoiseParameters>)
     * 首次调用时从 Noises 注册表取参数，用 fromHashOf(name).forkPositional() 创建 NormalNoise。
     * 后续调用返回缓存实例。
     *
     * @param name 噪声名称（如 "minecraft:surface"）
     * @return 噪声实例引用
     */
    [[nodiscard]] noise::NormalNoise& getOrCreateNoise(const std::string& name);

    /**
     * @brief 获取或创建命名位置随机工厂
     *
     * MC 1.21: RandomState.getOrCreateRandomFactory(Identifier)
     * 首次调用时用 fromHashOf(name).forkPositional() 创建，后续调用返回缓存。
     *
     * @param name 随机工厂名称（如 "minecraft:bedrock_floor"）
     * @return 位置随机工厂引用
     */
    [[nodiscard]] ::mc::math::PositionalRandomFactory& getOrCreateRandomFactory(const std::string& name);

    /**
     * @brief 获取或创建路由器内部噪声实例（按派生种子缓存）
     *
     * NoiseRouterData 构造密度函数树时，每个噪声用 derivedSeed（worldSeed ^ 常量）
     * 唯一标识。NormalNoise::getValue 是 const 且无 mutable 缓存，mapAll 前后语义等价，
     * 因此同一 derivedSeed 的 NormalNoise 可跨区块共享，避免每区块重建 PerlinNoise 倍频置换表。
     *
     * 返回 shared_ptr<const NormalNoise>：叶子密度函数（NoiseDensity/ShiftedNoise/MappedNoise）
     * 持有共享引用，mapAll 时共享而非 clone。
     *
     * @param derivedSeed 派生种子（worldSeed ^ 噪声常量）
     * @param firstOctave 首个倍频索引
     * @param amplitudes 倍频振幅列表
     * @return 共享的 NormalNoise 实例
     */
    [[nodiscard]] std::shared_ptr<const noise::NormalNoise> getOrCreateRouterNoise(
        u64 derivedSeed, i32 firstOctave, const std::vector<f64>& amplitudes) const;

private:
    RandomState(u64 worldSeed, const DimensionSettings& settings);

    u64 m_worldSeed;
    DimensionSettings m_settings;

    std::unique_ptr<density::NoiseRouter> m_router;
    std::unique_ptr<biome::climate::Sampler> m_sampler;
    std::unique_ptr<surface::SurfaceSystem> m_surfaceSystem;
    std::unique_ptr<::mc::math::PositionalRandomFactory> m_aquiferRandom;
    std::unique_ptr<::mc::math::PositionalRandomFactory> m_oreRandom;
    std::unique_ptr<::mc::math::PositionalRandomFactory> m_positionalRandom;

    // MC 1.21: 噪声实例缓存（name → NormalNoise）
    std::unordered_map<std::string, std::unique_ptr<noise::NormalNoise>> m_noiseCache;

    // MC 1.21: 位置随机工厂缓存（name → PositionalRandomFactory）
    std::unordered_map<std::string, std::unique_ptr<::mc::math::PositionalRandomFactory>> m_randomFactoryCache;

    // 路由器内部噪声缓存（derivedSeed → 共享 NormalNoise）
    // NormalNoise::getValue 是 const 无 mutable，可跨区块共享。每个 derivedSeed 唯一标识一个噪声，
    // 避免每区块 createRouterCopy 时重建 PerlinNoise 倍频置换表。
    mutable std::unordered_map<u64, std::shared_ptr<const noise::NormalNoise>> m_routerNoiseCache;
    mutable std::shared_mutex m_routerNoiseMutex;

    // 并发保护：buildSurface 在并行 worker 池上运行，多个区块会并发调用
    // getOrCreateNoise/getOrCreateRandomFactory。命中路径用 shared_lock（读），
    // miss 路径用 unique_lock（写）。SurfaceSystem 构造时预热了部分噪声名，
    // 但规则树引用的其余噪声名（SWAMP/PACKED_ICE/...）首次访问发生在 worker 线程。
    mutable std::shared_mutex m_noiseMutex;
    mutable std::shared_mutex m_randomFactoryMutex;
};

} // namespace mc::world::gen
