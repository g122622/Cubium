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

#pragma once

#include "NoiseSettings.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "common/world/block/Block.hpp"
#include <vector>

// 前向声明
namespace mc {
class BlockState;
}

namespace mc {

/**
 * @brief 维度类型标识
 *
 * 明确标识维度类型，用于选择正确的 NoiseRouter 和 SurfaceRules。
 * 用于选择正确的 NoiseRouter 和 SurfaceRules。
 */
enum class DimensionKind : u8 {
    Overworld,       ///< 主世界
    LargeBiomes,     ///< 主世界（大型生物群系）
    Amplified,       ///< 主世界（放大化）
    Nether,          ///< 下界
    End,             ///< 末地
    Caves,           ///< 洞穴预设
    FloatingIslands, ///< 浮岛预设
    Flat             ///< 超平坦
};

/**
 * @brief 维度生成设置
 *
 * 包含维度级别的生成配置（对齐 MC 1.21.11 NoiseGeneratorSettings）。
 * 使用 BlockState* 替代固定 BlockId，支持动态方块注册。
 */
struct DimensionSettings {
    NoiseSettings noise;
    const BlockState* defaultBlock = nullptr; ///< 默认方块（石头等）
    const BlockState* defaultFluid = nullptr; ///< 默认流体（水/熔岩）
    i32 seaLevel = world::SEA_LEVEL;
    DimensionKind dimensionKind = DimensionKind::Overworld; ///< 维度类型标识
    bool largeBiomes = false;                               ///< 是否使用大型生物群系预设
    bool oreVeinsEnabled = true;                            ///< 是否启用矿脉生成（主世界=true，下界/末地=false）
    bool disableMobGeneration = false;                      ///< 是否禁用生物生成（末地=true）

    /**
     * @brief 出生点气候目标参数列表
     *
     * MC 1.21.11: NoiseGeneratorSettings.spawnTarget
     * 用于 Climate.Sampler.findSpawnPosition() 在气候空间中径向搜索最佳出生点。
     *
     * - 主世界 / 大型生物群系 / 放大化：由 OverworldBiomeBuilder.spawnTarget() 提供
     *   （2 个 ParameterPoint，depth=0，weirdness 以 ±0.16 分割）
     * - 下界 / 末地 / 洞穴 / 浮岛 / 超平坦：空列表（沿用 (0,0) 区块作为出生点）
     *
     * 在 NoiseChunk.cachedClimateSampler 中传给 Climate::Sampler，
     * 同时由 RandomState::create 时设置到 m_sampler 上以供出生点查找使用。
     */
    std::vector<world::biome::climate::ParameterPoint> spawnTarget;

    // === 预设 ===

    /**
     * @brief 主世界设置
     */
    static DimensionSettings overworld() noexcept;

    /**
     * @brief 大型生物群系设置
     */
    static DimensionSettings largeBiomesPreset() noexcept;

    /**
     * @brief 放大化设置
     */
    static DimensionSettings amplified() noexcept;

    /**
     * @brief 下界设置
     */
    static DimensionSettings nether() noexcept;

    /**
     * @brief 末地设置
     */
    static DimensionSettings end() noexcept;

    /**
     * @brief 洞穴预设设置
     */
    static DimensionSettings caves() noexcept;

    /**
     * @brief 浮岛预设设置
     */
    static DimensionSettings floatingIslands() noexcept;

    /**
     * @brief 平坦世界设置（占位用，FlatChunkGenerator 不使用噪声生成）
     */
    static DimensionSettings flat() noexcept;
};

} // namespace mc
