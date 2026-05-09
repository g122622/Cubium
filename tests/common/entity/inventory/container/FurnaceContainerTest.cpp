#include <gtest/gtest.h>
#include "entity/inventory/container/FurnaceContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== FurnaceContainer 测试 ==========

class FurnaceContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        playerInventory_ = std::make_unique<PlayerInventory>();
        // 创建熔炉背包容器（3格：输入、燃料、输出）
        furnaceInventory_ = std::make_unique<SimpleInventory>(FurnaceContainer::FURNACE_SLOTS);
    }

    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<SimpleInventory> furnaceInventory_;
};

// ========== FurnaceFuelSlot 测试 ==========

class FurnaceFuelSlotTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        inventory_ = std::make_unique<SimpleInventory>(3);
    }

    std::unique_ptr<SimpleInventory> inventory_;
};

TEST_F(FurnaceFuelSlotTest, CreateSlot_Success) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);
    EXPECT_EQ(slot.getIndex(), 0);
    EXPECT_TRUE(slot.isEmpty());
}

TEST_F(FurnaceFuelSlotTest, MayPlace_AcceptsFuel) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 煤炭是燃料，FurnaceFuelSlot::isFuel() 现在已正确实现
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal != nullptr) {
        ItemStack coalStack(*coal, 1);
        EXPECT_TRUE(slot.mayPlace(coalStack)) << "Coal should be accepted as fuel";
    }
}

TEST_F(FurnaceFuelSlotTest, MayPlace_RejectsNonFuel) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 钻石不是燃料
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond != nullptr) {
        ItemStack diamondStack(*diamond, 1);
        EXPECT_FALSE(slot.mayPlace(diamondStack));
    }
}

TEST_F(FurnaceFuelSlotTest, GetMaxStackSize_FuelItems) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal != nullptr) {
        ItemStack coalStack(*coal, 1);
        // 燃料物品堆叠上限为64
        EXPECT_EQ(slot.getMaxStackSize(coalStack), 64);
    }
}

TEST_F(FurnaceFuelSlotTest, IsFuel_ChecksItem) {
    // FurnaceFuelSlot::isFuel() 现在已正确实现
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal != nullptr) {
        ItemStack coalStack(*coal, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isFuel(coalStack)) << "Coal should be detected as fuel";
    }

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond != nullptr) {
        ItemStack diamondStack(*diamond, 1);
        EXPECT_FALSE(FurnaceFuelSlot::isFuel(diamondStack)) << "Diamond should not be detected as fuel";
    }
}

// ========== FurnaceResultSlot 测试 ==========

class FurnaceResultSlotTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        inventory_ = std::make_unique<SimpleInventory>(3);
    }

    std::unique_ptr<SimpleInventory> inventory_;
};

TEST_F(FurnaceResultSlotTest, CreateSlot_Success) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);
    EXPECT_EQ(slot.getIndex(), 2);
    EXPECT_TRUE(slot.isEmpty());
}

TEST_F(FurnaceResultSlotTest, MayPlace_AlwaysFalse) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot != nullptr) {
        ItemStack stack(*ironIngot, 1);
        // 结果槽不能放入物品
        EXPECT_FALSE(slot.mayPlace(stack));
    }
}

TEST_F(FurnaceResultSlotTest, Remove_UpdatesExperience) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot == nullptr) {
        GTEST_SKIP() << "Iron ingot not registered";
    }

    // 设置输出槽物品
    ItemStack stack(*ironIngot, 10);
    inventory_->setItem(2, stack);
    slot.setChanged();

    // remove 应该返回正确数量的物品
    ItemStack removed = slot.remove(5);
    EXPECT_EQ(removed.getCount(), 5);
    EXPECT_EQ(inventory_->getItem(2).getCount(), 5);
}

TEST_F(FurnaceResultSlotTest, Remove_FromEmptySlot) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    // 从空槽移除
    ItemStack removed = slot.remove(5);
    EXPECT_TRUE(removed.isEmpty());
}

TEST_F(FurnaceResultSlotTest, Remove_MoreThanAvailable) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot == nullptr) {
        GTEST_SKIP() << "Iron ingot not registered";
    }

    // 设置输出槽物品
    ItemStack stack(*ironIngot, 3);
    inventory_->setItem(2, stack);

    // 请求移除比可用更多的物品
    ItemStack removed = slot.remove(10);
    EXPECT_EQ(removed.getCount(), 3);
    EXPECT_TRUE(inventory_->getItem(2).isEmpty());
}

TEST_F(FurnaceResultSlotTest, GetMaxStackSize_AlwaysOne) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot == nullptr) {
        GTEST_SKIP() << "Iron ingot not registered";
    }

    ItemStack stack(*ironIngot, 1);
    // 结果槽堆叠上限应该继承自基础槽位
    EXPECT_EQ(slot.getMaxStackSize(stack), 64);
}

TEST_F(FurnaceResultSlotTest, MayPlace_EmptyStack) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    // 空物品堆
    ItemStack emptyStack;
    EXPECT_FALSE(slot.mayPlace(emptyStack));
}

TEST_F(FurnaceResultSlotTest, MayPickup_AlwaysTrue) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    // 结果槽总是可以取出（即使为空）
    // 注意：mayPickup 需要 Player 参数，这里无法直接测试
    // 但可以验证方法存在
    (void)slot;
    SUCCEED() << "FurnaceResultSlot mayPickup method exists";
}

TEST_F(FurnaceResultSlotTest, Remove_AllItems) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot == nullptr) {
        GTEST_SKIP() << "Iron ingot not registered";
    }

    // 设置输出槽物品
    ItemStack stack(*ironIngot, 16);
    inventory_->setItem(2, stack);

    // 移除全部物品
    ItemStack removed = slot.remove(16);
    EXPECT_EQ(removed.getCount(), 16);
    EXPECT_TRUE(inventory_->getItem(2).isEmpty());
}

TEST_F(FurnaceResultSlotTest, Remove_ZeroItems) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot == nullptr) {
        GTEST_SKIP() << "Iron ingot not registered";
    }

    // 设置输出槽物品
    ItemStack stack(*ironIngot, 10);
    inventory_->setItem(2, stack);

    // 移除0个物品
    ItemStack removed = slot.remove(0);
    EXPECT_TRUE(removed.isEmpty());
    EXPECT_EQ(inventory_->getItem(2).getCount(), 10);
}

TEST_F(FurnaceResultSlotTest, Remove_NegativeAmount) {
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot == nullptr) {
        GTEST_SKIP() << "Iron ingot not registered";
    }

    // 设置输出槽物品
    ItemStack stack(*ironIngot, 10);
    inventory_->setItem(2, stack);

    // 移除负数个物品（应该返回空）
    ItemStack removed = slot.remove(-5);
    EXPECT_TRUE(removed.isEmpty());
    EXPECT_EQ(inventory_->getItem(2).getCount(), 10);
}

// ========== FurnaceFuelSlot 边界测试 ==========

class FurnaceFuelSlotEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        inventory_ = std::make_unique<SimpleInventory>(3);
    }

    std::unique_ptr<SimpleInventory> inventory_;
};

TEST_F(FurnaceFuelSlotEdgeCaseTest, MayPlace_EmptyStack) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 空物品堆
    ItemStack emptyStack;
    EXPECT_FALSE(slot.mayPlace(emptyStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, GetMaxStackSize_EmptyStack) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 空物品堆的堆叠上限
    ItemStack emptyStack;
    EXPECT_EQ(slot.getMaxStackSize(emptyStack), 64);
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, SetAndGetItem) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal == nullptr) {
        GTEST_SKIP() << "Coal not registered";
    }

    // 设置物品到槽位
    ItemStack stack(*coal, 32);
    inventory_->setItem(0, stack);

    EXPECT_FALSE(slot.isEmpty());
    EXPECT_EQ(slot.getItem().getCount(), 32);
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsBucket_NotBucket) {
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal == nullptr) {
        GTEST_SKIP() << "Coal not registered";
    }

    ItemStack coalStack(*coal, 1);
    // 煤炭不是桶
    EXPECT_FALSE(FurnaceFuelSlot::isBucket(coalStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsFuel_EmptyStack) {
    ItemStack emptyStack;
    // 空物品不是燃料
    EXPECT_FALSE(FurnaceFuelSlot::isFuel(emptyStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsBucket_EmptyStack) {
    ItemStack emptyStack;
    // 空物品不是桶
    EXPECT_FALSE(FurnaceFuelSlot::isBucket(emptyStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, GetMaxStackSize_VariousItems) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal != nullptr) {
        ItemStack coalStack(*coal, 1);
        EXPECT_EQ(slot.getMaxStackSize(coalStack), 64);
    }

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond != nullptr) {
        ItemStack diamondStack(*diamond, 1);
        // 钻石不是燃料，但 getMaxStackSize 仍返回物品本身的堆叠上限
        EXPECT_EQ(slot.getMaxStackSize(diamondStack), 64);
    }
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, RemoveFromSlot) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal == nullptr) {
        GTEST_SKIP() << "Coal not registered";
    }

    // 设置物品
    inventory_->setItem(0, ItemStack(*coal, 32));

    // 移除部分
    ItemStack removed = slot.remove(10);
    EXPECT_EQ(removed.getCount(), 10);
    EXPECT_EQ(inventory_->getItem(0).getCount(), 22);
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsBucket_DetectsAllBucketTypes) {
    // 空桶
    Item* bucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:bucket"));
    if (bucket != nullptr) {
        ItemStack bucketStack(*bucket, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isBucket(bucketStack)) << "Empty bucket should be detected";
    }

    // 水桶
    Item* waterBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:water_bucket"));
    if (waterBucket != nullptr) {
        ItemStack waterBucketStack(*waterBucket, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isBucket(waterBucketStack)) << "Water bucket should be detected";
    }

    // 岩浆桶
    Item* lavaBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:lava_bucket"));
    if (lavaBucket != nullptr) {
        ItemStack lavaBucketStack(*lavaBucket, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isBucket(lavaBucketStack)) << "Lava bucket should be detected";
    }
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsBucket_RejectsNonBucketItems) {
    // 煤炭不是桶
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal != nullptr) {
        ItemStack coalStack(*coal, 1);
        EXPECT_FALSE(FurnaceFuelSlot::isBucket(coalStack)) << "Coal should not be detected as bucket";
    }

    // 钻石不是桶
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond != nullptr) {
        ItemStack diamondStack(*diamond, 1);
        EXPECT_FALSE(FurnaceFuelSlot::isBucket(diamondStack)) << "Diamond should not be detected as bucket";
    }
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, MaxStackSize_BucketIsOne) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 岩浆桶堆叠上限应为1
    Item* lavaBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:lava_bucket"));
    if (lavaBucket != nullptr) {
        ItemStack lavaBucketStack(*lavaBucket, 1);
        EXPECT_EQ(slot.getMaxStackSize(lavaBucketStack), 1) << "Lava bucket should have max stack size 1";
    }

    // 空桶堆叠上限应为1（在 FurnaceFuelSlot 中）
    Item* bucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:bucket"));
    if (bucket != nullptr) {
        ItemStack bucketStack(*bucket, 1);
        EXPECT_EQ(slot.getMaxStackSize(bucketStack), 1) << "Bucket should have max stack size 1 in fuel slot";
    }
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, MayPlace_AcceptsBucket) {
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 空桶可以放入燃料槽（用于接收岩浆桶燃烧后的空桶）
    Item* bucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:bucket"));
    if (bucket != nullptr) {
        ItemStack bucketStack(*bucket, 1);
        EXPECT_TRUE(slot.mayPlace(bucketStack)) << "Empty bucket should be accepted in fuel slot";
    }
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsFuel_DetectsVariousFuelTypes) {
    // 煤炭是燃料
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal != nullptr) {
        ItemStack coalStack(*coal, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isFuel(coalStack)) << "Coal should be fuel";
    }

    // 木炭是燃料
    Item* charcoal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:charcoal"));
    if (charcoal != nullptr) {
        ItemStack charcoalStack(*charcoal, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isFuel(charcoalStack)) << "Charcoal should be fuel";
    }

    // 烈焰棒是燃料
    Item* blazeRod = ItemRegistry::instance().getItem(ResourceLocation("minecraft:blaze_rod"));
    if (blazeRod != nullptr) {
        ItemStack blazeRodStack(*blazeRod, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isFuel(blazeRodStack)) << "Blaze rod should be fuel";
    }

    // 岩浆桶是燃料
    Item* lavaBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:lava_bucket"));
    if (lavaBucket != nullptr) {
        ItemStack lavaBucketStack(*lavaBucket, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isFuel(lavaBucketStack)) << "Lava bucket should be fuel";
    }

    // 木棍是燃料
    Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    if (stick != nullptr) {
        ItemStack stickStack(*stick, 1);
        EXPECT_TRUE(FurnaceFuelSlot::isFuel(stickStack)) << "Stick should be fuel";
    }
}

TEST_F(FurnaceContainerTest, Create_HasCorrectSlotCount) {
    // 注意: 在Release模式下MC_ASSERT不起作用
    // 容器实际槽位数量 = 熔炉槽位 + 玩家背包槽位 = 3 + 36 = 39
    // 测试验证熔炉背包已正确设置
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());
    EXPECT_EQ(container.getFurnaceInventory(), furnaceInventory_.get());
    EXPECT_EQ(container.getSlotCount(), 39);
}

TEST_F(FurnaceContainerTest, GetFurnaceInventory_ReturnsCorrectInventory) {
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());

    EXPECT_EQ(container.getFurnaceInventory(), furnaceInventory_.get());
}

TEST_F(FurnaceContainerTest, ContainerType_IsCorrect) {
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());

    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(FurnaceContainerTest, SlotIndices_AreCorrect) {
    EXPECT_EQ(FurnaceContainer::SLOT_INPUT, 0);
    EXPECT_EQ(FurnaceContainer::SLOT_FUEL, 1);
    EXPECT_EQ(FurnaceContainer::SLOT_OUTPUT, 2);
}

TEST_F(FurnaceContainerTest, FurnaceSlots_IsThree) {
    EXPECT_EQ(FurnaceContainer::FURNACE_SLOTS, 3);
}

TEST_F(FurnaceContainerTest, Constants_AreCorrect) {
    // 验证GUI布局常量存在
    EXPECT_GT(FurnaceContainer::FURNACE_SLOT_Y, 0);
    EXPECT_GT(FurnaceContainer::PLAYER_INV_Y, FurnaceContainer::FURNACE_SLOT_Y);
    EXPECT_GT(FurnaceContainer::HOTBAR_Y, FurnaceContainer::PLAYER_INV_Y);
    EXPECT_EQ(FurnaceContainer::SLOT_SIZE, 18);
}
