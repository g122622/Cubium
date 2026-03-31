#include <gtest/gtest.h>
#include "../src/common/entity/inventory/Container.hpp"
#include "../src/common/entity/inventory/PlayerInventory.hpp"
#include "../src/common/item/core/ItemRegistry.hpp"
#include "../src/common/item/Items.hpp"
#include "../src/common/network/packet/InventoryPackets.hpp"

using namespace mc;

// ============================================================================
// Container 测试
// ============================================================================

class ContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_iron = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
        m_stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    }

    Item* m_diamond = nullptr;
    Item* m_iron = nullptr;
    Item* m_stick = nullptr;
};

TEST_F(ContainerTest, Creation) {
    Container container(ContainerType::Player, 0);

    EXPECT_EQ(container.type(), ContainerType::Player);
    EXPECT_EQ(container.id(), 0);
    EXPECT_EQ(container.getSlotCount(), 0);
    EXPECT_FALSE(container.hasChanged());
}

TEST_F(ContainerTest, AddSlot) {
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    Container container(ContainerType::Player, 0);

    auto slot = std::make_unique<Slot>(&inventory, 0, 10, 10);
    i32 index = container.addSlot(std::move(slot));

    EXPECT_EQ(index, 0);
    EXPECT_EQ(container.getSlotCount(), 1);
}

TEST_F(ContainerTest, AddInventorySlots) {
    PlayerInventory inventory;
    Container container(ContainerType::Player, 0);

    SlotRange range = container.addInventorySlots(&inventory, 0, 9, 0, 0);

    EXPECT_EQ(range.start, 0);
    EXPECT_EQ(range.end, 9);
    EXPECT_EQ(range.size(), 9);
    EXPECT_EQ(container.getSlotCount(), 9);
}

TEST_F(ContainerTest, GetSlot) {
    PlayerInventory inventory;
    Container container(ContainerType::Player, 0);

    container.addInventorySlots(&inventory, 0, 3, 0, 0);

    Slot* slot = container.getSlot(0);
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->getIndex(), 0);

    Slot* invalidSlot = container.getSlot(-1);
    EXPECT_EQ(invalidSlot, nullptr);

    Slot* outOfRangeSlot = container.getSlot(10);
    EXPECT_EQ(outOfRangeSlot, nullptr);
}

TEST_F(ContainerTest, GetSlotItem) {
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));

    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 3, 0, 0);

    ItemStack item = container.getSlotItem(0);
    EXPECT_FALSE(item.isEmpty());
    EXPECT_EQ(item.getItem(), m_diamond);
    EXPECT_EQ(item.getCount(), 10);

    ItemStack emptyItem = container.getSlotItem(100);
    EXPECT_TRUE(emptyItem.isEmpty());
}

TEST_F(ContainerTest, MergeItem) {
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 9, 0, 0);

    // 在槽位0放入5个钻石
    inventory.setItem(0, ItemStack(*m_diamond, 5));

    // 合并更多钻石 - 范围从0开始以合并到槽位0
    ItemStack stack(*m_diamond, 10);
    bool merged = container.mergeItem(stack, 0, 9, false);

    EXPECT_TRUE(merged);
    // 应该合并到槽位0
    EXPECT_EQ(inventory.getItem(0).getCount(), 15);
}

TEST_F(ContainerTest, MergeItemToEmptySlot) {
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 9, 0, 0);

    // 合并到空槽位
    ItemStack stack(*m_diamond, 10);
    bool merged = container.mergeItem(stack, 0, 9, false);

    EXPECT_TRUE(merged);
    EXPECT_EQ(inventory.getItem(0).getCount(), 10);
    EXPECT_TRUE(stack.isEmpty());
}

TEST_F(ContainerTest, ClickPickLeft) {
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));

    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 9, 0, 0);

    // 左键拾取
    ItemStack cursor = ItemStack::EMPTY;
    cursor = container.clicked(0, 0, ClickType::Pick, cursor);

    EXPECT_FALSE(cursor.isEmpty());
    EXPECT_EQ(cursor.getCount(), 10);
    EXPECT_TRUE(container.getSlotItem(0).isEmpty());
}

TEST_F(ContainerTest, ClickPickRight) {
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));

    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 9, 0, 0);

    // 右键拾取一半
    ItemStack cursor = ItemStack::EMPTY;
    cursor = container.clicked(0, 1, ClickType::PickAll, cursor);

    EXPECT_FALSE(cursor.isEmpty());
    EXPECT_EQ(cursor.getCount(), 5);
    EXPECT_EQ(container.getSlotItem(0).getCount(), 5);
}

TEST_F(ContainerTest, ClickPlaceLeft) {
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 9, 0, 0);

    // 左键放置
    ItemStack cursor(*m_diamond, 10);
    cursor = container.clicked(0, 0, ClickType::Pick, cursor);

    EXPECT_TRUE(cursor.isEmpty());
    EXPECT_EQ(container.getSlotItem(0).getCount(), 10);
}

TEST_F(ContainerTest, ClickPlaceRight) {
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 9, 0, 0);

    // 右键放置一个
    ItemStack cursor(*m_diamond, 10);
    cursor = container.clicked(0, 1, ClickType::PickAll, cursor);

    EXPECT_EQ(cursor.getCount(), 9);
    EXPECT_EQ(container.getSlotItem(0).getCount(), 1);
}

TEST_F(ContainerTest, ClickSwap) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_iron, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));
    inventory.setItem(1, ItemStack(*m_iron, 5));

    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 9, 0, 0);

    // 用钻石交换铁锭
    ItemStack cursor(*m_diamond, 10);
    cursor = container.clicked(1, 0, ClickType::Pick, cursor);

    EXPECT_EQ(cursor.getItem(), m_iron);
    EXPECT_EQ(cursor.getCount(), 5);
    EXPECT_EQ(container.getSlotItem(1).getItem(), m_diamond);
    EXPECT_EQ(container.getSlotItem(1).getCount(), 10);
}

TEST_F(ContainerTest, SlotRange) {
    Container container(ContainerType::Player, 0);

    container.setPlayerInventoryRange(0, 36);
    container.setContainerInventoryRange(36, 41);

    EXPECT_TRUE(container.playerInventoryRange().contains(0));
    EXPECT_TRUE(container.playerInventoryRange().contains(35));
    EXPECT_FALSE(container.playerInventoryRange().contains(36));

    EXPECT_TRUE(container.containerInventoryRange().contains(36));
    EXPECT_TRUE(container.containerInventoryRange().contains(40));
    EXPECT_FALSE(container.containerInventoryRange().contains(35));
}

TEST_F(ContainerTest, GetAllSlots) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_iron, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));
    inventory.setItem(1, ItemStack(*m_iron, 5));

    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 3, 0, 0);

    std::vector<ItemStack> items = container.getAllSlots();

    EXPECT_EQ(items.size(), 3);
    EXPECT_EQ(items[0].getItem(), m_diamond);
    EXPECT_EQ(items[0].getCount(), 10);
    EXPECT_EQ(items[1].getItem(), m_iron);
    EXPECT_EQ(items[1].getCount(), 5);
    EXPECT_TRUE(items[2].isEmpty());
}

TEST_F(ContainerTest, SetAllSlots) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_iron, nullptr);

    PlayerInventory inventory;
    Container container(ContainerType::Player, 0);
    container.addInventorySlots(&inventory, 0, 3, 0, 0);

    std::vector<ItemStack> items;
    items.emplace_back(*m_diamond, 10);
    items.emplace_back(*m_iron, 5);
    items.emplace_back(*m_diamond, 3);

    container.setAllSlots(items);

    EXPECT_EQ(container.getSlotItem(0).getCount(), 10);
    EXPECT_EQ(container.getSlotItem(1).getCount(), 5);
    EXPECT_EQ(container.getSlotItem(2).getCount(), 3);
    EXPECT_TRUE(container.hasChanged());
}

// ============================================================================
// PlayerContainer 测试
// ============================================================================

class PlayerContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    Item* m_diamond = nullptr;
};

TEST_F(PlayerContainerTest, Creation) {
    PlayerInventory playerInventory;
    PlayerContainer container(&playerInventory);

    EXPECT_EQ(container.type(), ContainerType::Player);
    EXPECT_EQ(container.id(), 0);
    // 41 slots: 9 hotbar + 27 main + 4 armor + 1 offhand
    EXPECT_EQ(container.getSlotCount(), 41);
}

TEST_F(PlayerContainerTest, HotbarSlots) {
    PlayerInventory playerInventory;
    PlayerContainer container(&playerInventory);

    // 快捷栏槽位应该在0-8
    for (int i = 0; i < 9; ++i) {
        Slot* slot = container.getSlot(i);
        ASSERT_NE(slot, nullptr);
        EXPECT_EQ(slot->getIndex(), i);
    }
}

TEST_F(PlayerContainerTest, PlayerInventoryRange) {
    PlayerInventory playerInventory;
    PlayerContainer container(&playerInventory);

    const SlotRange& playerRange = container.playerInventoryRange();
    EXPECT_EQ(playerRange.start, 0);
    EXPECT_EQ(playerRange.end, 36);  // hotbar + main
}

// ============================================================================
// 容器包测试
// ============================================================================

class ContainerPacketTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_iron = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    }

    Item* m_diamond = nullptr;
    Item* m_iron = nullptr;
};

TEST_F(ContainerPacketTest, ContainerContentPacket) {
    std::vector<ItemStack> items;
    items.emplace_back(*m_diamond, 10);
    items.emplace_back(*m_iron, 5);
    items.emplace_back(ItemStack::EMPTY);

    ContainerContentPacket packet(1, std::move(items));

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerContentPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    ContainerContentPacket decoded = result.value();

    EXPECT_EQ(decoded.containerId(), 1);
    EXPECT_EQ(decoded.size(), 3);
    EXPECT_EQ(decoded.items()[0].getItem(), m_diamond);
    EXPECT_EQ(decoded.items()[0].getCount(), 10);
    EXPECT_EQ(decoded.items()[1].getItem(), m_iron);
    EXPECT_EQ(decoded.items()[1].getCount(), 5);
    EXPECT_TRUE(decoded.items()[2].isEmpty());
}

TEST_F(ContainerPacketTest, ContainerSlotPacket) {
    ItemStack item(*m_diamond, 32);
    ContainerSlotPacket packet(2, 5, item);

    EXPECT_EQ(packet.containerId(), 2);
    EXPECT_EQ(packet.slotIndex(), 5);
    EXPECT_EQ(packet.item().getItem(), m_diamond);
    EXPECT_EQ(packet.item().getCount(), 32);

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerSlotPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    ContainerSlotPacket decoded = result.value();

    EXPECT_EQ(decoded.containerId(), 2);
    EXPECT_EQ(decoded.slotIndex(), 5);
    EXPECT_EQ(decoded.item().getItem(), m_diamond);
    EXPECT_EQ(decoded.item().getCount(), 32);
}

TEST_F(ContainerPacketTest, ContainerClickPacket) {
    ItemStack cursor(*m_iron, 64);
    ContainerClickPacket packet(3, 10, 0, ClickAction::Pick, cursor);

    EXPECT_EQ(packet.containerId(), 3);
    EXPECT_EQ(packet.slotIndex(), 10);
    EXPECT_EQ(packet.button(), 0);
    EXPECT_EQ(packet.action(), ClickAction::Pick);
    EXPECT_EQ(packet.cursorItem().getItem(), m_iron);

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerClickPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    ContainerClickPacket decoded = result.value();

    EXPECT_EQ(decoded.containerId(), 3);
    EXPECT_EQ(decoded.slotIndex(), 10);
    EXPECT_EQ(decoded.button(), 0);
    EXPECT_EQ(decoded.action(), ClickAction::Pick);
    EXPECT_EQ(decoded.cursorItem().getItem(), m_iron);
    EXPECT_EQ(decoded.cursorItem().getCount(), 64);
}

TEST_F(ContainerPacketTest, CloseContainerPacket) {
    CloseContainerPacket packet(5);

    EXPECT_EQ(packet.containerId(), 5);

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = CloseContainerPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_EQ(result.value().containerId(), 5);
}

TEST_F(ContainerPacketTest, OpenContainerPacket) {
    OpenContainerPacket packet(1, 2, "Chest", 27);

    EXPECT_EQ(packet.containerId(), 1);
    EXPECT_EQ(packet.type(), 2);
    EXPECT_EQ(packet.title(), "Chest");
    EXPECT_EQ(packet.slotCount(), 27);

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = OpenContainerPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    OpenContainerPacket decoded = result.value();

    EXPECT_EQ(decoded.containerId(), 1);
    EXPECT_EQ(decoded.type(), 2);
    EXPECT_EQ(decoded.title(), "Chest");
    EXPECT_EQ(decoded.slotCount(), 27);
}

TEST_F(ContainerPacketTest, HotbarSelectPacket) {
    HotbarSelectPacket packet(5);

    EXPECT_EQ(packet.slot(), 5);

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = HotbarSelectPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_EQ(result.value().slot(), 5);
}

TEST_F(ContainerPacketTest, HotbarSelectPacketInvalidSlot) {
    // 测试无效槽位
    HotbarSelectPacket packet(10);  // 超出范围

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = HotbarSelectPacket::deserialize(deser);

    // 应该失败
    EXPECT_FALSE(result.success());
}

TEST_F(ContainerPacketTest, HotbarSetPacket) {
    HotbarSetPacket packet(3);

    EXPECT_EQ(packet.slot(), 3);

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = HotbarSetPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_EQ(result.value().slot(), 3);
}

TEST_F(ContainerPacketTest, PlayerInventoryPacket) {
    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));
    inventory.setItem(20, ItemStack(*m_iron, 32));
    inventory.setSelectedSlot(5);

    PlayerInventoryPacket packet(inventory);

    EXPECT_EQ(packet.selectedSlot(), 5);
    EXPECT_EQ(packet.items().size(), 41);

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = PlayerInventoryPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    PlayerInventoryPacket decoded = result.value();

    EXPECT_EQ(decoded.selectedSlot(), 5);
    EXPECT_EQ(decoded.items().size(), 41);
    EXPECT_EQ(decoded.items()[0].getItem(), m_diamond);
    EXPECT_EQ(decoded.items()[0].getCount(), 10);
    EXPECT_EQ(decoded.items()[20].getItem(), m_iron);
    EXPECT_EQ(decoded.items()[20].getCount(), 32);
}

// ============================================================================
// Container 序列化测试
// ============================================================================

TEST_F(ContainerTest, SerializeDeserialize) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_iron, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));
    inventory.setItem(1, ItemStack(*m_iron, 5));

    Container container(ContainerType::Chest, 3);
    container.addInventorySlots(&inventory, 0, 9, 0, 0);
    container.setPlayerInventoryRange(0, 9);
    container.setContainerInventoryRange(9, 18);

    // 序列化
    network::PacketSerializer ser;
    container.serialize(ser);

    // 注意：反序列化创建的是一个空容器（槽位需要重新添加）
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = Container::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
}
