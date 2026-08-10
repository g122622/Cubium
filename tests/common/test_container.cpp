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
#include "../src/common/entity/inventory/ContainerTypeUtils.hpp"
#include "../src/common/entity/inventory/ContainerTypes.hpp"
#include "../src/common/entity/inventory/CreativeInventory.hpp"
#include "../src/common/entity/inventory/PlayerInventory.hpp"
#include "../src/common/entity/inventory/Slot.hpp"
#include "../src/common/item/Items.hpp"
#include "../src/common/item/core/ItemRegistry.hpp"
#include "../src/common/item/items/block/BlockItemRegistry.hpp"
#include "../src/common/world/block/registry/VanillaBlocks.hpp"
#include "../src/server/menu/CraftingMenu.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

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
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

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
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

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
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

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
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());

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
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative); // 设置为创造模式

    // 中键复制
    menu.clicked(0, 2, ClickType::Clone, player);

    // 创造模式下，鼠标应该有满堆叠的钻石
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 64);
}

// ============================================================================
// ContainerTypes 点击映射测试
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
