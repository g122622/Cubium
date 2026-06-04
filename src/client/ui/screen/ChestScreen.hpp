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

#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "core/Types.hpp"

namespace mc {
class PlayerInventory;
}

namespace mc::client {

/**
 * @brief 箱子屏幕
 */
class ChestScreen : public AbstractContainerScreen<mc::blockentity::ChestContainer> {
public:
    /**
     * @brief 构造函数
     * @param containerId 容器ID
     * @param playerInventory 玩家背包
     * @param rows 箱子行数
     * @param clickSender 点击发送器
     * @param closeSender 关闭发送器
     */
    ChestScreen(ContainerId containerId,
        mc::PlayerInventory* playerInventory,
        i32 rows,
        ContainerClickSender clickSender = {},
        ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] std::string getTitle() const noexcept override { return "Chest"; }

protected:
    void onInit() override;
    void renderContainerBackground() override;
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;
    void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY) override;
    void renderTooltip(i32 mouseX, i32 mouseY) override;

private:
    static constexpr i32 GUI_WIDTH = 176;       ///< GUI背景宽度（像素）
    static constexpr i32 BASE_GUI_HEIGHT = 114; ///< GUI基础高度（不含箱子槽位区域）
    static constexpr i32 SLOT_SPACING = 18;     ///< 槽位间距（像素）
    static constexpr i32 TITLE_X = 8;           ///< 标题X偏移
    static constexpr i32 TITLE_Y = 6;           ///< 标题Y偏移

    i32 m_rows; ///< 箱子行数
};

} // namespace mc::client
