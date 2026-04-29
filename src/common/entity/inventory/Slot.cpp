#include "Slot.hpp"
#include "IInventory.hpp"
#include "../../item/items/armor/ArmorItem.hpp"

namespace mc {

Slot::Slot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
    : m_inventory(inventory)
    , m_slotIndex(slotIndex)
    , m_x(x)
    , m_y(y)
{
}

ItemStack Slot::getItem() const {
    if (m_inventory == nullptr) {
        return ItemStack::EMPTY;
    }
    return m_inventory->getItem(m_slotIndex);
}

void Slot::set(const ItemStack& stack) {
    if (m_inventory != nullptr) {
        m_inventory->setItem(m_slotIndex, stack);
    }
}

bool Slot::isEmpty() const {
    return getItem().isEmpty();
}

ItemStack Slot::remove(i32 amount) {
    if (m_inventory == nullptr) {
        return ItemStack::EMPTY;
    }
    return m_inventory->removeItem(m_slotIndex, amount);
}

bool Slot::mayPlace(const ItemStack& stack) const {
    if (m_inventory == nullptr) {
        return false;
    }
    return m_inventory->canPlaceItem(m_slotIndex, stack);
}

bool Slot::mayPickup(Player& player) const {
    (void)player;
    // 默认允许拾取，子类可重写此方法
    return true;
}

void Slot::setChanged() {
    if (m_inventory != nullptr) {
        m_inventory->setChanged();
    }
}

i32 Slot::getMaxStackSize() const {
    if (m_inventory == nullptr) {
        return 64;
    }
    return m_inventory->getMaxStackSize();
}

i32 Slot::getMaxStackSize(const ItemStack& stack) const {
    if (stack.isEmpty()) {
        return getMaxStackSize();
    }
    return std::min(stack.getMaxStackSize(), getMaxStackSize());
}

// ============================================================================
// ArmorSlot
// ============================================================================

ArmorSlot::ArmorSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, ArmorType armorType)
    : Slot(inventory, slotIndex, x, y)
    , m_armorType(armorType)
{
}

bool ArmorSlot::mayPlace(const ItemStack& stack) const {
    if (!Slot::mayPlace(stack)) {
        return false;
    }

    const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(stack.getItem());
    if (armorItem == nullptr) {
        return false;
    }

    switch (m_armorType) {
        case ArmorType::Head:
            return armorItem->isHelmet();
        case ArmorType::Chest:
            return armorItem->isChestplate();
        case ArmorType::Legs:
            return armorItem->isLeggings();
        case ArmorType::Feet:
            return armorItem->isBoots();
    }

    return false;
}

// ============================================================================
// ResultSlot
// ============================================================================

ResultSlot::ResultSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y,
                       CraftingInventory* craftingGrid)
    : Slot(inventory, slotIndex, x, y)
    , m_craftingGrid(craftingGrid)
{
}

} // namespace mc
