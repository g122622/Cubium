/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "client/renderer/map/MapRenderer.hpp"
#include "client/ui/kagero/widget/ContainerInteraction.hpp"
#include "client/ui/minecraft/screens/ContainerScreenBase.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "common/entity/inventory/container/CartographyContainer.hpp"

namespace mc::client::ui::minecraft {

/**
 * @brief 制图台屏幕（kagero 体系）
 *
 * 地图槽 / 材料槽 / 结果槽 + 玩家背包（主背包 27 + 快捷栏 9），共 39 槽。
 * 槽位布局坐标直接取自 CartographyContainer 内 Slot 的 getX/getY（菜单为权威布局源）。
 * 继承 ContainerScreenBase<mc::CartographyContainer>，交互统一走 ContainerInteraction。
 *
 * 结果槽若为已填充地图，则在 GUI 右侧绘制 64×64 地图预览。预览由 MapRenderer 渲染，
 * 客户端无地图数据计算（updateResult 在 m_world==nullptr 时空操作，结果槽由服务端经
 * ContainerSlotPacket 下推）。MapRenderer 经 setMapRenderer 注入，未注入时不绘制预览。
 */
class CartographyScreen : public ContainerScreenBase<mc::CartographyContainer> {
public:
    using ContainerClickSender =
        typename kagero::widget::ContainerInteraction<mc::CartographyContainer>::ContainerClickSender;
    using ContainerCloseSender =
        typename kagero::widget::ContainerInteraction<mc::CartographyContainer>::ContainerCloseSender;

    /**
     * @brief 构造函数
     * @param menu 制图台容器菜单（子类转入，已建好槽位）
     * @param clickSender 点击发送器
     * @param closeSender 关闭发送器
     */
    CartographyScreen(std::unique_ptr<mc::CartographyContainer> menu,
        ContainerClickSender clickSender,
        ContainerCloseSender closeSender);

    /**
     * @brief 设置地图渲染器（用于结果槽地图预览），未注入则不绘制预览
     */
    void setMapRenderer(MapRenderer* mapRenderer) { m_mapRenderer = mapRenderer; }

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

    // 箭头区域（输入→输出指示），相对 GUI 左上角
    static constexpr i32 ARROW_X = 82;
    static constexpr i32 ARROW_Y = 28;

    // 地图预览区域（结果槽为已填充地图时绘制）
    static constexpr f64 MAP_PREVIEW_SIZE = 64.0;
    static constexpr f64 MAP_PREVIEW_X = 85.0;
    static constexpr f64 MAP_PREVIEW_Y = 18.0;

    std::unique_ptr<kagero::widget::ContainerInteraction<mc::CartographyContainer>> m_interaction;
    MapRenderer* m_mapRenderer = nullptr;
};

} // namespace mc::client::ui::minecraft
