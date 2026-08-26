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
#include "common/world/gen/noise/PerlinNoise.hpp"
#include "common/world/gen/noise/PerlinSoA.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc::world::gen::noise {

/**
 * @brief 双 Perlin 噪声（MC 1.18+ 核心噪声）
 *
 * NormalNoise 使用两个独立的 PerlinNoise 实例，
 * 第二个的坐标乘以 INPUT_FACTOR ≈ 1.018，取平均值后乘以 valueFactor。
 * 这是 MC 1.18+ 地形生成和气候参数采样的基础噪声。
 *
 * NormalNoise 使用双 PerlinNoise 实现，具有以下特性：
 * - 支持任意倍频振幅列表
 * - 使用 INPUT_FACTOR 偏移第二个噪声避免相关性
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
     *
     * @note 内部创建 Random 并通过 forkPositional() 派生两个 PerlinNoise 实例，
     *       与 MC 1.21 的 NormalNoise 构造流程一致。
     */
    NormalNoise(u64 seed, i32 firstOctave, std::vector<f64> amplitudes);

    /**
     * @brief 使用 RandomSource 构造
     * @param rng 随机数生成器引用（构造后状态会被推进）
     * @param firstOctave 首个倍频索引
     * @param amplitudes 倍频振幅列表
     *
     * @note 与 MC 1.21 的 NormalNoise.create(RandomSource, NoiseParameters) 一致。
     *       两次调用 rng.forkPositional() 为两个 PerlinNoise 创建不同的工厂。
     */
    NormalNoise(math::Random& rng, i32 firstOctave, std::vector<f64> amplitudes);

    ~NormalNoise() = default;

    NormalNoise(const NormalNoise&) = delete;
    NormalNoise& operator=(const NormalNoise&) = delete;
    NormalNoise(NormalNoise&&) noexcept = default;
    NormalNoise& operator=(NormalNoise&&) noexcept = default;

    /**
     * @brief 克隆噪声实例
     *
     * 创建一个具有相同配置的新 NormalNoise 实例。
     * 用于密度函数的 mapAll 克隆操作。
     */
    [[nodiscard]] std::unique_ptr<NormalNoise> clone() const;

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

    /**
     * @brief 构造时使用的种子
     *
     * 仅当通过种子构造时有效；通过 Random& 构造时为 nullopt。
     */
    [[nodiscard]] const std::optional<u64>& seed() const { return m_seed; }

    /**
     * @brief 计算期望标准差，用于归一化
     * @param octaveRange 非零倍频的范围
     *
     * MC 1.21.11: NormalNoise.expectedDeviation(int)
     * 公式: 0.1 * (1.0 + 1.0 / (octaveRange + 1))
     */
    [[nodiscard]] static f64 expectedDeviation(i32 octaveRange);

private:
    /**
     * @brief 计算 valueFactor 和 maxValue（两个构造函数共用）
     */
    void computeValueFactor();

    /**
     * @brief 从 m_first/m_second 收集非零 octave 子层为 SoA 数组
     *
     * 构造期一次性拍平：遍历 PerlinNoise::layers()，对每个非空层拷贝排列表 + 偏移 +
     * 振幅 + 预算 inputFactor/valueFactor。运行期 getValue 走 SoA 单循环求值，
     * 消除逐层 PerlinLayer 对象的虚分发与 cache miss。
     */
    void buildSoA();

    static constexpr f64 INPUT_FACTOR = 1.0181268882175227;
    static constexpr f64 VALUE_FACTOR_BASE = 1.0 / 6.0;

    std::optional<u64> m_seed; ///< nullopt = 通过 Random& 构造（无法克隆）
    i32 m_firstOctave;
    std::vector<f64> m_amplitudes;
    std::unique_ptr<PerlinNoise> m_first;
    std::unique_ptr<PerlinNoise> m_second;
    f64 m_valueFactor = 0.0;
    f64 m_maxValue = 0.0;

    /// first/second 采样器的非零 octave 拍平 SoA。getValue 遍历这两个数组求值，
    /// 累加顺序与原 (m_first->getValue() + m_second->getValue()) 一致（bit-exact）。
    std::vector<PerlinSoALayer> m_firstSoA;
    std::vector<PerlinSoALayer> m_secondSoA;
};

} // namespace mc::world::gen::noise
