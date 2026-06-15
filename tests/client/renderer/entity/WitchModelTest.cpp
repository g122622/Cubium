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
 * @file WitchModelTest.cpp
 * @brief 女巫模型单元测试
 *
 * 测试 WitchModel 的构造、部件层次结构、帽子部件、鼻子动画和持有物品状态。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/model/monster/WitchModel.hpp"

using namespace mc::client::renderer::entity::model;
using namespace mc::client::renderer::entity::model::monster;
using namespace mc::client::renderer::entity::model::animal;

namespace mc::client::renderer {
namespace {

/**
 * @brief 女巫模型构造测试夹具
 */
class WitchModelConstructionTest : public ::testing::Test {
protected:
    void SetUp() override { m_model = std::make_unique<WitchModel>(); }

    std::unique_ptr<WitchModel> m_model;
};

// ====== 构造测试 ======

TEST_F(WitchModelConstructionTest, InheritsFromVillagerModel)
{
    // WitchModel 应该能够转换为 VillagerModel 引用
    auto* villagerBase = static_cast<VillagerModel*>(m_model.get());
    ASSERT_NE(villagerBase, nullptr);
}

TEST_F(WitchModelConstructionTest, VillagerHatIsHidden)
{
    // 女巫构造时应隐藏村民默认帽子，因为女巫有自己的帽子
    auto hat = m_model->getHat();
    ASSERT_NE(hat, nullptr);
    EXPECT_FALSE(hat->isVisible());
}

TEST_F(WitchModelConstructionTest, HeadPartIsValid)
{
    auto head = m_model->getHead();
    ASSERT_NE(head, nullptr);
    EXPECT_TRUE(head->isVisible());
}

TEST_F(WitchModelConstructionTest, NosePartIsValid)
{
    auto nose = m_model->getNose();
    ASSERT_NE(nose, nullptr);
    EXPECT_TRUE(nose->isVisible());
}

TEST_F(WitchModelConstructionTest, BodyPartIsValid)
{
    auto body = m_model->getBody();
    ASSERT_NE(body, nullptr);
    EXPECT_TRUE(body->isVisible());
}

TEST_F(WitchModelConstructionTest, PartsCountIncludesWitchSpecificParts)
{
    // VillagerModel 基础部件: head, body, rightLeg, leftLeg, arms (5个)
    // WitchModel 额外部件: witchHat, hat2, hat3, hat4, mole (5个)
    // 总共 10 个部件
    const auto& parts = m_model->getParts();
    EXPECT_EQ(parts.size(), 10u);
}

TEST_F(WitchModelConstructionTest, WitchHatPartsAreChildrenOfHead)
{
    // 女巫帽子 (witchHat) 应该是头部的子节点
    auto head = m_model->getHead();
    ASSERT_NE(head, nullptr);

    const auto& children = head->children();
    // 头部的子节点应该包含: hat(隐藏), hatBrim(隐藏), nose, witchHat
    // 至少应该有 witchHat 和 nose
    bool foundWitchHat = false;
    for (const auto& child : children) {
        if (child->name() == "witchHat") {
            foundWitchHat = true;
            break;
        }
    }
    EXPECT_TRUE(foundWitchHat) << "witchHat should be a child of head";
}

TEST_F(WitchModelConstructionTest, HatHierarchyIsCorrect)
{
    // 帽子层次: witchHat -> hat2 -> hat3 -> hat4
    auto head = m_model->getHead();
    ASSERT_NE(head, nullptr);

    // 找到 witchHat
    std::shared_ptr<ModelRenderer> witchHat;
    for (const auto& child : head->children()) {
        if (child->name() == "witchHat") {
            witchHat = child;
            break;
        }
    }
    ASSERT_NE(witchHat, nullptr);

    // hat2 应该是 witchHat 的子节点
    const auto& hatChildren = witchHat->children();
    bool foundHat2 = false;
    std::shared_ptr<ModelRenderer> hat2;
    for (const auto& child : hatChildren) {
        if (child->name() == "hat2") {
            foundHat2 = true;
            hat2 = child;
            break;
        }
    }
    EXPECT_TRUE(foundHat2) << "hat2 should be a child of witchHat";

    if (!hat2) {
        return;
    }

    // hat3 应该是 hat2 的子节点
    bool foundHat3 = false;
    std::shared_ptr<ModelRenderer> hat3;
    for (const auto& child : hat2->children()) {
        if (child->name() == "hat3") {
            foundHat3 = true;
            hat3 = child;
            break;
        }
    }
    EXPECT_TRUE(foundHat3) << "hat3 should be a child of hat2";

    if (!hat3) {
        return;
    }

    // hat4 应该是 hat3 的子节点
    bool foundHat4 = false;
    for (const auto& child : hat3->children()) {
        if (child->name() == "hat4") {
            foundHat4 = true;
            break;
        }
    }
    EXPECT_TRUE(foundHat4) << "hat4 should be a child of hat3";
}

TEST_F(WitchModelConstructionTest, MoleIsChildOfNose)
{
    // 痣 (mole) 应该是鼻子的子节点
    auto nose = m_model->getNose();
    ASSERT_NE(nose, nullptr);

    const auto& noseChildren = nose->children();
    bool foundMole = false;
    for (const auto& child : noseChildren) {
        if (child->name() == "mole") {
            foundMole = true;
            break;
        }
    }
    EXPECT_TRUE(foundMole) << "mole should be a child of nose";
}

TEST_F(WitchModelConstructionTest, HoldingItemDefaultsToFalse)
{
    // 默认不持有物品
    EXPECT_FALSE(m_model->isHoldingItem());
}

/**
 * @brief 女巫模型动画测试夹具
 */
class WitchModelAnimationTest : public ::testing::Test {
protected:
    void SetUp() override { m_model = std::make_unique<WitchModel>(); }

    std::unique_ptr<WitchModel> m_model;
};

// ====== 鼻子动画测试 ======

TEST_F(WitchModelAnimationTest, NoseWobblesWhenNotHoldingItem)
{
    // 不持有物品时，鼻子应该根据 ageInTicks 产生微小的摆动
    constexpr f64 kModelScale = 1.0 / 16.0;

    // 设置 setAngles，验证鼻子的旋转值不是0（有摆动动画）
    m_model->setAngles(0.0, 0.0, 100.0, 0.0, 0.0, kModelScale);

    auto nose = m_model->getNose();
    ASSERT_NE(nose, nullptr);

    // 鼻子应该有 X 和 Z 轴的旋转（摆动动画）
    // 使用固定频率 0.05，ageInTicks=100 时：
    // xRot = sin(100 * 0.05) * 4.5 * (PI/180) ≈ sin(5) * 0.0785 ≈ -0.0757
    // zRot = cos(100 * 0.05) * 2.5 * (PI/180) ≈ cos(5) * 0.0436 ≈ 0.0173
    EXPECT_NE(nose->rotateAngleX(), 0.0);
    EXPECT_NE(nose->rotateAngleZ(), 0.0);
}

TEST_F(WitchModelAnimationTest, NoseWobbleAmplitudeIsSmall)
{
    // 鼻子摆动幅度应该很小（4.5度和2.5度）
    constexpr f64 kModelScale = 1.0 / 16.0;

    // 使用不同的 ageInTicks 值测试多个时间点
    for (f64 ageInTicks = 0.0; ageInTicks < 200.0; ageInTicks += 50.0) {
        m_model->setAngles(0.0, 0.0, ageInTicks, 0.0, 0.0, kModelScale);

        auto nose = m_model->getNose();
        ASSERT_NE(nose, nullptr);

        // X轴旋转幅度不应超过 4.5 * (PI/180) ≈ 0.0785
        constexpr f64 kMaxXRot = 4.5 * (3.14159265358979323846 / 180.0) * 1.01; // 1% 容差
        EXPECT_LT(std::abs(nose->rotateAngleX()), kMaxXRot) << "Nose X rotation too large at ageInTicks=" << ageInTicks;

        // Z轴旋转幅度不应超过 2.5 * (PI/180) ≈ 0.0436
        constexpr f64 kMaxZRot = 2.5 * (3.14159265358979323846 / 180.0) * 1.01; // 1% 容差
        EXPECT_LT(std::abs(nose->rotateAngleZ()), kMaxZRot) << "Nose Z rotation too large at ageInTicks=" << ageInTicks;
    }
}

TEST_F(WitchModelAnimationTest, NosePositionWhenHoldingItem)
{
    // 持有物品时鼻子的位置应该改变（上扬并前伸）
    constexpr f64 kModelScale = 1.0 / 16.0;

    m_model->setHoldingItem(true);
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, kModelScale);

    auto nose = m_model->getNose();
    ASSERT_NE(nose, nullptr);

    // 持有物品时鼻子位置: (0.0, 1.0, -1.5)
    EXPECT_DOUBLE_EQ(nose->rotationPointY(), 1.0);
    EXPECT_DOUBLE_EQ(nose->rotationPointZ(), -1.5);

    // 持有物品时鼻子上扬: rotateAngleX ≈ -0.9 (f32 精度)
    EXPECT_NEAR(nose->rotateAngleX(), -0.9, 1e-6);
}

TEST_F(WitchModelAnimationTest, NosePositionWhenNotHoldingItem)
{
    // 不持有物品时鼻子在默认位置
    constexpr f64 kModelScale = 1.0 / 16.0;

    m_model->setHoldingItem(false);
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, kModelScale);

    auto nose = m_model->getNose();
    ASSERT_NE(nose, nullptr);

    // 不持有物品时鼻子位置: (0.0, -2.0, 0.0)
    EXPECT_DOUBLE_EQ(nose->rotationPointY(), -2.0);
    EXPECT_DOUBLE_EQ(nose->rotationPointZ(), 0.0);
}

TEST_F(WitchModelAnimationTest, HoldingItemStateCanBeToggled)
{
    // 测试持有物品状态切换
    EXPECT_FALSE(m_model->isHoldingItem());

    m_model->setHoldingItem(true);
    EXPECT_TRUE(m_model->isHoldingItem());

    m_model->setHoldingItem(false);
    EXPECT_FALSE(m_model->isHoldingItem());
}

// ====== 头部旋转测试 ======

TEST_F(WitchModelAnimationTest, HeadRotationFollowsNetHeadYawAndPitch)
{
    // 女巫模型继承自 VillagerModel，头部应跟随 netHeadYaw 和 headPitch 旋转
    constexpr f64 kModelScale = 1.0 / 16.0;

    m_model->setAngles(0.0, 0.0, 0.0, 30.0, -15.0, kModelScale);

    auto head = m_model->getHead();
    ASSERT_NE(head, nullptr);

    // 头部应该有旋转值（由 VillagerModel::setAngles 设置）
    EXPECT_NE(head->rotateAngleY(), 0.0);
    EXPECT_NE(head->rotateAngleX(), 0.0);
}

TEST_F(WitchModelAnimationTest, LegAnimationFollowsLimbSwing)
{
    // 腿部应该跟随 limbSwing 摆动
    constexpr f64 kModelScale = 1.0 / 16.0;

    m_model->setAngles(1.0, 0.5, 0.0, 0.0, 0.0, kModelScale);

    const auto& parts = m_model->getParts();

    // 查找腿部部件（VillagerModel 中 rightLeg 和 leftLeg）
    std::shared_ptr<ModelRenderer> rightLeg;
    std::shared_ptr<ModelRenderer> leftLeg;
    for (const auto& part : parts) {
        if (part->name() == "rightLeg") {
            rightLeg = part;
        } else if (part->name() == "leftLeg") {
            leftLeg = part;
        }
    }

    ASSERT_NE(rightLeg, nullptr);
    ASSERT_NE(leftLeg, nullptr);

    // 当 limbSwingAmount > 0 时，腿部应该有 X 轴旋转
    EXPECT_NE(rightLeg->rotateAngleX(), 0.0);
    EXPECT_NE(leftLeg->rotateAngleX(), 0.0);
}

// ====== 渲染安全性测试 ======

TEST_F(WitchModelAnimationTest, RenderDoesNotThrow)
{
    // 确保渲染不会抛出异常
    constexpr f64 kModelScale = 1.0 / 16.0;
    EXPECT_NO_THROW(m_model->render(kModelScale));
}

TEST_F(WitchModelAnimationTest, RenderAfterSetAnglesDoesNotThrow)
{
    constexpr f64 kModelScale = 1.0 / 16.0;
    m_model->setAngles(0.5, 0.3, 100.0, 15.0, -10.0, kModelScale);
    EXPECT_NO_THROW(m_model->render(kModelScale));
}

TEST_F(WitchModelAnimationTest, RenderWhileHoldingItemDoesNotThrow)
{
    constexpr f64 kModelScale = 1.0 / 16.0;
    m_model->setHoldingItem(true);
    m_model->setAngles(0.5, 0.3, 100.0, 15.0, -10.0, kModelScale);
    EXPECT_NO_THROW(m_model->render(kModelScale));
}

// ====== 可见性测试 ======

TEST_F(WitchModelConstructionTest, SetAllVisibleFalseHidesAllParts)
{
    m_model->setAllVisible(false);

    const auto& parts = m_model->getParts();
    for (const auto& part : parts) {
        EXPECT_FALSE(part->isVisible()) << "Part '" << part->name() << "' should be hidden";
    }
}

TEST_F(WitchModelConstructionTest, SetAllVisibleTrueShowsAllParts)
{
    m_model->setAllVisible(false);
    m_model->setAllVisible(true);

    const auto& parts = m_model->getParts();
    for (const auto& part : parts) {
        EXPECT_TRUE(part->isVisible()) << "Part '" << part->name() << "' should be visible";
    }
}

// ====== 纹理尺寸测试 ======

TEST_F(WitchModelConstructionTest, TextureSizeIs64x128)
{
    // 女巫使用 64x128 的纹理尺寸（比普通村民的 64x64 更高，因为帽子占额外空间）
    EXPECT_EQ(m_model->textureWidth(), 64);
    EXPECT_EQ(m_model->textureHeight(), 128);
}

} // anonymous namespace
} // namespace mc::client::renderer
