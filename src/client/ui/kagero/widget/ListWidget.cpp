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

#include "ListWidget.hpp"
#include "../template/binder/BindingContext.hpp"

namespace mc::client::ui::kagero::widget {

void ListWidget::setItemsFromValue(const tpl::binder::Value& array)
{
    if (!array.isArray()) {
        clearItems();
        return;
    }

    clearItems();

    const auto& items = array.asArray();
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& itemData = items[i];
        if (m_itemFactory) {
            addItem(m_itemFactory(itemData, i));
        } else {
            // 默认：将 Value 转换为字符串显示
            addItem(std::make_unique<TextListItem>(itemData.toString()));
        }
    }

    if (m_onItemsChanged) {
        m_onItemsChanged();
    }
}

void ListWidget::refreshItems()
{
    // TODO: 实现基于数据源的自动刷新，当前需要外部重新调用 setItemsFromValue
    updateContentHeight();
}

} // namespace mc::client::ui::kagero::widget
