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

void PlayerInventoryPacket::serialize(network::PacketSerializer& ser) const
{
    // 写入选中的快捷栏槽位
    ser.writeVarInt(m_selectedSlot);

    // 写入物品数量
    ser.writeVarUInt(static_cast<u32>(m_items.size()));

    // 写入每个槽位的物品
    for (const auto& item : m_items) {
        item.serialize(ser);
    }
}

Result<PlayerInventoryPacket> PlayerInventoryPacket::deserialize(network::PacketDeserializer& deser)
{
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

} // namespace mc
