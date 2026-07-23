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

#include "client/ui/kagero/widget/ContainerInteraction.hpp"
#include "client/ui/minecraft/screens/ContainerScreenBase.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"

namespace mc::client::ui::minecraft {

/**
 * @brief 箱子屏幕（kagero 体系）
 *
 * 支持单箱（3 行 27 槽）与双箱（6 行 54 槽），GUI 高度随行数动态变化。
 * 槽位布局坐标直接取自 ChestContainer 内 Slot 的 getX/getY（菜单为权威布局源），
 * 避免屏幕层重复维护坐标常量导致漂移。继承 ContainerScreenBase<ChestContainer>，
 * 交互统一走 ContainerInteraction。
 */
class ChestScreen : public ContainerScreenBase<mc::blockentity::ChestContainer> {
public:
    using ContainerClickSender =
        typename kagero::widget::ContainerInteraction<mc::blockentity::ChestContainer>::ContainerClickSender;
    using ContainerCloseSender =
        typename kagero::widget::ContainerInteraction<mc::blockentity::ChestContainer>::ContainerCloseSender;

    /**
     * @brief 构造函数
     * @param menu 箱子容器菜单（子类转入，已按行数建好槽位）
     * @param clickSender 点击发送器
     * @param closeSender 关闭发送器
     */
    ChestScreen(std::unique_ptr<mc::blockentity::ChestContainer> menu,
        ContainerClickSender clickSender,
        ContainerCloseSender closeSender);

    // ==================== 事件（转发给 m_interaction） ====================

    void updateHover(i32 mouseX, i32 mouseY) override;
    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override;
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

protected:
    // ==================== ContainerScreenBase 钩子 ====================

    [[nodiscard]] i32 guiWidth() const override { return GUI_WIDTH; }
    [[nodiscard]] i32 guiHeight() const override;
    [[nodiscard]] std::pair<i32, i32> slotLocalPos(i32 slotIndex) const override;
    [[nodiscard]] const mc::ItemStack& getCarriedItem() const override;
    void renderContainerBackground(kagero::widget::PaintContext& ctx) override;
    void renderContainerForeground(kagero::widget::PaintContext& ctx) override;
    void renderTooltip(kagero::widget::PaintContext& ctx) override;

private:
    // ==================== 布局常量（相对 GUI 左上角） ====================

    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 BASE_GUI_HEIGHT = 114; ///< GUI 基础高度（不含箱子槽位区域）
    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;

    std::unique_ptr<kagero::widget::ContainerInteraction<mc::blockentity::ChestContainer>> m_interaction;
};

} // namespace mc::client::ui::minecraft
