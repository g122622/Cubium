#include "InventoryPackets.hpp"
#include "../../entity/inventory/PlayerInventory.hpp"

namespace mc {

// ============================================================================
// PlayerInventoryPacket 实现
// ============================================================================

PlayerInventoryPacket::PlayerInventoryPacket(const PlayerInventory& inventory)
    : m_selectedSlot(inventory.getSelectedSlot())
    , m_items(inventory::PLAYER_INVENTORY_SIZE)
{
    // 复制所有物品
    for (i32 i = 0; i < inventory::PLAYER_INVENTORY_SIZE; ++i) {
        m_items[i] = inventory.getItem(i);
    }
}

void PlayerInventoryPacket::serialize(network::PacketSerializer& ser) const {
    // 写入选中的快捷栏槽位
    ser.writeVarInt(m_selectedSlot);

    // 写入物品数量
    ser.writeVarUInt(static_cast<u32>(m_items.size()));

    // 写入每个槽位的物品
    for (const auto& item : m_items) {
        item.serialize(ser);
    }
}

Result<PlayerInventoryPacket> PlayerInventoryPacket::deserialize(network::PacketDeserializer& deser) {
    PlayerInventoryPacket packet;

    // 读入选中的快捷栏槽位
    auto slotResult = deser.readVarInt();
    if (slotResult.failed()) return slotResult.error();
    packet.m_selectedSlot = slotResult.value();

    // 验证槽位范围
    if (packet.m_selectedSlot < 0 || packet.m_selectedSlot > 8) {
        return Error(ErrorCode::InvalidData, "Invalid hotbar slot in PlayerInventoryPacket");
    }

    // 读取物品数量
    auto countResult = deser.readVarUInt();
    if (countResult.failed()) return countResult.error();
    u32 count = countResult.value();

    // 验证数量
    if (count > static_cast<u32>(inventory::PLAYER_INVENTORY_SIZE)) {
        return Error(ErrorCode::InvalidData, "Invalid inventory size in PlayerInventoryPacket");
    }

    // 读取每个槽位的物品
    packet.m_items.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        auto itemResult = ItemStack::deserialize(deser);
        if (itemResult.failed()) return itemResult.error();
        packet.m_items.push_back(itemResult.value());
    }

    return packet;
}

// ============================================================================
// CreativeInventoryActionPacket 实现
// ============================================================================

// 该包仅在头文件中实现序列化/反序列化逻辑。

} // namespace mc
