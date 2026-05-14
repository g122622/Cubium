#include "InventorySlot.hpp"

namespace mc::client::ui::minecraft {

void InventorySlot::setSlotGroup(std::string group)
{
    m_slotGroup = std::move(group);
}

const std::string& InventorySlot::slotGroup() const
{
    return m_slotGroup;
}

} // namespace mc::client::ui::minecraft
