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

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/bundle/BundleContents.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::item::items;

// ============================================================================
// 测试夹具
// ============================================================================

class BundleContentsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 → 物品（收纳袋依赖 Items::initialize 注册）
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// 静态常量测试
// ============================================================================

TEST_F(BundleContentsTest, Constants_HaveExpectedValues)
{
    // 对应 MC 1.21.11 BundleContents 的权重上限和嵌套权重
    EXPECT_EQ(BundleContents::MAX_WEIGHT, 64);
    EXPECT_EQ(BundleContents::BUNDLE_IN_BUNDLE_WEIGHT, 4);
    EXPECT_EQ(BundleContents::NO_SELECTED_ITEM, -1);
}

// ============================================================================
// 空内容物测试
// ============================================================================

TEST_F(BundleContentsTest, Empty_DefaultConstructor_IsEmpty)
{
    BundleContents contents;
    EXPECT_TRUE(contents.isEmpty());
    EXPECT_EQ(contents.size(), 0u);
    EXPECT_EQ(contents.weight(), 0);
    EXPECT_EQ(contents.selectedItem(), BundleContents::NO_SELECTED_ITEM);
    EXPECT_FALSE(contents.hasSelectedItem());
}

TEST_F(BundleContentsTest, Empty_StaticEMPTY_IsEmpty)
{
    EXPECT_TRUE(BundleContents::EMPTY.isEmpty());
    EXPECT_EQ(BundleContents::EMPTY.size(), 0u);
}

// ============================================================================
// 权重计算测试
// ============================================================================

TEST_F(BundleContentsTest, GetWeight_StackableItem_64MaxStack)
{
    // 石头 maxStackSize=64，权重 = 64/64 = 1
    ItemStack stone(*Items::STONE, 1);
    EXPECT_EQ(BundleContents::getWeight(stone), 1);
}

TEST_F(BundleContentsTest, GetWeight_StackableItem_16MaxStack)
{
    // 末影珍珠 maxStackSize=16，权重 = ceil(64/16) = 4
    ItemStack enderPearl(*Items::ENDER_PEARL, 1);
    EXPECT_EQ(BundleContents::getWeight(enderPearl), 4);
}

TEST_F(BundleContentsTest, GetWeight_StackSize1_MaxWeight)
{
    // 剑 maxStackSize=1，权重 = ceil(64/1) = 64（满权重）
    ItemStack sword(*Items::DIAMOND_SWORD, 1);
    EXPECT_EQ(BundleContents::getWeight(sword), BundleContents::MAX_WEIGHT);
}

TEST_F(BundleContentsTest, GetWeight_EmptyStack_ReturnsZero)
{
    ItemStack empty;
    EXPECT_EQ(BundleContents::getWeight(empty), 0);
}

TEST_F(BundleContentsTest, GetWeight_BundleItem_HasInBundleWeight)
{
    // 收纳袋本身：BUNDLE_IN_BUNDLE_WEIGHT + 内袋权重
    // 空收纳袋：4 + 0 = 4
    ItemStack bundle(*Items::BUNDLE, 1);
    EXPECT_EQ(BundleContents::getWeight(bundle), BundleContents::BUNDLE_IN_BUNDLE_WEIGHT);
}

TEST_F(BundleContentsTest, GetWeight_BundleWithContents_AccumulatesInnerWeight)
{
    // 收纳袋 + 内含 1 石头：4 + 1 = 5
    ItemStack bundle(*Items::BUNDLE, 1);
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 1);
    mutableContents.tryInsert(stone);
    BundleContents inner = mutableContents.toImmutable();
    bundle.getOrCreateTag()["BundleContents"] = inner.toJson();

    EXPECT_EQ(BundleContents::getWeight(bundle), BundleContents::BUNDLE_IN_BUNDLE_WEIGHT + 1);
}

// ============================================================================
// canItemBeInBundle 测试
// ============================================================================

TEST_F(BundleContentsTest, CanItemBeInBundle_NormalItem_ReturnsTrue)
{
    ItemStack stone(*Items::STONE, 1);
    EXPECT_TRUE(BundleContents::canItemBeInBundle(stone));
}

TEST_F(BundleContentsTest, CanItemBeInBundle_EmptyStack_ReturnsFalse)
{
    ItemStack empty;
    EXPECT_FALSE(BundleContents::canItemBeInBundle(empty));
}

// ============================================================================
// 构造与查询测试
// ============================================================================

TEST_F(BundleContentsTest, Constructor_FromItems_ComputesWeight)
{
    // 构造 3 个石头堆：权重 = 1×3 = 3
    std::vector<ItemStack> items;
    items.emplace_back(*Items::STONE, 3);

    BundleContents contents(std::move(items));
    EXPECT_EQ(contents.size(), 1u);
    EXPECT_EQ(contents.weight(), 3);
    EXPECT_FALSE(contents.isEmpty());
}

TEST_F(BundleContentsTest, Constructor_FullArgs_PreservesSelected)
{
    std::vector<ItemStack> items;
    items.emplace_back(*Items::STONE, 1);
    items.emplace_back(*Items::DIRT, 1);

    BundleContents contents(std::move(items), 2, 1);
    EXPECT_EQ(contents.weight(), 2);
    EXPECT_EQ(contents.selectedItem(), 1);
    EXPECT_TRUE(contents.hasSelectedItem());
}

// ============================================================================
// Mutable 测试
// ============================================================================

TEST_F(BundleContentsTest, Mutable_TryInsert_NormalItem_Succeeds)
{
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 5);
    i32 inserted = mutableContents.tryInsert(stone);
    EXPECT_EQ(inserted, 5);
    EXPECT_EQ(mutableContents.weight(), 5);
    EXPECT_TRUE(stone.isEmpty());
}

TEST_F(BundleContentsTest, Mutable_TryInsert_WeightLimitStopsInsertion)
{
    // 剑权重=64，已经满权重，无法再插入
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack sword(*Items::DIAMOND_SWORD, 1);
    i32 inserted = mutableContents.tryInsert(sword);
    EXPECT_EQ(inserted, 1);

    // 再插入一个剑应该失败
    ItemStack sword2(*Items::DIAMOND_SWORD, 1);
    i32 inserted2 = mutableContents.tryInsert(sword2);
    EXPECT_EQ(inserted2, 0);
}

TEST_F(BundleContentsTest, Mutable_TryInsert_StackableAccumulatesWeight)
{
    // 插入 16 个末影珍珠（权重 4×16=64）应成功
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack enderPearls(*Items::ENDER_PEARL, 16);
    i32 inserted = mutableContents.tryInsert(enderPearls);
    EXPECT_EQ(inserted, 16);
    EXPECT_EQ(mutableContents.weight(), 64);

    // 再插入 1 个应该失败（超权重）
    ItemStack extraPearl(*Items::ENDER_PEARL, 1);
    i32 inserted2 = mutableContents.tryInsert(extraPearl);
    EXPECT_EQ(inserted2, 0);
}

TEST_F(BundleContentsTest, Mutable_TryInsert_MergesStackableItems)
{
    // 先插入 5 个石头
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone1(*Items::STONE, 5);
    mutableContents.tryInsert(stone1);

    // 再插入 3 个石头，应该合并
    ItemStack stone2(*Items::STONE, 3);
    i32 inserted = mutableContents.tryInsert(stone2);
    EXPECT_EQ(inserted, 3);
    EXPECT_EQ(mutableContents.weight(), 8);

    BundleContents contents = mutableContents.toImmutable();
    ASSERT_EQ(contents.size(), 1u);
    EXPECT_EQ(contents.getItemUnsafe(0).getCount(), 8);
}

TEST_F(BundleContentsTest, Mutable_RemoveOne_FromEmpty_ReturnsNullopt)
{
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    auto result = mutableContents.removeOne();
    EXPECT_FALSE(result.has_value());
}

TEST_F(BundleContentsTest, Mutable_RemoveOne_RemovesFirstItem)
{
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 5);
    mutableContents.tryInsert(stone);

    auto removed = mutableContents.removeOne();
    ASSERT_TRUE(removed.has_value());
    EXPECT_EQ(removed->getCount(), 5);
    EXPECT_EQ(removed->getItem(), Items::STONE);

    BundleContents contents = mutableContents.toImmutable();
    EXPECT_TRUE(contents.isEmpty());
    EXPECT_EQ(contents.weight(), 0);
}

TEST_F(BundleContentsTest, Mutable_ToggleSelectedItem_SetsSelected)
{
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 1);
    ItemStack dirt(*Items::DIRT, 1);
    mutableContents.tryInsert(stone);
    mutableContents.tryInsert(dirt);

    // 选中索引 0
    mutableContents.toggleSelectedItem(0);
    EXPECT_EQ(mutableContents.selectedItem(), 0);

    // 再次切换同一索引应清除
    mutableContents.toggleSelectedItem(0);
    EXPECT_EQ(mutableContents.selectedItem(), BundleContents::NO_SELECTED_ITEM);
}

TEST_F(BundleContentsTest, Mutable_ClearItems_RemovesAll)
{
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 5);
    mutableContents.tryInsert(stone);
    ASSERT_FALSE(mutableContents.toImmutable().isEmpty());

    mutableContents.clearItems();
    EXPECT_TRUE(mutableContents.toImmutable().isEmpty());
    EXPECT_EQ(mutableContents.weight(), 0);
}

// ============================================================================
// 序列化测试
// ============================================================================

TEST_F(BundleContentsTest, Serialization_RoundTrip_PreservesData)
{
    std::vector<ItemStack> items;
    items.emplace_back(*Items::STONE, 5);
    items.emplace_back(*Items::DIRT, 3);

    BundleContents original(std::move(items));
    nlohmann::json json = original.toJson();
    BundleContents restored = BundleContents::fromJson(json);

    EXPECT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored.weight(), original.weight());
    EXPECT_EQ(restored, original);
}

TEST_F(BundleContentsTest, Serialization_Empty_RoundTrip)
{
    BundleContents original;
    nlohmann::json json = original.toJson();
    BundleContents restored = BundleContents::fromJson(json);

    EXPECT_TRUE(restored.isEmpty());
    EXPECT_EQ(restored, BundleContents::EMPTY);
}

TEST_F(BundleContentsTest, FromItemStack_NoTag_ReturnsEmpty)
{
    // 物品堆无 NBT 标签应返回空内容物
    ItemStack bundle(*Items::BUNDLE, 1);
    BundleContents contents = BundleContents::fromItemStack(bundle);
    EXPECT_TRUE(contents.isEmpty());
}

TEST_F(BundleContentsTest, FromItemStack_WithTag_ReturnsContents)
{
    // 设置 BundleContents NBT 后应能正确读取
    ItemStack bundle(*Items::BUNDLE, 1);
    BundleContents::Mutable mutableContents(BundleContents::EMPTY);
    ItemStack stone(*Items::STONE, 3);
    mutableContents.tryInsert(stone);
    bundle.getOrCreateTag()["BundleContents"] = mutableContents.toImmutable().toJson();

    BundleContents contents = BundleContents::fromItemStack(bundle);
    EXPECT_EQ(contents.size(), 1u);
    EXPECT_EQ(contents.weight(), 3);
}

// ============================================================================
// numberOfItemsToShow 测试
// ============================================================================

TEST_F(BundleContentsTest, NumberOfItemsToShow_FewItems_ReturnsCount)
{
    std::vector<ItemStack> items;
    items.emplace_back(*Items::STONE, 1);
    BundleContents contents(std::move(items));
    EXPECT_EQ(contents.numberOfItemsToShow(), 1);
}

// ============================================================================
// 比较操作符测试
// ============================================================================

TEST_F(BundleContentsTest, Equality_SameContents_AreEqual)
{
    std::vector<ItemStack> items1;
    items1.emplace_back(*Items::STONE, 3);
    BundleContents c1(std::move(items1));

    std::vector<ItemStack> items2;
    items2.emplace_back(*Items::STONE, 3);
    BundleContents c2(std::move(items2));

    EXPECT_EQ(c1, c2);
}

TEST_F(BundleContentsTest, Equality_DifferentCounts_NotEqual)
{
    std::vector<ItemStack> items1;
    items1.emplace_back(*Items::STONE, 3);
    BundleContents c1(std::move(items1));

    std::vector<ItemStack> items2;
    items2.emplace_back(*Items::STONE, 5);
    BundleContents c2(std::move(items2));

    EXPECT_NE(c1, c2);
}
