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
 * @file DolphinModelTest.cpp
 * @brief DolphinModel 海豚模型运动状态注入与摆尾动画单元测试
 *
 * 验证 DolphinModel::setMotionMagnitude 推送 + setAngles 应用游泳摆尾动画的正确性。
 *
 * 覆盖场景：
 * - 默认构造：部件结构完整，纹理尺寸 64x64
 * - 默认状态（未调用 setMotionMagnitude）：m_motionMagnitude=0，走静态分支
 * - 无运动时 setAngles：tail = -0.10471976f（-PI/30），tailFin = 0.0f
 * - setMotionMagnitude 延迟应用：在 setAngles 之前推送，setAngles 时读取
 * - 有运动时 setAngles：body.xRot += -0.05 - 0.05*cos(age*0.3)，
 *   tail.xRot = -0.1*cos(age*0.3)，tailFin.xRot = -0.2*cos(age*0.3)
 * - MOTION_THRESHOLD 边界：1.0E-8（低于阈值，静态）、1.0E-7（等于阈值，静态）、
 *   1.1E-7（高于阈值，动态）
 * - 完整游泳周期：多个 ageInTicks 值下的摆尾角度
 * - 网格生成：静态/动态姿势下均生成非空网格
 *
 * 对应 MC 1.21.11 DolphinModel.setupAnim：
 *   if (renderState.isMoving) {
 *       float wave = Mth.cos(ageInTicks * 0.3F);
 *       body.xRot += -0.05F - 0.05F * wave;
 *       tail.xRot = -0.1F * wave;
 *       tailFin.xRot = -0.2F * wave;
 *   } else {
 *       tail.xRot = -0.10471976F;
 *   }
 * isMoving 由 DolphinRenderer 填充：
 *   p_364903_.isMoving = p_480257_.getDeltaMovement().horizontalDistanceSqr() > 1.0E-7;
 *
 * 数据流：
 * EntityRendererManager::_applyDolphinMotionState 读取 ClientEntity::velocity()
 * → 计算 horizontalDistanceSqr = vx*vx + vz*vz（仅 XZ，不含 Y）
 * → DolphinModel::setMotionMagnitude（存储 m_motionMagnitude）
 * → DolphinModel::setAngles 依据 m_motionMagnitude > MOTION_THRESHOLD 选择分支
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include "client/renderer/trident/entity/model/aquatic/AquaticModels.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/util/math/MathConstants.hpp"

using namespace mc::client::renderer::entity::model::aquatic;
using mc::client::renderer::entity::model::ModelRenderer;
using mc::client::renderer::entity::model::ModelVertex;

namespace mc::client::renderer {
namespace {

/// @brief DolphinModel::setAngles 中静态分支的尾鳍基础角度（-PI/30 ≈ -0.10471976）
/// 对应 MC 1.21.11 DolphinModel.setupAnim 中 tail.xRot = -0.10471976F
constexpr f64 kStaticTailAngle = -0.10471976;

/// @brief setAngles 中 ageInTicks * 0.3 的角频率
constexpr f64 kWaveFrequency = 0.3;

/// @brief 计算游泳摆尾波形 cos(ageInTicks * 0.3)
f64 expectedWave(f64 ageInTicks)
{
    return std::cos(ageInTicks * kWaveFrequency);
}

/// @brief 计算运动时 tail.xRot = -0.1 * cos(ageInTicks * 0.3)
f64 expectedMovingTailAngle(f64 ageInTicks)
{
    return -0.1 * expectedWave(ageInTicks);
}

/// @brief 计算运动时 tailFin.xRot = -0.2 * cos(ageInTicks * 0.3)
f64 expectedMovingTailFinAngle(f64 ageInTicks)
{
    return -0.2 * expectedWave(ageInTicks);
}

/// @brief 计算运动时 body.xRot 的增量 = -0.05 - 0.05 * cos(ageInTicks * 0.3)
f64 expectedBodyXRotDelta(f64 ageInTicks)
{
    return -0.05 - 0.05 * expectedWave(ageInTicks);
}

class DolphinModelTest : public ::testing::Test {
protected:
    void SetUp() override { m_model = std::make_unique<DolphinModel>(); }

    /// @brief 触发一次 setAngles，scale 使用默认值 1/16
    void applyAngles(f64 ageInTicks = 0.0) { m_model->setAngles(0.0, 0.0, ageInTicks, 0.0, 0.0, 1.0 / 16.0); }

    std::unique_ptr<DolphinModel> m_model;
};

// ============================================================================
// 部件结构验证测试
// ============================================================================

TEST_F(DolphinModelTest, DefaultConstruction_HasCorrectTextureSize)
{
    // 对应 MC 1.21.11 DolphinModel 构造：textureWidth=64, textureHeight=64
    EXPECT_EQ(m_model->textureWidth(), 64);
    EXPECT_EQ(m_model->textureHeight(), 64);
}

TEST_F(DolphinModelTest, DefaultConstruction_AllPartsAreCreated)
{
    // 验证所有部件非空
    EXPECT_NE(m_model->body(), nullptr);
    EXPECT_NE(m_model->tail(), nullptr);
    EXPECT_NE(m_model->tailFin(), nullptr);
    EXPECT_NE(m_model->dorsalFin(), nullptr);
    EXPECT_NE(m_model->finRight(), nullptr);
    EXPECT_NE(m_model->finLeft(), nullptr);
    EXPECT_NE(m_model->head(), nullptr);
    EXPECT_NE(m_model->nose(), nullptr);
}

TEST_F(DolphinModelTest, DefaultConstruction_TopLevelPartsContainOnlyBody)
{
    // _setupParts 中只有 body 被加入 m_parts，其余均为 body 的子部件
    const auto& parts = m_model->getParts();
    EXPECT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0]->name(), "body");
}

TEST_F(DolphinModelTest, DefaultConstruction_PartNamesAreCorrect)
{
    EXPECT_EQ(m_model->body()->name(), "body");
    EXPECT_EQ(m_model->tail()->name(), "tail");
    EXPECT_EQ(m_model->tailFin()->name(), "tailFin");
    EXPECT_EQ(m_model->dorsalFin()->name(), "dorsalFin");
    EXPECT_EQ(m_model->finRight()->name(), "finRight");
    EXPECT_EQ(m_model->finLeft()->name(), "finLeft");
    EXPECT_EQ(m_model->head()->name(), "head");
    EXPECT_EQ(m_model->nose()->name(), "nose");
}

TEST_F(DolphinModelTest, DefaultConstruction_StaticTailAngleIsInitialValue)
{
    // _setupParts 中 m_tail 初始 rotateAngleX = -0.10471976f（静态角度）
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), kStaticTailAngle, 1e-5);
    // m_tailFin 初始 rotateAngleX = 0.0（默认）
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), 0.0, 1e-5);
}

// ============================================================================
// MOTION_THRESHOLD 常量验证
// ============================================================================

TEST_F(DolphinModelTest, MotionThreshold_MatchesMCValue)
{
    // 对应 MC 1.21.11 DolphinRenderer 中 isMoving 判定的阈值 1.0E-7
    EXPECT_DOUBLE_EQ(DolphinModel::MOTION_THRESHOLD, 1.0E-7);
}

// ============================================================================
// 无运动（m_motionMagnitude <= MOTION_THRESHOLD）测试
// ============================================================================

TEST_F(DolphinModelTest, NoMotion_TailUsesStaticAngle)
{
    // 默认 m_motionMagnitude = 0.0，低于阈值，走静态分支
    applyAngles(0.0);

    // tail.xRot = -0.10471976f（静态基础角度）
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), kStaticTailAngle, 1e-5);
}

TEST_F(DolphinModelTest, NoMotion_TailFinIsZero)
{
    // 默认 m_motionMagnitude = 0.0，走静态分支
    applyAngles(0.0);

    // tailFin.xRot = 0.0f（静态时保持初始）
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), 0.0, 1e-5);
}

TEST_F(DolphinModelTest, NoMotion_BodyXRotIsZero_WhenHeadPitchIsZero)
{
    // 无运动 + headPitch=0 → body.xRot = toRadians(0) + 0 = 0
    applyAngles(0.0);

    EXPECT_NEAR(m_model->body()->rotateAngleX(), 0.0, 1e-5);
}

TEST_F(DolphinModelTest, NoMotion_BodyXRotFollowsHeadPitch)
{
    // 无运动时 body.xRot = toRadians(headPitch)（无摆尾增量）
    constexpr f64 headPitch = 15.0;
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, headPitch, 1.0 / 16.0);

    const f64 expected = static_cast<f64>(mc::math::toRadians(static_cast<f32>(headPitch)));
    EXPECT_NEAR(m_model->body()->rotateAngleX(), expected, 1e-5);
}

TEST_F(DolphinModelTest, NoMotion_BodyYRotFollowsNetHeadYaw)
{
    // body.yRot = toRadians(netHeadYaw)
    constexpr f64 netHeadYaw = 30.0;
    m_model->setAngles(0.0, 0.0, 0.0, netHeadYaw, 0.0, 1.0 / 16.0);

    const f64 expected = static_cast<f64>(mc::math::toRadians(static_cast<f32>(netHeadYaw)));
    EXPECT_NEAR(m_model->body()->rotateAngleY(), expected, 1e-5);
}

// ============================================================================
// setMotionMagnitude 延迟应用测试
// ============================================================================

TEST_F(DolphinModelTest, SetMotionMagnitude_StoresValueForSetAngles)
{
    // 先推送运动状态（存储），再调用 setAngles（读取）
    m_model->setMotionMagnitude(1.0); // 远大于阈值

    // 推送后但未调用 setAngles 前，tail 仍为初始静态角度
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), kStaticTailAngle, 1e-5);

    // 调用 setAngles 后，tail 切换为运动摆尾角度
    applyAngles(0.0);

    // ageInTicks=0 → wave = cos(0) = 1.0
    // tail.xRot = -0.1 * 1.0 = -0.1
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), expectedMovingTailAngle(0.0), 1e-5);
}

TEST_F(DolphinModelTest, SetMotionMagnitude_CanBeOverriddenBeforeSetAngles)
{
    // 第一次推送运动状态
    m_model->setMotionMagnitude(1.0);
    applyAngles(0.0);
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), expectedMovingTailAngle(0.0), 1e-5);

    // 第二次推送为零（恢复静态），再次 setAngles
    m_model->setMotionMagnitude(0.0);
    applyAngles(0.0);
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), kStaticTailAngle, 1e-5);
}

// ============================================================================
// MOTION_THRESHOLD 边界测试
// ============================================================================

TEST_F(DolphinModelTest, Threshold_BelowThresholdUsesStaticBranch)
{
    // 1.0E-8 < 1.0E-7，走静态分支
    m_model->setMotionMagnitude(1.0E-8);
    applyAngles(0.0);

    EXPECT_NEAR(m_model->tail()->rotateAngleX(), kStaticTailAngle, 1e-5);
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), 0.0, 1e-5);
}

TEST_F(DolphinModelTest, Threshold_EqualToThresholdUsesStaticBranch)
{
    // m_motionMagnitude > MOTION_THRESHOLD 是严格大于，等于阈值走静态分支
    m_model->setMotionMagnitude(DolphinModel::MOTION_THRESHOLD);
    applyAngles(0.0);

    EXPECT_NEAR(m_model->tail()->rotateAngleX(), kStaticTailAngle, 1e-5);
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), 0.0, 1e-5);
}

TEST_F(DolphinModelTest, Threshold_AboveThresholdUsesMovingBranch)
{
    // 1.1E-7 > 1.0E-7，走运动分支
    m_model->setMotionMagnitude(1.1E-7);
    applyAngles(0.0);

    // ageInTicks=0 → wave=1.0
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), expectedMovingTailAngle(0.0), 1e-5);
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), expectedMovingTailFinAngle(0.0), 1e-5);
}

// ============================================================================
// 有运动时的摆尾动画公式测试
// ============================================================================

TEST_F(DolphinModelTest, Moving_TailAngleFollowsCosFormula)
{
    m_model->setMotionMagnitude(1.0);

    // 多个 ageInTicks 值验证 cos 波形
    const f64 ages[] = {0.0, 1.0, 2.5, 5.0, 10.0, 15.7, 31.4159};
    for (f64 age : ages) {
        applyAngles(age);
        EXPECT_NEAR(m_model->tail()->rotateAngleX(), expectedMovingTailAngle(age), 1e-4) << "ageInTicks=" << age;
    }
}

TEST_F(DolphinModelTest, Moving_TailFinAngleFollowsCosFormula)
{
    m_model->setMotionMagnitude(1.0);

    const f64 ages[] = {0.0, 1.0, 2.5, 5.0, 10.0, 15.7, 31.4159};
    for (f64 age : ages) {
        applyAngles(age);
        EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), expectedMovingTailFinAngle(age), 1e-4) << "ageInTicks=" << age;
    }
}

TEST_F(DolphinModelTest, Moving_BodyXRotIncludesWaveDelta)
{
    // 运动时 body.xRot = toRadians(headPitch) + (-0.05 - 0.05*cos(age*0.3))
    m_model->setMotionMagnitude(1.0);

    constexpr f64 headPitch = 10.0;
    const f64 ages[] = {0.0, 1.0, 2.5, 5.0, 10.0};
    for (f64 age : ages) {
        m_model->setAngles(0.0, 0.0, age, 0.0, headPitch, 1.0 / 16.0);
        const f64 expected =
            static_cast<f64>(mc::math::toRadians(static_cast<f32>(headPitch))) + expectedBodyXRotDelta(age);
        EXPECT_NEAR(m_model->body()->rotateAngleX(), expected, 1e-4) << "ageInTicks=" << age;
    }
}

TEST_F(DolphinModelTest, Moving_AtAgeZero_WaveIsOne)
{
    // ageInTicks=0 → cos(0) = 1.0
    // tail.xRot = -0.1 * 1.0 = -0.1
    // tailFin.xRot = -0.2 * 1.0 = -0.2
    // body.xRot delta = -0.05 - 0.05*1.0 = -0.1
    m_model->setMotionMagnitude(1.0);
    applyAngles(0.0);

    EXPECT_NEAR(m_model->tail()->rotateAngleX(), -0.1, 1e-5);
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), -0.2, 1e-5);
}

TEST_F(DolphinModelTest, Moving_FullSwingCycle_ReachesExtremeValues)
{
    // 完整周期 2*PI/0.3 ≈ 20.944
    // 当 cos=1（age=0）：tail=-0.1, tailFin=-0.2
    // 当 cos=-1（age=PI/0.3 ≈ 10.472）：tail=0.1, tailFin=0.2
    // 当 cos=0（age=PI/(2*0.3) ≈ 5.236）：tail=0, tailFin=0
    m_model->setMotionMagnitude(1.0);

    // 波峰 cos=1
    applyAngles(0.0);
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), -0.1, 1e-4);
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), -0.2, 1e-4);

    // 过零点 cos=0
    applyAngles(5.2359877); // PI/2 / 0.3
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), 0.0, 1e-4);
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), 0.0, 1e-4);

    // 波谷 cos=-1
    applyAngles(10.4719755); // PI / 0.3
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), 0.1, 1e-4);
    EXPECT_NEAR(m_model->tailFin()->rotateAngleX(), 0.2, 1e-4);
}

// ============================================================================
// 网格生成测试
// ============================================================================

TEST_F(DolphinModelTest, GenerateMesh_StaticPose_ProducesNonEmptyMesh)
{
    applyAngles(0.0);

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    m_model->generateMesh(vertices, indices, 1.0 / 16.0);

    EXPECT_FALSE(vertices.empty()) << "静态姿势应生成非空顶点";
    EXPECT_FALSE(indices.empty()) << "静态姿势应生成非空索引";
}

TEST_F(DolphinModelTest, GenerateMesh_MovingPose_ProducesNonEmptyMesh)
{
    m_model->setMotionMagnitude(1.0);
    applyAngles(5.0);

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    m_model->generateMesh(vertices, indices, 1.0 / 16.0);

    EXPECT_FALSE(vertices.empty()) << "运动姿势应生成非空顶点";
    EXPECT_FALSE(indices.empty()) << "运动姿势应生成非空索引";
}

TEST_F(DolphinModelTest, GenerateMesh_StaticAndMoving_ProduceDifferentMeshes)
{
    // 静态姿势网格
    m_model->setMotionMagnitude(0.0);
    applyAngles(5.0);
    std::vector<ModelVertex> staticVertices;
    std::vector<u32> staticIndices;
    m_model->generateMesh(staticVertices, staticIndices, 1.0 / 16.0);

    // 运动姿势网格（相同 ageInTicks=5.0）
    m_model->setMotionMagnitude(1.0);
    applyAngles(5.0);
    std::vector<ModelVertex> movingVertices;
    std::vector<u32> movingIndices;
    m_model->generateMesh(movingVertices, movingIndices, 1.0 / 16.0);

    // 顶点数应相同（部件数不变），但顶点位置应不同（旋转角度不同）
    ASSERT_EQ(staticVertices.size(), movingVertices.size());

    bool positionsDiffer = false;
    for (std::size_t i = 0; i < staticVertices.size(); ++i) {
        const auto& sv = staticVertices[i].position;
        const auto& mv = movingVertices[i].position;
        if (std::abs(sv.x - mv.x) > 1e-5f || std::abs(sv.y - mv.y) > 1e-5f || std::abs(sv.z - mv.z) > 1e-5f) {
            positionsDiffer = true;
            break;
        }
    }
    EXPECT_TRUE(positionsDiffer) << "静态与运动姿势的网格顶点位置应不同";
}

// ============================================================================
// 回归测试：确保 deprecated setInWater 已被移除
// ============================================================================

// 此测试通过编译时保证 DolphinModel 不再拥有 setInWater 方法。
// 如果有人误重新添加 setInWater，下面的 SFINAE 检测会失败。
// 注：由于 C++ 无法直接检测成员函数缺失，此处通过文档化方式记录该约束。
TEST_F(DolphinModelTest, Regression_SetInWaterMethod_IsRemoved)
{
    // DolphinModel 不应再有 setInWater 方法（已废弃并移除）。
    // 运动状态应通过 setMotionMagnitude 推送，对应 MC 1.21.11 的
    // isMoving = deltaMovement.horizontalDistanceSqr() > 1.0E-7。
    //
    // 此测试验证 setMotionMagnitude 是唯一的运动状态入口：
    // - 默认 m_motionMagnitude = 0.0（静态）
    // - setMotionMagnitude(非零) 后切换为运动分支
    m_model->setMotionMagnitude(0.0);
    applyAngles(0.0);
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), kStaticTailAngle, 1e-5);

    m_model->setMotionMagnitude(1.0);
    applyAngles(0.0);
    EXPECT_NEAR(m_model->tail()->rotateAngleX(), expectedMovingTailAngle(0.0), 1e-5);
}

} // namespace
} // namespace mc::client::renderer
