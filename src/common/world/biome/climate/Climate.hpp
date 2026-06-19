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
#include "common/world/block/BlockPos.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

// 前向声明 DensityFunction（定义在 density/ 目录）
namespace mc::world::gen::density {
class DensityFunction;
}

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
     * 如果 other 为 nullptr，返回当前参数自身。
     * 否则返回 min(this.min, other.min) ~ max(this.max, other.max) 的范围。
     */
    [[nodiscard]] static Parameter merge(const Parameter& a, const Parameter& b)
    {
        return {std::min(a.min, b.min), std::max(a.max, b.max)};
    }

    /**
     * @brief 计算两个参数范围之间的距离
     *
     * 用于 RTree 构建时计算节点间距离。
     * 如果两个范围重叠，返回 0。
     */
    [[nodiscard]] i64 distance(const Parameter& other) const
    {
        const i64 above = min - other.max;
        const i64 below = other.min - max;
        return above > 0 ? above : (below > 0 ? below : i64{0});
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
 * 由 Climate.Sampler 在指定位置采样得到的 6 个气候参数值。
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
     * MC 1.21.11: Climate.ParameterPoint.parameterSpace()
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

// ============================================================================
// ParameterList - 参数列表 + 最近邻搜索
// ============================================================================

/**
 * @brief 参数列表，支持基于最近邻匹配的生物群系查找
 *
 * 存储 ParameterPoint → T 的映射，通过 fitness() 查找最匹配的值。
 * 原版使用 RTree 加速搜索，当前实现使用线性搜索，
 * 后续可根据性能需求替换为 RTree。
 */
template <typename T>
class ParameterList {
public:
    using Entry = std::pair<ParameterPoint, T>;

    ParameterList() = default;
    explicit ParameterList(std::vector<Entry> entries)
        : m_entries(std::move(entries))
    {}

    /** 添加条目 */
    void add(ParameterPoint point, T value) { m_entries.emplace_back(std::move(point), std::move(value)); }

    /** 获取所有条目 */
    [[nodiscard]] const std::vector<Entry>& entries() const { return m_entries; }

    /** 迭代器支持 */
    auto begin() const { return m_entries.begin(); }
    auto end() const { return m_entries.end(); }

    /** 条目数量 */
    [[nodiscard]] size_t size() const { return m_entries.size(); }

    /** 是否为空 */
    [[nodiscard]] bool empty() const { return m_entries.empty(); }

    /**
     * @brief 查找最匹配的值
     *
     * 通过计算每个 ParameterPoint 与 target 的 fitness，
     * 返回 fitness 最小（最匹配）的值。
     */
    [[nodiscard]] const T& findValue(const TargetPoint& target) const
    {
        MC_ASSERT_RELEASE(!m_entries.empty());

        i64 bestFitness = std::numeric_limits<i64>::max();
        size_t bestIndex = 0;

        for (size_t i = 0; i < m_entries.size(); ++i) {
            const i64 fitness = m_entries[i].first.fitness(target);
            if (fitness < bestFitness) {
                bestFitness = fitness;
                bestIndex = i;
            }
        }

        return m_entries[bestIndex].second;
    }

    /**
     * @brief 暴力搜索最匹配的值（用于测试验证 RTree 结果）
     *
     * 与 findValue 算法相同，但使用 MC 1.21.11 风格的初始化方式：
     * 从第一个元素的 fitness 开始比较。
     * MC: Climate.ParameterList.findValueBruteForce()
     */
    [[nodiscard]] const T& findValueBruteForce(const TargetPoint& target) const
    {
        MC_ASSERT_RELEASE(!m_entries.empty());

        auto it = m_entries.begin();
        i64 bestFitness = it->first.fitness(target);
        const T* bestValue = &it->second;
        ++it;

        for (; it != m_entries.end(); ++it) {
            const i64 fitness = it->first.fitness(target);
            if (fitness < bestFitness) {
                bestFitness = fitness;
                bestValue = &it->second;
            }
        }

        return *bestValue;
    }

private:
    std::vector<Entry> m_entries;
};

// ============================================================================
// Sampler
// ============================================================================

/**
 * @brief 气候采样器
 *
 * 持有 6 个密度函数引用，在任意 3D 位置采样气候参数值。
 * 密度函数的实例由 NoiseRouter 创建并持有，Sampler 仅引用。
 */
class Sampler {
public:
    /**
     * @brief 构造气候采样器
     *
     * @param temperature 温度密度函数
     * @param humidity 湿度密度函数
     * @param continentalness 大陆度密度函数
     * @param erosion 侵蚀密度函数
     * @param depth 深度密度函数
     * @param weirdness 奇异度密度函数
     */
    Sampler(const mc::world::gen::density::DensityFunction& temperature,
        const mc::world::gen::density::DensityFunction& humidity,
        const mc::world::gen::density::DensityFunction& continentalness,
        const mc::world::gen::density::DensityFunction& erosion,
        const mc::world::gen::density::DensityFunction& depth,
        const mc::world::gen::density::DensityFunction& weirdness);

    /**
     * @brief 在指定 quart 坐标处采样气候值
     *
     * quart 坐标 = block 坐标 / 4
     *
     * @param quartX X quart 坐标
     * @param quartY Y quart 坐标
     * @param quartZ Z quart 坐标
     * @return 采样得到的 TargetPoint
     */
    [[nodiscard]] TargetPoint sample(i32 quartX, i32 quartY, i32 quartZ) const;

    /**
     * @brief 获取生成目标参数列表
     *
     * MC 1.21.11: Climate.Sampler.spawnTarget
     * 用于 SpawnFinder 计算出生点。
     */
    [[nodiscard]] const std::vector<ParameterPoint>& spawnTarget() const { return m_spawnTarget; }

    /**
     * @brief 设置生成目标参数列表
     */
    void setSpawnTarget(std::vector<ParameterPoint> target) { m_spawnTarget = std::move(target); }

    /**
     * @brief 使用气候采样器查找出生点
     *
     * MC 1.21.11: Climate.Sampler.findSpawnPosition()
     * 如果 spawnTarget 为空，返回 (0, 0)。
     */
    [[nodiscard]] BlockPos findSpawnPosition() const;

    /**
     * @brief 创建空的气候采样器
     *
     * MC 1.21.11: Climate.empty()
     * 返回一个使用零密度函数的采样器，spawnTarget 为空。
     */
    [[nodiscard]] static std::unique_ptr<Sampler> empty();

    /**
     * @brief 检查采样器是否有效（非空）
     */
    [[nodiscard]] bool isEmpty() const { return m_temperature == nullptr; }

private:
    const mc::world::gen::density::DensityFunction* m_temperature;
    const mc::world::gen::density::DensityFunction* m_humidity;
    const mc::world::gen::density::DensityFunction* m_continentalness;
    const mc::world::gen::density::DensityFunction* m_erosion;
    const mc::world::gen::density::DensityFunction* m_depth;
    const mc::world::gen::density::DensityFunction* m_weirdness;
    std::vector<ParameterPoint> m_spawnTarget;
};

// ============================================================================
// SpawnFinder - 出生点查找器
// ============================================================================

/**
 * @brief 气候空间中的出生点查找器
 *
 * MC 1.21.11: Climate.SpawnFinder
 * 通过径向搜索在气候参数空间中找到最佳出生点。
 *
 * 搜索策略：
 * 1. 从 (0, 0) 开始计算初始 fitness
 * 2. 粗搜索：半径 512..2048，步长 512
 * 3. 精搜索：半径 32..512，步长 32
 *
 * fitness = minParameterFitness * 2048² + distanceFromOrigin²
 * 其中 minParameterFitness 是所有 spawn target 中最小的 fitness 值，
 * depth 参数被置零。
 */
class SpawnFinder {
public:
    /**
     * @brief 搜索结果
     */
    struct Result {
        i32 x;       ///< 出生点 X 坐标
        i32 z;       ///< 出生点 Z 坐标
        i64 fitness; ///< fitness 值（越小越好）
    };

    /**
     * @brief 构造出生点查找器
     *
     * @param spawnTargets 生成目标参数列表
     * @param sampler 气候采样器
     */
    SpawnFinder(std::vector<ParameterPoint> spawnTargets, const Sampler& sampler);

    /**
     * @brief 获取搜索结果
     */
    [[nodiscard]] const Result& result() const { return m_result; }

    /**
     * @brief 静态方法：查找出生点
     *
     * MC 1.21.11: Climate.findSpawnPosition(List<ParameterPoint>, Sampler)
     *
     * @param spawnTargets 生成目标参数列表
     * @param sampler 气候采样器
     * @return 出生点坐标 (x, 0, z)
     */
    [[nodiscard]] static BlockPos findSpawnPosition(
        const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler);

private:
    static constexpr f64 MAX_RADIUS = 2048.0;
    static constexpr i64 SQUARE_MAX_RADIUS = 2048LL * 2048LL;

    /**
     * @brief 计算指定位置的 fitness
     *
     * fitness = minParameterFitness * 2048² + distanceFromOrigin²
     * depth 参数被置零。
     */
    [[nodiscard]] static Result getSpawnPositionAndFitness(
        const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler, i32 x, i32 z);

    /**
     * @brief 径向搜索
     *
     * @param spawnTargets 生成目标参数列表
     * @param sampler 气候采样器
     * @param maxRadius 最大搜索半径
     * @param stepSize 搜索步长
     */
    void radialSearch(
        const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler, f32 maxRadius, f32 stepSize);

    Result m_result;
};

/**
 * @brief 查找出生点的便捷函数
 *
 * MC 1.21.11: Climate.findSpawnPosition()
 * 如果 spawnTargets 为空，返回 BlockPos(0, 0, 0)。
 */
[[nodiscard]] inline BlockPos findSpawnPosition(const std::vector<ParameterPoint>& spawnTargets, const Sampler& sampler)
{
    return SpawnFinder::findSpawnPosition(spawnTargets, sampler);
}

} // namespace mc::world::biome::climate
