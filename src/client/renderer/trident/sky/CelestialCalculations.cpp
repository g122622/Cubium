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

#include "CelestialCalculations.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>

namespace mc::client {

// 静态成员定义
constexpr f64 CelestialCalculations::MOON_PHASE_FACTORS[8];

f64 CelestialCalculations::calculateCelestialAngle(i64 dayTime)
{
    // 采用与天空渲染一致的线性天体角度映射：
    // - 6000 ticks = 正午 -> 0.0
    // - 12000 ticks = 日落 -> 0.25
    // - 18000 ticks = 午夜 -> 0.5
    // - 0 ticks = 日出 -> 0.75

    constexpr f64 TICKS_PER_DAY = static_cast<f64>(mc::game::DAY_LENGTH_TICKS);
    constexpr f64 NOON_TICKS = 6000.0;

    f64 normalizedDayTime = std::fmod(static_cast<f64>(dayTime), TICKS_PER_DAY);
    if (normalizedDayTime < 0.0) {
        normalizedDayTime += TICKS_PER_DAY;
    }

    f64 celestialAngle = (normalizedDayTime - NOON_TICKS) / TICKS_PER_DAY;
    if (celestialAngle < 0.0) {
        celestialAngle += 1.0;
    }

    return celestialAngle;
}

f64 CelestialCalculations::calculateCelestialAngleInterpolated(i64 dayTime, f64 partialTick)
{
    // 计算插值后的 dayTime
    i64 nextDayTime = (dayTime + 1) % mc::game::DAY_LENGTH_TICKS;
    f64 currentAngle = calculateCelestialAngle(dayTime);
    f64 nextAngle = calculateCelestialAngle(nextDayTime);

    // 线性插值
    // 注意: 需要处理角度跨越 0/1 边界的情况
    f64 diff = nextAngle - currentAngle;
    if (diff > 0.5) {
        diff -= 1.0;
    } else if (diff < -0.5) {
        diff += 1.0;
    }

    f64 result = currentAngle + diff * partialTick;
    if (result < 0.0) {
        result += 1.0;
    } else if (result >= 1.0) {
        result -= 1.0;
    }

    return result;
}

i32 CelestialCalculations::calculateMoonPhase(i64 gameTime)
{
    return static_cast<i32>((gameTime / mc::game::DAY_LENGTH_TICKS) % 8);
}

f64 CelestialCalculations::getMoonPhaseFactor(i32 moonPhase)
{
    if (moonPhase < 0 || moonPhase > 7) {
        return 0.5;
    }
    return MOON_PHASE_FACTORS[moonPhase];
}

glm::vec3 CelestialCalculations::calculateSunDirection(f64 celestialAngle)
{
    // 天体角度转弧度
    // 0.0 = 正午 (太阳在头顶)
    // 0.5 = 午夜 (太阳在脚底)
    //
    // MC 的天体角度定义:
    // - celestialAngle 是太阳/月亮在天空中的位置参数
    // - 当 celestialAngle = 0 时，太阳在最高点 (正午)
    // - 当 celestialAngle = 0.5 时，太阳在最低点 (午夜)
    //
    // 太阳绕 X 轴旋转 (东西方向):
    // - 在 MC 中，太阳从东升起，向西落下
    // - 我们使用 X-Z 平面作为地平线，Y 轴指向天顶
    // - 太阳角度从正午开始，所以需要偏移

    // celestialAngle 转弧度，乘以 2π
    f64 angle = celestialAngle * mc::math::TAU_F;

    // 太阳高度角:
    // - 正午 (angle=0): cos(0) = 1, 太阳在头顶
    // - 日落 (angle=π/2): cos(π/2) = 0, 太阳在地平线
    // - 午夜 (angle=π): cos(π) = -1, 太阳在地下
    f64 height = std::cos(angle);

    // 太阳绕 Y 轴的角度 (东西方向)
    // 使用 sin 来模拟太阳从东到西的运动
    f64 xz = std::sin(angle);

    // 太阳方向: X 是东西方向，Y 是高度，Z 是南北方向
    // 注意: MC 使用右手坐标系，Z 是南北
    glm::dvec3 dir(xz, height, 0.0);

    const glm::dvec3 normalized = glm::normalize(dir);
    return glm::vec3(static_cast<f32>(normalized.x), static_cast<f32>(normalized.y), static_cast<f32>(normalized.z));
}

f64 CelestialCalculations::calculateSunIntensity(f64 celestialAngle)
{
    f64 angleRad = celestialAngle * mc::math::TAU_F;
    f64 sunHeight = std::cos(angleRad); // 正午=1, 午夜=-1

    // 在地平线附近做轻微软过渡，避免晨昏突变。
    f64 t = glm::clamp((sunHeight + 0.06) / 1.06, 0.0, 1.0);
    return glm::smoothstep(0.0, 1.0, t);
}

glm::vec4 CelestialCalculations::calculateSkyColor(f64 celestialAngle, f64 rainStrength, f64 thunderStrength)
{
    MC_ASSERT_RELEASE_MSG(std::isfinite(celestialAngle), "celestialAngle must be finite");

    const f64 angleRad = celestialAngle * mc::math::TAU_F;
    const f64 sunHeight = std::cos(angleRad);

    // 主世界基础昼夜渐变（白天默认 #78A7FF）。
    const glm::vec3 daySky = getOverworldBaseSkyColor();
    const glm::vec3 nightSky(0.02f, 0.03f, 0.08f);

    // 接近 MC 的昼夜过渡：白天拉满，夜晚降到深蓝。
    const f64 daylight = glm::smoothstep(-0.18, 0.14, sunHeight);
    glm::vec3 skyColor = glm::mix(nightSky, daySky, static_cast<f32>(daylight));

    // 日出/日落暖色（主色由 MC sunrise/sunset 曲线提供）。
    const glm::vec4 sunrise = calculateSunriseSunsetColor(celestialAngle, rainStrength, thunderStrength);
    if (sunrise.a > 0.0f) {
        skyColor = glm::mix(skyColor, glm::vec3(sunrise), sunrise.a * 0.37f);
    }

    // 天气影响
    if (rainStrength > 0.0f || thunderStrength > 0.0f) {
        // 雨天/雷暴时天空偏灰偏暗（Java 版观感）。
        glm::vec3 fogGray(0.58f, 0.60f, 0.64f);
        f64 weatherFactor = glm::clamp(std::max(rainStrength, thunderStrength), 0.0, 1.0);
        skyColor = glm::mix(skyColor, fogGray, static_cast<f32>(weatherFactor * 0.82));
    }

    return glm::vec4(skyColor, 1.0f);
}

glm::vec4 CelestialCalculations::calculateSunriseSunsetColor(f64 celestialAngle, f64 rainStrength, f64 thunderStrength)
{
    MC_ASSERT_RELEASE_MSG(std::isfinite(celestialAngle), "celestialAngle must be finite");

    const f64 cosine = std::cos(celestialAngle * mc::math::TAU_F);
    if (cosine < -0.4 || cosine > 0.4) {
        return glm::vec4(0.0f);
    }

    const f64 t = cosine / 0.4 * 0.5 + 0.5;
    f64 alpha = 1.0 - (1.0 - std::sin(t * mc::math::PI_DOUBLE)) * 0.99;
    alpha *= alpha;

    glm::vec3 color;
    color.r = static_cast<f32>(t * 0.20 + 0.80);
    color.g = static_cast<f32>(t * t * 0.52 + 0.12);
    color.b = 0.10f;

    const f64 rainAttenuation = 1.0 - glm::clamp(rainStrength, 0.0, 1.0) * 0.75;
    const f64 thunderAttenuation = 1.0 - glm::clamp(thunderStrength, 0.0, 1.0) * 0.75;
    alpha *= rainAttenuation * thunderAttenuation;

    return glm::vec4(color, static_cast<f32>(glm::clamp(alpha, 0.0, 1.0)));
}

f64 CelestialCalculations::calculateSunriseFacingFactor(
    const glm::vec3& cameraForward, const glm::vec3& sunriseDirection)
{
    const glm::dvec2 cam(cameraForward.x, cameraForward.z);
    const glm::dvec2 sunrise(sunriseDirection.x, sunriseDirection.z);

    const f64 camLen2 = glm::dot(cam, cam);
    const f64 sunriseLen2 = glm::dot(sunrise, sunrise);
    if (camLen2 < 1e-6 || sunriseLen2 < 1e-6) {
        return 0.0;
    }

    const glm::dvec2 camN = cam / std::sqrt(camLen2);
    const glm::dvec2 sunriseN = sunrise / std::sqrt(sunriseLen2);
    return glm::clamp(glm::dot(camN, sunriseN), 0.0, 1.0);
}

glm::vec4 CelestialCalculations::calculateFogColor(f64 celestialAngle, f64 rainStrength, f64 thunderStrength)
{
    glm::vec4 skyColor = calculateSkyColor(celestialAngle, rainStrength, thunderStrength);
    glm::vec3 fogColor = glm::vec3(skyColor);

    // 雾比天空更灰一些，模拟 MC 地平线"泛白"感。
    fogColor = glm::mix(fogColor, glm::vec3(0.70f, 0.75f, 0.80f), 0.22f);

    return glm::vec4(fogColor, 1.0f);
}

f64 CelestialCalculations::calculateStarBrightness(f64 celestialAngle, f64 rainStrength)
{
    // 星空亮度曲线：
    // f = 1 - (cos(angle * TAU) * 2 + 0.25)
    // clamp 到 [0,1] 后平方再缩放。
    const f64 angleRad = celestialAngle * mc::math::TAU_F;
    f64 brightness = 1.0 - (std::cos(angleRad) * 2.0 + 0.25);
    brightness = glm::clamp(brightness, 0.0, 1.0);
    // 下雨时星星不可见
    return brightness * brightness * 0.5 * (1.0 - rainStrength);
}

} // namespace mc::client
