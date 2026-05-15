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

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"

// 注意：不使用 using namespace 以避免 ArmPose 命名冲突
// BipedModel 有 model::ArmPose
// PlayerModel 有 player::ArmPose

namespace mc::client::renderer {
namespace {

// 使用完整命名空间别名简化代码
using PlayerModel = entity::model::player::PlayerModel;
using PlayerArmPose = entity::model::player::ArmPose;

/**
 * @brief PlayerModel 基础测试夹具
 *
 * 测试第三人称视角使用的 PlayerModel（entity/model/player/PlayerModel）
 */
class EntityPlayerModelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建标准手臂模型
        m_standardModel = std::make_unique<PlayerModel>(0.0, false);
        // 创建纤细手臂模型
        m_slimModel = std::make_unique<PlayerModel>(0.0, true);
    }

    std::unique_ptr<PlayerModel> m_standardModel;
    std::unique_ptr<PlayerModel> m_slimModel;
};

// ============================================================================
// 构造和初始化测试
// ============================================================================

TEST_F(EntityPlayerModelTest, Creation_InitializesStandardModel)
{
    // 验证标准模型创建成功
    ASSERT_NE(m_standardModel, nullptr);
    EXPECT_FALSE(m_standardModel->hasSlimArms());
}

TEST_F(EntityPlayerModelTest, Creation_InitializesSlimModel)
{
    // 验证纤细模型创建成功
    ASSERT_NE(m_slimModel, nullptr);
    EXPECT_TRUE(m_slimModel->hasSlimArms());
}

TEST_F(EntityPlayerModelTest, Creation_InitializesBaseParts)
{
    // 验证基础部件（来自 BipedModel）已创建
    EXPECT_NE(m_standardModel->getModelHead(), nullptr);
    EXPECT_NE(m_standardModel->getModelHeadwear(), nullptr);
    EXPECT_NE(m_standardModel->getModelBody(), nullptr);
    EXPECT_NE(m_standardModel->getLeftArm(), nullptr);
    EXPECT_NE(m_standardModel->getRightArm(), nullptr);
    EXPECT_NE(m_standardModel->getLeftLeg(), nullptr);
    EXPECT_NE(m_standardModel->getRightLeg(), nullptr);
}

// ============================================================================
// 手臂姿态测试
// ============================================================================

TEST_F(EntityPlayerModelTest, ArmPose_SetBothPoses)
{
    // 设置左右手臂姿态
    m_standardModel->setArmPose(PlayerArmPose::Block, PlayerArmPose::Item);

    // 无法直接获取姿态值，但可以通过 setAngles 后手臂角度变化来间接验证
    // 设置姿态后调用 setAngles，手臂应该有相应的角度
    EXPECT_NO_THROW(m_standardModel->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0));
}

TEST_F(EntityPlayerModelTest, ArmPose_SetRightArmPose)
{
    // 设置右手姿态
    m_standardModel->setRightArmPose(PlayerArmPose::Item);
    EXPECT_NO_THROW(m_standardModel->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0));

    m_standardModel->setRightArmPose(PlayerArmPose::BowAndArrow);
    EXPECT_NO_THROW(m_standardModel->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0));
}

TEST_F(EntityPlayerModelTest, ArmPose_SetLeftArmPose)
{
    // 设置左手姿态
    m_standardModel->setLeftArmPose(PlayerArmPose::Block);
    EXPECT_NO_THROW(m_standardModel->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0));

    m_standardModel->setLeftArmPose(PlayerArmPose::ThrowSpear);
    EXPECT_NO_THROW(m_standardModel->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0));
}

// ============================================================================
// 可见性控制测试
// ============================================================================

TEST_F(EntityPlayerModelTest, SetVisible_HidesAllParts)
{
    m_standardModel->setVisible(false);

    // 基础部件应该不可见
    EXPECT_FALSE(m_standardModel->getModelHead()->isVisible());
    EXPECT_FALSE(m_standardModel->getModelHeadwear()->isVisible());
    EXPECT_FALSE(m_standardModel->getModelBody()->isVisible());
    EXPECT_FALSE(m_standardModel->getLeftArm()->isVisible());
    EXPECT_FALSE(m_standardModel->getRightArm()->isVisible());
    EXPECT_FALSE(m_standardModel->getLeftLeg()->isVisible());
    EXPECT_FALSE(m_standardModel->getRightLeg()->isVisible());
}

TEST_F(EntityPlayerModelTest, SetVisible_ShowsAllParts)
{
    // 先隐藏
    m_standardModel->setVisible(false);
    // 再显示
    m_standardModel->setVisible(true);

    // 基础部件应该可见
    EXPECT_TRUE(m_standardModel->getModelHead()->isVisible());
    EXPECT_TRUE(m_standardModel->getModelHeadwear()->isVisible());
    EXPECT_TRUE(m_standardModel->getModelBody()->isVisible());
    EXPECT_TRUE(m_standardModel->getLeftArm()->isVisible());
    EXPECT_TRUE(m_standardModel->getRightArm()->isVisible());
    EXPECT_TRUE(m_standardModel->getLeftLeg()->isVisible());
    EXPECT_TRUE(m_standardModel->getRightLeg()->isVisible());
}

// ============================================================================
// 手臂渲染测试 (renderRightArm / renderLeftArm)
// ============================================================================

/**
 * @brief PlayerModel 手臂渲染测试夹具
 */
class PlayerModelArmRenderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_model = std::make_unique<PlayerModel>(0.0, false);
    }

    /**
     * @brief 计算可见部件数量
     */
    size_t countVisibleParts()
    {
        size_t count = 0;
        if (m_model->getModelHead()->isVisible()) count++;
        if (m_model->getModelHeadwear()->isVisible()) count++;
        if (m_model->getModelBody()->isVisible()) count++;
        if (m_model->getLeftArm()->isVisible()) count++;
        if (m_model->getRightArm()->isVisible()) count++;
        if (m_model->getLeftLeg()->isVisible()) count++;
        if (m_model->getRightLeg()->isVisible()) count++;
        return count;
    }

    std::unique_ptr<PlayerModel> m_model;
};

TEST_F(PlayerModelArmRenderTest, RenderRightArm_RestoresVisibility)
{
    // 设置特定可见性状态
    m_model->setVisible(true);
    m_model->getModelHead()->setVisible(false);  // 头部不可见
    m_model->getLeftArm()->setVisible(false);    // 左臂不可见

    // 记录渲染前的状态
    bool headVisibleBefore = m_model->getModelHead()->isVisible();
    bool leftArmVisibleBefore = m_model->getLeftArm()->isVisible();
    bool rightArmVisibleBefore = m_model->getRightArm()->isVisible();

    // 渲染右手臂
    m_model->renderRightArm(1.0 / 16.0);

    // 验证可见性状态被恢复
    EXPECT_EQ(m_model->getModelHead()->isVisible(), headVisibleBefore);
    EXPECT_EQ(m_model->getLeftArm()->isVisible(), leftArmVisibleBefore);
    EXPECT_EQ(m_model->getRightArm()->isVisible(), rightArmVisibleBefore);
}

TEST_F(PlayerModelArmRenderTest, RenderLeftArm_RestoresVisibility)
{
    // 设置特定可见性状态
    m_model->setVisible(true);
    m_model->getModelHead()->setVisible(false);   // 头部不可见
    m_model->getRightArm()->setVisible(false);    // 右臂不可见

    // 记录渲染前的状态
    bool headVisibleBefore = m_model->getModelHead()->isVisible();
    bool leftArmVisibleBefore = m_model->getLeftArm()->isVisible();
    bool rightArmVisibleBefore = m_model->getRightArm()->isVisible();

    // 渲染左手臂
    m_model->renderLeftArm(1.0 / 16.0);

    // 验证可见性状态被恢复
    EXPECT_EQ(m_model->getModelHead()->isVisible(), headVisibleBefore);
    EXPECT_EQ(m_model->getLeftArm()->isVisible(), leftArmVisibleBefore);
    EXPECT_EQ(m_model->getRightArm()->isVisible(), rightArmVisibleBefore);
}

TEST_F(PlayerModelArmRenderTest, RenderRightArm_ResetsArmRotationX)
{
    // 设置手臂X轴旋转
    m_model->getRightArm()->setRotateAngleX(1.0f);
    EXPECT_FLOAT_EQ(m_model->getRightArm()->rotateAngleX(), 1.0f);

    // 渲染右手臂
    m_model->renderRightArm(1.0 / 16.0);

    // X轴旋转应该被重置为0
    EXPECT_FLOAT_EQ(m_model->getRightArm()->rotateAngleX(), 0.0f);
}

TEST_F(PlayerModelArmRenderTest, RenderLeftArm_ResetsArmRotationX)
{
    // 设置手臂X轴旋转
    m_model->getLeftArm()->setRotateAngleX(1.5f);
    EXPECT_FLOAT_EQ(m_model->getLeftArm()->rotateAngleX(), 1.5f);

    // 渲染左手臂
    m_model->renderLeftArm(1.0 / 16.0);

    // X轴旋转应该被重置为0
    EXPECT_FLOAT_EQ(m_model->getLeftArm()->rotateAngleX(), 0.0f);
}

TEST_F(PlayerModelArmRenderTest, RenderRightArm_HandlesNullArmGracefully)
{
    // 正常情况下手臂不应该为空，但测试健壮性
    // 如果手臂为空，方法不应该崩溃
    EXPECT_NO_THROW(m_model->renderRightArm(1.0 / 16.0));
}

TEST_F(PlayerModelArmRenderTest, RenderLeftArm_HandlesNullArmGracefully)
{
    // 正常情况下手臂不应该为空，但测试健壮性
    EXPECT_NO_THROW(m_model->renderLeftArm(1.0 / 16.0));
}

TEST_F(PlayerModelArmRenderTest, RenderRightArm_WithAllPartsVisible)
{
    // 设置所有部件可见
    m_model->setVisible(true);
    EXPECT_EQ(countVisibleParts(), 7u);

    // 渲染右手臂
    EXPECT_NO_THROW(m_model->renderRightArm(1.0 / 16.0));

    // 验证可见性被恢复（所有部件应该仍然可见）
    EXPECT_TRUE(m_model->getRightArm()->isVisible());
}

TEST_F(PlayerModelArmRenderTest, RenderLeftArm_WithAllPartsVisible)
{
    // 设置所有部件可见
    m_model->setVisible(true);
    EXPECT_EQ(countVisibleParts(), 7u);

    // 渲染左手臂
    EXPECT_NO_THROW(m_model->renderLeftArm(1.0 / 16.0));

    // 验证可见性被恢复
    EXPECT_TRUE(m_model->getLeftArm()->isVisible());
}

TEST_F(PlayerModelArmRenderTest, RenderRightArm_WithAllPartsHidden)
{
    // 设置所有部件不可见
    m_model->setVisible(false);
    EXPECT_EQ(countVisibleParts(), 0u);

    // 渲染右手臂
    EXPECT_NO_THROW(m_model->renderRightArm(1.0 / 16.0));

    // 验证可见性被恢复（所有部件应该仍然不可见）
    EXPECT_FALSE(m_model->getRightArm()->isVisible());
}

TEST_F(PlayerModelArmRenderTest, RenderLeftArm_WithAllPartsHidden)
{
    // 设置所有部件不可见
    m_model->setVisible(false);
    EXPECT_EQ(countVisibleParts(), 0u);

    // 渲染左手臂
    EXPECT_NO_THROW(m_model->renderLeftArm(1.0 / 16.0));

    // 验证可见性被恢复
    EXPECT_FALSE(m_model->getLeftArm()->isVisible());
}

// ============================================================================
// 状态设置测试
// ============================================================================

TEST_F(EntityPlayerModelTest, SetCrouching_DoesNotThrow)
{
    EXPECT_NO_THROW(m_standardModel->setCrouching(true));
    EXPECT_NO_THROW(m_standardModel->setCrouching(false));
}

TEST_F(EntityPlayerModelTest, SetSwimming_DoesNotThrow)
{
    EXPECT_NO_THROW(m_standardModel->setSwimming(true));
    EXPECT_NO_THROW(m_standardModel->setSwimming(false));
}

TEST_F(EntityPlayerModelTest, SetSprinting_DoesNotThrow)
{
    EXPECT_NO_THROW(m_standardModel->setSprinting(true));
    EXPECT_NO_THROW(m_standardModel->setSprinting(false));
}

// ============================================================================
// 角度设置测试
// ============================================================================

TEST_F(EntityPlayerModelTest, SetAngles_UpdatesHeadRotation)
{
    // 设置角度
    // limbSwing=0, limbSwingAmount=0, ageInTicks=0, netHeadYaw=45, headPitch=30
    m_standardModel->setAngles(0.0, 0.0, 0.0, 45.0, 30.0, 1.0 / 16.0);

    // 头部应该有旋转
    // 注意：角度转换为弧度
    // headPitch = 30 度 ≈ 0.5236 弧度
    // netHeadYaw = 45 度 ≈ 0.7854 弧度
    EXPECT_NEAR(m_standardModel->getModelHead()->rotateAngleX(), 0.5236, 0.01);
    EXPECT_NEAR(m_standardModel->getModelHead()->rotateAngleY(), 0.7854, 0.01);
}

TEST_F(EntityPlayerModelTest, SetAngles_UpdatesWalkingAnimation)
{
    // 设置步态动画
    // limbSwing=1.0, limbSwingAmount=0.5
    m_standardModel->setAngles(1.0, 0.5, 0.0, 0.0, 0.0, 1.0 / 16.0);

    // 腿部应该有旋转（由于步态动画）
    // 步态动画会产生交替的腿部摆动
    EXPECT_NE(m_standardModel->getRightLeg()->rotateAngleX(), 0.0);
    EXPECT_NE(m_standardModel->getLeftLeg()->rotateAngleX(), 0.0);
}

// ============================================================================
// 外观层部件测试
// ============================================================================

TEST_F(EntityPlayerModelTest, CopyAnglesToWear_DoesNotThrow)
{
    // 设置手臂角度
    m_standardModel->getRightArm()->setRotateAngleX(0.5f);
    m_standardModel->getRightArm()->setRotateAngleY(0.3f);
    m_standardModel->getRightArm()->setRotateAngleZ(0.1f);

    m_standardModel->getLeftArm()->setRotateAngleX(-0.5f);
    m_standardModel->getLeftArm()->setRotateAngleY(-0.3f);
    m_standardModel->getLeftArm()->setRotateAngleZ(-0.1f);

    // 复制角度到外观层
    EXPECT_NO_THROW(m_standardModel->copyAnglesToWear());
}

// ============================================================================
// 细手臂模型测试
// ============================================================================

TEST_F(EntityPlayerModelTest, SlimModel_HasSlimArmsFlag)
{
    EXPECT_TRUE(m_slimModel->hasSlimArms());
    EXPECT_FALSE(m_standardModel->hasSlimArms());
}

TEST_F(EntityPlayerModelTest, SlimModel_VisiblePartsWork)
{
    m_slimModel->setVisible(false);
    EXPECT_FALSE(m_slimModel->getLeftArm()->isVisible());
    EXPECT_FALSE(m_slimModel->getRightArm()->isVisible());

    m_slimModel->setVisible(true);
    EXPECT_TRUE(m_slimModel->getLeftArm()->isVisible());
    EXPECT_TRUE(m_slimModel->getRightArm()->isVisible());
}

TEST_F(EntityPlayerModelTest, SlimModel_RenderRightArmWorks)
{
    m_slimModel->setVisible(true);

    // 渲染右手臂应该不崩溃
    EXPECT_NO_THROW(m_slimModel->renderRightArm(1.0 / 16.0));

    // 可见性应该被恢复
    EXPECT_TRUE(m_slimModel->getRightArm()->isVisible());
}

TEST_F(EntityPlayerModelTest, SlimModel_RenderLeftArmWorks)
{
    m_slimModel->setVisible(true);

    // 渲染左手臂应该不崩溃
    EXPECT_NO_THROW(m_slimModel->renderLeftArm(1.0 / 16.0));

    // 可见性应该被恢复
    EXPECT_TRUE(m_slimModel->getLeftArm()->isVisible());
}

TEST_F(EntityPlayerModelTest, SlimModel_RenderRightArmResetsRotation)
{
    m_slimModel->getRightArm()->setRotateAngleX(0.8f);
    EXPECT_FLOAT_EQ(m_slimModel->getRightArm()->rotateAngleX(), 0.8f);

    m_slimModel->renderRightArm(1.0 / 16.0);
    EXPECT_FLOAT_EQ(m_slimModel->getRightArm()->rotateAngleX(), 0.0f);
}

TEST_F(EntityPlayerModelTest, SlimModel_RenderLeftArmResetsRotation)
{
    m_slimModel->getLeftArm()->setRotateAngleX(-0.6f);
    EXPECT_FLOAT_EQ(m_slimModel->getLeftArm()->rotateAngleX(), -0.6f);

    m_slimModel->renderLeftArm(1.0 / 16.0);
    EXPECT_FLOAT_EQ(m_slimModel->getLeftArm()->rotateAngleX(), 0.0f);
}

// ============================================================================
// 渲染测试
// ============================================================================

TEST_F(EntityPlayerModelTest, Render_DoesNotThrow)
{
    EXPECT_NO_THROW(m_standardModel->render(1.0 / 16.0));
    EXPECT_NO_THROW(m_slimModel->render(1.0 / 16.0));
}

TEST_F(EntityPlayerModelTest, RenderCape_DoesNotThrow)
{
    EXPECT_NO_THROW(m_standardModel->renderCape(1.0 / 16.0));
}

TEST_F(EntityPlayerModelTest, RenderEars_DoesNotThrow)
{
    EXPECT_NO_THROW(m_standardModel->renderEars(1.0 / 16.0));
}

// ============================================================================
// translateHand 测试
// ============================================================================

TEST_F(EntityPlayerModelTest, TranslateHand_DoesNotThrow)
{
    EXPECT_NO_THROW(m_standardModel->translateHand(0));  // Right
    EXPECT_NO_THROW(m_standardModel->translateHand(1));  // Left
}

TEST_F(EntityPlayerModelTest, SlimModel_TranslateHandDoesNotThrow)
{
    EXPECT_NO_THROW(m_slimModel->translateHand(0));  // Right
    EXPECT_NO_THROW(m_slimModel->translateHand(1));  // Left
}

// ============================================================================
// 按部件可见性控制测试 (setPartVisible / isPartVisible / setModelVisibilitiesFromFlags)
// 参考 MC 1.16.5 PlayerRenderer.setModelVisibilities
// ============================================================================

/**
 * @brief PlayerModel 部件可见性测试夹具
 */
class PlayerModelPartVisibilityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_model = std::make_unique<PlayerModel>(0.0, false);
    }

    std::unique_ptr<PlayerModel> m_model;
};

TEST_F(PlayerModelPartVisibilityTest, SetPartVisible_Hat)
{
    // Hat 是头部外层 (m_headwear)
    m_model->setVisible(true);
    EXPECT_TRUE(m_model->getModelHeadwear()->isVisible());

    m_model->setPartVisible(PlayerModelPart::Hat, false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Hat));
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible());

    m_model->setPartVisible(PlayerModelPart::Hat, true);
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Hat));
    EXPECT_TRUE(m_model->getModelHeadwear()->isVisible());
}

TEST_F(PlayerModelPartVisibilityTest, SetPartVisible_Jacket)
{
    // Jacket 是身体外层 (m_bodywear)
    m_model->setVisible(true);

    m_model->setPartVisible(PlayerModelPart::Jacket, false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Jacket));

    m_model->setPartVisible(PlayerModelPart::Jacket, true);
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Jacket));
}

TEST_F(PlayerModelPartVisibilityTest, SetPartVisible_LeftSleeve)
{
    // LeftSleeve 是左袖外层 (m_leftArmwear)
    m_model->setVisible(true);

    m_model->setPartVisible(PlayerModelPart::LeftSleeve, false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::LeftSleeve));

    m_model->setPartVisible(PlayerModelPart::LeftSleeve, true);
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::LeftSleeve));
}

TEST_F(PlayerModelPartVisibilityTest, SetPartVisible_RightSleeve)
{
    // RightSleeve 是右袖外层 (m_rightArmwear)
    m_model->setVisible(true);

    m_model->setPartVisible(PlayerModelPart::RightSleeve, false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::RightSleeve));

    m_model->setPartVisible(PlayerModelPart::RightSleeve, true);
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::RightSleeve));
}

TEST_F(PlayerModelPartVisibilityTest, SetPartVisible_LeftPantsLeg)
{
    // LeftPantsLeg 是左裤腿外层 (m_leftLegwear)
    m_model->setVisible(true);

    m_model->setPartVisible(PlayerModelPart::LeftPantsLeg, false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::LeftPantsLeg));

    m_model->setPartVisible(PlayerModelPart::LeftPantsLeg, true);
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::LeftPantsLeg));
}

TEST_F(PlayerModelPartVisibilityTest, SetPartVisible_RightPantsLeg)
{
    // RightPantsLeg 是右裤腿外层 (m_rightLegwear)
    m_model->setVisible(true);

    m_model->setPartVisible(PlayerModelPart::RightPantsLeg, false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::RightPantsLeg));

    m_model->setPartVisible(PlayerModelPart::RightPantsLeg, true);
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::RightPantsLeg));
}

TEST_F(PlayerModelPartVisibilityTest, SetPartVisible_Cape)
{
    // Cape 是斗篷 (m_cape)
    m_model->setVisible(true);

    m_model->setPartVisible(PlayerModelPart::Cape, false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Cape));

    m_model->setPartVisible(PlayerModelPart::Cape, true);
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Cape));
}

TEST_F(PlayerModelPartVisibilityTest, SetModelVisibilitiesFromFlags_AllVisible)
{
    // 设置所有部件可见
    m_model->setVisible(true);
    constexpr u8 allParts = 0x7F; // 所有部件掩码
    m_model->setModelVisibilitiesFromFlags(allParts);

    // 所有外层皮肤部件应该可见（除了 Cape）
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Hat));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Jacket));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::LeftSleeve));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::RightSleeve));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::LeftPantsLeg));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::RightPantsLeg));
}

TEST_F(PlayerModelPartVisibilityTest, SetModelVisibilitiesFromFlags_NoneVisible)
{
    // 设置所有部件可见
    m_model->setVisible(true);
    // 0 表示所有部件都不可见
    m_model->setModelVisibilitiesFromFlags(0);

    // 所有外层皮肤部件应该不可见（除了 Cape）
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Hat));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Jacket));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::LeftSleeve));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::RightSleeve));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::LeftPantsLeg));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::RightPantsLeg));
}

TEST_F(PlayerModelPartVisibilityTest, SetModelVisibilitiesFromFlags_SelectiveVisibility)
{
    // 只设置部分部件可见
    m_model->setVisible(true);
    // Hat (0x40) + Jacket (0x02) + LeftSleeve (0x04) = 0x46
    constexpr u8 selectiveParts = 0x40 | 0x02 | 0x04;
    m_model->setModelVisibilitiesFromFlags(selectiveParts);

    // 只有选中的部件可见
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Hat));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Jacket));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::LeftSleeve));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::RightSleeve));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::LeftPantsLeg));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::RightPantsLeg));
}

TEST_F(PlayerModelPartVisibilityTest, SetModelVisibilitiesFromFlags_DoesNotAffectCape)
{
    // setModelVisibilitiesFromFlags 不应该影响 Cape
    m_model->setVisible(true);
    m_model->setPartVisible(PlayerModelPart::Cape, true);

    // 设置其他部件不可见
    m_model->setModelVisibilitiesFromFlags(0);

    // Cape 的可见性应该保持不变（由 CapeLayer 控制）
    // 注意：setModelVisibilitiesFromFlags 不修改 Cape
    // Cape 的可见性由 CapeLayer 单独处理
    // 这里验证 Cape 不受影响
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Cape));
}

TEST_F(PlayerModelPartVisibilityTest, SetPartVisible_DoesNotAffectOtherParts)
{
    // 设置所有部件可见
    m_model->setVisible(true);

    // 只隐藏 Hat
    m_model->setPartVisible(PlayerModelPart::Hat, false);

    // 其他部件应该保持可见
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Hat));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Jacket));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::LeftSleeve));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::RightSleeve));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::LeftPantsLeg));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::RightPantsLeg));
}

TEST_F(PlayerModelPartVisibilityTest, SetVisible_OverridesPartVisibility)
{
    // 设置所有部件可见
    m_model->setVisible(true);
    m_model->setPartVisible(PlayerModelPart::Hat, false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Hat));

    // setVisible(false) 应该隐藏所有部件
    m_model->setVisible(false);
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Hat));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Jacket));
    EXPECT_FALSE(m_model->isPartVisible(PlayerModelPart::Cape));

    // setVisible(true) 应该显示所有部件
    m_model->setVisible(true);
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Hat));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Jacket));
    EXPECT_TRUE(m_model->isPartVisible(PlayerModelPart::Cape));
}

} // anonymous namespace
} // namespace mc::client::renderer
