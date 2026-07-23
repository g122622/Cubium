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
#include "client/ui/kagero/widget/ContainerInteraction.hpp"
#include "client/ui/kagero/widget/CreativePaletteGridWidget.hpp"
#include "client/ui/kagero/widget/TextFieldWidget.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "core/Types.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
class ItemPickerMenu;
} // namespace mc

namespace mc::client::ui::minecraft {

/**
 * @brief 创造模式物品库屏幕（kagero 体系）
 *
 * 继承 ContainerScreenBase<ItemPickerMenu>：玩家背包槽位（护甲/主背包/快捷栏/
 * 副手，41 槽，右面板）的渲染/布局/居中定位/carried item 由基类提供；交互（背包
 * 槽 Pick/QuickMove/Swap/Throw）走 ContainerInteraction<ItemPickerMenu>。
 *
 * 创造特有 UI 作为子 widget 挂在本屏：
 * - 左面板 CreativePaletteGridWidget（9×5 可见行，带剔除+裁剪）：承载创造物品池，
 *   点击经回调上抛，由本屏调 ContainerInteraction::sendClone 发 ClickAction::Clone
 *   （虚拟槽 slotIndex = ItemPickerMenu::PALETTE_VIRTUAL_BASE + visibleIndex），
 *   服务端 ItemPickerMenu clone 到光标，carried 经 ContainerContentPacket 回传。
 * - 搜索框 TextFieldWidget：文本变化触发 _rebuildVisibleEntries + palette refresh。
 * - 垃圾桶：左面板右上角，丢弃 carried（发 outside-click Throw 或本地清 carried）。
 *
 * 槽位布局：右面板偏移 (INVENTORY_X=180, INVENTORY_Y=6) 内叠加基类的
 * armor/main/hotbar/offhand 相对坐标（与 InventoryScreen 一致）。GUI 360×226。
 */
class CreativeScreen : public ContainerScreenBase<mc::ItemPickerMenu> {
public:
    using ContainerClickSender =
        typename kagero::widget::ContainerInteraction<mc::ItemPickerMenu>::ContainerClickSender;
    using ContainerCloseSender =
        typename kagero::widget::ContainerInteraction<mc::ItemPickerMenu>::ContainerCloseSender;

    /**
     * @brief 构造函数
     * @param menu 创造物品选择菜单（客户端本地构造，containerId=PLAYER_CONTAINER_ID）
     * @param clickSender 容器点击事件发送器（网络/本地）
     * @param closeSender 容器关闭事件发送器
     */
    CreativeScreen(
        std::unique_ptr<mc::ItemPickerMenu> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender);

    // ==================== 生命周期 ====================

    void onOpen() override;
    void updateHover(i32 mouseX, i32 mouseY) override;

    // ==================== 事件 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override;
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override;
    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override;
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;
    bool onChar(u32 codePoint) override;

private:
    // ========== ContainerScreenBase 钩子实现 ==========
    [[nodiscard]] i32 guiWidth() const override { return GUI_WIDTH; }
    [[nodiscard]] i32 guiHeight() const override { return GUI_HEIGHT; }
    [[nodiscard]] std::pair<i32, i32> slotLocalPos(i32 slotIndex) const override;
    [[nodiscard]] const mc::ItemStack& getCarriedItem() const override;
    void renderContainerBackground(kagero::widget::PaintContext& ctx) override;
    void renderExtraWidgets(kagero::widget::PaintContext& ctx) override;
    void renderContainerForeground(kagero::widget::PaintContext& ctx) override;
    void renderTooltip(kagero::widget::PaintContext& ctx) override;

    // ========== 内部逻辑 ==========
    void _buildPalette();
    void _rebuildVisibleEntries();
    void _injectPalettePaintCallback();
    [[nodiscard]] bool _isMouseOverTrash(i32 mouseX, i32 mouseY) const;
    [[nodiscard]] static std::string _normalizeSearchText(std::string_view text);

    // ========== GUI 尺寸与布局常量 ==========
    static constexpr i32 GUI_WIDTH = 360;
    static constexpr i32 GUI_HEIGHT = 226;

    // 左面板（调色板 + 搜索框 + 垃圾桶）
    static constexpr i32 PALETTE_X = 8;
    static constexpr i32 PALETTE_Y = 26;
    static constexpr i32 PALETTE_VISIBLE_ROWS = 5;
    static constexpr i32 SEARCH_X = 8;
    static constexpr i32 SEARCH_Y = 6;
    static constexpr i32 SEARCH_WIDTH = 160;
    static constexpr i32 SEARCH_HEIGHT = 16;
    static constexpr i32 TRASH_X = 170;
    static constexpr i32 TRASH_Y = 6;

    // 右面板（玩家背包）偏移
    static constexpr i32 INVENTORY_X = 180;
    static constexpr i32 INVENTORY_Y = 6;

    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;

    // ========== 数据成员 ==========
    std::unique_ptr<kagero::widget::ContainerInteraction<mc::ItemPickerMenu>> m_interaction;

    kagero::widget::CreativePaletteGridWidget* m_paletteGrid = nullptr;
    kagero::widget::TextFieldWidget* m_searchField = nullptr;

    std::vector<mc::CreativeInventoryEntry> m_paletteEntries;
    std::vector<i32> m_visibleEntries;
};

} // namespace mc::client::ui::minecraft
