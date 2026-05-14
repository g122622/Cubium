#pragma once

#include "../../kagero/paint/PaintContext.hpp"
#include "Screen.hpp"
#include <memory>
#include <vector>

namespace mc::client::ui::minecraft {

class ScreenManager {
public:
    void push(std::unique_ptr<Screen> screen);
    void pop();
    void clear();

    [[nodiscard]] Screen* top();
    [[nodiscard]] const Screen* top() const;

    /**
     * @brief 绘制所有屏幕
     * @param ctx 绘图上下文
     */
    void paint(kagero::widget::PaintContext& ctx);

    /**
     * @brief 更新所有屏幕的悬停状态
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    void updateHover(i32 mouseX, i32 mouseY);

private:
    std::vector<std::unique_ptr<Screen>> m_stack;
};

} // namespace mc::client::ui::minecraft
