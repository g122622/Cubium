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

#include "world/blockentity/processing/CampfireBlockEntity.hpp"
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

// ========== CampfireBlockEntity 测试 ==========

class CampfireBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        campfire_ = std::make_unique<CampfireBlockEntity>(BlockPos(10, 20, 30));
        m_testItem = ensureTestItem("beef");
    }

    std::unique_ptr<CampfireBlockEntity> campfire_;
    Item* m_testItem = nullptr;
};

TEST_F(CampfireBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(campfire_->getType(), BlockEntityType::Campfire);
}

TEST_F(CampfireBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(campfire_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(CampfireBlockEntityTest, Create_HasCorrectSlotCount)
{
    EXPECT_EQ(campfire_->getContainerSize(), CampfireBlockEntity::SLOT_COUNT);
    EXPECT_EQ(CampfireBlockEntity::SLOT_COUNT, 4);
}

TEST_F(CampfireBlockEntityTest, Create_IsEmpty)
{
    EXPECT_TRUE(campfire_->isEmpty());
}

TEST_F(CampfireBlockEntityTest, Create_CookTimesAreZero)
{
    for (i32 i = 0; i < CampfireBlockEntity::SLOT_COUNT; ++i) {
        EXPECT_EQ(campfire_->getCookTime(i), 0);
        EXPECT_EQ(campfire_->getCookTimeTotal(i), 0);
    }
}

TEST_F(CampfireBlockEntityTest, Create_CookProgressIsZero)
{
    for (i32 i = 0; i < CampfireBlockEntity::SLOT_COUNT; ++i) {
        EXPECT_FLOAT_EQ(campfire_->getCookProgress(i), 0.0f);
    }
}

TEST_F(CampfireBlockEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(campfire_->needsTick());
}

TEST_F(CampfireBlockEntityTest, GetInventory_ReturnsValidPointer)
{
    IInventory* inventory = campfire_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), CampfireBlockEntity::SLOT_COUNT);
}

TEST_F(CampfireBlockEntityTest, GetCookProgress_InvalidSlotReturnsZero)
{
    EXPECT_FLOAT_EQ(campfire_->getCookProgress(-1), 0.0f);
    EXPECT_FLOAT_EQ(campfire_->getCookProgress(100), 0.0f);
}

TEST_F(CampfireBlockEntityTest, GetCookTime_InvalidSlotReturnsZero)
{
    EXPECT_EQ(campfire_->getCookTime(-1), 0);
    EXPECT_EQ(campfire_->getCookTime(100), 0);
}

TEST_F(CampfireBlockEntityTest, GetCookTimeTotal_InvalidSlotReturnsZero)
{
    EXPECT_EQ(campfire_->getCookTimeTotal(-1), 0);
    EXPECT_EQ(campfire_->getCookTimeTotal(100), 0);
}

// ========== 添加物品测试 ==========

TEST_F(CampfireBlockEntityTest, AddItem_EmptyStack_ReturnsFalse)
{
    ItemStack emptyStack;
    EXPECT_FALSE(campfire_->addItem(emptyStack, 600));
}

TEST_F(CampfireBlockEntityTest, AddItem_ToEmptySlot_Succeeds)
{
    ItemStack stack(m_testItem, 10);
    EXPECT_TRUE(campfire_->addItem(stack, 600));

    // 验证槽位中有1个物品（split出1个）
    IInventory* inventory = campfire_->getInventory();
    EXPECT_FALSE(inventory->getItem(0).isEmpty());
    EXPECT_EQ(inventory->getItem(0).getCount(), 1);

    // 验证烹饪时间设置正确
    EXPECT_EQ(campfire_->getCookTime(0), 0);
    EXPECT_EQ(campfire_->getCookTimeTotal(0), 600);

    // 原始堆叠减少了1个
    EXPECT_EQ(stack.getCount(), 9);
}

TEST_F(CampfireBlockEntityTest, AddItem_DefaultCookTime)
{
    ItemStack stack(m_testItem, 5);
    EXPECT_TRUE(campfire_->addItem(stack, 0)); // cookTime = 0，使用默认值

    EXPECT_EQ(campfire_->getCookTimeTotal(0), CampfireBlockEntity::DEFAULT_COOK_TIME);
}

TEST_F(CampfireBlockEntityTest, AddItem_FillsMultipleSlots)
{
    ItemStack stack1(m_testItem, 2);
    EXPECT_TRUE(campfire_->addItem(stack1, 300));

    ItemStack stack2(m_testItem, 3);
    EXPECT_TRUE(campfire_->addItem(stack2, 400));

    IInventory* inventory = campfire_->getInventory();
    EXPECT_EQ(inventory->getItem(0).getCount(), 1);
    EXPECT_EQ(inventory->getItem(1).getCount(), 1);
    EXPECT_EQ(campfire_->getCookTimeTotal(0), 300);
    EXPECT_EQ(campfire_->getCookTimeTotal(1), 400);
}

TEST_F(CampfireBlockEntityTest, AddItem_FullCampfire_ReturnsFalse)
{
    // 填满所有4个槽位
    for (i32 i = 0; i < CampfireBlockEntity::SLOT_COUNT; ++i) {
        ItemStack stack(m_testItem, 5);
        EXPECT_TRUE(campfire_->addItem(stack, 600));
    }

    // 再次添加应该失败
    ItemStack stack(m_testItem, 5);
    EXPECT_FALSE(campfire_->addItem(stack, 600));
}

TEST_F(CampfireBlockEntityTest, FindMatchingRecipe_EmptyStack_ReturnsNullopt)
{
    ItemStack emptyStack;
    EXPECT_FALSE(campfire_->findMatchingRecipe(emptyStack).has_value());
}

TEST_F(CampfireBlockEntityTest, FindMatchingRecipe_FullCampfire_ReturnsNullopt)
{
    // 填满所有槽位
    for (i32 i = 0; i < CampfireBlockEntity::SLOT_COUNT; ++i) {
        ItemStack stack(m_testItem, 5);
        campfire_->addItem(stack, 600);
    }

    ItemStack stack(m_testItem, 5);
    EXPECT_FALSE(campfire_->findMatchingRecipe(stack).has_value());
}

// ========== 清空测试 ==========

TEST_F(CampfireBlockEntityTest, Clear_ResetsAllSlots)
{
    // 添加物品到所有槽位
    for (i32 i = 0; i < CampfireBlockEntity::SLOT_COUNT; ++i) {
        ItemStack stack(m_testItem, 5);
        campfire_->addItem(stack, 600);
    }

    campfire_->clear();

    EXPECT_TRUE(campfire_->isEmpty());
    for (i32 i = 0; i < CampfireBlockEntity::SLOT_COUNT; ++i) {
        EXPECT_EQ(campfire_->getCookTime(i), 0);
        EXPECT_EQ(campfire_->getCookTimeTotal(i), 0);
    }
}

// ========== 烹饪进度测试 ==========

TEST_F(CampfireBlockEntityTest, GetCookProgress_CalculatesCorrectly)
{
    ItemStack stack(m_testItem, 5);
    campfire_->addItem(stack, 600);

    // 初始进度 = 0/600 = 0
    EXPECT_FLOAT_EQ(campfire_->getCookProgress(0), 0.0f);

    // 手动设置烹饪时间来测试进度计算
    // 注意：这需要通过内部方法或序列化来设置，这里只测试基本逻辑
    IInventory* inventory = campfire_->getInventory();
    EXPECT_FALSE(inventory->getItem(0).isEmpty());
}

TEST_F(CampfireBlockEntityTest, GetCookProgress_EmptySlot_ReturnsZero)
{
    // 空槽位返回0
    EXPECT_FLOAT_EQ(campfire_->getCookProgress(0), 0.0f);
    EXPECT_FLOAT_EQ(campfire_->getCookProgress(1), 0.0f);
}

// ========== 序列化测试 ==========

TEST_F(CampfireBlockEntityTest, SaveLoad_PreservesData)
{
    // 添加物品
    ItemStack stack(m_testItem, 5);
    campfire_->addItem(stack, 600);

    // 保存
    nlohmann::json data;
    campfire_->save(data);

    // 创建新的实体并加载（位置由构造函数设置，不从JSON加载）
    auto loaded = std::make_unique<CampfireBlockEntity>(BlockPos(10, 20, 30));
    EXPECT_TRUE(loaded->load(data));

    // 验证数据（位置由构造函数设置，不是从JSON加载）
    EXPECT_EQ(loaded->getType(), BlockEntityType::Campfire);
    EXPECT_FALSE(loaded->isEmpty());
    EXPECT_EQ(loaded->getCookTimeTotal(0), 600);
}

TEST_F(CampfireBlockEntityTest, SaveLoad_EmptyEntity)
{
    // 空实体保存/加载
    nlohmann::json data;
    campfire_->save(data);

    auto loaded = std::make_unique<CampfireBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->load(data));

    EXPECT_TRUE(loaded->isEmpty());
    for (i32 i = 0; i < CampfireBlockEntity::SLOT_COUNT; ++i) {
        EXPECT_EQ(loaded->getCookTime(i), 0);
        EXPECT_EQ(loaded->getCookTimeTotal(i), 0);
    }
}

// ========== 克隆测试 ==========

TEST_F(CampfireBlockEntityTest, Clone_CreatesDeepCopy)
{
    // 添加物品
    ItemStack stack(m_testItem, 5);
    campfire_->addItem(stack, 600);

    // 克隆
    std::unique_ptr<BlockEntity> copy = campfire_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Campfire);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));

    // 验证是深拷贝
    auto* campfireCopy = static_cast<CampfireBlockEntity*>(copy.get());
    EXPECT_FALSE(campfireCopy->isEmpty());
    EXPECT_EQ(campfireCopy->getCookTimeTotal(0), 600);
}

// ========== SetChanged 测试 ==========

TEST_F(CampfireBlockEntityTest, SetChanged_MarksAsChanged)
{
    EXPECT_FALSE(campfire_->isChanged());
    campfire_->setChanged();
    EXPECT_TRUE(campfire_->isChanged());
}

// ========== 常量测试 ==========

TEST_F(CampfireBlockEntityTest, Constants_HaveCorrectValues)
{
    EXPECT_EQ(CampfireBlockEntity::SLOT_COUNT, 4);
    EXPECT_EQ(CampfireBlockEntity::DEFAULT_COOK_TIME, 600); // MC 1.16.5 = 600 tick = 30秒
}
