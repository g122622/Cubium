#pragma once

#include "SlotWidget.hpp"

namespace mc::client::ui::minecraft {

class InventorySlot : public SlotWidget {
public:
    using SlotWidget::SlotWidget;

    void setSlotGroup(std::string group);
    [[nodiscard]] const std::string& slotGroup() const;

private:
    std::string m_slotGroup = "inventory";
};

} // namespace mc::client::ui::minecraft
