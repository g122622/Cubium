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
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/noise/PerlinNoise.hpp"
#include <memory>

namespace mc::world::gen::density {

/**
 * @brief 混合噪声密度函数（MC 1.18+ 旧式三层 Perlin 噪声）
 *
 * 参考 MC 1.21.11: BlendedNoise.java
 * 使用三个 PerlinNoise 实例（minLimitNoise、maxLimitNoise、mainNoise）
 * 计算密度值，模拟 MC 1.16.5 时代的三层噪声地形生成。
 *
 * 在 MC 1.21 中注册为密度函数 "old_blended_noise"，
 * 用于所有三个维度的 BASE_3D_NOISE：
 * - 主世界: BlendedNoise(0.25, 0.125, 80.0, 160.0, 8.0)
 * - 下界:   BlendedNoise(0.25, 0.375, 80.0, 60.0, 8.0)
 * - 末地:   BlendedNoise(0.25, 0.25, 80.0, 160.0, 4.0)
 *
 * 算法流程：
 * 1. 计算 mainNoise 的 8 倍频值，得到插值因子 d16 = (result/10 + 1) / 2（不做预裁剪，由 clampedLerp 处理边界）
 * 2. 如果 d16 >= 1：只采样 minLimitNoise（16 倍频）
 * 3. 如果 d16 <= 0：只采样 maxLimitNoise（16 倍频）
 * 4. 否则：对两者进行插值 clampedLerp(d16, minResult/512, maxResult/512)
 * 5. 最终结果除以 128
 */
class BlendedNoise final : public DensityFunction {
public:
    /**
     * @brief 使用种子和参数构造
     * @param seed 随机种子
     * @param xzScale XZ 方向缩放因子
     * @param yScale Y 方向缩放因子
     * @param xzFactor XZ 方向因子
     * @param yFactor Y 方向因子
     * @param smearScaleMultiplier Y 方向涂抹缩放乘数
     */
    BlendedNoise(u64 seed, f64 xzScale, f64 yScale, f64 xzFactor, f64 yFactor, f64 smearScaleMultiplier);

    ~BlendedNoise() override = default;

    BlendedNoise(const BlendedNoise&) = delete;
    BlendedNoise& operator=(const BlendedNoise&) = delete;
    BlendedNoise(BlendedNoise&&) noexcept = default;
    BlendedNoise& operator=(BlendedNoise&&) noexcept = default;

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;

    [[nodiscard]] f64 minValue() const override { return -m_maxValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        return visitor.apply(
            std::make_unique<BlendedNoise>(m_seed, m_xzScale, m_yScale, m_xzFactor, m_yFactor, m_smearScaleMultiplier));
    }

    /**
     * @brief 创建未种子化的 BlendedNoise（用于密度函数注册/序列化）
     */
    [[nodiscard]] static std::unique_ptr<BlendedNoise> createUnseeded(
        f64 xzScale, f64 yScale, f64 xzFactor, f64 yFactor, f64 smearScaleMultiplier);

    /**
     * @brief 内部构造函数，接受三个已初始化的 PerlinNoise
     */
    BlendedNoise(std::unique_ptr<noise::PerlinNoise> minLimitNoise,
        std::unique_ptr<noise::PerlinNoise> maxLimitNoise,
        std::unique_ptr<noise::PerlinNoise> mainNoise,
        f64 xzScale,
        f64 yScale,
        f64 xzFactor,
        f64 yFactor,
        f64 smearScaleMultiplier);

private:
    /**
     * @brief 初始化各倍频参数
     */
    void initFromNoises();

    std::unique_ptr<noise::PerlinNoise> m_minLimitNoise;
    std::unique_ptr<noise::PerlinNoise> m_maxLimitNoise;
    std::unique_ptr<noise::PerlinNoise> m_mainNoise;

    f64 m_xzScale;
    f64 m_yScale;
    f64 m_xzFactor;
    f64 m_yFactor;
    f64 m_smearScaleMultiplier;
    u64 m_seed = 0;

    f64 m_xzMultiplier = 0.0; ///< 684.412 * xzScale
    f64 m_yMultiplier = 0.0;  ///< 684.412 * yScale
    f64 m_maxValue = 0.0;
};

} // namespace mc::world::gen::density
