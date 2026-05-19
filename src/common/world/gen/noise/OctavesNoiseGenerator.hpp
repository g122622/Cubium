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

#include "../../../util/math/random/Random.hpp"
#include "INoiseGenerator.hpp"
#include "ImprovedNoiseGenerator.hpp"
#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class SimplexNoiseGenerator;

/**
 * @brief 多倍频噪声生成器
 *
 * 参考 MC OctavesNoiseGenerator，组合多个 Perlin 噪声层（倍频）。
 * 每个倍频层有不同的频率和振幅，叠加产生更自然的地形。
 *
 * 使用方法：
 * @code
 * // 创建 16 个倍频（从 -15 到 0）
 * OctavesNoiseGenerator noise(seed, -15, 0);
 * f32 value = noise.noise(x, y, z);
 * @endcode
 *
 * @note 参考 MC 1.16.5 的实现
 */
class OctavesNoiseGenerator : public INoiseGenerator {
public:
    /**
     * @brief 创建指定倍频范围的噪声生成器
     * @param seed 随机种子
     * @param minOctave 最小倍频索引（负数，表示低频）
     * @param maxOctave 最大倍频索引（通常是 0）
     */
    OctavesNoiseGenerator(u64 seed, i32 minOctave, i32 maxOctave);

    /**
     * @brief 使用随机生成器构造
     * @param rng 随机数生成器
     * @param minOctave 最小倍频索引
     * @param maxOctave 最大倍频索引
     */
    OctavesNoiseGenerator(math::IRandom& rng, i32 minOctave, i32 maxOctave);

    ~OctavesNoiseGenerator() override = default;

    // 禁止拷贝
    OctavesNoiseGenerator(const OctavesNoiseGenerator&) = delete;
    OctavesNoiseGenerator& operator=(const OctavesNoiseGenerator&) = delete;

    // 允许移动
    OctavesNoiseGenerator(OctavesNoiseGenerator&&) noexcept = default;
    OctavesNoiseGenerator& operator=(OctavesNoiseGenerator&&) noexcept = default;

    /**
     * @brief 采样 3D 噪声值
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 噪声值
     */
    [[nodiscard]] f32 noise(f32 x, f32 y, f32 z) const override;

    /**
     * @brief 采样 3D 噪声值（带额外参数）
     *
     * 参考 MC 的 getValue 方法
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param yScale Y 轴缩放
     * @param yBound Y 轴边界
     * @param fixY 是否固定 Y 轴
     * @return 噪声值
     */
    [[nodiscard]] f32 getValue(f32 x, f32 y, f32 z, f32 yScale, f32 yBound, bool fixY) const;

    /**
     * @brief 采样 2D 噪声值（简化版）
     *
     * 参考 MC 的 noiseAt 方法
     * @param x X 坐标
     * @param y Y 参数（在 MC 中用于高度权重）
     * @param z Z 坐标
     * @param scale 缩放因子
     * @return 噪声值
     */
    [[nodiscard]] f32 noiseAt(f32 x, f32 y, f32 z, f32 scale) const;

    /**
     * @brief 获取指定倍频层的噪声生成器
     * @param octave 倍频索引（从最大倍频开始，0 是最大）
     * @return 倍频层指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] ImprovedNoiseGenerator* getOctave(i32 octave);
    [[nodiscard]] const ImprovedNoiseGenerator* getOctave(i32 octave) const;

    /**
     * @brief 获取倍频数量
     */
    [[nodiscard]] i32 octaveCount() const { return static_cast<i32>(m_octaves.size()); }

    /**
     * @brief 获取最小倍频索引
     */
    [[nodiscard]] i32 minOctave() const { return m_minOctave; }

    /**
     * @brief 获取最大倍频索引
     */
    [[nodiscard]] i32 maxOctave() const { return m_maxOctave; }

    /**
     * @brief 保持精度（参考 MC maintainPrecision）
     *
     * 防止大坐标导致的精度问题
     */
    [[nodiscard]] static f32 maintainPrecision(f32 value)
    {
        // 参考 MC: value - floor(value / 33554432.0 + 0.5) * 33554432.0
        constexpr f32 PRECISION_FACTOR = 33554432.0f;
        return value - std::floor(value / PRECISION_FACTOR + 0.5f) * PRECISION_FACTOR;
    }

private:
    std::vector<std::unique_ptr<ImprovedNoiseGenerator>> m_octaves;
    i32 m_minOctave;
    i32 m_maxOctave;

    // 缓存的振幅值
    f32 m_amplitudeLow;  // 低频振幅
    f32 m_amplitudeHigh; // 高频振幅

    void initOctaves(math::IRandom& rng);
};

/**
 * @brief Simplex 噪声生成器
 *
 * 参考 MC SimplexNoiseGenerator，用于特定维度的地形生成。
 * 与 Perlin 噪声相比，Simplex 噪声在低维度时更高效，伪影更少。
 *
 * 使用方法：
 * @code
 * SimplexNoiseGenerator noise(seed);
 * f64 value = noise.getValue(x, z);
 * @endcode
 */
class SimplexNoiseGenerator {
public:
    explicit SimplexNoiseGenerator(u64 seed);
    explicit SimplexNoiseGenerator(math::IRandom& rng);

    ~SimplexNoiseGenerator() = default;

    // 禁止拷贝
    SimplexNoiseGenerator(const SimplexNoiseGenerator&) = delete;
    SimplexNoiseGenerator& operator=(const SimplexNoiseGenerator&) = delete;

    // 允许移动
    SimplexNoiseGenerator(SimplexNoiseGenerator&&) noexcept = default;
    SimplexNoiseGenerator& operator=(SimplexNoiseGenerator&&) noexcept = default;

    /**
     * @brief 采样 3D 噪声值
     */
    [[nodiscard]] f32 noise(f32 x, f32 y, f32 z) const;

    /**
     * @brief 采样 2D 噪声值
     */
    [[nodiscard]] f32 noise2D(f32 x, f32 z) const;

    /**
     * @brief 采样 2D 噪声值（双精度版本）
     *
     * 参考 MC SimplexNoiseGenerator.getValue
     * @param x X 坐标
     * @param z Z 坐标
     * @return 噪声值
     */
    [[nodiscard]] f64 getValue(f64 x, f64 z) const;

    /**
     * @brief 采样用于末地维度的高度
     */
    [[nodiscard]] f32 sampleEndHeight(i32 x, i32 z) const;

    /**
     * @brief 获取 X 偏移
     * 参考 MC SimplexNoiseGenerator.xo
     */
    [[nodiscard]] f32 xOffset() const { return m_offset[0]; }

    /**
     * @brief 获取 Y 偏移
     * 参考 MC SimplexNoiseGenerator.yo
     */
    [[nodiscard]] f32 yOffset() const { return m_offset[1]; }

    /**
     * @brief 获取 Z 偏移
     * 参考 MC SimplexNoiseGenerator.zo
     */
    [[nodiscard]] f32 zOffset() const { return m_offset[2]; }

private:
    std::array<u8, 256> m_permutation;
    std::array<u8, 512> m_p;
    std::array<f32, 3> m_offset;

    void initPermutation(math::IRandom& rng);
    [[nodiscard]] static i32 fastFloor(f32 x);
    [[nodiscard]] static i32 fastFloor(f64 x);
    [[nodiscard]] f32 grad(i32 hash, f32 x, f32 y, f32 z) const;
    [[nodiscard]] f64 grad2D(i32 hash, f64 x, f64 z) const;
};

/**
 * @brief Perlin 噪声生成器
 *
 * 参考 MC PerlinNoiseGenerator，用于地表深度噪声。
 * 注意：MC 的 PerlinNoiseGenerator 实际使用 SimplexNoiseGenerator 作为倍频层。
 *
 * 使用方法：
 * @code
 * PerlinNoiseGenerator noise(rng, -3, 0);  // 创建 4 个倍频层
 * f32 value = noise.noiseAt(x, z, true);
 * @endcode
 */
class PerlinNoiseGenerator : public INoiseGenerator {
public:
    /**
     * @brief 使用种子和倍频范围构造
     * @param seed 随机种子
     * @param minOctave 最小倍频索引（负数）
     * @param maxOctave 最大倍频索引
     */
    PerlinNoiseGenerator(u64 seed, i32 minOctave, i32 maxOctave);

    /**
     * @brief 使用随机生成器构造
     * @param rng 随机数生成器接口
     * @param minOctave 最小倍频索引（负数）
     * @param maxOctave 最大倍频索引
     */
    PerlinNoiseGenerator(math::IRandom& rng, i32 minOctave, i32 maxOctave);

    ~PerlinNoiseGenerator() override = default;

    // 禁止拷贝
    PerlinNoiseGenerator(const PerlinNoiseGenerator&) = delete;
    PerlinNoiseGenerator& operator=(const PerlinNoiseGenerator&) = delete;

    // 允许移动
    PerlinNoiseGenerator(PerlinNoiseGenerator&&) noexcept = default;
    PerlinNoiseGenerator& operator=(PerlinNoiseGenerator&&) noexcept = default;

    [[nodiscard]] f32 noise(f32 x, f32 y, f32 z) const override;

    /**
     * @brief 采样 2D 噪声（带偏移选项）
     *
     * 参考 MC PerlinNoiseGenerator.noiseAt
     * @param x X 坐标
     * @param z Z 坐标
     * @param useNoiseOffsets 是否使用噪声偏移
     * @return 噪声值
     */
    [[nodiscard]] f32 noiseAt(f32 x, f32 z, bool useNoiseOffsets) const;

    /**
     * @brief 采样 2D 噪声（简化版）
     */
    [[nodiscard]] f32 noise2D(f32 x, f32 z) const override;

    /**
     * @brief 获取指定倍频层
     * @param octave 倍频索引
     * @return 倍频层指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] SimplexNoiseGenerator* getOctave(i32 octave);
    [[nodiscard]] const SimplexNoiseGenerator* getOctave(i32 octave) const;

    /**
     * @brief 获取倍频数量
     */
    [[nodiscard]] i32 octaveCount() const { return static_cast<i32>(m_noiseLevels.size()); }

private:
    std::vector<std::unique_ptr<SimplexNoiseGenerator>> m_noiseLevels;
    f32 m_xFactor = 1.0f; // field_227463_c_ = 2^maxOctave
    f32 m_yFactor = 1.0f; // field_227462_b_ = 1 / (2^(count) - 1)
    i32 m_minOctave = 0;
    i32 m_maxOctave = 0;

    void initNoiseLevels(math::IRandom& rng);
};

} // namespace mc
