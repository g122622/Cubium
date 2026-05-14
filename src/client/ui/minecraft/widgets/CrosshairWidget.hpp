#pragma once

#include "../../kagero/paint/PaintContext.hpp"
#include "../../kagero/widget/Widget.hpp"

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
}

namespace mc::client::ui::minecraft::widgets {

/**
 * @brief 准星Widget
 *
 * 在屏幕中心渲染十字准星，用于第一人称视角瞄准。
 *
 * 参考 MC 1.16.5 IngameGui.renderAttackIndicator()
 */
class CrosshairWidget : public kagero::widget::Widget {
public:
    CrosshairWidget();
    ~CrosshairWidget() override = default;

    /**
     * @brief 绘制准星
     */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 设置准星颜色（ARGB格式）
     */
    void setColor(u32 color) { m_color = color; }

    /**
     * @brief 设置准星大小（十字线长度）
     */
    void setSize(f32 size) { m_size = size; }

    /**
     * @brief 设置准星线宽
     */
    void setThickness(f32 thickness) { m_thickness = thickness; }

    /**
     * @brief 获取准星颜色
     */
    [[nodiscard]] u32 color() const { return m_color; }

    /**
     * @brief 获取准星大小
     */
    [[nodiscard]] f32 size() const { return m_size; }

private:
    u32 m_color = 0xFFFFFFFF; ///< 白色，完全不透明
    f32 m_size = 10.0f;       ///< 十字线长度
    f32 m_thickness = 1.0f;   ///< 线宽
};

} // namespace mc::client::ui::minecraft::widgets
