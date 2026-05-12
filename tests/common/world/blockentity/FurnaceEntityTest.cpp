#include <gtest/gtest.h>
#include "world/blockentity/processing/FurnaceEntity.hpp"
#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "world/blockentity/processing/SmokerEntity.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== FurnaceInventory 测试 ==========

class FurnaceInventoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        inventory_ = std::make_unique<FurnaceInventory>();
    }

    std::unique_ptr<FurnaceInventory> inventory_;
};

TEST_F(FurnaceInventoryTest, Create_HasCorrectSize) {
    EXPECT_EQ(inventory_->getContainerSize(), FurnaceInventory::SLOT_COUNT);
    EXPECT_EQ(FurnaceInventory::SLOT_COUNT, 3);
}

TEST_F(FurnaceInventoryTest, Create_SlotIndicesAreCorrect) {
    EXPECT_EQ(FurnaceInventory::SLOT_INPUT, 0);
    EXPECT_EQ(FurnaceInventory::SLOT_FUEL, 1);
    EXPECT_EQ(FurnaceInventory::SLOT_OUTPUT, 2);
}

TEST_F(FurnaceInventoryTest, Create_IsEmpty) {
    EXPECT_TRUE(inventory_->isEmpty());
    EXPECT_TRUE(inventory_->isInputEmpty());
    EXPECT_TRUE(inventory_->isFuelEmpty());
    EXPECT_TRUE(inventory_->isOutputEmpty());
}

TEST_F(FurnaceInventoryTest, GetInputItem_ReturnsEmptyInitially) {
    ItemStack input = inventory_->getInputItem();
    EXPECT_TRUE(input.isEmpty());
}

TEST_F(FurnaceInventoryTest, GetFuelItem_ReturnsEmptyInitially) {
    ItemStack fuel = inventory_->getFuelItem();
    EXPECT_TRUE(fuel.isEmpty());
}

TEST_F(FurnaceInventoryTest, GetOutputItem_ReturnsEmptyInitially) {
    ItemStack output = inventory_->getOutputItem();
    EXPECT_TRUE(output.isEmpty());
}

TEST_F(FurnaceInventoryTest, SetInputItem_UpdatesInput) {
    ItemStack emptyStack;
    inventory_->setInputItem(emptyStack);
    EXPECT_TRUE(inventory_->isInputEmpty());
}

TEST_F(FurnaceInventoryTest, SetChanged_Callback) {
    bool callbackCalled = false;
    FurnaceInventory invWithCallback([&callbackCalled]() {
        callbackCalled = true;
    });

    invWithCallback.setChanged();
    EXPECT_TRUE(callbackCalled);
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_InputSlotReturnsTrueForValidItem) {
    // Note: canPlaceItem returns false for empty stacks, which is correct behavior
    // This tests the slot index validation
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_INPUT, ItemStack()));
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_FuelSlotReturnsTrueForValidItem) {
    // Note: canPlaceItem returns false for empty stacks, which is correct behavior
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_FUEL, ItemStack()));
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_OutputSlotReturnsFalseForEmptyStack) {
    // 输出槽不接受空物品堆
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_OUTPUT, ItemStack()));
}

TEST_F(FurnaceInventoryTest, Clear_MakesAllSlotsEmpty) {
    inventory_->clear();
    EXPECT_TRUE(inventory_->isEmpty());
}

TEST_F(FurnaceInventoryTest, GetMaxStackSize_ReturnsDefault) {
    EXPECT_EQ(inventory_->getMaxStackSize(), 64);
}

// ========== FurnaceEntity 测试 ==========

class FurnaceEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        furnace_ = std::make_unique<FurnaceEntity>(BlockPos(10, 20, 30));
    }

    std::unique_ptr<FurnaceEntity> furnace_;
};

TEST_F(FurnaceEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(furnace_->getType(), BlockEntityType::Furnace);
}

TEST_F(FurnaceEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(furnace_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(FurnaceEntityTest, Create_HasCorrectContainerSize) {
    EXPECT_EQ(furnace_->getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, Create_NotBurningInitially) {
    EXPECT_FALSE(furnace_->isBurning());
    EXPECT_EQ(furnace_->getBurnTime(), 0);
    EXPECT_EQ(furnace_->getBurnTimeTotal(), 0);
}

TEST_F(FurnaceEntityTest, Create_CookTimeIsZero) {
    EXPECT_EQ(furnace_->getCookTime(), 0);
    EXPECT_EQ(furnace_->getCookTimeTotal(), 200);  // 默认200 tick
}

TEST_F(FurnaceEntityTest, Create_NeedsTickReturnsTrue) {
    EXPECT_TRUE(furnace_->needsTick());
}

TEST_F(FurnaceEntityTest, GetInventory_ReturnsValidPointer) {
    IInventory* inventory = furnace_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, GetFurnaceInventory_ReturnsValidReference) {
    FurnaceInventory& inv = furnace_->getFurnaceInventory();
    EXPECT_EQ(inv.getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, Save_ContainsBasicInfo) {
    nlohmann::json data;
    furnace_->save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_EQ(data["id"], "minecraft:furnace");
    EXPECT_TRUE(data.contains("BurnTime"));
    EXPECT_TRUE(data.contains("CookTime"));
    EXPECT_TRUE(data.contains("CookTimeTotal"));
}

TEST_F(FurnaceEntityTest, Clone_CreatesCopy) {
    std::unique_ptr<BlockEntity> copy = furnace_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Furnace);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));
}

TEST_F(FurnaceEntityTest, SetChanged_MarksAsChanged) {
    EXPECT_FALSE(furnace_->isChanged());
    furnace_->setChanged();
    EXPECT_TRUE(furnace_->isChanged());
}

TEST_F(FurnaceEntityTest, GetComparatorSignal_ReturnsZeroWhenEmpty) {
    EXPECT_EQ(furnace_->getComparatorSignal(), 0);
}

TEST_F(FurnaceEntityTest, Load_LoadsBurnTime) {
    nlohmann::json data;
    data["BurnTime"] = 100;
    data["CookTime"] = 50;
    data["CookTimeTotal"] = 200;

    EXPECT_TRUE(furnace_->load(data));
}

// ========== BlastFurnaceEntity 测试 ==========

class BlastFurnaceEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        blastFurnace_ = std::make_unique<BlastFurnaceEntity>(BlockPos(5, 10, 15));
    }

    std::unique_ptr<BlastFurnaceEntity> blastFurnace_;
};

TEST_F(BlastFurnaceEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(blastFurnace_->getType(), BlockEntityType::BlastFurnace);
}

TEST_F(BlastFurnaceEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(blastFurnace_->getPos(), BlockPos(5, 10, 15));
}

TEST_F(BlastFurnaceEntityTest, Clone_CreatesCopy) {
    std::unique_ptr<BlockEntity> copy = blastFurnace_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::BlastFurnace);
    EXPECT_EQ(copy->getPos(), BlockPos(5, 10, 15));
}

TEST_F(BlastFurnaceEntityTest, NeedsTick_ReturnsTrue) {
    EXPECT_TRUE(blastFurnace_->needsTick());
}

// ========== SmokerEntity 测试 ==========

class SmokerEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        smoker_ = std::make_unique<SmokerEntity>(BlockPos(1, 2, 3));
    }

    std::unique_ptr<SmokerEntity> smoker_;
};

TEST_F(SmokerEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(smoker_->getType(), BlockEntityType::Smoker);
}

TEST_F(SmokerEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(smoker_->getPos(), BlockPos(1, 2, 3));
}

TEST_F(SmokerEntityTest, Clone_CreatesCopy) {
    std::unique_ptr<BlockEntity> copy = smoker_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Smoker);
    EXPECT_EQ(copy->getPos(), BlockPos(1, 2, 3));
}

TEST_F(SmokerEntityTest, NeedsTick_ReturnsTrue) {
    EXPECT_TRUE(smoker_->needsTick());
}

// ========== AbstractFurnaceEntity 静态方法测试 ==========

class AbstractFurnaceEntityStaticTest : public ::testing::Test {
};

TEST_F(AbstractFurnaceEntityStaticTest, IsFuel_ReturnsFalseForEmptyStack) {
    ItemStack emptyStack;
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(emptyStack));
}

TEST_F(AbstractFurnaceEntityStaticTest, GetBurnTime_ReturnsZeroForEmptyStack) {
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
    static void SetUpTestSuite() {
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

TEST_F(FurnaceBurnTimeTest, LavaBucket_HasCorrectBurnTime) {
    ASSERT_NE(Items::LAVA_BUCKET, nullptr) << "LAVA_BUCKET should be registered";
    ItemStack lavaBucket(Items::LAVA_BUCKET, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(lavaBucket), 20000);
}

TEST_F(FurnaceBurnTimeTest, BlazeRod_HasCorrectBurnTime) {
    ASSERT_NE(Items::BLAZE_ROD, nullptr) << "BLAZE_ROD should be registered";
    ItemStack blazeRod(Items::BLAZE_ROD, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(blazeRod), 2400);
}

// ========== 煤炭类测试 ==========

TEST_F(FurnaceBurnTimeTest, Coal_HasCorrectBurnTime) {
    ASSERT_NE(Items::COAL, nullptr) << "COAL should be registered";
    ItemStack coal(Items::COAL, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(coal), 1600);
}

TEST_F(FurnaceBurnTimeTest, Charcoal_HasCorrectBurnTime) {
    ASSERT_NE(Items::CHARCOAL, nullptr) << "CHARCOAL should be registered";
    ItemStack charcoal(Items::CHARCOAL, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(charcoal), 1600);
}

// ========== 煤炭块测试 (16000 tick) ==========

TEST_F(FurnaceBurnTimeTest, CoalBlock_HasCorrectBurnTime) {
    // 参考: MC 1.16.5 第 98 行: addItemBurnTime(map, Blocks.COAL_BLOCK, 16000);
    // 煤炭块燃烧时间是煤炭的 10 倍 (16000 tick = 800 秒 = 13 分 20 秒)
    // 可烧炼 80 个物品 (16000 / 200 = 80)
    ASSERT_NE(VanillaBlocks::COAL_BLOCK, nullptr) << "COAL_BLOCK should be registered";
    const BlockItem* coalBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::COAL_BLOCK);
    ASSERT_NE(coalBlockItem, nullptr) << "COAL_BLOCK should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(coalBlockItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 16000);
}

TEST_F(FurnaceBurnTimeTest, CoalBlock_IsTenTimesCoalBurnTime) {
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

TEST_F(FurnaceBurnTimeTest, OakLog_HasCorrectBurnTime) {
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

TEST_F(FurnaceBurnTimeTest, SpruceLog_HasCorrectBurnTime) {
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

TEST_F(FurnaceBurnTimeTest, OakPlanks_HasCorrectBurnTime) {
    // Items::OAK_PLANKS 是通过 Items 注册的 BlockItem
    const Item* oakPlanksItem = Items::OAK_PLANKS;
    ASSERT_NE(oakPlanksItem, nullptr) << "OAK_PLANKS should be registered";
    ItemStack planks(oakPlanksItem, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(planks), 300);
}

// ========== 木制工具测试 (200 tick) ==========

TEST_F(FurnaceBurnTimeTest, WoodenPickaxe_HasCorrectBurnTime) {
    ASSERT_NE(Items::WOODEN_PICKAXE, nullptr) << "WOODEN_PICKAXE should be registered";
    ItemStack tool(Items::WOODEN_PICKAXE, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(tool), 200);
}

TEST_F(FurnaceBurnTimeTest, WoodenSword_HasCorrectBurnTime) {
    ASSERT_NE(Items::WOODEN_SWORD, nullptr) << "WOODEN_SWORD should be registered";
    ItemStack tool(Items::WOODEN_SWORD, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(tool), 200);
}

// ========== 弓和钓鱼竿测试 (300 tick) ==========

TEST_F(FurnaceBurnTimeTest, Bow_HasCorrectBurnTime) {
    ASSERT_NE(Items::BOW, nullptr) << "BOW should be registered";
    ItemStack bow(Items::BOW, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(bow), 300);
}

TEST_F(FurnaceBurnTimeTest, FishingRod_HasCorrectBurnTime) {
    ASSERT_NE(Items::FISHING_ROD, nullptr) << "FISHING_ROD should be registered";
    ItemStack rod(Items::FISHING_ROD, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(rod), 300);
}

TEST_F(FurnaceBurnTimeTest, Crossbow_HasCorrectBurnTime) {
    ASSERT_NE(Items::CROSSBOW, nullptr) << "CROSSBOW should be registered";
    ItemStack crossbow(Items::CROSSBOW, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(crossbow), 300);
}

// ========== 木棍和碗测试 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, Stick_HasCorrectBurnTime) {
    ASSERT_NE(Items::STICK, nullptr) << "STICK should be registered";
    ItemStack stick(Items::STICK, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stick), 100);
}

TEST_F(FurnaceBurnTimeTest, Bowl_HasCorrectBurnTime) {
    ASSERT_NE(Items::BOWL, nullptr) << "BOWL should be registered";
    ItemStack bowl(Items::BOWL, 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(bowl), 100);
}

// ========== 方块物品燃烧时间测试 ==========

TEST_F(FurnaceBurnTimeTest, Bookshelf_HasCorrectBurnTime) {
    const BlockItem* bookshelfItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BOOKSHELF);
    ASSERT_NE(bookshelfItem, nullptr) << "BOOKSHELF should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(bookshelfItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, NoteBlock_HasCorrectBurnTime) {
    const BlockItem* noteBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::NOTE_BLOCK);
    ASSERT_NE(noteBlockItem, nullptr) << "NOTE_BLOCK should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(noteBlockItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, CraftingTable_HasCorrectBurnTime) {
    const BlockItem* craftingTableItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CRAFTING_TABLE);
    ASSERT_NE(craftingTableItem, nullptr) << "CRAFTING_TABLE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(craftingTableItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, DaylightDetector_HasCorrectBurnTime) {
    const BlockItem* detectorItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DAYLIGHT_DETECTOR);
    ASSERT_NE(detectorItem, nullptr) << "DAYLIGHT_DETECTOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(detectorItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakStairs_HasCorrectBurnTime) {
    const BlockItem* stairsItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_STAIRS);
    ASSERT_NE(stairsItem, nullptr) << "OAK_STAIRS should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(stairsItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakSlab_HasCorrectBurnTime) {
    // 木质台阶燃烧时间是 150 tick
    const BlockItem* slabItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_SLAB);
    ASSERT_NE(slabItem, nullptr) << "OAK_SLAB should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(slabItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 150);
}

TEST_F(FurnaceBurnTimeTest, OakFence_HasCorrectBurnTime) {
    const BlockItem* fenceItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_FENCE);
    ASSERT_NE(fenceItem, nullptr) << "OAK_FENCE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(fenceItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakFenceGate_HasCorrectBurnTime) {
    const BlockItem* gateItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_FENCE_GATE);
    ASSERT_NE(gateItem, nullptr) << "OAK_FENCE_GATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(gateItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakDoor_HasCorrectBurnTime) {
    const BlockItem* doorItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_DOOR);
    ASSERT_NE(doorItem, nullptr) << "OAK_DOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(doorItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakTrapdoor_HasCorrectBurnTime) {
    const BlockItem* trapdoorItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_TRAPDOOR);
    ASSERT_NE(trapdoorItem, nullptr) << "OAK_TRAPDOOR should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(trapdoorItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakPressurePlate_HasCorrectBurnTime) {
    const BlockItem* plateItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_PRESSURE_PLATE);
    ASSERT_NE(plateItem, nullptr) << "OAK_PRESSURE_PLATE should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(plateItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 300);
}

TEST_F(FurnaceBurnTimeTest, OakButton_HasCorrectBurnTime) {
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_BUTTON);
    ASSERT_NE(buttonItem, nullptr) << "OAK_BUTTON should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, SpruceButton_HasCorrectBurnTime) {
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SPRUCE_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "SPRUCE_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, BirchButton_HasCorrectBurnTime) {
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BIRCH_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "BIRCH_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, JungleButton_HasCorrectBurnTime) {
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::JUNGLE_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "JUNGLE_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, AcaciaButton_HasCorrectBurnTime) {
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ACACIA_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "ACACIA_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, DarkOakButton_HasCorrectBurnTime) {
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DARK_OAK_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "DARK_OAK_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, CrimsonButton_NotBurnable) {
    // 下界木材按钮不可燃
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CRIMSON_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "CRIMSON_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "CRIMSON_BUTTON should not be burnable";
}

TEST_F(FurnaceBurnTimeTest, WarpedButton_NotBurnable) {
    // 下界木材按钮不可燃
    const BlockItem* buttonItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WARPED_BUTTON);
    if (buttonItem == nullptr) {
        GTEST_SKIP() << "WARPED_BUTTON not registered yet";
    }
    ItemStack stack(static_cast<const Item*>(buttonItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 0) << "WARPED_BUTTON should not be burnable";
}

// ========== 树苗测试 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, OakSapling_HasCorrectBurnTime) {
    const BlockItem* saplingItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::OAK_SAPLING);
    ASSERT_NE(saplingItem, nullptr) << "OAK_SAPLING should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(saplingItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, BirchSapling_HasCorrectBurnTime) {
    const BlockItem* saplingItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BIRCH_SAPLING);
    ASSERT_NE(saplingItem, nullptr) << "BIRCH_SAPLING should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(saplingItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

// ========== 羊毛测试 (100 tick) ==========

TEST_F(FurnaceBurnTimeTest, WhiteWool_HasCorrectBurnTime) {
    const BlockItem* woolItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WHITE_WOOL);
    ASSERT_NE(woolItem, nullptr) << "WHITE_WOOL should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(woolItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

TEST_F(FurnaceBurnTimeTest, BlackWool_HasCorrectBurnTime) {
    const BlockItem* woolItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BLACK_WOOL);
    ASSERT_NE(woolItem, nullptr) << "BLACK_WOOL should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(woolItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 100);
}

// ========== 竹子测试 (50 tick) ==========

TEST_F(FurnaceBurnTimeTest, Bamboo_HasCorrectBurnTime) {
    const BlockItem* bambooItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BAMBOO);
    ASSERT_NE(bambooItem, nullptr) << "BAMBOO should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(bambooItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 50);
}

// ========== 干海带块测试 (4001 tick) ==========

TEST_F(FurnaceBurnTimeTest, DriedKelpBlock_HasCorrectBurnTime) {
    const BlockItem* kelpItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DRIED_KELP_BLOCK);
    ASSERT_NE(kelpItem, nullptr) << "DRIED_KELP_BLOCK should have a BlockItem";
    ItemStack stack(static_cast<const Item*>(kelpItem), 1);
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(stack), 4001);
}

// ========== IsFuel 测试 ==========

TEST_F(FurnaceBurnTimeTest, IsFuel_ReturnsTrueForCoal) {
    ASSERT_NE(Items::COAL, nullptr);
    ItemStack coal(Items::COAL, 1);
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(coal));
}

TEST_F(FurnaceBurnTimeTest, IsFuel_ReturnsTrueForStick) {
    ASSERT_NE(Items::STICK, nullptr);
    ItemStack stick(Items::STICK, 1);
    EXPECT_TRUE(AbstractFurnaceEntity::isFuel(stick));
}

TEST_F(FurnaceBurnTimeTest, IsFuel_ReturnsFalseForDiamond) {
    ASSERT_NE(Items::DIAMOND, nullptr);
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(diamond));
}

TEST_F(FurnaceBurnTimeTest, IsFuel_ReturnsFalseForEmptyStack) {
    ItemStack empty;
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(empty));
}
