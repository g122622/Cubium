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

#pragma once

#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/biome/climate/Climate.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include <memory>
#include <string>

namespace mc::world::gen::surface {
class SurfaceSystem;
class SurfaceRule;
} // namespace mc::world::gen::surface

namespace mc::world::gen::density {

/**
 * @brief MC 1.21 RandomState — 统一的随机源管理器
 *
 * 从世界种子创建，持有所有噪声生成和表面系统所需的状态：
 * - NoiseRouter（密度函数管线）
 * - Climate.Sampler（气候参数采样器）
 * - SurfaceSystem（表面规则系统）
 * - PositionalRandomFactory（含水层、矿石等的位置随机源）
 *
 * 对应 MC Java 版的 RandomState / NoiseBasedChunkGenerator 随机源管理。
 * 将原本分散在 NoiseChunkGenerator 中的噪声初始化逻辑统一到此类。
 */
class RandomState {
public:
    /**
     * @brief 构造 RandomState
     * @param seed 世界种子
     * @param router 已初始化的噪声路由器
     * @param surfaceRule 表面规则（可能为 nullptr，如下界/末地）
     * @param defaultBlock 默认方块（石头/下界岩/末地石）
     * @param defaultFluid 默认流体（水/熔岩/空气）
     * @param seaLevel 海平面高度
     * @param noiseMinY 噪声最小 Y
     * @param noiseHeight 噪声高度
     */
    RandomState(u64 seed,
        NoiseRouter router,
        std::unique_ptr<surface::SurfaceRule> surfaceRule,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        i32 noiseMinY,
        i32 noiseHeight);

    ~RandomState();

    RandomState(RandomState&&) noexcept;
    RandomState& operator=(RandomState&&) noexcept;

    RandomState(const RandomState&) = delete;
    RandomState& operator=(const RandomState&) = delete;

    // ========== 核心访问器 ==========

    /** 噪声路由器 */
    [[nodiscard]] const NoiseRouter& router() const { return m_router; }

    /** 气候采样器 */
    [[nodiscard]] const biome::climate::Sampler& climateSampler() const { return m_climateSampler; }

    /** 表面系统（可能为 nullptr） */
    [[nodiscard]] const surface::SurfaceSystem* surfaceSystem() const { return m_surfaceSystem.get(); }

    // ========== 位置随机工厂 ==========

    /** 含水层随机工厂 */
    [[nodiscard]] const math::PositionalRandomFactory& aquiferRandom() const { return m_aquiferRandom; }

    /** 矿脉随机工厂 */
    [[nodiscard]] const math::PositionalRandomFactory& oreRandom() const { return m_oreRandom; }

    /** 通用位置随机工厂 */
    [[nodiscard]] const math::PositionalRandomFactory& positionalRandom() const { return m_positionalRandom; }

    // ========== 噪声缓存 ==========

    /** 世界种子 */
    [[nodiscard]] u64 seed() const { return m_seed; }

    /** 默认方块 */
    [[nodiscard]] const BlockState* defaultBlock() const { return m_defaultBlock; }

    /** 默认流体 */
    [[nodiscard]] const BlockState* defaultFluid() const { return m_defaultFluid; }

    /** 海平面高度 */
    [[nodiscard]] i32 seaLevel() const { return m_seaLevel; }

    /** 噪声最小 Y */
    [[nodiscard]] i32 noiseMinY() const { return m_noiseMinY; }

    /** 噪声高度 */
    [[nodiscard]] i32 noiseHeight() const { return m_noiseHeight; }

private:
    /** 从世界种子创建位置随机工厂 */
    static math::PositionalRandomFactory _createPositionalRandom(u64 seed);

    u64 m_seed;
    NoiseRouter m_router;
    biome::climate::Sampler m_climateSampler;
    std::unique_ptr<surface::SurfaceSystem> m_surfaceSystem;

    math::PositionalRandomFactory m_positionalRandom;
    math::PositionalRandomFactory m_aquiferRandom;
    math::PositionalRandomFactory m_oreRandom;

    const BlockState* m_defaultBlock;
    const BlockState* m_defaultFluid;
    i32 m_seaLevel;
    i32 m_noiseMinY;
    i32 m_noiseHeight;
};

} // namespace mc::world::gen::density
