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

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "world/gen/noise/INoiseGenerator.hpp"
#include <array>

namespace mc {

/**
 * @brief 改进的 Perlin 噪声生成器
 *
 * 实现标准的 3D Perlin 噪声，与 MC 1.21.11 ImprovedNoise 对齐。
 * 所有内部计算使用 f64 精度，与 MC 的 double 精度一致，
 * 避免大坐标时精度丢失。
 *
 * 使用方法：
 * @code
 * ImprovedNoiseGenerator noise(seed);
 * f64 value = noise.noise(x, y, z);
 * @endcode
 *
 * @note 噪声值范围约为 [-1, 1]
 */
class ImprovedNoiseGenerator : public INoiseGenerator {
public:
    /**
     * @brief 使用种子构造噪声生成器
     * @param seed 随机种子
     */
    explicit ImprovedNoiseGenerator(u64 seed);

    /**
     * @brief 使用随机生成器构造
     * @param rng 随机数生成器接口
     */
    explicit ImprovedNoiseGenerator(math::IRandom& rng);

    ~ImprovedNoiseGenerator() override = default;

    /**
     * @brief 采样 3D 噪声值（f32 接口，兼容 INoiseGenerator）
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] f32 noise(f32 x, f32 y, f32 z) const override;

    /**
     * @brief 采样 3D 噪声值（f64 精度版本，与 MC ImprovedNoise 对齐）
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] f64 noise(f64 x, f64 y, f64 z) const;

    /**
     * @brief 采样 3D 噪声值（带 Y 轴缩放，f64 版本）
     *
     * 参考 MC 1.21.11: ImprovedNoise.noise(x, y, z, yScale, maxY)
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param yScale Y 轴缩放因子
     * @param maxY Y 轴边界
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] f64 noise(f64 x, f64 y, f64 z, f64 yScale, f64 maxY) const;

    /**
     * @brief 采样噪声值并计算导数
     *
     * 参考 MC 1.21.11: ImprovedNoise.noiseWithDerivative
     * 用于密度函数导数计算。
     *
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param derivatives 输出导数数组 [dx/dx, dy/dy, dz/dz]
     * @return 噪声值 [-1, 1]
     */
    [[nodiscard]] f64 noiseWithDerivative(f64 x, f64 y, f64 z, f64 derivatives[3]) const;

    // 坐标偏移（f64 精度，与 MC 对齐）
    [[nodiscard]] f64 xOffset() const noexcept { return m_xOffset; }
    [[nodiscard]] f64 yOffset() const noexcept { return m_yOffset; }
    [[nodiscard]] f64 zOffset() const noexcept { return m_zOffset; }

private:
    // 排列表（256 字节，复制一份用于快速查找）
    std::array<u8, 256> m_permutation;
    // 工作数组（用于避免每次调用时的临时分配）
    mutable std::array<u8, 512> m_p;

    // 坐标偏移（f64 精度，与 MC ImprovedNoise.xo/yo/zo 对齐）
    f64 m_xOffset = 0.0;
    f64 m_yOffset = 0.0;
    f64 m_zOffset = 0.0;

    /**
     * @brief 初始化排列数组
     */
    void _initPermutation(math::IRandom& rng);

    /**
     * @brief 获取排列值
     */
    [[nodiscard]] u8 _getPermut(i32 index) const noexcept { return m_p[static_cast<size_t>(index & 255)]; }

    /**
     * @brief 梯度点积（f64 精度）
     */
    [[nodiscard]] static f64 gradDot(i32 hash, f64 x, f64 y, f64 z) noexcept;

    /**
     * @brief 平滑插值（Perlin 的 fade 函数，f64 版本）
     */
    [[nodiscard]] static f64 fade(f64 t) noexcept { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

    /**
     * @brief 平滑插值的导数
     */
    [[nodiscard]] static f64 fadeDerivative(f64 t) noexcept { return 30.0 * t * t * (t - 1.0) * (t - 1.0); }

    /**
     * @brief 线性插值（f64 版本）
     */
    [[nodiscard]] static f64 lerp(f64 a, f64 b, f64 t) noexcept { return a + t * (b - a); }

    /**
     * @brief 采样并三线性插值
     */
    [[nodiscard]] f64 sampleAndLerp(i32 cellX, i32 cellY, i32 cellZ, f64 fracX, f64 fracY, f64 fracZ) const noexcept;

    /**
     * @brief 采样并计算导数
     */
    [[nodiscard]] f64 sampleWithDerivative(
        i32 cellX, i32 cellY, i32 cellZ, f64 fracX, f64 fracY, f64 fracZ, f64 derivatives[3]) const noexcept;
};

// ============================================================================
// 梯度向量表（f64 精度，与 MC SimplexNoise.GRADIENT 共享）
// ============================================================================

/**
 * @brief Perlin 噪声梯度向量
 *
 * 格式: {x, y, z}
 * 参考 MC 1.21.11: SimplexNoise.GRADIENT
 */
constexpr f64 PERLIN_GRADIENTS_F64[16][3] = {{1.0, 1.0, 0.0},
    {-1.0, 1.0, 0.0},
    {1.0, -1.0, 0.0},
    {-1.0, -1.0, 0.0},
    {1.0, 0.0, 1.0},
    {-1.0, 0.0, 1.0},
    {1.0, 0.0, -1.0},
    {-1.0, 0.0, -1.0},
    {0.0, 1.0, 1.0},
    {0.0, -1.0, 1.0},
    {0.0, 1.0, -1.0},
    {0.0, -1.0, -1.0},
    {1.0, 1.0, 0.0},
    {0.0, -1.0, 1.0},
    {-1.0, 1.0, 0.0},
    {0.0, -1.0, -1.0}};

/**
 * @brief 保留旧版 f32 梯度表以兼容
 */
constexpr f32 PERLIN_GRADIENTS[16][3] = {{1.0f, 1.0f, 0.0f},
    {-1.0f, 1.0f, 0.0f},
    {1.0f, -1.0f, 0.0f},
    {-1.0f, -1.0f, 0.0f},
    {1.0f, 0.0f, 1.0f},
    {-1.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, -1.0f},
    {-1.0f, 0.0f, -1.0f},
    {0.0f, 1.0f, 1.0f},
    {0.0f, -1.0f, 1.0f},
    {0.0f, 1.0f, -1.0f},
    {0.0f, -1.0f, -1.0f},
    {1.0f, 1.0f, 0.0f},
    {0.0f, -1.0f, 1.0f},
    {-1.0f, 1.0f, 0.0f},
    {0.0f, -1.0f, -1.0f}};

} // namespace mc
