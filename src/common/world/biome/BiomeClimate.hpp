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

// 前向声明（必须在 mc::world::biome 命名空间之外）
namespace mc {
namespace world {
namespace gen {
namespace noise {
class PerlinSimplexNoise;
}
} // namespace gen
} // namespace world
} // namespace mc

namespace mc {
namespace world {
namespace biome {

// ============================================================================
// 生物群系气候常量
// ============================================================================

/// 降雪温度阈值（温度低于此值时生成雪）
inline constexpr f32 SNOW_TEMPERATURE_THRESHOLD = 0.15f;

/// 结冰温度阈值（温度低于此值时水结冰）
inline constexpr f32 FREEZE_TEMPERATURE_THRESHOLD = 0.15f;

// ============================================================================
// 生物群系气候设置
// ============================================================================

/**
 * @brief 生物群系气候设置
 *
 * 包含生物群系的所有气候参数：温度、降水、湿度、大陆度和侵蚀度。
 */
struct BiomeClimate {
    enum class Precipitation { None, Rain, Snow };

    /**
     * @brief 温度修改器
     *
     * FROZEN 用于冰冻生物群系的特殊温度处理，
     * 使得在某些情况下温度固定为冰点以下。
     */
    enum class TemperatureModifier { None, Frozen };

    Precipitation precipitation = Precipitation::Rain;
    f32 temperature = 0.5f;
    TemperatureModifier temperatureModifier = TemperatureModifier::None;
    f32 downfall = 0.5f;
    f32 humidity = 0.5f;
    f32 continentalness = 0.0f;
    f32 erosion = 0.0f;

    BiomeClimate() = default;
    BiomeClimate(Precipitation precip, f32 temp, TemperatureModifier modifier, f32 down, f32 hum, f32 cont, f32 ero)
        : precipitation(precip)
        , temperature(temp)
        , temperatureModifier(modifier)
        , downfall(down)
        , humidity(hum)
        , continentalness(cont)
        , erosion(ero)
    {}

    /** 便捷构造函数，TemperatureModifier 默认为 None */
    BiomeClimate(Precipitation precip, f32 temp, f32 down, f32 hum, f32 cont, f32 ero)
        : precipitation(precip)
        , temperature(temp)
        , temperatureModifier(TemperatureModifier::None)
        , downfall(down)
        , humidity(hum)
        , continentalness(cont)
        , erosion(ero)
    {}

    /** 旧版便捷构造函数（不含湿度、大陆度、侵蚀度），用于向后兼容 */
    BiomeClimate(Precipitation precip, f32 temp, TemperatureModifier modifier, f32 down)
        : precipitation(precip)
        , temperature(temp)
        , temperatureModifier(modifier)
        , downfall(down)
        , humidity(0.5f)
        , continentalness(0.0f)
        , erosion(0.0f)
    {}

    /** 旧版便捷构造函数（不含湿度、大陆度、侵蚀度、TemperatureModifier），用于向后兼容 */
    BiomeClimate(Precipitation precip, f32 temp, f32 down)
        : precipitation(precip)
        , temperature(temp)
        , temperatureModifier(TemperatureModifier::None)
        , downfall(down)
        , humidity(0.5f)
        , continentalness(0.0f)
        , erosion(0.0f)
    {}
};

// ============================================================================
// 温度噪声函数声明
// ============================================================================

/**
 * @brief 获取温度高度调整噪声实例
 *
 * 种子 1234L，单倍频 [0]，用于 Y > seaLevel+17 时的高度温度调整
 */
const world::gen::noise::PerlinSimplexNoise& temperatureNoise();

/**
 * @brief 获取冻结温度噪声实例
 *
 * 种子 3456L，倍频 [-2, -1, 0]，用于 TemperatureModifier::Frozen
 */
const world::gen::noise::PerlinSimplexNoise& frozenTemperatureNoise();

/**
 * @brief 获取生物群系信息噪声实例
 *
 * 种子 2345L，单倍频 [0]，用于 TemperatureModifier::Frozen 和其他生物群系逻辑
 */
const world::gen::noise::PerlinSimplexNoise& biomeInfoNoise();

/**
 * @brief 应用温度修改器
 *
 * - None: 返回原始温度
 * - Frozen: 根据位置噪声决定是否将温度覆盖为 0.2（冰冻模式）
 *
 * @param x 方块 X 坐标
 * @param z 方块 Z 坐标
 * @param baseTemperature 基础温度
 * @param modifier 温度修改器
 * @return 修改后的温度
 */
[[nodiscard]] f32 applyTemperatureModifier(
    i32 x, i32 z, f32 baseTemperature, BiomeClimate::TemperatureModifier modifier);

} // namespace biome
} // namespace world
} // namespace mc

// 旧命名空间兼容别名
namespace mc {
using BiomeClimate = ::mc::world::biome::BiomeClimate;
} // namespace mc
