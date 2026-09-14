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
#include "common/util/math/random/IRandom.hpp"
#include <algorithm>
#include <memory>
#include <utility>

namespace mc::world::gen::valueprovider {

/**
 * @brief 浮点值提供器（MC 1.21 FloatProvider）
 *
 * 提供可控随机性的浮点采样。用于雕刻器配置中的半径乘数、Y 缩放等参数。
 */
class FloatProvider {
public:
    virtual ~FloatProvider() = default;

    /** @brief 采样一个浮点值 */
    [[nodiscard]] virtual f32 sample(math::IRandom& rng) const = 0;

    /** @brief 获取最小可能值 */
    [[nodiscard]] virtual f32 getMinValue() const = 0;

    /** @brief 获取最大可能值 */
    [[nodiscard]] virtual f32 getMaxValue() const = 0;

    /** @brief 获取提供器类型名称（用于调试） */
    [[nodiscard]] virtual const char* getTypeName() const = 0;
};

// ============================================================================
// ConstantFloat — 固定值提供器
// ============================================================================

/**
 * @brief 固定浮点值提供器（MC 1.21 ConstantFloat）
 *
 * 始终返回相同的值。用于不需要随机性的场景，如峡谷的
 * verticalRadiusDefaultFactor=1.0、verticalRadiusCenterFactor=0.0。
 */
class ConstantFloat final : public FloatProvider {
public:
    static std::unique_ptr<ConstantFloat> create(f32 value) { return std::make_unique<ConstantFloat>(value); }

    explicit ConstantFloat(f32 value)
        : m_value(value)
    {}

    [[nodiscard]] f32 sample(math::IRandom& /*rng*/) const override { return m_value; }
    [[nodiscard]] f32 getMinValue() const override { return m_value; }
    [[nodiscard]] f32 getMaxValue() const override { return m_value; }
    [[nodiscard]] const char* getTypeName() const override { return "constant"; }

    [[nodiscard]] f32 getValue() const { return m_value; }

private:
    f32 m_value;
};

// ============================================================================
// UniformFloat — 均匀分布浮点值提供器
// ============================================================================

/**
 * @brief 均匀分布浮点值提供器（MC 1.21 UniformFloat）
 *
 * 在 [minInclusive, maxExclusive) 范围内均匀采样。
 * 参考 MC 1.21.11: net.minecraft.util.valueproviders.UniformFloat
 * 采样方法：Mth.randomBetween(rng, min, max) = min + nextFloat() * (max - min)
 *
 * 用途：洞穴的 horizontalRadiusMultiplier(0.7, 1.4)、
 *       verticalRadiusMultiplier(0.8, 1.3)、floorLevel(-1.0, -0.4) 等。
 */
class UniformFloat final : public FloatProvider {
public:
    static std::unique_ptr<UniformFloat> create(f32 minInclusive, f32 maxExclusive)
    {
        return std::make_unique<UniformFloat>(minInclusive, maxExclusive);
    }

    UniformFloat(f32 minInclusive, f32 maxExclusive)
        : m_min(minInclusive)
        , m_max(maxExclusive)
    {}

    [[nodiscard]] f32 sample(math::IRandom& rng) const override
    {
        if (m_min >= m_max) {
            return m_min;
        }
        // MC: Mth.randomBetween(rng, min, max) = min + nextFloat() * (max - min)
        return m_min + rng.nextFloat() * (m_max - m_min);
    }

    [[nodiscard]] f32 getMinValue() const override { return m_min; }
    [[nodiscard]] f32 getMaxValue() const override { return m_max; }
    [[nodiscard]] const char* getTypeName() const override { return "uniform"; }

private:
    f32 m_min;
    f32 m_max;
};

// ============================================================================
// TrapezoidFloat — 梯形分布浮点值提供器
// ============================================================================

/**
 * @brief 梯形分布浮点值提供器（MC 1.21 TrapezoidFloat）
 *
 * 在 [min, max] 范围内以梯形分布采样。plateau 定义平坦区域的宽度：
 * - plateau = 0 时退化为三角分布
 * - plateau = (max - min) 时退化为均匀分布
 *
 * 参考 MC 1.21.11: net.minecraft.util.valueproviders.TrapezoidFloat
 *
 * 用途：峡谷的 thickness(0.0, 6.0, 2.0) 参数。
 */
class TrapezoidFloat final : public FloatProvider {
public:
    static std::unique_ptr<TrapezoidFloat> create(f32 minInclusive, f32 maxInclusive, f32 plateau)
    {
        return std::make_unique<TrapezoidFloat>(minInclusive, maxInclusive, plateau);
    }

    TrapezoidFloat(f32 minInclusive, f32 maxInclusive, f32 plateau)
        : m_min(minInclusive)
        , m_max(maxInclusive)
        , m_plateau(plateau)
    {}

    [[nodiscard]] f32 sample(math::IRandom& rng) const override
    {
        // MC: f = max - min; f1 = (f - plateau) / 2; f2 = f - f1;
        //     return min + nextFloat() * f2 + nextFloat() * f1;
        const f32 f = m_max - m_min;
        if (f <= 0.0f) {
            return m_min;
        }
        const f32 f1 = (f - m_plateau) / 2.0f;
        const f32 f2 = f - f1;
        return m_min + rng.nextFloat() * f2 + rng.nextFloat() * f1;
    }

    [[nodiscard]] f32 getMinValue() const override { return m_min; }
    [[nodiscard]] f32 getMaxValue() const override { return m_max; }
    [[nodiscard]] const char* getTypeName() const override { return "trapezoid"; }

    [[nodiscard]] f32 getPlateau() const { return m_plateau; }

private:
    f32 m_min;
    f32 m_max;
    f32 m_plateau;
};

// ============================================================================
// ClampedFloat — 钳位浮点值提供器
// ============================================================================

/**
 * @brief 钳位浮点值提供器（MC 1.21 ClampedFloat）
 *
 * 将另一个 FloatProvider 的采样结果钳位到 [minInclusive, maxInclusive] 范围。
 */
class ClampedFloat final : public FloatProvider {
public:
    static std::unique_ptr<ClampedFloat> create(
        std::unique_ptr<FloatProvider> source, f32 minInclusive, f32 maxInclusive)
    {
        return std::make_unique<ClampedFloat>(std::move(source), minInclusive, maxInclusive);
    }

    ClampedFloat(std::unique_ptr<FloatProvider> source, f32 minInclusive, f32 maxInclusive)
        : m_source(std::move(source))
        , m_min(minInclusive)
        , m_max(maxInclusive)
    {}

    [[nodiscard]] f32 sample(math::IRandom& rng) const override
    {
        return std::clamp(m_source->sample(rng), m_min, m_max);
    }

    [[nodiscard]] f32 getMinValue() const override { return m_min; }
    [[nodiscard]] f32 getMaxValue() const override { return m_max; }
    [[nodiscard]] const char* getTypeName() const override { return "clamped"; }

private:
    std::unique_ptr<FloatProvider> m_source;
    f32 m_min;
    f32 m_max;
};

// ============================================================================
// ClampedNormalFloat — 钳位正态分布浮点值提供器
// ============================================================================

/**
 * @brief 钳位正态分布浮点值提供器（MC 1.21 ClampedNormalFloat）
 *
 * 以 (mean, deviation) 正态分布采样，再钳位到 [min, max]。
 * 采样方法：Mth.clamp(Mth.normal(rng, mean, deviation), min, max)。
 *
 * 用途：DripstoneClusterFeature 的滴石高度采样、LargeDripstoneFeature 的钝度/高度缩放。
 */
class ClampedNormalFloat final : public FloatProvider {
public:
    static std::unique_ptr<ClampedNormalFloat> create(f32 mean, f32 deviation, f32 min, f32 max)
    {
        return std::make_unique<ClampedNormalFloat>(mean, deviation, min, max);
    }

    ClampedNormalFloat(f32 mean, f32 deviation, f32 min, f32 max)
        : m_mean(mean)
        , m_deviation(deviation)
        , m_min(min)
        , m_max(max)
    {}

    [[nodiscard]] f32 sample(math::IRandom& rng) const override
    {
        return sampleStatic(rng, m_mean, m_deviation, m_min, m_max);
    }

    [[nodiscard]] f32 getMinValue() const override { return m_min; }
    [[nodiscard]] f32 getMaxValue() const override { return m_max; }
    [[nodiscard]] const char* getTypeName() const override { return "clamped_normal"; }

    [[nodiscard]] f32 getMean() const noexcept { return m_mean; }
    [[nodiscard]] f32 getDeviation() const noexcept { return m_deviation; }

    /**
     * @brief 静态采样：clamp(normal(rng, mean, deviation), min, max)
     *
     * DripstoneClusterFeature.randomBetweenBiased 直接调用此静态方法。
     */
    [[nodiscard]] static f32 sampleStatic(math::IRandom& rng, f32 mean, f32 deviation, f32 min, f32 max)
    {
        return std::clamp(rng.nextGaussian(mean, deviation), min, max);
    }

private:
    f32 m_mean;
    f32 m_deviation;
    f32 m_min;
    f32 m_max;
};

} // namespace mc::world::gen::valueprovider
