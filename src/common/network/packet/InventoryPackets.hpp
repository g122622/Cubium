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

#pragma once

#include "../../core/Constants.hpp"
#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include "../../entity/inventory/ContainerTypes.hpp"
#include "../../item/core/ItemStack.hpp"
#include "PacketSerializer.hpp"
#include <memory>
#include <vector>

namespace mc {

// Forward declarations
class PlayerInventory;

// ============================================================================
// 背包相关协议常量
// ============================================================================

namespace inventory {
// 最大槽位数
constexpr i32 MAX_SLOTS = 256;

// 最大物品堆叠数（与物品默认最大堆叠数一致）
constexpr i32 MAX_STACK_SIZE = mc::item::DEFAULT_MAX_STACK_SIZE;

// 玩家背包槽位数
constexpr i32 PLAYER_INVENTORY_SIZE = 41;

// 玩家容器ID
constexpr ContainerId PLAYER_CONTAINER_ID = 0;
} // namespace inventory

// ============================================================================
// 容器内容同步包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 容器内容同步包
 *
 * 同步整个容器的所有槽位内容 + 玩家光标携带物品（carried）。
 * 参考: MC 1.16.5 SPacketWindowItems（末尾携带 carried ItemStack）。
 *
 * carried 字段用于服务端→客户端同步点击后的光标物品（拾取/创造 clone），
 * 修复此前客户端 m_carried 从不回传导致光标显示为空的缺陷。
 */
class ContainerContentPacket {
public:
    ContainerContentPacket() = default;

    /**
     * @brief 构造容器内容包
     * @param containerId 容器ID
     * @param items 槽位物品列表
     * @param carriedItem 玩家光标携带物品（末尾同步，默认空堆）
     */
    ContainerContentPacket(ContainerId containerId, std::vector<ItemStack> items, ItemStack carriedItem = {})
        : m_containerId(containerId)
        , m_items(std::move(items))
        , m_carriedItem(std::move(carriedItem))
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] const std::vector<ItemStack>& items() const { return m_items; }
    [[nodiscard]] const ItemStack& carriedItem() const { return m_carriedItem; }
    [[nodiscard]] size_t size() const { return m_items.size(); }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setItems(std::vector<ItemStack> items) { m_items = std::move(items); }
    void setCarriedItem(ItemStack item) { m_carriedItem = std::move(item); }

    // 序列化
    void serialize(network::PacketSerializer& ser) const
    {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        // MC 1.16.5: count 是 i16 (short)，不是 VarUInt
        ser.writeI16(static_cast<i16>(m_items.size()));

        for (const auto& item : m_items) {
            item.serialize(ser);
        }

        // 末尾携带光标物品（对齐 SPacketWindowItems）
        m_carriedItem.serialize(ser);
    }

    // 反序列化
    [[nodiscard]] static Result<ContainerContentPacket> deserialize(network::PacketDeserializer& deser)
    {
        ContainerContentPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        // MC 1.16.5: count 是 i16 (short)
        auto countResult = deser.readI16();
        if (countResult.failed()) return countResult.error();
        i16 count = countResult.value();

        if (count < 0 || count > static_cast<i16>(inventory::MAX_SLOTS)) {
            return Error(ErrorCode::InvalidData, "Invalid slot count in container content packet");
        }

        packet.m_items.reserve(static_cast<size_t>(count));
        for (i16 i = 0; i < count; ++i) {
            auto itemResult = ItemStack::deserialize(deser);
            if (itemResult.failed()) return itemResult.error();
            packet.m_items.push_back(itemResult.value());
        }

        // 末尾光标物品（对齐 SPacketWindowItems）
        auto carriedResult = ItemStack::deserialize(deser);
        if (carriedResult.failed()) return carriedResult.error();
        packet.m_carriedItem = carriedResult.value();

        return packet;
    }

private:
    ContainerId m_containerId = 0;
    std::vector<ItemStack> m_items;
    ItemStack m_carriedItem;
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

    // 序列化 (参考 MC 1.16.5 SPacketSetSlot)
    void serialize(network::PacketSerializer& ser) const
    {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        ser.writeI16(static_cast<i16>(m_slotIndex));
        m_item.serialize(ser);
    }

    // 反序列化
    [[nodiscard]] static Result<ContainerSlotPacket> deserialize(network::PacketDeserializer& deser)
    {
        ContainerSlotPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto slotResult = deser.readI16();
        if (slotResult.failed()) return slotResult.error();
        packet.m_slotIndex = static_cast<i32>(slotResult.value());

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
 * 协议格式: windowId(u8) + slot(i16) + button(u8) + transactionId(i16) + mode(u8) + item
 */
class ContainerClickPacket {
public:
    ContainerClickPacket() = default;

    /**
     * @brief 构造点击包
     * @param containerId 容器ID
     * @param slotIndex 槽位索引
     * @param button 按钮 (0=左键, 1=右键)
     * @param transactionId 事务ID (用于防重放)
     * @param action 点击类型
     * @param cursorItem 点击后的鼠标物品
     */
    ContainerClickPacket(
        ContainerId containerId, i32 slotIndex, i32 button, i16 transactionId, ClickAction action, ItemStack cursorItem)
        : m_containerId(containerId)
        , m_slotIndex(slotIndex)
        , m_button(button)
        , m_transactionId(transactionId)
        , m_action(action)
        , m_cursorItem(std::move(cursorItem))
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] i32 slotIndex() const { return m_slotIndex; }
    [[nodiscard]] i32 button() const { return m_button; }
    [[nodiscard]] i16 transactionId() const { return m_transactionId; }
    [[nodiscard]] ClickAction action() const { return m_action; }
    [[nodiscard]] const ItemStack& cursorItem() const { return m_cursorItem; }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setSlotIndex(i32 index) { m_slotIndex = index; }
    void setButton(i32 button) { m_button = button; }
    void setTransactionId(i16 id) { m_transactionId = id; }
    void setAction(ClickAction action) { m_action = action; }
    void setCursorItem(ItemStack item) { m_cursorItem = std::move(item); }

    // 序列化
    void serialize(network::PacketSerializer& ser) const
    {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        ser.writeI16(static_cast<i16>(m_slotIndex));
        ser.writeU8(static_cast<u8>(m_button));
        ser.writeI16(m_transactionId);
        ser.writeU8(static_cast<u8>(m_action));
        m_cursorItem.serialize(ser);
    }

    // 反序列化
    [[nodiscard]] static Result<ContainerClickPacket> deserialize(network::PacketDeserializer& deser)
    {
        ContainerClickPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto slotResult = deser.readI16();
        if (slotResult.failed()) return slotResult.error();
        packet.m_slotIndex = static_cast<i32>(slotResult.value());

        auto buttonResult = deser.readU8();
        if (buttonResult.failed()) return buttonResult.error();
        packet.m_button = static_cast<i32>(buttonResult.value());

        auto transResult = deser.readI16();
        if (transResult.failed()) return transResult.error();
        packet.m_transactionId = transResult.value();

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
    i16 m_transactionId = 0;
    ClickAction m_action = ClickAction::Pickup;
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
    void serialize(network::PacketSerializer& ser) const { ser.writeU8(static_cast<ContainerIdU8>(m_containerId)); }

    // 反序列化
    [[nodiscard]] static Result<CloseContainerPacket> deserialize(network::PacketDeserializer& deser)
    {
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
// 打开玩家背包容器请求包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 请求服务端打开玩家背包容器（containerId = PLAYER_CONTAINER_ID）
 *
 * 玩家按 E 打开生存背包时，客户端本地构造 InventoryScreen，同时发送本包通知
 * 服务端在 containerId=0 上建立 InventoryCraftingMenu，使后续 ContainerClickPacket
 * 能被服务端正确受理（修复历史遗留：服务端此前对 containerId=0 无打开容器而
 * 静默丢弃点击）。本包无负载。
 */
class OpenPlayerInventoryPacket {
public:
    OpenPlayerInventoryPacket() = default;

    void serialize(network::PacketSerializer& ser) const { (void)ser; }

    [[nodiscard]] static Result<OpenPlayerInventoryPacket> deserialize(network::PacketDeserializer& deser)
    {
        (void)deser;
        return OpenPlayerInventoryPacket{};
    }
};

// ============================================================================
// 打开容器包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 打开容器包
 *
 * 服务端通知客户端打开一个容器窗口。
 * 参考: MC 1.16.5 SOpenWindowPacket
 * 协议格式: windowId(VarInt) + type(VarInt) + title(Component)
 */
class OpenContainerPacket {
public:
    OpenContainerPacket() = default;

    /**
     * @brief 构造打开容器包
     * @param containerId 容器ID
     * @param type 容器类型
     * @param title 容器标题
     */
    OpenContainerPacket(ContainerId containerId, i32 type, const std::string& title)
        : m_containerId(containerId)
        , m_type(type)
        , m_title(title)
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] i32 type() const { return m_type; }
    [[nodiscard]] const std::string& title() const { return m_title; }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setType(i32 type) { m_type = type; }
    void setTitle(const std::string& title) { m_title = title; }

    // 序列化 (MC 1.16.5: VarInt windowId, VarInt type, Component title)
    void serialize(network::PacketSerializer& ser) const
    {
        ser.writeVarInt(m_containerId);
        ser.writeVarInt(m_type);
        ser.writeString(m_title);
    }

    // 反序列化
    [[nodiscard]] static Result<OpenContainerPacket> deserialize(network::PacketDeserializer& deser)
    {
        OpenContainerPacket packet;

        auto idResult = deser.readVarInt();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto typeResult = deser.readVarInt();
        if (typeResult.failed()) return typeResult.error();
        packet.m_type = typeResult.value();

        auto titleResult = deser.readString();
        if (titleResult.failed()) return titleResult.error();
        packet.m_title = titleResult.value();

        return packet;
    }

private:
    ContainerId m_containerId = 0;
    i32 m_type = 0;
    std::string m_title;
};

// ============================================================================
// 快捷栏选择包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 快捷栏选择包
 *
 * 客户端通知服务端切换选中的快捷栏槽位。
 * 参考: MC 1.16.5 CPacketHeldItemChange
 * 协议格式: slotId(i16)
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

    // 序列化 (MC 1.16.5: slotId 是 i16)
    void serialize(network::PacketSerializer& ser) const { ser.writeI16(static_cast<i16>(m_slot)); }

    // 反序列化
    [[nodiscard]] static Result<HotbarSelectPacket> deserialize(network::PacketDeserializer& deser)
    {
        HotbarSelectPacket packet;

        auto slotResult = deser.readI16();
        if (slotResult.failed()) return slotResult.error();
        packet.m_slot = static_cast<i32>(slotResult.value());

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
    void serialize(network::PacketSerializer& ser) const { ser.writeU8(static_cast<u8>(m_slot)); }

    // 反序列化
    [[nodiscard]] static Result<HotbarSetPacket> deserialize(network::PacketDeserializer& deser)
    {
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

// ============================================================================
// 事务确认包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 服务端事务确认包
 *
 * 服务端确认或拒绝事务。
 * 参考: MC 1.16.5 SConfirmTransactionPacket
 * 协议格式: windowId(u8) + actionNumber(i16) + accepted(bool)
 */
class ServerConfirmTransactionPacket {
public:
    ServerConfirmTransactionPacket() = default;

    /**
     * @brief 构造事务确认包
     * @param containerId 容器ID
     * @param actionNumber 事务ID
     * @param accepted 是否接受
     */
    ServerConfirmTransactionPacket(ContainerId containerId, i16 actionNumber, bool accepted)
        : m_containerId(containerId)
        , m_actionNumber(actionNumber)
        , m_accepted(accepted)
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] i16 actionNumber() const { return m_actionNumber; }
    [[nodiscard]] bool accepted() const { return m_accepted; }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setActionNumber(i16 number) { m_actionNumber = number; }
    void setAccepted(bool accepted) { m_accepted = accepted; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const
    {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        ser.writeI16(m_actionNumber);
        ser.writeBool(m_accepted);
    }

    // 反序列化
    [[nodiscard]] static Result<ServerConfirmTransactionPacket> deserialize(network::PacketDeserializer& deser)
    {
        ServerConfirmTransactionPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto actionResult = deser.readI16();
        if (actionResult.failed()) return actionResult.error();
        packet.m_actionNumber = actionResult.value();

        auto acceptedResult = deser.readBool();
        if (acceptedResult.failed()) return acceptedResult.error();
        packet.m_accepted = acceptedResult.value();

        return packet;
    }

private:
    ContainerId m_containerId = 0;
    i16 m_actionNumber = 0;
    bool m_accepted = false;
};

// ============================================================================
// 事务确认包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 客户端事务确认包
 *
 * 客户端确认事务处理结果。
 * 参考: MC 1.16.5 CConfirmTransactionPacket
 * 协议格式: windowId(i8) + uid(i16) + accepted(u8)
 */
class ClientConfirmTransactionPacket {
public:
    ClientConfirmTransactionPacket() = default;

    /**
     * @brief 构造事务确认包
     * @param containerId 容器ID
     * @param uid 事务ID
     * @param accepted 是否接受
     */
    ClientConfirmTransactionPacket(i8 containerId, i16 uid, bool accepted)
        : m_containerId(containerId)
        , m_uid(uid)
        , m_accepted(accepted)
    {}

    // Getters
    [[nodiscard]] i8 containerId() const { return m_containerId; }
    [[nodiscard]] i16 uid() const { return m_uid; }
    [[nodiscard]] bool accepted() const { return m_accepted; }

    // Setters
    void setContainerId(i8 id) { m_containerId = id; }
    void setUid(i16 uid) { m_uid = uid; }
    void setAccepted(bool accepted) { m_accepted = accepted; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const
    {
        ser.writeI8(m_containerId);
        ser.writeI16(m_uid);
        ser.writeU8(m_accepted ? 1 : 0); // MC 1.16.5 使用 u8 而非 bool
    }

    // 反序列化
    [[nodiscard]] static Result<ClientConfirmTransactionPacket> deserialize(network::PacketDeserializer& deser)
    {
        ClientConfirmTransactionPacket packet;

        auto idResult = deser.readI8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto uidResult = deser.readI16();
        if (uidResult.failed()) return uidResult.error();
        packet.m_uid = uidResult.value();

        auto acceptedResult = deser.readU8();
        if (acceptedResult.failed()) return acceptedResult.error();
        packet.m_accepted = (acceptedResult.value() != 0);

        return packet;
    }

private:
    i8 m_containerId = 0;
    i16 m_uid = 0;
    bool m_accepted = false;
};

// ============================================================================
// 窗口属性包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 窗口属性包
 *
 * 同步容器属性（如熔炉燃烧进度、酿造台状态等）。
 * 参考: MC 1.16.5 SWindowPropertyPacket
 * 协议格式: windowId(u8) + property(i16) + value(i16)
 */
class WindowPropertyPacket {
public:
    WindowPropertyPacket() = default;

    /**
     * @brief 构造窗口属性包
     * @param containerId 容器ID
     * @param property 属性ID
     * @param value 属性值
     */
    WindowPropertyPacket(ContainerId containerId, i16 property, i16 value)
        : m_containerId(containerId)
        , m_property(property)
        , m_value(value)
    {}

    // Getters
    [[nodiscard]] ContainerId containerId() const { return m_containerId; }
    [[nodiscard]] i16 property() const { return m_property; }
    [[nodiscard]] i16 value() const { return m_value; }

    // Setters
    void setContainerId(ContainerId id) { m_containerId = id; }
    void setProperty(i16 property) { m_property = property; }
    void setValue(i16 value) { m_value = value; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const
    {
        ser.writeU8(static_cast<ContainerIdU8>(m_containerId));
        ser.writeI16(m_property);
        ser.writeI16(m_value);
    }

    // 反序列化
    [[nodiscard]] static Result<WindowPropertyPacket> deserialize(network::PacketDeserializer& deser)
    {
        WindowPropertyPacket packet;

        auto idResult = deser.readU8();
        if (idResult.failed()) return idResult.error();
        packet.m_containerId = idResult.value();

        auto propertyResult = deser.readI16();
        if (propertyResult.failed()) return propertyResult.error();
        packet.m_property = propertyResult.value();

        auto valueResult = deser.readI16();
        if (valueResult.failed()) return valueResult.error();
        packet.m_value = valueResult.value();

        return packet;
    }

private:
    ContainerId m_containerId = 0;
    i16 m_property = 0;
    i16 m_value = 0;
};

// ============================================================================
// 拾取物品包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 拾取物品包
 *
 * 玩家使用中键拾取方块时发送。
 * 参考: MC 1.16.5 CPickItemPacket
 * 协议格式: pickIndex(VarInt)
 */
class PickItemPacket {
public:
    PickItemPacket() = default;

    /**
     * @brief 构造拾取物品包
     * @param pickIndex 拾取索引（玩家背包中的槽位）
     */
    explicit PickItemPacket(i32 pickIndex)
        : m_pickIndex(pickIndex)
    {}

    // Getters
    [[nodiscard]] i32 pickIndex() const { return m_pickIndex; }

    // Setters
    void setPickIndex(i32 index) { m_pickIndex = index; }

    // 序列化
    void serialize(network::PacketSerializer& ser) const { ser.writeVarInt(m_pickIndex); }

    // 反序列化
    [[nodiscard]] static Result<PickItemPacket> deserialize(network::PacketDeserializer& deser)
    {
        PickItemPacket packet;

        auto indexResult = deser.readVarInt();
        if (indexResult.failed()) return indexResult.error();
        packet.m_pickIndex = indexResult.value();

        return packet;
    }

private:
    i32 m_pickIndex = 0;
};

} // namespace mc
