#include "SimpleInventory.hpp"
#include "network/packet/PacketSerializer.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

SimpleInventory::SimpleInventory(i32 size)
    : m_items(static_cast<std::size_t>(size))
{
    MC_ASSERT(size > 0 && "Inventory size must be positive");
}

SimpleInventory::SimpleInventory(i32 size, std::function<void()> onChanged)
    : m_items(static_cast<std::size_t>(size))
    , m_onChanged(std::move(onChanged))
{
    MC_ASSERT(size > 0 && "Inventory size must be positive");
}

bool SimpleInventory::isEmpty() const
{
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack SimpleInventory::getItem(i32 slot) const
{
    if (!isValidSlot(slot)) {
        return ItemStack();
    }
    return m_items[static_cast<std::size_t>(slot)];
}

void SimpleInventory::setItem(i32 slot, const ItemStack& stack)
{
    MC_ASSERT(isValidSlot(slot) && "Slot index out of bounds");
    const std::size_t slotIndex = static_cast<std::size_t>(slot);

    // 只在物品实际变化时触发回调
    if (m_items[slotIndex] == stack) {
        return;
    }

    m_items[slotIndex] = stack;
    onChanged();
}

ItemStack SimpleInventory::removeItem(i32 slot, i32 count)
{
    if (!isValidSlot(slot) || count <= 0) {
        return ItemStack();
    }

    ItemStack& stack = m_items[static_cast<std::size_t>(slot)];
    if (stack.isEmpty()) {
        return ItemStack();
    }

    // 限制取出数量不超过当前堆叠数量
    const i32 actualCount = std::min(count, stack.getCount());
    ItemStack result = stack.split(actualCount);

    // 如果堆叠变空，清空槽位
    if (stack.isEmpty()) {
        m_items[static_cast<std::size_t>(slot)] = ItemStack();
    }

    onChanged();
    return result;
}

ItemStack SimpleInventory::removeItemNoUpdate(i32 slot)
{
    if (!isValidSlot(slot)) {
        return ItemStack();
    }

    const std::size_t slotIndex = static_cast<std::size_t>(slot);
    ItemStack result = std::move(m_items[slotIndex]);
    m_items[slotIndex] = ItemStack();
    return result;
}

void SimpleInventory::clear()
{
    for (auto& item : m_items) {
        item = ItemStack();
    }
    onChanged();
}

void SimpleInventory::setChanged()
{
    onChanged();
}

bool SimpleInventory::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    if (!isValidSlot(slot) || stack.isEmpty()) {
        return false;
    }

    const ItemStack& existing = m_items[static_cast<std::size_t>(slot)];
    if (existing.isEmpty()) {
        return true;
    }

    // 检查是否可以堆叠
    if (!existing.canStackWith(stack)) {
        return false;
    }

    // 检查堆叠数量限制
    const i32 maxCount = std::min(m_maxStackSize, existing.getMaxStackSize());
    return existing.getCount() + stack.getCount() <= maxCount;
}

void SimpleInventory::serialize(network::PacketSerializer& ser) const
{
    ser.writeVarInt(static_cast<i32>(m_items.size()));
    for (const auto& item : m_items) {
        item.serialize(ser);
    }
}

ItemStack SimpleInventory::addItem(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return ItemStack();
    }

    ItemStack remaining = stack;

    // 首先尝试堆叠到已有物品
    for (std::size_t i = 0; i < m_items.size() && !remaining.isEmpty(); ++i) {
        ItemStack& existing = m_items[i];
        if (!existing.isEmpty() && existing.canStackWith(remaining)) {
            const i32 maxCount = std::min(m_maxStackSize, existing.getMaxStackSize());
            const i32 space = maxCount - existing.getCount();

            if (space > 0) {
                const i32 toAdd = std::min(space, remaining.getCount());
                existing.grow(toAdd);
                remaining.shrink(toAdd);
            }
        }
    }

    // 然后尝试放入空槽位
    for (std::size_t i = 0; i < m_items.size() && !remaining.isEmpty(); ++i) {
        if (m_items[i].isEmpty()) {
            const i32 maxCount = std::min(m_maxStackSize, remaining.getMaxStackSize());
            const i32 toAdd = std::min(maxCount, remaining.getCount());

            m_items[i] = remaining.split(toAdd);
        }
    }

    if (remaining.getCount() != stack.getCount()) {
        onChanged();
    }

    return remaining;
}

bool SimpleInventory::canAddItem(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return true;
    }

    i32 remaining = stack.getCount();

    // 检查已有物品的可堆叠空间
    for (std::size_t i = 0; i < m_items.size() && remaining > 0; ++i) {
        const ItemStack& existing = m_items[i];
        if (!existing.isEmpty() && existing.canStackWith(stack)) {
            const i32 maxCount = std::min(m_maxStackSize, existing.getMaxStackSize());
            const i32 space = maxCount - existing.getCount();
            remaining -= std::min(space, remaining);
        }
    }

    // 检查空槽位
    for (std::size_t i = 0; i < m_items.size() && remaining > 0; ++i) {
        if (m_items[i].isEmpty()) {
            const i32 maxCount = std::min(m_maxStackSize, stack.getMaxStackSize());
            remaining -= maxCount;
        }
    }

    return remaining <= 0;
}

ItemStack SimpleInventory::extractItem(i32 slot)
{
    return removeItemNoUpdate(slot);
}

bool SimpleInventory::isSlotEmpty(i32 slot) const
{
    if (!isValidSlot(slot)) {
        return true;
    }
    return m_items[static_cast<std::size_t>(slot)].isEmpty();
}

i32 SimpleInventory::getNonEmptySlotCount() const
{
    i32 count = 0;
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            ++count;
        }
    }
    return count;
}

void SimpleInventory::forEachItem(std::function<bool(i32 slot, const ItemStack& stack)> consumer) const
{
    for (std::size_t i = 0; i < m_items.size(); ++i) {
        if (!m_items[i].isEmpty()) {
            if (!consumer(static_cast<i32>(i), m_items[i])) {
                break;
            }
        }
    }
}

void SimpleInventory::onChanged()
{
    if (m_onChanged) {
        m_onChanged();
    }
}

void SimpleInventory::load(const nlohmann::json& data)
{
    if (!data.is_array()) {
        return;
    }

    for (std::size_t i = 0; i < m_items.size(); ++i) {
        m_items[i] = ItemStack();
    }

    for (std::size_t i = 0; i < data.size() && i < m_items.size(); ++i) {
        const auto& itemJson = data[i];
        if (!itemJson.is_object() || itemJson.empty()) {
            continue;
        }

        auto stackResult = ItemStack::fromJson(itemJson);
        if (stackResult.success()) {
            m_items[i] = stackResult.value();
        }
    }
}

void SimpleInventory::save(nlohmann::json& data) const
{
    data = nlohmann::json::array();
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (!m_items[i].isEmpty()) {
            data.push_back(m_items[i].toJson());
        } else {
            data.push_back(nlohmann::json::object());
        }
    }
}

} // namespace blockentity
} // namespace mc
