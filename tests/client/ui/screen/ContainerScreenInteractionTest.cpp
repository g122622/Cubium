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

// 测试容器屏幕交互系统的7种交互类型：
// 1. Shift+click（快速移动）
// 2. 数字键1-9（快捷栏交换）
// 3. Q键（丢弃物品）
// 4. 中键（创造模式复制）
// 5. 双击（拾取全部相同物品）
// 6. 拖拽分发（均匀/逐个/填满）
// 7. 点击外部（丢弃光标物品）
//
// 这些测试验证 AbstractContainerMenu 的 clicked() 方法正确处理
// 各种 ClickType。屏幕层的交互映射（_actionToClickType, _getQuickCraftType）
// 是纯函数，在此测试其映射正确性。

#include "../src/client/ui/kagero/Types.hpp"
#include "../src/common/entity/entities/player/Player.hpp"
#include "../src/common/entity/inventory/AbstractContainerMenu.hpp"
#include "../src/common/entity/inventory/ContainerTypes.hpp"
#include "../src/common/entity/inventory/PlayerInventory.hpp"
#include "../src/common/entity/inventory/Slot.hpp"
#include "../src/common/item/Items.hpp"
#include "../src/common/item/core/ItemRegistry.hpp"
#include "../src/common/item/items/block/BlockItemRegistry.hpp"
#include "../src/common/item/items/special/bundle/BundleContents.hpp"
#include "../src/common/network/packet/ContainerPacketHandler.hpp"
#include "../src/common/network/packet/InventoryPackets.hpp"
#include "../src/common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

// GLFW 修饰键常量（与 GLFW 定义一致，避免测试依赖 GLFW）
// 这些值在 ContainerInteraction 中用于检测 Shift/Ctrl 等修饰键
namespace TestGlfwConstants {
constexpr int GLFW_MOD_SHIFT = static_cast<int>(mc::client::ui::kagero::KeyMods::Shift);
constexpr int GLFW_MOD_CONTROL = static_cast<int>(mc::client::ui::kagero::KeyMods::Control);
constexpr int GLFW_MOD_ALT = static_cast<int>(mc::client::ui::kagero::KeyMods::Alt);
constexpr int GLFW_MOD_SUPER = static_cast<int>(mc::client::ui::kagero::KeyMods::Super);
} // namespace TestGlfwConstants

using namespace mc;

// ============================================================================
// ClickAction/ClickType 映射测试
// 验证 ContainerInteraction::_actionToClickType 的映射逻辑
// （此映射逻辑与 ContainerPacketHandler::toClickType 一致）
// ============================================================================

class ContainerScreenInteractionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        VanillaBlocks::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_iron = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
        m_stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stone"));
    }

    Item* m_diamond = nullptr;
    Item* m_iron = nullptr;
    Item* m_stone = nullptr;
};

// ============================================================================
// 1. Shift+click（快速移动）测试
// ============================================================================

TEST_F(ContainerScreenInteractionTest, ShiftClickQuickMovesFromPlayerInventoryToContainer)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    // 放钻石到玩家主背包槽位9（对应菜单槽位1）
    inventory.setItem(9, ItemStack(*m_diamond, 16));

    class TestMenu : public AbstractContainerMenu {
    public:
        TestMenu(PlayerInventory* inv)
            : AbstractContainerMenu(0, inv)
        {
            // 容器槽位（槽位0）
            addSlot(std::make_unique<Slot>(inv, 0, 0, 0));
            // 玩家主背包槽位（槽位1-27）
            for (int i = 0; i < 27; ++i) {
                addSlot(std::make_unique<Slot>(inv, 9 + i, 8 + (i % 9) * 18, 18 + (i / 9) * 18));
            }
            // 快捷栏槽位（槽位28-36）
            for (int i = 0; i < 9; ++i) {
                addSlot(std::make_unique<Slot>(inv, i, 8 + i * 18, 142));
            }
            m_playerInvStart = 1;
        }
        bool stillValid(const Player& player) const override { return true; }
    };

    TestMenu menu(&inventory);
    Player player(1, "TestPlayer");

    // Shift+点击玩家背包槽位1应将钻石移到容器槽位0
    menu.clicked(1, 0, ClickType::QuickMove, player);

    // 钻石应该移到容器槽位0
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 16);
    EXPECT_TRUE(menu.getSlot(1)->isEmpty());
}

// ============================================================================
// 2. 数字键交换（Swap）测试
// ============================================================================

TEST_F(ContainerScreenInteractionTest, NumberKeySwapExchangesSlotWithHotbar)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_iron, nullptr);

    PlayerInventory inventory;
    // 快捷栏槽位0放钻石
    inventory.setItem(0, ItemStack(*m_diamond, 10));
    // 主背包槽位9放铁锭
    inventory.setItem(9, ItemStack(*m_iron, 5));

    class TestMenu : public AbstractContainerMenu {
    public:
        TestMenu(PlayerInventory* inv)
            : AbstractContainerMenu(0, inv)
        {
            // 快捷栏槽位
            for (int i = 0; i < 9; ++i) {
                addSlot(std::make_unique<Slot>(inv, i, 8 + i * 18, 142));
            }
            // 主背包槽位
            for (int i = 0; i < 27; ++i) {
                addSlot(std::make_unique<Slot>(inv, 9 + i, 8 + (i % 9) * 18, 84 + (i / 9) * 18));
            }
            m_playerInvStart = 0;
        }
        bool stillValid(const Player& player) const override { return true; }
    };

    TestMenu menu(&inventory);
    Player player(1, "TestPlayer");

    // 数字键1（button=0）交换快捷栏槽位0和菜单槽位9
    // 菜单槽位9 = 主背包槽位9(铁锭)，快捷栏0 = 钻石
    menu.clicked(9, 0, ClickType::Swap, player);

    // 槽位0（原快捷栏0，钻石）应该变成铁锭
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_iron);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 5);
    // 槽位9（原主背包0，铁锭）应该变成钻石
    EXPECT_EQ(menu.getSlot(9)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(9)->getItem().getCount(), 10);
}

TEST_F(ContainerScreenInteractionTest, NumberKeySwapWithEmptyHotbarSlot)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    // 快捷栏槽位0放钻石
    inventory.setItem(0, ItemStack(*m_diamond, 64));
    // 快捷栏槽位3为空

    class TestMenu : public AbstractContainerMenu {
    public:
        TestMenu(PlayerInventory* inv)
            : AbstractContainerMenu(0, inv)
        {
            for (int i = 0; i < 9; ++i) {
                addSlot(std::make_unique<Slot>(inv, i, 8 + i * 18, 142));
            }
            for (int i = 0; i < 27; ++i) {
                addSlot(std::make_unique<Slot>(inv, 9 + i, 8 + (i % 9) * 18, 84 + (i / 9) * 18));
            }
            m_playerInvStart = 0;
        }
        bool stillValid(const Player& player) const override { return true; }
    };

    TestMenu menu(&inventory);
    Player player(1, "TestPlayer");

    // 数字键4（button=3）交换快捷栏槽位3和菜单槽位9（主背包0，空）
    menu.clicked(9, 3, ClickType::Swap, player);

    // 槽位3（原快捷栏3，空）应该变成钻石（从槽位9取来）
    // 但槽位9是空的，所以应该是槽位9从快捷栏3取来空物品
    // 等等 - 槽位9 = 主背包index 9 + 9 = inventory slot 18 = 空
    // 交换后：快捷栏3 = 空，槽位9 = 钻石
    // 不对，_handleSwap交换的是当前菜单slot和inventory的swapSlot
    // slot 9 是菜单槽位9，对应inventory slot 18
    // button=3表示快捷栏slot 3，inventory slot 3
    // 交换：slot9(空) <-> inventory[3](空)
    // 两个都空，所以什么都没发生

    // 更好的测试：交换有物品的槽位
    // 槽位0有钻石，与空快捷栏slot3交换
    menu.clicked(0, 3, ClickType::Swap, player);

    // 槽位0应该变空（快捷栏3是空的，交换过来）
    EXPECT_TRUE(menu.getSlot(0)->isEmpty());
    // 快捷栏3应该有钻石
    EXPECT_EQ(inventory.getItem(3).getItem(), m_diamond);
    EXPECT_EQ(inventory.getItem(3).getCount(), 64);
}

// ============================================================================
// 3. Q键丢弃（Throw）测试
// ============================================================================

TEST_F(ContainerScreenInteractionTest, ThrowDropsOneItemFromSlot)
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

    // 设置丢弃回调来捕获丢弃的物品
    ItemStack droppedStack;
    menu.setItemDropCallback([&droppedStack](const ItemStack& stack, Player& p, bool retainOwnership) {
        droppedStack = stack;
        (void)p;
        (void)retainOwnership;
    });

    // Q键丢弃槽位0的一个物品（ClickType::Throw, button=0表示丢弃一个）
    menu.clicked(0, 0, ClickType::Throw, player);

    // 槽位0应该有9个钻石
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 9);

    // 丢弃1个钻石
    EXPECT_EQ(droppedStack.getItem(), m_diamond);
    EXPECT_EQ(droppedStack.getCount(), 1);
}

TEST_F(ContainerScreenInteractionTest, ThrowAllDropsEntireStackFromSlot)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 32));

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

    // 设置丢弃回调
    ItemStack droppedStack;
    menu.setItemDropCallback([&droppedStack](const ItemStack& stack, Player& p, bool retainOwnership) {
        droppedStack = stack;
        (void)p;
        (void)retainOwnership;
    });

    // Ctrl+Q丢弃整组（ClickType::Throw, button=1表示丢弃整组）
    menu.clicked(0, 1, ClickType::Throw, player);

    // 槽位0应该为空
    EXPECT_TRUE(menu.getSlot(0)->isEmpty());

    // 丢弃32个钻石
    EXPECT_EQ(droppedStack.getItem(), m_diamond);
    EXPECT_EQ(droppedStack.getCount(), 32);
}

// ============================================================================
// 4. 中键复制（Clone，创造模式）测试
// ============================================================================

TEST_F(ContainerScreenInteractionTest, CloneInCreativeModeCopiesFullStack)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 5));

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
    player.setGameMode(GameMode::Creative);

    // 中键（button=2）创造模式复制
    menu.clicked(0, 2, ClickType::Clone, player);

    // 创造模式下，鼠标应该有满堆叠的钻石（64）
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 64);

    // 原槽位不变
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 5);
}

TEST_F(ContainerScreenInteractionTest, CloneDoesNothingInSurvivalMode)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 5));

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
    // 生存模式（默认）

    // 中键复制在生存模式下无效
    menu.clicked(0, 2, ClickType::Clone, player);

    // 生存模式下光标应该为空
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());

    // 原槽位不变
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 5);
}

// ============================================================================
// 5. 双击拾取全部（PickupAll）测试
// ============================================================================

TEST_F(ContainerScreenInteractionTest, PickupAllCollectsAllMatchingItems)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_iron, nullptr);

    PlayerInventory inventory;
    // 在多个槽位放钻石
    inventory.setItem(0, ItemStack(*m_diamond, 10));
    inventory.setItem(1, ItemStack(*m_diamond, 20));
    inventory.setItem(2, ItemStack(*m_diamond, 30));
    // 不同物品不应被拾取
    inventory.setItem(3, ItemStack(*m_iron, 5));

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

    // PickAll要求光标为空且槽位有物品
    // 双击第一个槽位应该拾取所有钻石
    menu.clicked(0, 0, ClickType::PickAll, player);

    // 鼠标应该有10+20+30=60个钻石
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 60);

    // 钻石槽位应该都变空了
    EXPECT_TRUE(menu.getSlot(0)->isEmpty());
    EXPECT_TRUE(menu.getSlot(1)->isEmpty());
    EXPECT_TRUE(menu.getSlot(2)->isEmpty());

    // 铁锭槽位不应受影响
    EXPECT_EQ(menu.getSlot(3)->getItem().getItem(), m_iron);
    EXPECT_EQ(menu.getSlot(3)->getItem().getCount(), 5);
}

TEST_F(ContainerScreenInteractionTest, PickupAllRespectsMaxStackSize)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    // 放置足够多的钻石超过64（最大堆叠）
    inventory.setItem(0, ItemStack(*m_diamond, 64));
    inventory.setItem(1, ItemStack(*m_diamond, 10));

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

    // PickAll 应拾取最大堆叠量（64）
    menu.clicked(0, 0, ClickType::PickAll, player);

    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 64);

    // 槽位0变空，槽位1应有10个钻石
    EXPECT_TRUE(menu.getSlot(0)->isEmpty());
    EXPECT_EQ(menu.getSlot(1)->getItem().getCount(), 10);
}

// ============================================================================
// 6. 拖拽分发（QuickCraft）测试
// 注意：拖拽协议使用 -999 槽位进行 START 和 END 事件
// ADD_SLOT 事件发送到实际槽位
// ============================================================================

TEST_F(ContainerScreenInteractionTest, FullDragProtocolEvenDistribution)
{
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);

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
    menu.setCarriedItem(ItemStack(*m_diamond, 60));

    // 完整拖拽协议：START(-999) → ADD_SLOT × 3 → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);

    // START (发送到-999)
    menu.clicked(-999, startButton, ClickType::QuickCraft, player);

    // ADD_SLOT（3个槽位）
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(1, addButton, ClickType::QuickCraft, player);
    menu.clicked(2, addButton, ClickType::QuickCraft, player);

    // END (发送到-999)
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 均匀分发60个到3个槽位：20, 20, 20
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 20);
    EXPECT_EQ(menu.getSlot(1)->getItem().getCount(), 20);
    EXPECT_EQ(menu.getSlot(2)->getItem().getCount(), 20);

    // 光标清空
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());
}

TEST_F(ContainerScreenInteractionTest, FullDragProtocolSingleDistribution)
{
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);

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
    menu.setCarriedItem(ItemStack(*m_diamond, 5));

    // 逐个分发：START(-999) → ADD_SLOT × 3 → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(1, addButton, ClickType::QuickCraft, player);
    menu.clicked(2, addButton, ClickType::QuickCraft, player);
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 逐个分发：每个槽位1个
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 1);
    EXPECT_EQ(menu.getSlot(1)->getItem().getCount(), 1);
    EXPECT_EQ(menu.getSlot(2)->getItem().getCount(), 1);

    // 光标剩余5-3=2
    EXPECT_EQ(menu.getCarriedItem().getCount(), 2);
}

TEST_F(ContainerScreenInteractionTest, DragCannotDistributeToOccupiedDifferentSlot)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_iron, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);
    // 槽位1放铁锭（与光标上的钻石不同）
    inventory.setItem(1, ItemStack(*m_iron, 10));

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
    menu.setCarriedItem(ItemStack(*m_diamond, 64));

    // 开始拖拽（均匀模式）
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);

    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    // 槽位1有铁锭，不能放钻石（不同物品无法合并）
    // _canDragIntoSlot检查：如果槽位有不同物品，不应添加到拖拽列表
    menu.clicked(1, addButton, ClickType::QuickCraft, player);
    menu.clicked(2, addButton, ClickType::QuickCraft, player);

    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 槽位0和2应有钻石，槽位1保持铁锭不变
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(1)->getItem().getItem(), m_iron);
    EXPECT_EQ(menu.getSlot(2)->getItem().getItem(), m_diamond);

    // 槽位1的铁锭数量不变
    EXPECT_EQ(menu.getSlot(1)->getItem().getCount(), 10);
}

// ============================================================================
// 6.1 QuickCraft 单槽降级测试
// 对应 MC 1.21.11 AbstractContainerMenu#doClick 中 quickcraftSlots.size()==1 的降级路径：
// 当拖拽仅包含一个槽位时，重置拖拽状态后递归调用 clicked(slot, dragMode, Pick, player)，
// 让单槽拖拽降级为普通 PICKUP 点击，从而触发 _tryItemClickBehaviourOverride
// （收纳袋的 overrideStackedOnOther/overrideOtherStackedOnMe）。
// 与多槽分发不同：单槽不调用 _distributeToDragSlot，而是走完整 PICKUP 流程，
// 因此会触发槽位覆写协议。
// ============================================================================

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_EvenMode_DegradesToPickupPrimary)
{
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);

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
    // 光标持有 32 个钻石
    menu.setCarriedItem(ItemStack(*m_diamond, 32));

    // 单槽拖拽协议（MODE_EVEN）：START(-999) → ADD_SLOT(slot 0) → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 单槽降级为 PICKUP 左键：32 个钻石应整体放入槽位 0
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 32);
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());
}

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_SingleMode_DegradesToPickupSecondary)
{
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);

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
    // 光标持有 32 个钻石
    menu.setCarriedItem(ItemStack(*m_diamond, 32));

    // 单槽拖拽协议（MODE_SINGLE）：START(-999) → ADD_SLOT(slot 0) → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 单槽降级为 PICKUP 右键：仅放入 1 个钻石
    // （右键放置语义：从光标取 1 个放入槽位）
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 1);
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 31);
}

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_EvenMode_PickupFromOccupiedSlot)
{
    // 验证单槽降级到 PICKUP 后，PICKUP 自身的"光标有物品 + 槽位有相同物品 → 合并"分支。
    // 单槽拖拽降级只发生在光标非空时（MC Java 在 START 时检查 carried 非空），
    // 因此本测试验证的是：光标有钻石 + 槽位也有钻石 → 降级为 PICKUP 后合并。
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);
    // 槽位 0 已有 16 个钻石
    inventory.setItem(0, ItemStack(*m_diamond, 16));

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
    // 光标持有 32 个钻石（与槽位 0 同物品，可合并）
    menu.setCarriedItem(ItemStack(*m_diamond, 32));

    // 单槽拖拽协议（MODE_EVEN）：START(-999) → ADD_SLOT(slot 0) → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 单槽降级为 PICKUP 左键 + 光标有 32 + 槽位有 16 = 48 ≤ 64 → 全部合并到槽位
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 48);
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());
}

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_SingleMode_PickupHalfFromOccupiedSlot)
{
    // 验证单槽降级到 PICKUP 右键：光标有钻石 + 槽位有同物品 → 右键放置 1 个
    // 单槽拖拽降级只发生在光标非空时（MC Java 在 START 时检查 carried 非空）。
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);
    // 槽位 0 有 16 个钻石
    inventory.setItem(0, ItemStack(*m_diamond, 16));

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
    // 光标持有 32 个钻石
    menu.setCarriedItem(ItemStack(*m_diamond, 32));

    // 单槽拖拽协议（MODE_SINGLE）：START(-999) → ADD_SLOT(slot 0) → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 单槽降级为 PICKUP 右键 + 光标有 32 + 槽位有 16：右键放置 1 个，槽位变 17，光标变 31
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 31);
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 17);
}

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_MultiSlotStillDistributes)
{
    // 反向回归测试：多槽（>=2）仍走原分发路径，不降级
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);

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
    menu.setCarriedItem(ItemStack(*m_diamond, 60));

    // 双槽拖拽协议（MODE_EVEN）：START(-999) → ADD_SLOT × 2 → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(1, addButton, ClickType::QuickCraft, player);
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 多槽均匀分发：30 + 30
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 30);
    EXPECT_EQ(menu.getSlot(1)->getItem().getCount(), 30);
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());
}

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_EvenMode_EndOnRealSlot_DegradesToPickup)
{
    // 验证 _handleQuickCraft（slotIndex >= 0）路径的单槽降级：
    // 当 END 事件发送到实际槽位（而非 -999）时，也应触发单槽降级到 PICKUP。
    // 对应 MC 1.21.11 AbstractContainerMenu#doClick 中 quickcraftSlots.size()==1
    // 的降级路径——MC Java 不区分 END 事件槽位是 -999 还是实际槽位，统一处理。
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);

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
    menu.setCarriedItem(ItemStack(*m_diamond, 32));

    // 单槽拖拽协议（MODE_EVEN）：
    // START(-999) → ADD_SLOT(slot 0) → END(slot 0)
    // 注意：END 事件发送到实际槽位 0，而非 -999，触发 _handleQuickCraft 中的 END 分支
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    // END 发送到实际槽位 0（而非 -999）
    menu.clicked(0, endButton, ClickType::QuickCraft, player);

    // 单槽降级为 PICKUP 左键：32 个钻石应整体放入槽位 0
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 32);
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());
}

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_SingleMode_EndOnRealSlot_DegradesToPickup)
{
    // 验证 _handleQuickCraft（slotIndex >= 0）路径的单槽降级（MODE_SINGLE）：
    // END 事件发送到实际槽位时，MODE_SINGLE 应降级为 PICKUP 右键。
    ASSERT_NE(m_diamond, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);

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
    menu.setCarriedItem(ItemStack(*m_diamond, 32));

    // 单槽拖拽协议（MODE_SINGLE）：START(-999) → ADD_SLOT(slot 0) → END(slot 0)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    // END 发送到实际槽位 0
    menu.clicked(0, endButton, ClickType::QuickCraft, player);

    // 单槽降级为 PICKUP 右键：仅放入 1 个钻石
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 1);
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 31);
}

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_BundleOverride_NotTriggeredForDifferentItem)
{
    // 反向验证：当光标物品与槽位物品不同时（如光标=钻石、槽位=收纳袋），
    // _canDragIntoSlot 会拒绝将该槽位加入拖拽列表（不可合并），
    // 因此 m_dragSlots 为空，单槽降级不会触发。
    // 这与 MC Java 的 canItemQuickReplace 行为一致：不同物品不能拖拽分发。
    // 收纳袋的 overrideOtherStackedOnMe 通过直接 PICKUP 点击触发，而非拖拽降级。
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(Items::BUNDLE, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);
    // 槽位 0 放空收纳袋
    inventory.setItem(0, ItemStack(*Items::BUNDLE, 1));

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
    // 光标持有 16 个钻石（与槽位 0 的收纳袋不同）
    menu.setCarriedItem(ItemStack(*m_diamond, 16));

    // 单槽拖拽（MODE_EVEN）：START(-999) → ADD_SLOT(slot 0=收纳袋) → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 由于 _canDragIntoSlot 拒绝不同物品，槽位 0 未加入拖拽列表，
    // 拖拽结束时分发为空操作：
    // - 槽位 0 仍是空收纳袋
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), Items::BUNDLE);
    auto contents = item::items::BundleContents::fromItemStack(menu.getSlot(0)->getItem());
    EXPECT_TRUE(contents.isEmpty());
    // - 光标仍是 16 个钻石
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 16);
}

TEST_F(ContainerScreenInteractionTest, QuickCraftSingleSlot_BundleInHand_NotTriggeredForDifferentItem)
{
    // 反向验证：当光标=收纳袋、槽位=钻石（不同物品）时，
    // _canDragIntoSlot 拒绝将该槽位加入拖拽列表，单槽降级不会触发。
    // 收纳袋的 overrideStackedOnOther 通过直接 PICKUP 点击触发，而非拖拽降级。
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(Items::BUNDLE, nullptr);

    Player player(1, "TestPlayer");
    PlayerInventory inventory(&player);
    // 槽位 0 放 16 个钻石
    inventory.setItem(0, ItemStack(*m_diamond, 16));

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
    // 光标持有空收纳袋
    menu.setCarriedItem(ItemStack(*Items::BUNDLE, 1));

    // 单槽拖拽（MODE_EVEN）：START(-999) → ADD_SLOT(slot 0=钻石) → END(-999)
    const i32 startButton = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    const i32 endButton = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);

    menu.clicked(-999, startButton, ClickType::QuickCraft, player);
    menu.clicked(0, addButton, ClickType::QuickCraft, player);
    menu.clicked(-999, endButton, ClickType::QuickCraft, player);

    // 槽位 0 未加入拖拽列表，拖拽结束时分发为空操作：
    // - 槽位 0 仍是 16 个钻石
    EXPECT_EQ(menu.getSlot(0)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 16);
    // - 光标仍是空收纳袋
    EXPECT_EQ(menu.getCarriedItem().getItem(), Items::BUNDLE);
    auto contents = item::items::BundleContents::fromItemStack(menu.getCarriedItem());
    EXPECT_TRUE(contents.isEmpty());
}

// ============================================================================
// 7. 点击外部丢弃光标物品测试
// ============================================================================

TEST_F(ContainerScreenInteractionTest, ClickOutsideDropsCarriedItem)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;

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

    // 设置光标物品
    menu.setCarriedItem(ItemStack(*m_diamond, 32));

    // 设置丢弃回调
    bool dropCallbackCalled = false;
    ItemStack droppedStack;
    menu.setItemDropCallback(
        [&dropCallbackCalled, &droppedStack](const ItemStack& stack, Player& p, bool retainOwnership) {
            dropCallbackCalled = true;
            droppedStack = stack;
            (void)p;
            (void)retainOwnership;
        });

    // 点击外部（slotIndex = -999，左键Pick丢弃全部光标物品）
    menu.clicked(-999, 0, ClickType::Pick, player);

    // 光标应该清空
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());
    EXPECT_TRUE(dropCallbackCalled);
    EXPECT_EQ(droppedStack.getItem(), m_diamond);
    EXPECT_EQ(droppedStack.getCount(), 32);
}

TEST_F(ContainerScreenInteractionTest, ClickOutsideDropsOneWithRightClick)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;

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

    // 设置光标物品
    menu.setCarriedItem(ItemStack(*m_diamond, 32));

    // 设置丢弃回调
    ItemStack droppedStack;
    menu.setItemDropCallback([&droppedStack](const ItemStack& stack, Player& p, bool retainOwnership) {
        droppedStack = stack;
        (void)p;
        (void)retainOwnership;
    });

    // 右键点击外部（PickSome丢弃一个）
    menu.clicked(-999, 1, ClickType::PickSome, player);

    // 光标应该有31个钻石
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 31);

    // 丢弃1个钻石
    EXPECT_EQ(droppedStack.getItem(), m_diamond);
    EXPECT_EQ(droppedStack.getCount(), 1);
}

TEST_F(ContainerScreenInteractionTest, ClickOutsideDoesNothingWhenCarriedEmpty)
{
    PlayerInventory inventory;

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

    // 光标为空，点击外部不应有副作用
    menu.clicked(-999, 0, ClickType::Pick, player);

    // 光标应该仍为空
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());
}

// ============================================================================
// ClickAction/ClickType 映射正确性测试
// 验证屏幕层映射与 ContainerPacketHandler::toClickType 一致
// ============================================================================

TEST_F(ContainerScreenInteractionTest, ClickActionMappingMatchesContainerTypes)
{
    // 验证所有 ClickAction -> ClickType 映射与 ContainerPacketHandler 一致
    using ContainerTypes::toClickType;

    // Pickup + button 0 = Pick
    EXPECT_EQ(toClickType(ClickAction::Pickup, 0), ClickType::Pick);
    // Pickup + button 1 = PickSome
    EXPECT_EQ(toClickType(ClickAction::Pickup, 1), ClickType::PickSome);
    // QuickMove = QuickMove
    EXPECT_EQ(toClickType(ClickAction::QuickMove, 0), ClickType::QuickMove);
    // Swap = Swap
    EXPECT_EQ(toClickType(ClickAction::Swap, 4), ClickType::Swap);
    // Clone = Clone
    EXPECT_EQ(toClickType(ClickAction::Clone, 0), ClickType::Clone);
    // Throw + button 0 = Throw (丢弃一个)
    EXPECT_EQ(toClickType(ClickAction::Throw, 0), ClickType::Throw);
    // Throw + button 1 = ThrowAll (丢弃整组)
    EXPECT_EQ(toClickType(ClickAction::Throw, 1), ClickType::ThrowAll);
    // QuickCraft = QuickCraft
    EXPECT_EQ(toClickType(ClickAction::QuickCraft, 0), ClickType::QuickCraft);
    // PickupAll = PickAll
    EXPECT_EQ(toClickType(ClickAction::PickupAll, 0), ClickType::PickAll);
}

// ============================================================================
// DragConstants 编码测试
// 验证拖拽按钮编码的正确性
// ============================================================================

TEST_F(ContainerScreenInteractionTest, DragConstantsEncodingStart)
{
    // 验证按钮编码：低2位=事件状态，高2位=拖拽模式
    const i32 evenStart = DragConstants::EVENT_START | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    EXPECT_EQ(evenStart & DragConstants::EVENT_MASK, DragConstants::EVENT_START);
    EXPECT_EQ((evenStart >> DragConstants::MODE_SHIFT) & DragConstants::MODE_MASK, DragConstants::MODE_EVEN);

    const i32 singleStart = DragConstants::EVENT_START | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    EXPECT_EQ(singleStart & DragConstants::EVENT_MASK, DragConstants::EVENT_START);
    EXPECT_EQ((singleStart >> DragConstants::MODE_SHIFT) & DragConstants::MODE_MASK, DragConstants::MODE_SINGLE);
}

TEST_F(ContainerScreenInteractionTest, DragConstantsEncodingAddSlot)
{
    const i32 addEven = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    EXPECT_EQ(addEven & DragConstants::EVENT_MASK, DragConstants::EVENT_ADD_SLOT);
    EXPECT_EQ((addEven >> DragConstants::MODE_SHIFT) & DragConstants::MODE_MASK, DragConstants::MODE_EVEN);

    const i32 addSingle = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_SINGLE << DragConstants::MODE_SHIFT);
    EXPECT_EQ(addSingle & DragConstants::EVENT_MASK, DragConstants::EVENT_ADD_SLOT);
    EXPECT_EQ((addSingle >> DragConstants::MODE_SHIFT) & DragConstants::MODE_MASK, DragConstants::MODE_SINGLE);
}

TEST_F(ContainerScreenInteractionTest, DragConstantsEncodingEnd)
{
    const i32 endEven = DragConstants::EVENT_END | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    EXPECT_EQ(endEven & DragConstants::EVENT_MASK, DragConstants::EVENT_END);
    EXPECT_EQ((endEven >> DragConstants::MODE_SHIFT) & DragConstants::MODE_MASK, DragConstants::MODE_EVEN);

    const i32 endFill = DragConstants::EVENT_END | (DragConstants::MODE_FILL << DragConstants::MODE_SHIFT);
    EXPECT_EQ(endFill & DragConstants::EVENT_MASK, DragConstants::EVENT_END);
    EXPECT_EQ((endFill >> DragConstants::MODE_SHIFT) & DragConstants::MODE_MASK, DragConstants::MODE_FILL);
}

// ============================================================================
// GLFW 修饰键常量验证
// 确认屏幕层使用的GLFW修饰键常量与平台一致
// ============================================================================

TEST_F(ContainerScreenInteractionTest, GlfwModifierConstants)
{
    // 验证GLFW修饰键常量值（屏幕层用这些来检测Shift/Ctrl）
    // 这些值必须与 GLFW/glfw3.h 中的定义一致
    using namespace TestGlfwConstants;
    EXPECT_EQ(GLFW_MOD_SHIFT, 0x0001);
    EXPECT_EQ(GLFW_MOD_CONTROL, 0x0002);
    EXPECT_EQ(GLFW_MOD_ALT, 0x0004);
    EXPECT_EQ(GLFW_MOD_SUPER, 0x0008);
}

// ============================================================================
// ContainerClickPacket 序列化测试（扩展：验证所有交互类型的网络包）
// ============================================================================

class ContainerClickPacketInteractionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    Item* m_diamond = nullptr;
};

TEST_F(ContainerClickPacketInteractionTest, ShiftClickPacket)
{
    ItemStack cursor;
    ContainerClickPacket packet(1, 5, 0, 1, ClickAction::QuickMove, cursor);
    EXPECT_EQ(packet.action(), ClickAction::QuickMove);

    network::PacketSerializer ser;
    packet.serialize(ser);
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerClickPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().action(), ClickAction::QuickMove);
}

TEST_F(ContainerClickPacketInteractionTest, SwapPacket)
{
    ItemStack cursor;
    ContainerClickPacket packet(1, 10, 3, 1, ClickAction::Swap, cursor);
    EXPECT_EQ(packet.action(), ClickAction::Swap);
    EXPECT_EQ(packet.button(), 3);

    network::PacketSerializer ser;
    packet.serialize(ser);
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerClickPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().action(), ClickAction::Swap);
    EXPECT_EQ(result.value().button(), 3);
}

TEST_F(ContainerClickPacketInteractionTest, ClonePacket)
{
    ItemStack cursor;
    ContainerClickPacket packet(1, 5, 2, 1, ClickAction::Clone, cursor);
    EXPECT_EQ(packet.action(), ClickAction::Clone);

    network::PacketSerializer ser;
    packet.serialize(ser);
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerClickPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().action(), ClickAction::Clone);
}

TEST_F(ContainerClickPacketInteractionTest, ThrowPacket)
{
    ItemStack cursor;
    ContainerClickPacket packet(1, 5, 0, 1, ClickAction::Throw, cursor);
    EXPECT_EQ(packet.action(), ClickAction::Throw);

    network::PacketSerializer ser;
    packet.serialize(ser);
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerClickPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().action(), ClickAction::Throw);
}

TEST_F(ContainerClickPacketInteractionTest, QuickCraftPacket)
{
    ItemStack cursor(*m_diamond, 64);
    // 模拟拖拽ADD_SLOT按钮
    const i32 addButton = DragConstants::EVENT_ADD_SLOT | (DragConstants::MODE_EVEN << DragConstants::MODE_SHIFT);
    ContainerClickPacket packet(1, 3, addButton, 1, ClickAction::QuickCraft, cursor);

    EXPECT_EQ(packet.action(), ClickAction::QuickCraft);
    EXPECT_EQ(packet.button(), addButton);

    network::PacketSerializer ser;
    packet.serialize(ser);
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerClickPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().action(), ClickAction::QuickCraft);
    EXPECT_EQ(result.value().button(), addButton);
}

TEST_F(ContainerClickPacketInteractionTest, PickupAllPacket)
{
    ItemStack cursor;
    ContainerClickPacket packet(1, 5, 0, 1, ClickAction::PickupAll, cursor);
    EXPECT_EQ(packet.action(), ClickAction::PickupAll);

    network::PacketSerializer ser;
    packet.serialize(ser);
    network::PacketDeserializer deser(ser.data(), ser.size());
    auto result = ContainerClickPacket::deserialize(deser);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().action(), ClickAction::PickupAll);
}

// ============================================================================
// 左键/右键拾取放置测试
// 注意：放置操作使用 ClickType::Pick（而非Place），
// 因为 _handleClickPick 根据 m_carried 是否为空自动决定拾取还是放置
// ============================================================================

TEST_F(ContainerScreenInteractionTest, LeftClickPickupAndPlace)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 32));

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

    // 左键点击槽位0拾取（ClickType::Pick, button=0, 光标为空）
    menu.clicked(0, 0, ClickType::Pick, player);
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 32);
    EXPECT_TRUE(menu.getSlot(0)->isEmpty());

    // 左键点击槽位1放置（ClickType::Pick, button=0, 光标有物品）
    menu.clicked(1, 0, ClickType::Pick, player);
    EXPECT_TRUE(menu.getCarriedItem().isEmpty());
    EXPECT_EQ(menu.getSlot(1)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(1)->getItem().getCount(), 32);
}

TEST_F(ContainerScreenInteractionTest, RightClickPickupHalf)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;
    inventory.setItem(0, ItemStack(*m_diamond, 32));

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

    // 右键点击槽位0拾取一半（ClickType::PickSome, button=1）
    menu.clicked(0, 1, ClickType::PickSome, player);
    EXPECT_EQ(menu.getCarriedItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 16);
    EXPECT_EQ(menu.getSlot(0)->getItem().getCount(), 16);
}

TEST_F(ContainerScreenInteractionTest, RightClickPlaceOne)
{
    ASSERT_NE(m_diamond, nullptr);

    PlayerInventory inventory;

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

    // 设置光标有10个钻石
    menu.setCarriedItem(ItemStack(*m_diamond, 10));

    // 右键点击空槽位放置1个（ClickType::PickSome, button=1, 光标有物品）
    menu.clicked(1, 1, ClickType::PickSome, player);
    EXPECT_EQ(menu.getSlot(1)->getItem().getItem(), m_diamond);
    EXPECT_EQ(menu.getSlot(1)->getItem().getCount(), 1);
    EXPECT_EQ(menu.getCarriedItem().getCount(), 9);
}
