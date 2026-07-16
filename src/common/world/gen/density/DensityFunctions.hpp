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

#include "common/core/Constants.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/gen/density/BlendedNoise.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/gen/noise/SimplexNoise.hpp"
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace mc::world::gen {
class RandomState;
} // namespace mc::world::gen

namespace mc::world::gen::density {

// ============================================================================
// 常量
// ============================================================================

/// YClampedGradient 的默认范围常量（MC 1.21 主世界）
inline constexpr i32 OVERWORLD_MIN_Y = world::MIN_BUILD_HEIGHT;
inline constexpr i32 OVERWORLD_MAX_Y = world::MAX_BUILD_HEIGHT;

// ============================================================================
// Constant — 常量密度函数
// ============================================================================

/**
 * @brief 常量密度函数
 *
 * 始终返回固定值，用于表示固定的密度值或零值。
 */
class Constant final : public DensityFunction {
public:
    explicit Constant(f64 value)
        : m_value(value)
    {}

    [[nodiscard]] f64 compute(i32, i32, i32) const override { return m_value; }
    [[nodiscard]] f64 minValue() const override { return m_value; }
    [[nodiscard]] f64 maxValue() const override { return m_value; }

    [[nodiscard]] f64 value() const { return m_value; }

    DENSITY_FUNCTION_MAP_ALL_LEAF(Constant, m_value)

private:
    f64 m_value;
};

// ============================================================================
// YClampedGradient — Y 轴梯度密度函数
// ============================================================================

/**
 * @brief Y 轴钳制梯度密度函数
 *
 * 将 blockY 线性映射到 [fromValue, toValue]，
 * 超出 [fromY, toY] 范围时钳制到边界值。
 * 这是 Climate.Sampler 中 depth 参数的核心实现。
 */
class YClampedGradient final : public DensityFunction {
public:
    YClampedGradient(i32 fromY, i32 toY, f64 fromValue, f64 toValue)
        : m_fromY(fromY)
        , m_toY(toY)
        , m_fromValue(fromValue)
        , m_toValue(toValue)
        , m_minValue(std::min(fromValue, toValue))
        , m_maxValue(std::max(fromValue, toValue))
    {}

    [[nodiscard]] f64 compute(i32, i32 blockY, i32) const override
    {
        return clampedMap(
            static_cast<f64>(blockY), static_cast<f64>(m_fromY), static_cast<f64>(m_toY), m_fromValue, m_toValue);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] i32 fromY() const { return m_fromY; }
    [[nodiscard]] i32 toY() const { return m_toY; }
    [[nodiscard]] f64 fromValue() const { return m_fromValue; }
    [[nodiscard]] f64 toValue() const { return m_toValue; }

    DENSITY_FUNCTION_MAP_ALL_LEAF(YClampedGradient, m_fromY, m_toY, m_fromValue, m_toValue)

private:
    i32 m_fromY;
    i32 m_toY;
    f64 m_fromValue;
    f64 m_toValue;
    f64 m_minValue;
    f64 m_maxValue;

    [[nodiscard]] static f64 clampedLerp(f64 t, f64 from, f64 to)
    {
        if (t < 0.0) return from;
        if (t > 1.0) return to;
        return from + t * (to - from);
    }

    [[nodiscard]] static f64 inverseLerp(f64 value, f64 from, f64 to) { return (value - from) / (to - from); }

    [[nodiscard]] static f64 clampedMap(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax)
    {
        return clampedLerp(inverseLerp(value, fromMin, fromMax), toMin, toMax);
    }
};

// ============================================================================
// Clamp — 钳制密度函数
// ============================================================================

/**
 * @brief 钳制密度函数
 *
 * 将输入密度函数的输出限制在 [minValue, maxValue] 范围内。
 */
class Clamp final : public DensityFunction {
public:
    Clamp(std::unique_ptr<DensityFunction> input, f64 minValue, f64 maxValue)
        : m_input(std::move(input))
        , m_minValue(minValue)
        , m_maxValue(maxValue)
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        return std::clamp(m_input->compute(blockX, blockY, blockZ), m_minValue, m_maxValue);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<Clamp>(std::move(newInput), m_minValue, m_maxValue));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    f64 m_minValue;
    f64 m_maxValue;
};

// ============================================================================
// Mapped — 单参数变换密度函数
// ============================================================================

/**
 * @brief 单参数变换类型枚举
 */
enum class MappedType : u8 { Abs, Square, Cube, HalfNegative, QuarterNegative, Squeeze, Invert };

/**
 * @brief 单参数变换密度函数
 *
 * 对输入密度函数的输出应用数学变换：
 * - Abs: 绝对值
 * - Square: 平方
 * - Cube: 立方
 * - HalfNegative: 负值减半
 * - QuarterNegative: 负值四分之一
 * - Squeeze: 压缩映射
 */
class Mapped final : public DensityFunction {
public:
    Mapped(std::unique_ptr<DensityFunction> input, MappedType type)
        : m_input(std::move(input))
        , m_type(type)
    {
        computeBounds();
    }

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 value = m_input->compute(blockX, blockY, blockZ);
        return transform(value);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] MappedType type() const { return m_type; }
    [[nodiscard]] const DensityFunction& input() const { return *m_input; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<Mapped>(std::move(newInput), m_type));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    MappedType m_type;
    f64 m_minValue = 0.0;
    f64 m_maxValue = 0.0;

    [[nodiscard]] f64 transform(f64 value) const
    {
        switch (m_type) {
            case MappedType::Abs:
                return std::abs(value);
            case MappedType::Square:
                return value * value;
            case MappedType::Cube:
                return value * value * value;
            case MappedType::HalfNegative:
                return value > 0.0 ? value : value * 0.5;
            case MappedType::QuarterNegative:
                return value > 0.0 ? value : value * 0.25;
            case MappedType::Squeeze: {
                const f64 clamped = std::clamp(value, -1.0, 1.0);
                return clamped / 2.0 - clamped * clamped * clamped / 24.0;
            }
            case MappedType::Invert:
                return 1.0 / value;
        }
        return 0.0;
    }

    void computeBounds()
    {
        const f64 inMin = m_input->minValue();
        const f64 inMax = m_input->maxValue();

        switch (m_type) {
            case MappedType::Abs:
                // MC 1.21: minValue = max(0, input.minValue), maxValue = max(transform(min), transform(max))
                m_minValue = std::max(0.0, inMin);
                m_maxValue = std::max(std::abs(inMin), std::abs(inMax));
                break;
            case MappedType::Square: {
                // MC 1.21: minValue = max(0, input.minValue), maxValue = max(transform(min), transform(max))
                m_minValue = std::max(0.0, inMin);
                m_maxValue = std::max(inMin * inMin, inMax * inMax);
                break;
            }
            case MappedType::Cube:
                m_minValue = inMin * inMin * inMin;
                m_maxValue = inMax * inMax * inMax;
                break;
            case MappedType::HalfNegative:
                m_minValue = inMin < 0.0 ? inMin * 0.5 : inMin;
                m_maxValue = inMax > 0.0 ? inMax : inMax * 0.5;
                break;
            case MappedType::QuarterNegative:
                m_minValue = inMin < 0.0 ? inMin * 0.25 : inMin;
                m_maxValue = inMax > 0.0 ? inMax : inMax * 0.25;
                break;
            case MappedType::Squeeze:
                // Squeeze 的输出范围取决于输入范围，这里使用保守估计
                m_minValue = transform(inMin);
                m_maxValue = transform(inMax);
                if (m_minValue > m_maxValue) {
                    std::swap(m_minValue, m_maxValue);
                }
                break;
            case MappedType::Invert:
                // MC 1.21: 1/x — 当输入为 0 时返回 +infinity（IEEE 754）
                if (inMin > 0.0) {
                    m_minValue = 1.0 / inMax;
                    m_maxValue = 1.0 / inMin;
                } else if (inMax < 0.0) {
                    m_minValue = 1.0 / inMax;
                    m_maxValue = 1.0 / inMin;
                } else {
                    m_minValue = -std::numeric_limits<f64>::infinity();
                    m_maxValue = std::numeric_limits<f64>::infinity();
                }
                break;
        }
    }
};

// ============================================================================
// TwoArgument — 双参数密度函数
// ============================================================================

/**
 * @brief 双参数运算类型枚举
 */
enum class TwoArgumentType : u8 { Add, Mul, Min, Max };

/**
 * @brief 双参数密度函数
 *
 * 对两个输入密度函数的输出执行二元运算：
 * - Add: 加法
 * - Mul: 乘法
 * - Min: 最小值
 * - Max: 最大值
 */
class TwoArgument final : public DensityFunction {
public:
    TwoArgument(std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2, TwoArgumentType type)
        : m_arg1(std::move(arg1))
        , m_arg2(std::move(arg2))
        , m_type(type)
    {
        computeBounds();
    }

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 v1 = m_arg1->compute(blockX, blockY, blockZ);

        switch (m_type) {
            case TwoArgumentType::Add:
                return v1 + m_arg2->compute(blockX, blockY, blockZ);
            case TwoArgumentType::Mul:
                return v1 == 0.0 ? 0.0 : v1 * m_arg2->compute(blockX, blockY, blockZ);
            case TwoArgumentType::Min:
                return v1 < m_arg2->minValue() ? v1 : std::min(v1, m_arg2->compute(blockX, blockY, blockZ));
            case TwoArgumentType::Max:
                return v1 > m_arg2->maxValue() ? v1 : std::max(v1, m_arg2->compute(blockX, blockY, blockZ));
        }
        return 0.0;
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] TwoArgumentType type() const { return m_type; }
    [[nodiscard]] const DensityFunction& arg1() const { return *m_arg1; }
    [[nodiscard]] const DensityFunction& arg2() const { return *m_arg2; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newArg1 = m_arg1->mapAll(visitor);
        auto newArg2 = m_arg2->mapAll(visitor);
        return visitor.apply(std::make_unique<TwoArgument>(std::move(newArg1), std::move(newArg2), m_type));
    }

private:
    std::unique_ptr<DensityFunction> m_arg1;
    std::unique_ptr<DensityFunction> m_arg2;
    TwoArgumentType m_type;
    f64 m_minValue = 0.0;
    f64 m_maxValue = 0.0;

    void computeBounds()
    {
        const f64 min1 = m_arg1->minValue();
        const f64 max1 = m_arg1->maxValue();
        const f64 min2 = m_arg2->minValue();
        const f64 max2 = m_arg2->maxValue();

        switch (m_type) {
            case TwoArgumentType::Add:
                m_minValue = min1 + min2;
                m_maxValue = max1 + max2;
                break;
            case TwoArgumentType::Mul:
                // MC 1.21: MUL 的 minValue/maxValue 计算
                // minValue: d0>0 && d1>0 ? d0*d1 : (d2<0 && d3<0 ? d2*d3 : min(d0*d3, d2*d1))
                // maxValue: d0>0 && d1>0 ? d2*d3 : (d2<0 && d3<0 ? d0*d1 : max(d0*d1, d2*d3))
                // 其中 d0=min1, d1=min2, d2=max1, d3=max2
                if (min1 > 0.0 && min2 > 0.0) {
                    m_minValue = min1 * min2;
                } else if (max1 < 0.0 && max2 < 0.0) {
                    m_minValue = max1 * max2;
                } else {
                    m_minValue = std::min(min1 * max2, max1 * min2);
                }
                if (min1 > 0.0 && min2 > 0.0) {
                    m_maxValue = max1 * max2;
                } else if (max1 < 0.0 && max2 < 0.0) {
                    m_maxValue = min1 * min2;
                } else {
                    m_maxValue = std::max(min1 * min2, max1 * max2);
                }
                break;
            case TwoArgumentType::Min:
                m_minValue = std::min(min1, min2);
                m_maxValue = std::min(max1, max2);
                break;
            case TwoArgumentType::Max:
                m_minValue = std::max(min1, min2);
                m_maxValue = std::max(max1, max2);
                break;
        }
    }
};

// ============================================================================
// Lerp — 线性插值密度函数
// ============================================================================

/**
 * @brief 线性插值密度函数
 *
 * lerp(delta, start, end) = start + delta * (end - start)
 * MC 1.21 用于 spline 系统中的插值。
 */
class Lerp final : public DensityFunction {
public:
    Lerp(std::unique_ptr<DensityFunction> delta,
        std::unique_ptr<DensityFunction> start,
        std::unique_ptr<DensityFunction> end)
        : m_delta(std::move(delta))
        , m_start(std::move(start))
        , m_end(std::move(end))
    {
        computeBounds();
    }

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 d = m_delta->compute(blockX, blockY, blockZ);
        const f64 s = m_start->compute(blockX, blockY, blockZ);
        if (d <= 0.0) return s;
        const f64 e = m_end->compute(blockX, blockY, blockZ);
        if (d >= 1.0) return e;
        return s + d * (e - s);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const DensityFunction& delta() const { return *m_delta; }
    [[nodiscard]] const DensityFunction& start() const { return *m_start; }
    [[nodiscard]] const DensityFunction& end() const { return *m_end; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newDelta = m_delta->mapAll(visitor);
        auto newStart = m_start->mapAll(visitor);
        auto newEnd = m_end->mapAll(visitor);
        return visitor.apply(std::make_unique<Lerp>(std::move(newDelta), std::move(newStart), std::move(newEnd)));
    }

private:
    std::unique_ptr<DensityFunction> m_delta;
    std::unique_ptr<DensityFunction> m_start;
    std::unique_ptr<DensityFunction> m_end;
    f64 m_minValue = 0.0;
    f64 m_maxValue = 0.0;

    void computeBounds()
    {
        const f64 sMin = m_start->minValue();
        const f64 sMax = m_start->maxValue();
        const f64 eMin = m_end->minValue();
        const f64 eMax = m_end->maxValue();
        m_minValue = std::min({sMin, sMax, eMin, eMax});
        m_maxValue = std::max({sMin, sMax, eMin, eMax});
    }
};

// ============================================================================
// MappedNoise — 带输出重映射的噪声密度函数
// ============================================================================

/**
 * @brief 带输出重映射的噪声密度函数
 *
 * compute(x, y, z) = noise(x * xzScale, y * yScale, z * xzScale) * scaleFactor + fromValue
 * MC 1.21 中用于 SPAGHETTI_2D_ELEVATION 等噪声。
 */
class MappedNoise final : public DensityFunction {
public:
    MappedNoise(std::shared_ptr<const noise::NormalNoise> noise, f64 xzScale, f64 yScale, f64 fromValue, f64 toValue)
        : m_noise(std::move(noise))
        , m_xzScale(xzScale)
        , m_yScale(yScale)
        , m_fromValue(fromValue)
        , m_toValue(toValue)
    {
        // MC 1.21: bounds = add(constant(midpoint), mul(constant(halfAmplitude), noise))
        // midpoint = (fromValue + toValue) / 2, halfAmplitude = (toValue - fromValue) / 2
        // minValue = midpoint - |halfAmplitude| * noise.maxValue()
        // maxValue = midpoint + |halfAmplitude| * noise.maxValue()
        const f64 maxNoise = m_noise->maxValue();
        const f64 midpoint = (fromValue + toValue) * 0.5;
        const f64 halfAmplitude = (toValue - fromValue) * 0.5;
        const f64 absHalfAmplitude = std::abs(halfAmplitude);
        m_minValue = midpoint - absHalfAmplitude * maxNoise;
        m_maxValue = midpoint + absHalfAmplitude * maxNoise;
    }

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 noiseVal = m_noise->getValue(static_cast<f64>(blockX) * m_xzScale,
            static_cast<f64>(blockY) * m_yScale,
            static_cast<f64>(blockZ) * m_xzScale);
        return m_fromValue + noiseVal * (m_toValue - m_fromValue);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    // NormalNoise::getValue 是 const 无 mutable，mapAll 前后语义等价，
    // 因此共享 m_noise（不再 clone），仅重建叶子壳子。详见 RandomState::getOrCreateRouterNoise。
    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        return visitor.apply(std::make_unique<MappedNoise>(m_noise, m_xzScale, m_yScale, m_fromValue, m_toValue));
    }

private:
    std::shared_ptr<const noise::NormalNoise> m_noise;
    f64 m_xzScale;
    f64 m_yScale;
    f64 m_fromValue;
    f64 m_toValue;
    f64 m_minValue;
    f64 m_maxValue;
};

/**
 * @brief 基础噪声密度函数
 *
 * 在指定坐标处采样 NormalNoise，乘以 XZ 和 Y 缩放因子。
 */
class NoiseDensity final : public DensityFunction {
public:
    NoiseDensity(std::shared_ptr<const noise::NormalNoise> noise, f64 xzScale, f64 yScale)
        : m_noise(std::move(noise))
        , m_xzScale(xzScale)
        , m_yScale(yScale)
        , m_minValue(-m_noise->maxValue())
        , m_maxValue(m_noise->maxValue())
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        return m_noise->getValue(static_cast<f64>(blockX) * m_xzScale,
            static_cast<f64>(blockY) * m_yScale,
            static_cast<f64>(blockZ) * m_xzScale);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const noise::NormalNoise& noise() const { return *m_noise; }
    [[nodiscard]] f64 xzScale() const { return m_xzScale; }
    [[nodiscard]] f64 yScale() const { return m_yScale; }

    // NormalNoise::getValue 是 const 无 mutable，mapAll 前后语义等价，
    // 因此共享 m_noise（不再 clone），仅重建叶子壳子。详见 RandomState::getOrCreateRouterNoise。
    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        return visitor.apply(std::make_unique<NoiseDensity>(m_noise, m_xzScale, m_yScale));
    }

private:
    std::shared_ptr<const noise::NormalNoise> m_noise;
    f64 m_xzScale;
    f64 m_yScale;
    f64 m_minValue;
    f64 m_maxValue;
};

// ============================================================================
// ShiftedNoise — 带偏移的噪声密度函数
// ============================================================================

/**
 * @brief 带坐标偏移的噪声密度函数
 *
 * 在噪声采样前，先通过其他密度函数计算坐标偏移。
 * 用于 temperature、humidity、continentalness、erosion、ridges 等气候参数。
 */
class ShiftedNoise final : public DensityFunction {
public:
    ShiftedNoise(std::shared_ptr<const noise::NormalNoise> noise,
        f64 xzScale,
        f64 yScale,
        std::unique_ptr<DensityFunction> shiftX,
        std::unique_ptr<DensityFunction> shiftY,
        std::unique_ptr<DensityFunction> shiftZ)
        : m_noise(std::move(noise))
        , m_xzScale(xzScale)
        , m_yScale(yScale)
        , m_shiftX(std::move(shiftX))
        , m_shiftY(std::move(shiftY))
        , m_shiftZ(std::move(shiftZ))
        , m_minValue(-m_noise->maxValue())
        , m_maxValue(m_noise->maxValue())
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 sx = static_cast<f64>(blockX) * m_xzScale + m_shiftX->compute(blockX, blockY, blockZ);
        const f64 sy = static_cast<f64>(blockY) * m_yScale + m_shiftY->compute(blockX, blockY, blockZ);
        const f64 sz = static_cast<f64>(blockZ) * m_xzScale + m_shiftZ->compute(blockX, blockY, blockZ);
        return m_noise->getValue(sx, sy, sz);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const noise::NormalNoise& noise() const { return *m_noise; }
    [[nodiscard]] f64 xzScale() const { return m_xzScale; }
    [[nodiscard]] f64 yScale() const { return m_yScale; }
    [[nodiscard]] const DensityFunction& shiftX() const { return *m_shiftX; }
    [[nodiscard]] const DensityFunction& shiftY() const { return *m_shiftY; }
    [[nodiscard]] const DensityFunction& shiftZ() const { return *m_shiftZ; }

    // m_noise 共享（NormalNoise const 无 mutable，mapAll 前后等价，不再 clone）；
    // m_shiftX/Y/Z 是复合子函数，仍递归 mapAll 重建（与原语义一致）。
    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newShiftX = m_shiftX->mapAll(visitor);
        auto newShiftY = m_shiftY->mapAll(visitor);
        auto newShiftZ = m_shiftZ->mapAll(visitor);
        return visitor.apply(std::make_unique<ShiftedNoise>(
            m_noise, m_xzScale, m_yScale, std::move(newShiftX), std::move(newShiftY), std::move(newShiftZ)));
    }

private:
    std::shared_ptr<const noise::NormalNoise> m_noise;
    f64 m_xzScale;
    f64 m_yScale;
    std::unique_ptr<DensityFunction> m_shiftX;
    std::unique_ptr<DensityFunction> m_shiftY;
    std::unique_ptr<DensityFunction> m_shiftZ;
    f64 m_minValue;
    f64 m_maxValue;
};

// ============================================================================
// ShiftA / ShiftB / Shift — 坐标偏移噪声密度函数
// ============================================================================

/**
 * @brief 坐标偏移类型枚举
 */
enum class ShiftType : u8 { ShiftA, ShiftB, Shift };

/**
 * @brief 坐标偏移噪声密度函数
 *
 * ShiftA: noise(x*0.25, 0, z*0.25) * 4
 * ShiftB: noise(z*0.25, x*0.25, 0) * 4（坐标交换）
 * Shift:  noise(x*0.25, y*0.25, z*0.25) * 4
 *
 * 这些函数为 ShiftedNoise 提供坐标偏移量。
 */
class ShiftNoise final : public DensityFunction {
public:
    ShiftNoise(std::shared_ptr<const noise::NormalNoise> noise, ShiftType type)
        : m_noise(std::move(noise))
        , m_type(type)
        , m_minValue(-m_noise->maxValue() * 4.0)
        , m_maxValue(m_noise->maxValue() * 4.0)
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 x4 = static_cast<f64>(blockX) * 0.25;
        const f64 y4 = static_cast<f64>(blockY) * 0.25;
        const f64 z4 = static_cast<f64>(blockZ) * 0.25;

        switch (m_type) {
            case ShiftType::ShiftA:
                return m_noise->getValue(x4, 0.0, z4) * 4.0;
            case ShiftType::ShiftB:
                return m_noise->getValue(z4, x4, 0.0) * 4.0;
            case ShiftType::Shift:
                return m_noise->getValue(x4, y4, z4) * 4.0;
        }
        return 0.0;
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] ShiftType type() const { return m_type; }
    [[nodiscard]] const noise::NormalNoise& noise() const { return *m_noise; }

    // NormalNoise const 无 mutable，mapAll 前后等价，共享 m_noise 不再 clone。
    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        return visitor.apply(std::make_unique<ShiftNoise>(m_noise, m_type));
    }

private:
    std::shared_ptr<const noise::NormalNoise> m_noise;
    ShiftType m_type;
    f64 m_minValue;
    f64 m_maxValue;
};

// ============================================================================
// RangeChoice — 条件选择密度函数
// ============================================================================

/**
 * @brief 条件选择密度函数
 *
 * 根据输入密度函数的值是否在 [min, max] 范围内，
 * 选择两个分支密度函数之一。
 */
class RangeChoice final : public DensityFunction {
public:
    RangeChoice(std::unique_ptr<DensityFunction> input,
        f64 minInclusive,
        f64 maxExclusive,
        std::unique_ptr<DensityFunction> whenInRange,
        std::unique_ptr<DensityFunction> whenOutOfRange)
        : m_input(std::move(input))
        , m_minInclusive(minInclusive)
        , m_maxExclusive(maxExclusive)
        , m_whenInRange(std::move(whenInRange))
        , m_whenOutOfRange(std::move(whenOutOfRange))
        , m_minValue(std::min(m_whenInRange->minValue(), m_whenOutOfRange->minValue()))
        , m_maxValue(std::max(m_whenInRange->maxValue(), m_whenOutOfRange->maxValue()))
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 value = m_input->compute(blockX, blockY, blockZ);
        if (value >= m_minInclusive && value < m_maxExclusive) {
            return m_whenInRange->compute(blockX, blockY, blockZ);
        }
        return m_whenOutOfRange->compute(blockX, blockY, blockZ);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }
    [[nodiscard]] f64 minInclusive() const { return m_minInclusive; }
    [[nodiscard]] f64 maxExclusive() const { return m_maxExclusive; }
    [[nodiscard]] const DensityFunction& whenInRange() const { return *m_whenInRange; }
    [[nodiscard]] const DensityFunction& whenOutOfRange() const { return *m_whenOutOfRange; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        auto newWhenInRange = m_whenInRange->mapAll(visitor);
        auto newWhenOutOfRange = m_whenOutOfRange->mapAll(visitor);
        return visitor.apply(std::make_unique<RangeChoice>(std::move(newInput),
            m_minInclusive,
            m_maxExclusive,
            std::move(newWhenInRange),
            std::move(newWhenOutOfRange)));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    f64 m_minInclusive;
    f64 m_maxExclusive;
    std::unique_ptr<DensityFunction> m_whenInRange;
    std::unique_ptr<DensityFunction> m_whenOutOfRange;
    f64 m_minValue;
    f64 m_maxValue;
};

// ============================================================================
// CubicSpline — 支持嵌套的三次样条密度函数
//
// MC 1.21 CubicSpline<Point, Coordinate> 实现:
// 每个控制点的 value 可以是 f64 常量，也可以是嵌套的 CubicSpline
// （以另一个输入密度函数为坐标轴），从而支持多维度样条树。
//
// 例如主世界地形 offset spline 的结构为:
// CubicSpline(continentalness) -> points -> CubicSpline(erosion) -> points -> CubicSpline(ridges)
// 实现了 (continentalness, erosion, ridges) -> offset 的多维度映射。
// ============================================================================

/**
 * @brief 样条点（支持嵌套子样条）
 *
 * 每个控制点的 value 可以是常量值（f64）或嵌套的 CubicSpline。
 * MC 1.21 CubicSpline.Multipoint 的每个坐标点。
 */
class CubicSpline;

/**
 * @brief 样条点（支持嵌套子样条）
 *
 * 每个控制点的 value 可以是常量值（f64）或嵌套的 CubicSpline。
 * MC 1.21 CubicSpline.Multipoint 的每个坐标点。
 * 使用 shared_ptr<CubicSpline> 以支持多个控制点共享子样条。
 */
struct SplinePoint {
    f64 location = 0.0;                                         ///< 控制点位置（升序排列）
    std::variant<f64, std::shared_ptr<CubicSpline>> value{0.0}; ///< 常量值或嵌套子样条
    f64 derivative = 0.0;                                       ///< 控制点导数

    SplinePoint() = default;

    /** 便利构造: 常量值控制点 */
    SplinePoint(f64 loc, f64 val, f64 der = 0.0)
        : location(loc)
        , value(val)
        , derivative(der)
    {}

    /** 便利构造: 嵌套子样条控制点 */
    SplinePoint(f64 loc, std::shared_ptr<CubicSpline> spline, f64 der = 0.0)
        : location(loc)
        , value(std::move(spline))
        , derivative(der)
    {}
};

/**
 * @brief 支持嵌套的三次样条密度函数
 *
 * MC 1.21 对应 CubicSpline<Multipoint> 实现:
 * - 使用三次 Hermite 插值
 * - 控制点的 value 可以是 f64 或嵌套 CubicSpline（递归）
 * - 用于主世界地形的 offset/factor/jaggedness 计算
 *
 * 算法:
 * 1. 根据输入密度函数值查找所在区间 [i, i+1]
 * 2. 计算区间内归一化位置 t
 * 3. 递归计算左/右端点的值（如果是嵌套样条则递归 compute）
 * 4. 应用三次 Hermite 插值公式
 */
class CubicSpline final : public DensityFunction {
public:
    /**
     * @brief 使用样条点列表构造
     * @param input 输入密度函数（坐标轴），使用 shared_ptr 以支持多个子样条共享同一输入
     * @param points 样条控制点（必须按 location 升序排列）
     */
    CubicSpline(std::shared_ptr<DensityFunction> input, std::vector<SplinePoint> points);

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }
    [[nodiscard]] const std::vector<SplinePoint>& points() const { return m_points; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override;

    /**
     * @brief 在给定坐标处计算样条值（内部使用，支持递归嵌套）
     */
    [[nodiscard]] f64 apply(f64 coordinate, i32 blockX, i32 blockY, i32 blockZ) const;

    /**
     * @brief 计算样条点的值（处理嵌套）
     */
    [[nodiscard]] f64 resolvePointValue(size_t index, i32 blockX, i32 blockY, i32 blockZ) const;

private:
    std::shared_ptr<DensityFunction> m_input;
    std::vector<SplinePoint> m_points;
    f64 m_minValue = 0.0;
    f64 m_maxValue = 0.0;

    void computeBounds();

    /// 线性外推（超出样条范围时使用）
    [[nodiscard]] static f64 linearExtend(f64 coordinate, f64 location, f64 value, f64 derivative);
};

// ============================================================================
// 保留旧 Spline 类型别名以兼容（使用 CubicSpline 的单层简化版本）
// ============================================================================

/**
 * @brief 旧式样条点（仅常量值，不嵌套）
 *
 * 为保持向后兼容保留，新建代码应使用 SplinePoint + CubicSpline。
 */
struct FlatSplinePoint {
    f64 location;
    f64 value;
    f64 derivative;
};

// ============================================================================
// Cache2D — 2D 缓存密度函数
// ============================================================================

/**
 * @brief 2D 缓存密度函数
 *
 * 当 blockX 和 blockZ 不变时复用上一次的结果。
 * 适用于仅依赖 XZ 的密度函数（如 temperature、humidity 等）。
 * 线程不安全，每个区块生成任务应有独立实例。
 */
class Cache2D final : public DensityFunction {
public:
    explicit Cache2D(std::unique_ptr<DensityFunction> input)
        : m_input(std::move(input))
        , m_cachedX(0)
        , m_cachedZ(0)
        , m_cachedValue(0.0)
        , m_valid(false)
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        if (m_valid && blockX == m_cachedX && blockZ == m_cachedZ) {
            return m_cachedValue;
        }
        m_cachedX = blockX;
        m_cachedZ = blockZ;
        m_cachedValue = m_input->compute(blockX, blockY, blockZ);
        m_valid = true;
        return m_cachedValue;
    }

    [[nodiscard]] f64 minValue() const override { return m_input->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_input->maxValue(); }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<Cache2D>(std::move(newInput)));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    mutable i32 m_cachedX;
    mutable i32 m_cachedZ;
    mutable f64 m_cachedValue;
    mutable bool m_valid;
};

// ============================================================================
// FlatCache — 区块级扁平缓存密度函数
// ============================================================================

/**
 * @brief 区块级扁平缓存密度函数 — MC 1.21 NoiseChunk.FlatCache
 *
 * 对仅依赖 XZ 的密度函数（如 continents/erosion/ridges），在 NoiseChunk 构造时
 * 一次性预计算整个区块 quart XZ 网格的值到一维数组 values[(sizeXZ+1)²]，
 * 之后 compute() 退化为 O(1) 数组查表。对齐原版 NoiseChunk.FlatCache
 * （NoiseChunk.java:619-665）的区块级预计算语义。
 *
 * 当未注入区块几何（非 NoiseChunk 上下文，如零散getHeight 查询走非mapAll路径）
 * 时，退化为单值 lastPos 缓存，保证正确性。线程不安全，每个区块生成任务应有独立实例。
 */
class FlatCache final : public DensityFunction {
public:
    /**
     * @brief 构造区块级扁平缓存
     * @param input 被包装的 XZ-only 密度函数
     * @param firstQuartX 区块首个 quart X（= floorDiv(startBlockX, 4)）
     * @param firstQuartZ 区块首个 quart Z（= floorDiv(startBlockZ, 4)）
     * @param sizeXZ quart XZ 网格边长（= cellCountXZ * cellWidth / 4），数组维度为 (sizeXZ+1)²
     * @param precompute true 时构造期双 for 预计算整张表（NoiseChunk::apply 替换时传 true）
     */
    FlatCache(std::unique_ptr<DensityFunction> input, i32 firstQuartX, i32 firstQuartZ, i32 sizeXZ, bool precompute)
        : m_input(std::move(input))
        , m_firstQuartX(firstQuartX)
        , m_firstQuartZ(firstQuartZ)
        , m_sizeXZ(sizeXZ)
        , m_precomputed(precompute)
    {
        if (precompute) {
            // 对齐原版 NoiseChunk.FlatCache 构造期预计算：
            // values[i + l*sizeXZ] = noiseFiller.compute((firstQuartX+i)<<2, 0, (firstQuartZ+l)<<2)
            // Y 传 0（XZ-only 函数忽略 Y），与原版 SinglePointContext(k, 0, j1) 一致
            m_values.resize(static_cast<size_t>(sizeXZ + 1) * static_cast<size_t>(sizeXZ + 1));
            for (i32 l = 0; l <= sizeXZ; ++l) {
                const i32 blockZ = (firstQuartZ + l) << 2;
                for (i32 i = 0; i <= sizeXZ; ++i) {
                    const i32 blockX = (firstQuartX + i) << 2;
                    m_values[static_cast<size_t>(i) + static_cast<size_t>(l) * static_cast<size_t>(sizeXZ + 1)] =
                        m_input->compute(blockX, 0, blockZ);
                }
            }
        }
    }

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        if (m_precomputed) {
            // quart 坐标 = floorDiv(block 坐标, 4)，负坐标下 >> 2 不是向下取整
            const i32 quartX = math::floorDiv(blockX, 4);
            const i32 quartZ = math::floorDiv(blockZ, 4);
            const i32 k = quartX - m_firstQuartX;
            const i32 l = quartZ - m_firstQuartZ;
            if (k >= 0 && l >= 0 && k <= m_sizeXZ && l <= m_sizeXZ) {
                return m_values[static_cast<size_t>(k) + static_cast<size_t>(l) * static_cast<size_t>(m_sizeXZ + 1)];
            }
            // 越界回退（对齐原版 NoiseChunk.FlatCache.compute 的越界分支）
            return m_input->compute(blockX, blockY, blockZ);
        }
        // 非 NoiseChunk 上下文：退化为单值 lastPos 缓存
        if (m_valid && blockX == m_cachedBlockX && blockZ == m_cachedBlockZ) {
            return m_cachedValue;
        }
        m_cachedBlockX = blockX;
        m_cachedBlockZ = blockZ;
        m_cachedValue = m_input->compute(blockX, blockY, blockZ);
        m_valid = true;
        return m_cachedValue;
    }

    [[nodiscard]] f64 minValue() const override { return m_input->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_input->maxValue(); }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        // mapAll 递归替换子节点后，预计算数组需重新生成（子节点指针已变）。
        // 保留几何参数与 precompute 标志，由上层 NoiseChunk 上下文重新触发预计算。
        // 注意：NoiseChunk::apply 在 mapAll 之后替换 Marker，构造的新 FlatCache 自带预计算；
        // 此处 mapAll 用于非 NoiseChunk 路径（如 NoiseRouter 独立 mapAll），退化为单值缓存即可。
        return visitor.apply(std::make_unique<FlatCache>(std::move(newInput), 0, 0, 0, false));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    i32 m_firstQuartX = 0;
    i32 m_firstQuartZ = 0;
    i32 m_sizeXZ = 0;
    bool m_precomputed = false;
    std::vector<f64> m_values;

    // 单值回退路径（非预计算模式）使用
    mutable i32 m_cachedBlockX = 0;
    mutable i32 m_cachedBlockZ = 0;
    mutable f64 m_cachedValue = 0.0;
    mutable bool m_valid = false;
};

// ============================================================================
// CacheAllInCell — 区块内全缓存密度函数
// ============================================================================

/**
 * @brief 区块内全缓存密度函数
 *
 * 对每个区块的每个采样位置（4x4x4 网格）缓存密度值。
 * 在区块内多次采样同一位置时避免重复计算。
 * 使用简单的哈希表缓存。
 */
class CacheAllInCell final : public DensityFunction {
public:
    explicit CacheAllInCell(std::unique_ptr<DensityFunction> input)
        : m_input(std::move(input))
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        // 查找缓存
        for (const auto& entry : m_cache) {
            if (entry.x == blockX && entry.y == blockY && entry.z == blockZ) {
                return entry.value;
            }
        }

        // 缓存未命中，计算并添加
        const f64 value = m_input->compute(blockX, blockY, blockZ);
        m_cache.push_back({blockX, blockY, blockZ, value});
        return value;
    }

    [[nodiscard]] f64 minValue() const override { return m_input->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_input->maxValue(); }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }

    /**
     * @brief 清除缓存（每个新区块开始时调用）
     */
    void clearCache() const { m_cache.clear(); }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<CacheAllInCell>(std::move(newInput)));
    }

private:
    struct CacheEntry {
        i32 x;
        i32 y;
        i32 z;
        f64 value;
    };

    std::unique_ptr<DensityFunction> m_input;
    mutable std::vector<CacheEntry> m_cache;
};

// ============================================================================
// WeirdScaledSampler — 奇异缩放采样器
// ============================================================================

/**
 * @brief 奇异缩放采样器类型枚举
 */
enum class WeirdScaledSamplerType : u8 { Type1, Type2 };

/**
 * @brief 奇异缩放采样器
 *
 * 根据 rarity 映射对噪声值进行非线性缩放，
 * 用于山峰和洞穴的生成判断。
 * - Type1: 使用 MC_TYPE1 映射表
 * - Type2: 使用 MC_TYPE2 映射表
 */
class WeirdScaledSampler final : public DensityFunction {
public:
    WeirdScaledSampler(std::unique_ptr<DensityFunction> input,
        std::shared_ptr<const noise::NormalNoise> noise,
        WeirdScaledSamplerType type)
        : m_input(std::move(input))
        , m_noise(std::move(noise))
        , m_type(type)
    {
        computeBounds();
    }

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 inputValue = m_input->compute(blockX, blockY, blockZ);
        const f64 rarityValue = getRarity(inputValue);
        return std::abs(m_noise->getValue(static_cast<f64>(blockX) / rarityValue,
                   static_cast<f64>(blockY) / rarityValue,
                   static_cast<f64>(blockZ) / rarityValue)) *
            rarityValue;
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }
    [[nodiscard]] const noise::NormalNoise& noise() const { return *m_noise; }
    [[nodiscard]] WeirdScaledSamplerType type() const { return m_type; }

    // m_noise 共享（const 无 mutable，mapAll 前后等价，不再 clone）；
    // m_input 仍递归 mapAll 重建。
    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<WeirdScaledSampler>(std::move(newInput), m_noise, m_type));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    std::shared_ptr<const noise::NormalNoise> m_noise;
    WeirdScaledSamplerType m_type;
    f64 m_minValue = 0.0;
    f64 m_maxValue = 0.0;

    [[nodiscard]] f64 getRarity(f64 value) const
    {
        // MC 1.21 的 rarity 映射
        if (m_type == WeirdScaledSamplerType::Type1) {
            if (value < -0.5) return 0.75;
            if (value < 0.0) return 1.0;
            if (value < 0.5) return 1.5;
            return 2.0;
        }
        // Type2 (sphaghettiRarity2D in MC 1.21)
        if (value < -0.75) return 0.5;
        if (value < -0.5) return 0.75;
        if (value < 0.5) return 1.0;
        if (value < 0.75) return 2.0;
        return 3.0;
    }

    void computeBounds()
    {
        // MC 1.21: compute 使用 abs()，结果非负
        // minValue = 0.0
        // maxValue = maxRarity * noise.maxValue()
        // Type1 maxRarity = 2.0, Type2 maxRarity = 3.0
        const f64 maxNoiseVal = m_noise->maxValue();
        const f64 maxRarity = (m_type == WeirdScaledSamplerType::Type1) ? 2.0 : 3.0;
        m_minValue = 0.0;
        m_maxValue = maxRarity * maxNoiseVal;
    }
};

// ============================================================================
// Marker — MC 1.21 DensityFunctions.Marker
//
// 标记密度函数，用于在 NoiseChunk 构造时替换为特定实现。
// NoiseChunk::wrap() 遍历密度函数树，将 Marker 替换为：
// - Interpolated → NoiseInterpolator
// - CacheOnce → 带 interpolationCounter 的缓存
// - CacheAllInCell → CellCache
// - FlatCache → 区块级扁平缓存
// - Cache2D → 2D 位置缓存
// ============================================================================

/**
 * @brief 标记类型枚举
 *
 * MC 1.21 对应 DensityFunctions.Marker.Type。
 * 每种标记类型对应 NoiseChunk 中的特定替换实现。
 */
enum class MarkerType : u8 {
    Interpolated,    ///< 替换为 NoiseInterpolator（三线性插值）
    CacheOnce,       ///< 绑定 interpolationCounter 缓存
    CacheAllInCell,  ///< 替换为 CellCache（selectCellYZ 时预填充）
    FlatCache,       ///< 区块级扁平缓存（Y=0 的 2D 缓存）
    Cache2D,         ///< XZ 位置缓存
    BeardifierMarker ///< 替换为 Beardifier（结构物对地形的密度贡献），标记阶段返回 0.0
};

/**
 * @brief 标记密度函数 — MC 1.21 DensityFunctions.Marker
 *
 * 包装一个子函数并标记其类型。
 * 在 NoiseRouterData 工厂函数中使用 Marker 代替具体缓存实现，
 * NoiseChunk 构造时通过 wrap() 将 Marker 替换为 NoiseChunk 特定实现。
 */
class Marker final : public DensityFunction {
public:
    Marker(MarkerType type, std::unique_ptr<DensityFunction> wrapped)
        : m_type(type)
        , m_wrapped(std::move(wrapped))
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        return m_wrapped->compute(blockX, blockY, blockZ);
    }

    [[nodiscard]] f64 minValue() const override { return m_wrapped->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_wrapped->maxValue(); }

    [[nodiscard]] MarkerType markerType() const { return m_type; }
    [[nodiscard]] const DensityFunction& wrapped() const { return *m_wrapped; }

    /** 释放被包装的函数（用于 NoiseChunk::wrap 替换时取出子函数） */
    [[nodiscard]] std::unique_ptr<DensityFunction> releaseWrapped() { return std::move(m_wrapped); }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newWrapped = m_wrapped->mapAll(visitor);
        return visitor.apply(std::make_unique<Marker>(m_type, std::move(newWrapped)));
    }

private:
    MarkerType m_type;
    std::unique_ptr<DensityFunction> m_wrapped;
};

/**
 * @brief 共享持有密度函数 — 允许同一密度函数被多个父节点引用
 *
 * MC 1.21 使用 Holder<DensityFunction> 实现引用共享。
 * C++ 使用 unique_ptr 管理密度函数树的所有权，
 * 但 slopedCheese 等函数需要被多个父节点引用（caveCheeseFactor 和 rangeChoice）。
 *
 * SharedHolder 包装 shared_ptr<DensityFunction>，使得多个 SharedHolder 实例
 * 可以指向同一个底层密度函数。compute() 调用委托给共享的子函数。
 * mapAll() 递归到共享的子函数，并创建新的 SharedHolder 指向替换后的结果。
 *
 * 注意：NoiseChunk::apply() 不会对 SharedHolder 做特殊处理，
 * 因为 shared_ptr 指向的子函数在 mapAll 后已经是替换后的版本。
 */
class SharedHolder final : public DensityFunction {
public:
    explicit SharedHolder(std::shared_ptr<DensityFunction> shared)
        : m_shared(std::move(shared))
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        return m_shared->compute(blockX, blockY, blockZ);
    }

    [[nodiscard]] f64 minValue() const override { return m_shared->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_shared->maxValue(); }

    [[nodiscard]] const DensityFunction& inner() const { return *m_shared; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInner = m_shared->mapAll(visitor);
        return visitor.apply(std::make_unique<SharedHolder>(std::shared_ptr<DensityFunction>(std::move(newInner))));
    }

private:
    std::shared_ptr<DensityFunction> m_shared;
};

// ============================================================================
// EndIslands — 末地岛屿密度函数
// ============================================================================

/**
 * @brief 末地岛屿密度函数
 *
 * 参考 MC 1.21.11: DensityFunctions.EndIslandDensityFunction
 * 在 8 格间距网格上检测岛屿，使用 SimplexNoise 生成外岛。
 *
 * 算法流程：
 * 1. 将方块坐标缩放到 8 格间距网格
 * 2. 计算基础高度值（基于到原点距离）
 * 3. 遍历周围的 25x25 网格，检测外岛（距中心 > 64 区块 且 噪声 < -0.9）
 * 4. 返回 (height - 8.0) / 128.0
 */
class EndIslands final : public DensityFunction {
public:
    explicit EndIslands(u64 seed);

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override;

    [[nodiscard]] f64 minValue() const override { return -0.84375; }
    [[nodiscard]] f64 maxValue() const override { return 0.5625; }

    [[nodiscard]] u64 seed() const { return m_seed; }

    DENSITY_FUNCTION_MAP_ALL_LEAF(EndIslands, m_seed)

private:
    /// MC 1.21.11: 检测岛屿高度值（Java 返回 float）
    [[nodiscard]] f32 getHeightValue(i32 x, i32 z) const;

    u64 m_seed;
    std::unique_ptr<noise::SimplexNoise> m_islandNoise;
};

// ============================================================================
// UnboundNoiseLeaf — 数据驱动解析期噪声叶子占位
//
// density_function JSON 的 noise/shifted_noise/shift_a/shift_b/shift/
// mapped_noise/weird_scaled_sampler/old_blended_noise type 在解析期无法构造真实噪声
// 叶子（NormalNoise/BlendedNoise 需 RandomState 上下文，由 name-hash 派生）。解析期
// 存本占位，RandomState 组装 NoiseRouter 时由 NoiseBindingVisitor 遍历树，调用
// rs.getOrCreateRouterNoiseByName(name) 取 NormalNoise（或 old_blended_noise 走
// fromHashOf("minecraft:terrain")），替换为真正的 NoiseDensity/ShiftedNoise/MappedNoise/
// ShiftNoise/WeirdScaledSampler/BlendedNoise。
//
// 占位期 compute() 返回 0.0（组装前不应被采样）。mapAll 深拷贝自身及持有的子 DF。
// ============================================================================

/**
 * @brief 数据驱动解析期噪声叶子占位
 *
 * 持有构造真实噪声叶子所需的全部参数：noise 名（RL 字符串）、缩放、可选 fromValue/toValue
 * （MappedNoise）、可选 shiftX/Y/Z（ShiftedNoise）、可选 input（WeirdScaledSampler），
 * OldBlendedNoise 额外用 xzFactor/yFactor/smearScaleMultiplier，
 * 以及 Kind 标识要替换成哪个真叶子子类。
 */
class UnboundNoiseLeaf final : public DensityFunction {
public:
    enum class Kind : u8 {
        Noise,              ///< → NoiseDensity
        ShiftedNoise,       ///< → ShiftedNoise
        MappedNoise,        ///< → MappedNoise
        ShiftA,             ///< → ShiftNoise(ShiftA)
        ShiftB,             ///< → ShiftNoise(ShiftB)
        Shift,              ///< → ShiftNoise(Shift)
        WeirdScaledSampler, ///< → WeirdScaledSampler
        OldBlendedNoise     ///< → BlendedNoise（noiseName 固定 "minecraft:terrain"）
    };

    UnboundNoiseLeaf(std::string noiseName,
        f64 xzScale,
        f64 yScale,
        Kind kind,
        std::unique_ptr<DensityFunction> shiftX,
        std::unique_ptr<DensityFunction> shiftY,
        std::unique_ptr<DensityFunction> shiftZ,
        std::unique_ptr<DensityFunction> input,
        std::optional<f64> fromValue,
        std::optional<f64> toValue,
        WeirdScaledSamplerType weirdType,
        f64 xzFactor,
        f64 yFactor,
        f64 smearScaleMultiplier)
        : m_noiseName(std::move(noiseName))
        , m_xzScale(xzScale)
        , m_yScale(yScale)
        , m_kind(kind)
        , m_shiftX(std::move(shiftX))
        , m_shiftY(std::move(shiftY))
        , m_shiftZ(std::move(shiftZ))
        , m_input(std::move(input))
        , m_fromValue(fromValue)
        , m_toValue(toValue)
        , m_weirdType(weirdType)
        , m_xzFactor(xzFactor)
        , m_yFactor(yFactor)
        , m_smearScaleMultiplier(smearScaleMultiplier)
    {}

    [[nodiscard]] f64 compute(i32, i32, i32) const override { return 0.0; }
    [[nodiscard]] f64 minValue() const override { return 0.0; }
    [[nodiscard]] f64 maxValue() const override { return 0.0; }

    [[nodiscard]] const std::string& noiseName() const { return m_noiseName; }
    [[nodiscard]] f64 xzScale() const { return m_xzScale; }
    [[nodiscard]] f64 yScale() const { return m_yScale; }
    [[nodiscard]] Kind kind() const { return m_kind; }
    [[nodiscard]] const std::unique_ptr<DensityFunction>& shiftX() const { return m_shiftX; }
    [[nodiscard]] const std::unique_ptr<DensityFunction>& shiftY() const { return m_shiftY; }
    [[nodiscard]] const std::unique_ptr<DensityFunction>& shiftZ() const { return m_shiftZ; }
    [[nodiscard]] const std::unique_ptr<DensityFunction>& input() const { return m_input; }
    [[nodiscard]] std::optional<f64> fromValue() const { return m_fromValue; }
    [[nodiscard]] std::optional<f64> toValue() const { return m_toValue; }
    [[nodiscard]] WeirdScaledSamplerType weirdType() const { return m_weirdType; }
    [[nodiscard]] f64 xzFactor() const { return m_xzFactor; }
    [[nodiscard]] f64 yFactor() const { return m_yFactor; }
    [[nodiscard]] f64 smearScaleMultiplier() const { return m_smearScaleMultiplier; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        // 深拷贝：子 DF 递归 mapAll，标量字段复制
        auto shiftX = m_shiftX ? m_shiftX->mapAll(visitor) : nullptr;
        auto shiftY = m_shiftY ? m_shiftY->mapAll(visitor) : nullptr;
        auto shiftZ = m_shiftZ ? m_shiftZ->mapAll(visitor) : nullptr;
        auto input = m_input ? m_input->mapAll(visitor) : nullptr;
        return visitor.apply(std::make_unique<UnboundNoiseLeaf>(m_noiseName,
            m_xzScale,
            m_yScale,
            m_kind,
            std::move(shiftX),
            std::move(shiftY),
            std::move(shiftZ),
            std::move(input),
            m_fromValue,
            m_toValue,
            m_weirdType,
            m_xzFactor,
            m_yFactor,
            m_smearScaleMultiplier));
    }

private:
    std::string m_noiseName;
    f64 m_xzScale;
    f64 m_yScale;
    Kind m_kind;
    std::unique_ptr<DensityFunction> m_shiftX;
    std::unique_ptr<DensityFunction> m_shiftY;
    std::unique_ptr<DensityFunction> m_shiftZ;
    std::unique_ptr<DensityFunction> m_input;
    std::optional<f64> m_fromValue;
    std::optional<f64> m_toValue;
    WeirdScaledSamplerType m_weirdType;
    f64 m_xzFactor;             ///< OldBlendedNoise: xz_factor
    f64 m_yFactor;              ///< OldBlendedNoise: y_factor
    f64 m_smearScaleMultiplier; ///< OldBlendedNoise: smear_scale_multiplier
};

/**
 * @brief 数据驱动解析期末地岛屿占位
 *
 * end_islands type 解析期无参，但真实 EndIslands 需 seed（原版固定 0）。存本占位，
 * NoiseBindingVisitor 替换为 EndIslands(0)。占位期 compute 返回 0。
 */
class UnboundEndIslands final : public DensityFunction {
public:
    UnboundEndIslands() = default;

    [[nodiscard]] f64 compute(i32, i32, i32) const override { return 0.0; }
    [[nodiscard]] f64 minValue() const override { return -0.84375; }
    [[nodiscard]] f64 maxValue() const override { return 0.5625; }

    DENSITY_FUNCTION_MAP_ALL_LEAF(UnboundEndIslands)
};

// ============================================================================
// FindTopSurface — 查找顶部地表密度函数
//
// MC 1.21.11: DensityFunctions.FindTopSurface record
// 从 upperBound 向 lowerBound 方向逐 cellHeight 步查找，
// 返回第一个 density > 0.0 的 Y 坐标；若全部 ≤ 0.0 则返回 lowerBound。
//
// 用途：preliminarySurfaceLevel 中确定初步地表高度。
// ============================================================================

/**
 * @brief 查找顶部地表密度函数
 *
 * 算法：
 * 1. 计算 i = floor(upperBound.compute() / cellHeight) * cellHeight
 * 2. 若 i <= lowerBound，直接返回 lowerBound
 * 3. 否则从 j = i 开始向下逐 cellHeight 步采样 density.compute(blockX, j, blockZ)
 * 4. 返回第一个 > 0.0 的 j；若全部 ≤ 0.0 则返回 lowerBound
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.DensityFunctions.FindTopSurface
 */
class FindTopSurface final : public DensityFunction {
public:
    FindTopSurface(std::unique_ptr<DensityFunction> density,
        std::unique_ptr<DensityFunction> upperBound,
        i32 lowerBound,
        i32 cellHeight)
        : m_density(std::move(density))
        , m_upperBound(std::move(upperBound))
        , m_lowerBound(lowerBound)
        , m_cellHeight(cellHeight)
        , m_minValue(static_cast<f64>(lowerBound))
        , m_maxValue(std::max(static_cast<f64>(lowerBound), m_upperBound->maxValue()))
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 upperVal = m_upperBound->compute(blockX, blockY, blockZ);
        const i32 i = static_cast<i32>(std::floor(upperVal / static_cast<f64>(m_cellHeight))) * m_cellHeight;
        if (i <= m_lowerBound) {
            return static_cast<f64>(m_lowerBound);
        }
        for (i32 j = i; j >= m_lowerBound; j -= m_cellHeight) {
            if (m_density->compute(blockX, j, blockZ) > 0.0) {
                return static_cast<f64>(j);
            }
        }
        return static_cast<f64>(m_lowerBound);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const DensityFunction& density() const { return *m_density; }
    [[nodiscard]] const DensityFunction& upperBound() const { return *m_upperBound; }
    [[nodiscard]] i32 lowerBound() const { return m_lowerBound; }
    [[nodiscard]] i32 cellHeight() const { return m_cellHeight; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newDensity = m_density->mapAll(visitor);
        auto newUpperBound = m_upperBound->mapAll(visitor);
        return visitor.apply(std::make_unique<FindTopSurface>(
            std::move(newDensity), std::move(newUpperBound), m_lowerBound, m_cellHeight));
    }

private:
    std::unique_ptr<DensityFunction> m_density;
    std::unique_ptr<DensityFunction> m_upperBound;
    i32 m_lowerBound;
    i32 m_cellHeight;
    f64 m_minValue;
    f64 m_maxValue;
};

// ============================================================================
// 工厂函数
// ============================================================================

namespace factory {

/**
 * @brief 创建常量密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> constant(f64 value);

/**
 * @brief 创建 Y 轴钳制梯度
 */
[[nodiscard]] std::unique_ptr<DensityFunction> yClampedGradient(i32 fromY, i32 toY, f64 fromValue, f64 toValue);

/**
 * @brief 创建钳制密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> clamp(
    std::unique_ptr<DensityFunction> input, f64 minValue, f64 maxValue);

/**
 * @brief 创建绝对值密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> abs(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建平方密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> square(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建立方密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> cube(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建半负密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> halfNegative(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建四分之一负密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> quarterNegative(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建压缩密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> squeeze(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建反转密度函数 (1/x)
 */
[[nodiscard]] std::unique_ptr<DensityFunction> invert(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建加法密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> add(
    std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2);

/**
 * @brief 创建乘法密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> mul(
    std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2);

/**
 * @brief 创建最小值密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> min(
    std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2);

/**
 * @brief 创建最大值密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> max(
    std::unique_ptr<DensityFunction> arg1, std::unique_ptr<DensityFunction> arg2);

/**
 * @brief 创建噪声密度函数
 *
 * 从 RandomState 的派生种子缓存获取 NormalNoise，跨区块复用，
 * 避免每区块 createRouterCopy 时重建 PerlinNoise 倍频置换表。
 * derivedSeed 通常是 worldSeed ^ 噪声常量。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> noise(
    const RandomState& rs, u64 derivedSeed, i32 firstOctave, std::vector<f64> amplitudes, f64 xzScale, f64 yScale);

/**
 * @brief 创建带偏移的噪声密度函数
 *
 * 从 RandomState 缓存获取 NormalNoise，跨区块复用。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shiftedNoise(const RandomState& rs,
    u64 derivedSeed,
    i32 firstOctave,
    std::vector<f64> amplitudes,
    f64 xzScale,
    f64 yScale,
    std::unique_ptr<DensityFunction> shiftX,
    std::unique_ptr<DensityFunction> shiftY,
    std::unique_ptr<DensityFunction> shiftZ);

/**
 * @brief 创建 2D 带偏移噪声（yScale=0, shiftY=zero）
 *
 * MC 1.21 的气候参数（temperature, vegetation, continents, erosion, ridges）
 * 均使用此函数。从 RandomState 缓存获取 NormalNoise，跨区块复用。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shiftedNoise2d(const RandomState& rs,
    std::unique_ptr<DensityFunction> shiftX,
    std::unique_ptr<DensityFunction> shiftZ,
    f64 xzScale,
    u64 derivedSeed,
    i32 firstOctave,
    std::vector<f64> amplitudes);

/**
 * @brief 创建线性插值密度函数
 *
 * lerp(delta, start, end) = start + delta * (end - start)
 * MC 1.21 用于 spline 系统中的插值。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> lerp(std::unique_ptr<DensityFunction> delta,
    std::unique_ptr<DensityFunction> start,
    std::unique_ptr<DensityFunction> end);

/**
 * @brief 创建 ShiftA 偏移噪声
 *
 * 从 RandomState 缓存获取 NormalNoise，跨区块复用。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shiftA(
    const RandomState& rs, u64 derivedSeed, i32 firstOctave, std::vector<f64> amplitudes);

/**
 * @brief 创建 ShiftB 偏移噪声
 *
 * 从 RandomState 缓存获取 NormalNoise，跨区块复用。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shiftB(
    const RandomState& rs, u64 derivedSeed, i32 firstOctave, std::vector<f64> amplitudes);

/**
 * @brief 创建 Shift 偏移噪声
 *
 * 从 RandomState 缓存获取 NormalNoise，跨区块复用。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shift(
    const RandomState& rs, u64 derivedSeed, i32 firstOctave, std::vector<f64> amplitudes);

/**
 * @brief 创建 RangeChoice 条件选择
 */
[[nodiscard]] std::unique_ptr<DensityFunction> rangeChoice(std::unique_ptr<DensityFunction> input,
    f64 minInclusive,
    f64 maxExclusive,
    std::unique_ptr<DensityFunction> whenInRange,
    std::unique_ptr<DensityFunction> whenOutOfRange);

/**
 * @brief 创建嵌套样条密度函数（CubicSpline）
 *
 * 每个控制点的 value 可以是 f64 常量或嵌套 CubicSpline。
 * 这是 MC 1.21 多维度样条树的核心构建方式。
 * 输入密度函数使用 shared_ptr，以支持多个子样条共享同一输入。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> cubicSpline(
    std::shared_ptr<DensityFunction> input, std::vector<SplinePoint> points);

/**
 * @brief 创建扁平样条密度函数（兼容旧接口）
 *
 * 所有控制点的 value 都是 f64 常量，无嵌套。
 * 输入密度函数使用 shared_ptr。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> spline(
    std::shared_ptr<DensityFunction> input, std::vector<FlatSplinePoint> points);

/**
 * @brief 创建扁平样条密度函数（unique_ptr 输入版本）
 *
 * 输入密度函数使用 unique_ptr，内部转换为 shared_ptr。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> spline(
    std::unique_ptr<DensityFunction> input, std::vector<FlatSplinePoint> points);

/**
 * @brief 创建共享持有密度函数
 *
 * 将 unique_ptr 转换为 shared_ptr 包装的 SharedHolder，
 * 允许同一密度函数被多个父节点引用。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> sharedHolder(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建共享持有密度函数（从已有 shared_ptr）
 */
[[nodiscard]] std::unique_ptr<DensityFunction> sharedHolder(std::shared_ptr<DensityFunction> shared);

/**
 * @brief 创建 2D 缓存
 */
[[nodiscard]] std::unique_ptr<DensityFunction> cache2D(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建区块级扁平缓存
 */
[[nodiscard]] std::unique_ptr<DensityFunction> flatCache(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建区块内全缓存
 */
[[nodiscard]] std::unique_ptr<DensityFunction> cacheAllInCell(std::unique_ptr<DensityFunction> input);

/**
 * @brief 创建奇异缩放采样器
 *
 * 从 RandomState 缓存获取 NormalNoise，跨区块复用。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> weirdScaledSampler(std::unique_ptr<DensityFunction> input,
    const RandomState& rs,
    u64 derivedSeed,
    i32 firstOctave,
    std::vector<f64> amplitudes,
    WeirdScaledSamplerType type);

/**
 * @brief 创建带输出重映射的噪声密度函数
 *
 * compute = fromValue + noise(x*xzScale, y*yScale, z*xzScale) * (toValue - fromValue)
 * 从 RandomState 缓存获取 NormalNoise，跨区块复用。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> mappedNoise(const RandomState& rs,
    u64 derivedSeed,
    i32 firstOctave,
    std::vector<f64> amplitudes,
    f64 xzScale,
    f64 yScale,
    f64 fromValue,
    f64 toValue);

/**
 * @brief 创建末地岛屿密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> endIslands(u64 seed);

/**
 * @brief 创建查找顶部地表密度函数
 *
 * MC 1.21.11: DensityFunctions.FindTopSurface
 * 从 upperBound 向 lowerBound 方向逐 cellHeight 步查找第一个 density > 0.0 的 Y 坐标。
 *
 * @param density 采样用的密度函数（通常为地形密度）
 * @param upperBound 搜索上界（动态计算）
 * @param lowerBound 搜索下界（静态，返回值不低于此值）
 * @param cellHeight 步长（通常为 NoiseSettings::getCellHeight()）
 */
[[nodiscard]] std::unique_ptr<DensityFunction> findTopSurface(std::unique_ptr<DensityFunction> density,
    std::unique_ptr<DensityFunction> upperBound,
    i32 lowerBound,
    i32 cellHeight);

/**
 * @brief 创建混合噪声密度函数（旧式三层 Perlin 噪声）
 *
 * MC 1.21: BlendedNoise 密度函数，用于 BASE_3D_NOISE。
 * 主世界参数: (0.25, 0.125, 80.0, 160.0, 8.0)
 * 下界参数:   (0.25, 0.375, 80.0, 60.0, 8.0)
 * 末地参数:   (0.25, 0.25, 80.0, 160.0, 4.0)
 *
 * @param seed 世界种子
 * @param xzScale XZ 方向缩放因子
 * @param yScale Y 方向缩放因子
 * @param xzFactor XZ 方向因子
 * @param yFactor Y 方向因子
 * @param smearScaleMultiplier Y 方向涂抹缩放乘数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> blendedNoise(
    u64 seed, f64 xzScale, f64 yScale, f64 xzFactor, f64 yFactor, f64 smearScaleMultiplier);

/**
 * @brief 创建标记密度函数（Interpolated 类型）
 *
 * MC 1.21: NoiseRouterData 使用 Marker 代替直接创建 NoiseInterpolator。
 * NoiseChunk::wrap() 在构造时替换为 NoiseInterpolator。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> interpolated(std::unique_ptr<DensityFunction> wrapped);

/**
 * @brief 创建标记密度函数（CacheOnce 类型）
 *
 * MC 1.21: NoiseRouterData 使用 Marker 代替直接创建 CacheOnce。
 * NoiseChunk::wrap() 在构造时替换为带 interpolationCounter 的缓存。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> cacheOnce(std::unique_ptr<DensityFunction> wrapped);

/**
 * @brief 创建标记密度函数（CacheAllInCell 类型）
 *
 * MC 1.21: NoiseRouterData 使用 Marker 代替直接创建 CellCache。
 * NoiseChunk::wrap() 在构造时替换为 CellCache。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> cacheAllInCellMarker(std::unique_ptr<DensityFunction> wrapped);

/**
 * @brief 创建标记密度函数（FlatCache 类型）
 *
 * MC 1.21: NoiseRouterData 使用 Marker 代替直接创建 FlatCache。
 * NoiseChunk::wrap() 在构造时替换为区块级扁平缓存。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> flatCacheMarker(std::unique_ptr<DensityFunction> wrapped);

/**
 * @brief 创建标记密度函数（Cache2D 类型）
 *
 * MC 1.21: NoiseRouterData 使用 Marker 代替直接创建 Cache2D。
 * NoiseChunk::wrap() 在构造时替换为 2D 位置缓存。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> cache2DMarker(std::unique_ptr<DensityFunction> wrapped);

/**
 * @brief 创建标记密度函数（BeardifierMarker 类型）
 *
 * MC 1.21: BeardifierMarker 是占位标记，在密度函数树中返回 0.0。
 * NoiseChunk 构造时将其替换为实际的 Beardifier（基于结构物的密度贡献）。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> beardifierMarker();

} // namespace factory

} // namespace mc::world::gen::density
