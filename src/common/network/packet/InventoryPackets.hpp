#pragma once

#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include "PacketSerializer.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../entity/inventory/ContainerTypes.hpp"
#include <vector>
#include <memory>

namespace mc {

// Forward declarations
class PlayerInventory;

// ============================================================================
// 背包相关协议常量
// ============================================================================

namespace inventory {
    // 最大槽位数
    constexpr i32 MAX_SLOTS = 256;

    // 最大物品堆叠数
    constexpr i32 MAX_STACK_SIZE = 64;

    // 玩家背包槽位数
    constexpr i32 PLAYER_INVENTORY_SIZE = 41;

    // 玩家容器ID
    constexpr ContainerId PLAYER_CONTAINER_ID = 0;
}

// ============================================================================
// 容器内容同步包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 容器内容同步包
 *
 * 同步整个容器的所有槽位内容。
 * 参考: MC 1.16.5 SPacketWindowItems
 */
class ContainerContentPacket {
public:
    ContainerContentPacket() = default;

    /**
     * @brief 构造容器内容包
     * @param containerId 容器ID
     * @param items 槽位物品列表
     */
    ContainerContentPacket(ContainerId containerId, std::vector<ItemStack> items)
        : m_containerId(containerId)
        , m_items(std::move(items))
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] const std::vector<ItemStack>& items() const { return m_items; }
    [[nodiscard]] size_t size() const { return m_items.size(); }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setItems(std::vector<ItemStack> items) { m_items = std::move(items); }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        ser.writeVarUInt(static_cast<u32>(m_items.size()));

        for (const auto& item : m_items) {
            item.serialize(ser);
        }
    }

    // 反序列化
    [[nodiscard]] static Result<ContainerContentPacket> deserialize(network::PacketDeserializer& deser) {
        ContainerContentPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto countResult = deser.readVarUInt();
        if (countResult.failed()) return countResult.error();
        u32 count = countResult.value();

        if (count > static_cast<u32>(inventory::MAX_SLOTS)) {
            return Error(ErrorCode::InvalidData, "Too many slots in container content packet");
        }

        packet.m_items.reserve(count);
        for (u32 i = 0; i < count; ++i) {
            auto itemResult = ItemStack::deserialize(deser);
            if (itemResult.failed()) return itemResult.error();
            packet.m_items.push_back(itemResult.value());
        }

        return packet;
    }

private:
    ContainerId m_containerId = 0;
    std::vector<ItemStack> m_items;
};

// ============================================================================
// 单个槽位更新包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 槽位更新包
 *
 * 同步单个槽位的内容。
 * 参考: MC 1.16.5 SPacketSetSlot
 */
class ContainerSlotPacket {
public:
    ContainerSlotPacket() = default;

    /**
     * @brief 构造槽位更新包
     * @param containerId 容器ID
     * @param slotIndex 槽位索引
     * @param item 物品
     */
    ContainerSlotPacket(ContainerId containerId, i32 slotIndex, ItemStack item)
        : m_containerId(containerId)
        , m_slotIndex(slotIndex)
        , m_item(std::move(item))
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] i32 slotIndex() const { return m_slotIndex; }
    [[nodiscard]] const ItemStack& item() const { return m_item; }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setSlotIndex(i32 index) { m_slotIndex = index; }
    void setItem(ItemStack item) { m_item = std::move(item); }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        ser.writeVarInt(m_slotIndex);
        m_item.serialize(ser);
    }

    // 反序列化
    [[nodiscard]] static Result<ContainerSlotPacket> deserialize(network::PacketDeserializer& deser) {
        ContainerSlotPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto slotResult = deser.readVarInt();
        if (slotResult.failed()) return slotResult.error();
        packet.m_slotIndex = slotResult.value();

        auto itemResult = ItemStack::deserialize(deser);
        if (itemResult.failed()) return itemResult.error();
        packet.m_item = itemResult.value();

        return packet;
    }

private:
    ContainerId m_containerId = 0;
    i32 m_slotIndex = 0;
    ItemStack m_item;
};

// ============================================================================
// 玩家背包同步包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 玩家背包同步包
 *
 * 同步玩家的完整背包内容。
 * 使用专门的包以减少序列化开销。
 */
class PlayerInventoryPacket {
public:
    PlayerInventoryPacket() = default;

    /**
     * @brief 从玩家背包构造
     * @param inventory 玩家背包
     */
    explicit PlayerInventoryPacket(const PlayerInventory& inventory);

    // Getters
    [[nodiscard]] i32 selectedSlot() const { return m_selectedSlot; }
    [[nodiscard]] const std::vector<ItemStack>& items() const { return m_items; }

    // Setters
    void setSelectedSlot(i32 slot) { m_selectedSlot = slot; }
    void setItems(std::vector<ItemStack> items) { m_items = std::move(items); }

    // 序列化
    void serialize(network::PacketSerializer& ser) const;

    // 反序列化
    [[nodiscard]] static Result<PlayerInventoryPacket> deserialize(network::PacketDeserializer& deser);

private:
    i32 m_selectedSlot = 0;
    std::vector<ItemStack> m_items;
};

// ============================================================================
// 容器点击包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 容器点击包
 *
 * 客户端发送点击操作到服务端。
 * 参考: MC 1.16.5 CPacketClickWindow
 */
class ContainerClickPacket {
public:
    ContainerClickPacket() = default;

    /**
     * @brief 构造点击包
     * @param containerId 容器ID
     * @param slotIndex 槽位索引
     * @param button 按钮 (0=左键, 1=右键)
     * @param action 点击类型
     * @param cursorItem 点击后的鼠标物品
     */
    ContainerClickPacket(ContainerId containerId, i32 slotIndex, i32 button,
                         ClickAction action, ItemStack cursorItem)
        : m_containerId(containerId)
        , m_slotIndex(slotIndex)
        , m_button(button)
        , m_action(action)
        , m_cursorItem(std::move(cursorItem))
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] i32 slotIndex() const { return m_slotIndex; }
    [[nodiscard]] i32 button() const { return m_button; }
    [[nodiscard]] ClickAction action() const { return m_action; }
    [[nodiscard]] const ItemStack& cursorItem() const { return m_cursorItem; }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setSlotIndex(i32 index) { m_slotIndex = index; }
    void setButton(i32 button) { m_button = button; }
    void setAction(ClickAction action) { m_action = action; }
    void setCursorItem(ItemStack item) { m_cursorItem = std::move(item); }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        ser.writeVarInt(m_slotIndex);
        ser.writeU8(static_cast<u8>(m_button));
        ser.writeU8(static_cast<u8>(m_action));
        m_cursorItem.serialize(ser);
    }

    // 反序列化
    [[nodiscard]] static Result<ContainerClickPacket> deserialize(network::PacketDeserializer& deser) {
        ContainerClickPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto slotResult = deser.readVarInt();
        if (slotResult.failed()) return slotResult.error();
        packet.m_slotIndex = slotResult.value();

        auto buttonResult = deser.readU8();
        if (buttonResult.failed()) return buttonResult.error();
        packet.m_button = static_cast<i32>(buttonResult.value());

        auto actionResult = deser.readU8();
        if (actionResult.failed()) return actionResult.error();
        packet.m_action = static_cast<ClickAction>(actionResult.value());

        auto itemResult = ItemStack::deserialize(deser);
        if (itemResult.failed()) return itemResult.error();
        packet.m_cursorItem = itemResult.value();

        return packet;
    }

private:
    ContainerId m_containerId = 0;
    i32 m_slotIndex = 0;
    i32 m_button = 0;
    ClickAction m_action = ClickAction::Pick;
    ItemStack m_cursorItem;
};

// ============================================================================
// 关闭容器包 (双向)
// ============================================================================

/**
 * @brief 关闭容器包
 *
 * 客户端或服务端都可以发送关闭容器。
 * 参考: MC 1.16.5 CPacketCloseWindow / SPacketCloseWindow
 */
class CloseContainerPacket {
public:
    CloseContainerPacket() = default;

    /**
     * @brief 构造关闭容器包
     * @param containerId 容器ID
     */
    explicit CloseContainerPacket(ContainerId containerId)
        : m_containerId(containerId)
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
    }

    // 反序列化
    [[nodiscard]] static Result<CloseContainerPacket> deserialize(network::PacketDeserializer& deser) {
        CloseContainerPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        return packet;
    }

private:
    ContainerId m_containerId = 0;
};

// ============================================================================
// 打开容器包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 打开容器包
 *
 * 服务端通知客户端打开一个容器窗口。
 */
class OpenContainerPacket {
public:
    OpenContainerPacket() = default;

    /**
     * @brief 构造打开容器包
     * @param containerId 容器ID
     * @param type 容器类型
     * @param title 容器标题
     * @param slotCount 槽位数量
     */
    OpenContainerPacket(ContainerId containerId, u8 type, const String& title, i32 slotCount)
        : m_containerId(containerId)
        , m_type(type)
        , m_title(title)
        , m_slotCount(slotCount)
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] u8 type() const { return m_type; }
    [[nodiscard]] const String& title() const { return m_title; }
    [[nodiscard]] i32 slotCount() const { return m_slotCount; }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setType(u8 type) { m_type = type; }
    void setTitle(const String& title) { m_title = title; }
    void setSlotCount(i32 count) { m_slotCount = count; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        ser.writeU8(m_type);
        ser.writeString(m_title);
        ser.writeVarInt(m_slotCount);
    }

    // 反序列化
    [[nodiscard]] static Result<OpenContainerPacket> deserialize(network::PacketDeserializer& deser) {
        OpenContainerPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto typeResult = deser.readU8();
        if (typeResult.failed()) return typeResult.error();
        packet.m_type = typeResult.value();

        auto titleResult = deser.readString();
        if (titleResult.failed()) return titleResult.error();
        packet.m_title = titleResult.value();

        auto countResult = deser.readVarInt();
        if (countResult.failed()) return countResult.error();
        packet.m_slotCount = countResult.value();

        return packet;
    }

private:
    ContainerId m_containerId = 0;
    u8 m_type = 0;
    String m_title;
    i32 m_slotCount = 0;
};

// ============================================================================
// 快捷栏选择包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 快捷栏选择包
 *
 * 客户端通知服务端切换选中的快捷栏槽位。
 * 参考: MC 1.16.5 CPacketHeldItemChange
 */
class HotbarSelectPacket {
public:
    HotbarSelectPacket() = default;

    /**
     * @brief 构造快捷栏选择包
     * @param slot 槽位索引 (0-8)
     */
    explicit HotbarSelectPacket(i32 slot)
        : m_slot(slot)
    {}

    // Getters
    [[nodiscard]] i32 slot() const { return m_slot; }

    // Setters
    void setSlot(i32 slot) { m_slot = slot; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeVarInt(m_slot);
    }

    // 反序列化
    [[nodiscard]] static Result<HotbarSelectPacket> deserialize(network::PacketDeserializer& deser) {
        HotbarSelectPacket packet;

        auto slotResult = deser.readVarInt();
        if (slotResult.failed()) return slotResult.error();
        packet.m_slot = slotResult.value();

        // 验证槽位范围
        if (packet.m_slot < 0 || packet.m_slot > 8) {
            return Error(ErrorCode::InvalidData, "Invalid hotbar slot");
        }

        return packet;
    }

private:
    i32 m_slot = 0;
};

// ============================================================================
// 快捷栏设置包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 快捷栏设置包
 *
 * 服务端通知客户端设置选中的快捷栏槽位。
 * 参考: MC 1.16.5 SPacketHeldItemChange
 */
class HotbarSetPacket {
public:
    HotbarSetPacket() = default;

    /**
     * @brief 构造快捷栏设置包
     * @param slot 槽位索引 (0-8)
     */
    explicit HotbarSetPacket(i32 slot)
        : m_slot(slot)
    {}

    // Getters
    [[nodiscard]] i32 slot() const { return m_slot; }

    // Setters
    void setSlot(i32 slot) { m_slot = slot; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const {
        ser.writeU8(static_cast<u8>(m_slot));
    }

    // 反序列化
    [[nodiscard]] static Result<HotbarSetPacket> deserialize(network::PacketDeserializer& deser) {
        HotbarSetPacket packet;

        auto slotResult = deser.readU8();
        if (slotResult.failed()) return slotResult.error();
        packet.m_slot = slotResult.value();

        // 验证槽位范围
        if (packet.m_slot > 8) {
            return Error(ErrorCode::InvalidData, "Invalid hotbar slot");
        }

        return packet;
    }

private:
    i32 m_slot = 0;
};

} // namespace mc
