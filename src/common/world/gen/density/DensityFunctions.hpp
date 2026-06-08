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
#include <vector>

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
                return value == 0.0 ? 0.0 : 1.0 / value;
        }
        return 0.0;
    }

    void computeBounds()
    {
        const f64 inMin = m_input->minValue();
        const f64 inMax = m_input->maxValue();

        switch (m_type) {
            case MappedType::Abs:
                m_minValue = std::max(0.0, std::min(std::abs(inMin), std::abs(inMax)));
                m_maxValue = std::max(std::abs(inMin), std::abs(inMax));
                break;
            case MappedType::Square: {
                const f64 minSq = inMin * inMin;
                const f64 maxSq = inMax * inMax;
                m_minValue = inMin < 0.0 && inMax > 0.0 ? 0.0 : std::min(minSq, maxSq);
                m_maxValue = std::max(minSq, maxSq);
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
            case MappedType::Invert: {
                // 1/x 的范围取决于输入范围是否跨零
                if (inMin > 0.0) {
                    m_minValue = 1.0 / inMax;
                    m_maxValue = 1.0 / inMin;
                } else if (inMax < 0.0) {
                    m_minValue = 1.0 / inMin;
                    m_maxValue = 1.0 / inMax;
                } else {
                    // 跨越零点，范围无界
                    m_minValue = -1e6;
                    m_maxValue = 1e6;
                }
                break;
            }
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
                // 考虑符号组合
                if (min1 > 0.0 && min2 > 0.0) {
                    m_minValue = min1 * min2;
                } else if (max1 < 0.0 && max2 < 0.0) {
                    m_minValue = max1 * max2;
                } else {
                    m_minValue = std::min(min1 * max2, max1 * min2);
                }
                if (min1 > 0.0 && max2 > 0.0) {
                    m_maxValue = max1 * max2;
                } else if (max1 < 0.0 && min2 < 0.0) {
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
 * MC 1.21 用于 BlendAlpha/BlendOffset 混合以及 spline 系统中的插值。
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
    MappedNoise(std::unique_ptr<noise::NormalNoise> noise, f64 xzScale, f64 yScale, f64 fromValue, f64 toValue)
        : m_noise(std::move(noise))
        , m_xzScale(xzScale)
        , m_yScale(yScale)
        , m_fromValue(fromValue)
        , m_toValue(toValue)
    {
        const f64 maxNoise = m_noise->maxValue();
        const f64 scale = toValue - fromValue;
        m_minValue = fromValue - maxNoise * std::abs(scale);
        m_maxValue = fromValue + maxNoise * std::abs(scale);
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

    DENSITY_FUNCTION_MAP_ALL_LEAF(MappedNoise, m_noise->clone(), m_xzScale, m_yScale, m_fromValue, m_toValue)

private:
    std::unique_ptr<noise::NormalNoise> m_noise;
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
    NoiseDensity(std::unique_ptr<noise::NormalNoise> noise, f64 xzScale, f64 yScale)
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

    DENSITY_FUNCTION_MAP_ALL_LEAF(NoiseDensity, m_noise->clone(), m_xzScale, m_yScale)

private:
    std::unique_ptr<noise::NormalNoise> m_noise;
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
    ShiftedNoise(std::unique_ptr<noise::NormalNoise> noise,
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

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newShiftX = m_shiftX->mapAll(visitor);
        auto newShiftY = m_shiftY->mapAll(visitor);
        auto newShiftZ = m_shiftZ->mapAll(visitor);
        return visitor.apply(std::make_unique<ShiftedNoise>(
            m_noise->clone(), m_xzScale, m_yScale, std::move(newShiftX), std::move(newShiftY), std::move(newShiftZ)));
    }

private:
    std::unique_ptr<noise::NormalNoise> m_noise;
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
    ShiftNoise(std::unique_ptr<noise::NormalNoise> noise, ShiftType type)
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

    DENSITY_FUNCTION_MAP_ALL_LEAF(ShiftNoise, m_noise->clone(), m_type)

private:
    std::unique_ptr<noise::NormalNoise> m_noise;
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
// Spline — 样条插值密度函数
// ============================================================================

/**
 * @brief 样条点
 *
 * 定义样条插值的一个控制点，包含位置、值和导数。
 */
struct SplinePoint {
    f64 location;
    f64 value;
    f64 derivative;
};

/**
 * @brief 样条密度函数
 *
 * 使用三次 Hermite 样条插值，根据输入密度函数的输出值计算结果。
 * MC 1.21 用于地形高度和气候参数的精细控制。
 */
class Spline final : public DensityFunction {
public:
    /**
     * @brief 使用样条点列表构造
     * @param input 输入密度函数
     * @param points 样条控制点（必须按 location 升序排列）
     */
    Spline(std::unique_ptr<DensityFunction> input, std::vector<SplinePoint> points)
        : m_input(std::move(input))
        , m_points(std::move(points))
    {
        MC_ASSERT_RELEASE(!m_points.empty());
        computeBounds();
    }

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        const f64 coordinate = m_input->compute(blockX, blockY, blockZ);
        return applySpline(coordinate);
    }

    [[nodiscard]] f64 minValue() const override { return m_minValue; }
    [[nodiscard]] f64 maxValue() const override { return m_maxValue; }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }
    [[nodiscard]] const std::vector<SplinePoint>& points() const { return m_points; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<Spline>(std::move(newInput), m_points));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    std::vector<SplinePoint> m_points;
    f64 m_minValue = 0.0;
    f64 m_maxValue = 0.0;

    [[nodiscard]] f64 applySpline(f64 coordinate) const
    {
        const size_t n = m_points.size();

        // 在第一个点之前 - 线性外推
        if (coordinate <= m_points[0].location) {
            const f64 slope = m_points[0].derivative;
            return m_points[0].value + slope * (coordinate - m_points[0].location);
        }

        // 在最后一个点之后 - 线性外推
        if (coordinate >= m_points[n - 1].location) {
            const f64 slope = m_points[n - 1].derivative;
            return m_points[n - 1].value + slope * (coordinate - m_points[n - 1].location);
        }

        // 二分查找区间
        size_t lo = 0;
        size_t hi = n - 2;
        while (lo < hi) {
            const size_t mid = (lo + hi + 1) / 2;
            if (m_points[mid].location <= coordinate) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }

        // 三次 Hermite 插值
        const f64 loc0 = m_points[lo].location;
        const f64 loc1 = m_points[lo + 1].location;
        const f64 val0 = m_points[lo].value;
        const f64 val1 = m_points[lo + 1].value;
        const f64 der0 = m_points[lo].derivative;
        const f64 der1 = m_points[lo + 1].derivative;

        const f64 width = loc1 - loc0;
        const f64 t = (coordinate - loc0) / width;

        // 缩放导数到区间宽度
        const f64 d0 = der0 * width;
        const f64 d1 = der1 * width;

        // Hermite 基函数
        const f64 a = d0 - (val1 - val0);
        const f64 b = -d1 + (val1 - val0);

        return val0 * (1.0 - t) + val1 * t + t * (1.0 - t) * (a * (1.0 - t) + b * t);
    }

    void computeBounds()
    {
        // 保守估计：遍历样条点计算值范围
        m_minValue = std::numeric_limits<f64>::max();
        m_maxValue = std::numeric_limits<f64>::lowest();

        for (const auto& point : m_points) {
            m_minValue = std::min(m_minValue, point.value);
            m_maxValue = std::max(m_maxValue, point.value);
        }

        // 考虑导数导致的超出范围
        // 简化处理：使用输入范围的首尾样条值
        const f64 inputMin = m_input->minValue();
        const f64 inputMax = m_input->maxValue();
        const f64 valAtMin = applySpline(inputMin);
        const f64 valAtMax = applySpline(inputMax);
        m_minValue = std::min({m_minValue, valAtMin, valAtMax});
        m_maxValue = std::max({m_maxValue, valAtMin, valAtMax});
    }
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
 * @brief 区块级扁平缓存密度函数
 *
 * 对每个区块位置（XZ），在 Y=0 处计算一次并缓存。
 * 适用于仅需要表面高度的密度函数。
 * 线程不安全，每个区块生成任务应有独立实例。
 */
class FlatCache final : public DensityFunction {
public:
    explicit FlatCache(std::unique_ptr<DensityFunction> input)
        : m_input(std::move(input))
        , m_cachedQuartX(0)
        , m_cachedQuartZ(0)
        , m_cachedValue(0.0)
        , m_valid(false)
    {}

    [[nodiscard]] f64 compute(i32 blockX, i32 blockY, i32 blockZ) const override
    {
        // quart 坐标 = floorDiv(block 坐标, 4)，负坐标下 >> 2 不是向下取整
        const i32 quartX = math::floorDiv(blockX, 4);
        const i32 quartZ = math::floorDiv(blockZ, 4);
        if (m_valid && quartX == m_cachedQuartX && quartZ == m_cachedQuartZ) {
            return m_cachedValue;
        }
        m_cachedQuartX = quartX;
        m_cachedQuartZ = quartZ;
        m_cachedValue = m_input->compute(quartX << 2, 0, quartZ << 2);
        m_valid = true;
        return m_cachedValue;
    }

    [[nodiscard]] f64 minValue() const override { return m_input->minValue(); }
    [[nodiscard]] f64 maxValue() const override { return m_input->maxValue(); }

    [[nodiscard]] const DensityFunction& input() const { return *m_input; }

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<FlatCache>(std::move(newInput)));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    mutable i32 m_cachedQuartX;
    mutable i32 m_cachedQuartZ;
    mutable f64 m_cachedValue;
    mutable bool m_valid;
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
    WeirdScaledSampler(
        std::unique_ptr<DensityFunction> input, std::unique_ptr<noise::NormalNoise> noise, WeirdScaledSamplerType type)
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

    [[nodiscard]] std::unique_ptr<DensityFunction> mapAll(Visitor& visitor) const override
    {
        auto newInput = m_input->mapAll(visitor);
        return visitor.apply(std::make_unique<WeirdScaledSampler>(std::move(newInput), m_noise->clone(), m_type));
    }

private:
    std::unique_ptr<DensityFunction> m_input;
    std::unique_ptr<noise::NormalNoise> m_noise;
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
        // 保守估计
        const f64 maxNoiseVal = m_noise->maxValue();
        m_minValue = -maxNoiseVal * 3.0;
        m_maxValue = maxNoiseVal * 3.0;
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
    Interpolated,   ///< 替换为 NoiseInterpolator（三线性插值）
    CacheOnce,      ///< 绑定 interpolationCounter 缓存
    CacheAllInCell, ///< 替换为 CellCache（selectCellYZ 时预填充）
    FlatCache,      ///< 区块级扁平缓存（Y=0 的 2D 缓存）
    Cache2D         ///< XZ 位置缓存
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
    /// MC 1.21: 检测岛屿高度值
    [[nodiscard]] f64 getHeightValue(i32 x, i32 z) const;

    u64 m_seed;
    std::unique_ptr<noise::SimplexNoise> m_islandNoise;
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
 */
[[nodiscard]] std::unique_ptr<DensityFunction> noise(
    u64 seed, i32 firstOctave, std::vector<f64> amplitudes, f64 xzScale, f64 yScale);

/**
 * @brief 创建带偏移的噪声密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shiftedNoise(u64 seed,
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
 * 均使用此函数。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shiftedNoise2d(std::unique_ptr<DensityFunction> shiftX,
    std::unique_ptr<DensityFunction> shiftZ,
    f64 xzScale,
    u64 seed,
    i32 firstOctave,
    std::vector<f64> amplitudes);

/**
 * @brief 创建线性插值密度函数
 *
 * lerp(delta, start, end) = start + delta * (end - start)
 * MC 1.21 用于 BlendAlpha/BlendOffset 混合。
 */
[[nodiscard]] std::unique_ptr<DensityFunction> lerp(std::unique_ptr<DensityFunction> delta,
    std::unique_ptr<DensityFunction> start,
    std::unique_ptr<DensityFunction> end);

/**
 * @brief 创建 ShiftA 偏移噪声
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shiftA(u64 seed, i32 firstOctave, std::vector<f64> amplitudes);

/**
 * @brief 创建 ShiftB 偏移噪声
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shiftB(u64 seed, i32 firstOctave, std::vector<f64> amplitudes);

/**
 * @brief 创建 Shift 偏移噪声
 */
[[nodiscard]] std::unique_ptr<DensityFunction> shift(u64 seed, i32 firstOctave, std::vector<f64> amplitudes);

/**
 * @brief 创建 RangeChoice 条件选择
 */
[[nodiscard]] std::unique_ptr<DensityFunction> rangeChoice(std::unique_ptr<DensityFunction> input,
    f64 minInclusive,
    f64 maxExclusive,
    std::unique_ptr<DensityFunction> whenInRange,
    std::unique_ptr<DensityFunction> whenOutOfRange);

/**
 * @brief 创建样条密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> spline(
    std::unique_ptr<DensityFunction> input, std::vector<SplinePoint> points);

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
 */
[[nodiscard]] std::unique_ptr<DensityFunction> weirdScaledSampler(std::unique_ptr<DensityFunction> input,
    u64 seed,
    i32 firstOctave,
    std::vector<f64> amplitudes,
    WeirdScaledSamplerType type);

/**
 * @brief 创建带输出重映射的噪声密度函数
 *
 * compute = fromValue + noise(x*xzScale, y*yScale, z*xzScale) * (toValue - fromValue)
 */
[[nodiscard]] std::unique_ptr<DensityFunction> mappedNoise(
    u64 seed, i32 firstOctave, std::vector<f64> amplitudes, f64 xzScale, f64 yScale, f64 fromValue, f64 toValue);

/**
 * @brief 创建末地岛屿密度函数
 */
[[nodiscard]] std::unique_ptr<DensityFunction> endIslands(u64 seed);

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

} // namespace factory

} // namespace mc::world::gen::density
