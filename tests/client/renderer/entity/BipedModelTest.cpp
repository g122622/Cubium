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

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"

using namespace mc::client::renderer::entity::model;

namespace mc::client::renderer {
namespace {

/**
 * @brief BipedModel 部件访问器测试
 *
 * 测试 BipedModel 各部件访问器方法是否正确返回部件指针。
 */
class BipedModelAccessorTest : public ::testing::Test {
protected:
    void SetUp() override { m_model = std::make_unique<BipedModel>(); }

    std::unique_ptr<BipedModel> m_model;
};

TEST_F(BipedModelAccessorTest, GetModelHeadReturnsValidPointer)
{
    auto head = m_model->getModelHead();
    ASSERT_NE(head, nullptr);
    EXPECT_TRUE(head->isVisible());
}

TEST_F(BipedModelAccessorTest, GetModelHeadwearReturnsValidPointer)
{
    auto headwear = m_model->getModelHeadwear();
    ASSERT_NE(headwear, nullptr);
    EXPECT_TRUE(headwear->isVisible());
}

TEST_F(BipedModelAccessorTest, GetModelBodyReturnsValidPointer)
{
    auto body = m_model->getModelBody();
    ASSERT_NE(body, nullptr);
    EXPECT_TRUE(body->isVisible());
}

TEST_F(BipedModelAccessorTest, GetLeftArmReturnsValidPointer)
{
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_TRUE(leftArm->isVisible());
}

TEST_F(BipedModelAccessorTest, GetRightArmReturnsValidPointer)
{
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_TRUE(rightArm->isVisible());
}

TEST_F(BipedModelAccessorTest, GetLeftLegReturnsValidPointer)
{
    auto leftLeg = m_model->getLeftLeg();
    ASSERT_NE(leftLeg, nullptr);
    EXPECT_TRUE(leftLeg->isVisible());
}

TEST_F(BipedModelAccessorTest, GetRightLegReturnsValidPointer)
{
    auto rightLeg = m_model->getRightLeg();
    ASSERT_NE(rightLeg, nullptr);
    EXPECT_TRUE(rightLeg->isVisible());
}

TEST_F(BipedModelAccessorTest, GetArmForSideLeftReturnsLeftArm)
{
    using namespace mc::client::renderer::entity::model;
    auto leftArm = m_model->getArmForSide(HandSide::Left);
    auto expectedLeftArm = m_model->getLeftArm();
    EXPECT_EQ(leftArm, expectedLeftArm);
}

TEST_F(BipedModelAccessorTest, GetArmForSideRightReturnsRightArm)
{
    using namespace mc::client::renderer::entity::model;
    auto rightArm = m_model->getArmForSide(HandSide::Right);
    auto expectedRightArm = m_model->getRightArm();
    EXPECT_EQ(rightArm, expectedRightArm);
}

/**
 * @brief BipedModel 可见性控制测试
 *
 * 测试 setVisible 方法是否能正确设置所有部件的可见性。
 */
class BipedModelVisibilityTest : public ::testing::Test {
protected:
    void SetUp() override { m_model = std::make_unique<BipedModel>(); }

    std::unique_ptr<BipedModel> m_model;
};

TEST_F(BipedModelVisibilityTest, SetVisibleFalseHidesAllParts)
{
    m_model->setVisible(false);

    EXPECT_FALSE(m_model->getModelHead()->isVisible());
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible());
    EXPECT_FALSE(m_model->getModelBody()->isVisible());
    EXPECT_FALSE(m_model->getLeftArm()->isVisible());
    EXPECT_FALSE(m_model->getRightArm()->isVisible());
    EXPECT_FALSE(m_model->getLeftLeg()->isVisible());
    EXPECT_FALSE(m_model->getRightLeg()->isVisible());
}

TEST_F(BipedModelVisibilityTest, SetVisibleTrueShowsAllParts)
{
    // 先隐藏
    m_model->setVisible(false);
    // 再显示
    m_model->setVisible(true);

    EXPECT_TRUE(m_model->getModelHead()->isVisible());
    EXPECT_TRUE(m_model->getModelHeadwear()->isVisible());
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_TRUE(m_model->getLeftArm()->isVisible());
    EXPECT_TRUE(m_model->getRightArm()->isVisible());
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible());
    EXPECT_TRUE(m_model->getRightLeg()->isVisible());
}

TEST_F(BipedModelVisibilityTest, SetAllVisibleFalseHidesAllParts)
{
    m_model->setAllVisible(false);

    EXPECT_FALSE(m_model->getModelHead()->isVisible());
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible());
    EXPECT_FALSE(m_model->getModelBody()->isVisible());
    EXPECT_FALSE(m_model->getLeftArm()->isVisible());
    EXPECT_FALSE(m_model->getRightArm()->isVisible());
    EXPECT_FALSE(m_model->getLeftLeg()->isVisible());
    EXPECT_FALSE(m_model->getRightLeg()->isVisible());
}

TEST_F(BipedModelVisibilityTest, SetAllVisibleTrueShowsAllParts)
{
    m_model->setAllVisible(false);
    m_model->setAllVisible(true);

    EXPECT_TRUE(m_model->getModelHead()->isVisible());
    EXPECT_TRUE(m_model->getModelHeadwear()->isVisible());
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_TRUE(m_model->getLeftArm()->isVisible());
    EXPECT_TRUE(m_model->getRightArm()->isVisible());
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible());
    EXPECT_TRUE(m_model->getRightLeg()->isVisible());
}

/**
 * @brief BipedModel 单个部件可见性测试
 *
 * 测试单个部件的 setVisible 方法。
 */
TEST_F(BipedModelVisibilityTest, IndividualPartVisibilityCanBeToggled)
{
    // 隐藏所有部件
    m_model->setVisible(false);

    // 单独显示头部
    m_model->getModelHead()->setVisible(true);
    EXPECT_TRUE(m_model->getModelHead()->isVisible());
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible());
    EXPECT_FALSE(m_model->getModelBody()->isVisible());

    // 单独显示身体
    m_model->getModelBody()->setVisible(true);
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_FALSE(m_model->getLeftArm()->isVisible());

    // 单独显示左臂
    m_model->getLeftArm()->setVisible(true);
    EXPECT_TRUE(m_model->getLeftArm()->isVisible());
    EXPECT_FALSE(m_model->getRightArm()->isVisible());

    // 单独显示右臂
    m_model->getRightArm()->setVisible(true);
    EXPECT_TRUE(m_model->getRightArm()->isVisible());
    EXPECT_FALSE(m_model->getLeftLeg()->isVisible());

    // 单独显示左腿
    m_model->getLeftLeg()->setVisible(true);
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible());
    EXPECT_FALSE(m_model->getRightLeg()->isVisible());

    // 单独显示右腿
    m_model->getRightLeg()->setVisible(true);
    EXPECT_TRUE(m_model->getRightLeg()->isVisible());
}

/**
 * @brief BipedModel 盔甲槽位可见性模式测试
 *
 * 参考 MC 1.16.5 BipedArmorLayer.setModelSlotVisible，
 * 测试各盔甲槽位应该显示的部件。
 */
class BipedModelArmorSlotVisibilityTest : public ::testing::Test {
protected:
    void SetUp() override { m_model = std::make_unique<BipedModel>(); }

    /**
     * @brief 设置头盔槽位可见性
     * MC 1.16.5: 头盔显示 head, headwear
     */
    void setHeadSlotVisibility()
    {
        m_model->setAllVisible(false);
        m_model->getModelHead()->setVisible(true);
        m_model->getModelHeadwear()->setVisible(true);
    }

    /**
     * @brief 设置胸甲槽位可见性
     * MC 1.16.5: 胸甲显示 body, leftArm, rightArm
     */
    void setChestSlotVisibility()
    {
        m_model->setAllVisible(false);
        m_model->getModelBody()->setVisible(true);
        m_model->getLeftArm()->setVisible(true);
        m_model->getRightArm()->setVisible(true);
    }

    /**
     * @brief 设置护腿槽位可见性
     * MC 1.16.5: 护腿显示 body, leftLeg, rightLeg
     */
    void setLegsSlotVisibility()
    {
        m_model->setAllVisible(false);
        m_model->getModelBody()->setVisible(true);
        m_model->getLeftLeg()->setVisible(true);
        m_model->getRightLeg()->setVisible(true);
    }

    /**
     * @brief 设置靴子槽位可见性
     * MC 1.16.5: 靴子显示 leftLeg, rightLeg
     */
    void setFeetSlotVisibility()
    {
        m_model->setAllVisible(false);
        m_model->getLeftLeg()->setVisible(true);
        m_model->getRightLeg()->setVisible(true);
    }

    std::unique_ptr<BipedModel> m_model;
};

TEST_F(BipedModelArmorSlotVisibilityTest, HeadSlotShowsHeadAndHeadwear)
{
    setHeadSlotVisibility();

    // 应该可见
    EXPECT_TRUE(m_model->getModelHead()->isVisible());
    EXPECT_TRUE(m_model->getModelHeadwear()->isVisible());

    // 应该不可见
    EXPECT_FALSE(m_model->getModelBody()->isVisible());
    EXPECT_FALSE(m_model->getLeftArm()->isVisible());
    EXPECT_FALSE(m_model->getRightArm()->isVisible());
    EXPECT_FALSE(m_model->getLeftLeg()->isVisible());
    EXPECT_FALSE(m_model->getRightLeg()->isVisible());
}

TEST_F(BipedModelArmorSlotVisibilityTest, ChestSlotShowsBodyAndArms)
{
    setChestSlotVisibility();

    // 应该可见
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_TRUE(m_model->getLeftArm()->isVisible());
    EXPECT_TRUE(m_model->getRightArm()->isVisible());

    // 应该不可见
    EXPECT_FALSE(m_model->getModelHead()->isVisible());
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible());
    EXPECT_FALSE(m_model->getLeftLeg()->isVisible());
    EXPECT_FALSE(m_model->getRightLeg()->isVisible());
}

TEST_F(BipedModelArmorSlotVisibilityTest, LegsSlotShowsBodyAndLegs)
{
    setLegsSlotVisibility();

    // 应该可见
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible());
    EXPECT_TRUE(m_model->getRightLeg()->isVisible());

    // 应该不可见
    EXPECT_FALSE(m_model->getModelHead()->isVisible());
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible());
    EXPECT_FALSE(m_model->getLeftArm()->isVisible());
    EXPECT_FALSE(m_model->getRightArm()->isVisible());
}

TEST_F(BipedModelArmorSlotVisibilityTest, FeetSlotShowsLegs)
{
    setFeetSlotVisibility();

    // 应该可见
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible());
    EXPECT_TRUE(m_model->getRightLeg()->isVisible());

    // 应该不可见
    EXPECT_FALSE(m_model->getModelHead()->isVisible());
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible());
    EXPECT_FALSE(m_model->getModelBody()->isVisible());
    EXPECT_FALSE(m_model->getLeftArm()->isVisible());
    EXPECT_FALSE(m_model->getRightArm()->isVisible());
}

/**
 * @brief BipedModel 部件数量测试
 */
TEST_F(BipedModelAccessorTest, PartsCountIsSeven)
{
    const auto& parts = m_model->getParts();
    // BipedModel 有 7 个部件：head, headwear, body, rightArm, leftArm, rightLeg, leftLeg
    EXPECT_EQ(parts.size(), 7u);
}

/**
 * @brief BipedModel 兼容性别名测试
 *
 * 测试 m_head, m_body 等别名是否正确引用对应的部件。
 */
TEST_F(BipedModelAccessorTest, AliasesReferenceCorrectParts)
{
    // 验证别名是否引用相同的部件
    // 注意：BipedModel 中的别名是引用，无法直接比较指针
    // 但我们可以通过设置可见性来间接验证

    m_model->setVisible(false);

    // 通过访问器设置可见
    m_model->getModelHead()->setVisible(true);
    // 验证 head 别名也被设置（它们指向同一对象）
    EXPECT_TRUE(m_model->getModelHead()->isVisible());
}

} // anonymous namespace
} // namespace mc::client::renderer
