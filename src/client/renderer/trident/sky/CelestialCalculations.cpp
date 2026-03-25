#include "CelestialCalculations.hpp"
#include "../../../../common/util/assert/AssertAll.hpp"
#include <cmath>

namespace mc::client {

// 静态成员定义
constexpr f32 CelestialCalculations::MOON_PHASE_FACTORS[8];
constexpr f32 CelestialCalculations::SKY_COLORS[4][3];

f32 CelestialCalculations::calculateCelestialAngle(i64 dayTime) {
    // MC 1.16.5 DimensionType.calculateCelestialAngle()
    // d0 = frac(dayTime / 24000 - 0.25)
    // d1 = 0.5 - cos(d0 * PI) / 2.0
    // return (d0 * 2.0 + d1) / 3.0

    constexpr f32 TICKS_PER_DAY = 24000.0f;

    f32 d0 = std::fmod(dayTime / TICKS_PER_DAY - 0.25f, 1.0f);
    if (d0 < 0.0f) {
        d0 += 1.0f;
    }

    f32 d1 = 0.5f - std::cos(d0 * mc::math::PI) / 2.0f;

    return (d0 * 2.0f + d1) / 3.0f;
}

f32 CelestialCalculations::calculateCelestialAngleInterpolated(i64 dayTime, f32 partialTick) {
    // 计算插值后的 dayTime
    i64 nextDayTime = (dayTime + 1) % mc::game::DAY_LENGTH_TICKS;
    f32 currentAngle = calculateCelestialAngle(dayTime);
    f32 nextAngle = calculateCelestialAngle(nextDayTime);

    // 线性插值
    // 注意: 需要处理角度跨越 0/1 边界的情况
    f32 diff = nextAngle - currentAngle;
    if (diff > 0.5f) {
        diff -= 1.0f;
    } else if (diff < -0.5f) {
        diff += 1.0f;
    }

    f32 result = currentAngle + diff * partialTick;
    if (result < 0.0f) {
        result += 1.0f;
    } else if (result >= 1.0f) {
        result -= 1.0f;
    }

    return result;
}

i32 CelestialCalculations::calculateMoonPhase(i64 gameTime) {
    // MC 1.16.5: 月相 = (gameTime / 24000) % 8
    return static_cast<i32>((gameTime / mc::game::DAY_LENGTH_TICKS) % 8);
}

f32 CelestialCalculations::getMoonPhaseFactor(i32 moonPhase) {
    if (moonPhase < 0 || moonPhase > 7) {
        return 0.5f;
    }
    return MOON_PHASE_FACTORS[moonPhase];
}

glm::vec3 CelestialCalculations::calculateSunDirection(f32 celestialAngle) {
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
    f32 angle = celestialAngle * mc::math::TAU_F;

    // 太阳高度角:
    // - 正午 (angle=0): cos(0) = 1, 太阳在头顶
    // - 日落 (angle=π/2): cos(π/2) = 0, 太阳在地平线
    // - 午夜 (angle=π): cos(π) = -1, 太阳在地下
    f32 height = std::cos(angle);

    // 太阳绕 Y 轴的角度 (东西方向)
    // 使用 sin 来模拟太阳从东到西的运动
    f32 xz = std::sin(angle);

    // 太阳方向: X 是东西方向，Y 是高度，Z 是南北方向
    // 注意: MC 使用右手坐标系，Z 是南北
    glm::vec3 dir(xz, height, 0.0f);

    return glm::normalize(dir);
}

f32 CelestialCalculations::calculateSunIntensity(f32 celestialAngle) {
    f32 angleRad = celestialAngle * mc::math::TAU_F;
    f32 sunHeight = std::cos(angleRad); // 正午=1, 午夜=-1

    // 在地平线附近做轻微软过渡，避免晨昏突变。
    f32 t = glm::clamp((sunHeight + 0.06f) / 1.06f, 0.0f, 1.0f);
    return glm::smoothstep(0.0f, 1.0f, t);
}

glm::vec4 CelestialCalculations::calculateSkyColor(
    f32 celestialAngle,
    f32 rainStrength,
    f32 thunderStrength) {
    MC_ASSERT_RELEASE_MSG(std::isfinite(celestialAngle), "celestialAngle must be finite");

    const f32 angleRad = celestialAngle * mc::math::TAU_F;
    const f32 sunHeight = std::cos(angleRad);

    // 主世界基础昼夜渐变（白天默认 #78A7FF）。
    const glm::vec3 daySky = getOverworldBaseSkyColor();
    const glm::vec3 nightSky(0.02f, 0.03f, 0.08f);

    // 接近 MC 的昼夜过渡：白天拉满，夜晚降到深蓝。
    const f32 daylight = glm::smoothstep(-0.18f, 0.14f, sunHeight);
    glm::vec3 skyColor = glm::mix(nightSky, daySky, daylight);

    // 日出/日落暖色（主色由 MC sunrise/sunset 曲线提供）。
    const glm::vec4 sunrise = calculateSunriseSunsetColor(celestialAngle, rainStrength, thunderStrength);
    if (sunrise.a > 0.0f) {
        skyColor = glm::mix(skyColor, glm::vec3(sunrise), sunrise.a * 0.37f);
    }

    // 天气影响
    if (rainStrength > 0.0f || thunderStrength > 0.0f) {
        // 雨天/雷暴时天空偏灰偏暗（Java 版观感）。
        glm::vec3 fogGray(0.58f, 0.60f, 0.64f);
        f32 weatherFactor = glm::clamp(std::max(rainStrength, thunderStrength), 0.0f, 1.0f);
        skyColor = glm::mix(skyColor, fogGray, weatherFactor * 0.82f);
    }

    return glm::vec4(skyColor, 1.0f);
}

glm::vec4 CelestialCalculations::calculateSunriseSunsetColor(
    f32 celestialAngle,
    f32 rainStrength,
    f32 thunderStrength) {
    MC_ASSERT_RELEASE_MSG(std::isfinite(celestialAngle), "celestialAngle must be finite");

    // 对齐 MC 1.16.5 DimensionType#calcSunriseSunsetColors。
    const f32 cosine = std::cos(celestialAngle * mc::math::TAU_F);
    if (cosine < -0.4f || cosine > 0.4f) {
        return glm::vec4(0.0f);
    }

    const f32 t = cosine / 0.4f * 0.5f + 0.5f;
    f32 alpha = 1.0f - (1.0f - std::sin(t * mc::math::PI)) * 0.99f;
    alpha *= alpha;

    glm::vec3 color;
    color.r = t * 0.20f + 0.80f;
    color.g = t * t * 0.52f + 0.12f;
    color.b = 0.10f;

    const f32 rainAttenuation = 1.0f - glm::clamp(rainStrength, 0.0f, 1.0f) * 0.75f;
    const f32 thunderAttenuation = 1.0f - glm::clamp(thunderStrength, 0.0f, 1.0f) * 0.75f;
    alpha *= rainAttenuation * thunderAttenuation;

    return glm::vec4(color, glm::clamp(alpha, 0.0f, 1.0f));
}

f32 CelestialCalculations::calculateSunriseFacingFactor(
    const glm::vec3& cameraForward,
    const glm::vec3& sunriseDirection) {
    const glm::vec2 cam(cameraForward.x, cameraForward.z);
    const glm::vec2 sunrise(sunriseDirection.x, sunriseDirection.z);

    const f32 camLen2 = glm::dot(cam, cam);
    const f32 sunriseLen2 = glm::dot(sunrise, sunrise);
    if (camLen2 < 1e-6f || sunriseLen2 < 1e-6f) {
        return 0.0f;
    }

    const glm::vec2 camN = cam / std::sqrt(camLen2);
    const glm::vec2 sunriseN = sunrise / std::sqrt(sunriseLen2);
    return glm::clamp(glm::dot(camN, sunriseN), 0.0f, 1.0f);
}

glm::vec4 CelestialCalculations::calculateFogColor(
    f32 celestialAngle,
    f32 rainStrength,
    f32 thunderStrength) {
    glm::vec4 skyColor = calculateSkyColor(celestialAngle, rainStrength, thunderStrength);
    glm::vec3 fogColor = glm::vec3(skyColor);

    // 雾比天空更灰一些，模拟 MC 地平线“泛白”感。
    fogColor = glm::mix(fogColor, glm::vec3(0.70f, 0.75f, 0.80f), 0.22f);

    return glm::vec4(fogColor, 1.0f);
}

f32 CelestialCalculations::calculateStarBrightness(f32 celestialAngle) {
    // MC 风格的星空亮度曲线：
    // f = 1 - (cos(angle * TAU) * 2 + 0.25)
    // clamp 到 [0,1] 后平方再缩放。
    const f32 angleRad = celestialAngle * mc::math::TAU_F;
    f32 brightness = 1.0f - (std::cos(angleRad) * 2.0f + 0.25f);
    brightness = glm::clamp(brightness, 0.0f, 1.0f);
    return brightness * brightness * 0.5f;
}

} // namespace mc::client
