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
 * @file WitherModelSideHeadTest.cpp
 * @brief WitherModel 侧头朝向注入单元测试
 *
 * 验证 WitherModel::setSideHeadRotations 正确注入侧头朝向，
 * 并由 setAngles 应用到 m_heads[1]（左头）和 m_heads[2]（右头）。
 *
 * 覆盖场景：
 * - 默认状态（未调用 setSideHeadRotations）：侧头复制主头旋转
 * - 调用 setSideHeadRotations 后：侧头使用独立朝向（度→弧度转换）
 * - 主头始终由 netHeadYaw/headPitch 参数驱动，不受 setSideHeadRotations 影响
 * - 两侧头可以设置不同的独立朝向
 * - setSideHeadRotations 在 setAngles 之前调用（存储），setAngles 时应用
 *
 * 对应 MC 1.21.11 WitherBossModel.setupHeadRotation(state, head, index)：
 *   head.yRot = (yHeadRots[index] - bodyRot) * PI / 180
 *   head.xRot = xHeadRots[index] * PI / 180
 *
 * 数据流：
 * EntityRendererManager 读取 ClientEntity::getInterpolatedWitherSideHeadYaw/Pitch
 * → 减去 bodyYaw（yaw）得到 body 相对值
 * → WitherModel::setSideHeadRotations(yaw0, pitch0, yaw1, pitch1)（度）
 * → WitherModel::setAngles → m_heads[1/2]->setRotateAngleY/X（度→弧度）
 */

#include <gtest/gtest.h>

#include <cmath>

#include "client/renderer/trident/entity/model/monster/SpecialMonsterModels.hpp"
#include "common/util/math/MathConstants.hpp"

using namespace mc::client::renderer::entity::model;
using namespace mc::client::renderer::entity::model::monster;
using namespace mc::math;

namespace mc::client::renderer {
namespace {

/// @brief 角度转弧度
constexpr f64 degToRad(f64 deg)
{
    return deg * PI_DOUBLE / 180.0;
}

class WitherModelSideHeadTest : public ::testing::Test {
protected:
    void SetUp() override { m_model = std::make_unique<WitherModel>(); }

    /// @brief 触发一次 setAngles，scale 使用默认值
    void applyAngles()
    {
        // netHeadYaw=0, headPitch=0 让主头角度为 0，便于断言侧头独立值
        m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);
    }

    /// @brief 获取主头（head0）部件
    std::shared_ptr<ModelRenderer> getMainHead() { return m_model->getParts().at(3); }

    /// @brief 获取左头（head1）部件
    std::shared_ptr<ModelRenderer> getLeftHead() { return m_model->getParts().at(4); }

    /// @brief 获取右头（head2）部件
    std::shared_ptr<ModelRenderer> getRightHead() { return m_model->getParts().at(5); }

    std::unique_ptr<WitherModel> m_model;
};

// ============================================================================
// 部件结构验证测试
// ============================================================================

TEST_F(WitherModelSideHeadTest, Parts_ContainsThreeHeads)
{
    // getParts() 顺序：upperBody0, upperBody1, upperBody2, head0, head1, head2
    const auto& parts = m_model->getParts();
    EXPECT_GE(parts.size(), 6u);

    // 验证头部名称
    EXPECT_EQ(parts[3]->name(), "head0") << "索引 3 应为主头";
    EXPECT_EQ(parts[4]->name(), "head1") << "索引 4 应为左头";
    EXPECT_EQ(parts[5]->name(), "head2") << "索引 5 应为右头";
}

// ============================================================================
// 默认状态（未调用 setSideHeadRotations）测试
// ============================================================================

TEST_F(WitherModelSideHeadTest, DefaultState_SideHeadsCopyMainHead)
{
    // 未调用 setSideHeadRotations 时，侧头复制主头旋转
    // setAngles(0, 0, 0, netHeadYaw=30, headPitch=15, scale)
    m_model->setAngles(0.0, 0.0, 0.0, 30.0, 15.0, 1.0 / 16.0);

    auto mainHead = getMainHead();
    auto leftHead = getLeftHead();
    auto rightHead = getRightHead();

    // 主头：netHeadYaw=30 → rotateAngleY = 30 * PI/180
    EXPECT_NEAR(mainHead->rotateAngleY(), degToRad(30.0), 1e-5);
    EXPECT_NEAR(mainHead->rotateAngleX(), degToRad(15.0), 1e-5);

    // 侧头应复制主头
    EXPECT_NEAR(leftHead->rotateAngleY(), mainHead->rotateAngleY(), 1e-5) << "默认状态下左头应复制主头 yaw";
    EXPECT_NEAR(leftHead->rotateAngleX(), mainHead->rotateAngleX(), 1e-5) << "默认状态下左头应复制主头 pitch";
    EXPECT_NEAR(rightHead->rotateAngleY(), mainHead->rotateAngleY(), 1e-5) << "默认状态下右头应复制主头 yaw";
    EXPECT_NEAR(rightHead->rotateAngleX(), mainHead->rotateAngleX(), 1e-5) << "默认状态下右头应复制主头 pitch";
}

// ============================================================================
// setSideHeadRotations 注入独立朝向测试
// ============================================================================

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_LeftHeadUsesInjectedYaw)
{
    // 注入左头 yaw=45°, pitch=10°，右头 yaw=-30°, pitch=20°
    m_model->setSideHeadRotations(45.0f, 10.0f, -30.0f, 20.0f);
    applyAngles();

    auto leftHead = getLeftHead();
    EXPECT_NEAR(leftHead->rotateAngleY(), degToRad(45.0), 1e-5) << "左头 yaw 应为注入的 45° 转弧度";
}

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_LeftHeadUsesInjectedPitch)
{
    m_model->setSideHeadRotations(45.0f, 10.0f, -30.0f, 20.0f);
    applyAngles();

    auto leftHead = getLeftHead();
    EXPECT_NEAR(leftHead->rotateAngleX(), degToRad(10.0), 1e-5) << "左头 pitch 应为注入的 10° 转弧度";
}

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_RightHeadUsesInjectedYaw)
{
    m_model->setSideHeadRotations(45.0f, 10.0f, -30.0f, 20.0f);
    applyAngles();

    auto rightHead = getRightHead();
    EXPECT_NEAR(rightHead->rotateAngleY(), degToRad(-30.0), 1e-5) << "右头 yaw 应为注入的 -30° 转弧度";
}

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_RightHeadUsesInjectedPitch)
{
    m_model->setSideHeadRotations(45.0f, 10.0f, -30.0f, 20.0f);
    applyAngles();

    auto rightHead = getRightHead();
    EXPECT_NEAR(rightHead->rotateAngleX(), degToRad(20.0), 1e-5) << "右头 pitch 应为注入的 20° 转弧度";
}

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_MainHeadUnaffected)
{
    // 主头始终由 netHeadYaw/headPitch 参数驱动，不受 setSideHeadRotations 影响
    m_model->setSideHeadRotations(45.0f, 10.0f, -30.0f, 20.0f);
    // 使用 netHeadYaw=25, headPitch=-5
    m_model->setAngles(0.0, 0.0, 0.0, 25.0, -5.0, 1.0 / 16.0);

    auto mainHead = getMainHead();
    EXPECT_NEAR(mainHead->rotateAngleY(), degToRad(25.0), 1e-5)
        << "主头 yaw 应由 netHeadYaw 驱动，不受 setSideHeadRotations 影响";
    EXPECT_NEAR(mainHead->rotateAngleX(), degToRad(-5.0), 1e-5)
        << "主头 pitch 应由 headPitch 驱动，不受 setSideHeadRotations 影响";
}

// ============================================================================
// 两侧头独立朝向测试
// ============================================================================

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_TwoHeadsHaveDifferentRotations)
{
    // 两侧头设置完全不同的朝向
    m_model->setSideHeadRotations(90.0f, -30.0f, -90.0f, 30.0f);
    applyAngles();

    auto leftHead = getLeftHead();
    auto rightHead = getRightHead();

    EXPECT_NEAR(leftHead->rotateAngleY(), degToRad(90.0), 1e-5);
    EXPECT_NEAR(leftHead->rotateAngleX(), degToRad(-30.0), 1e-5);
    EXPECT_NEAR(rightHead->rotateAngleY(), degToRad(-90.0), 1e-5);
    EXPECT_NEAR(rightHead->rotateAngleX(), degToRad(30.0), 1e-5);

    // 两侧头不应相同
    EXPECT_NE(leftHead->rotateAngleY(), rightHead->rotateAngleY());
    EXPECT_NE(leftHead->rotateAngleX(), rightHead->rotateAngleX());
}

// ============================================================================
// 调用顺序测试
// ============================================================================

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_BeforeSetAngles_AppliesCorrectly)
{
    // setSideHeadRotations 在 setAngles 之前调用（存储），setAngles 时应用
    m_model->setSideHeadRotations(60.0f, 5.0f, -60.0f, -5.0f);
    applyAngles();

    auto leftHead = getLeftHead();
    auto rightHead = getRightHead();

    EXPECT_NEAR(leftHead->rotateAngleY(), degToRad(60.0), 1e-5);
    EXPECT_NEAR(rightHead->rotateAngleY(), degToRad(-60.0), 1e-5);
}

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_AfterSetAngles_RequiresReapply)
{
    // 先 setAngles（此时 m_hasSideHeadRotations=false，侧头复制主头）
    applyAngles();
    auto leftHeadBefore = getLeftHead();
    f32 leftYawBefore = static_cast<f32>(leftHeadBefore->rotateAngleY());

    // 再 setSideHeadRotations（仅存储，不立即应用）
    m_model->setSideHeadRotations(45.0f, 0.0f, -45.0f, 0.0f);

    // 不调用 setAngles 的情况下，侧头角度不变
    EXPECT_FLOAT_EQ(static_cast<f32>(leftHeadBefore->rotateAngleY()), leftYawBefore)
        << "setSideHeadRotations 仅存储，不立即应用";

    // 再次 setAngles 才会应用
    applyAngles();
    EXPECT_NEAR(leftHeadBefore->rotateAngleY(), degToRad(45.0), 1e-5) << "再次 setAngles 后侧头使用注入的朝向";
}

// ============================================================================
// 零角度测试
// ============================================================================

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_ZeroAngles)
{
    m_model->setSideHeadRotations(0.0f, 0.0f, 0.0f, 0.0f);
    applyAngles();

    auto leftHead = getLeftHead();
    auto rightHead = getRightHead();

    EXPECT_NEAR(leftHead->rotateAngleY(), 0.0, 1e-5);
    EXPECT_NEAR(leftHead->rotateAngleX(), 0.0, 1e-5);
    EXPECT_NEAR(rightHead->rotateAngleY(), 0.0, 1e-5);
    EXPECT_NEAR(rightHead->rotateAngleX(), 0.0, 1e-5);
}

// ============================================================================
// 负角度测试
// ============================================================================

TEST_F(WitherModelSideHeadTest, SetSideHeadRotations_NegativeAngles)
{
    m_model->setSideHeadRotations(-45.0f, -15.0f, -90.0f, -30.0f);
    applyAngles();

    auto leftHead = getLeftHead();
    auto rightHead = getRightHead();

    EXPECT_NEAR(leftHead->rotateAngleY(), degToRad(-45.0), 1e-5);
    EXPECT_NEAR(leftHead->rotateAngleX(), degToRad(-15.0), 1e-5);
    EXPECT_NEAR(rightHead->rotateAngleY(), degToRad(-90.0), 1e-5);
    EXPECT_NEAR(rightHead->rotateAngleX(), degToRad(-30.0), 1e-5);
}

} // namespace
} // namespace mc::client::renderer
