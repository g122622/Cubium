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
#include "client/ui/kagero/widget/ScrollableWidget.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::widget {

// 析构函数在此定义，此时 tpl::binder::Value 已完整可见，
// std::unique_ptr<Value> 才能安全释放其持有的对象。
ListWidget::~ListWidget() = default;

// 默认/移动构造与移动赋值同样定义于此，避免头文件中因 m_dataSource 的
// unique_ptr<Value> 在潜在异常或默认实现路径上要求 Value 完整类型。
ListWidget::ListWidget() = default;

ListWidget::ListWidget(ListWidget&&) noexcept = default;
ListWidget& ListWidget::operator=(ListWidget&&) noexcept = default;

// 构造函数同样定义于此，避免头文件中因 m_dataSource 的 unique_ptr<Value>
// 在异常路径上要求 Value 完整类型而导致编译错误。
ListWidget::ListWidget(std::string id)
    : ScrollableWidget(std::move(id), 0, 0, 0, 0)
{}

ListWidget::ListWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
    : ScrollableWidget(std::move(id), x, y, width, height)
{}

void ListWidget::_rebuildItemsFromArray(const tpl::binder::Value& array)
{
    MC_ASSERT_RELEASE(array.isArray());

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
}

void ListWidget::setItemsFromValue(const tpl::binder::Value& array)
{
    // 缓存数据源，供后续 refreshItems() 重建使用
    m_dataSource = std::make_unique<tpl::binder::Value>(array);

    if (!array.isArray()) {
        clearItems();
        return;
    }

    _rebuildItemsFromArray(array);

    if (m_onItemsChanged) {
        m_onItemsChanged();
    }
}

void ListWidget::refreshItems()
{
    // 无缓存数据源时仅更新内容高度
    if (!m_dataSource) {
        updateContentHeight();
        return;
    }

    // 捕获当前选中状态，重建后尽可能恢复（重新校验范围）
    const i32 prevSelectedIndex = m_selectedIndex;
    const std::vector<i32> prevSelectedIndices = m_selectedIndices;

    if (!m_dataSource->isArray()) {
        // 数据源非数组：清空并通知
        clearItems();
        if (m_onItemsChanged) {
            m_onItemsChanged();
        }
        return;
    }

    _rebuildItemsFromArray(*m_dataSource);

    // 恢复选中状态：单选索引若仍有效则保留，多选列表过滤掉越界索引
    if (prevSelectedIndex >= 0 && prevSelectedIndex < static_cast<i32>(m_items.size())) {
        m_selectedIndex = prevSelectedIndex;
    } else {
        m_selectedIndex = -1;
    }

    if (!prevSelectedIndices.empty()) {
        m_selectedIndices.clear();
        m_selectedIndices.reserve(prevSelectedIndices.size());
        for (i32 idx : prevSelectedIndices) {
            if (idx >= 0 && idx < static_cast<i32>(m_items.size())) {
                m_selectedIndices.push_back(idx);
            }
        }
    }

    // 数据变更通知，与 setItemsFromValue 行为保持一致
    if (m_onItemsChanged) {
        m_onItemsChanged();
    }
}

} // namespace mc::client::ui::kagero::widget
