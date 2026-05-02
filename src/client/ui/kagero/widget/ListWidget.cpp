#include "ListWidget.hpp"
#include "../template/binder/BindingContext.hpp"

namespace mc::client::ui::kagero::widget {

void ListWidget::setItemsFromValue(const tpl::binder::Value& array) {
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

void ListWidget::refreshItems() {
    // 刷新当前项目（如果数据源仍然可用）
    // 这需要外部重新调用 setItemsFromValue
    updateContentHeight();
}

} // namespace mc::client::ui::kagero::widget
