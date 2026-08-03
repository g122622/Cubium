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
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc::client::ui::minecraft {

/**
 * @brief Minecraft 游戏界面屏幕基类
 *
 * 所有 Minecraft 风格的屏幕（如主菜单、设置界面、背包界面等）都应继承此类。
 * 提供屏幕生命周期回调（onOpen/onClose）、绘制和悬停更新等基础功能。
 */
class Screen : public kagero::widget::ContainerWidget {
public:
    explicit Screen(std::string id);

    /** @brief 屏幕被打开时调用，子类可重写以初始化界面状态 */
    virtual void onOpen();

    /** @brief 屏幕被关闭时调用，子类可重写以清理资源 */
    virtual void onClose();

    /** @brief 绘制屏幕内容，同时绘制所有子组件 */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 更新屏幕及其子组件的悬停状态
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    void updateHover(i32 mouseX, i32 mouseY) override;

    /** @brief 查询此屏幕是否为模态（模态屏幕会阻止下层屏幕接收输入） */
    [[nodiscard]] bool isModal() const;

    /** @brief 设置屏幕的模态状态 */
    void setModal(bool modal);

    /** @brief 查询此屏幕是否为暂停屏幕（暂停屏幕会导致游戏暂停） */
    [[nodiscard]] bool isPauseScreen() const;

    /** @brief 设置此屏幕是否为暂停屏幕 */
    void setPauseScreen(bool pauseScreen);

private:
    bool m_modal = true;
    bool m_pauseScreen = false;
};

} // namespace mc::client::ui::minecraft
