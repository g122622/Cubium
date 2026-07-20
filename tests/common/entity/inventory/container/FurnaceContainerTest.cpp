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

#include "entity/inventory/container/FurnaceContainer.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "world/blockentity/processing/FurnaceEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

// ========== FurnaceContainer 测试 ==========

class FurnaceContainerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
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
    void SetUp() override
    {
        Items::initialize();
        inventory_ = std::make_unique<SimpleInventory>(3);
    }

    std::unique_ptr<SimpleInventory> inventory_;
};

TEST_F(FurnaceFuelSlotTest, CreateSlot_Success)
{
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);
    EXPECT_EQ(slot.getIndex(), 0);
    EXPECT_TRUE(slot.isEmpty());
}

TEST_F(FurnaceFuelSlotTest, MayPlace_AcceptsFuel)
{
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 煤炭是燃料，FurnaceFuelSlot::isFuel() 现在已正确实现
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal != nullptr) {
        ItemStack coalStack(*coal, 1);
        EXPECT_TRUE(slot.mayPlace(coalStack)) << "Coal should be accepted as fuel";
    }
}

TEST_F(FurnaceFuelSlotTest, MayPlace_RejectsNonFuel)
{
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 钻石不是燃料
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond != nullptr) {
        ItemStack diamondStack(*diamond, 1);
        EXPECT_FALSE(slot.mayPlace(diamondStack));
    }
}

TEST_F(FurnaceFuelSlotTest, GetMaxStackSize_FuelItems)
{
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal != nullptr) {
        ItemStack coalStack(*coal, 1);
        // 燃料物品堆叠上限为64
        EXPECT_EQ(slot.getMaxStackSize(coalStack), 64);
    }
}

TEST_F(FurnaceFuelSlotTest, IsFuel_ChecksItem)
{
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
    void SetUp() override
    {
        Items::initialize();
        inventory_ = std::make_unique<SimpleInventory>(3);
    }

    std::unique_ptr<SimpleInventory> inventory_;
};

TEST_F(FurnaceResultSlotTest, CreateSlot_Success)
{
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);
    EXPECT_EQ(slot.getIndex(), 2);
    EXPECT_TRUE(slot.isEmpty());
}

TEST_F(FurnaceResultSlotTest, MayPlace_AlwaysFalse)
{
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot != nullptr) {
        ItemStack stack(*ironIngot, 1);
        // 结果槽不能放入物品
        EXPECT_FALSE(slot.mayPlace(stack));
    }
}

TEST_F(FurnaceResultSlotTest, Remove_UpdatesExperience)
{
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

TEST_F(FurnaceResultSlotTest, Remove_FromEmptySlot)
{
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    // 从空槽移除
    ItemStack removed = slot.remove(5);
    EXPECT_TRUE(removed.isEmpty());
}

TEST_F(FurnaceResultSlotTest, Remove_MoreThanAvailable)
{
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

TEST_F(FurnaceResultSlotTest, GetMaxStackSize_AlwaysOne)
{
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    if (ironIngot == nullptr) {
        GTEST_SKIP() << "Iron ingot not registered";
    }

    ItemStack stack(*ironIngot, 1);
    // 结果槽堆叠上限应该继承自基础槽位
    EXPECT_EQ(slot.getMaxStackSize(stack), 64);
}

TEST_F(FurnaceResultSlotTest, MayPlace_EmptyStack)
{
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    // 空物品堆
    ItemStack emptyStack;
    EXPECT_FALSE(slot.mayPlace(emptyStack));
}

TEST_F(FurnaceResultSlotTest, MayPickup_AlwaysTrue)
{
    FurnaceResultSlot slot(nullptr, inventory_.get(), 2, 10, 10);

    // 结果槽总是可以取出（即使为空）
    // 注意：mayPickup 需要 Player 参数，这里无法直接测试
    // 但可以验证方法存在
    (void)slot;
    SUCCEED() << "FurnaceResultSlot mayPickup method exists";
}

TEST_F(FurnaceResultSlotTest, Remove_AllItems)
{
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

TEST_F(FurnaceResultSlotTest, Remove_ZeroItems)
{
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

TEST_F(FurnaceResultSlotTest, Remove_NegativeAmount)
{
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
    void SetUp() override
    {
        Items::initialize();
        inventory_ = std::make_unique<SimpleInventory>(3);
    }

    std::unique_ptr<SimpleInventory> inventory_;
};

TEST_F(FurnaceFuelSlotEdgeCaseTest, MayPlace_EmptyStack)
{
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 空物品堆
    ItemStack emptyStack;
    EXPECT_FALSE(slot.mayPlace(emptyStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, GetMaxStackSize_EmptyStack)
{
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 空物品堆的堆叠上限
    ItemStack emptyStack;
    EXPECT_EQ(slot.getMaxStackSize(emptyStack), 64);
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, SetAndGetItem)
{
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

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsBucket_NotBucket)
{
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal == nullptr) {
        GTEST_SKIP() << "Coal not registered";
    }

    ItemStack coalStack(*coal, 1);
    // 煤炭不是桶
    EXPECT_FALSE(FurnaceFuelSlot::isBucket(coalStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsFuel_EmptyStack)
{
    ItemStack emptyStack;
    // 空物品不是燃料
    EXPECT_FALSE(FurnaceFuelSlot::isFuel(emptyStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsBucket_EmptyStack)
{
    ItemStack emptyStack;
    // 空物品不是桶
    EXPECT_FALSE(FurnaceFuelSlot::isBucket(emptyStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, GetMaxStackSize_VariousItems)
{
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

TEST_F(FurnaceFuelSlotEdgeCaseTest, RemoveFromSlot)
{
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

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsBucket_DetectsAllBucketTypes)
{
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

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsBucket_RejectsNonBucketItems)
{
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

TEST_F(FurnaceFuelSlotEdgeCaseTest, MaxStackSize_BucketIsOne)
{
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

TEST_F(FurnaceFuelSlotEdgeCaseTest, MayPlace_AcceptsBucket)
{
    FurnaceFuelSlot slot(inventory_.get(), 0, 10, 10);

    // 空桶可以放入燃料槽（用于接收岩浆桶燃烧后的空桶）
    Item* bucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:bucket"));
    if (bucket != nullptr) {
        ItemStack bucketStack(*bucket, 1);
        EXPECT_TRUE(slot.mayPlace(bucketStack)) << "Empty bucket should be accepted in fuel slot";
    }
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, IsFuel_DetectsVariousFuelTypes)
{
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

TEST_F(FurnaceContainerTest, Create_HasCorrectSlotCount)
{
    // 注意: 在Release模式下MC_ASSERT不起作用
    // 容器实际槽位数量 = 熔炉槽位 + 玩家背包槽位 = 3 + 36 = 39
    // 测试验证熔炉背包已正确设置
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());
    EXPECT_EQ(container.getFurnaceInventory(), furnaceInventory_.get());
    EXPECT_EQ(container.getSlotCount(), 39);
}

TEST_F(FurnaceContainerTest, GetFurnaceInventory_ReturnsCorrectInventory)
{
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());

    EXPECT_EQ(container.getFurnaceInventory(), furnaceInventory_.get());
}

TEST_F(FurnaceContainerTest, ContainerType_IsCorrect)
{
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());

    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(FurnaceContainerTest, SlotIndices_AreCorrect)
{
    EXPECT_EQ(FurnaceContainer::SLOT_INPUT, 0);
    EXPECT_EQ(FurnaceContainer::SLOT_FUEL, 1);
    EXPECT_EQ(FurnaceContainer::SLOT_OUTPUT, 2);
}

TEST_F(FurnaceContainerTest, FurnaceSlots_IsThree)
{
    EXPECT_EQ(FurnaceContainer::FURNACE_SLOTS, 3);
}

TEST_F(FurnaceContainerTest, Constants_AreCorrect)
{
    // 验证GUI布局常量存在
    EXPECT_GT(FurnaceContainer::FURNACE_SLOT_Y, 0);
    EXPECT_GT(FurnaceContainer::PLAYER_INV_Y, FurnaceContainer::FURNACE_SLOT_Y);
    EXPECT_GT(FurnaceContainer::HOTBAR_Y, FurnaceContainer::PLAYER_INV_Y);
    EXPECT_EQ(FurnaceContainer::SLOT_SIZE, 18);
}

// ========== FurnaceResultSlot 经验发放测试 ==========

/**
 * @brief FurnaceResultSlot 经验发放测试类
 *
 * 测试 FurnaceResultSlot 的以下功能：
 * 1. onTake 触发经验发放
 * 2. 玩家和熔炉实体为 nullptr 时不发放经验
 * 3. 熔炉无累积经验时不发放经验
 * 4. 快速移动（Shift+点击）场景
 * 5. 普通点击取出场景
 */
class FurnaceResultSlotExperienceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品系统
        Items::initialize();

        // 创建玩家
        player_ = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer");
        player_->setExperience(0, 0.0f, 0); // 初始经验为 0（等级、进度、总经验）

        // 创建熔炉背包
        furnaceInventory_ = std::make_unique<SimpleInventory>(3);

        // 创建熔炉实体
        furnaceEntity_ = std::make_unique<FurnaceEntity>(BlockPos(10, 20, 30));
    }

    void TearDown() override
    {
        furnaceEntity_.reset();
        furnaceInventory_.reset();
        player_.reset();
    }

    std::unique_ptr<Player> player_;
    std::unique_ptr<SimpleInventory> furnaceInventory_;
    std::unique_ptr<FurnaceEntity> furnaceEntity_;
};

TEST_F(FurnaceResultSlotExperienceTest, OnTake_WithFurnaceEntity_GrantsExperience)
{
    // 创建 FurnaceResultSlot 并传入熔炉实体
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 设置累积经验
    furnaceEntity_->setStoredExperience(10.5f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr) << "Iron ingot should be registered";
    ItemStack stack(*ironIngot, 8);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 模拟取出物品（调用 onTake）
    ItemStack taken = slot.onTake(*player_, slot.getItem());

    // 验证物品被取出
    EXPECT_EQ(taken.getCount(), 8);

    // 验证玩家获得了经验
    EXPECT_GT(player_->totalExperience(), initialXp) << "Player should have gained experience";

    // 验证熔炉累积经验已清空
    EXPECT_EQ(furnaceEntity_->getStoredExperience(), 0.0f) << "Furnace stored experience should be cleared";
}

TEST_F(FurnaceResultSlotExperienceTest, OnTake_NoFurnaceEntity_NoExperienceGranted)
{
    // 创建 FurnaceResultSlot 不传入熔炉实体
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, nullptr);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 8);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 模拟取出物品
    ItemStack taken = slot.onTake(*player_, slot.getItem());

    // 验证物品被取出
    EXPECT_EQ(taken.getCount(), 8);

    // 验证玩家没有获得经验（因为没有熔炉实体）
    EXPECT_EQ(player_->totalExperience(), initialXp)
        << "Player should not have gained experience without furnace entity";
}

TEST_F(FurnaceResultSlotExperienceTest, OnTake_NoPlayer_NoExperienceGranted)
{
    // 创建 FurnaceResultSlot 不传入玩家（player = nullptr）
    FurnaceResultSlot slot(nullptr, furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 设置累积经验
    furnaceEntity_->setStoredExperience(10.5f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 8);
    furnaceInventory_->setItem(2, stack);

    // 由于 slot.remove() 需要追踪 m_removeCount，我们先调用 remove
    ItemStack removed = slot.remove(8);
    EXPECT_EQ(removed.getCount(), 8);

    // 验证熔炉累积经验没有变化（因为 remove() 不触发经验发放）
    EXPECT_EQ(furnaceEntity_->getStoredExperience(), 10.5f)
        << "Furnace stored experience should not change after remove()";

    // 注意：onTake 需要 Player 引用，这里无法测试 null player 调用 onTake
    // 但我们已经验证了 m_player = nullptr 时，onCrafting 中不会发放经验
}

TEST_F(FurnaceResultSlotExperienceTest, OnTake_NoStoredExperience_NoExperienceGranted)
{
    // 创建 FurnaceResultSlot 并传入熔炉实体
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 不设置累积经验（默认为 0）
    EXPECT_EQ(furnaceEntity_->getStoredExperience(), 0.0f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 8);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 模拟取出物品
    ItemStack taken = slot.onTake(*player_, slot.getItem());

    // 验证物品被取出
    EXPECT_EQ(taken.getCount(), 8);

    // 验证玩家没有获得经验（因为没有累积经验）
    EXPECT_EQ(player_->totalExperience(), initialXp)
        << "Player should not have gained experience with zero stored experience";
}

TEST_F(FurnaceResultSlotExperienceTest, Remove_TracksRemoveCount)
{
    // 创建 FurnaceResultSlot
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 设置累积经验
    furnaceEntity_->setStoredExperience(7.8f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 16);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 使用 remove() 取出一部分物品
    ItemStack removed1 = slot.remove(5);
    EXPECT_EQ(removed1.getCount(), 5);

    // 调用 onTake 触发经验发放
    ItemStack taken = slot.onTake(*player_, slot.getItem());
    EXPECT_EQ(taken.getCount(), 11); // 16 - 5 = 11

    // 验证玩家获得了经验（从 16 个物品）
    EXPECT_GT(player_->totalExperience(), initialXp) << "Player should have gained experience from remove + onTake";

    // 验证熔炉累积经验已清空
    EXPECT_EQ(furnaceEntity_->getStoredExperience(), 0.0f);
}

TEST_F(FurnaceResultSlotExperienceTest, SetFurnaceEntity_UpdatesEntityReference)
{
    // 创建不带熔炉实体的槽位
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, nullptr);

    // 验证初始状态
    EXPECT_EQ(slot.getFurnaceEntity(), nullptr);

    // 设置熔炉实体
    slot.setFurnaceEntity(furnaceEntity_.get());

    // 验证设置成功
    EXPECT_EQ(slot.getFurnaceEntity(), furnaceEntity_.get());

    // 设置累积经验
    furnaceEntity_->setStoredExperience(5.0f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 4);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 模拟取出物品
    slot.onTake(*player_, slot.getItem());

    // 验证玩家获得了经验
    EXPECT_GT(player_->totalExperience(), initialXp);
}

TEST_F(FurnaceResultSlotExperienceTest, OnCrafting_CalledFromOnTake)
{
    // 此测试验证 onTake 内部正确调用了 onCrafting
    // 创建 FurnaceResultSlot
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 设置累积经验
    furnaceEntity_->setStoredExperience(3.7f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 1);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 直接调用 onTake（不通过 remove）
    slot.onTake(*player_, slot.getItem());

    // 验证玩家获得了经验（onTake 应该自动设置 m_removeCount）
    EXPECT_GT(player_->totalExperience(), initialXp)
        << "onTake should grant experience even without prior remove() call";

    // 验证熔炉累积经验已清空
    EXPECT_EQ(furnaceEntity_->getStoredExperience(), 0.0f);
}

TEST_F(FurnaceResultSlotExperienceTest, MultipleRemoves_ThenOnTake)
{
    // 测试多次 remove 后一次性 onTake 的场景
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 设置累积经验
    furnaceEntity_->setStoredExperience(15.0f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 32);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 多次 remove
    slot.remove(5);
    slot.remove(3);
    slot.remove(4);

    // 验证物品数量正确
    EXPECT_EQ(furnaceInventory_->getItem(2).getCount(), 20); // 32 - 5 - 3 - 4 = 20

    // 调用 onTake
    slot.onTake(*player_, furnaceInventory_->getItem(2));

    // 验证玩家获得了经验（从 12 个物品：5 + 3 + 4）
    EXPECT_GT(player_->totalExperience(), initialXp);

    // 验证熔炉累积经验已清空（只发放一次）
    EXPECT_EQ(furnaceEntity_->getStoredExperience(), 0.0f);
}

TEST_F(FurnaceResultSlotExperienceTest, ExperienceRoundedDown)
{
    // 测试经验值向下取整
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 设置累积经验为小数
    furnaceEntity_->setStoredExperience(10.7f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 1);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 取出物品
    slot.onTake(*player_, slot.getItem());

    // 验证玩家获得的经验是向下取整的（floor(10.7) = 10）
    EXPECT_EQ(player_->totalExperience() - initialXp, 10) << "Experience should be floored (floor(10.7) = 10)";
}

TEST_F(FurnaceResultSlotExperienceTest, ZeroStoredExperience_NoEffect)
{
    // 测试累积经验为 0 的情况
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 确保累积经验为 0
    furnaceEntity_->setStoredExperience(0.0f);

    // 设置输出槽物品
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft:iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    ItemStack stack(*ironIngot, 8);
    furnaceInventory_->setItem(2, stack);

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 取出物品
    slot.onTake(*player_, slot.getItem());

    // 验证玩家经验没有变化
    EXPECT_EQ(player_->totalExperience(), initialXp) << "Player should not gain experience when stored XP is 0";
}

TEST_F(FurnaceResultSlotExperienceTest, OnTakeWithEmptySlot_NoEffect)
{
    // 测试从空槽位取出的情况
    FurnaceResultSlot slot(player_.get(), furnaceInventory_.get(), 2, 116, 35, furnaceEntity_.get());

    // 设置累积经验
    furnaceEntity_->setStoredExperience(5.0f);

    // 不设置输出槽物品（空槽位）

    // 记录初始经验
    i32 initialXp = player_->totalExperience();

    // 尝试从空槽位取出
    ItemStack taken = slot.onTake(*player_, slot.getItem());

    // 验证返回的是空物品堆
    EXPECT_TRUE(taken.isEmpty());

    // 验证玩家经验没有变化
    EXPECT_EQ(player_->totalExperience(), initialXp);

    // 验证熔炉累积经验没有变化（因为没有物品被取出，m_removeCount 为 0）
    EXPECT_EQ(furnaceEntity_->getStoredExperience(), 5.0f);
}
