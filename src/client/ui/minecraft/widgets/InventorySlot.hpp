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

#include "SlotWidget.hpp"
#include <string>

namespace mc::client::ui::minecraft {

/**
 * @brief 背物品栏槽位控件
 *
 * 在 SlotWidget 基础上扩展了槽位分组（slotGroup）概念，
 * 用于区分不同区域的物品栏槽位（如主物品栏、快捷栏、盔甲栏等）。
 */
class InventorySlot : public SlotWidget {
public:
    using SlotWidget::SlotWidget;

    void setSlotGroup(std::string group) noexcept;
    [[nodiscard]] const std::string& slotGroup() const noexcept;

private:
    std::string m_slotGroup = "inventory";
};

} // namespace mc::client::ui::minecraft
