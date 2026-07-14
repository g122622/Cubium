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

#include "client/renderer/trident/firstperson/FirstPersonTransforms.hpp"
#include "client/renderer/trident/firstperson/MatrixStack.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc::client::renderer::trident::firstperson;
using namespace mc::client::renderer;
using namespace mc::math;
using namespace mc;

namespace {

// 把 Matrix4f 的平移分量取出来（行主序：translation = (m3, m7, m11)）
Vector3f translationOf(const Matrix4f& m)
{
    return m.translation();
}

f32 deg(f32 radians)
{
    return toDegrees(radians);
}

// 4x4 矩阵左上 3x3 线性部分的行列式（旋转+缩放的体积比，不受旋转影响，
// 仅反映各轴缩放乘积）。
f32 linearDeterminant(const Matrix4f& m)
{
    // 行主序：data[row*4+col]
    const f32 a = m(0, 0), b = m(0, 1), c = m(0, 2);
    const f32 d = m(1, 0), e = m(1, 1), f = m(1, 2);
    const f32 g = m(2, 0), h = m(2, 1), i = m(2, 2);
    return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
}

} // namespace

// ============================================================================
// applyItemArmTransform
// ============================================================================

TEST(FirstPersonTransformsTest, ApplyItemArmTransform_RightHandZeroEquip)
{
    MatrixStack stack;
    applyItemArmTransform(stack, HandSide::Right, 0.0f);
    const Vector3f t = translationOf(stack.last());
    // 右手 i=1：translate(0.56, -0.52 + 0*-0.6, -0.72)
    EXPECT_FLOAT_EQ(t.x, 0.56f);
    EXPECT_FLOAT_EQ(t.y, -0.52f);
    EXPECT_FLOAT_EQ(t.z, -0.72f);
}

TEST(FirstPersonTransformsTest, ApplyItemArmTransform_LeftHandFullEquip)
{
    MatrixStack stack;
    applyItemArmTransform(stack, HandSide::Left, 1.0f);
    const Vector3f t = translationOf(stack.last());
    // 左手 i=-1：translate(-0.56, -0.52 + 1*-0.6, -0.72)
    EXPECT_FLOAT_EQ(t.x, -0.56f);
    EXPECT_FLOAT_EQ(t.y, -1.12f);
    EXPECT_FLOAT_EQ(t.z, -0.72f);
}

TEST(FirstPersonTransformsTest, ApplyItemArmTransform_MidEquip)
{
    MatrixStack stack;
    applyItemArmTransform(stack, HandSide::Right, 0.5f);
    const Vector3f t = translationOf(stack.last());
    EXPECT_FLOAT_EQ(t.x, 0.56f);
    EXPECT_FLOAT_EQ(t.y, -0.52f + 0.5f * -0.6f);
    EXPECT_FLOAT_EQ(t.z, -0.72f);
}

// ============================================================================
// swingArm + applyItemArmAttackTransform
// ============================================================================

TEST(FirstPersonTransformsTest, SwingArm_ZeroSwingIsTranslateOnly)
{
    MatrixStack stack;
    swingArm(stack, HandSide::Right, 0.0f);
    // swing=0 → sin(0)=0，平移全部为 0；旋转：sin(0)=0 → 仅剩 YP 45 与 YP -45 相消。
    const Matrix4f& m = stack.last();
    const Vector3f t = translationOf(m);
    EXPECT_FLOAT_EQ(t.x, 0.0f);
    EXPECT_FLOAT_EQ(t.y, 0.0f);
    EXPECT_FLOAT_EQ(t.z, 0.0f);
    // YP 45 与 YP -45 互消 → 单位旋转
    EXPECT_NEAR(m(0, 0), 1.0f, 0.0001f);
    EXPECT_NEAR(m(1, 1), 1.0f, 0.0001f);
    EXPECT_NEAR(m(2, 2), 1.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, SwingArm_HalfSwingRightHand)
{
    MatrixStack stack;
    swingArm(stack, HandSide::Right, 0.5f);
    const f32 sqrtSwing = std::sqrt(0.5f);
    const f32 expectedX = -0.4f * std::sin(sqrtSwing * PI);
    const f32 expectedY = 0.2f * std::sin(sqrtSwing * TWO_PI);
    const f32 expectedZ = -0.2f * std::sin(0.5f * PI);
    const Vector3f t = translationOf(stack.last());
    EXPECT_NEAR(t.x, expectedX, 0.0001f);
    EXPECT_NEAR(t.y, expectedY, 0.0001f);
    EXPECT_NEAR(t.z, expectedZ, 0.0001f);
}

TEST(FirstPersonTransformsTest, SwingArm_LeftHandNegatesX)
{
    MatrixStack stackRight;
    swingArm(stackRight, HandSide::Right, 0.5f);
    MatrixStack stackLeft;
    swingArm(stackLeft, HandSide::Left, 0.5f);
    // 左手 X 平移取反，Y/Z 相同
    EXPECT_NEAR(translationOf(stackLeft.last()).x, -translationOf(stackRight.last()).x, 0.0001f);
    EXPECT_NEAR(translationOf(stackLeft.last()).y, translationOf(stackRight.last()).y, 0.0001f);
    EXPECT_NEAR(translationOf(stackLeft.last()).z, translationOf(stackRight.last()).z, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyItemArmAttackTransform_ZeroSwingIsIdentity)
{
    // swing=0：swing=sin(0)=0、swingSqrt=sin(0)=0 →
    // Ry(i*45) · Rz(0) · Rx(0) · Ry(i*-45) = Ry(i*45)·Ry(i*-45) = I
    MatrixStack stack;
    applyItemArmAttackTransform(stack, HandSide::Right, 0.0f);
    const Matrix4f& m = stack.last();
    EXPECT_NEAR(m(0, 0), 1.0f, 0.0001f);
    EXPECT_NEAR(m(1, 1), 1.0f, 0.0001f);
    EXPECT_NEAR(m(2, 2), 1.0f, 0.0001f);
    EXPECT_NEAR(m(0, 1), 0.0f, 0.0001f);
    EXPECT_NEAR(m(1, 0), 0.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyItemArmAttackTransform_HalfSwingNonIdentity)
{
    MatrixStack stackR;
    applyItemArmAttackTransform(stackR, HandSide::Right, 0.5f);
    // 非单位（有攻击旋转）：行列式仍为 1（纯旋转），但矩阵非单位。
    const Matrix4f& mR = stackR.last();
    EXPECT_NEAR(linearDeterminant(mR), 1.0f, 0.0001f);
    EXPECT_FALSE(std::abs(mR(0, 0) - 1.0f) < 0.0001f && std::abs(mR(1, 1) - 1.0f) < 0.0001f &&
        std::abs(mR(2, 2) - 1.0f) < 0.0001f);
}

// ============================================================================
// renderPlayerArmTransform
// ============================================================================

TEST(FirstPersonTransformsTest, RenderPlayerArmTransform_RightHandNonTrivialPose)
{
    // swing=0：sqrtSwing=0 → 三个 sin 项为 0，translate = (f*0.64000005, -0.6, -0.71999997)，
    // 随后 Ry(f*45)（纯旋转不改平移）、再 translate(f*-1, 3.6, 3.5)（受 Ry(45) 旋转作用改变平移列），
    // 后续 Rz/Rx/Ry 进一步旋转。最终姿态为非平凡的手臂伸出位置（非原点、非单位旋转）。
    MatrixStack stack;
    renderPlayerArmTransform(stack, HandSide::Right, 0.0f, 0.0f);
    const Matrix4f& m = stack.last();
    const Vector3f t = translationOf(m);
    // 平移列非零（手臂不在原点）
    EXPECT_GT(std::abs(t.x) + std::abs(t.y) + std::abs(t.z), 0.5f);
    // 线性部分行列式为 1（纯旋转序列）
    EXPECT_NEAR(linearDeterminant(m), 1.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, RenderPlayerArmTransform_EquipAffectsY)
{
    MatrixStack stack0;
    renderPlayerArmTransform(stack0, HandSide::Right, 0.0f, 0.0f);
    MatrixStack stack1;
    renderPlayerArmTransform(stack1, HandSide::Right, 1.0f, 0.0f);
    // equip=1 时 Y 平移额外 -0.6（其余 swing=0 部分相同）
    EXPECT_NEAR(translationOf(stack1.last()).y, translationOf(stack0.last()).y - 0.6f, 0.0001f);
}

TEST(FirstPersonTransformsTest, RenderPlayerArmTransform_LeftHandMirror)
{
    MatrixStack stackR;
    renderPlayerArmTransform(stackR, HandSide::Right, 0.0f, 0.0f);
    MatrixStack stackL;
    renderPlayerArmTransform(stackL, HandSide::Left, 0.0f, 0.0f);
    EXPECT_NEAR(translationOf(stackL.last()).x, -translationOf(stackR.last()).x, 0.0001f);
}

// ============================================================================
// applyEatTransform
// ============================================================================

TEST(FirstPersonTransformsTest, ApplyEatTransform_StartProgress)
{
    // useDuration=32, useCount=32（刚开始吃）：f = 0 + 0 + 1 = 1, f1 = 1/32 ≈ 0.03125
    MatrixStack stack;
    applyEatTransform(stack, 0.0f, HandSide::Right, 32, 32);
    const f32 f = 1.0f;
    const f32 f1 = f / 32.0f;
    const f32 f3 = 1.0f - static_cast<f32>(std::pow(static_cast<f64>(f1), 27.0));
    // f1 < 0.8 → 额外 Y 平移 |cos(f/4*PI)*0.1| = |cos(PI/4)*0.1|
    const f32 extraY = std::abs(std::cos(f / 4.0f * PI) * 0.1f);
    const Vector3f t = translationOf(stack.last());
    EXPECT_NEAR(t.x, f3 * 0.6f, 0.0001f);
    EXPECT_NEAR(t.y, f3 * -0.5f + extraY, 0.0001f);
    EXPECT_NEAR(t.z, 0.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyEatTransform_LateProgressNoJiggle)
{
    // useDuration=32, useCount=2：f = 30 + 1 = 31, f1 = 31/32 ≈ 0.969 > 0.8 → 无额外抖动
    MatrixStack stack;
    applyEatTransform(stack, 0.0f, HandSide::Right, 32, 2);
    const f32 f = 31.0f;
    const f32 f1 = f / 32.0f;
    const f32 f3 = 1.0f - static_cast<f32>(std::pow(static_cast<f64>(f1), 27.0));
    const Vector3f t = translationOf(stack.last());
    EXPECT_NEAR(t.x, f3 * 0.6f, 0.0001f);
    EXPECT_NEAR(t.y, f3 * -0.5f, 0.0001f); // 无 extraY
}

TEST(FirstPersonTransformsTest, ApplyEatTransform_LeftHandNegatesX)
{
    MatrixStack stackR;
    applyEatTransform(stackR, 0.0f, HandSide::Right, 32, 32);
    MatrixStack stackL;
    applyEatTransform(stackL, 0.0f, HandSide::Left, 32, 32);
    EXPECT_NEAR(translationOf(stackL.last()).x, -translationOf(stackR.last()).x, 0.0001f);
}

// ============================================================================
// applyBowTransform（D1 基座）
// ============================================================================

namespace {
// 让蓄力进度归零：useCount = useDuration + 1 → f7 = useDuration - (useDuration+1) + 0 + 1 = 0
// → f9 = 0（无蓄力平移/scale/抖动），最后的 rotateY 为纯旋转，不改变平移列。
// 此时最终平移列 == 基座 translate。
constexpr i32 BOW_NO_CHARGE_DURATION = 72000;
constexpr i32 BOW_NO_CHARGE_COUNT = BOW_NO_CHARGE_DURATION + 1;
// 弓满蓄力：f7 = 20 → f9 = 20/20 = 1.0 → (1+2)/3 = 1.0 → scale(1,1,1.2)
// f7 = useDuration - useCount + partial + 1 = 20 → useCount = useDuration - 19
constexpr i32 BOW_FULL_CHARGE_COUNT = BOW_NO_CHARGE_DURATION - 19;
// 三叉戟满蓄力：f6 = 10 → f8 = 10/10 = 1.0 → scale(1,1,1.2)
// f6 = useDuration - useCount + 1 = 10 → useCount = useDuration - 9
constexpr i32 TRIDENT_FULL_CHARGE_COUNT = BOW_NO_CHARGE_DURATION - 9;
} // namespace

TEST(FirstPersonTransformsTest, ApplyBowTransform_BaseTranslationRightHand)
{
    MatrixStack stack;
    applyBowTransform(stack, 0.0f, HandSide::Right, BOW_NO_CHARGE_DURATION, BOW_NO_CHARGE_COUNT);
    const Vector3f t = translationOf(stack.last());
    // 右手 j=1：基座 translate(-0.2785682, 0.18344387, 0.15731531)
    EXPECT_NEAR(t.x, -0.2785682f, 0.0001f);
    EXPECT_NEAR(t.y, 0.18344387f, 0.0001f);
    EXPECT_NEAR(t.z, 0.15731531f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyBowTransform_BaseTranslationLeftHand)
{
    MatrixStack stack;
    applyBowTransform(stack, 0.0f, HandSide::Left, BOW_NO_CHARGE_DURATION, BOW_NO_CHARGE_COUNT);
    const Vector3f t = translationOf(stack.last());
    // 左手 j=-1：基座 translate(0.2785682, 0.18344387, 0.15731531)
    EXPECT_NEAR(t.x, 0.2785682f, 0.0001f);
    EXPECT_NEAR(t.y, 0.18344387f, 0.0001f);
    EXPECT_NEAR(t.z, 0.15731531f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyBowTransform_ChargeScalesZ)
{
    // 满蓄力：f7 = 20 → f9 = (1+2)/3 = 1.0 → scale(1,1,1.2)
    // 线性部分 = 基座旋转 · scale(1,1,1.2) · Ry(-45)，行列式 = 1·1.2·1 = 1.2（旋转保行列式）
    MatrixStack stack;
    applyBowTransform(stack, 0.0f, HandSide::Right, BOW_NO_CHARGE_DURATION, BOW_FULL_CHARGE_COUNT);
    EXPECT_NEAR(linearDeterminant(stack.last()), 1.2f, 0.0001f);
}

// ============================================================================
// applyTridentTransform（D1 基座）
// ============================================================================

TEST(FirstPersonTransformsTest, ApplyTridentTransform_BaseTranslationRightHand)
{
    // 蓄力归零：useCount = useDuration + 1 → f6 = 0 → f8 = 0，平移列 = 基座
    MatrixStack stack;
    applyTridentTransform(stack, 0.0f, HandSide::Right, BOW_NO_CHARGE_DURATION, BOW_NO_CHARGE_COUNT);
    const Vector3f t = translationOf(stack.last());
    EXPECT_NEAR(t.x, -0.5f, 0.0001f);
    EXPECT_NEAR(t.y, 0.7f, 0.0001f);
    EXPECT_NEAR(t.z, 0.1f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyTridentTransform_BaseTranslationLeftHand)
{
    MatrixStack stack;
    applyTridentTransform(stack, 0.0f, HandSide::Left, BOW_NO_CHARGE_DURATION, BOW_NO_CHARGE_COUNT);
    const Vector3f t = translationOf(stack.last());
    EXPECT_NEAR(t.x, 0.5f, 0.0001f);
    EXPECT_NEAR(t.y, 0.7f, 0.0001f);
    EXPECT_NEAR(t.z, 0.1f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyTridentTransform_ChargeAffectsMatrix)
{
    // 满蓄力 f6=10 → f8=1.0：translate(0,0,0.2) 与 scale(1,1,1.2) 施加。
    // 无蓄力：scale 单位 → 行列式 1.0；满蓄力：scale(1,1,1.2) → 行列式 1.2。
    MatrixStack stackNoCharge;
    applyTridentTransform(stackNoCharge, 0.0f, HandSide::Right, BOW_NO_CHARGE_DURATION, BOW_NO_CHARGE_COUNT);
    MatrixStack stackFullCharge;
    applyTridentTransform(stackFullCharge, 0.0f, HandSide::Right, BOW_NO_CHARGE_DURATION, TRIDENT_FULL_CHARGE_COUNT);
    EXPECT_NEAR(linearDeterminant(stackNoCharge.last()), 1.0f, 0.0001f);
    EXPECT_NEAR(linearDeterminant(stackFullCharge.last()), 1.2f, 0.0001f);
}

// ============================================================================
// applyCrossbowTransform（D1 基座 + D2 已装填分支）
// ============================================================================

TEST(FirstPersonTransformsTest, ApplyCrossbowTransform_ChargingBaseTranslation)
{
    // 装填蓄力归零：useCount = useDuration + 1 → f = 0 → f1 = 0，平移列 = 基座
    MatrixStack stack;
    applyCrossbowTransform(stack, 0.0f, HandSide::Right, 28, 29, true, false, true, 25, 0.0f);
    const Vector3f t = translationOf(stack.last());
    // 装填基座 translate(i*-0.4785682, -0.094387, 0.05731531)，右手 i=1
    EXPECT_NEAR(t.x, -0.4785682f, 0.0001f);
    EXPECT_NEAR(t.y, -0.094387f, 0.0001f);
    EXPECT_NEAR(t.z, 0.05731531f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyCrossbowTransform_ChargedIdleMainHand)
{
    // 已装填空闲：swing=0、isCharged、isMainHand → 额外 translate(i*-0.641864, 0, 0)
    MatrixStack stack;
    applyCrossbowTransform(stack, 0.0f, HandSide::Right, 28, 0, false, true, true, 25, 0.0f);
    EXPECT_NEAR(translationOf(stack.last()).x, -0.641864f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyCrossbowTransform_ChargedIdleOffHandNoOffset)
{
    // 已装填但副手（isMainHand=false）：不施加额外偏移
    MatrixStack stack;
    applyCrossbowTransform(stack, 0.0f, HandSide::Right, 28, 0, false, true, false, 25, 0.0f);
    EXPECT_NEAR(translationOf(stack.last()).x, 0.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyCrossbowTransform_ChargedSwingingNoOffset)
{
    // 已装填主手但 swing>0.001：不施加额外偏移
    MatrixStack stack;
    applyCrossbowTransform(stack, 0.0f, HandSide::Right, 28, 0, false, true, true, 25, 0.5f);
    // swingArm 会施加平移，但不应含 -0.641864
    const f32 sqrtSwing = std::sqrt(0.5f);
    const f32 expectedX = -0.4f * std::sin(sqrtSwing * PI);
    EXPECT_NEAR(translationOf(stack.last()).x, expectedX, 0.0001f);
}

// ============================================================================
// applyBlockTransform
// ============================================================================

TEST(FirstPersonTransformsTest, ApplyBlockTransform_RightHand)
{
    MatrixStack stack;
    applyBlockTransform(stack, HandSide::Right);
    EXPECT_NEAR(translationOf(stack.last()).x, -0.14142136f, 0.0001f);
    EXPECT_NEAR(translationOf(stack.last()).y, 0.08f, 0.0001f);
    EXPECT_NEAR(translationOf(stack.last()).z, 0.14142136f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ApplyBlockTransform_LeftHandMirror)
{
    MatrixStack stack;
    applyBlockTransform(stack, HandSide::Left);
    EXPECT_NEAR(translationOf(stack.last()).x, 0.14142136f, 0.0001f);
}

// ============================================================================
// calculateMapTilt
// ============================================================================

TEST(FirstPersonTransformsTest, CalculateMapTilt_PitchZero)
{
    // pitch=0：f = 1 - 0 + 0.1 = 1.1 → clamp 1.0 → -cos(PI)*0.5+0.5 = 0.5+0.5 = 1.0
    EXPECT_NEAR(calculateMapTilt(0.0f), 1.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, CalculateMapTilt_Pitch45)
{
    // pitch=45：f = 1 - 1 + 0.1 = 0.1 → -cos(0.1*PI)*0.5+0.5
    const f32 f = 0.1f;
    const f32 expected = -std::cos(f * PI) * 0.5f + 0.5f;
    EXPECT_NEAR(calculateMapTilt(45.0f), expected, 0.0001f);
}

TEST(FirstPersonTransformsTest, CalculateMapTilt_Pitch90Clamped)
{
    // pitch=90：f = 1 - 2 + 0.1 = -0.9 → clamp 0.0 → -cos(0)*0.5+0.5 = 0
    EXPECT_NEAR(calculateMapTilt(90.0f), 0.0f, 0.0001f);
}

// ============================================================================
// computeViewBobbing
// ============================================================================

TEST(FirstPersonTransformsTest, ComputeViewBobbing_ZeroBobIsIdentity)
{
    MatrixStack stack;
    computeViewBobbing(stack, 0.0f, 0.0f);
    const Matrix4f& m = stack.last();
    EXPECT_NEAR(m(0, 0), 1.0f, 0.0001f);
    EXPECT_NEAR(m(1, 1), 1.0f, 0.0001f);
    EXPECT_NEAR(m(2, 2), 1.0f, 0.0001f);
    const Vector3f t = translationOf(m);
    EXPECT_NEAR(t.x, 0.0f, 0.0001f);
    EXPECT_NEAR(t.y, 0.0f, 0.0001f);
    EXPECT_NEAR(t.z, 0.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeViewBobbing_WalkingAppliesTranslation)
{
    MatrixStack stack;
    const f32 walk = 0.25f;
    const f32 bob = 0.5f;
    computeViewBobbing(stack, walk, bob);
    const f32 sinWalk = std::sin(walk * PI);
    const f32 expectedX = sinWalk * bob * 0.5f;
    EXPECT_NEAR(translationOf(stack.last()).x, expectedX, 0.0001f);
}

// ============================================================================
// computeEquipProgress
// ============================================================================

TEST(FirstPersonTransformsTest, ComputeEquipProgress_FullyVisibleAtHeight1)
{
    // height=1（完全可见）→ equipProgress=0
    EXPECT_NEAR(computeEquipProgress(1.0f, 1.0f, 0.0f, 1.0f), 0.0f, 0.0001f);
    EXPECT_NEAR(computeEquipProgress(1.0f, 1.0f, 0.5f, 1.0f), 0.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeEquipProgress_FullyHiddenAtHeight0)
{
    // height=0（完全隐藏）→ equipProgress=1
    EXPECT_NEAR(computeEquipProgress(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.0001f);
    EXPECT_NEAR(computeEquipProgress(0.0f, 0.0f, 1.0f, 1.0f), 1.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeEquipProgress_LerpsBetweenOldAndNewHeight)
{
    // oldHeight=0, height=1, partial=0.5 → lerp=0.5 → equip=1-0.5=0.5
    EXPECT_NEAR(computeEquipProgress(0.0f, 1.0f, 0.5f, 1.0f), 0.5f, 0.0001f);
    // oldHeight=1, height=0, partial=0.25 → lerp=0.75 → equip=0.25
    EXPECT_NEAR(computeEquipProgress(1.0f, 0.0f, 0.25f, 1.0f), 0.25f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeEquipProgress_PartialZeroUsesOldHeight)
{
    // partial=0 → lerp(old, new, 0) = old → equip = 1 - old
    EXPECT_NEAR(computeEquipProgress(0.3f, 0.9f, 0.0f, 1.0f), 0.7f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeEquipProgress_PartialOneUsesNewHeight)
{
    // partial=1 → lerp(old, new, 1) = new → equip = 1 - new
    EXPECT_NEAR(computeEquipProgress(0.3f, 0.9f, 1.0f, 1.0f), 0.1f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeEquipProgress_SwapAnimationScaleMultiplies)
{
    // swapScale=0.5 时结果减半
    EXPECT_NEAR(computeEquipProgress(0.0f, 0.0f, 0.0f, 0.5f), 0.5f, 0.0001f);
    EXPECT_NEAR(computeEquipProgress(0.0f, 1.0f, 0.5f, 0.5f), 0.25f, 0.0001f);
}

// ============================================================================
// computeDamageTilt
// ============================================================================

TEST(FirstPersonTransformsTest, ComputeDamageTilt_NoHurtIsIdentity)
{
    MatrixStack stack;
    computeDamageTilt(stack, 0.0f, 0.0f, 10.0f, 0.0f, 1.0f, 0.0f, false);
    const Matrix4f& m = stack.last();
    EXPECT_NEAR(m(0, 0), 1.0f, 0.0001f);
    EXPECT_NEAR(m(1, 1), 1.0f, 0.0001f);
    EXPECT_NEAR(m(2, 2), 1.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeDamageTilt_HurtAppliesZRotation)
{
    // hurtTime=5, hurtDuration=10, partialTick=0, hurtDir=0, strength=1
    MatrixStack stack;
    computeDamageTilt(stack, 0.0f, 5.0f, 10.0f, 0.0f, 1.0f, 0.0f, false);
    // f2 = 5/10 = 0.5, f2 = sin(0.5^4 * PI) = sin(0.0625*PI)
    // hurtDir=0 → 无 Y 包裹，Z 旋转 = -sin(0.0625*PI)*14
    const f32 f2 = std::sin(std::pow(0.5f, 4) * PI);
    const f32 expectedZRot = toRadians(-f2 * 14.0f * 1.0f);
    const Matrix4f& m = stack.last();
    // Z 旋转矩阵：(cos, -sin, 0; sin, cos, 0; 0,0,1) —— 但右乘语义下 current=current*Rz
    EXPECT_NEAR(m(0, 0), std::cos(expectedZRot), 0.0001f);
    EXPECT_NEAR(m(0, 1), -std::sin(expectedZRot), 0.0001f);
    EXPECT_NEAR(m(1, 0), std::sin(expectedZRot), 0.0001f);
    EXPECT_NEAR(m(1, 1), std::cos(expectedZRot), 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeDamageTilt_ZeroStrengthNoRotation)
{
    MatrixStack stack;
    computeDamageTilt(stack, 0.0f, 5.0f, 10.0f, 0.0f, 0.0f, 0.0f, false);
    const Matrix4f& m = stack.last();
    EXPECT_NEAR(m(0, 0), 1.0f, 0.0001f);
    EXPECT_NEAR(m(1, 1), 1.0f, 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeDamageTilt_DeathAppliesZRotation)
{
    // isDeadOrDying + deathTime=5：Z 旋转 40 - 8000/(5+200)
    MatrixStack stack;
    computeDamageTilt(stack, 0.0f, 0.0f, 10.0f, 0.0f, 1.0f, 5.0f, true);
    // hurtTime=0 → f2 = -partial = 0 → f2<0 return（但仍先施加了死亡 Z 旋转）
    const f32 deathZRot = toRadians(40.0f - 8000.0f / (5.0f + 200.0f));
    const Matrix4f& m = stack.last();
    EXPECT_NEAR(m(0, 0), std::cos(deathZRot), 0.0001f);
}

TEST(FirstPersonTransformsTest, ComputeDamageTilt_HurtDirWrapsWithY)
{
    // hurtDir 非零：先 -Y，再 Z，再 +Y。当 strength=0 时 Z 旋转为 0，Y 包裹应完全相消。
    MatrixStack stack;
    computeDamageTilt(stack, 0.0f, 5.0f, 10.0f, 45.0f, 0.0f, 0.0f, false);
    const Matrix4f& m = stack.last();
    EXPECT_NEAR(m(0, 0), 1.0f, 0.0001f);
    EXPECT_NEAR(m(1, 1), 1.0f, 0.0001f);
    EXPECT_NEAR(m(2, 2), 1.0f, 0.0001f);
}

// ============================================================================
// calculateMapTilt（ItemInHandRenderer.calculateMapTilt）
// f = clamp(1 - pitch/45 + 0.1, 0, 1); return -cos(f*π)*0.5 + 0.5
// ============================================================================

TEST(FirstPersonTransformsTest, CalculateMapTilt_ZeroPitch)
{
    // pitch=0 → f = clamp(1 - 0 + 0.1, 0, 1) = 1.0 → -cos(π)*0.5 + 0.5 = -(-1)*0.5 + 0.5 = 1.0
    EXPECT_FLOAT_EQ(calculateMapTilt(0.0f), 1.0f);
}

TEST(FirstPersonTransformsTest, CalculateMapTilt_HighPitch)
{
    // pitch=45 → f = clamp(1 - 1 + 0.1, 0, 1) = 0.1 → -cos(0.1π)*0.5 + 0.5
    const f32 expected = -std::cos(0.1f * PI) * 0.5f + 0.5f;
    EXPECT_FLOAT_EQ(calculateMapTilt(45.0f), expected);
}

TEST(FirstPersonTransformsTest, CalculateMapTilt_MidPitch)
{
    // pitch=22.5 → f = clamp(1 - 0.5 + 0.1, 0, 1) = 0.6 → -cos(0.6π)*0.5 + 0.5
    const f32 expected = -std::cos(0.6f * PI) * 0.5f + 0.5f;
    EXPECT_FLOAT_EQ(calculateMapTilt(22.5f), expected);
}

TEST(FirstPersonTransformsTest, CalculateMapTilt_NegativePitchClampsFToOne)
{
    // pitch<0 → f>1 clamp 到 1 → 同 pitch=0 结果 1.0
    EXPECT_FLOAT_EQ(calculateMapTilt(-30.0f), 1.0f);
}

TEST(FirstPersonTransformsTest, CalculateMapTilt_VeryHighPitchClampsFToZero)
{
    // pitch 很大 → f<0 clamp 到 0 → -cos(0)*0.5 + 0.5 = -1*0.5 + 0.5 = 0
    EXPECT_FLOAT_EQ(calculateMapTilt(100.0f), 0.0f);
}

// ============================================================================
// applyTwoHandedMapTransform（renderTwoHandedMap）
// ============================================================================

TEST(FirstPersonTransformsTest, ApplyTwoHandedMapTransform_ZeroSwingHasMapTiltTranslation)
{
    // swing=0、equip=0、pitch=0：f3 = calculateMapTilt(0) = 1.0
    // translate(0, -f1/2, f2)：f1=-0.2*sin(0)=0, f2=-0.4*sin(0)=0 → (0,0,0)
    // translate(0, 0.04 + 0*-1.2 + 1.0*-0.5, -0.72) = (0, -0.46, -0.72)
    MatrixStack stack;
    applyTwoHandedMapTransform(stack, 0.0f, 0.0f, 0.0f);
    const Vector3f t = translationOf(stack.last());
    EXPECT_FLOAT_EQ(t.x, 0.0f);
    EXPECT_FLOAT_EQ(t.y, -0.46f);
    EXPECT_FLOAT_EQ(t.z, -0.72f);
}

TEST(FirstPersonTransformsTest, ApplyTwoHandedMapTransform_AppliesScale2)
{
    // 末尾 scale(2,2,2)：线性部分行列式 = 8（2*2*2）
    MatrixStack stack;
    applyTwoHandedMapTransform(stack, 0.0f, 0.0f, 0.0f);
    EXPECT_NEAR(linearDeterminant(stack.last()), 8.0f, 0.0001f);
}

// ============================================================================
// applyOneHandedMapTransform（renderOneHandedMap）
// ============================================================================

TEST(FirstPersonTransformsTest, ApplyOneHandedMapTransform_RightHandBaseOffset)
{
    // 右手 f=1：translate(0.125, -0.125, 0)（map 板在 push/pop 内已弹出）
    MatrixStack stack;
    applyOneHandedMapTransform(stack, HandSide::Right, 0.0f, 0.0f);
    const Vector3f t = translationOf(stack.last());
    EXPECT_FLOAT_EQ(t.x, 0.125f);
    EXPECT_FLOAT_EQ(t.y, -0.125f);
    EXPECT_FLOAT_EQ(t.z, 0.0f);
}

TEST(FirstPersonTransformsTest, ApplyOneHandedMapTransform_LeftHandBaseOffset)
{
    // 左手 f=-1：translate(-0.125, -0.125, 0)
    MatrixStack stack;
    applyOneHandedMapTransform(stack, HandSide::Left, 0.0f, 0.0f);
    const Vector3f t = translationOf(stack.last());
    EXPECT_FLOAT_EQ(t.x, -0.125f);
    EXPECT_FLOAT_EQ(t.y, -0.125f);
    EXPECT_FLOAT_EQ(t.z, 0.0f);
}
