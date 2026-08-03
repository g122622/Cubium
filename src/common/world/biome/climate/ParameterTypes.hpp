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
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <array>

namespace mc::world::biome::climate {

// ============================================================================
// 常量
// ============================================================================

/// 量化因子：将浮点气候参数转换为整数以优化比较性能
inline constexpr f32 QUANTIZATION_FACTOR = 10000.0f;

/// 气候参数维度数量（temperature, humidity, continentalness, erosion, depth, weirdness + offset = 7）
inline constexpr i32 PARAMETER_COUNT = 7;

// ============================================================================
// Parameter - 气候参数范围
// ============================================================================

/**
 * @brief 气候参数范围
 *
 * 使用量化整数存储参数范围 [min, max]，优化最近邻搜索性能。
 * 每个气候参数（temperature, humidity, continentalness, erosion, depth, weirdness）
 * 都用 Parameter 定义其匹配范围。
 */
struct Parameter {
    i64 min;
    i64 max;

    /** 创建单点参数（min == max） */
    static Parameter point(f32 value)
    {
        const auto q = static_cast<i64>(value * QUANTIZATION_FACTOR);
        return {q, q};
    }

    /** 创建范围参数 */
    static Parameter span(f32 minValue, f32 maxValue)
    {
        MC_ASSERT_RELEASE(minValue <= maxValue && "Climate::Parameter::span: minValue must be <= maxValue");
        return {static_cast<i64>(minValue * QUANTIZATION_FACTOR), static_cast<i64>(maxValue * QUANTIZATION_FACTOR)};
    }

    /** 从两个参数的范围创建跨度参数（使用 first.min 和 second.max） */
    static Parameter span(const Parameter& first, const Parameter& second)
    {
        MC_ASSERT_RELEASE(first.min <= second.max && "Climate::Parameter::span: first.min must be <= second.max");
        return {first.min, second.max};
    }

    /**
     * @brief 合并两个参数的范围（用于 RTree 构建时的参数空间合并）
     *
     * 返回 min(a.min, b.min) ~ max(a.max, b.max) 的范围。
     */
    [[nodiscard]] static Parameter merge(const Parameter& a, const Parameter& b)
    {
        return {std::min(a.min, b.min), std::max(a.max, b.max)};
    }

    /** 全范围参数 [-2, 2] */
    static Parameter fullRange() { return span(-2.0f, 2.0f); }

    /**
     * @brief 计算量化值到此参数范围的距离
     *
     * 若值在范围内返回 0，否则返回到最近边界的距离。
     */
    [[nodiscard]] i64 distance(i64 value) const
    {
        const i64 above = value - max;
        const i64 below = min - value;
        return above > 0 ? above : std::max(below, i64{0});
    }

    [[nodiscard]] bool operator==(const Parameter& other) const { return min == other.min && max == other.max; }

    [[nodiscard]] bool operator!=(const Parameter& other) const { return !(*this == other); }
};

// ============================================================================
// TargetPoint - 气候采样目标点
// ============================================================================

/**
 * @brief 气候采样目标点
 *
 * 由 Sampler 在指定位置采样得到的 6 个气候参数值。
 * 所有值已量化为整数，用于与 ParameterPoint 进行最近邻匹配。
 */
struct TargetPoint {
    i64 temperature;
    i64 humidity;
    i64 continentalness;
    i64 erosion;
    i64 depth;
    i64 weirdness;

    /** 从浮点值创建 TargetPoint */
    static TargetPoint fromFloats(f32 temp, f32 humid, f32 cont, f32 ero, f32 dep, f32 weird)
    {
        return {static_cast<i64>(temp * QUANTIZATION_FACTOR),
            static_cast<i64>(humid * QUANTIZATION_FACTOR),
            static_cast<i64>(cont * QUANTIZATION_FACTOR),
            static_cast<i64>(ero * QUANTIZATION_FACTOR),
            static_cast<i64>(dep * QUANTIZATION_FACTOR),
            static_cast<i64>(weird * QUANTIZATION_FACTOR)};
    }

    /** 转换为参数数组（7个元素，最后一个是offset=0） */
    [[nodiscard]] std::array<i64, 7> toParameterArray() const
    {
        return {temperature, humidity, continentalness, erosion, depth, weirdness, 0};
    }
};

// ============================================================================
// ParameterPoint - 气候参数定义点
// ============================================================================

/**
 * @brief 气候参数定义点
 *
 * 定义一个生物群系所需的气候条件范围。
 * 每个生物群系注册一到多个 ParameterPoint（如表面和地下各一个）。
 * 通过与 TargetPoint 的 fitness 计算进行最近邻匹配。
 */
struct ParameterPoint {
    Parameter temperature;
    Parameter humidity;
    Parameter continentalness;
    Parameter erosion;
    Parameter depth;
    Parameter weirdness;
    i64 offset;

    /**
     * @brief 计算与目标点的适配度（距离的平方和）
     *
     * 值越小表示越匹配。用于在 ParameterList 中查找最匹配的生物群系。
     */
    [[nodiscard]] i64 fitness(const TargetPoint& target) const
    {
        return temperature.distance(target.temperature) * temperature.distance(target.temperature) +
            humidity.distance(target.humidity) * humidity.distance(target.humidity) +
            continentalness.distance(target.continentalness) * continentalness.distance(target.continentalness) +
            erosion.distance(target.erosion) * erosion.distance(target.erosion) +
            depth.distance(target.depth) * depth.distance(target.depth) +
            weirdness.distance(target.weirdness) * weirdness.distance(target.weirdness) + offset * offset;
    }

    [[nodiscard]] bool operator==(const ParameterPoint& other) const
    {
        return temperature == other.temperature && humidity == other.humidity &&
            continentalness == other.continentalness && erosion == other.erosion && depth == other.depth &&
            weirdness == other.weirdness && offset == other.offset;
    }

    /**
     * @brief 获取参数空间数组（7 个 Parameter，用于 RTree 构建）
     *
     * 将 6 个气候参数和 offset 转换为 7 个 Parameter 对象数组。
     * offset 被包装为 point(offset, offset) 形式的 Parameter。
     */
    [[nodiscard]] std::array<Parameter, 7> parameterSpace() const
    {
        return {temperature, humidity, continentalness, erosion, depth, weirdness, Parameter{offset, offset}};
    }
};

// ============================================================================
// 辅助函数
// ============================================================================

/** 量化浮点气候值为整数 */
[[nodiscard]] inline i64 quantizeCoord(f32 value)
{
    return static_cast<i64>(value * QUANTIZATION_FACTOR);
}

/** 反量化整数值为浮点 */
[[nodiscard]] inline f32 unquantizeCoord(i64 value)
{
    return static_cast<f32>(value) / QUANTIZATION_FACTOR;
}

/**
 * @brief 快速创建 ParameterPoint 的便捷函数
 *
 * 所有浮点参数自动量化。offset 默认为 0。
 */
[[nodiscard]] inline ParameterPoint parameters(Parameter temperature,
    Parameter humidity,
    Parameter continentalness,
    Parameter erosion,
    Parameter depth,
    Parameter weirdness,
    f32 offset = 0.0f)
{
    return {temperature, humidity, continentalness, erosion, depth, weirdness, quantizeCoord(offset)};
}

/**
 * @brief 使用浮点值创建 ParameterPoint 的便捷函数
 *
 * 所有参数使用 point() 创建（单点匹配）。
 */
[[nodiscard]] inline ParameterPoint pointParameters(
    f32 temperature, f32 humidity, f32 continentalness, f32 erosion, f32 depth, f32 weirdness, f32 offset = 0.0f)
{
    return {Parameter::point(temperature),
        Parameter::point(humidity),
        Parameter::point(continentalness),
        Parameter::point(erosion),
        Parameter::point(depth),
        Parameter::point(weirdness),
        quantizeCoord(offset)};
}

} // namespace mc::world::biome::climate
