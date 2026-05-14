#include "world/blockentity/processing/FurnaceInventory.hpp"
#include "network/packet/PacketSerializer.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

FurnaceInventory::FurnaceInventory()
    : m_items{}
{
    // 所有槽位初始化为空物品堆
}

FurnaceInventory::FurnaceInventory(std::function<void()> onChanged)
    : m_items{}
    , m_onChanged(std::move(onChanged))
{}

bool FurnaceInventory::isEmpty() const
{
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack FurnaceInventory::getItem(i32 slot) const
{
    if (!isValidSlot(slot)) {
        return ItemStack();
    }
    return m_items[static_cast<std::size_t>(slot)];
}

void FurnaceInventory::setItem(i32 slot, const ItemStack& stack)
{
    MC_ASSERT(isValidSlot(slot) && "Slot index out of bounds");
    m_items[static_cast<std::size_t>(slot)] = stack;
    onChanged();
}

ItemStack FurnaceInventory::removeItem(i32 slot, i32 count)
{
    if (!isValidSlot(slot) || count <= 0) {
        return ItemStack();
    }

    ItemStack& stack = m_items[static_cast<std::size_t>(slot)];
    if (stack.isEmpty()) {
        return ItemStack();
    }

    const i32 actualCount = std::min(count, stack.getCount());
    ItemStack result = stack.split(actualCount);

    if (stack.isEmpty()) {
        m_items[static_cast<std::size_t>(slot)] = ItemStack();
    }

    onChanged();
    return result;
}

ItemStack FurnaceInventory::removeItemNoUpdate(i32 slot)
{
    if (!isValidSlot(slot)) {
        return ItemStack();
    }

    const std::size_t slotIndex = static_cast<std::size_t>(slot);
    ItemStack result = std::move(m_items[slotIndex]);
    m_items[slotIndex] = ItemStack();
    return result;
}

void FurnaceInventory::clear()
{
    for (auto& item : m_items) {
        item = ItemStack();
    }
    onChanged();
}

void FurnaceInventory::setChanged()
{
    onChanged();
}

bool FurnaceInventory::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    if (!isValidSlot(slot) || stack.isEmpty()) {
        return false;
    }

    if (slot == SLOT_OUTPUT) {
        return false;
    }

    const ItemStack& existing = m_items[static_cast<std::size_t>(slot)];
    if (existing.isEmpty()) {
        return true;
    }

    if (!existing.canStackWith(stack)) {
        return false;
    }

    const i32 maxCount = existing.getMaxStackSize();
    return existing.getCount() + stack.getCount() <= maxCount;
}

void FurnaceInventory::serialize(network::PacketSerializer& ser) const
{
    ser.writeVarInt(SLOT_COUNT);
    for (const auto& item : m_items) {
        item.serialize(ser);
    }
}

ItemStack FurnaceInventory::addToOutput(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return ItemStack();
    }

    ItemStack& output = m_items[static_cast<std::size_t>(SLOT_OUTPUT)];

    if (output.isEmpty()) {
        // 输出槽为空，直接放入
        output = stack;
        onChanged();
        return ItemStack();
    }

    // 检查是否可以堆叠
    if (output.canStackWith(stack)) {
        const i32 maxCount = output.getMaxStackSize();
        const i32 space = maxCount - output.getCount();

        if (space > 0) {
            const i32 toAdd = std::min(space, stack.getCount());
            output.grow(toAdd);

            ItemStack remaining = stack;
            remaining.shrink(toAdd);
            onChanged();
            return remaining;
        }
    }

    // 无法堆叠，返回原物品
    return stack;
}

bool FurnaceInventory::canAcceptOutput(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return true;
    }

    const ItemStack& output = m_items[static_cast<std::size_t>(SLOT_OUTPUT)];

    if (output.isEmpty()) {
        return true;
    }

    if (!output.canStackWith(stack)) {
        return false;
    }

    const i32 maxCount = output.getMaxStackSize();
    return output.getCount() + stack.getCount() <= maxCount;
}

void FurnaceInventory::onChanged()
{
    if (m_onChanged) {
        m_onChanged();
    }
}

} // namespace blockentity
} // namespace mc
