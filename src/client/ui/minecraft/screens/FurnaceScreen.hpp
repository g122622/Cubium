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

#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ContainerInteraction.hpp"
#include "client/ui/minecraft/screens/ContainerScreenBase.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>
#include <utility>

namespace mc::client::ui::minecraft {

/**
 * @brief 熔炉屏幕（kagero 体系）
 *
 * 输入槽 / 燃料槽 / 输出槽 + 玩家背包（主背包 27 + 快捷栏 9），共 39 槽。
 * 继承 ContainerScreenBase<mc::blockentity::FurnaceContainer>，交互统一走 ContainerInteraction。
 *
 * 燃烧火焰指示器与熔炼进度箭头由 FurnaceContainer::getLitProgress/getBurnProgress 驱动，
 * 其值经 tracked int + WindowPropertyPacket 在服务端/客户端间同步（服务端每 tick 从实体刷新，
 * 客户端经 setTrackedInt 写入），客户端侧无需熔炉方块实体。
 */
class FurnaceScreen : public ContainerScreenBase<mc::blockentity::FurnaceContainer> {
public:
    using ContainerClickSender =
        typename kagero::widget::ContainerInteraction<mc::blockentity::FurnaceContainer>::ContainerClickSender;
    using ContainerCloseSender =
        typename kagero::widget::ContainerInteraction<mc::blockentity::FurnaceContainer>::ContainerCloseSender;

    /**
     * @brief 构造函数
     * @param menu 熔炉容器菜单（子类转入，已建好槽位）
     * @param clickSender 点击发送器
     * @param closeSender 关闭发送器
     */
    FurnaceScreen(std::unique_ptr<mc::blockentity::FurnaceContainer> menu,
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
    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;

    std::unique_ptr<kagero::widget::ContainerInteraction<mc::blockentity::FurnaceContainer>> m_interaction;
};

} // namespace mc::client::ui::minecraft
