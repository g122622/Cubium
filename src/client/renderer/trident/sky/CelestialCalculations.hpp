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
#include "common/util/math/MathConstants.hpp"
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>

namespace mc::client {

/**
 * @brief 天体计算工具类
 *
 * 提供基于 Minecraft 1.16.5 的天体位置计算。
 * 参考: DimensionType.java, WorldRenderer.java
 *
 * 时间系统:
 * - 0 ticks = 日出 (太阳在地平线)
 * - 6000 ticks = 正午 (太阳在最高点)
 * - 12000 ticks = 日落 (太阳在地平线)
 * - 18000 ticks = 午夜 (月亮在最高点)
 */
class CelestialCalculations {
public:
    // ========== 天体角度计算 ==========

    /**
     * @brief 计算天体角度 (太阳/月亮的位置)
     * @param dayTime 当前一天内的时间 (0-23999)
     * @return 天体角度 (0.0-1.0)
     *
     * 返回值含义:
     * - 0.0 = 正午 (太阳最高)
     * - 0.25 = 日落
     * - 0.5 = 午夜 (月亮最高)
     * - 0.75 = 日出
     */
    [[nodiscard]] static f64 calculateCelestialAngle(i64 dayTime);

    /**
     * @brief 计算插值后的天体角度 (用于平滑渲染)
     * @param dayTime 当前 dayTime
     * @param partialTick 部分 tick (0.0-1.0)
     * @return 插值后的天体角度
     */
    [[nodiscard]] static f64 calculateCelestialAngleInterpolated(i64 dayTime, f64 partialTick);

    // ========== 月相计算 ==========

    /**
     * @brief 计算当前月相
     * @param gameTime 游戏总 tick 数
     * @return 月相索引 (0-7)
     *
     * 月相:
     * - 0 = 满月
     * - 1 = 盈凸月
     * - 2 = 上弦月
     * - 3 = 盈月
     * - 4 = 新月
     * - 5 = 亏月
     * - 6 = 下弦月
     * - 7 = 亏凸月
     */
    [[nodiscard]] static i32 calculateMoonPhase(i64 gameTime);

    /**
     * @brief 获取月相亮度因子
     * @param moonPhase 月相索引 (0-7)
     * @return 亮度因子 (0.0-1.0)
     *
     * 满月 = 1.0, 新月 = 0.0
     */
    [[nodiscard]] static f64 getMoonPhaseFactor(i32 moonPhase);

    // ========== 太阳方向计算 ==========

    /**
     * @brief 计算太阳方向向量 (用于光照)
     * @param celestialAngle 天体角度
     * @return 归一化的太阳方向向量
     *
     * 向量指向太阳在天空中的方向。
     * 白天向量朝上，夜晚朝下。
     */
    [[nodiscard]] static glm::vec3 calculateSunDirection(f64 celestialAngle);

    /**
     * @brief 计算太阳强度 (光照强度)
     * @param celestialAngle 天体角度
     * @return 太阳强度 (0.0-1.0)
     *
     * 白天强度高，夜晚强度低。
     * 用于计算环境光照和阴影强度。
     */
    [[nodiscard]] static f64 calculateSunIntensity(f64 celestialAngle);

    // ========== 天空颜色计算 ==========

    /**
     * @brief 计算天空颜色
     * @param celestialAngle 天体角度
     * @param rainStrength 雨强度 (0.0-1.0)
     * @param thunderStrength 雷暴强度 (0.0-1.0)
     * @return RGBA 天空颜色
     *
     * 颜色根据时间和天气变化:
     * - 日出/日落: 橙红色渐变
     * - 正午: 亮蓝色
     * - 夜晚: 深蓝色/黑色
     * - 雨天: 灰色
     */
    [[nodiscard]] static glm::vec4 calculateSkyColor(f64 celestialAngle, f64 rainStrength, f64 thunderStrength);

    /**
     * @brief 计算主世界日出/日落颜色（含强度）
     * @param celestialAngle 天体角度
     * @param rainStrength 雨强度
     * @param thunderStrength 雷暴强度
     * @return RGBA，A 通道为效果强度。若当前时刻无效果则返回全 0。
     *
     * 只在太阳接近地平线时生效。
     * 生效曲线为"渐入 -> 峰值 -> 渐出"。
     * 受天气影响衰减。
     */
    [[nodiscard]] static glm::vec4 calculateSunriseSunsetColor(
        f64 celestialAngle, f64 rainStrength, f64 thunderStrength);

    /**
     * @brief 计算摄像机朝向与日出日落中心方向的对齐因子
     * @param cameraForward 摄像机前向向量（世界空间）
     * @param sunriseDirection 日出/日落中心方向（世界空间）
     * @return [0, 1]，1 表示完全朝向中心，0 表示背向。
     *
     * 注意：
     * - 仅使用水平面（XZ）分量，不受俯仰角影响。
     * - 若任一向量在 XZ 平面长度过小，返回 0 以避免除零。
     */
    [[nodiscard]] static f64 calculateSunriseFacingFactor(
        const glm::vec3& cameraForward, const glm::vec3& sunriseDirection);

    /**
     * @brief 获取主世界默认天空色（#78A7FF）
     * @return 线性空间 RGB
     */
    [[nodiscard]] static constexpr glm::vec3 getOverworldBaseSkyColor()
    {
        return glm::vec3(OVERWORLD_BASE_SKY_R, OVERWORLD_BASE_SKY_G, OVERWORLD_BASE_SKY_B);
    }

    /**
     * @brief 计算雾颜色
     * @param celestialAngle 天体角度
     * @param rainStrength 雨强度
     * @param thunderStrength 雷暴强度
     * @return RGBA 雾颜色
     */
    [[nodiscard]] static glm::vec4 calculateFogColor(f64 celestialAngle, f64 rainStrength, f64 thunderStrength);

    // ========== 星星计算 ==========

    /**
     * @brief 计算星星亮度
     * @param celestialAngle 天体角度
     * @return 星星亮度 (0.0-1.0)
     *
     * 夜晚亮度高，白天不可见。
     */
    [[nodiscard]] static f64 calculateStarBrightness(f64 celestialAngle, f64 rainStrength);

    /**
     * @brief 获取星星生成种子
     * @return 星星位置随机种子
     */
    [[nodiscard]] static constexpr i64 getStarSeed() { return 10842LL; }

    /**
     * @brief 获取星星数量
     * @return 星星数量 (~1500)
     */
    [[nodiscard]] static constexpr i32 getStarCount() { return 1500; }

private:
    /// 主世界默认天空色 (#78A7FF)
    static constexpr f64 OVERWORLD_BASE_SKY_R = 120.0 / 255.0;
    static constexpr f64 OVERWORLD_BASE_SKY_G = 167.0 / 255.0;
    static constexpr f64 OVERWORLD_BASE_SKY_B = 1.0;

    /// 月相亮度因子 (满月=1.0, 新月=0.0)
    static constexpr f64 MOON_PHASE_FACTORS[8] = {
        1.0,  // 满月
        0.75, // 盈凸月
        0.5,  // 上弦月
        0.25, // 盈月
        0.0,  // 新月
        0.25, // 亏月
        0.5,  // 下弦月
        0.75  // 亏凸月
    };
};

} // namespace mc::client
