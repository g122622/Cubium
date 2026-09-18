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
 * @file VehicleRendererMatrixTest.cpp
 * @brief 载具渲染器模型矩阵构建与 TNT 闪烁计算单元测试
 *
 * 验证内容：
 * - 行主序矩阵工具（matrix 命名空间）构造与乘法的正确性
 * - BoatRenderer / MinecartRenderer 的 computeCustomModelMatrix 在已知同步状态下
 *   产出的矩阵与手算的 MC Java 变换链一致
 * - MinecartRenderer::calculateTntFlashScale / isTntFlashFrame 对齐 MC TntMinecartRenderer
 *
 * 矩阵布局：行主序 std::array<f64, 16>，索引 [row*4 + col]，
 *   矩阵-向量乘法：result.x = m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3]
 * 平移分量位于 m[3], m[7], m[11]。
 */

#include "client/renderer/trident/entity/renderer/vehicle/VehicleRenderers.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client;
using namespace mc::client::renderer::entity::renderer::vehicle;

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
// 矩阵工具契约测试
// ============================================================================

TEST(VehicleRendererMatrixTest, IdentityMatrixHasCorrectLayout)
{
    const auto m = matrix::identity();
    // 对角线为 1
    EXPECT_DOUBLE_EQ(1.0, m[0]);
    EXPECT_DOUBLE_EQ(1.0, m[5]);
    EXPECT_DOUBLE_EQ(1.0, m[10]);
    EXPECT_DOUBLE_EQ(1.0, m[15]);
    // 平移列为 0
    EXPECT_DOUBLE_EQ(0.0, m[3]);
    EXPECT_DOUBLE_EQ(0.0, m[7]);
    EXPECT_DOUBLE_EQ(0.0, m[11]);
}

TEST(VehicleRendererMatrixTest, TranslationMatrixStoresInTranslationColumn)
{
    const auto m = matrix::translation(1.5, -2.0, 3.25);
    // 行主序：平移位于 m[3], m[7], m[11]
    EXPECT_DOUBLE_EQ(1.5, m[3]);
    EXPECT_DOUBLE_EQ(-2.0, m[7]);
    EXPECT_DOUBLE_EQ(3.25, m[11]);
    // 对角线仍为 1
    EXPECT_DOUBLE_EQ(1.0, m[0]);
    EXPECT_DOUBLE_EQ(1.0, m[5]);
    EXPECT_DOUBLE_EQ(1.0, m[10]);
}

TEST(VehicleRendererMatrixTest, ScaleMatrixIsDiagonal)
{
    const auto m = matrix::scale(2.0, -3.0, 0.5);
    EXPECT_DOUBLE_EQ(2.0, m[0]);
    EXPECT_DOUBLE_EQ(-3.0, m[5]);
    EXPECT_DOUBLE_EQ(0.5, m[10]);
    EXPECT_DOUBLE_EQ(1.0, m[15]);
    // 非对角线为 0
    EXPECT_DOUBLE_EQ(0.0, m[1]);
    EXPECT_DOUBLE_EQ(0.0, m[3]);
}

TEST(VehicleRendererMatrixTest, RotationY90DegreesMapsXToNegativeZ)
{
    // 绕 Y 轴 90 度：x 轴映射到 -z 轴
    const auto m = matrix::rotationY(static_cast<f64>(mc::math::PI_DOUBLE / 2.0));
    const auto p = transformPoint(m, 1.0, 0.0, 0.0);
    // 旋转 90 度后 (1,0,0) -> (0,0,-1)（右手系绕 Y 轴）
    EXPECT_NEAR(0.0, p[0], 1e-10);
    EXPECT_NEAR(0.0, p[1], 1e-10);
    EXPECT_NEAR(-1.0, p[2], 1e-10);
}

TEST(VehicleRendererMatrixTest, RotationX90DegreesMapsYToZ)
{
    const auto m = matrix::rotationX(static_cast<f64>(mc::math::PI_DOUBLE / 2.0));
    const auto p = transformPoint(m, 0.0, 1.0, 0.0);
    // 绕 X 轴 90 度：(0,1,0) -> (0,0,1)
    EXPECT_NEAR(0.0, p[0], 1e-10);
    EXPECT_NEAR(0.0, p[1], 1e-10);
    EXPECT_NEAR(1.0, p[2], 1e-10);
}

TEST(VehicleRendererMatrixTest, RotationZ90DegreesMapsXToY)
{
    const auto m = matrix::rotationZ(static_cast<f64>(mc::math::PI_DOUBLE / 2.0));
    const auto p = transformPoint(m, 1.0, 0.0, 0.0);
    // 绕 Z 轴 90 度：(1,0,0) -> (0,1,0)
    EXPECT_NEAR(0.0, p[0], 1e-10);
    EXPECT_NEAR(1.0, p[1], 1e-10);
    EXPECT_NEAR(0.0, p[2], 1e-10);
}

TEST(VehicleRendererMatrixTest, RotationAxisZeroAxisReturnsIdentity)
{
    // 零轴应安全返回单位矩阵
    const auto m = matrix::rotationAxis(1.0, 0.0, 0.0, 0.0);
    EXPECT_DOUBLE_EQ(1.0, m[0]);
    EXPECT_DOUBLE_EQ(1.0, m[5]);
    EXPECT_DOUBLE_EQ(1.0, m[10]);
    EXPECT_DOUBLE_EQ(1.0, m[15]);
}

TEST(VehicleRendererMatrixTest, RotationAxis45DegreesAroundYMatchesRotationY)
{
    const f64 angle = static_cast<f64>(mc::math::PI / 4.0);
    const auto mAxis = matrix::rotationAxis(angle, 0.0, 1.0, 0.0);
    const auto mY = matrix::rotationY(angle);
    expectMatrixNear(mY, mAxis);
}

TEST(VehicleRendererMatrixTest, MultiplyIdentityIsNoOp)
{
    const auto t = matrix::translation(1.0, 2.0, 3.0);
    const auto i = matrix::identity();
    const auto result = matrix::multiply(i, t);
    expectMatrixNear(t, result);
}

TEST(VehicleRendererMatrixTest, MultiplyTranslationThenRotationYieldsComposite)
{
    // 先平移 (1,2,3) 再绕 Y 90 度
    // 矩阵乘法 m = r * t，对点 v：m * v = r * (t * v)
    // 即先平移再旋转。点 (0,0,0) 经平移 -> (1,2,3)，
    // 再绕 Y 90 度（右手系：x→-z）：(1,2,3) -> (3,2,-1)
    const auto t = matrix::translation(1.0, 2.0, 3.0);
    const auto r = matrix::rotationY(static_cast<f64>(mc::math::PI_DOUBLE / 2.0));
    const auto m = matrix::multiply(r, t);
    const auto p = transformPoint(m, 0.0, 0.0, 0.0);
    EXPECT_NEAR(3.0, p[0], 1e-10);
    EXPECT_NEAR(2.0, p[1], 1e-10);
    EXPECT_NEAR(-1.0, p[2], 1e-10);
}

// ============================================================================
// TNT 闪烁计算测试
// ============================================================================

TEST(VehicleRendererTntFlashTest, FlashScaleIsOneWhenUnprimed)
{
    // fuse = -1 表示未点燃，缩放因子为 1
    EXPECT_DOUBLE_EQ(1.0, MinecartRenderer::calculateTntFlashScale(-1));
}

TEST(VehicleRendererTntFlashTest, FlashScaleIsOneWhenFuseAtLeast10)
{
    // fuse >= 10 时不缩放
    EXPECT_DOUBLE_EQ(1.0, MinecartRenderer::calculateTntFlashScale(10));
    EXPECT_DOUBLE_EQ(1.0, MinecartRenderer::calculateTntFlashScale(20));
    EXPECT_DOUBLE_EQ(1.0, MinecartRenderer::calculateTntFlashScale(80));
}

TEST(VehicleRendererTntFlashTest, FlashScaleGrowsAsFuseDecreases)
{
    // fuse 越接近 0，缩放因子越大
    const f64 at9 = MinecartRenderer::calculateTntFlashScale(9);
    const f64 at5 = MinecartRenderer::calculateTntFlashScale(5);
    const f64 at1 = MinecartRenderer::calculateTntFlashScale(1);
    const f64 at0 = MinecartRenderer::calculateTntFlashScale(0);
    EXPECT_GT(at5, at9);
    EXPECT_GT(at1, at5);
    EXPECT_GT(at0, at1);
    // fuse = 0 时 f = 1，缩放 = 1 + 1*0.3 = 1.3
    EXPECT_NEAR(1.3, at0, 1e-10);
}

TEST(VehicleRendererTntFlashTest, FlashScaleHandComputedAtFuse5)
{
    // fuse = 5: f = 1 - 0.5 = 0.5; f^4 = 0.0625; scale = 1 + 0.0625*0.3 = 1.01875
    const f64 scale = MinecartRenderer::calculateTntFlashScale(5);
    EXPECT_NEAR(1.01875, scale, 1e-10);
}

TEST(VehicleRendererTntFlashTest, FlashFrameFalseWhenUnprimed)
{
    EXPECT_FALSE(MinecartRenderer::isTntFlashFrame(-1));
    EXPECT_FALSE(MinecartRenderer::isTntFlashFrame(-5));
}

TEST(VehicleRendererTntFlashTest, FlashFrameAlternatesEvery5Ticks)
{
    // fuse / 5 % 2 == 0 即为闪烁帧
    // fuse = 0,1,2,3,4 -> 0/5=0, 0%2=0 -> 闪烁
    // fuse = 5,6,7,8,9 -> 1, 1%2=1 -> 不闪烁
    // fuse = 10,11,12,13,14 -> 2, 2%2=0 -> 闪烁
    for (i32 fuse = 0; fuse < 5; ++fuse) {
        EXPECT_TRUE(MinecartRenderer::isTntFlashFrame(fuse)) << "fuse=" << fuse;
    }
    for (i32 fuse = 5; fuse < 10; ++fuse) {
        EXPECT_FALSE(MinecartRenderer::isTntFlashFrame(fuse)) << "fuse=" << fuse;
    }
    for (i32 fuse = 10; fuse < 15; ++fuse) {
        EXPECT_TRUE(MinecartRenderer::isTntFlashFrame(fuse)) << "fuse=" << fuse;
    }
}

// ============================================================================
// 船渲染器模型矩阵构建测试
// ============================================================================

class BoatRendererMatrixTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:boat");
        renderer = std::make_unique<BoatRenderer>(BoatType::Oak);
    }

    void TearDown() override
    {
        entity.reset();
        renderer.reset();
    }

    std::unique_ptr<ClientEntity> entity;
    std::unique_ptr<BoatRenderer> renderer;
};

TEST_F(BoatRendererMatrixTest, CustomMatrixReturnsTrue)
{
    std::array<f64, 16> m{};
    f32 hurtTime = -1.0f;
    f32 deathTime = -1.0f;
    const bool used = renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);
    EXPECT_TRUE(used);
}

TEST_F(BoatRendererMatrixTest, HurtTimeAndDeathTimeZeroedForBoat)
{
    // 船不是 LivingEntity，不使用着色器红色闪烁
    std::array<f64, 16> m{};
    f32 hurtTime = -1.0f;
    f32 deathTime = -1.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);
    EXPECT_FLOAT_EQ(0.0f, hurtTime);
    EXPECT_FLOAT_EQ(0.0f, deathTime);
}

TEST_F(BoatRendererMatrixTest, YawZeroYieldsTranslateThenRotateY180)
{
    // yaw = 0, hurtTime = 0, bubble = 0
    // 变换链：translate(0, 0.375, 0) * rotateY(180) * scale(-1,-1,1) * rotateY(90)
    // 点 (0,0,0) 经 rotateY(90) -> (0,0,0)
    //          经 scale(-1,-1,1) -> (0,0,0)
    //          经 rotateY(180) -> (0,0,0)
    //          经 translate(0,0.375,0) -> (0, 0.375, 0)
    entity->setRotation(0.0f, 0.0f);
    std::array<f64, 16> m{};
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);

    const auto p = transformPoint(m, 0.0, 0.0, 0.0);
    EXPECT_NEAR(0.0, p[0], 1e-6);
    EXPECT_NEAR(0.375, p[1], 1e-6);
    EXPECT_NEAR(0.0, p[2], 1e-6);
}

TEST_F(BoatRendererMatrixTest, HurtShakeAppliedWhenTimeSinceHitPositive)
{
    // 设置 timeSinceHit = 5, damage = 5, forwardDir = 1
    auto& dm = entity->dataManager();
    dm.registerParam(::mc::entity::BoatEntity::getTimeSinceHitParam(), 5);
    dm.registerParam(::mc::entity::BoatEntity::getForwardDirectionParam(), 1);
    dm.registerParam(::mc::entity::BoatEntity::getDamageTakenParam(), 5.0f);

    entity->setRotation(0.0f, 0.0f);
    std::array<f64, 16> m{};
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);

    // hurtTime = 5 - 0 = 5, damageTime = max(5 - 0, 0) = 5
    // shakeDeg = sin(5) * 5 * 5 / 10 * 1 = sin(5) * 2.5
    // 应用 rotateX(shakeDeg) 后，y 轴上的点 (0,1,0) 应被旋转
    // 若无 shake，(0,1,0) 经变换链后 y 分量为 -1 + 0.375 = -0.625（scale(-1,-1,1) 翻转）
    // 有 shake 则 y/z 分量会变化
    const auto p = transformPoint(m, 0.0, 1.0, 0.0);
    // 粗略验证：y 分量不等于 -0.625（即发生了旋转）
    EXPECT_NE(-0.625, p[1]);
}

TEST_F(BoatRendererMatrixTest, NoHurtShakeWhenTimeSinceHitZero)
{
    // timeSinceHit = 0, partialTicks = 0 -> hurtTime = 0, 不应用 shake
    auto& dm = entity->dataManager();
    dm.registerParam(::mc::entity::BoatEntity::getTimeSinceHitParam(), 0);
    dm.registerParam(::mc::entity::BoatEntity::getForwardDirectionParam(), 1);
    dm.registerParam(::mc::entity::BoatEntity::getDamageTakenParam(), 10.0f);

    entity->setRotation(0.0f, 0.0f);
    std::array<f64, 16> m{};
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);

    // 无 shake，点 (0,1,0) 经 rotateY(90) -> (0,1,0)
    //                  经 scale(-1,-1,1) -> (0,-1,0)
    //                  经 rotateY(180) -> (0,-1,0)
    //                  经 translate(0,0.375,0) -> (0,-0.625,0)
    const auto p = transformPoint(m, 0.0, 1.0, 0.0);
    EXPECT_NEAR(0.0, p[0], 1e-6);
    EXPECT_NEAR(-0.625, p[1], 1e-6);
    EXPECT_NEAR(0.0, p[2], 1e-6);
}

// ============================================================================
// 矿车渲染器模型矩阵构建测试
// ============================================================================

class MinecartRendererMatrixTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:minecart");
        renderer = std::make_unique<MinecartRenderer>();
    }

    void TearDown() override
    {
        entity.reset();
        renderer.reset();
    }

    std::unique_ptr<ClientEntity> entity;
    std::unique_ptr<MinecartRenderer> renderer;
};

TEST_F(MinecartRendererMatrixTest, CustomMatrixReturnsTrue)
{
    std::array<f64, 16> m{};
    f32 hurtTime = -1.0f;
    f32 deathTime = -1.0f;
    const bool used = renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);
    EXPECT_TRUE(used);
}

TEST_F(MinecartRendererMatrixTest, DeathTimeZeroedForMinecart)
{
    std::array<f64, 16> m{};
    f32 hurtTime = -1.0f;
    f32 deathTime = -1.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);
    EXPECT_FLOAT_EQ(0.0f, deathTime);
}

TEST_F(MinecartRendererMatrixTest, YawZeroPitchZeroYieldsTranslateThenRotateY180ThenScale)
{
    // yaw = 0, pitch = 0, hurtTime = 0
    // 变换链：translate(0, 0.375, 0) * rotateY(180) * rotateZ(0) * scale(-1,-1,1)
    // 点 (0,0,0) -> (0, 0.375, 0)
    entity->setRotation(0.0f, 0.0f);
    std::array<f64, 16> m{};
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);

    const auto p = transformPoint(m, 0.0, 0.0, 0.0);
    EXPECT_NEAR(0.0, p[0], 1e-6);
    EXPECT_NEAR(0.375, p[1], 1e-6);
    EXPECT_NEAR(0.0, p[2], 1e-6);
}

TEST_F(MinecartRendererMatrixTest, HurtTimeZeroedForNonTntMinecart)
{
    // 普通矿车不闪烁
    std::array<f64, 16> m{};
    f32 hurtTime = -1.0f;
    f32 deathTime = -1.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);
    EXPECT_FLOAT_EQ(0.0f, hurtTime);
}

TEST_F(MinecartRendererMatrixTest, HurtShakeNotAppliedAfterRollingWireRemoval)
{
    // 矿车摇晃动画在 vanilla 1.21.11 走 EntityEvent 而非 SynchedEntityData;对齐 vanilla 时已删
    // 项目自创的 rolling_amp/rolling_dir wire 字段,渲染器改用本地默认 0/1,客户端暂时看不到
    // 受损摇晃动画(功能回退,TODO 待 EntityEvent 接入)。故即使 damage>0,hurt shake 也不应用。
    // TODO: 待 EntityEvent 摇晃状态码接入后,恢复本测试为 HurtShakeApplied 验证 shake 生效。
    auto& dm = entity->dataManager();
    dm.registerParam(::mc::entity::AbstractMinecartEntity::getDamageParam(), 5.0f);

    entity->setRotation(0.0f, 0.0f);
    std::array<f64, 16> m{};
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);

    // 回退后无 shake:(0,1,0) 经变换链后 y 分量为 -0.625（scale(-1,-1,1) 翻转 + translate 0.375）。
    const auto p = transformPoint(m, 0.0, 1.0, 0.0);
    EXPECT_DOUBLE_EQ(-0.625, p[1]);
}

TEST_F(MinecartRendererMatrixTest, TntMinecartAppliesFlashScaleWhenFuseLow)
{
    // TNT 矿车 + fuse = 5 -> 应用缩放
    entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:tnt_minecart");
    entity->setRotation(0.0f, 0.0f);
    entity->setFuseTimer(5);

    std::array<f64, 16> m{};
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);

    // fuse = 5 是闪烁帧 (5/5=1, 1%2=1 -> false)，hurtTime 应为 0
    EXPECT_FALSE(MinecartRenderer::isTntFlashFrame(5));
    EXPECT_FLOAT_EQ(0.0f, hurtTime);

    // 但缩放仍应用：flashScale = 1.01875
    // 矿车变换链：translate(0,0.375,0) * rotateY(180) * rotateZ(0) * scale(-1,-1,1) * scale(1.01875)
    // 点 (1,0,0):
    //   scale(1.01875) -> (1.01875, 0, 0)
    //   scale(-1,-1,1) -> (-1.01875, 0, 0)
    //   rotateZ(0)     -> (-1.01875, 0, 0)
    //   rotateY(180)   -> (1.01875, 0, 0)   (c=-1, s=0: c*x+s*z = -1*-1.01875 = 1.01875)
    //   translate      -> (1.01875, 0.375, 0)
    const auto p = transformPoint(m, 1.0, 0.0, 0.0);
    EXPECT_NEAR(1.01875, p[0], 1e-6);
    EXPECT_NEAR(0.375, p[1], 1e-6);
    EXPECT_NEAR(0.0, p[2], 1e-6);
}

TEST_F(MinecartRendererMatrixTest, TntMinecartFlashFrameSetsHurtTime)
{
    // TNT 矿车 + fuse = 0 -> 闪烁帧 (0/5=0, 0%2=0 -> true)
    entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:tnt_minecart");
    entity->setRotation(0.0f, 0.0f);
    entity->setFuseTimer(0);

    std::array<f64, 16> m{};
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);

    EXPECT_TRUE(MinecartRenderer::isTntFlashFrame(0));
    // 闪烁帧时 hurtTime 被设为 1.0（通过 hurtTime 通道近似白色叠加）
    EXPECT_FLOAT_EQ(1.0f, hurtTime);
}

TEST_F(MinecartRendererMatrixTest, TntMinecartUnprimedNoFlash)
{
    // TNT 矿车 + fuse = -1（未点燃） -> 不缩放、不闪烁
    entity = std::make_unique<ClientEntity>(EntityInstanceId(1), "minecraft:tnt_minecart");
    entity->setRotation(0.0f, 0.0f);
    entity->setFuseTimer(-1);

    std::array<f64, 16> m{};
    f32 hurtTime = -1.0f;
    f32 deathTime = -1.0f;
    renderer->computeCustomModelMatrix(*entity, 0.0, m, hurtTime, deathTime);

    EXPECT_FLOAT_EQ(0.0f, hurtTime);
    // 不缩放，矿车变换链：translate(0,0.375,0) * rotateY(180) * rotateZ(0) * scale(-1,-1,1)
    // 点 (1,0,0):
    //   scale(-1,-1,1) -> (-1, 0, 0)
    //   rotateZ(0)     -> (-1, 0, 0)
    //   rotateY(180)   -> (1, 0, 0)
    //   translate      -> (1, 0.375, 0)
    const auto p = transformPoint(m, 1.0, 0.0, 0.0);
    EXPECT_NEAR(1.0, p[0], 1e-6);
    EXPECT_NEAR(0.375, p[1], 1e-6);
    EXPECT_NEAR(0.0, p[2], 1e-6);
}
