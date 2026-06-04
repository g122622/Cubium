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

#include "common/world/gen/noise/PerlinNoise.hpp"
#include <memory>

namespace mc::world::gen::noise {

/**
 * @brief 双 Perlin 噪声（MC 1.18+ 核心噪声）
 *
 * NormalNoise 使用两个独立的 PerlinNoise 实例，
 * 第二个的坐标乘以 INPUT_FACTOR ≈ 1.018，取平均值后乘以 valueFactor。
 * 这是 MC 1.18+ 地形生成和气候参数采样的基础噪声。
 *
 * 与旧版 OctavesNoiseGenerator 的区别：
 * - 支持任意倍频振幅列表
 - 使用 INPUT_FACTOR 偏移第二个噪声避免相关性
 * - 归一化系数保证输出范围
 */
class NormalNoise {
public:
    /**
     * @brief 噪声参数（与 PerlinNoise::NoiseParameters 兼容）
     */
    using NoiseParameters = PerlinNoise::NoiseParameters;

    /**
     * @brief 使用种子和参数构造
     * @param seed 随机种子
     * @param firstOctave 首个倍频索引
     * @param amplitudes 倍频振幅列表
     */
    NormalNoise(u64 seed, i32 firstOctave, std::vector<f64> amplitudes);

    ~NormalNoise() = default;

    NormalNoise(const NormalNoise&) = delete;
    NormalNoise& operator=(const NormalNoise&) = delete;
    NormalNoise(NormalNoise&&) noexcept = default;
    NormalNoise& operator=(NormalNoise&&) noexcept = default;

    /**
     * @brief 采样 3D 噪声值
     *
     * 两个 Perlin 噪声采样取平均值，第二个使用缩放坐标。
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值，范围约 [-maxValue, maxValue]
     */
    [[nodiscard]] f64 getValue(f64 x, f64 y, f64 z) const;

    /**
     * @brief 噪声最大可能输出值
     */
    [[nodiscard]] f64 maxValue() const { return m_maxValue; }

    /**
     * @brief 首个倍频索引
     */
    [[nodiscard]] i32 firstOctave() const { return m_firstOctave; }

    /**
     * @brief 振幅列表
     */
    [[nodiscard]] const std::vector<f64>& amplitudes() const { return m_amplitudes; }

private:
    /**
     * @brief 计算期望标准差，用于归一化
     * @param octaveRange 非零倍频的范围
     */
    [[nodiscard]] static f64 expectedDeviation(i32 octaveRange);

    static constexpr f64 INPUT_FACTOR = 1.0181268882175227;
    static constexpr f64 VALUE_FACTOR_BASE = 1.0 / 6.0;

    i32 m_firstOctave;
    std::vector<f64> m_amplitudes;
    PerlinNoise m_first;
    PerlinNoise m_second;
    f64 m_valueFactor = 0.0;
    f64 m_maxValue = 0.0;
};

} // namespace mc::world::gen::noise
