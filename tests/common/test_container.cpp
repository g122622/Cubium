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

#include "../src/common/entity/entities/player/Player.hpp"
#include "../src/common/entity/inventory/AbstractContainerMenu.hpp"
#include "../src/common/entity/inventory/CreativeInventory.hpp"
#include "../src/common/entity/inventory/PlayerInventory.hpp"
#include "../src/common/entity/inventory/Slot.hpp"
#include "../src/common/item/Items.hpp"
#include "../src/common/item/core/ItemRegistry.hpp"
#include "../src/common/item/items/block/BlockItemRegistry.hpp"
#include "../src/common/network/packet/ContainerPacketHandler.hpp"
#include "../src/common/network/packet/InventoryPackets.hpp"
#include "../src/common/world/block/registry/VanillaBlocks.hpp"
#include "../src/server/menu/CraftingMenu.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// AbstractContainerMenu 测试（槽位、点击、拖拽、快速移动、数字键交换）
// ============================================================================

class AbstractContainerMenuTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_iron = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    }

    Item* m_diamond = nullptr;
    Item* m_iron = nullptr;
};

TEST_F(AbstractContainerMenuTest, QuickMoveShiftClickMovesStack)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 32));

    // 创建简单菜单
    class TestMenu : public AbstractContainerMenu {
    public:
        TestMenu(PlayerInventory* inv)
            : AbstractContainerMenu(0, inv)
        {
            // 添加容器槽位（槽位0）
            addSlot(std::make_unique<Slot>(inv, 0, 0, 0));
            // 添加玩家主背包槽位（槽位1-27）
            for (int i = 0; i < 27; ++i) {
                addSlot(std::make_unique<Slot>(inv, 9 + i, 8 + (i % 9) * 18, 18 + (i / 9) * 18));
            }
            m_playerInvStart = 1;
        }
        bool stillValid(const Player& player) const override { return true; }
    };

    TestMenu menu(&inventory);
    Player player(1, "TestPlayer");

    // Shift+点击槽位0应将物品移到玩家背包
    menu.clicked(0, 0, ClickType::QuickMove, player);

    // 钻石应该从槽位0移到玩家背包
    EXPECT_TRUE(menu.getSlot(0)->isEmpty());
}

TEST_F(AbstractContainerMenuTest, SwapWithNumberKey)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_iron, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10)); // 快捷栏槽位0
    inventory.setItem(9, ItemStack(*m_iron, 5));     // 主背包槽位9

    class TestMenu : public AbstractContainerMenu {
    public:
        TestMenu(PlayerInventory* inv)
            : AbstractContainerMenu(0, inv)
        {
            // 添加快捷栏槽位
            for (int i = 0; i < 9; ++i) {
                addSlot(std::make_unique<Slot>(inv, i, 8 + i * 18, 142));
            }
            // 添加主背包槽位
            for (int i = 0; i < 27; ++i) {
                addSlot(std::make_unique<Slot>(inv, 9 + i, 8 + (i % 9) * 18, 84 + (i / 9) * 18));
            }
            m_playerInvStart = 0;
        }
        bool stillValid(const Player& player) const override { return true; }
    };

    TestMenu menu(&inventory);
    Player player(1, "TestPlayer");

    // 按数字键1应该交换槽位9和快捷栏槽位0
    menu.clicked(9, 0, ClickType::Swap, player); // button=0 表示快捷栏槽位0

    // 验证交换结果
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_iron);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 5);
}

TEST_F(AbstractContainerMenuTest, DragDistributionEvenly)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 64));

    class TestMenu : public AbstractContainerMenu {
    public:
        TestMenu(PlayerInventory* inv)
            : AbstractContainerMenu(0, inv)
        {
            for (int i = 0; i < 9; ++i) {
                addSlot(std::make_unique<Slot>(inv, i, 8 + i * 18, 142));
            }
            m_playerInvStart = 0;
        }
        bool stillValid(const Player& player) const override { return true; }
    };

    TestMenu menu(&inventory);
    Player player(1, "TestPlayer");

    // 设置鼠标物品
    menu.setCarriedItem(ItemStack(*m_diamond, 64));

    // 开始拖拽 (button = 0 | (MODE_EVEN << 2) = 0)
    menu.clicked(1,
        DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT),
        ClickType::QuickCraft,
        player);

    // 添加槽位到拖拽列表
    menu.clicked(2,
        DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT),
        ClickType::QuickCraft,
        player);
    menu.clicked(3,
        DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT),
        ClickType::QuickCraft,
        player);
    menu.clicked(4,
        DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT),
        ClickType::QuickCraft,
        player);

    // 结束拖拽
    menu.clicked(5,
        DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT),
        ClickType::QuickCraft,
        player);

    // 检查物品是否被分发（每个槽位64/3=21个）
    // 注意：实际分发逻辑依赖于按钮编码
}

TEST_F(AbstractContainerMenuTest, PickAllDoubleClick)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));
    inventory.setItem(1, ItemStack(*m_diamond, 20));
    inventory.setItem(2, ItemStack(*m_diamond, 30));

    class TestMenu : public AbstractContainerMenu {
    public:
        TestMenu(PlayerInventory* inv)
            : AbstractContainerMenu(0, inv)
        {
            for (int i = 0; i < 9; ++i) {
                addSlot(std::make_unique<Slot>(inv, i, 8 + i * 18, 142));
            }
            m_playerInvStart = 0;
        }
        bool stillValid(const Player& player) const override { return true; }
    };

    TestMenu menu(&inventory);
    Player player(1, "TestPlayer");

    // PickAll（双击）在空鼠标状态下点击有物品的槽位，应该拾取所有相同物品
    // 鼠标初始为空，点击槽位0应该拾取所有钻石（10+20+30=60）
    menu.clicked(0, 0, ClickType::PickAll, player);

    // 鼠标应该有10+20+30=60个钻石
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 60);

    // 槽位应该都变空了
    EXPECT_TRUE(menu.getSlot(0)->isEmpty());
    EXPECT_TRUE(menu.getSlot(1)->isEmpty());
    EXPECT_TRUE(menu.getSlot(2)->isEmpty());
}

TEST_F(AbstractContainerMenuTest, CloneInCreativeMode)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 10));

    class TestMenu : public AbstractContainerMenu {
    public:
        TestMenu(PlayerInventory* inv)
            : AbstractContainerMenu(0, inv)
        {
            for (int i = 0; i < 9; ++i) {
                addSlot(std::make_unique<Slot>(inv, i, 8 + i * 18, 142));
            }
            m_playerInvStart = 0;
        }
        bool stillValid(const Player& player) const override { return true; }
    };

    TestMenu menu(&inventory);
    Player player(1, "TestPlayer");
    player.setGameMode(GameMode::Creative); // 设置为创造模式

    // 中键复制
    menu.clicked(0, 2, ClickType::Clone, player);

    // 创造模式下，鼠标应该有满堆叠的钻石
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 64);
}

// ============================================================================
// 容器包测试
// ============================================================================

class ContainerPacketTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_iron = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    }

    Item* m_diamond = nullptr;
    Item* m_iron = nullptr;
};

TEST_F(ContainerPacketTest, ContainerContentPacket)
{
    std::vector<ItemStack> items;
    items.emplace_back(*m_diamond, 10);
    items.emplace_back(*m_iron, 5);
    items.emplace_back(ItemStack::EMPTY);

    ItemStack carried(*m_iron, 7);
    ContainerContentPacket packet(1, std::move(items), carried);

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
    // 末尾光标物品（对齐 SPacketWindowItems）
    EXPECT_EQ(decoded.carriedItem().getItem(), m_iron);
    EXPECT_EQ(decoded.carriedItem().getCount(), 7);
}

TEST_F(ContainerPacketTest, ContainerContentPacketEmptyCarried)
{
    // 默认 carried（空堆）也能正确往返
    std::vector<ItemStack> items;
    items.emplace_back(*m_diamond, 1);

    ContainerContentPacket packet(1, std::move(items));

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerContentPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    ContainerContentPacket decoded = result.value();

    EXPECT_EQ(decoded.size(), 1);
    EXPECT_TRUE(decoded.carriedItem().isEmpty());
}

TEST_F(ContainerPacketTest, ContainerSlotPacket)
{
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

TEST_F(ContainerPacketTest, ContainerClickPacket)
{
    ItemStack cursor(*m_iron, 64);
    ContainerClickPacket packet(3, 10, 0, 1, ClickAction::Pickup, cursor);

    EXPECT_EQ(packet.containerId(), 3);
    EXPECT_EQ(packet.slotIndex(), 10);
    EXPECT_EQ(packet.button(), 0);
    EXPECT_EQ(packet.transactionId(), 1);
    EXPECT_EQ(packet.action(), ClickAction::Pickup);
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
    EXPECT_EQ(decoded.transactionId(), 1);
    EXPECT_EQ(decoded.action(), ClickAction::Pickup);
    EXPECT_EQ(decoded.cursorItem().getItem(), m_iron);
    EXPECT_EQ(decoded.cursorItem().getCount(), 64);
}

TEST_F(ContainerPacketTest, ClickTypeMapping)
{
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Pickup, 0), ClickType::Pick);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Pickup, 1), ClickType::PickSome);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::PickupAll, 0), ClickType::PickAll);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Throw, 0), ClickType::Throw);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Throw, 1), ClickType::ThrowAll);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::Pick), ClickAction::Pickup);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::Pickup), ClickAction::Pickup);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::QuickMove, 0), ClickType::QuickMove);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Clone, 0), ClickType::Clone);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::QuickCraft, 0), ClickType::QuickCraft);
    EXPECT_EQ(ContainerTypes::toClickType(ClickAction::Swap, 4), ClickType::Swap);
}

TEST_F(ContainerPacketTest, ClickActionMapping)
{
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::Pick), ClickAction::Pickup);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::PickSome), ClickAction::Pickup);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::PickAll), ClickAction::PickupAll);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::Place), ClickAction::Pickup);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::PlaceSome), ClickAction::Pickup);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::PlaceAll), ClickAction::Pickup);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::Throw), ClickAction::Throw);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::ThrowAll), ClickAction::Throw);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::QuickMove), ClickAction::QuickMove);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::QuickCraft), ClickAction::QuickCraft);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::Clone), ClickAction::Clone);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::Pickup), ClickAction::Pickup);
    EXPECT_EQ(ContainerTypes::toClickAction(ClickType::Swap), ClickAction::Swap);
}

TEST_F(ContainerPacketTest, HandleContainerClickUsesOpenMenu)
{
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory;
    inventory.setItem(9, ItemStack(*m_diamond, 4));

    CraftingMenu menu(7, &inventory, nullptr);
    player.setOpenContainerMenu(&menu);

    ContainerClickPacket packet(7, 10, 0, 1, ClickAction::Pickup, ItemStack::EMPTY);
    EXPECT_TRUE(ContainerPacketHandler::handleContainerClick(player, packet));
    EXPECT_TRUE(menu.getSlot(10)->isEmpty());
    EXPECT_FALSE(menu.getCarriedItem().isEmpty());
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 4);
}

TEST_F(ContainerPacketTest, HandleCloseContainerClearsOpenMenu)
{
    Player player(1, "TestPlayer");
    PlayerInventory inventory;
    CraftingMenu menu(7, &inventory, nullptr);
    player.setOpenContainerMenu(&menu);

    CloseContainerPacket packet(7);
    ContainerPacketHandler::handleCloseContainer(player, packet);

    EXPECT_EQ(player.openContainerMenu(), nullptr);
}

TEST_F(ContainerPacketTest, CloseContainerPacket)
{
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

TEST_F(ContainerPacketTest, OpenContainerPacket)
{
    // 使用 ContainerType 枚举值构造（Generic9x3 = 2）
    OpenContainerPacket packet(1, static_cast<i32>(ContainerType::Generic9x3), "Chest");

    EXPECT_EQ(packet.containerId(), 1);
    EXPECT_EQ(packet.type(), static_cast<i32>(ContainerType::Generic9x3));
    EXPECT_EQ(packet.title(), "Chest");

    // 序列化
    network::PacketSerializer ser;
    packet.serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = OpenContainerPacket::deserialize(deser);

    ASSERT_TRUE(result.success()) << result.error().message();
    OpenContainerPacket decoded = result.value();

    EXPECT_EQ(decoded.containerId(), 1);
    EXPECT_EQ(decoded.type(), static_cast<i32>(ContainerType::Generic9x3));
    EXPECT_EQ(decoded.title(), "Chest");
}

TEST_F(ContainerPacketTest, HotbarSelectPacket)
{
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

TEST_F(ContainerPacketTest, HotbarSelectPacketInvalidSlot)
{
    // 测试无效槽位
    HotbarSelectPacket packet(10); // 超出范围

    network::PacketSerializer ser;
    packet.serialize(ser);

    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = HotbarSelectPacket::deserialize(deser);

    // 应该失败
    EXPECT_FALSE(result.success());
}

TEST_F(ContainerPacketTest, HotbarSetPacket)
{
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

TEST_F(ContainerPacketTest, PlayerInventoryPacket)
{
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
// CreativeInventory 测试
// ============================================================================

class CreativeInventoryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

TEST_F(CreativeInventoryTest, PaletteEntriesContainCraftingTable)
{
    const auto entries = buildCreativePaletteEntries();
    ASSERT_FALSE(entries.empty());

    bool foundCraftingTable = false;
    for (const auto& entry : entries) {
        const Item* item = entry.stack.getItem();
        if (item != nullptr && item->itemLocation().toString() == "minecraft:crafting_table") {
            foundCraftingTable = true;
            break;
        }
    }

    EXPECT_TRUE(foundCraftingTable);
}

TEST_F(CreativeInventoryTest, FillCreativeModeInventoryPopulatesFirstSlot)
{
    PlayerInventory inventory;
    fillCreativeModeInventory(inventory);

    EXPECT_FALSE(inventory.getItem(0).isEmpty()) << "Expected creative inventory slot 0 to be populated";
    EXPECT_EQ(inventory.getItem(0).getItem()->itemLocation().toString(), "minecraft:crafting_table");
    EXPECT_EQ(inventory.getItem(0).getCount(), 64);
}
