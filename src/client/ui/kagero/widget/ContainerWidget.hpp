#pragma once

#include "Widget.hpp"
#include "IWidgetContainer.hpp"
#include "../paint/PaintContext.hpp"
#include "../layout/algorithms/FlexLayout.hpp"
#include "../layout/algorithms/GridLayout.hpp"
#include <memory>

namespace mc::client::ui::kagero::widget {

/**
 * @brief 容器布局类型
 */
enum class ContainerLayoutType : u8 {
    None,   ///< 无布局（绝对定位）
    Flex,   ///< 弹性布局
    Grid,   ///< 网格布局
    Anchor  ///< 锚点布局
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
    explicit ContainerWidget(String id)
        : Widget(std::move(id)) {}

    // ========== 绘制和更新 ==========

    void paint(PaintContext& ctx) override;
    void tick(f32 dt) override;

    // ========== 鼠标事件 ==========

    bool onClick(i32 mouseX, i32 mouseY, i32 button) override;
    bool onRelease(i32 mouseX, i32 mouseY, i32 button) override;
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY) override;
    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override;

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
