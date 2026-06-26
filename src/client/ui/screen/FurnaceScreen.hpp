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

#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include "common/world/blockentity/processing/FurnaceInventory.hpp"
#include "core/Types.hpp"

namespace mc {
class PlayerInventory;
}

namespace mc::client {

/**
 * @brief 熔炉屏幕
 *
 * 显示熔炉GUI界面，包括背景纹理、燃烧火焰指示器和熔炼进度箭头。
 * 燃烧和熔炼进度数据从关联的AbstractFurnaceEntity读取。
 */
class FurnaceScreen : public AbstractContainerScreen<mc::blockentity::FurnaceContainer> {
public:
    /**
     * @brief 构造函数
     * @param containerId 容器ID
     * @param playerInventory 玩家背包
     * @param clickSender 点击发送器
     * @param closeSender 关闭发送器
     */
    FurnaceScreen(ContainerId containerId,
        mc::PlayerInventory* playerInventory,
        ContainerClickSender clickSender = {},
        ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] std::string getTitle() const override { return "Furnace"; }

    /// GUI宽度常量（与MC Java标准容器尺寸一致）
    static constexpr i32 GUI_WIDTH = 176;
    /// GUI高度常量（与MC Java标准容器尺寸一致）
    static constexpr i32 GUI_HEIGHT = 166;

    // 允许移动
    FurnaceScreen(FurnaceScreen&&) noexcept = default;
    FurnaceScreen& operator=(FurnaceScreen&&) noexcept = default;

protected:
    void onInit() override;
    void renderContainerBackground() override;
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;
    void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY) override;
    void renderTooltip(i32 mouseX, i32 mouseY) override;

private:
    /**
     * @brief 获取燃料燃烧进度（0.0~1.0）
     *
     * 从熔炉实体获取当前燃烧时间与总燃烧时间的比值，
     * 用于驱动火焰指示器动画。与MC Java的
     * AbstractFurnaceMenu.getLitProgress()一致。
     */
    [[nodiscard]] f32 getLitProgress() const;

    /**
     * @brief 获取熔炼进度（0.0~1.0）
     *
     * 从熔炉实体获取当前熔炼时间与总熔炼时间的比值，
     * 用于驱动进度箭头动画。与MC Java的
     * AbstractFurnaceMenu.getBurnProgress()一致。
     */
    [[nodiscard]] f32 getBurnProgress() const;

    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;
};

} // namespace mc::client
