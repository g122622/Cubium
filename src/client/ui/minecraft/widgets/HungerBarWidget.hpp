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
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Constants.hpp"

namespace mc::client::ui::minecraft {

/**
 * @brief 饥饿值条控件
 *
 * 显示玩家饥饿值的 HUD 控件。当前使用简单的矩形填充实现，
 * 未来需替换为与 MC 一致的鸡腿图标渲染。
 */
class HungerBarWidget : public kagero::widget::Widget {
public:
    HungerBarWidget();

    void setHunger(i32 hunger);
    [[nodiscard]] i32 hunger() const;

    void paint(kagero::widget::PaintContext& ctx) override;

private:
    i32 m_hunger = mc::game::PLAYER_MAX_HUNGER;
};

} // namespace mc::client::ui::minecraft
