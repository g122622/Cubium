#pragma once

#include "../../kagero/widget/ContainerWidget.hpp"

namespace mc::client::ui::minecraft {

class Screen : public kagero::widget::ContainerWidget {
public:
    explicit Screen(std::string id);

    virtual void onOpen();
    virtual void onClose();

    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 更新屏幕及其子组件的悬停状态
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    void updateHover(i32 mouseX, i32 mouseY);

    [[nodiscard]] bool isModal() const;
    void setModal(bool modal);

private:
    bool m_modal = true;
};

} // namespace mc::client::ui::minecraft
