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

/**
 * @file SpecialRendererMatrixTest.cpp
 * @brief 特殊实体渲染器（TNT/FallingBlock）模型矩阵与闪烁计算单元测试
 *
 * 验证内容：
 * - TNTRenderer::calculateTntFlashScale 对齐 MC 1.21.11 TntRenderer 闪烁缩放公式
 *   (1 + (1 - fuse/10)^4 * 0.3，fuse 在 [0,10) 范围内)
 * - TNTRenderer::isTntFlashFrame 对齐 MC 白色闪烁帧判定 (fuse/5 % 2 == 0)
 * - TNTRenderer::buildTntModelMatrix 变换链矩阵乘法顺序（右乘，对应 MC PoseStack）
 *   M = translate(0,0.5,0) * [scale] * rotateY(-90°) * translate(-0.5,-0.5,0.5) * rotateY(90°)
 * - FallingBlockRenderer::buildFallingBlockModelMatrix 产生 translate(-0.5, 0, -0.5) 矩阵
 *
 * 矩阵布局：行主序 std::array<f64, 16>，索引 [row*4 + col]，
 *   矩阵-向量乘法：result.x = m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3]
 * 平移分量位于 m[3], m[7], m[11]。
 */

#include "client/renderer/trident/entity/renderer/special/SpecialEntityRenderers.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::entity::renderer::special;

namespace {

/// 比较两个矩阵是否近似相等（按分量）
void expectMatrixNear(const std::array<f64, 16>& expected, const std::array<f64, 16>& actual, f64 tolerance = 1e-6)
{
    for (std::size_t i = 0; i < 16; ++i) {
        EXPECT_NEAR(expected[i], actual[i], tolerance) << "matrix index " << i;
    }
}

/// 用矩阵变换一个点（w=1）
[[nodiscard]] std::array<f64, 3> transformPoint(const std::array<f64, 16>& m, f64 x, f64 y, f64 z)
{
    return {m[0] * x + m[1] * y + m[2] * z + m[3],
        m[4] * x + m[5] * y + m[6] * z + m[7],
        m[8] * x + m[9] * y + m[10] * z + m[11]};
}

} // namespace

// ============================================================================
// TNT 闪烁缩放计算测试
// ============================================================================

TEST(TntRendererFlashScaleTest, ReturnsOneWhenFuseNegative)
{
    // fuseRemaining < 0 表示未点燃或已爆炸，无闪烁缩放
    EXPECT_DOUBLE_EQ(1.0, TNTRenderer::calculateTntFlashScale(-1.0f));
    EXPECT_DOUBLE_EQ(1.0, TNTRenderer::calculateTntFlashScale(-0.5f));
    EXPECT_DOUBLE_EQ(1.0, TNTRenderer::calculateTntFlashScale(-100.0f));
}

TEST(TntRendererFlashScaleTest, ReturnsOneWhenFuseAtLeast10)
{
    // fuseRemaining >= 10 时不缩放
    EXPECT_DOUBLE_EQ(1.0, TNTRenderer::calculateTntFlashScale(10.0f));
    EXPECT_DOUBLE_EQ(1.0, TNTRenderer::calculateTntFlashScale(20.0f));
    EXPECT_DOUBLE_EQ(1.0, TNTRenderer::calculateTntFlashScale(80.0f));
}

TEST(TntRendererFlashScaleTest, ReturnsOneAtFuseBoundary10)
{
    // 边界值 fuseRemaining = 10 应返回 1.0（不包含 10）
    EXPECT_DOUBLE_EQ(1.0, TNTRenderer::calculateTntFlashScale(10.0f));
}

TEST(TntRendererFlashScaleTest, ReturnsMaxAtFuseZero)
{
    // fuseRemaining = 0: f = 1 - 0 = 1; f^4 = 1; scale = 1 + 1*0.3 = 1.3
    EXPECT_NEAR(1.3, TNTRenderer::calculateTntFlashScale(0.0f), 1e-6);
}

TEST(TntRendererFlashScaleTest, HandComputedAtFuse5)
{
    // fuseRemaining = 5: f = 1 - 0.5 = 0.5; f^4 = 0.0625; scale = 1 + 0.0625*0.3 = 1.01875
    EXPECT_NEAR(1.01875, TNTRenderer::calculateTntFlashScale(5.0f), 1e-6);
}

TEST(TntRendererFlashScaleTest, HandComputedAtFuse1)
{
    // fuseRemaining = 1: f = 1 - 0.1 = 0.9; f^2 = 0.81; f^4 = 0.6561; scale = 1 + 0.6561*0.3 = 1.19683
    EXPECT_NEAR(1.19683, TNTRenderer::calculateTntFlashScale(1.0f), 1e-5);
}

TEST(TntRendererFlashScaleTest, GrowsMonotonicallyAsFuseDecreases)
{
    // fuseRemaining 越接近 0，缩放因子越大
    const f64 at9 = TNTRenderer::calculateTntFlashScale(9.0f);
    const f64 at5 = TNTRenderer::calculateTntFlashScale(5.0f);
    const f64 at1 = TNTRenderer::calculateTntFlashScale(1.0f);
    const f64 at0 = TNTRenderer::calculateTntFlashScale(0.0f);
    EXPECT_GT(at5, at9);
    EXPECT_GT(at1, at5);
    EXPECT_GT(at0, at1);
}

TEST(TntRendererFlashScaleTest, AcceptsFractionalFuseValues)
{
    // TNT 使用插值后的 fuseRemaining（fuse - partialTicks + 1），可能是小数
    // fuseRemaining = 4.5: f = 1 - 0.45 = 0.55; f^4 = 0.09150625; scale ≈ 1.02745
    EXPECT_NEAR(1.02745, TNTRenderer::calculateTntFlashScale(4.5f), 1e-4);
}

// ============================================================================
// TNT 白色闪烁帧判定测试
// ============================================================================

TEST(TntRendererFlashFrameTest, ReturnsFalseWhenFuseAtOrBelowMinusOne)
{
    // fuseRemaining <= -1 表示未点燃，不闪烁
    EXPECT_FALSE(TNTRenderer::isTntFlashFrame(-1.0f));
    EXPECT_FALSE(TNTRenderer::isTntFlashFrame(-5.0f));
    EXPECT_FALSE(TNTRenderer::isTntFlashFrame(-100.0f));
}

TEST(TntRendererFlashFrameTest, ReturnsTrueWhenFuseBetween0And4)
{
    // fuseRemaining = 0,1,2,3,4 -> (int)/5 = 0, 0%2 = 0 -> 闪烁
    for (i32 fuse = 0; fuse < 5; ++fuse) {
        EXPECT_TRUE(TNTRenderer::isTntFlashFrame(static_cast<f32>(fuse))) << "fuse=" << fuse;
    }
}

TEST(TntRendererFlashFrameTest, ReturnsFalseWhenFuseBetween5And9)
{
    // fuseRemaining = 5,6,7,8,9 -> (int)/5 = 1, 1%2 = 1 -> 不闪烁
    for (i32 fuse = 5; fuse < 10; ++fuse) {
        EXPECT_FALSE(TNTRenderer::isTntFlashFrame(static_cast<f32>(fuse))) << "fuse=" << fuse;
    }
}

TEST(TntRendererFlashFrameTest, ReturnsTrueWhenFuseBetween10And14)
{
    // fuseRemaining = 10,11,12,13,14 -> (int)/5 = 2, 2%2 = 0 -> 闪烁
    for (i32 fuse = 10; fuse < 15; ++fuse) {
        EXPECT_TRUE(TNTRenderer::isTntFlashFrame(static_cast<f32>(fuse))) << "fuse=" << fuse;
    }
}

TEST(TntRendererFlashFrameTest, ReturnsFalseWhenFuseBetween15And19)
{
    // fuseRemaining = 15,16,17,18,19 -> (int)/5 = 3, 3%2 = 1 -> 不闪烁
    for (i32 fuse = 15; fuse < 20; ++fuse) {
        EXPECT_FALSE(TNTRenderer::isTntFlashFrame(static_cast<f32>(fuse))) << "fuse=" << fuse;
    }
}

TEST(TntRendererFlashFrameTest, TruncatesFractionalFuseToInt)
{
    // fuseRemaining = 4.9 -> (int)4.9 = 4, 4/5 = 0, 0%2 = 0 -> 闪烁
    EXPECT_TRUE(TNTRenderer::isTntFlashFrame(4.9f));
    // fuseRemaining = 5.0 -> (int)5.0 = 5, 5/5 = 1, 1%2 = 1 -> 不闪烁
    EXPECT_FALSE(TNTRenderer::isTntFlashFrame(5.0f));
    // fuseRemaining = 9.9 -> (int)9.9 = 9, 9/5 = 1, 1%2 = 1 -> 不闪烁
    EXPECT_FALSE(TNTRenderer::isTntFlashFrame(9.9f));
    // fuseRemaining = 10.0 -> (int)10.0 = 10, 10/5 = 2, 2%2 = 0 -> 闪烁
    EXPECT_TRUE(TNTRenderer::isTntFlashFrame(10.0f));
}

TEST(TntRendererFlashFrameTest, AlternatesEvery5Ticks)
{
    // 验证完整的交替周期：0-4 闪, 5-9 不闪, 10-14 闪, 15-19 不闪
    for (i32 fuse = 0; fuse < 80; ++fuse) {
        const bool expected = (fuse / 5) % 2 == 0 && fuse >= 0;
        EXPECT_EQ(expected, TNTRenderer::isTntFlashFrame(static_cast<f32>(fuse))) << "fuse=" << fuse;
    }
}

// ============================================================================
// TNT 模型矩阵变换链测试
// ============================================================================

TEST(TntRendererModelMatrixTest, NoFlashScaleWhenFuseHigh)
{
    // fuseRemaining = 80（远大于 10），无闪烁缩放
    // 变换链: translate(0,0.5,0) * rotateY(-90°) * translate(-0.5,-0.5,0.5) * rotateY(90°)
    // 矩阵平移列 m[3]/m[7]/m[11] = M * 原点 = T1 * R1 * T2 * R2 * (0,0,0)
    //   R2 * (0,0,0) = (0,0,0)
    //   T2 * (0,0,0) = (-0.5, -0.5, 0.5)
    //   R1(-90°) * (-0.5, -0.5, 0.5) = (-0.5, -0.5, -0.5)
    //   T1 * (-0.5, -0.5, -0.5) = (-0.5, 0, -0.5)
    const auto m = TNTRenderer::buildTntModelMatrix(80.0f);

    EXPECT_NEAR(-0.5, m[3], 1e-10);  // X 平移 = 原点变换后的 X
    EXPECT_NEAR(0.0, m[7], 1e-10);   // Y 平移 = -0.5 + 0.5 = 0
    EXPECT_NEAR(-0.5, m[11], 1e-10); // Z 平移 = 原点变换后的 Z
}

TEST(TntRendererModelMatrixTest, AppliesFlashScaleWhenFuseLow)
{
    // fuseRemaining = 0: flashScale = 1.3
    // 变换链: translate(0,0.5,0) * scale(1.3) * rotateY(-90°) * translate(-0.5,-0.5,0.5) * rotateY(90°)
    // 矩阵平移列 = M * 原点 = T1 * S * R1 * T2 * R2 * (0,0,0)
    //   R2 * (0,0,0) = (0,0,0)
    //   T2 * (0,0,0) = (-0.5, -0.5, 0.5)
    //   S(1.3) * (-0.5, -0.5, 0.5) = (-0.65, -0.65, 0.65)
    //   R1(-90°) * (-0.65, -0.65, 0.65) = (-0.65, -0.65, -0.65)
    //   T1 * (-0.65, -0.65, -0.65) = (-0.65, -0.15, -0.65)
    const auto m = TNTRenderer::buildTntModelMatrix(0.0f);

    EXPECT_NEAR(-0.65, m[3], 1e-6);
    EXPECT_NEAR(-0.15, m[7], 1e-6); // -0.65 + 0.5 = -0.15
    EXPECT_NEAR(-0.65, m[11], 1e-6);
}

TEST(TntRendererModelMatrixTest, TransformVertexAtOriginNoFlash)
{
    // fuseRemaining = 80（无闪烁缩放）
    // 对单位方块顶点 (0, 0, 0) 应用变换链
    // MC PoseStack 右乘: M = T(0,0.5,0) * R(-90°) * T(-0.5,-0.5,0.5) * R(90°)
    // 顶点 (0,0,0):
    //   R(90°) * (0,0,0) = (0,0,0)
    //   T(-0.5,-0.5,0.5) * (0,0,0) = (-0.5, -0.5, 0.5)
    //   R(-90°) * (-0.5, -0.5, 0.5):
    //     rotateY(-90°): x' = c*x + s*z = 0*(-0.5) + (-1)*0.5 = -0.5
    //                    z' = -s*x + c*z = -(-1)*(-0.5) + 0*0.5 = -0.5
    //     => (-0.5, -0.5, -0.5)
    //   T(0,0.5,0) * (-0.5, -0.5, -0.5) = (-0.5, 0, -0.5)
    const auto m = TNTRenderer::buildTntModelMatrix(80.0f);
    const auto p = transformPoint(m, 0.0, 0.0, 0.0);
    EXPECT_NEAR(-0.5, p[0], 1e-6);
    EXPECT_NEAR(0.0, p[1], 1e-6);
    EXPECT_NEAR(-0.5, p[2], 1e-6);
}

TEST(TntRendererModelMatrixTest, TransformVertexAt111NoFlash)
{
    // 顶点 (1, 1, 1) 应用变换链（无闪烁缩放）
    // R(90°) * (1,1,1): rotateY(90°), c=0, s=1
    //   x' = c*x + s*z = 0*1 + 1*1 = 1
    //   z' = -s*x + c*z = -1*1 + 0*1 = -1
    //   => (1, 1, -1)
    // T(-0.5,-0.5,0.5) * (1, 1, -1) = (0.5, 0.5, -0.5)
    // R(-90°) * (0.5, 0.5, -0.5): c=0, s=-1
    //   x' = c*x + s*z = 0*0.5 + (-1)*(-0.5) = 0.5
    //   z' = -s*x + c*z = -(-1)*0.5 + 0*(-0.5) = 0.5
    //   => (0.5, 0.5, 0.5)
    // T(0,0.5,0) * (0.5, 0.5, 0.5) = (0.5, 1.0, 0.5)
    const auto m = TNTRenderer::buildTntModelMatrix(80.0f);
    const auto p = transformPoint(m, 1.0, 1.0, 1.0);
    EXPECT_NEAR(0.5, p[0], 1e-6);
    EXPECT_NEAR(1.0, p[1], 1e-6);
    EXPECT_NEAR(0.5, p[2], 1e-6);
}

TEST(TntRendererModelMatrixTest, RightMultiplyOrderMatchesMCPoseStack)
{
    // 验证矩阵乘法顺序为右乘（对应 MC PoseStack 语义）
    // M = T1 * [S] * R1 * T2 * R2
    // 若顺序错误（左乘），顶点变换结果会不同
    //
    // 关键验证：T1（translate(0,0.5,0)）在最外层，其 Y 平移不受内层变换影响
    // 对任意顶点 v，结果的 Y 分量 = 0.5 + (R1 * T2 * R2 * v).y
    // 对顶点 (0, 0, 0)：
    //   R2(90°) * (0,0,0) = (0,0,0)
    //   T2 * (0,0,0) = (-0.5, -0.5, 0.5)
    //   R1(-90°) * (-0.5, -0.5, 0.5) = (-0.5, -0.5, -0.5)  [Y 不变]
    //   T1 * (-0.5, -0.5, -0.5) = (-0.5, 0, -0.5)  [Y = -0.5 + 0.5 = 0]
    const auto m = TNTRenderer::buildTntModelMatrix(80.0f);
    const auto p = transformPoint(m, 0.0, 0.0, 0.0);
    // Y = 0 验证 T1 在最外层
    EXPECT_NEAR(0.0, p[1], 1e-6);
}

TEST(TntRendererModelMatrixTest, FlashScaleAffectsInnerVertices)
{
    // fuseRemaining = 0 时 flashScale = 1.3
    // 顶点 (0,0,0) 变换：
    //   R2(90°) * (0,0,0) = (0,0,0)
    //   T2 * (0,0,0) = (-0.5, -0.5, 0.5)
    //   S(1.3) * (-0.5, -0.5, 0.5) = (-0.65, -0.65, 0.65)
    //   R1(-90°) * (-0.65, -0.65, 0.65):
    //     x' = 0*(-0.65) + (-1)*0.65 = -0.65
    //     z' = -(-1)*(-0.65) + 0*0.65 = -0.65
    //     => (-0.65, -0.65, -0.65)
    //   T1(0,0.5,0) * (-0.65, -0.65, -0.65) = (-0.65, -0.15, -0.65)
    const auto m = TNTRenderer::buildTntModelMatrix(0.0f);
    const auto p = transformPoint(m, 0.0, 0.0, 0.0);
    EXPECT_NEAR(-0.65, p[0], 1e-5);
    EXPECT_NEAR(-0.15, p[1], 1e-5);
    EXPECT_NEAR(-0.65, p[2], 1e-5);
}

TEST(TntRendererModelMatrixTest, CubeCenterStaysCenteredWhenFlashScaleApplied)
{
    // 方块中心 (0.5, 0.5, 0.5) 在闪烁缩放下应保持居中（缩放围绕中心）
    // 无闪烁时:
    //   R2(90°) * (0.5, 0.5, 0.5) = (0.5, 0.5, -0.5)  [rotateY 90°: x'=z, z'=-x]
    //   T2 * (0.5, 0.5, -0.5) = (0, 0, 0)
    //   R1(-90°) * (0, 0, 0) = (0, 0, 0)
    //   T1(0, 0.5, 0) * (0, 0, 0) = (0, 0.5, 0)
    // 方块中心经变换后在 (0, 0.5, 0)（实体原点上方半个方块，即方块中心高度）
    const auto mNoFlash = TNTRenderer::buildTntModelMatrix(80.0f);
    const auto center = transformPoint(mNoFlash, 0.5, 0.5, 0.5);
    EXPECT_NEAR(0.0, center[0], 1e-6);
    EXPECT_NEAR(0.5, center[1], 1e-6);
    EXPECT_NEAR(0.0, center[2], 1e-6);

    // 有闪烁时，缩放围绕方块中心，中心点不变
    //   R2(90°) * (0.5, 0.5, 0.5) = (0.5, 0.5, -0.5)
    //   T2 * (0.5, 0.5, -0.5) = (0, 0, 0)
    //   S(1.3) * (0, 0, 0) = (0, 0, 0)  [原点缩放不变]
    //   R1(-90°) * (0, 0, 0) = (0, 0, 0)
    //   T1(0, 0.5, 0) * (0, 0, 0) = (0, 0.5, 0)
    const auto mFlash = TNTRenderer::buildTntModelMatrix(0.0f);
    const auto centerFlash = transformPoint(mFlash, 0.5, 0.5, 0.5);
    EXPECT_NEAR(0.0, centerFlash[0], 1e-5);
    EXPECT_NEAR(0.5, centerFlash[1], 1e-5);
    EXPECT_NEAR(0.0, centerFlash[2], 1e-5);
}

// ============================================================================
// FallingBlock 模型矩阵测试
// ============================================================================

TEST(FallingBlockRendererModelMatrixTest, ReturnsTranslateMinusHalfXAndZ)
{
    // FallingBlockRenderer 变换链: translate(-0.5, 0, -0.5)
    const auto m = FallingBlockRenderer::buildFallingBlockModelMatrix();

    // 验证平移分量
    EXPECT_NEAR(-0.5, m[3], 1e-10);  // X 平移
    EXPECT_NEAR(0.0, m[7], 1e-10);   // Y 平移
    EXPECT_NEAR(-0.5, m[11], 1e-10); // Z 平移
}

TEST(FallingBlockRendererModelMatrixTest, DiagonalIsIdentity)
{
    // 对角线应为 1（纯平移矩阵）
    const auto m = FallingBlockRenderer::buildFallingBlockModelMatrix();
    EXPECT_NEAR(1.0, m[0], 1e-10);
    EXPECT_NEAR(1.0, m[5], 1e-10);
    EXPECT_NEAR(1.0, m[10], 1e-10);
    EXPECT_NEAR(1.0, m[15], 1e-10);
}

TEST(FallingBlockRendererModelMatrixTest, OffDiagonalIsZero)
{
    // 非对角线非平移元素应为 0
    const auto m = FallingBlockRenderer::buildFallingBlockModelMatrix();
    EXPECT_NEAR(0.0, m[1], 1e-10);
    EXPECT_NEAR(0.0, m[2], 1e-10);
    EXPECT_NEAR(0.0, m[4], 1e-10);
    EXPECT_NEAR(0.0, m[6], 1e-10);
    EXPECT_NEAR(0.0, m[8], 1e-10);
    EXPECT_NEAR(0.0, m[9], 1e-10);
    EXPECT_NEAR(0.0, m[12], 1e-10);
    EXPECT_NEAR(0.0, m[13], 1e-10);
    EXPECT_NEAR(0.0, m[14], 1e-10);
}

TEST(FallingBlockRendererModelMatrixTest, TranslatesCubeCenterToOrigin)
{
    // 方块中心 (0.5, 0.5, 0.5) 经 translate(-0.5, 0, -0.5) 后应为 (0, 0.5, 0)
    // 即方块底部中心对齐实体原点
    const auto m = FallingBlockRenderer::buildFallingBlockModelMatrix();
    const auto p = transformPoint(m, 0.5, 0.5, 0.5);
    EXPECT_NEAR(0.0, p[0], 1e-10);
    EXPECT_NEAR(0.5, p[1], 1e-10);
    EXPECT_NEAR(0.0, p[2], 1e-10);
}

TEST(FallingBlockRendererModelMatrixTest, TranslatesCubeOriginToNegativeHalfXZ)
{
    // 方块原点 (0, 0, 0) 经变换后应为 (-0.5, 0, -0.5)
    const auto m = FallingBlockRenderer::buildFallingBlockModelMatrix();
    const auto p = transformPoint(m, 0.0, 0.0, 0.0);
    EXPECT_NEAR(-0.5, p[0], 1e-10);
    EXPECT_NEAR(0.0, p[1], 1e-10);
    EXPECT_NEAR(-0.5, p[2], 1e-10);
}

TEST(FallingBlockRendererModelMatrixTest, TranslatesCubeFarCorner)
{
    // 方块远角 (1, 1, 1) 经变换后应为 (0.5, 1, 0.5)
    const auto m = FallingBlockRenderer::buildFallingBlockModelMatrix();
    const auto p = transformPoint(m, 1.0, 1.0, 1.0);
    EXPECT_NEAR(0.5, p[0], 1e-10);
    EXPECT_NEAR(1.0, p[1], 1e-10);
    EXPECT_NEAR(0.5, p[2], 1e-10);
}
