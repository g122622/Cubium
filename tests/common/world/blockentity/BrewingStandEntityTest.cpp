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

#include "world/blockentity/processing/BrewingStandEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/block/BlockPos.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::blockentity;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品。
 * @param path 资源路径。
 * @return 已注册物品指针。
 */
Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}

} // namespace

// ========== BrewingStandEntity 测试 ==========

class BrewingStandEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        brewingStand_ = std::make_unique<BrewingStandEntity>(BlockPos(10, 20, 30));
        m_glassBottle = ensureTestItem("glass_bottle");
        m_blazePowder = ensureTestItem("blaze_powder");
    }

    std::unique_ptr<BrewingStandEntity> brewingStand_;
    Item* m_glassBottle = nullptr;
    Item* m_blazePowder = nullptr;
};

TEST_F(BrewingStandEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(brewingStand_->getType(), BlockEntityType::BrewingStand);
}

TEST_F(BrewingStandEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(brewingStand_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(BrewingStandEntityTest, Create_HasCorrectSlotCount)
{
    EXPECT_EQ(brewingStand_->getContainerSize(), BrewingStandEntity::TOTAL_SLOTS);
    EXPECT_EQ(BrewingStandEntity::TOTAL_SLOTS, 5);
    EXPECT_EQ(BrewingStandEntity::BOTTLE_SLOTS, 3);
    EXPECT_EQ(BrewingStandEntity::INGREDIENT_SLOT, 3);
    EXPECT_EQ(BrewingStandEntity::FUEL_SLOT, 4);
}

TEST_F(BrewingStandEntityTest, Create_IsEmpty)
{
    EXPECT_TRUE(brewingStand_->isEmpty());
}

TEST_F(BrewingStandEntityTest, Create_FuelIsZero)
{
    EXPECT_EQ(brewingStand_->getFuelLevel(), 0);
    EXPECT_FALSE(brewingStand_->hasFuel());
}

TEST_F(BrewingStandEntityTest, Create_BrewTimeIsZero)
{
    EXPECT_EQ(brewingStand_->getBrewTime(), 0);
    EXPECT_FALSE(brewingStand_->isBrewing());
}

TEST_F(BrewingStandEntityTest, Create_NoBottles)
{
    EXPECT_FALSE(brewingStand_->hasBottle(0));
    EXPECT_FALSE(brewingStand_->hasBottle(1));
    EXPECT_FALSE(brewingStand_->hasBottle(2));
}

TEST_F(BrewingStandEntityTest, SetFuelLevel_UpdatesFuel)
{
    brewingStand_->setFuelLevel(10);
    EXPECT_EQ(brewingStand_->getFuelLevel(), 10);
    EXPECT_TRUE(brewingStand_->hasFuel());
}

TEST_F(BrewingStandEntityTest, SetFuelLevel_ClampsToValidRange)
{
    brewingStand_->setFuelLevel(-5);
    EXPECT_EQ(brewingStand_->getFuelLevel(), 0);

    brewingStand_->setFuelLevel(2000);
    EXPECT_EQ(brewingStand_->getFuelLevel(), 1280); // FUEL_PER_BREW * 64 = 20 * 64
}

TEST_F(BrewingStandEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(brewingStand_->needsTick());
}

TEST_F(BrewingStandEntityTest, Clone_CreatesCopy)
{
    std::unique_ptr<BlockEntity> copy = brewingStand_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::BrewingStand);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));
}

TEST_F(BrewingStandEntityTest, SetChanged_MarksAsChanged)
{
    EXPECT_FALSE(brewingStand_->isChanged());
    brewingStand_->setChanged();
    EXPECT_TRUE(brewingStand_->isChanged());
}

// ========== 红石比较器信号测试 ==========

TEST_F(BrewingStandEntityTest, GetComparatorSignal_ReturnsZeroWhenEmpty)
{
    // 空酿造台应该返回 0
    EXPECT_EQ(brewingStand_->getComparatorSignal(), 0);
}

TEST_F(BrewingStandEntityTest, GetComparatorSignal_ReturnsOneWhenHasItems)
{
    // 放置一个物品，即使只有一个物品也应返回至少 1
    brewingStand_->setItem(0, ItemStack(m_glassBottle, 1));
    EXPECT_EQ(brewingStand_->getComparatorSignal(), 1);
}

TEST_F(BrewingStandEntityTest, GetComparatorSignal_CalculatesCorrectly)
{
    // 测试信号强度计算
    // 公式: floor(平均填充率 * 14) + (有非空槽位 ? 1 : 0)
    // 槽位填充率 = 物品数量 / min(容器堆叠上限, 物品最大堆叠数)

    // 放置 64 个物品在一个槽位 (假设物品最大堆叠数为 64)
    // 槽位填充率 = 64/64 = 1.0
    // 平均填充率 = 1.0 / 5 = 0.2
    // 信号 = floor(0.2 * 14) + 1 = 2 + 1 = 3
    brewingStand_->setItem(0, ItemStack(m_blazePowder, 64));
    EXPECT_EQ(brewingStand_->getComparatorSignal(), 3);
}

TEST_F(BrewingStandEntityTest, GetComparatorSignal_MultipleSlots)
{
    // 在两个槽位放置物品
    // 槽位 0: 32 个物品 (填充率 = 32/64 = 0.5)
    // 槽位 3: 32 个物品 (填充率 = 32/64 = 0.5)
    // 总填充率 = 0.5 + 0.5 = 1.0
    // 平均填充率 = 1.0 / 5 = 0.2
    // 信号 = floor(0.2 * 14) + 1 = 2 + 1 = 3
    brewingStand_->setItem(0, ItemStack(m_glassBottle, 32));
    brewingStand_->setItem(3, ItemStack(m_blazePowder, 32));
    EXPECT_EQ(brewingStand_->getComparatorSignal(), 3);
}

TEST_F(BrewingStandEntityTest, GetComparatorSignal_FullInventory)
{
    // 所有槽位都放满 64 个物品
    // 每个槽位填充率 = 64/64 = 1.0
    // 总填充率 = 5.0
    // 平均填充率 = 5.0 / 5 = 1.0
    // 信号 = floor(1.0 * 14) + 1 = 14 + 1 = 15
    for (i32 i = 0; i < BrewingStandEntity::TOTAL_SLOTS; ++i) {
        brewingStand_->setItem(i, ItemStack(m_blazePowder, 64));
    }
    EXPECT_EQ(brewingStand_->getComparatorSignal(), 15);
}

TEST_F(BrewingStandEntityTest, GetComparatorSignal_RespectsMaxSignal15)
{
    // 即使超过最大值，信号也不应超过 15
    // 当前实现已经限制在 15，这是边界测试
    for (i32 i = 0; i < BrewingStandEntity::TOTAL_SLOTS; ++i) {
        brewingStand_->setItem(i, ItemStack(m_blazePowder, 64));
    }
    // 信号应该是 15，不会超过
    EXPECT_LE(brewingStand_->getComparatorSignal(), 15);
}

// ========== ISidedInventory 测试 ==========

TEST_F(BrewingStandEntityTest, GetSlotsForFace_Up_ReturnsIngredientSlot)
{
    std::vector<i32> slots = brewingStand_->getSlotsForFace(Direction::Up);
    ASSERT_EQ(slots.size(), 1);
    EXPECT_EQ(slots[0], BrewingStandEntity::INGREDIENT_SLOT);
}

TEST_F(BrewingStandEntityTest, GetSlotsForFace_Down_ReturnsBottleAndIngredientSlots)
{
    std::vector<i32> slots = brewingStand_->getSlotsForFace(Direction::Down);
    ASSERT_EQ(slots.size(), 4);
    EXPECT_EQ(slots[0], 0);
    EXPECT_EQ(slots[1], 1);
    EXPECT_EQ(slots[2], 2);
    EXPECT_EQ(slots[3], BrewingStandEntity::INGREDIENT_SLOT);
}

TEST_F(BrewingStandEntityTest, GetSlotsForFace_Side_ReturnsBottleAndFuelSlots)
{
    std::vector<i32> slots = brewingStand_->getSlotsForFace(Direction::North);
    ASSERT_EQ(slots.size(), 4);
    EXPECT_EQ(slots[0], 0);
    EXPECT_EQ(slots[1], 1);
    EXPECT_EQ(slots[2], 2);
    EXPECT_EQ(slots[3], BrewingStandEntity::FUEL_SLOT);
}

// ========== 自定义名称测试 ==========

TEST_F(BrewingStandEntityTest, CustomName_DefaultEmpty)
{
    EXPECT_TRUE(brewingStand_->getCustomName().empty());
}

TEST_F(BrewingStandEntityTest, CustomName_SetAndGet)
{
    brewingStand_->setCustomName("My Stand");
    EXPECT_EQ(brewingStand_->getCustomName(), "My Stand");
}

TEST_F(BrewingStandEntityTest, CustomName_MarksChanged)
{
    EXPECT_FALSE(brewingStand_->isChanged());
    brewingStand_->setCustomName("Named");
    EXPECT_TRUE(brewingStand_->isChanged());
}

TEST_F(BrewingStandEntityTest, CustomName_SerializeRoundTrip)
{
    brewingStand_->setCustomName("Persisted Stand");

    nlohmann::json data;
    brewingStand_->save(data);

    ASSERT_TRUE(data.contains("CustomName"));
    EXPECT_EQ(data["CustomName"].get<std::string>(), "Persisted Stand");

    BrewingStandEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));
    EXPECT_EQ(loaded.getCustomName(), "Persisted Stand");
}

TEST_F(BrewingStandEntityTest, CustomName_EmptyNotSerialized)
{
    nlohmann::json data;
    brewingStand_->save(data);

    EXPECT_FALSE(data.contains("CustomName"));
}

TEST_F(BrewingStandEntityTest, CustomName_CloneCopies)
{
    brewingStand_->setCustomName("Clone Source");

    std::unique_ptr<BlockEntity> copy = brewingStand_->clone();
    ASSERT_NE(copy, nullptr);

    auto* brewingCopy = dynamic_cast<BrewingStandEntity*>(copy.get());
    ASSERT_NE(brewingCopy, nullptr);
    EXPECT_EQ(brewingCopy->getCustomName(), "Clone Source");
}
