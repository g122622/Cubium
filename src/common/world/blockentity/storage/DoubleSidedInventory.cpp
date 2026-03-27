#include "world/blockentity/storage/DoubleSidedInventory.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

DoubleSidedInventory::DoubleSidedInventory(::mc::IInventory* upper, ::mc::IInventory* lower)
    : m_upper(upper)
    , m_lower(lower) {
    MC_ASSERT(upper != nullptr && "Upper inventory cannot be null");
    MC_ASSERT(lower != nullptr && "Lower inventory cannot be null");
}

i32 DoubleSidedInventory::getContainerSize() const {
    return m_upper->getContainerSize() + m_lower->getContainerSize();
}

bool DoubleSidedInventory::isEmpty() const {
    return m_upper->isEmpty() && m_lower->isEmpty();
}

i32 DoubleSidedInventory::getMaxStackSize() const {
    return std::min(m_upper->getMaxStackSize(), m_lower->getMaxStackSize());
}

ItemStack DoubleSidedInventory::getItem(i32 slot) const {
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        return container->getItem(localSlot);
    }
    return ItemStack();
}

void DoubleSidedInventory::setItem(i32 slot, const ItemStack& stack) {
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        container->setItem(localSlot, stack);
    }
}

ItemStack DoubleSidedInventory::removeItem(i32 slot, i32 count) {
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        return container->removeItem(localSlot, count);
    }
    return ItemStack();
}

ItemStack DoubleSidedInventory::removeItemNoUpdate(i32 slot) {
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        return container->removeItemNoUpdate(localSlot);
    }
    return ItemStack();
}

void DoubleSidedInventory::clear() {
    m_upper->clear();
    m_lower->clear();
}

void DoubleSidedInventory::setChanged() {
    m_upper->setChanged();
    m_lower->setChanged();
}

bool DoubleSidedInventory::canPlaceItem(i32 slot, const ItemStack& stack) const {
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        return container->canPlaceItem(localSlot, stack);
    }
    return false;
}

void DoubleSidedInventory::serialize(network::PacketSerializer& ser) const {
    // 先序列化上半部分
    m_upper->serialize(ser);
    // 再序列化下半部分
    m_lower->serialize(ser);
}

bool DoubleSidedInventory::isPartOfLargeChest(const IInventory* inventory) const {
    return inventory == m_upper || inventory == m_lower;
}

bool DoubleSidedInventory::getContainerAndSlot(i32 globalSlot,
                                               IInventory** outContainer,
                                               i32& outLocalSlot) const {
    const i32 upperSize = m_upper->getContainerSize();

    if (globalSlot < 0) {
        return false;
    }

    if (globalSlot < upperSize) {
        // 在上半部分
        *outContainer = m_upper;
        outLocalSlot = globalSlot;
        return true;
    }

    // 在下半部分
    const i32 lowerSlot = globalSlot - upperSize;
    if (lowerSlot < m_lower->getContainerSize()) {
        *outContainer = m_lower;
        outLocalSlot = lowerSlot;
        return true;
    }

    return false;
}

i32 DoubleSidedInventory::getFirstEmptySlot() const {
    // 先检查上半部分
    for (i32 i = 0; i < m_upper->getContainerSize(); ++i) {
        if (m_upper->getItem(i).isEmpty()) {
            return i;
        }
    }

    // 再检查下半部分
    const i32 upperSize = m_upper->getContainerSize();
    for (i32 i = 0; i < m_lower->getContainerSize(); ++i) {
        if (m_lower->getItem(i).isEmpty()) {
            return upperSize + i;
        }
    }

    return -1;
}

i32 DoubleSidedInventory::countItem(const Item& item) const {
    return m_upper->countItem(item) + m_lower->countItem(item);
}

bool DoubleSidedInventory::hasItem(const Item& item) const {
    return m_upper->hasItem(item) || m_lower->hasItem(item);
}

} // namespace blockentity
} // namespace mc
