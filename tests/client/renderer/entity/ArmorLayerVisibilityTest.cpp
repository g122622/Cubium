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
 * @brief 盔甲槽位枚举（与 ArmorLayer 中定义一致）
 */
enum class ArmorSlot : u8 {
    Head = 0,  // 头盔
    Chest = 1, // 胸甲
    Legs = 2,  // 护腿
    Feet = 3   // 靴子
};

/**
 * @brief 模拟 ArmorLayer::setModelSlotVisible 逻辑的测试
 *
 * 由于 ArmorLayer 是模板类且依赖 Vulkan，这里单独测试可见性设置逻辑。
 * 该测试验证 MC 1.16.5 BipedArmorLayer.setModelSlotVisible 的正确实现。
 */
class ArmorLayerVisibilityTest : public ::testing::Test {
protected:
    void SetUp() override { m_model = std::make_unique<BipedModel>(); }

    /**
     * @brief 模拟 ArmorLayer::setModelSlotVisible 实现
     *
     * 参考 MC 1.16.5 BipedArmorLayer.setModelSlotVisible (第67-89行)
     */
    void setModelSlotVisible(BipedModel& model, ArmorSlot slot)
    {
        model.setAllVisible(false);

        switch (slot) {
            case ArmorSlot::Head:
                // 头盔：显示头部和帽子层
                if (auto head = model.getModelHead()) {
                    head->setVisible(true);
                }
                if (auto headwear = model.getModelHeadwear()) {
                    headwear->setVisible(true);
                }
                break;
            case ArmorSlot::Chest:
                // 胸甲：显示身体、左臂、右臂
                if (auto body = model.getModelBody()) {
                    body->setVisible(true);
                }
                if (auto leftArm = model.getLeftArm()) {
                    leftArm->setVisible(true);
                }
                if (auto rightArm = model.getRightArm()) {
                    rightArm->setVisible(true);
                }
                break;
            case ArmorSlot::Legs:
                // 护腿：显示身体、左腿、右腿
                if (auto body = model.getModelBody()) {
                    body->setVisible(true);
                }
                if (auto leftLeg = model.getLeftLeg()) {
                    leftLeg->setVisible(true);
                }
                if (auto rightLeg = model.getRightLeg()) {
                    rightLeg->setVisible(true);
                }
                break;
            case ArmorSlot::Feet:
                // 靴子：显示左腿、右腿
                if (auto leftLeg = model.getLeftLeg()) {
                    leftLeg->setVisible(true);
                }
                if (auto rightLeg = model.getRightLeg()) {
                    rightLeg->setVisible(true);
                }
                break;
        }
    }

    std::unique_ptr<BipedModel> m_model;
};

TEST_F(ArmorLayerVisibilityTest, HeadSlotSetsCorrectVisibility)
{
    setModelSlotVisible(*m_model, ArmorSlot::Head);

    // MC 1.16.5: 头盔显示 bipedHead 和 bipedHeadwear
    EXPECT_TRUE(m_model->getModelHead()->isVisible()) << "Head should be visible for Head slot";
    EXPECT_TRUE(m_model->getModelHeadwear()->isVisible()) << "Headwear should be visible for Head slot";

    // 其他部件应该不可见
    EXPECT_FALSE(m_model->getModelBody()->isVisible()) << "Body should NOT be visible for Head slot";
    EXPECT_FALSE(m_model->getLeftArm()->isVisible()) << "Left arm should NOT be visible for Head slot";
    EXPECT_FALSE(m_model->getRightArm()->isVisible()) << "Right arm should NOT be visible for Head slot";
    EXPECT_FALSE(m_model->getLeftLeg()->isVisible()) << "Left leg should NOT be visible for Head slot";
    EXPECT_FALSE(m_model->getRightLeg()->isVisible()) << "Right leg should NOT be visible for Head slot";
}

TEST_F(ArmorLayerVisibilityTest, ChestSlotSetsCorrectVisibility)
{
    setModelSlotVisible(*m_model, ArmorSlot::Chest);

    // MC 1.16.5: 胸甲显示 bipedBody, bipedRightArm, bipedLeftArm
    EXPECT_TRUE(m_model->getModelBody()->isVisible()) << "Body should be visible for Chest slot";
    EXPECT_TRUE(m_model->getLeftArm()->isVisible()) << "Left arm should be visible for Chest slot";
    EXPECT_TRUE(m_model->getRightArm()->isVisible()) << "Right arm should be visible for Chest slot";

    // 其他部件应该不可见
    EXPECT_FALSE(m_model->getModelHead()->isVisible()) << "Head should NOT be visible for Chest slot";
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible()) << "Headwear should NOT be visible for Chest slot";
    EXPECT_FALSE(m_model->getLeftLeg()->isVisible()) << "Left leg should NOT be visible for Chest slot";
    EXPECT_FALSE(m_model->getRightLeg()->isVisible()) << "Right leg should NOT be visible for Chest slot";
}

TEST_F(ArmorLayerVisibilityTest, LegsSlotSetsCorrectVisibility)
{
    setModelSlotVisible(*m_model, ArmorSlot::Legs);

    // MC 1.16.5: 护腿显示 bipedBody, bipedRightLeg, bipedLeftLeg
    EXPECT_TRUE(m_model->getModelBody()->isVisible()) << "Body should be visible for Legs slot";
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible()) << "Left leg should be visible for Legs slot";
    EXPECT_TRUE(m_model->getRightLeg()->isVisible()) << "Right leg should be visible for Legs slot";

    // 其他部件应该不可见
    EXPECT_FALSE(m_model->getModelHead()->isVisible()) << "Head should NOT be visible for Legs slot";
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible()) << "Headwear should NOT be visible for Legs slot";
    EXPECT_FALSE(m_model->getLeftArm()->isVisible()) << "Left arm should NOT be visible for Legs slot";
    EXPECT_FALSE(m_model->getRightArm()->isVisible()) << "Right arm should NOT be visible for Legs slot";
}

TEST_F(ArmorLayerVisibilityTest, FeetSlotSetsCorrectVisibility)
{
    setModelSlotVisible(*m_model, ArmorSlot::Feet);

    // MC 1.16.5: 靴子显示 bipedRightLeg, bipedLeftLeg
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible()) << "Left leg should be visible for Feet slot";
    EXPECT_TRUE(m_model->getRightLeg()->isVisible()) << "Right leg should be visible for Feet slot";

    // 其他部件应该不可见
    EXPECT_FALSE(m_model->getModelHead()->isVisible()) << "Head should NOT be visible for Feet slot";
    EXPECT_FALSE(m_model->getModelHeadwear()->isVisible()) << "Headwear should NOT be visible for Feet slot";
    EXPECT_FALSE(m_model->getModelBody()->isVisible()) << "Body should NOT be visible for Feet slot";
    EXPECT_FALSE(m_model->getLeftArm()->isVisible()) << "Left arm should NOT be visible for Feet slot";
    EXPECT_FALSE(m_model->getRightArm()->isVisible()) << "Right arm should NOT be visible for Feet slot";
}

/**
 * @brief 测试多次切换槽位时的可见性状态
 */
TEST_F(ArmorLayerVisibilityTest, MultipleSlotTransitions)
{
    // 先设置头部
    setModelSlotVisible(*m_model, ArmorSlot::Head);
    EXPECT_TRUE(m_model->getModelHead()->isVisible());
    EXPECT_FALSE(m_model->getModelBody()->isVisible());

    // 切换到胸甲
    setModelSlotVisible(*m_model, ArmorSlot::Chest);
    EXPECT_FALSE(m_model->getModelHead()->isVisible());
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_TRUE(m_model->getLeftArm()->isVisible());
    EXPECT_TRUE(m_model->getRightArm()->isVisible());

    // 切换到护腿
    setModelSlotVisible(*m_model, ArmorSlot::Legs);
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_FALSE(m_model->getLeftArm()->isVisible());
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible());

    // 切换到靴子
    setModelSlotVisible(*m_model, ArmorSlot::Feet);
    EXPECT_FALSE(m_model->getModelBody()->isVisible());
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible());
    EXPECT_TRUE(m_model->getRightLeg()->isVisible());
}

/**
 * @brief 测试所有部件初始可见
 */
TEST_F(ArmorLayerVisibilityTest, AllPartsInitiallyVisible)
{
    // 默认构造后所有部件应该可见
    EXPECT_TRUE(m_model->getModelHead()->isVisible());
    EXPECT_TRUE(m_model->getModelHeadwear()->isVisible());
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_TRUE(m_model->getLeftArm()->isVisible());
    EXPECT_TRUE(m_model->getRightArm()->isVisible());
    EXPECT_TRUE(m_model->getLeftLeg()->isVisible());
    EXPECT_TRUE(m_model->getRightLeg()->isVisible());
}

/**
 * @brief 测试 setAllVisible 后设置单个槽位
 */
TEST_F(ArmorLayerVisibilityTest, SetAllVisibleThenSlotVisibility)
{
    // 先隐藏所有
    m_model->setAllVisible(false);

    // 验证所有隐藏
    EXPECT_FALSE(m_model->getModelHead()->isVisible());
    EXPECT_FALSE(m_model->getModelBody()->isVisible());

    // 设置胸甲槽位
    setModelSlotVisible(*m_model, ArmorSlot::Chest);

    // 验证只有胸甲部件可见
    EXPECT_FALSE(m_model->getModelHead()->isVisible());
    EXPECT_TRUE(m_model->getModelBody()->isVisible());
    EXPECT_TRUE(m_model->getLeftArm()->isVisible());
    EXPECT_TRUE(m_model->getRightArm()->isVisible());
    EXPECT_FALSE(m_model->getLeftLeg()->isVisible());
    EXPECT_FALSE(m_model->getRightLeg()->isVisible());
}

/**
 * @brief 测试边界情况：空指针检查
 *
 * BipedModel 的所有部件访问器应该永远返回有效指针。
 */
TEST_F(ArmorLayerVisibilityTest, AllAccessorsReturnValidPointers)
{
    // 多次创建模型，确保每次都能正确初始化所有部件
    for (int i = 0; i < 10; ++i) {
        auto model = std::make_unique<BipedModel>();

        ASSERT_NE(model->getModelHead(), nullptr) << "getModelHead() should not return nullptr";
        ASSERT_NE(model->getModelHeadwear(), nullptr) << "getModelHeadwear() should not return nullptr";
        ASSERT_NE(model->getModelBody(), nullptr) << "getModelBody() should not return nullptr";
        ASSERT_NE(model->getLeftArm(), nullptr) << "getLeftArm() should not return nullptr";
        ASSERT_NE(model->getRightArm(), nullptr) << "getRightArm() should not return nullptr";
        ASSERT_NE(model->getLeftLeg(), nullptr) << "getLeftLeg() should not return nullptr";
        ASSERT_NE(model->getRightLeg(), nullptr) << "getRightLeg() should not return nullptr";
    }
}

} // anonymous namespace
} // namespace mc::client::renderer
