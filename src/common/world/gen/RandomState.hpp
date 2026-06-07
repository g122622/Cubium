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
#include "common/world/biome/climate/Climate.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/surface/SurfaceRules.hpp"
#include <memory>

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
};

} // namespace mc::world::gen
