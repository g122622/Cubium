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

#include "ContainerWidget.hpp"
#include "../layout/core/LayoutEngine.hpp"
#include "../layout/integration/WidgetLayoutAdaptor.hpp"

namespace mc::client::ui::kagero::widget {

void ContainerWidget::paint(PaintContext& ctx)
{
    if (!isVisible()) {
        return;
    }

    if (backgroundColor() != 0x00000000) {
        ctx.drawFilledRect(bounds(), backgroundColor());
    }
    if (borderColor() != 0x00000000) {
        ctx.drawBorder(bounds(), 1.0f, borderColor());
    }

    paintChildren(ctx);
}

void ContainerWidget::tick(f32 dt)
{
    tickChildren(dt);

    if (m_layoutDirty) {
        relayout();
    }
}

// ========== 鼠标事件 ==========

bool ContainerWidget::onClick(i32 mouseX, i32 mouseY, i32 button)
{
    return handleClickInChildren(mouseX, mouseY, button);
}

bool ContainerWidget::onRelease(i32 mouseX, i32 mouseY, i32 button)
{
    return handleReleaseInChildren(mouseX, mouseY, button);
}

bool ContainerWidget::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY)
{
    return handleDragInChildren(mouseX, mouseY, deltaX, deltaY);
}

bool ContainerWidget::onScroll(i32 mouseX, i32 mouseY, f64 delta)
{
    return handleScrollInChildren(mouseX, mouseY, delta);
}

// ========== 键盘事件 ==========

bool ContainerWidget::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    return handleKeyInChildren(key, scanCode, action, mods);
}

bool ContainerWidget::onChar(u32 codePoint)
{
    return handleCharInChildren(codePoint);
}

// ========== 布局系统 ==========

void ContainerWidget::setLayoutType(ContainerLayoutType type)
{
    if (m_layoutType != type) {
        m_layoutType = type;
        requestLayout();
    }
}

void ContainerWidget::setFlexConfig(const layout::FlexConfig& config)
{
    m_flexConfig = config;
    if (m_layoutType == ContainerLayoutType::Flex) {
        requestLayout();
    }
}

void ContainerWidget::setGridConfig(const layout::GridConfig& config)
{
    m_gridConfig = config;
    if (m_layoutType == ContainerLayoutType::Grid) {
        requestLayout();
    }
}

void ContainerWidget::requestLayout()
{
    m_layoutDirty = true;
    if (parent() != nullptr) {
        if (auto* container = dynamic_cast<ContainerWidget*>(parent())) {
            container->requestLayout();
        }
    }
}

void ContainerWidget::relayout()
{
    if (!m_layoutDirty || m_layoutType == ContainerLayoutType::None) {
        return;
    }

    auto& engine = layout::LayoutEngine::instance();
    layout::WidgetLayoutAdaptor adaptor(this);

    switch (m_layoutType) {
        case ContainerLayoutType::Flex:
            engine.layoutFlex(&adaptor, bounds(), m_flexConfig);
            break;
        case ContainerLayoutType::Grid:
            engine.layoutWith("grid", &adaptor, bounds());
            break;
        case ContainerLayoutType::Anchor:
            engine.layoutWith("anchor", &adaptor, bounds());
            break;
        default:
            break;
    }

    m_layoutDirty = false;

    for (auto& child : m_children) {
        if (auto* container = dynamic_cast<ContainerWidget*>(child.get())) {
            if (container->layoutType() != ContainerLayoutType::None) {
                container->relayout();
            }
        }
    }
}

void ContainerWidget::onResize(i32 width, i32 height)
{
    Widget::onResize(width, height);
    requestLayout();
}

} // namespace mc::client::ui::kagero::widget
