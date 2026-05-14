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
 * @file MockWidget.hpp
 * @brief 用于布局系统测试的 Mock Widget 类
 *
 * 提供可配置的 Widget 实现，用于测试布局算法。
 */

#pragma once

#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/widget/IWidgetContainer.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include <memory>
#include <vector>

namespace mc::client::ui::kagero::test {

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::layout;
using namespace mc::client::ui::kagero::widget;

/**
 * @brief Mock Widget 类
 *
 * 简单的 Widget 实现，用于布局测试。
 * 可以设置固定尺寸、测量回调等。
 */
class MockWidget : public Widget {
public:
    using Ptr = std::unique_ptr<MockWidget>;

    MockWidget() = default;
    explicit MockWidget(const std::string& id)
        : Widget(id)
    {}

    // 设置固定尺寸
    void setFixedSize(i32 width, i32 height)
    {
        m_fixedWidth = width;
        m_fixedHeight = height;
        setSize(width, height);
    }

    // 设置测量回调
    using MeasureCallback = std::function<Size(const MeasureSpec&, const MeasureSpec&)>;
    void setMeasureCallback(MeasureCallback callback) { m_measureCallback = std::move(callback); }

    // 设置期望尺寸（用于自适应测量）
    void setDesiredSize(i32 width, i32 height)
    {
        m_desiredWidth = width;
        m_desiredHeight = height;
    }

    // 获取渲染调用次数
    i32 renderCount() const { return m_renderCount; }

    // 获取最后一次渲染位置
    i32 lastRenderX() const { return m_lastRenderX; }
    i32 lastRenderY() const { return m_lastRenderY; }

    // Widget 接口实现
    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override
    {
        (void)ctx;
        (void)mouseX;
        (void)mouseY;
        (void)partialTick;
        m_renderCount++;
        m_lastRenderX = m_bounds.x;
        m_lastRenderY = m_bounds.y;
    }

    // 测量方法（用于布局测试）
    Size measureForTest(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec)
    {
        if (m_measureCallback) {
            return m_measureCallback(widthSpec, heightSpec);
        }
        if (m_fixedWidth >= 0 && m_fixedHeight >= 0) {
            return Size(m_fixedWidth, m_fixedHeight);
        }
        return Size(widthSpec.resolve(m_desiredWidth >= 0 ? m_desiredWidth : m_bounds.width),
            heightSpec.resolve(m_desiredHeight >= 0 ? m_desiredHeight : m_bounds.height));
    }

private:
    i32 m_fixedWidth = -1;
    i32 m_fixedHeight = -1;
    i32 m_desiredWidth = -1;
    i32 m_desiredHeight = -1;
    MeasureCallback m_measureCallback;
    i32 m_renderCount = 0;
    i32 m_lastRenderX = 0;
    i32 m_lastRenderY = 0;
};

/**
 * @brief Mock 容器 Widget 类
 *
 * 可以包含子元素的容器，用于测试布局算法。
 */
class MockContainerWidget : public Widget, public WidgetContainerMixin<MockContainerWidget> {
public:
    using Ptr = std::unique_ptr<MockContainerWidget>;

    MockContainerWidget() = default;
    explicit MockContainerWidget(const std::string& id)
        : Widget(id)
    {}

    // 使用 WidgetContainerMixin 的方法
    using WidgetContainerMixin<MockContainerWidget>::addWidget;
    using WidgetContainerMixin<MockContainerWidget>::widgets;
    using WidgetContainerMixin<MockContainerWidget>::widgetCount;
    using WidgetContainerMixin<MockContainerWidget>::findWidgetById;
    using WidgetContainerMixin<MockContainerWidget>::getWidgetAt;

    void render(RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override
    {
        renderChildren(ctx, mouseX, mouseY, partialTick);
    }

    // 添加 MockWidget 并返回指针
    MockWidget* addMockWidget(const std::string& id, i32 width, i32 height)
    {
        auto widget = std::make_unique<MockWidget>(id);
        widget->setFixedSize(width, height);
        auto* ptr = widget.get();
        addWidget(std::move(widget));
        return ptr;
    }
};

/**
 * @brief Mock Widget Layout Adaptor
 *
 * 用于布局测试的 WidgetLayoutAdaptor 包装器。
 */
class MockLayoutAdaptor : public WidgetLayoutAdaptor {
public:
    explicit MockLayoutAdaptor(Widget* widget)
        : WidgetLayoutAdaptor(widget)
        , m_widgetPtr(widget)
    {}

    /**
     * @brief 创建一个简单的 Mock 适配器
     *
     * @param id Widget ID
     * @param width 固定宽度
     * @param height 固定高度
     * @return MockLayoutAdaptor 实例
     */
    static std::pair<std::unique_ptr<MockWidget>, std::unique_ptr<MockLayoutAdaptor>> create(
        const std::string& id, i32 width, i32 height)
    {
        auto widget = std::make_unique<MockWidget>(id);
        widget->setFixedSize(width, height);
        auto* widgetPtr = widget.get();
        auto adaptor = std::make_unique<MockLayoutAdaptor>(widgetPtr);
        return {std::move(widget), std::move(adaptor)};
    }

    Widget* widgetPtr() const { return m_widgetPtr; }

private:
    Widget* m_widgetPtr;
};

/**
 * @brief 测试辅助函数：创建 MockWidget 和其适配器
 */
inline std::unique_ptr<WidgetLayoutAdaptor> createMockAdaptor(
    const std::string& id, i32 width, i32 height, LayoutConstraints constraints = LayoutConstraints{})
{
    auto widget = new MockWidget(id);
    widget->setFixedSize(width, height);
    auto adaptor = std::make_unique<WidgetLayoutAdaptor>(widget);
    adaptor->setConstraints(constraints);
    // 注意：这里 widget 的生命周期由调用者管理
    // 在测试中，通常将两者一起保存在 vector 中
    return adaptor;
}

/**
 * @brief 测试辅助类：管理一组 MockWidget 和它们的适配器
 */
class MockLayoutTestHelper {
public:
    /**
     * @brief 添加一个 MockWidget
     *
     * @param id Widget ID
     * @param width 固定宽度
     * @param height 固定高度
     * @return WidgetLayoutAdaptor 指针
     */
    WidgetLayoutAdaptor* addWidget(const std::string& id, i32 width, i32 height)
    {
        auto widget = std::make_unique<MockWidget>(id);
        widget->setFixedSize(width, height);
        auto* widgetPtr = widget.get();
        m_widgets.push_back(std::move(widget));

        auto adaptor = std::make_unique<WidgetLayoutAdaptor>(widgetPtr);
        auto* adaptorPtr = adaptor.get();
        m_adaptors.push_back(std::move(adaptor));

        return adaptorPtr;
    }

    /**
     * @brief 添加一个带约束的 MockWidget
     */
    WidgetLayoutAdaptor* addWidget(const std::string& id, i32 width, i32 height, const LayoutConstraints& constraints)
    {
        auto* adaptor = addWidget(id, width, height);
        adaptor->setConstraints(constraints);
        return adaptor;
    }

    /**
     * @brief 添加一个容器 Widget
     */
    MockContainerWidget* addContainer(const std::string& id)
    {
        auto container = std::make_unique<MockContainerWidget>(id);
        auto* ptr = container.get();
        m_containers.push_back(std::move(container));
        return ptr;
    }

    /**
     * @brief 获取所有适配器指针
     */
    std::vector<WidgetLayoutAdaptor*> getAdaptors() const
    {
        std::vector<WidgetLayoutAdaptor*> result;
        for (const auto& adaptor : m_adaptors) {
            result.push_back(adaptor.get());
        }
        return result;
    }

    /**
     * @brief 清空所有 MockWidget
     */
    void clear()
    {
        m_widgets.clear();
        m_adaptors.clear();
        m_containers.clear();
    }

private:
    std::vector<std::unique_ptr<Widget>> m_widgets;
    std::vector<std::unique_ptr<WidgetLayoutAdaptor>> m_adaptors;
    std::vector<std::unique_ptr<MockContainerWidget>> m_containers;
};

} // namespace mc::client::ui::kagero::test
