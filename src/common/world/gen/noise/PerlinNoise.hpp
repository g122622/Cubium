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

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::noise {

/**
 * @brief MC 1.18+ 多倍频 Perlin 噪声
 *
 * 与旧版 OctavesNoiseGenerator 不同，PerlinNoise 支持：
 * - 任意 firstOctave（负数表示更低频率）
 * - 振幅列表（每个倍频可独立设置振幅）
 * - 坐标环绕（防止大坐标精度丢失）
 *
 * 用于 NormalNoise 内部和 DensityFunction 的噪声采样。
 */
class PerlinNoise {
public:
    /**
     * @brief 噪声参数
     * @param firstOctave 首个倍频索引（负数 = 更低频率）
     * @param amplitudes 每个倍频的振幅列表
     */
    struct NoiseParameters {
        i32 firstOctave;
        std::vector<f64> amplitudes;
    };

    /**
     * @brief 使用种子和参数构造
     * @param seed 随机种子
     * @param firstOctave 首个倍频索引
     * @param amplitudes 倍频振幅列表
     */
    PerlinNoise(u64 seed, i32 firstOctave, std::vector<f64> amplitudes);

    ~PerlinNoise() = default;

    PerlinNoise(const PerlinNoise&) = delete;
    PerlinNoise& operator=(const PerlinNoise&) = delete;
    PerlinNoise(PerlinNoise&&) noexcept = default;
    PerlinNoise& operator=(PerlinNoise&&) noexcept = default;

    /**
     * @brief 采样 3D 噪声值
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值
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
     * @brief 坐标环绕，防止大坐标精度丢失
     * MC 使用 2^25 = 33554432.0 作为环绕周期
     */
    [[nodiscard]] static f64 wrap(f64 value);

    /**
     * @brief 根据最大单倍频输出值计算总最大值
     */
    [[nodiscard]] f64 edgeValue(f64 maxInputValue) const;

    /**
     * @brief 简化版 Perlin 噪声核心（用于每个倍频层）
     */
    class PerlinLayer {
    public:
        explicit PerlinLayer(math::Random& rng);

        [[nodiscard]] f64 noise(f64 x, f64 y, f64 z) const;

    private:
        [[nodiscard]] f64 sampleAndLerp(i32 cellX, i32 cellY, i32 cellZ,
            f64 deltaX, f64 deltaY, f64 deltaZ) const;

        [[nodiscard]] i32 p(i32 index) const;

        [[nodiscard]] static f64 gradDot(i32 hash, f64 x, f64 y, f64 z);

        std::vector<u8> m_permutation;
        mutable std::vector<u8> m_p;
        f64 m_xOffset = 0.0;
        f64 m_yOffset = 0.0;
        f64 m_zOffset = 0.0;
    };

    i32 m_firstOctave;
    std::vector<f64> m_amplitudes;
    std::vector<std::unique_ptr<PerlinLayer>> m_layers;
    f64 m_lowestFreqInputFactor = 0.0;
    f64 m_lowestFreqValueFactor = 0.0;
    f64 m_maxValue = 0.0;
};

} // namespace mc::world::gen::noise
