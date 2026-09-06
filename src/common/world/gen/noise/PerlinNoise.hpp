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
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/gen/noise/PerlinNoiseSoA.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc::world::gen::noise {

/**
 * @brief MC 1.18+ 多倍频 Perlin 噪声
 *
 * PerlinNoise 是 MC 1.18+ 的多倍频 Perlin 噪声实现，支持：
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
     *
     * @note 内部创建 PositionalRandomFactory 用于派生各倍频种子，
     *       与 MC 1.21 的 PerlinNoise.create(seed, firstOctave, amplitudes) 一致。
     */
    PerlinNoise(u64 seed, i32 firstOctave, std::vector<f64> amplitudes);

    /**
     * @brief 使用 PositionalRandomFactory 构造
     * @param factory 位置随机工厂
     * @param firstOctave 首个倍频索引
     * @param amplitudes 倍频振幅列表
     *
     * @note 使用 fromHashOf("octave_N") 为每个倍频创建独立随机源，
     *       与 MC 1.21 的 PerlinNoise(RandomSource, firstOctave, amplitudes) 一致。
     *       调用者应先调用 random.forkPositional() 获取工厂。
     */
    PerlinNoise(const math::PositionalRandomFactory& factory, i32 firstOctave, std::vector<f64> amplitudes);

    /**
     * @brief 旧版构造函数（共享 RandomSource，顺序消费随机数）
     * @param rng JavaLegacyRandom 共享随机数生成器（调用后状态会推进）
     * @param firstOctave 首个倍频索引
     * @param amplitudes 倍频振幅列表
     *
     * @note 对应 MC 1.21.11 的 PerlinNoise(RandomSource, Pair<Integer, DoubleList>, false) 构造路径，
     *       即 createLegacyForBlendedNoise 和 createLegacyForLegacyNetherBiome 使用的路径。
     *       旧版模式不使用 PositionalRandomFactory，而是从同一个 RandomSource 顺序消费随机数。
     *
     *       算法流程（MC PerlinNoise 构造函数 p_230517_==false 分支）：
     *       1. j = -firstOctave（octave 0 在振幅列表中的索引）
     *       2. 创建第一个 ImprovedNoise(rng)，如果 amplitudes[j]!=0 则放入 noiseLevels[j]
     *       3. 从 i1=j-1 向下到 0：
     *          - 如果 i1 < octaveCount 且 amplitudes[i1]!=0，创建 ImprovedNoise(rng)
     *          - 否则调用 skipOctave(rng) 即 rng.consumeCount(262)
     *       4. 验证：非空层数 == 非零振幅数，且 j >= octaveCount-1（禁止正倍频）
     *
     *       用于 BlendedNoise 和旧版下界生物群系噪声。
     */
    PerlinNoise(math::JavaLegacyRandom& rng, i32 firstOctave, std::vector<f64> amplitudes);

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
     * @brief 纯标量采样路径(调试/性能对比用)
     *
     * 遍历 m_layers 逐层调用 PerlinLayer::noise,不经 SoA 向量化。
     * 数值与 getValue bit-exact(两者都复刻原循环顺序)。
     */
    [[nodiscard]] f64 getValueScalar(f64 x, f64 y, f64 z) const;

    /**
     * @brief 采样带涂抹效果的 3D 噪声值
     *
     * 参考 MC 1.21.11: BlendedNoise 使用 ImprovedNoise.noise(x, y, z, yOffset, y)
     * 对每个倍频层应用 Y 轴涂抹，产生条纹状结构。
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param smearScaleMultiplier 涂抹缩放乘数（smearScaleMultiplier * yMultiplier / yFactor）
     * @return 噪声值
     */
    [[nodiscard]] f64 getValueWithSmear(f64 x, f64 y, f64 z, f64 smearScaleMultiplier) const;

    /**
     * @brief 噪声最大可能输出值
     */
    [[nodiscard]] f64 maxValue() const { return m_maxValue; }

    /**
     * @brief 计算给定 Y 输入范围下的最大可能输出值
     *
     * 参考 MC 1.21.11: PerlinNoise.maxBrokenValue(maxInputValue)
     * 用于 BlendedNoise 计算 maxValue。
     * 与 edgeValue 不同，此方法假设输入坐标完全在边缘上，
     * 因此每个倍频的最大值为 |amplitude| * maxInputValue。
     *
     * @param maxInputValue 最大输入值（对于 BlendedNoise 是 yMultiplier）
     * @return 最大可能输出值
     */
    [[nodiscard]] f64 maxBrokenValue(f64 maxInputValue) const;

    /**
     * @brief 坐标环绕，防止大坐标精度丢失
     * MC 使用 2^25 = 33554432.0 作为环绕周期
     */
    [[nodiscard]] static f64 wrap(f64 value);

    /**
     * @brief 首个倍频索引
     */
    [[nodiscard]] i32 firstOctave() const { return m_firstOctave; }

    /**
     * @brief 振幅列表
     */
    [[nodiscard]] const std::vector<f64>& amplitudes() const { return m_amplitudes; }

    /**
     * @brief Perlin 噪声核心层（对应 MC ImprovedNoise，每个倍频层使用一个实例）
     *
     * 参考 MC 1.21.11: PerlinNoise.PerlinNoiseLayer
     * 公开以支持 getOctaveNoise() 返回值和密度函数导数计算。
     */
    class PerlinLayer {
    public:
        explicit PerlinLayer(math::IRandom& rng);

        [[nodiscard]] f64 noise(f64 x, f64 y, f64 z) const;

        /**
         * @brief 带涂抹效果的 Perlin 噪声采样
         *
         * 参考 MC 1.21.11: ImprovedNoise.noise(x, y, z, yOffset, y)
         * 在 Y 轴方向应用"涂抹"效果：将 Y 分数吸附到 yOffset 间隔的网格线上，
         * 使 Y 方向出现条纹状结构，用于 BlendedNoise 的地形高度拉伸。
         *
         * @param x X 坐标
         * @param y Y 坐标
         * @param z Z 坐标
         * @param yOffset Y 轴涂抹间隔（0 表示不涂抹）
         * @param yFraction Y 方向原始分数坐标（用于 smoothstep 插值）
         * @return 噪声值
         */
        [[nodiscard]] f64 noiseWithSmear(f64 x, f64 y, f64 z, f64 yOffset, f64 yFraction) const;

        // ---- SoA 构造期访问器(buildSoA 从各 layer 收集置换表与偏移到连续块)----
        [[nodiscard]] const std::vector<u8>& permutation() const noexcept { return m_permutation; }
        [[nodiscard]] f64 xOffset() const noexcept { return m_xOffset; }
        [[nodiscard]] f64 yOffset() const noexcept { return m_yOffset; }
        [[nodiscard]] f64 zOffset() const noexcept { return m_zOffset; }

    private:
        [[nodiscard]] f64 sampleAndLerp(
            i32 cellX, i32 cellY, i32 cellZ, f64 deltaX, f64 deltaY, f64 deltaZ, f64 smoothstepY = -1.0) const;

        [[nodiscard]] i32 p(i32 index) const;

        [[nodiscard]] static f64 gradDot(i32 hash, f64 x, f64 y, f64 z);

        std::vector<u8> m_permutation;
        std::vector<u8> m_p;
        f64 m_xOffset = 0.0;
        f64 m_yOffset = 0.0;
        f64 m_zOffset = 0.0;
    };

    /**
     * @brief 获取指定倍频层的噪声生成器
     *
     * 参考 MC 1.21.11: PerlinNoise.getOctaveNoise(int)
     * 用于外部访问特定倍频层，例如密度函数导数计算。
     *
     * @param octave 倍频索引（相对于 firstOctave）
     * @return 倍频层指针，如果振幅为零或索引越界则返回 nullptr
     */
    [[nodiscard]] const PerlinLayer* getOctaveNoise(i32 octave) const
    {
        // MC 1.21.11: getOctaveNoise 使用反向索引
        // Java: this.noiseLevels[this.noiseLevels.length - 1 - octave]
        // octave=0 返回最高频率层，octave=1 返回次高频率层，依此类推
        const i32 index = static_cast<i32>(m_layers.size()) - 1 - octave;
        if (index < 0 || index >= static_cast<i32>(m_layers.size())) {
            return nullptr;
        }
        return m_layers[static_cast<size_t>(index)].get();
    }

    // ---- SoA 访问器(buildSoA 收集后,NormalNoise/BlendedNoise 的向量化求值读取)----
    [[nodiscard]] const std::vector<std::unique_ptr<PerlinLayer>>& layers() const noexcept { return m_layers; }
    [[nodiscard]] const std::vector<f64>& amplitudesVec() const noexcept { return m_amplitudes; }
    [[nodiscard]] f64 lowestFreqInputFactor() const noexcept { return m_lowestFreqInputFactor; }
    [[nodiscard]] f64 lowestFreqValueFactor() const noexcept { return m_lowestFreqValueFactor; }
    [[nodiscard]] const PerlinNoiseSoA& soa() const noexcept { return m_soa; }

    /**
     * @brief 把所有非空 octave 子层拍平到连续 SoA 块
     *
     * 构造期一次性调用。inputFactor 从 lowestFreqInputFactor 起每层 ×2,
     * valueFactor 从 lowestFreqValueFactor 起每层 ÷2(空层跳过采样但仍推进缩放序列,
     * 与 PerlinNoise::getValue 循环语义一致)。amplitude 取 m_amplitudes[i]。
     */
    void buildSoA();

private:
    /**
     * @brief 根据最大单倍频输出值计算总最大值
     */
    [[nodiscard]] f64 edgeValue(f64 maxInputValue) const;

    /**
     * @brief 初始化倍频层（PositionalRandomFactory 模式，新版本）
     * @param factory 位置随机工厂
     */
    void initLayers(const math::PositionalRandomFactory& factory);

    /**
     * @brief 初始化倍频层（旧版模式，共享 JavaLegacyRandom）
     * @param rng JavaLegacyRandom 共享随机数生成器
     *
     * 对应 MC 1.21.11 的 PerlinNoise(RandomSource, Pair<Integer, DoubleList>, false) 构造路径：
     * - j = -firstOctave（octave 0 在振幅列表中的索引）
     * - 创建第一个 ImprovedNoise(rng)，如果 amplitudes[j]!=0 则放入 noiseLevels[j]
     * - 从 i1=j-1 向下到 0：amplitudes[i1]!=0 则创建 ImprovedNoise(rng)，否则 consumeCount(262)
     * - 禁止正倍频（j < octaveCount-1 时抛出异常）
     */
    void initLayersLegacy(math::JavaLegacyRandom& rng);

    i32 m_firstOctave;
    std::vector<f64> m_amplitudes;
    std::vector<std::unique_ptr<PerlinLayer>> m_layers;
    f64 m_lowestFreqInputFactor = 0.0;
    f64 m_lowestFreqValueFactor = 0.0;
    f64 m_maxValue = 0.0;
    PerlinNoiseSoA m_soa; ///< 非空 octave 拍平的连续 SoA 块(buildSoA 填充)
};

} // namespace mc::world::gen::noise
