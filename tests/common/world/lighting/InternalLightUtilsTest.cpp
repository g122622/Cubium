#include <gtest/gtest.h>
#include "common/world/lighting/InternalLightUtils.hpp"
#include "common/core/Constants.hpp"
#include <cmath>

namespace mc {
namespace {

/**
 * @brief InternalLightUtils 单元测试
 *
 * 测试天体角度计算和其他光照相关功能
 */
class InternalLightUtilsTest : public ::testing::Test {
protected:
    // 测试用常数
    static constexpr f64 EPSILON = 0.001;
};

// ============================================================================
// getCelestialAngleMC 测试
// ============================================================================

TEST_F(InternalLightUtilsTest, GetCelestialAngleMC_Noon_ReturnsZero) {
    // 正午 (dayTime = 6000) 时天体角度应为 0.0
    // 参考 MC 1.16.5: DimensionType.func_236032_b_()
    f32 angle = InternalLightUtils::getCelestialAngleMC(6000);
    EXPECT_NEAR(angle, 0.0f, EPSILON);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngleMC_Midnight_ReturnsHalf) {
    // 午夜 (dayTime = 18000) 时天体角度应为 0.5
    f32 angle = InternalLightUtils::getCelestialAngleMC(18000);
    EXPECT_NEAR(angle, 0.5f, EPSILON);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngleMC_Sunrise_Around075) {
    // 日出 (dayTime = 0) 时天体角度应约为 0.75
    f32 angle = InternalLightUtils::getCelestialAngleMC(0);
    EXPECT_NEAR(angle, 0.75f, 0.01f);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngleMC_Sunset_Around025) {
    // 日落 (dayTime = 12000) 时天体角度应约为 0.25
    f32 angle = InternalLightUtils::getCelestialAngleMC(12000);
    EXPECT_NEAR(angle, 0.25f, 0.01f);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngleMC_Dawn_065to069) {
    // 黎明时分 (dayTime ≈ 22000-22600)
    // 这是海龟蛋孵化的最佳时间
    // 天体角度应在 0.65-0.69 范围内

    // dayTime = 22000 对应天体角度约 0.67
    f32 angle22000 = InternalLightUtils::getCelestialAngleMC(22000);
    EXPECT_GT(angle22000, 0.65f);
    EXPECT_LT(angle22000, 0.69f);

    // dayTime = 22600 对应天体角度约 0.69
    f32 angle22600 = InternalLightUtils::getCelestialAngleMC(22600);
    EXPECT_GT(angle22600, 0.65f);
    EXPECT_LT(angle22600, 0.70f);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngleMC_MatchesMCFormula) {
    // 验证公式的正确性
    // 公式: d0 = frac(dayTime / 24000 - 0.25)
    //       d1 = 0.5 - cos(d0 * π) / 2.0
    //       result = (d0 * 2.0 + d1) / 3.0

    // 手动计算 dayTime = 6000 的情况
    // d0 = frac(6000/24000 - 0.25) = frac(0.25 - 0.25) = frac(0) = 0
    // d1 = 0.5 - cos(0) / 2.0 = 0.5 - 0.5 = 0
    // result = (0 * 2 + 0) / 3 = 0
    f32 angle6000 = InternalLightUtils::getCelestialAngleMC(6000);
    EXPECT_NEAR(angle6000, 0.0f, EPSILON);

    // 手动计算 dayTime = 18000 的情况
    // d0 = frac(18000/24000 - 0.25) = frac(0.75 - 0.25) = frac(0.5) = 0.5
    // d1 = 0.5 - cos(0.5π) / 2.0 = 0.5 - 0 / 2.0 = 0.5
    // result = (0.5 * 2 + 0.5) / 3 = 1.5 / 3 = 0.5
    f32 angle18000 = InternalLightUtils::getCelestialAngleMC(18000);
    EXPECT_NEAR(angle18000, 0.5f, EPSILON);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngleMC_WrapsAround) {
    // 验证 dayTime 超过 24000 时会正确循环
    // dayTime = 30000 应等于 dayTime = 6000
    f32 angle30000 = InternalLightUtils::getCelestialAngleMC(30000);
    f32 angle6000 = InternalLightUtils::getCelestialAngleMC(6000);
    EXPECT_NEAR(angle30000, angle6000, EPSILON);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngleMC_IncreasingMonotonicallyInDawn) {
    // 黎明期间天体角度应该单调递增
    // dayTime 从 20000 增加到 24000，天体角度应该增加
    f32 prev = InternalLightUtils::getCelestialAngleMC(20000);
    for (i64 dayTime = 20500; dayTime <= 24000; dayTime += 500) {
        f32 curr = InternalLightUtils::getCelestialAngleMC(dayTime);
        // 处理循环边界：当角度接近 1.0 时会跳到 0.0
        if (prev > 0.9f && curr < 0.1f) {
            // 这是正常的循环
        } else {
            EXPECT_GT(curr, prev) << "dayTime=" << dayTime;
        }
        prev = curr;
    }
}

// ============================================================================
// getCelestialAngle 测试（简化版本）
// ============================================================================

TEST_F(InternalLightUtilsTest, GetCelestialAngle_Sunrise_ReturnsZero) {
    // 日出 (dayTime = 0) 时天体角度应为 0.0
    f32 angle = InternalLightUtils::getCelestialAngle(0);
    EXPECT_NEAR(angle, 0.0f, EPSILON);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngle_Noon_Returns025) {
    // 正午 (dayTime = 6000) 时天体角度应为 0.25
    f32 angle = InternalLightUtils::getCelestialAngle(6000);
    EXPECT_NEAR(angle, 0.25f, EPSILON);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngle_Sunset_Returns05) {
    // 日落 (dayTime = 12000) 时天体角度应为 0.5
    f32 angle = InternalLightUtils::getCelestialAngle(12000);
    EXPECT_NEAR(angle, 0.5f, EPSILON);
}

TEST_F(InternalLightUtilsTest, GetCelestialAngle_Midnight_Returns075) {
    // 午夜 (dayTime = 18000) 时天体角度应为 0.75
    f32 angle = InternalLightUtils::getCelestialAngle(18000);
    EXPECT_NEAR(angle, 0.75f, EPSILON);
}

// ============================================================================
// calculateSkyDarkening 测试
// ============================================================================

TEST_F(InternalLightUtilsTest, CalculateSkyDarkening_Noon_ReturnsZero) {
    // 正午时天空减暗因子应为 0
    i32 darkening = InternalLightUtils::calculateSkyDarkening(6000, false, false);
    EXPECT_EQ(darkening, 0);
}

TEST_F(InternalLightUtilsTest, CalculateSkyDarkening_Midnight_ReturnsMax) {
    // 午夜时天空减暗因子应为最大值 11
    i32 darkening = InternalLightUtils::calculateSkyDarkening(18000, false, false);
    EXPECT_GE(darkening, 10);  // 接近最大值
}

TEST_F(InternalLightUtilsTest, CalculateSkyDarkening_Rain_Increases) {
    // 下雨时天空减暗增加
    i32 normalNoon = InternalLightUtils::calculateSkyDarkening(6000, false, false);
    i32 rainyNoon = InternalLightUtils::calculateSkyDarkening(6000, true, false);
    EXPECT_GT(rainyNoon, normalNoon);
}

TEST_F(InternalLightUtilsTest, CalculateSkyDarkening_Thunder_IncreasesMore) {
    // 雷暴时天空减暗更多
    i32 rainyNoon = InternalLightUtils::calculateSkyDarkening(6000, true, false);
    i32 stormyNoon = InternalLightUtils::calculateSkyDarkening(6000, false, true);
    EXPECT_GE(stormyNoon, rainyNoon);
}

// ============================================================================
// isDaytime/isNighttime 测试
// ============================================================================

TEST_F(InternalLightUtilsTest, IsDaytime_Noon_ReturnsTrue) {
    EXPECT_TRUE(InternalLightUtils::isDaytime(6000));
}

TEST_F(InternalLightUtilsTest, IsDaytime_Midnight_ReturnsFalse) {
    EXPECT_FALSE(InternalLightUtils::isDaytime(18000));
}

TEST_F(InternalLightUtilsTest, IsNighttime_Midnight_ReturnsTrue) {
    EXPECT_TRUE(InternalLightUtils::isNighttime(18000));
}

TEST_F(InternalLightUtilsTest, IsNighttime_Noon_ReturnsFalse) {
    EXPECT_FALSE(InternalLightUtils::isNighttime(6000));
}

TEST_F(InternalLightUtilsTest, IsDaytime_Sunrise_ReturnsTrue) {
    // dayTime = 0 是日出，属于白天
    EXPECT_TRUE(InternalLightUtils::isDaytime(0));
}

TEST_F(InternalLightUtilsTest, IsNighttime_Sunset_ReturnsTrue) {
    // dayTime = 12000 是日落，开始进入夜晚
    EXPECT_TRUE(InternalLightUtils::isNighttime(12000));
}

// ============================================================================
// 月相测试
// ============================================================================

TEST_F(InternalLightUtilsTest, GetMoonPhase_ReturnsCorrectPhase) {
    // 月相周期为 8 天，每天 24000 ticks
    EXPECT_EQ(InternalLightUtils::getMoonPhase(0), 0);        // 第 0 天 = 满月
    EXPECT_EQ(InternalLightUtils::getMoonPhase(24000), 1);    // 第 1 天 = 亏凸月
    EXPECT_EQ(InternalLightUtils::getMoonPhase(48000), 2);    // 第 2 天 = 下弦月
    EXPECT_EQ(InternalLightUtils::getMoonPhase(7 * 24000), 7); // 第 7 天 = 盈凸月
    EXPECT_EQ(InternalLightUtils::getMoonPhase(8 * 24000), 0); // 第 8 天 = 满月（循环）
}

TEST_F(InternalLightUtilsTest, GetMoonBrightness_FullMoon_Returns1) {
    EXPECT_FLOAT_EQ(InternalLightUtils::getMoonBrightness(0), 1.0f);
}

TEST_F(InternalLightUtilsTest, GetMoonBrightness_NewMoon_Returns0) {
    EXPECT_FLOAT_EQ(InternalLightUtils::getMoonBrightness(4), 0.0f);
}

} // namespace
} // namespace mc
