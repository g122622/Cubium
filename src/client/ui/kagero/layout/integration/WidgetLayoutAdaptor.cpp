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

#include "WidgetLayoutAdaptor.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/core/LayoutResult.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include "client/ui/kagero/widget/IWidgetContainer.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::layout {

namespace {

/**
 * @brief 为子 Widget 创建对应的布局适配器
 *
 * 容器型 Widget 会创建 `ContainerLayoutAdaptor`，其余 Widget 使用通用适配器。
 * 该函数只负责包装，不接管 Widget 所有权。
 */
std::unique_ptr<WidgetLayoutAdaptor> createChildAdaptor(widget::Widget* widget)
{
    if (widget == nullptr) {
        return nullptr;
    }

    if (auto* container = dynamic_cast<widget::IWidgetContainer*>(widget); container != nullptr) {
        return std::make_unique<ContainerLayoutAdaptor>(widget, container);
    }

    return std::make_unique<WidgetLayoutAdaptor>(widget);
}

} // namespace

// ============================================================================
// WidgetLayoutAdaptor 实现
// ============================================================================

WidgetLayoutAdaptor::WidgetLayoutAdaptor(widget::Widget* widget)
    : m_widget(widget)
{
    if (m_widget != nullptr) {
        m_constraints.margin = m_widget->margin();
        m_constraints.padding = m_widget->padding();
    }
}

const std::string& WidgetLayoutAdaptor::id() const
{
    static const std::string empty;
    return m_widget != nullptr ? m_widget->id() : empty;
}

Size WidgetLayoutAdaptor::currentSize() const
{
    if (m_widget == nullptr) {
        return Size();
    }

    return Size(m_widget->width(), m_widget->height());
}

Rect WidgetLayoutAdaptor::currentBounds() const
{
    if (m_widget == nullptr) {
        return Rect();
    }

    return m_widget->bounds();
}

Margin WidgetLayoutAdaptor::margin() const
{
    return m_constraints.margin;
}

Padding WidgetLayoutAdaptor::padding() const
{
    return m_constraints.padding;
}

Size WidgetLayoutAdaptor::measure(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec)
{
    // 检查Widget约束是否变化（preferredSize、minSize等）
    bool constraintsChanged = false;
    if (m_widget != nullptr) {
        auto& currentConstraints = m_constraints;
        if (m_lastPreferredWidth != currentConstraints.preferredWidth ||
            m_lastPreferredHeight != currentConstraints.preferredHeight ||
            m_lastMinWidth != currentConstraints.minWidth || m_lastMinHeight != currentConstraints.minHeight) {
            constraintsChanged = true;
            m_lastPreferredWidth = currentConstraints.preferredWidth;
            m_lastPreferredHeight = currentConstraints.preferredHeight;
            m_lastMinWidth = currentConstraints.minWidth;
            m_lastMinHeight = currentConstraints.minHeight;
        }
    }

    if (m_cacheValid && !constraintsChanged && m_lastWidthSpec == widthSpec && m_lastHeightSpec == heightSpec) {
        return m_lastMeasuredSize;
    }

    Size measured = m_measureFunc ? m_measureFunc(this, widthSpec, heightSpec) : _measureDefault(widthSpec, heightSpec);

    measured.width = m_constraints.clampWidth(measured.width);
    measured.height = m_constraints.clampHeight(measured.height);

    m_lastMeasuredSize = measured;
    m_lastWidthSpec = widthSpec;
    m_lastHeightSpec = heightSpec;
    m_cacheValid = true;
    return measured;
}

Size WidgetLayoutAdaptor::_measureDefault(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec)
{
    if (m_widget == nullptr) {
        return Size();
    }

    const i32 preferredWidth = m_constraints.preferredWidth;
    const i32 preferredHeight = m_constraints.preferredHeight;
    const bool hasPreferredWidth = preferredWidth >= 0;
    const bool hasPreferredHeight = preferredHeight >= 0;
    const bool needContainerMeasure = isContainer() && (!hasPreferredWidth || !hasPreferredHeight);

    Size containerSize;
    if (needContainerMeasure) {
        containerSize = _measureContainer(widthSpec, heightSpec);
    }

    Size result;

    if (hasPreferredWidth) {
        result.width = widthSpec.resolve(preferredWidth);
    } else if (needContainerMeasure) {
        result.width = containerSize.width;
    } else if (m_widget->width() > 0) {
        result.width = widthSpec.resolve(m_widget->width());
    } else {
        result.width = widthSpec.resolve(m_constraints.minWidth);
    }

    if (hasPreferredHeight) {
        result.height = heightSpec.resolve(preferredHeight);
    } else if (needContainerMeasure) {
        result.height = containerSize.height;
    } else if (m_widget->height() > 0) {
        result.height = heightSpec.resolve(m_widget->height());
    } else {
        result.height = heightSpec.resolve(m_constraints.minHeight);
    }

    if (m_constraints.aspectRatio > 0.0f && result.width > 0 && result.height > 0) {
        const f32 currentRatio = static_cast<f32>(result.width) / static_cast<f32>(result.height);
        if (currentRatio > m_constraints.aspectRatio) {
            result.height = static_cast<i32>(result.width / m_constraints.aspectRatio);
        } else if (currentRatio < m_constraints.aspectRatio) {
            result.width = static_cast<i32>(result.height * m_constraints.aspectRatio);
        }
    }

    return result;
}

Size WidgetLayoutAdaptor::_measureContainer(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec)
{
    const auto children = getChildren();
    if (children.empty()) {
        return Size(m_constraints.minWidth + m_constraints.padding.horizontal(),
            m_constraints.minHeight + m_constraints.padding.vertical());
    }

    i32 maxWidth = m_constraints.minWidth;
    i32 maxHeight = m_constraints.minHeight;

    for (auto* child : children) {
        if (child == nullptr || !child->constraints().isLayoutEnabled()) {
            continue;
        }

        Size childSize = child->measure(MeasureSpec::MakeUnspecified(), MeasureSpec::MakeUnspecified());

        childSize.width += child->constraints().margin.horizontal();
        childSize.height += child->constraints().margin.vertical();

        maxWidth = std::max(maxWidth, childSize.width);
        maxHeight = std::max(maxHeight, childSize.height);
    }

    maxWidth += m_constraints.padding.horizontal();
    maxHeight += m_constraints.padding.vertical();

    return Size(widthSpec.resolve(maxWidth), heightSpec.resolve(maxHeight));
}

void WidgetLayoutAdaptor::applyLayout(const LayoutResult& result)
{
    if (m_widget == nullptr) {
        return;
    }

    m_widget->setBounds(result.bounds);
    m_layoutDirty = false;
    m_renderDirty = result.needsRepaint;
}

void WidgetLayoutAdaptor::applyLayout(i32 x, i32 y, i32 width, i32 height)
{
    applyLayout(LayoutResult(x, y, width, height));
}

void WidgetLayoutAdaptor::requestLayout()
{
    if (m_layoutDirty) {
        return;
    }

    m_layoutDirty = true;
    m_cacheValid = false;
    m_childrenCacheDirty = true;
    _propagateLayoutRequest();
}

void WidgetLayoutAdaptor::_propagateLayoutRequest()
{
    if (m_widget == nullptr) {
        return;
    }

    auto* parent = m_widget->parent();
    if (parent == nullptr) {
        return;
    }

    // TODO: 当前架构没有显式的父级布局失效接口，只能触发现有的边界更新回调。
    //       未来需要添加 ILayoutParent::requestChildLayout() 之类的接口来正确传播。
    auto* parentWidget = dynamic_cast<widget::Widget*>(parent);
    if (parentWidget == nullptr) {
        return;
    }

    parentWidget->setBounds(parentWidget->bounds());
}

std::vector<WidgetLayoutAdaptor*> WidgetLayoutAdaptor::getChildren()
{
    std::vector<WidgetLayoutAdaptor*> children;

    if (m_widget == nullptr) {
        m_childAdaptorsCache.clear();
        m_childrenCacheDirty = false;
        return children;
    }

    auto* container = dynamic_cast<widget::IWidgetContainer*>(m_widget);
    if (container == nullptr) {
        m_childAdaptorsCache.clear();
        m_childrenCacheDirty = false;
        return children;
    }

    if (m_childrenCacheDirty) {
        m_childAdaptorsCache.clear();
        const auto& widgets = container->widgets();
        m_childAdaptorsCache.reserve(widgets.size());

        for (const auto& child : widgets) {
            auto adaptor = createChildAdaptor(child.get());
            if (adaptor == nullptr) {
                continue;
            }

            adaptor->setDepth(m_depth + 1);
            m_childAdaptorsCache.push_back(std::move(adaptor));
        }
        m_childrenCacheDirty = false;
    }

    children.reserve(m_childAdaptorsCache.size());
    for (const auto& adaptor : m_childAdaptorsCache) {
        children.push_back(adaptor.get());
    }

    return children;
}

size_t WidgetLayoutAdaptor::childCount() const
{
    if (m_widget == nullptr) {
        return 0;
    }

    auto* container = dynamic_cast<const widget::IWidgetContainer*>(m_widget);
    if (container == nullptr) {
        return 0;
    }

    return container->widgetCount();
}

bool WidgetLayoutAdaptor::isContainer() const
{
    return m_widget != nullptr && dynamic_cast<const widget::IWidgetContainer*>(m_widget) != nullptr;
}

i32 WidgetLayoutAdaptor::getBaseline() const
{
    if (m_widget == nullptr) {
        return 0;
    }

    // 优先使用Widget的userData中存储的baseline值
    if (auto* baselineStr = m_widget->getUserData("baseline")) {
        try {
            return std::stoi(*baselineStr);
        }
        catch (...) {
            // 解析失败，使用默认值
        }
    }

    // 默认：返回元素高度的一半作为近似基线
    return m_widget->height() / 2;
}

// ============================================================================
// ContainerLayoutAdaptor 实现
// ============================================================================

ContainerLayoutAdaptor::ContainerLayoutAdaptor(widget::Widget* widget, widget::IWidgetContainer* container)
    : WidgetLayoutAdaptor(widget)
    , m_container(container)
{}

std::vector<WidgetLayoutAdaptor> ContainerLayoutAdaptor::getChildAdaptors()
{
    std::vector<WidgetLayoutAdaptor> result;
    if (m_container == nullptr) {
        return result;
    }

    const auto& widgets = m_container->widgets();
    result.reserve(widgets.size());

    for (const auto& child : widgets) {
        result.emplace_back(child.get());
        result.back().setDepth(depth() + 1);
    }

    return result;
}

} // namespace mc::client::ui::kagero::layout
