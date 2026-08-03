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

#include "ContainerScreenBase.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ContainerInteraction.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/item/core/ItemStack.hpp"
#include "core/Types.hpp"
#include <memory>
#include <utility>

namespace mc {
class InventoryCraftingMenu;
} // namespace mc

namespace mc::client::ui::minecraft {

/**
 * @brief 玩家背包界面（kagero 体系）
 *
 * 用 kagero 的 SlotWidget 组件化实现玩家背包：2x2 合成网格、护甲槽、副手槽、
 * 主背包 3x9、快捷栏 1x9。渲染与槽位/布局/居中定位由 ContainerScreenBase 提供，
 * 交互逻辑委托给 ContainerInteraction<InventoryCraftingMenu>（与 MC 容器协议字节级对齐）。
 *
 * 槽位布局采用绝对定位，坐标常量与 MC 原版背包一致（GUI 176×166）。
 */
class InventoryScreen : public ContainerScreenBase<mc::InventoryCraftingMenu> {
public:
    using ContainerClickSender =
        typename kagero::widget::ContainerInteraction<mc::InventoryCraftingMenu>::ContainerClickSender;
    using ContainerCloseSender =
        typename kagero::widget::ContainerInteraction<mc::InventoryCraftingMenu>::ContainerCloseSender;

    /**
     * @brief 构造函数
     * @param menu 玩家背包合成菜单（客户端本地构造，containerId=PLAYER_CONTAINER_ID）
     * @param clickSender 容器点击事件发送器（网络/本地）
     * @param closeSender 容器关闭事件发送器
     */
    InventoryScreen(std::unique_ptr<mc::InventoryCraftingMenu> menu,
        ContainerClickSender clickSender,
        ContainerCloseSender closeSender);

    // ==================== 生命周期 ====================

    void updateHover(i32 mouseX, i32 mouseY) override;

    // ==================== 事件（转发给 ContainerInteraction） ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override;
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    // ========== ContainerScreenBase 钩子实现 ==========
    [[nodiscard]] i32 guiWidth() const override { return GUI_WIDTH; }
    [[nodiscard]] i32 guiHeight() const override { return GUI_HEIGHT; }
    [[nodiscard]] std::pair<i32, i32> slotLocalPos(i32 slotIndex) const override;
    [[nodiscard]] const mc::ItemStack& getCarriedItem() const override;
    void renderContainerBackground(kagero::widget::PaintContext& ctx) override;
    void renderContainerForeground(kagero::widget::PaintContext& ctx) override;

    // ========== GUI 尺寸与槽位坐标常量 ==========
    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;
    static constexpr i32 GRID_X = 98;
    static constexpr i32 GRID_Y = 18;
    static constexpr i32 RESULT_X = 154;
    static constexpr i32 RESULT_Y = 28;
    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;

    std::unique_ptr<kagero::widget::ContainerInteraction<mc::InventoryCraftingMenu>> m_interaction;
};

} // namespace mc::client::ui::minecraft
