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
 * @file Viewport3DWidgetTest.cpp
 * @brief Viewport3DWidget单元测试
 */

#include "client/ui/kagero/widget/Viewport3DWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::Colors;
using namespace mc;

// ==================== 构造函数测试 ====================

TEST(Viewport3DWidgetTest, DefaultConstructor)
{
    Viewport3DWidget viewport;
    EXPECT_TRUE(viewport.id().empty());
    EXPECT_EQ(Viewport3DWidget::RenderMode::Entity, viewport.renderMode());
    EXPECT_FLOAT_EQ(30.0f, viewport.cameraDistance());
    EXPECT_FLOAT_EQ(0.0f, viewport.pitch());
    EXPECT_FLOAT_EQ(180.0f, viewport.yaw());
}

TEST(Viewport3DWidgetTest, ConstructorWithBounds)
{
    Viewport3DWidget viewport("viewport_3d", 10, 20, 400, 300);

    EXPECT_EQ("viewport_3d", viewport.id());
    EXPECT_EQ(10, viewport.x());
    EXPECT_EQ(20, viewport.y());
    EXPECT_EQ(400, viewport.width());
    EXPECT_EQ(300, viewport.height());
}

// ==================== 相机操作测试 ====================

TEST(Viewport3DWidgetTest, SetCameraDistance)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    viewport.setCameraDistance(50.0f);
    EXPECT_FLOAT_EQ(50.0f, viewport.cameraDistance());

    // 超出范围约束
    viewport.setCameraDistance(1.0f);                 // 小于最小值
    EXPECT_FLOAT_EQ(5.0f, viewport.cameraDistance()); // 最小值

    viewport.setCameraDistance(200.0f);                 // 大于最大值
    EXPECT_FLOAT_EQ(100.0f, viewport.cameraDistance()); // 最大值
}

TEST(Viewport3DWidgetTest, SetRotation)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    viewport.setRotation(30.0f, 90.0f);
    EXPECT_FLOAT_EQ(30.0f, viewport.pitch());
    EXPECT_FLOAT_EQ(90.0f, viewport.yaw());

    // 俯仰角限制
    viewport.setRotation(-100.0f, 0.0f);
    EXPECT_FLOAT_EQ(-90.0f, viewport.pitch()); // 限制在-90

    viewport.setRotation(100.0f, 0.0f);
    EXPECT_FLOAT_EQ(90.0f, viewport.pitch()); // 限制在90
}

TEST(Viewport3DWidgetTest, Rotate)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setRotation(0.0f, 0.0f);

    viewport.rotate(10.0f, 20.0f);
    EXPECT_FLOAT_EQ(10.0f, viewport.pitch());
    EXPECT_FLOAT_EQ(20.0f, viewport.yaw());

    viewport.rotate(-5.0f, 30.0f);
    EXPECT_FLOAT_EQ(5.0f, viewport.pitch());
    EXPECT_FLOAT_EQ(50.0f, viewport.yaw());
}

TEST(Viewport3DWidgetTest, ResetRotation)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setRotation(45.0f, 90.0f);

    viewport.resetRotation();
    EXPECT_FLOAT_EQ(0.0f, viewport.pitch());
    EXPECT_FLOAT_EQ(180.0f, viewport.yaw()); // 默认面向正面
}

// ==================== 显示属性测试 ====================

TEST(Viewport3DWidgetTest, SetRenderMode)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    viewport.setRenderMode(Viewport3DWidget::RenderMode::Item);
    EXPECT_EQ(Viewport3DWidget::RenderMode::Item, viewport.renderMode());

    viewport.setRenderMode(Viewport3DWidget::RenderMode::Block);
    EXPECT_EQ(Viewport3DWidget::RenderMode::Block, viewport.renderMode());
}

TEST(Viewport3DWidgetTest, SetBackgroundColor)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    viewport.setBackgroundColor(fromARGB(255, 100, 149, 237));
    EXPECT_EQ(fromARGB(255, 100, 149, 237), viewport.backgroundColor());
}

TEST(Viewport3DWidgetTest, SetShowBackground)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    EXPECT_FALSE(viewport.showBackground()); // 默认透明背景

    viewport.setShowBackground(true);
    EXPECT_TRUE(viewport.showBackground());
}

TEST(Viewport3DWidgetTest, SetAutoRotate)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    EXPECT_FALSE(viewport.autoRotate());

    viewport.setAutoRotate(true);
    EXPECT_TRUE(viewport.autoRotate());

    viewport.setAutoRotateSpeed(2.0f);
    EXPECT_FLOAT_EQ(2.0f, viewport.autoRotateSpeed());
}

TEST(Viewport3DWidgetTest, SetSensitivity)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    viewport.setRotationSensitivity(3.0f);
    viewport.setZoomSensitivity(4.0f);
    // 这些是内部状态，通过行为验证
}

TEST(Viewport3DWidgetTest, SetZoomRange)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    viewport.setZoomRange(10.0f, 200.0f);
    viewport.setCameraDistance(100.0f);
    EXPECT_FLOAT_EQ(100.0f, viewport.cameraDistance());

    // 设置后超出范围的值应被约束
    viewport.setCameraDistance(5.0f);
    EXPECT_FLOAT_EQ(10.0f, viewport.cameraDistance());

    viewport.setCameraDistance(300.0f);
    EXPECT_FLOAT_EQ(200.0f, viewport.cameraDistance());
}

// ==================== 状态测试 ====================

TEST(Viewport3DWidgetTest, SetVisible)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    EXPECT_TRUE(viewport.isVisible());

    viewport.setVisible(false);
    EXPECT_FALSE(viewport.isVisible());
}

TEST(Viewport3DWidgetTest, SetActive)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);

    EXPECT_TRUE(viewport.isActive());

    viewport.setActive(false);
    EXPECT_FALSE(viewport.isActive());
}

// ==================== 拖拽开始/结束事件测试 ====================

TEST(Viewport3DWidgetTest, OnDragStartReturnsTrueAfterClick)
{
    // onClick 后 onDragStart 应返回 true（m_dragging 已设置）
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setActive(true);
    viewport.setVisible(true);

    // 点击前 onDragStart 返回 false
    EXPECT_FALSE(viewport.onDragStart(50, 50, 0, 0));

    viewport.onClick(50, 50, 0, 0);
    // 点击后 onDragStart 返回 true
    EXPECT_TRUE(viewport.onDragStart(50, 50, 0, 0));
}

TEST(Viewport3DWidgetTest, OnDragStartRejectsNonLeftButton)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setActive(true);
    viewport.setVisible(true);
    viewport.onClick(50, 50, 0, 0);

    // 右键/中键不应触发拖拽
    EXPECT_FALSE(viewport.onDragStart(50, 50, 1, 0));
    EXPECT_FALSE(viewport.onDragStart(50, 50, 2, 0));
}

TEST(Viewport3DWidgetTest, OnDragEndReturnsTrueWhenDragging)
{
    // 拖拽中 onDragEnd 返回 true，但不清理 m_dragging（由 onRelease 统一清理）
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setActive(true);
    viewport.setVisible(true);
    viewport.onClick(50, 50, 0, 0);

    EXPECT_TRUE(viewport.onDragEnd(60, 60, 0, false));
    // onDragEnd 不清理 m_dragging，onRelease 仍能正确清理
    EXPECT_TRUE(viewport.onRelease(60, 60, 0, 0));
}

TEST(Viewport3DWidgetTest, OnDragEndRejectsNonLeftButton)
{
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setActive(true);
    viewport.setVisible(true);
    viewport.onClick(50, 50, 0, 0);

    EXPECT_FALSE(viewport.onDragEnd(60, 60, 1, false));
    EXPECT_FALSE(viewport.onDragEnd(60, 60, 2, false));
}

TEST(Viewport3DWidgetTest, OnDragEndReturnsFalseWhenNotDragging)
{
    // 未点击直接调用 onDragEnd 应返回 false
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setActive(true);
    viewport.setVisible(true);

    EXPECT_FALSE(viewport.onDragEnd(60, 60, 0, false));
}

TEST(Viewport3DWidgetTest, OnDragEndPassesDroppedFlag)
{
    // onDragEnd 的 dropped 标志不影响状态清理（onDragEnd 不清理状态）
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setActive(true);
    viewport.setVisible(true);
    viewport.onClick(50, 50, 0, 0);

    // dropped=true 也不应清理状态，返回 true
    EXPECT_TRUE(viewport.onDragEnd(60, 60, 0, true));
    // onRelease 仍能正常清理
    EXPECT_TRUE(viewport.onRelease(60, 60, 0, 0));
}

TEST(Viewport3DWidgetTest, DragRotatesViewport)
{
    // 验证 onDrag 在拖拽中旋转视角
    Viewport3DWidget viewport("test", 0, 0, 100, 100);
    viewport.setActive(true);
    viewport.setVisible(true);

    f32 initialYaw = viewport.yaw();
    f32 initialPitch = viewport.pitch();

    viewport.onClick(50, 50, 0, 0);
    viewport.onDrag(60, 50, 10, 0, 0);

    // deltaX=10 应导致 yaw 变化
    EXPECT_NE(initialYaw, viewport.yaw());
    // deltaY=0 应保持 pitch 不变
    EXPECT_FLOAT_EQ(initialPitch, viewport.pitch());
}
