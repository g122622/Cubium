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

namespace mc {
class CraftingMenu;
}

namespace mc::client::ui::minecraft {

/**
 * @brief 工作台屏幕（kagero 体系）
 *
 * 3x3 合成网格 + 结果槽 + 玩家背包（主背包 27 + 快捷栏 9），共 46 槽。
 * 继承 ContainerScreenBase<mc::CraftingMenu>，交互统一走 ContainerInteraction。
 * 结果槽点击 / Shift 合成由 CraftingMenu::clicked 在菜单层处理，屏幕层无特有逻辑。
 */
class CraftingScreen : public ContainerScreenBase<mc::CraftingMenu> {
public:
    using ContainerClickSender = typename kagero::widget::ContainerInteraction<mc::CraftingMenu>::ContainerClickSender;
    using ContainerCloseSender = typename kagero::widget::ContainerInteraction<mc::CraftingMenu>::ContainerCloseSender;

    CraftingScreen(
        std::unique_ptr<mc::CraftingMenu> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender);

    // ==================== 事件（转发给 m_interaction） ====================

    void updateHover(i32 mouseX, i32 mouseY) override;
    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override;
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

protected:
    // ==================== ContainerScreenBase 钩子 ====================

    [[nodiscard]] i32 guiWidth() const override { return GUI_WIDTH; }
    [[nodiscard]] i32 guiHeight() const override { return GUI_HEIGHT; }
    [[nodiscard]] std::pair<i32, i32> slotLocalPos(i32 slotIndex) const override;
    [[nodiscard]] const mc::ItemStack& getCarriedItem() const override;
    void renderContainerBackground(kagero::widget::PaintContext& ctx) override;
    void renderContainerForeground(kagero::widget::PaintContext& ctx) override;
    void renderTooltip(kagero::widget::PaintContext& ctx) override;

private:
    // ==================== 布局常量（相对 GUI 左上角） ====================

    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;

    static constexpr i32 GRID_X = 30; // 3x3 合成网格左上 X
    static constexpr i32 GRID_Y = 17; // 3x3 合成网格左上 Y
    static constexpr i32 GRID_COL_COUNT = 3;

    static constexpr i32 RESULT_X = 123; // 结果槽 X
    static constexpr i32 RESULT_Y = 35;  // 结果槽 Y

    static constexpr i32 TITLE_X = 28;
    static constexpr i32 TITLE_Y = 6;

    std::unique_ptr<kagero::widget::ContainerInteraction<mc::CraftingMenu>> m_interaction;
};

} // namespace mc::client::ui::minecraft
