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

/**
 * @file CompostableItemsTest.cpp
 * @brief CompostableItems 注册表测试
 *
 * 测试可堆肥物品注册表的功能：
 * - 初始化
 * - 堆肥概率查询
 * - 可堆肥检查
 */

#include "world/block/blocks/functional/CompostableItems.hpp"
#include "item/Items.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blocks;

class CompostableItemsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化物品系统
        Items::initialize();
        // 初始化可堆肥物品注册表
        CompostableItems::initialize();
    }
};

// 测试初始化
TEST_F(CompostableItemsTest, Initialization)
{
    EXPECT_TRUE(CompostableItems::isInitialized());
}

// 测试苹果的堆肥概率（65%）
TEST_F(CompostableItemsTest, AppleCompostChance)
{
    ASSERT_NE(Items::APPLE, nullptr);
    float chance = CompostableItems::getCompostChance(Items::APPLE);
    EXPECT_FLOAT_EQ(chance, 0.65f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::APPLE));
}

// 测试面包的堆肥概率（85%）
TEST_F(CompostableItemsTest, BreadCompostChance)
{
    ASSERT_NE(Items::BREAD, nullptr);
    float chance = CompostableItems::getCompostChance(Items::BREAD);
    EXPECT_FLOAT_EQ(chance, 0.85f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::BREAD));
}

// 测试南瓜派的堆肥概率（100%）
TEST_F(CompostableItemsTest, PumpkinPieCompostChance)
{
    ASSERT_NE(Items::PUMPKIN_PIE, nullptr);
    float chance = CompostableItems::getCompostChance(Items::PUMPKIN_PIE);
    EXPECT_FLOAT_EQ(chance, 1.0f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::PUMPKIN_PIE));
}

// 测试西瓜片的堆肥概率（50%）
TEST_F(CompostableItemsTest, MelonSliceCompostChance)
{
    ASSERT_NE(Items::MELON_SLICE, nullptr);
    float chance = CompostableItems::getCompostChance(Items::MELON_SLICE);
    EXPECT_FLOAT_EQ(chance, 0.5f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::MELON_SLICE));
}

// 测试小麦种子的堆肥概率（30%）
TEST_F(CompostableItemsTest, WheatSeedsCompostChance)
{
    ASSERT_NE(Items::WHEAT_SEEDS, nullptr);
    float chance = CompostableItems::getCompostChance(Items::WHEAT_SEEDS);
    EXPECT_FLOAT_EQ(chance, 0.3f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::WHEAT_SEEDS));
}

// 测试小麦的堆肥概率（65%）
TEST_F(CompostableItemsTest, WheatCompostChance)
{
    ASSERT_NE(Items::WHEAT, nullptr);
    float chance = CompostableItems::getCompostChance(Items::WHEAT);
    EXPECT_FLOAT_EQ(chance, 0.65f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::WHEAT));
}

// 测试胡萝卜的堆肥概率（65%）
TEST_F(CompostableItemsTest, CarrotCompostChance)
{
    ASSERT_NE(Items::CARROT, nullptr);
    float chance = CompostableItems::getCompostChance(Items::CARROT);
    EXPECT_FLOAT_EQ(chance, 0.65f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::CARROT));
}

// 测试马铃薯的堆肥概率（65%）
TEST_F(CompostableItemsTest, PotatoCompostChance)
{
    ASSERT_NE(Items::POTATO, nullptr);
    float chance = CompostableItems::getCompostChance(Items::POTATO);
    EXPECT_FLOAT_EQ(chance, 0.65f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::POTATO));
}

// 测试甜菜根的堆肥概率（65%）
TEST_F(CompostableItemsTest, BeetrootCompostChance)
{
    ASSERT_NE(Items::BEETROOT, nullptr);
    float chance = CompostableItems::getCompostChance(Items::BEETROOT);
    EXPECT_FLOAT_EQ(chance, 0.65f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::BEETROOT));
}

// 测试曲奇的堆肥概率（85%）
TEST_F(CompostableItemsTest, CookieCompostChance)
{
    ASSERT_NE(Items::COOKIE, nullptr);
    float chance = CompostableItems::getCompostChance(Items::COOKIE);
    EXPECT_FLOAT_EQ(chance, 0.85f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::COOKIE));
}

// 测试烤马铃薯的堆肥概率（85%）
TEST_F(CompostableItemsTest, BakedPotatoCompostChance)
{
    ASSERT_NE(Items::BAKED_POTATO, nullptr);
    float chance = CompostableItems::getCompostChance(Items::BAKED_POTATO);
    EXPECT_FLOAT_EQ(chance, 0.85f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::BAKED_POTATO));
}

// 测试干海带的堆肥概率（30%）
TEST_F(CompostableItemsTest, DriedKelpCompostChance)
{
    ASSERT_NE(Items::DRIED_KELP, nullptr);
    float chance = CompostableItems::getCompostChance(Items::DRIED_KELP);
    EXPECT_FLOAT_EQ(chance, 0.3f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::DRIED_KELP));
}

// 测试甜浆果的堆肥概率（30%）
TEST_F(CompostableItemsTest, SweetBerriesCompostChance)
{
    ASSERT_NE(Items::SWEET_BERRIES, nullptr);
    float chance = CompostableItems::getCompostChance(Items::SWEET_BERRIES);
    EXPECT_FLOAT_EQ(chance, 0.3f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::SWEET_BERRIES));
}

// 测试地狱疣的堆肥概率（65%）
TEST_F(CompostableItemsTest, NetherWartCompostChance)
{
    ASSERT_NE(Items::NETHER_WART, nullptr);
    float chance = CompostableItems::getCompostChance(Items::NETHER_WART);
    EXPECT_FLOAT_EQ(chance, 0.65f);
    EXPECT_TRUE(CompostableItems::isCompostable(Items::NETHER_WART));
}

// 测试不可堆肥物品
TEST_F(CompostableItemsTest, NonCompostableItems)
{
    // 钻石不应该可堆肥
    ASSERT_NE(Items::DIAMOND, nullptr);
    EXPECT_FALSE(CompostableItems::isCompostable(Items::DIAMOND));
    EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(Items::DIAMOND), 0.0f);

    // 石头不应该可堆肥
    ASSERT_NE(Items::STONE, nullptr);
    EXPECT_FALSE(CompostableItems::isCompostable(Items::STONE));
    EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(Items::STONE), 0.0f);

    // 木棍不应该可堆肥
    ASSERT_NE(Items::STICK, nullptr);
    EXPECT_FALSE(CompostableItems::isCompostable(Items::STICK));
    EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(Items::STICK), 0.0f);
}

// 测试空指针处理
TEST_F(CompostableItemsTest, NullItemHandling)
{
    EXPECT_FALSE(CompostableItems::isCompostable(nullptr));
    EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(nullptr), 0.0f);
}

// 测试堆肥概率等级分类
TEST_F(CompostableItemsTest, CompostChanceCategories)
{
    // 30% 概率物品
    std::vector<Item*> chance30Items = {Items::WHEAT_SEEDS,
        Items::PUMPKIN_SEEDS,
        Items::MELON_SEEDS,
        Items::BEETROOT_SEEDS,
        Items::DRIED_KELP,
        Items::SWEET_BERRIES};

    for (Item* item : chance30Items) {
        if (item != nullptr) {
            EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(item), 0.3f) << "Item should have 30% compost chance";
        }
    }

    // 50% 概率物品
    std::vector<Item*> chance50Items = {Items::MELON_SLICE};

    for (Item* item : chance50Items) {
        if (item != nullptr) {
            EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(item), 0.5f) << "Item should have 50% compost chance";
        }
    }

    // 65% 概率物品
    std::vector<Item*> chance65Items = {
        Items::APPLE, Items::WHEAT, Items::CARROT, Items::POTATO, Items::BEETROOT, Items::NETHER_WART};

    for (Item* item : chance65Items) {
        if (item != nullptr) {
            EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(item), 0.65f) << "Item should have 65% compost chance";
        }
    }

    // 85% 概率物品
    std::vector<Item*> chance85Items = {Items::BREAD, Items::COOKIE, Items::BAKED_POTATO};

    for (Item* item : chance85Items) {
        if (item != nullptr) {
            EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(item), 0.85f) << "Item should have 85% compost chance";
        }
    }

    // 100% 概率物品
    std::vector<Item*> chance100Items = {Items::PUMPKIN_PIE};

    for (Item* item : chance100Items) {
        if (item != nullptr) {
            EXPECT_FLOAT_EQ(CompostableItems::getCompostChance(item), 1.0f) << "Item should have 100% compost chance";
        }
    }
}
