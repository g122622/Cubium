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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/map/MapRenderer.hpp"
#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "common/entity/inventory/container/CartographyContainer.hpp"
#include "core/Types.hpp"

namespace mc::client {

/**
 * @brief 制图台屏幕
 *
 * 显示制图台GUI界面，包含：
 * - 地图槽位（左上）
 * - 材料槽位（左下）
 * - 结果槽位（右侧，带箭头）
 * - 地图预览区域（显示结果地图）
 * - 玩家背包和快捷栏
 *
 * 参考 MC 1.16.5 CartographyScreen
 */
class CartographyScreen : public AbstractContainerScreen<mc::CartographyContainer> {
public:
    /**
     * @brief 构造函数
     * @param containerId 容器ID
     * @param playerInventory 玩家背包
     * @param clickSender 点击发送器
     * @param closeSender 关闭发送器
     */
    CartographyScreen(ContainerId containerId,
        mc::PlayerInventory* playerInventory,
        ContainerClickSender clickSender = {},
        ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] std::string getTitle() const override { return "Cartography Table"; }

    /**
     * @brief 设置地图渲染器
     */
    void setMapRenderer(MapRenderer* mapRenderer) { m_mapRenderer = mapRenderer; }

protected:
    void onInit() override;
    void renderContainerBackground() override;
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;
    void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY) override;
    void renderTooltip(i32 mouseX, i32 mouseY) override;

private:
    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;

    // 制图台槽位布局常量（与CartographyContainer一致）
    static constexpr i32 MAP_SLOT_SCREEN_X = 15;
    static constexpr i32 MAP_SLOT_SCREEN_Y = 15;
    static constexpr i32 MATERIAL_SLOT_SCREEN_X = 15;
    static constexpr i32 MATERIAL_SLOT_SCREEN_Y = 40;
    static constexpr i32 RESULT_SLOT_SCREEN_X = 145;
    static constexpr i32 RESULT_SLOT_SCREEN_Y = 28;

    // 地图预览区域
    static constexpr f64 MAP_PREVIEW_SIZE = 64.0;
    static constexpr f64 MAP_PREVIEW_X = 85.0;
    static constexpr f64 MAP_PREVIEW_Y = 18.0;

    /** 地图渲染器 */
    MapRenderer* m_mapRenderer = nullptr;
};

} // namespace mc::client
