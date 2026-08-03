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

#pragma once

#include "IWidgetContainer.hpp"
#include "Widget.hpp"
#include "client/ui/kagero/layout/algorithms/FlexLayout.hpp"
#include "client/ui/kagero/layout/algorithms/GridLayout.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 容器布局类型
 */
enum class ContainerLayoutType : u8 {
    None,  ///< 无布局（绝对定位）
    Flex,  ///< 弹性布局
    Grid,  ///< 网格布局
    Anchor ///< 锚点布局
};

/**
 * @brief 通用容器控件
 *
 * 提供子组件管理、事件传播和布局功能。
 * 所有事件都会自动传播到子组件。
 */
class ContainerWidget : public Widget, public WidgetContainerMixin<ContainerWidget> {
public:
    ContainerWidget() = default;
    explicit ContainerWidget(std::string id)
        : Widget(std::move(id))
    {}

    // ========== 绘制和更新 ==========

    void paint(PaintContext& ctx) override;
    void tick(f32 dt) override;

    // ========== 鼠标事件 ==========

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onDoubleClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onRightClick(i32 mouseX, i32 mouseY, i32 mods) override;
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override;
    bool onDragStart(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onDragEnd(i32 mouseX, i32 mouseY, i32 button, bool dropped) override;
    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override;

    /**
     * @brief 更新自身及所有子控件的悬停状态
     */
    void updateHover(i32 mouseX, i32 mouseY) override;

    // ========== 键盘事件 ==========

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;
    bool onChar(u32 codePoint) override;

    // ========== 布局系统 ==========

    /**
     * @brief 设置布局类型
     */
    void setLayoutType(ContainerLayoutType type);

    /**
     * @brief 获取布局类型
     */
    [[nodiscard]] ContainerLayoutType layoutType() const { return m_layoutType; }

    /**
     * @brief 设置 Flex 布局配置
     */
    void setFlexConfig(const layout::FlexConfig& config);

    /**
     * @brief 获取 Flex 布局配置
     */
    [[nodiscard]] const layout::FlexConfig& flexConfig() const { return m_flexConfig; }

    /**
     * @brief 设置 Grid 布局配置
     */
    void setGridConfig(const layout::GridConfig& config);

    /**
     * @brief 获取 Grid 布局配置
     */
    [[nodiscard]] const layout::GridConfig& gridConfig() const { return m_gridConfig; }

    /**
     * @brief 请求重新布局
     */
    void requestLayout();

    /**
     * @brief 执行布局
     */
    void relayout();

    /**
     * @brief 当尺寸改变时调用
     */
    void onResize(i32 width, i32 height) override;

private:
    ContainerLayoutType m_layoutType = ContainerLayoutType::None;
    layout::FlexConfig m_flexConfig;
    layout::GridConfig m_gridConfig;
    bool m_layoutDirty = true;
};

} // namespace mc::client::ui::kagero::widget
