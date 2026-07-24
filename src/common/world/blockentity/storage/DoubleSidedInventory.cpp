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

#include "world/blockentity/storage/DoubleSidedInventory.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

DoubleSidedInventory::DoubleSidedInventory(::mc::IInventory* upper, ::mc::IInventory* lower)
    : m_upper(upper)
    , m_lower(lower)
{
    MC_ASSERT(upper != nullptr && "Upper inventory cannot be null");
    MC_ASSERT(lower != nullptr && "Lower inventory cannot be null");
}

i32 DoubleSidedInventory::getContainerSize() const
{
    return m_upper->getContainerSize() + m_lower->getContainerSize();
}

bool DoubleSidedInventory::isEmpty() const
{
    return m_upper->isEmpty() && m_lower->isEmpty();
}

i32 DoubleSidedInventory::getMaxStackSize() const
{
    return std::min(m_upper->getMaxStackSize(), m_lower->getMaxStackSize());
}

ItemStack DoubleSidedInventory::getItem(i32 slot) const
{
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        return container->getItem(localSlot);
    }
    return ItemStack();
}

void DoubleSidedInventory::setItem(i32 slot, const ItemStack& stack)
{
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        container->setItem(localSlot, stack);
    }
}

ItemStack DoubleSidedInventory::removeItem(i32 slot, i32 count)
{
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        return container->removeItem(localSlot, count);
    }
    return ItemStack();
}

ItemStack DoubleSidedInventory::removeItemNoUpdate(i32 slot)
{
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        return container->removeItemNoUpdate(localSlot);
    }
    return ItemStack();
}

void DoubleSidedInventory::clear()
{
    m_upper->clear();
    m_lower->clear();
}

void DoubleSidedInventory::setChanged()
{
    m_upper->setChanged();
    m_lower->setChanged();
}

bool DoubleSidedInventory::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    IInventory* container = nullptr;
    i32 localSlot = 0;

    if (getContainerAndSlot(slot, &container, localSlot)) {
        return container->canPlaceItem(localSlot, stack);
    }
    return false;
}

bool DoubleSidedInventory::isPartOfLargeChest(const IInventory* inventory) const
{
    return inventory == m_upper || inventory == m_lower;
}

bool DoubleSidedInventory::getContainerAndSlot(i32 globalSlot, IInventory** outContainer, i32& outLocalSlot) const
{
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

i32 DoubleSidedInventory::getFirstEmptySlot() const
{
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

i32 DoubleSidedInventory::countItem(const Item& item) const
{
    return m_upper->countItem(item) + m_lower->countItem(item);
}

bool DoubleSidedInventory::hasItem(const Item& item) const
{
    return m_upper->hasItem(item) || m_lower->hasItem(item);
}

} // namespace blockentity
} // namespace mc
