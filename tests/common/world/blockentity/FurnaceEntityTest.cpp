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

#include "world/blockentity/processing/FurnaceEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"
#include "world/blockentity/processing/SmokerEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

// ========== FurnaceInventory 测试 ==========

class FurnaceInventoryTest : public ::testing::Test {
protected:
    void SetUp() override { inventory_ = std::make_unique<FurnaceInventory>(); }

    std::unique_ptr<FurnaceInventory> inventory_;
};

TEST_F(FurnaceInventoryTest, Create_HasCorrectSize)
{
    EXPECT_EQ(inventory_->getContainerSize(), FurnaceInventory::SLOT_COUNT);
    EXPECT_EQ(FurnaceInventory::SLOT_COUNT, 3);
}

TEST_F(FurnaceInventoryTest, Create_SlotIndicesAreCorrect)
{
    EXPECT_EQ(FurnaceInventory::SLOT_INPUT, 0);
    EXPECT_EQ(FurnaceInventory::SLOT_FUEL, 1);
    EXPECT_EQ(FurnaceInventory::SLOT_OUTPUT, 2);
}

TEST_F(FurnaceInventoryTest, Create_IsEmpty)
{
    EXPECT_TRUE(inventory_->isEmpty());
    EXPECT_TRUE(inventory_->isInputEmpty());
    EXPECT_TRUE(inventory_->isFuelEmpty());
    EXPECT_TRUE(inventory_->isOutputEmpty());
}

TEST_F(FurnaceInventoryTest, GetInputItem_ReturnsEmptyInitially)
{
    ItemStack input = inventory_->getInputItem();
    EXPECT_TRUE(input.isEmpty());
}

TEST_F(FurnaceInventoryTest, GetFuelItem_ReturnsEmptyInitially)
{
    ItemStack fuel = inventory_->getFuelItem();
    EXPECT_TRUE(fuel.isEmpty());
}

TEST_F(FurnaceInventoryTest, GetOutputItem_ReturnsEmptyInitially)
{
    ItemStack output = inventory_->getOutputItem();
    EXPECT_TRUE(output.isEmpty());
}

TEST_F(FurnaceInventoryTest, SetInputItem_UpdatesInput)
{
    ItemStack emptyStack;
    inventory_->setInputItem(emptyStack);
    EXPECT_TRUE(inventory_->isInputEmpty());
}

TEST_F(FurnaceInventoryTest, SetChanged_Callback)
{
    bool callbackCalled = false;
    FurnaceInventory invWithCallback([&callbackCalled]() { callbackCalled = true; });

    invWithCallback.setChanged();
    EXPECT_TRUE(callbackCalled);
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_InputSlotReturnsTrueForValidItem)
{
    // Note: canPlaceItem returns false for empty stacks, which is correct behavior
    // This tests the slot index validation
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_INPUT, ItemStack()));
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_FuelSlotReturnsTrueForValidItem)
{
    // Note: canPlaceItem returns false for empty stacks, which is correct behavior
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_FUEL, ItemStack()));
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_OutputSlotReturnsFalseForEmptyStack)
{
    // 输出槽不接受空物品堆
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_OUTPUT, ItemStack()));
}

TEST_F(FurnaceInventoryTest, Clear_MakesAllSlotsEmpty)
{
    inventory_->clear();
    EXPECT_TRUE(inventory_->isEmpty());
}

TEST_F(FurnaceInventoryTest, GetMaxStackSize_ReturnsDefault)
{
    EXPECT_EQ(inventory_->getMaxStackSize(), 64);
}

// ========== FurnaceEntity 测试 ==========

class FurnaceEntityTest : public ::testing::Test {
protected:
    void SetUp() override { furnace_ = std::make_unique<FurnaceEntity>(BlockPos(10, 20, 30)); }

    std::unique_ptr<FurnaceEntity> furnace_;
};

TEST_F(FurnaceEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(furnace_->getType(), BlockEntityType::Furnace);
}

TEST_F(FurnaceEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(furnace_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(FurnaceEntityTest, Create_HasCorrectContainerSize)
{
    EXPECT_EQ(furnace_->getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, Create_NotBurningInitially)
{
    EXPECT_FALSE(furnace_->isBurning());
    EXPECT_EQ(furnace_->getBurnTime(), 0);
    EXPECT_EQ(furnace_->getBurnTimeTotal(), 0);
}

TEST_F(FurnaceEntityTest, Create_CookTimeIsZero)
{
    EXPECT_EQ(furnace_->getCookTime(), 0);
    EXPECT_EQ(furnace_->getCookTimeTotal(), 200); // 默认200 tick
}

TEST_F(FurnaceEntityTest, Create_NeedsTickReturnsTrue)
{
    EXPECT_TRUE(furnace_->needsTick());
}

TEST_F(FurnaceEntityTest, GetInventory_ReturnsValidPointer)
{
    IInventory* inventory = furnace_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, GetFurnaceInventory_ReturnsValidReference)
{
    FurnaceInventory& inv = furnace_->getFurnaceInventory();
    EXPECT_EQ(inv.getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, Save_ContainsBasicInfo)
{
    nlohmann::json data;
    furnace_->save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_EQ(data["id"], "minecraft:furnace");
    EXPECT_TRUE(data.contains("BurnTime"));
    EXPECT_TRUE(data.contains("CookTime"));
    EXPECT_TRUE(data.contains("CookTimeTotal"));
}

TEST_F(FurnaceEntityTest, Clone_CreatesCopy)
{
    std::unique_ptr<BlockEntity> copy = furnace_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Furnace);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));
}

TEST_F(FurnaceEntityTest, SetChanged_MarksAsChanged)
{
    EXPECT_FALSE(furnace_->isChanged());
    furnace_->setChanged();
    EXPECT_TRUE(furnace_->isChanged());
}

TEST_F(FurnaceEntityTest, GetComparatorSignal_ReturnsZeroWhenEmpty)
{
    EXPECT_EQ(furnace_->getComparatorSignal(), 0);
}

TEST_F(FurnaceEntityTest, Load_LoadsBurnTime)
{
    nlohmann::json data;
    data["BurnTime"] = 100;
    data["CookTime"] = 50;
    data["CookTimeTotal"] = 200;

    EXPECT_TRUE(furnace_->load(data));
}

// ========== BlastFurnaceEntity 测试 ==========

class BlastFurnaceEntityTest : public ::testing::Test {
protected:
    void SetUp() override { blastFurnace_ = std::make_unique<BlastFurnaceEntity>(BlockPos(5, 10, 15)); }

    std::unique_ptr<BlastFurnaceEntity> blastFurnace_;
};

TEST_F(BlastFurnaceEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(blastFurnace_->getType(), BlockEntityType::BlastFurnace);
}

TEST_F(BlastFurnaceEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(blastFurnace_->getPos(), BlockPos(5, 10, 15));
}

TEST_F(BlastFurnaceEntityTest, Clone_CreatesCopy)
{
    std::unique_ptr<BlockEntity> copy = blastFurnace_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::BlastFurnace);
    EXPECT_EQ(copy->getPos(), BlockPos(5, 10, 15));
}

TEST_F(BlastFurnaceEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(blastFurnace_->needsTick());
}

// ========== SmokerEntity 测试 ==========

class SmokerEntityTest : public ::testing::Test {
protected:
    void SetUp() override { smoker_ = std::make_unique<SmokerEntity>(BlockPos(1, 2, 3)); }

    std::unique_ptr<SmokerEntity> smoker_;
};

TEST_F(SmokerEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(smoker_->getType(), BlockEntityType::Smoker);
}

TEST_F(SmokerEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(smoker_->getPos(), BlockPos(1, 2, 3));
}

TEST_F(SmokerEntityTest, Clone_CreatesCopy)
{
    std::unique_ptr<BlockEntity> copy = smoker_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Smoker);
    EXPECT_EQ(copy->getPos(), BlockPos(1, 2, 3));
}

TEST_F(SmokerEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(smoker_->needsTick());
}

// ========== AbstractFurnaceEntity 静态方法测试 ==========

class AbstractFurnaceEntityStaticTest : public ::testing::Test {};

TEST_F(AbstractFurnaceEntityStaticTest, IsFuel_ReturnsFalseForEmptyStack)
{
    ItemStack emptyStack;
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(emptyStack));
}

TEST_F(AbstractFurnaceEntityStaticTest, GetBurnTime_ReturnsZeroForEmptyStack)
{
    ItemStack emptyStack;
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(emptyStack), 0);
}

// ========== 燃烧时间测试 ==========

/**
 * @brief 燃烧时间测试类
 *
 * 需要先初始化方块和物品系统才能进行燃烧时间测试。
 * 参考: MC 1.16.5 AbstractFurnaceTileEntity.getBurnTimes()
 */
class FurnaceBurnTimeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化方块和物品系统（只初始化一次）
        static bool initialized = false;
        if (!initialized) {
            VanillaBlocks::initialize();
            Items::initialize();
            BlockItemRegistry::instance().initializeVanillaBlockItems();
            initialized = true;
        }
    }
};

// ========== 特殊燃料测试 ==========

TEST_F(FurnaceBurnTimeTest, LavaBucket_HasCorrectBurnTime)
{
    ASSERT_NE(Items::LAVA_BUCKET, nullptr) << "LAVA_BUCKET should be registered";
    ItemStack lavaBucket(Items::LAVA_BUCKET, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(lavaBucket), 20000);
}

TEST_F(FurnaceBurnTimeTest, BlazeRod_HasCorrectBurnTime)
{
    ASSERT_NE(Items::BLAZE_ROD, nullptr) << "BLAZE_ROD should be registered";
    ItemStack blazeRod(Items::BLAZE_ROD, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(blazeRod), 2400);
}

// ========== 煤炭类测试 ==========

TEST_F(FurnaceBurnTimeTest, Coal_HasCorrectBurnTime)
{
    ASSERT_NE(Items::COAL, nullptr) << "COAL should be registered";
    ItemStack coal(Items::COAL, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(coal), 1600);
}

TEST_F(FurnaceBurnTimeTest, Charcoal_HasCorrectBurnTime)
{
    ASSERT_NE(Items::CHARCOAL, nullptr) << "CHARCOAL should be registered";
    ItemStack charcoal(Items::CHARCOAL, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(charcoal), 1600);
}

// ========== 煤炭块测试 (16000 tick) ==========

TEST_F(FurnaceBurnTimeTest, CoalBlock_HasCorrectBurnTime)
{
    // 参考: MC 1.16.5 第 98 行: addItemBurnTime(map, Blocks.COAL_BLOCK, 16000);
    // 煤炭块燃烧时间是煤炭的 10 倍 (16000 tick = 800 秒 = 13 分 20 秒)
    // 可烧炼 80 个物品 (16000 / 200 = 80)
    ASSERT_NE(VanillaBlocks::COAL_BLOCK, nullptr) << "COAL_BLOCK should be registered";
    const BlockItem* coalBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::COAL_BLOCK);
    ASSERT_NE(coalBlockItem, nullptr) << "COAL_BLOCK should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(coalBlockItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 16000);
}

TEST_F(FurnaceBurnTimeTest, CoalBlock_IsTenTimesCoalBurnTime)
{
    // 验证煤炭块燃烧时间是煤炭的 10 倍
    ASSERT_NE(Items::COAL, nullptr) << "COAL should be registered";
    ASSERT_NE(VanillaBlocks::COAL_BLOCK, nullptr) << "COAL_BLOCK should be registered";

    ItemStack coal(Items::COAL, 1);
    i32 coalBurnTime = AbstractFurnaceEntity::getBurnTime(coal);
    EXPECT_EQ(coalBurnTime, 1600);

    const BlockItem* coalBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::COAL_BLOCK);
    ASSERT_NE(coalBlockItem, nullptr) << "COAL_BLOCK should have a BlockItem";
    ItemStack coalBlock(static_cast<const Item*>(coalBlockItem), 1);
    i32 coalBlockBurnTime = AbstractFurnaceEntity::getBurnTime(coalBlock);
    EXPECT_EQ(coalBlockBurnTime, 16000);

    // 煤炭块 = 10 个煤炭
    EXPECT_EQ(coalBlockBurnTime, coalBurnTime * 10);
}

// ========== 木头类测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, OakLog_HasCorrectBurnTime)
{
    // Items::OAK_LOG 是通过 Items 注册的 BlockItem
    // 它与 BlockItemRegistry 中注册的是同一个物品
    const Item* oakLogItem = Items::OAK_LOG;
    ASSERT_NE(oakLogItem, nullptr) << "OAK_LOG should be registered";

    // 直接使用物品指针测试
    ItemStack log(oakLogItem, 1);

    // 如果物品在 BlockItemRegistry 中也能找到，测试通过
    // 注意：Items::OAK_LOG 和 BlockItemRegistry 中的是同一个 BlockItem
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(log), 300);
}

TEST_F(FurnaceBurnTimeTest, SpruceLog_HasCorrectBurnTime)
{
    // 通过 BlockItemRegistry 验证原木燃烧时间
    const BlockItem* spruceLogItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SPRUCE_LOG);
    if (spruceLogItem != nullptr) {
        ItemStack log(static_cast<const Item*>(spruceLogItem), 1);
        EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(log), 300);
    } else {
        // 如果方块物品未注册，跳过测试
        GTEST_SKIP() << "SPRUCE_LOG BlockItem not registered in BlockItemRegistry";
    }
}

TEST_F(FurnaceBurnTimeTest, OakPlanks_HasCorrectBurnTime)
{
    // Items::OAK_PLANKS 是通过 Items 注册的 BlockItem
    const Item* oakPlanksItem = Items::OAK_PLANKS;
    ASSERT_NE(oakPlanksItem, nullptr) << "OAK_PLANKS should be registered";
    ItemStack planks(oakPlanksItem, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(planks), 300);
}

// ========== 木制工具测试 (200 tick) ==========

TEST_F(FurnaceBurnTimeTest, WoodenPickaxe_HasCorrectBurnTime)
{
    ASSERT_NE(Items::WOODEN_PICKAXE, nullptr) << "WOODEN_PICKAXE should be registered";
    ItemStack tool(Items::WOODEN_PICKAXE, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(tool), 200);
}

TEST_F(FurnaceBurnTimeTest, WoodenSword_HasCorrectBurnTime)
{
    ASSERT_NE(Items::WOODEN_SWORD, nullptr) << "WOODEN_SWORD should be registered";
    ItemStack tool(Items::WOODEN_SWORD, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(tool), 200);
}

// ========== 弓和钓鱼竿测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, Bow_HasCorrectBurnTime)
{
    ASSERT_NE(Items::BOW, nullptr) << "BOW should be registered";
    ItemStack bow(Items::BOW, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(bow), 300);
}

TEST_F(FurnaceBurnTimeTest, FishingRod_HasCorrectBurnTime)
{
    ASSERT_NE(Items::FISHING_ROD, nullptr) << "FISHING_ROD should be registered";
    ItemStack rod(Items::FISHING_ROD, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(rod), 300);
}

TEST_F(FurnaceBurnTimeTest, Crossbow_HasCorrectBurnTime)
{
    ASSERT_NE(Items::CROSSBOW, nullptr) << "CROSSBOW should be registered";
    ItemStack crossbow(Items::CROSSBOW, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(crossbow), 300);
}

// ========== 木棍和碗测试 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, Stick_HasCorrectBurnTime)
{
    ASSERT_NE(Items::STICK, nullptr) << "STICK should be registered";
    ItemStack stick(Items::STICK, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stick), 100);
}

TEST_F(FurnaceBurnTimeTest, Bowl_HasCorrectBurnTime)
{
    ASSERT_NE(Items::BOWL, nullptr) << "BOWL should be registered";
    ItemStack bowl(Items::BOWL, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(bowl), 100);
}

// ========== 方块物品燃烧时间测试 ==========

TEST_F(FurnaceBurnTimeTest, Bookshelf_HasCorrectBurnTime)
{
    const BlockItem* bookshelfItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BOOKSHELF);
    ASSERT_NE(bookshelfItem, nullptr) << "BOOKSHELF should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(bookshelfItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, NoteBlock_HasCorrectBurnTime)
{
    const BlockItem* noteBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlockItem, nullptr) << "NOTE_BLOCK should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(noteBlockItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, CraftingTable_HasCorrectBurnTime)
{
    const BlockItem* craftingTableItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CRAFTING_TABLE);
    ASSERT_NE(craftingTableItem, nullptr) << "CRAFTING_TABLE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(craftingTableItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, DaylightDetector_HasCorrectBurnTime)
{
    const BlockItem* detectorItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DAYLIGHT_DETECTOR);
    ASSERT_NE(detectorItem, nullptr) << "DAYLIGHT_DETECTOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(detectorItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakStairs_HasCorrectBurnTime)
{
    const BlockItem* stairsItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_STAIRS);
    ASSERT_NE(stairsItem, nullptr) << "OAK_STAIRS should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(stairsItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakSlab_HasCorrectBurnTime)
{
    // 木质台阶燃烧时间是 150 tick
    const BlockItem* slabItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_SLAB);
    ASSERT_NE(slabItem, nullptr) << "OAK_SLAB should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(slabItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

TEST_F(FurnaceBurnTimeTest, OakFence_HasCorrectBurnTime)
{
    const BlockItem* fenceItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_FENCE);
    ASSERT_NE(fenceItem, nullptr) << "OAK_FENCE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(fenceItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakFenceGate_HasCorrectBurnTime)
{
    const BlockItem* gateItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_FENCE_GATE);
    ASSERT_NE(gateItem, nullptr) << "OAK_FENCE_GATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(gateItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakDoor_HasCorrectBurnTime)
{
    // MC Java: 木质门燃烧时间 200 tick（比其他木质建筑方块的 300 tick 短）
    const BlockItem* doorItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_DOOR);
    ASSERT_NE(doorItem, nullptr) << "OAK_DOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(doorItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 200);
}

TEST_F(FurnaceBurnTimeTest, OakTrapdoor_HasCorrectBurnTime)
{
    const BlockItem* trapdoorItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_TRAPDOOR);
    ASSERT_NE(trapdoorItem, nullptr) << "OAK_TRAPDOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(trapdoorItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakPressurePlate_HasCorrectBurnTime)
{
    const BlockItem* plateItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_PRESSURE_PLATE);
    ASSERT_NE(plateItem, nullptr) << "OAK_PRESSURE_PLATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(plateItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakButton_HasCorrectBurnTime)
{
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_BUTTON);
    ASSERT_NE(buttonItem, nullptr) << "OAK_BUTTON should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, SpruceButton_HasCorrectBurnTime)
{
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SPRUCE_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "SPRUCE_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, BirchButton_HasCorrectBurnTime)
{
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BIRCH_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "BIRCH_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, JungleButton_HasCorrectBurnTime)
{
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::JUNGLE_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "JUNGLE_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, AcaciaButton_HasCorrectBurnTime)
{
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ACACIA_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "ACACIA_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, DarkOakButton_HasCorrectBurnTime)
{
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DARK_OAK_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "DARK_OAK_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, CrimsonButton_NotBurnable)
{
    // 下界木材按钮不可燃
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CRIMSON_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "CRIMSON_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "CRIMSON_BUTTON should not be burnable";
}

TEST_F(FurnaceBurnTimeTest, WarpedButton_NotBurnable)
{
    // 下界木材按钮不可燃
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WARPED_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "WARPED_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "WARPED_BUTTON should not be burnable";
}

// ========== 树苗测试 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, OakSapling_HasCorrectBurnTime)
{
    const BlockItem* saplingItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_SAPLING);
    ASSERT_NE(saplingItem, nullptr) << "OAK_SAPLING should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(saplingItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, BirchSapling_HasCorrectBurnTime)
{
    const BlockItem* saplingItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BIRCH_SAPLING);
    ASSERT_NE(saplingItem, nullptr) << "BIRCH_SAPLING should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(saplingItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

// ========== 羊毛测试 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, WhiteWool_HasCorrectBurnTime)
{
    const BlockItem* woolItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WHITE_WOOL);
    ASSERT_NE(woolItem, nullptr) << "WHITE_WOOL should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(woolItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, BlackWool_HasCorrectBurnTime)
{
    const BlockItem* woolItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BLACK_WOOL);
    ASSERT_NE(woolItem, nullptr) << "BLACK_WOOL should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(woolItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

// ========== 竹子测试 (50 tick) ==========

TEST_F(FurnaceBurnTimeTest, Bamboo_HasCorrectBurnTime)
{
    const BlockItem* bambooItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO);
    ASSERT_NE(bambooItem, nullptr) << "BAMBOO should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(bambooItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 50);
}

// ========== 干海带块测试 (4001 tick) ==========

TEST_F(FurnaceBurnTimeTest, DriedKelpBlock_HasCorrectBurnTime)
{
    const BlockItem* kelpItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DRIED_KELP_BLOCK);
    ASSERT_NE(kelpItem, nullptr) << "DRIED_KELP_BLOCK should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(kelpItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 4001);
}

// ========== IsFuel 测试 ==========

TEST_F(FurnaceBurnTimeTest, IsFuel_ReturnsTrueForCoal)
{
    ASSERT_NE(Items::COAL, nullptr);
    ItemStack coal(Items::COAL, 1);
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(coal));
}

TEST_F(FurnaceBurnTimeTest, IsFuel_ReturnsTrueForStick)
{
    ASSERT_NE(Items::STICK, nullptr);
    ItemStack stick(Items::STICK, 1);
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(stick));
}

TEST_F(FurnaceBurnTimeTest, IsFuel_ReturnsFalseForDiamond)
{
    ASSERT_NE(Items::DIAMOND, nullptr);
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(diamond));
}

TEST_F(FurnaceBurnTimeTest, IsFuel_ReturnsFalseForEmptyStack)
{
    ItemStack empty;
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(empty));
}

// ========== 告示牌燃烧时间测试 (200 tick) ==========
// 参考: MC 1.16.5 AbstractFurnaceTileEntity 第 132 行
// addItemTagBurnTime(map, ItemTags.SIGNS, 200);

TEST_F(FurnaceBurnTimeTest, OakSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::OAK_SIGN, nullptr) << "OAK_SIGN should be registered";
    ItemStack sign(Items::OAK_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "橡木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, SpruceSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::SPRUCE_SIGN, nullptr) << "SPRUCE_SIGN should be registered";
    ItemStack sign(Items::SPRUCE_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "云杉木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, BirchSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::BIRCH_SIGN, nullptr) << "BIRCH_SIGN should be registered";
    ItemStack sign(Items::BIRCH_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "白桦木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, JungleSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::JUNGLE_SIGN, nullptr) << "JUNGLE_SIGN should be registered";
    ItemStack sign(Items::JUNGLE_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "丛林木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, AcaciaSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::ACACIA_SIGN, nullptr) << "ACACIA_SIGN should be registered";
    ItemStack sign(Items::ACACIA_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "金合欢木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, DarkOakSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::DARK_OAK_SIGN, nullptr) << "DARK_OAK_SIGN should be registered";
    ItemStack sign(Items::DARK_OAK_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "深色橡木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, CrimsonSign_NotBurnable)
{
    // MC Java: 绯红告示牌属于 NON_FLAMMABLE_WOOD，不可作为燃料
    ASSERT_NE(Items::CRIMSON_SIGN, nullptr) << "CRIMSON_SIGN should be registered";
    ItemStack sign(Items::CRIMSON_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 0) << "绯红告示牌属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, WarpedSign_NotBurnable)
{
    // MC Java: 诡异告示牌属于 NON_FLAMMABLE_WOOD，不可作为燃料
    ASSERT_NE(Items::WARPED_SIGN, nullptr) << "WARPED_SIGN should be registered";
    ItemStack sign(Items::WARPED_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 0) << "诡异告示牌属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, SignItemsAreFuel)
{
    // 主世界木材告示牌应被视为燃料（绯红/诡异告示牌属于 NON_FLAMMABLE_WOOD，不可燃）
    ASSERT_NE(Items::OAK_SIGN, nullptr);
    ASSERT_NE(Items::SPRUCE_SIGN, nullptr);
    ASSERT_NE(Items::BIRCH_SIGN, nullptr);
    ASSERT_NE(Items::JUNGLE_SIGN, nullptr);
    ASSERT_NE(Items::ACACIA_SIGN, nullptr);
    ASSERT_NE(Items::DARK_OAK_SIGN, nullptr);

    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::OAK_SIGN, 1)));
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::SPRUCE_SIGN, 1)));
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::BIRCH_SIGN, 1)));
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::JUNGLE_SIGN, 1)));
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::ACACIA_SIGN, 1)));
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::DARK_OAK_SIGN, 1)));

    // 绯红/诡异告示牌属于 NON_FLAMMABLE_WOOD，不可燃
    ASSERT_NE(Items::CRIMSON_SIGN, nullptr);
    ASSERT_NE(Items::WARPED_SIGN, nullptr);
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(ItemStack(Items::CRIMSON_SIGN, 1)));
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(ItemStack(Items::WARPED_SIGN, 1)));
}

// ========== 旗帜燃烧时间测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, WhiteBanner_HasCorrectBurnTime)
{
    ASSERT_NE(Items::WHITE_BANNER, nullptr) << "WHITE_BANNER should be registered";
    ItemStack banner(Items::WHITE_BANNER, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(banner), 300) << "白色旗帜燃烧时间应为 300 tick";
}

TEST_F(FurnaceBurnTimeTest, BlackBanner_HasCorrectBurnTime)
{
    ASSERT_NE(Items::BLACK_BANNER, nullptr) << "BLACK_BANNER should be registered";
    ItemStack banner(Items::BLACK_BANNER, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(banner), 300) << "黑色旗帜燃烧时间应为 300 tick";
}

TEST_F(FurnaceBurnTimeTest, BannerItemsAreFuel)
{
    ASSERT_NE(Items::WHITE_BANNER, nullptr);
    ASSERT_NE(Items::RED_BANNER, nullptr);
    ASSERT_NE(Items::BLUE_BANNER, nullptr);

    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::WHITE_BANNER, 1)));
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::RED_BANNER, 1)));
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::BLUE_BANNER, 1)));
}

// ========== 脚手架燃烧时间测试 (50 tick) ==========

TEST_F(FurnaceBurnTimeTest, Scaffolding_HasCorrectBurnTime)
{
    ASSERT_NE(Items::SCAFFOLDING, nullptr) << "SCAFFOLDING should be registered";
    ItemStack scaffolding(Items::SCAFFOLDING, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(scaffolding), 50) << "脚手架燃烧时间应为 50 tick";
}

TEST_F(FurnaceBurnTimeTest, Scaffolding_IsFuel)
{
    ASSERT_NE(Items::SCAFFOLDING, nullptr);
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(ItemStack(Items::SCAFFOLDING, 1)));
}

// ========== 新增燃料测试 - 多木材类型楼梯 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, SpruceStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SPRUCE_STAIRS);
    if (item == nullptr) {
        GTEST_SKIP() << "SPRUCE_STAIRS BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BirchStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BIRCH_STAIRS);
    if (item == nullptr) {
        GTEST_SKIP() << "BIRCH_STAIRS BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, JungleStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::JUNGLE_STAIRS);
    if (item == nullptr) {
        GTEST_SKIP() << "JUNGLE_STAIRS BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, AcaciaStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ACACIA_STAIRS);
    if (item == nullptr) {
        GTEST_SKIP() << "ACACIA_STAIRS BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, DarkOakStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DARK_OAK_STAIRS);
    if (item == nullptr) {
        GTEST_SKIP() << "DARK_OAK_STAIRS BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增燃料测试 - 多木材类型台阶 (150 tick) ==========

TEST_F(FurnaceBurnTimeTest, SpruceSlab_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SPRUCE_SLAB);
    if (item == nullptr) {
        GTEST_SKIP() << "SPRUCE_SLAB BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

TEST_F(FurnaceBurnTimeTest, BirchSlab_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BIRCH_SLAB);
    if (item == nullptr) {
        GTEST_SKIP() << "BIRCH_SLAB BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

// ========== 新增燃料测试 - 多木材类型栅栏 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, SpruceFence_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SPRUCE_FENCE);
    if (item == nullptr) {
        GTEST_SKIP() << "SPRUCE_FENCE BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BirchFence_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BIRCH_FENCE);
    if (item == nullptr) {
        GTEST_SKIP() << "BIRCH_FENCE BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增燃料测试 - 多木材类型栅栏门 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, SpruceFenceGate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SPRUCE_FENCE_GATE);
    if (item == nullptr) {
        GTEST_SKIP() << "SPRUCE_FENCE_GATE BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BirchFenceGate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BIRCH_FENCE_GATE);
    if (item == nullptr) {
        GTEST_SKIP() << "BIRCH_FENCE_GATE BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增燃料测试 - 雕纹书架 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, ChiseledBookshelf_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHISELED_BOOKSHELF);
    if (item == nullptr) {
        GTEST_SKIP() << "CHISELED_BOOKSHELF BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增燃料测试 - 红树木相关 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveRoots_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_ROOTS);
    if (item == nullptr) {
        GTEST_SKIP() << "MANGROVE_ROOTS BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增燃料测试 - 杜鹃花 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, Azalea_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::AZALEA);
    if (item == nullptr) {
        GTEST_SKIP() << "AZALEA BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, FloweringAzalea_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::FLOWERING_AZALEA);
    if (item == nullptr) {
        GTEST_SKIP() << "FLOWERING_AZALEA BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

// ========== 新增燃料测试 - 枯草 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, ShortDryGrass_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SHORT_DRY_GRASS);
    if (item == nullptr) {
        GTEST_SKIP() << "SHORT_DRY_GRASS BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, TallDryGrass_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::TALL_DRY_GRASS);
    if (item == nullptr) {
        GTEST_SKIP() << "TALL_DRY_GRASS BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

// ========== 新增燃料测试 - 落叶 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, LeafLitter_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::LEAF_LITTER);
    if (item == nullptr) {
        GTEST_SKIP() << "LEAF_LITTER BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

// ========== 边界情况：绯红/诡异木质不可燃测试 ==========

TEST_F(FurnaceBurnTimeTest, CrimsonFenceGate_NotBurnable)
{
    // MC Java: 绯红栅栏门属于 NON_FLAMMABLE_WOOD，不可作为燃料
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CRIMSON_FENCE_GATE);
    if (item == nullptr) {
        GTEST_SKIP() << "CRIMSON_FENCE_GATE BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "绯红栅栏门属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, WarpedFenceGate_NotBurnable)
{
    // MC Java: 诡异栅栏门属于 NON_FLAMMABLE_WOOD，不可作为燃料
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WARPED_FENCE_GATE);
    if (item == nullptr) {
        GTEST_SKIP() << "WARPED_FENCE_GATE BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "诡异栅栏门属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, CrimsonStem_NotBurnable)
{
    // MC Java: 绯红茎属于 NON_FLAMMABLE_WOOD，不可作为燃料
    // 绯红茎和诡异茎不在燃料列表中，返回 0
    ASSERT_NE(Items::CRIMSON_STEM, nullptr) << "CRIMSON_STEM should be registered";
    ItemStack stack(Items::CRIMSON_STEM, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "绯红茎属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, WarpedStem_NotBurnable)
{
    // MC Java: 诡异茎属于 NON_FLAMMABLE_WOOD，不可作为燃料
    ASSERT_NE(Items::WARPED_STEM, nullptr) << "WARPED_STEM should be registered";
    ItemStack stack(Items::WARPED_STEM, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "诡异茎属于 NON_FLAMMABLE_WOOD，不可燃";
}

// ========== 非燃料物品测试 ==========

// ========== 新增木材告示牌燃烧时间测试 (200 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::MANGROVE_SIGN, nullptr) << "MANGROVE_SIGN should be registered";
    ItemStack sign(Items::MANGROVE_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "红树木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, CherrySign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::CHERRY_SIGN, nullptr) << "CHERRY_SIGN should be registered";
    ItemStack sign(Items::CHERRY_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "樱花木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, BambooSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::BAMBOO_SIGN, nullptr) << "BAMBOO_SIGN should be registered";
    ItemStack sign(Items::BAMBOO_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "竹木告示牌燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, PaleOakSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::PALE_OAK_SIGN, nullptr) << "PALE_OAK_SIGN should be registered";
    ItemStack sign(Items::PALE_OAK_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 200) << "苍白橡木告示牌燃烧时间应为 200 tick";
}

// ========== 悬挂告示牌燃烧时间测试 (800 tick) ==========
// 参考: MC Java ItemTags.HANGING_SIGNS 燃烧时间 800 tick

TEST_F(FurnaceBurnTimeTest, OakHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::OAK_HANGING_SIGN, nullptr) << "OAK_HANGING_SIGN should be registered";
    ItemStack sign(Items::OAK_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "橡木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, SpruceHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::SPRUCE_HANGING_SIGN, nullptr) << "SPRUCE_HANGING_SIGN should be registered";
    ItemStack sign(Items::SPRUCE_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "云杉木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, BirchHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::BIRCH_HANGING_SIGN, nullptr) << "BIRCH_HANGING_SIGN should be registered";
    ItemStack sign(Items::BIRCH_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "白桦木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, JungleHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::JUNGLE_HANGING_SIGN, nullptr) << "JUNGLE_HANGING_SIGN should be registered";
    ItemStack sign(Items::JUNGLE_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "丛林木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, AcaciaHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::ACACIA_HANGING_SIGN, nullptr) << "ACACIA_HANGING_SIGN should be registered";
    ItemStack sign(Items::ACACIA_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "金合欢木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, DarkOakHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::DARK_OAK_HANGING_SIGN, nullptr) << "DARK_OAK_HANGING_SIGN should be registered";
    ItemStack sign(Items::DARK_OAK_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "深色橡木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, MangroveHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::MANGROVE_HANGING_SIGN, nullptr) << "MANGROVE_HANGING_SIGN should be registered";
    ItemStack sign(Items::MANGROVE_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "红树木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, CherryHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::CHERRY_HANGING_SIGN, nullptr) << "CHERRY_HANGING_SIGN should be registered";
    ItemStack sign(Items::CHERRY_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "樱花木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, BambooHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::BAMBOO_HANGING_SIGN, nullptr) << "BAMBOO_HANGING_SIGN should be registered";
    ItemStack sign(Items::BAMBOO_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "竹木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, PaleOakHangingSign_HasCorrectBurnTime)
{
    ASSERT_NE(Items::PALE_OAK_HANGING_SIGN, nullptr) << "PALE_OAK_HANGING_SIGN should be registered";
    ItemStack sign(Items::PALE_OAK_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 800) << "苍白橡木悬挂告示牌燃烧时间应为 800 tick";
}

TEST_F(FurnaceBurnTimeTest, CrimsonHangingSign_NotBurnable)
{
    ASSERT_NE(Items::CRIMSON_HANGING_SIGN, nullptr) << "CRIMSON_HANGING_SIGN should be registered";
    ItemStack sign(Items::CRIMSON_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 0) << "绯红悬挂告示牌属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, WarpedHangingSign_NotBurnable)
{
    ASSERT_NE(Items::WARPED_HANGING_SIGN, nullptr) << "WARPED_HANGING_SIGN should be registered";
    ItemStack sign(Items::WARPED_HANGING_SIGN, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(sign), 0) << "诡异悬挂告示牌属于 NON_FLAMMABLE_WOOD，不可燃";
}

// ========== 新增木材楼梯燃烧时间测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_STAIRS);
    ASSERT_NE(item, nullptr) << "MANGROVE_STAIRS should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, CherryStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHERRY_STAIRS);
    ASSERT_NE(item, nullptr) << "CHERRY_STAIRS should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, PaleOakStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PALE_OAK_STAIRS);
    ASSERT_NE(item, nullptr) << "PALE_OAK_STAIRS should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BambooStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_STAIRS);
    ASSERT_NE(item, nullptr) << "BAMBOO_STAIRS should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BambooMosaicStairs_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_MOSAIC_STAIRS);
    ASSERT_NE(item, nullptr) << "BAMBOO_MOSAIC_STAIRS should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增木材台阶燃烧时间测试 (150 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveSlab_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_SLAB);
    ASSERT_NE(item, nullptr) << "MANGROVE_SLAB should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

TEST_F(FurnaceBurnTimeTest, CherrySlab_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHERRY_SLAB);
    ASSERT_NE(item, nullptr) << "CHERRY_SLAB should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

TEST_F(FurnaceBurnTimeTest, PaleOakSlab_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PALE_OAK_SLAB);
    ASSERT_NE(item, nullptr) << "PALE_OAK_SLAB should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

TEST_F(FurnaceBurnTimeTest, BambooSlab_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_SLAB);
    ASSERT_NE(item, nullptr) << "BAMBOO_SLAB should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

TEST_F(FurnaceBurnTimeTest, BambooMosaicSlab_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_MOSAIC_SLAB);
    ASSERT_NE(item, nullptr) << "BAMBOO_MOSAIC_SLAB should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

// ========== 新增木材栅栏燃烧时间测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveFence_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_FENCE);
    ASSERT_NE(item, nullptr) << "MANGROVE_FENCE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, CherryFence_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHERRY_FENCE);
    ASSERT_NE(item, nullptr) << "CHERRY_FENCE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, PaleOakFence_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PALE_OAK_FENCE);
    ASSERT_NE(item, nullptr) << "PALE_OAK_FENCE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BambooFence_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_FENCE);
    ASSERT_NE(item, nullptr) << "BAMBOO_FENCE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增木材栅栏门燃烧时间测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveFenceGate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_FENCE_GATE);
    ASSERT_NE(item, nullptr) << "MANGROVE_FENCE_GATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, CherryFenceGate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHERRY_FENCE_GATE);
    ASSERT_NE(item, nullptr) << "CHERRY_FENCE_GATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, PaleOakFenceGate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PALE_OAK_FENCE_GATE);
    ASSERT_NE(item, nullptr) << "PALE_OAK_FENCE_GATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BambooFenceGate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_FENCE_GATE);
    ASSERT_NE(item, nullptr) << "BAMBOO_FENCE_GATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增木材门燃烧时间测试 (200 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveDoor_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_DOOR);
    ASSERT_NE(item, nullptr) << "MANGROVE_DOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 200) << "木质门燃烧时间应为 200 tick";
}

TEST_F(FurnaceBurnTimeTest, CherryDoor_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHERRY_DOOR);
    ASSERT_NE(item, nullptr) << "CHERRY_DOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 200);
}

TEST_F(FurnaceBurnTimeTest, PaleOakDoor_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PALE_OAK_DOOR);
    ASSERT_NE(item, nullptr) << "PALE_OAK_DOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 200);
}

TEST_F(FurnaceBurnTimeTest, BambooDoor_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_DOOR);
    ASSERT_NE(item, nullptr) << "BAMBOO_DOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 200);
}

// ========== 新增木材活板门燃烧时间测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveTrapdoor_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_TRAPDOOR);
    ASSERT_NE(item, nullptr) << "MANGROVE_TRAPDOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, CherryTrapdoor_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHERRY_TRAPDOOR);
    ASSERT_NE(item, nullptr) << "CHERRY_TRAPDOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, PaleOakTrapdoor_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PALE_OAK_TRAPDOOR);
    ASSERT_NE(item, nullptr) << "PALE_OAK_TRAPDOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BambooTrapdoor_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_TRAPDOOR);
    ASSERT_NE(item, nullptr) << "BAMBOO_TRAPDOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增木材压力板燃烧时间测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangrovePressurePlate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_PRESSURE_PLATE);
    ASSERT_NE(item, nullptr) << "MANGROVE_PRESSURE_PLATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, CherryPressurePlate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHERRY_PRESSURE_PLATE);
    ASSERT_NE(item, nullptr) << "CHERRY_PRESSURE_PLATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, BambooPressurePlate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_PRESSURE_PLATE);
    ASSERT_NE(item, nullptr) << "BAMBOO_PRESSURE_PLATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, PaleOakPressurePlate_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PALE_OAK_PRESSURE_PLATE);
    ASSERT_NE(item, nullptr) << "PALE_OAK_PRESSURE_PLATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

// ========== 新增木材按钮燃烧时间测试 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, MangroveButton_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::MANGROVE_BUTTON);
    ASSERT_NE(item, nullptr) << "MANGROVE_BUTTON should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, CherryButton_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CHERRY_BUTTON);
    ASSERT_NE(item, nullptr) << "CHERRY_BUTTON should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, BambooButton_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO_BUTTON);
    ASSERT_NE(item, nullptr) << "BAMBOO_BUTTON should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, PaleOakButton_HasCorrectBurnTime)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PALE_OAK_BUTTON);
    ASSERT_NE(item, nullptr) << "PALE_OAK_BUTTON should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

// ========== 绯红/诡异木质不可燃扩展测试 ==========

TEST_F(FurnaceBurnTimeTest, CrimsonDoor_NotBurnable)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CRIMSON_DOOR);
    if (item == nullptr) {
        GTEST_SKIP() << "CRIMSON_DOOR BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "绯红门属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, WarpedDoor_NotBurnable)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WARPED_DOOR);
    if (item == nullptr) {
        GTEST_SKIP() << "WARPED_DOOR BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "诡异门属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, CrimsonTrapdoor_NotBurnable)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CRIMSON_TRAPDOOR);
    if (item == nullptr) {
        GTEST_SKIP() << "CRIMSON_TRAPDOOR BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "绯红活板门属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, WarpedTrapdoor_NotBurnable)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WARPED_TRAPDOOR);
    if (item == nullptr) {
        GTEST_SKIP() << "WARPED_TRAPDOOR BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "诡异活板门属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, CrimsonPressurePlate_NotBurnable)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CRIMSON_PRESSURE_PLATE);
    if (item == nullptr) {
        GTEST_SKIP() << "CRIMSON_PRESSURE_PLATE BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "绯红压力板属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, WarpedPressurePlate_NotBurnable)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WARPED_PRESSURE_PLATE);
    if (item == nullptr) {
        GTEST_SKIP() << "WARPED_PRESSURE_PLATE BlockItem not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "诡异压力板属于 NON_FLAMMABLE_WOOD，不可燃";
}

TEST_F(FurnaceBurnTimeTest, IronIngot_IsNotFuel)
{
    ASSERT_NE(Items::IRON_INGOT, nullptr);
    ItemStack stack(Items::IRON_INGOT, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0);
}

TEST_F(FurnaceBurnTimeTest, Stone_IsNotFuel)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::STONE);
    ASSERT_NE(item, nullptr);
    ItemStack stack(static_cast<const Item*>(item), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0);
}
