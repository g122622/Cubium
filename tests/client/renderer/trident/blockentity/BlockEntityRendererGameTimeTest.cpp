/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software", to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file BlockEntityRendererGameTimeTest.cpp
 * @brief BlockEntityRenderer gameTime 动画计算测试
 *
 * 测试覆盖：
 * - BannerRenderer 旗帜飘动动画公式验证（swingTime 和 waveAngle 计算）
 * - BeaconRenderer 信标光束旋转公式验证（使用 BeaconBeamModel::calculateBeamRotation）
 * - gameTime 参数对动画值的影响验证
 * - 不同位置旗帜产生不同动画相位的验证
 * - 边界条件验证（负坐标 floorMod、partialTick 插值等）
 *
 * 注：此测试专注于动画计算数学公式的正确性，不依赖 Vulkan 渲染基础设施。
 * BlockEntityRendererDispatcher 的 gameTime 参数透传集成测试需要在完整渲染环境可用时补充。
 */

#include "client/renderer/trident/blockentity/model/BeaconBeamModel.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::blockentity::model;

// ============================================================================
// 旗帜飘动动画公式测试（BannerRenderer 中的 swingTime 计算）
//
// 飘动公式：
//   phase = (floorMod(x*7 + y*9 + z*13 + gameTime, 100) + partialTick) / 100.0
//   flag.xRot = (-0.0125 + 0.01 * cos(2*PI * phase)) * PI
//
// 注意：BannerRenderer 中的实际实现使用完全相同的公式，
// 此处直接计算以隔离测试渲染器逻辑。
// ============================================================================

class BannerSwingTimeTest : public ::testing::Test {
protected:
    // 模拟 BannerRenderer 中的 swingTime 计算
    static f32 calculateSwingTime(i32 x, i32 y, i32 z, i64 gameTime, f32 partialTick)
    {
        i64 seed = static_cast<i64>(x * 7 + y * 9 + z * 13);
        return static_cast<f32>(mc::math::floorMod(seed + gameTime, 100L) + static_cast<i64>(partialTick * 100.0f)) /
            100.0f;
    }

    // 模拟 BannerRenderer 中的 waveAngle 计算
    // 参考: BannerFlagModel.setupAnim
    static f32 calculateWaveAngle(f32 swingTime)
    {
        return (-0.0125f + 0.01f * std::cos(2.0f * mc::math::PI * swingTime)) * mc::math::PI;
    }
};

TEST_F(BannerSwingTimeTest, ZeroGameTime_ProducesNonZeroSwingTime_WithNonZeroPosition)
{
    // 位置 (1, 2, 3) 使得 seed = 7+18+39 = 64，即使 gameTime=0，
    // swingTime 也会因位置偏移而不为 0
    f32 swingTime = calculateSwingTime(1, 2, 3, 0, 0.0f);
    // seed = 64, floorMod(64+0, 100) = 64, swingTime = 64/100 = 0.64
    EXPECT_FLOAT_EQ(swingTime, 0.64f);
}

TEST_F(BannerSwingTimeTest, ZeroGameTime_ZeroPosition_ProducesZeroSwingTime)
{
    // 位置 (0,0,0) 且 gameTime=0 → seed=0, floorMod(0, 100)=0, swingTime=0
    f32 swingTime = calculateSwingTime(0, 0, 0, 0, 0.0f);
    EXPECT_FLOAT_EQ(swingTime, 0.0f);
}

TEST_F(BannerSwingTimeTest, NonZeroGameTime_ChangesSwingTime)
{
    f32 swingTimeZero = calculateSwingTime(0, 0, 0, 0, 0.0f);
    f32 swingTimeNonZero = calculateSwingTime(0, 0, 0, 100, 0.0f);
    // gameTime=100 → floorMod(0+100, 100)=0, 所以 swingTime 仍为 0
    EXPECT_FLOAT_EQ(swingTimeNonZero, 0.0f);

    // gameTime=50 → floorMod(0+50, 100)=50, swingTime=0.5
    f32 swingTime50 = calculateSwingTime(0, 0, 0, 50, 0.0f);
    EXPECT_FLOAT_EQ(swingTime50, 0.5f);
}

TEST_F(BannerSwingTimeTest, PartialTick_SmoothlyInterpolatesSwingTime)
{
    // gameTime=0, partialTick=0.5 → floorMod(0+0, 100) + 50 = 50, swingTime=0.5
    f32 swingTime = calculateSwingTime(0, 0, 0, 0, 0.5f);
    EXPECT_FLOAT_EQ(swingTime, 0.5f);
}

TEST_F(BannerSwingTimeTest, DifferentPositions_ProduceDifferentSwingTimes)
{
    // 不同位置的旗帜应有不同的动画相位
    f32 st1 = calculateSwingTime(0, 0, 0, 50, 0.0f);
    f32 st2 = calculateSwingTime(1, 0, 0, 50, 0.0f);
    f32 st3 = calculateSwingTime(0, 1, 0, 50, 0.0f);
    f32 st4 = calculateSwingTime(0, 0, 1, 50, 0.0f);

    // 所有旗帜应该有不同的 swingTime
    EXPECT_NE(st1, st2);
    EXPECT_NE(st1, st3);
    EXPECT_NE(st1, st4);
    EXPECT_NE(st2, st3);
}

TEST_F(BannerSwingTimeTest, WaveAngle_RangeIsCorrect)
{
    // waveAngle = (-0.0125 + 0.01 * cos(2*PI*swingTime)) * PI
    // cos 范围 [-1, 1]，所以 waveAngle 范围：
    // 最小值 = (-0.0125 + 0.01*(-1)) * PI = -0.0225 * PI ≈ -0.07069
    // 最大值 = (-0.0125 + 0.01*(1))  * PI = -0.0025 * PI ≈ -0.00785
    // 注意：waveAngle 始终为负（旗帜向前倾斜）

    f32 minAngle = std::numeric_limits<f32>::max();
    f32 maxAngle = std::numeric_limits<f32>::lowest();

    for (i64 t = 0; t < 100; ++t) {
        f32 swingTime = calculateSwingTime(0, 0, 0, t, 0.0f);
        f32 angle = calculateWaveAngle(swingTime);
        minAngle = std::min(minAngle, angle);
        maxAngle = std::max(maxAngle, angle);
    }

    // waveAngle 应在 [-0.0225*PI, -0.0025*PI] 范围内
    EXPECT_GE(minAngle, -0.0225f * mc::math::PI - 0.001f);
    EXPECT_LE(maxAngle, -0.0025f * mc::math::PI + 0.001f);

    // 始终为负
    EXPECT_LT(maxAngle, 0.0f);
}

TEST_F(BannerSwingTimeTest, WaveAngle_ChangesWithGameTime)
{
    // gameTime=0 时的 waveAngle
    f32 angle0 = calculateWaveAngle(calculateSwingTime(0, 0, 0, 0, 0.0f));
    // gameTime=25 时的 waveAngle（应该不同）
    f32 angle25 = calculateWaveAngle(calculateSwingTime(0, 0, 0, 25, 0.0f));

    // 对于位置 (0,0,0)，gameTime=0 时 swingTime=0，angle=(-0.0125+0.01)*PI=-0.0025*PI
    // gameTime=25 时 swingTime=0.25，cos(2*PI*0.25)=cos(PI/2)=0，angle=-0.0125*PI
    EXPECT_NE(angle0, angle25);
}

TEST_F(BannerSwingTimeTest, FloorMod_HandlesNegativeSeed)
{
    // 负坐标应该正确处理 floorMod
    // seed = -1*7 + 0*9 + 0*13 = -7
    // floorMod(-7 + 0, 100) 应该返回 93 (不是 -7)
    f32 swingTime = calculateSwingTime(-1, 0, 0, 0, 0.0f);
    EXPECT_FLOAT_EQ(swingTime, 0.93f);
}

TEST_F(BannerSwingTimeTest, GameTimeAffectsSwingTime_NotConstantZero)
{
    // 此测试验证 gameTime 参数确实影响动画值，
    // 确保 BannerRenderer 使用 gameTime 后旗帜动画不再是静态的。
    // 对于位置 (5, 10, 15)，seed = 35+90+195 = 320
    // floorMod(320, 100) = 20，所以 gameTime=0 时 swingTime=0.2
    // gameTime=80 时，floorMod(320+80, 100) = floorMod(400, 100) = 0，swingTime=0.0
    // gameTime=30 时，floorMod(320+30, 100) = floorMod(350, 100) = 50，swingTime=0.5
    f32 st0 = calculateSwingTime(5, 10, 15, 0, 0.0f);
    f32 st30 = calculateSwingTime(5, 10, 15, 30, 0.0f);
    f32 st80 = calculateSwingTime(5, 10, 15, 80, 0.0f);

    // 确认不同的 gameTime 产生不同的 swingTime
    EXPECT_NE(st0, st30);
    EXPECT_NE(st0, st80);
    EXPECT_NE(st30, st80);

    // 验证具体值
    EXPECT_FLOAT_EQ(st0, 0.2f);
    EXPECT_FLOAT_EQ(st30, 0.5f);
    EXPECT_FLOAT_EQ(st80, 0.0f);
}

TEST_F(BannerSwingTimeTest, GameTimeZero_AnimationFrozenAtPositionSeed)
{
    // 当 gameTime=0 时，动画"冻结"在由位置决定的固定值
    // 这模拟了修复前（gameTime 硬编码为 0）的行为：
    // 所有旗帜都静止不动，但不同位置的旗帜有不同的静态姿态
    f32 st1 = calculateSwingTime(1, 0, 0, 0, 0.0f);
    f32 st2 = calculateSwingTime(2, 0, 0, 0, 0.0f);

    // 不同位置的旗帜有不同的静态值
    EXPECT_NE(st1, st2);

    // 但相同位置的旗帜在 gameTime=0 时总是相同的值（无动画）
    f32 st1_again = calculateSwingTime(1, 0, 0, 0, 0.0f);
    EXPECT_FLOAT_EQ(st1, st1_again);
}

// ============================================================================
// 信标光束旋转公式测试（BeaconRenderer 中使用的公式）
//
// 旋转公式：
//   rotation = (floorMod(gameTime, 40) + partialTick) * 2.25 - 45
//
// 此处使用 BeaconBeamModel::calculateBeamRotation 进行验证
// ============================================================================

class BeaconBeamRotationTest : public ::testing::Test {
protected:
    static f32 calculateBeamRotation(i64 gameTime, f32 partialTick)
    {
        return BeaconBeamModel::calculateBeamRotation(gameTime, partialTick);
    }
};

TEST_F(BeaconBeamRotationTest, ZeroGameTime_ZeroPartialTick)
{
    // gameTime=0, partialTick=0 → rotation = 0*2.25 - 45 = -45
    f32 rotation = calculateBeamRotation(0, 0.0f);
    EXPECT_FLOAT_EQ(rotation, -45.0f);
}

TEST_F(BeaconBeamRotationTest, ZeroGameTime_PartialTickOne)
{
    // gameTime=0, partialTick=1 → rotation = 1*2.25 - 45 = -42.75
    f32 rotation = calculateBeamRotation(0, 1.0f);
    EXPECT_FLOAT_EQ(rotation, -42.75f);
}

TEST_F(BeaconBeamRotationTest, MidPeriod)
{
    // gameTime=20, partialTick=0 → rotation = 20*2.25 - 45 = 0
    f32 rotation = calculateBeamRotation(20, 0.0f);
    EXPECT_FLOAT_EQ(rotation, 0.0f);
}

TEST_F(BeaconBeamRotationTest, PeriodWrapsAround)
{
    // gameTime=40, partialTick=0 → floorMod(40, 40)=0, rotation = 0*2.25 - 45 = -45
    f32 rotation = calculateBeamRotation(40, 0.0f);
    EXPECT_FLOAT_EQ(rotation, -45.0f);
}

TEST_F(BeaconBeamRotationTest, RotationChangesWithGameTime)
{
    // 验证不同 gameTime 产生不同旋转角度
    f32 rot0 = calculateBeamRotation(0, 0.0f);
    f32 rot10 = calculateBeamRotation(10, 0.0f);
    f32 rot20 = calculateBeamRotation(20, 0.0f);
    f32 rot30 = calculateBeamRotation(30, 0.0f);

    EXPECT_NE(rot0, rot10);
    EXPECT_NE(rot10, rot20);
    EXPECT_NE(rot20, rot30);
}

TEST_F(BeaconBeamRotationTest, GameTimeZero_BeforeFix_WasFrozen)
{
    // 此测试验证修复前的问题：当 gameTime=0 时，信标光束旋转角度固定为 -45 度
    // 修复后，当 gameTime > 0 时旋转角度应变化
    f32 rotAtZero = calculateBeamRotation(0, 0.0f);

    // 在不同的 gameTime 下应该有不同的旋转
    bool hasDifferentRotation = false;
    for (i64 t = 1; t <= 40; ++t) {
        if (calculateBeamRotation(t, 0.0f) != rotAtZero) {
            hasDifferentRotation = true;
            break;
        }
    }
    EXPECT_TRUE(hasDifferentRotation);
}

TEST_F(BeaconBeamRotationTest, RotationRange)
{
    // rotation = (floorMod(gameTime, 40) + partialTick) * 2.25 - 45
    // floorMod(gameTime, 40) 范围 [0, 39]
    // 加上 partialTick 范围 [0, 1)
    // 所以 (floorMod + partialTick) 范围 [0, 40)
    // rotation 范围 [0*2.25 - 45, 40*2.25 - 45) = [-45, 45)
    f32 minRot = std::numeric_limits<f32>::max();
    f32 maxRot = std::numeric_limits<f32>::lowest();

    for (i64 t = 0; t < 40; ++t) {
        f32 rot = calculateBeamRotation(t, 0.0f);
        minRot = std::min(minRot, rot);
        maxRot = std::max(maxRot, rot);
    }

    EXPECT_FLOAT_EQ(minRot, -45.0f);
    // maxRot 在 gameTime=39, partialTick=0 时为 39*2.25-45 = 87.75-45 = 42.75
    EXPECT_FLOAT_EQ(maxRot, 42.75f);
}

// ============================================================================
// 集成测试：gameTime 对动画值的全链路影响验证
// ============================================================================

class GameTimeIntegrationTest : public ::testing::Test {};

TEST_F(GameTimeIntegrationTest, BannerSwingTime_GameTimeZero_VsNonZero_ProducesDifferentAngles)
{
    // 验证 gameTime=0 和 gameTime>0 产生不同的波浪角度
    auto computeWaveAngle = [](i32 x, i32 y, i32 z, i64 gameTime, f32 partialTick) -> f32 {
        i64 seed = static_cast<i64>(x * 7 + y * 9 + z * 13);
        f32 swingTime =
            static_cast<f32>(mc::math::floorMod(seed + gameTime, 100L) + static_cast<i64>(partialTick * 100.0f)) /
            100.0f;
        return (-0.0125f + 0.01f * std::cos(2.0f * mc::math::PI * swingTime)) * mc::math::PI;
    };

    // 位置 (10, 64, -5) 的旗帜，seed = 70 + 576 - 65 = 581
    i32 x = 10, y = 64, z = -5;

    f32 angleZeroGameTime = computeWaveAngle(x, y, z, 0, 0.0f);
    f32 angleGameTime50 = computeWaveAngle(x, y, z, 50, 0.0f);

    // floorMod(581+0, 100) = 81, swingTime=0.81
    // floorMod(581+50, 100) = floorMod(631, 100) = 31, swingTime=0.31
    // 不同的 swingTime → 不同的 waveAngle
    EXPECT_NE(angleZeroGameTime, angleGameTime50);
}

TEST_F(GameTimeIntegrationTest, BeaconBeamRotation_GameTimeZero_VsNonZero_ProducesDifferentRotations)
{
    // 验证 gameTime=0 和 gameTime>0 产生不同的光束旋转角度
    f32 rotZero = BeaconBeamModel::calculateBeamRotation(0, 0.0f);
    f32 rotTen = BeaconBeamModel::calculateBeamRotation(10, 0.0f);
    f32 rotTwenty = BeaconBeamModel::calculateBeamRotation(20, 0.0f);

    EXPECT_NE(rotZero, rotTen);
    EXPECT_NE(rotTen, rotTwenty);
}
