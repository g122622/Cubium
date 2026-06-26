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

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "item/tag/ItemTags.hpp"

using namespace mc;

class ItemTagsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }

    void SetUp() override
    {
        // 每个测试用例前的设置
    }
};

// ============================================================================
// FLOWERS 标签测试 - 小型花朵
// ============================================================================

TEST_F(ItemTagsTest, FlowersContainsDandelion)
{
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    ASSERT_NE(dandelion, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(dandelion));
}

TEST_F(ItemTagsTest, FlowersContainsPoppy)
{
    Item* poppy = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "poppy"));
    ASSERT_NE(poppy, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(poppy));
}

TEST_F(ItemTagsTest, FlowersContainsBlueOrchid)
{
    Item* blueOrchid = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "blue_orchid"));
    ASSERT_NE(blueOrchid, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(blueOrchid));
}

TEST_F(ItemTagsTest, FlowersContainsAllium)
{
    Item* allium = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "allium"));
    ASSERT_NE(allium, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(allium));
}

TEST_F(ItemTagsTest, FlowersContainsAzureBluet)
{
    Item* azureBluet = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "azure_bluet"));
    ASSERT_NE(azureBluet, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(azureBluet));
}

// ============================================================================
// FLOWERS 标签测试 - 郁金香系列
// ============================================================================

TEST_F(ItemTagsTest, FlowersContainsRedTulip)
{
    Item* redTulip = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "red_tulip"));
    ASSERT_NE(redTulip, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(redTulip));
}

TEST_F(ItemTagsTest, FlowersContainsOrangeTulip)
{
    Item* orangeTulip = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "orange_tulip"));
    ASSERT_NE(orangeTulip, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(orangeTulip));
}

TEST_F(ItemTagsTest, FlowersContainsWhiteTulip)
{
    Item* whiteTulip = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_tulip"));
    ASSERT_NE(whiteTulip, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(whiteTulip));
}

TEST_F(ItemTagsTest, FlowersContainsPinkTulip)
{
    Item* pinkTulip = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pink_tulip"));
    ASSERT_NE(pinkTulip, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(pinkTulip));
}

// ============================================================================
// FLOWERS 标签测试 - 其他小型花朵
// ============================================================================

TEST_F(ItemTagsTest, FlowersContainsOxeyeDaisy)
{
    Item* oxeyeDaisy = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oxeye_daisy"));
    ASSERT_NE(oxeyeDaisy, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(oxeyeDaisy));
}

TEST_F(ItemTagsTest, FlowersContainsLilyOfTheValley)
{
    Item* lilyOfTheValley = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lily_of_the_valley"));
    ASSERT_NE(lilyOfTheValley, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(lilyOfTheValley));
}

TEST_F(ItemTagsTest, FlowersContainsCornflower)
{
    Item* cornflower = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cornflower"));
    ASSERT_NE(cornflower, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(cornflower));
}

TEST_F(ItemTagsTest, FlowersContainsWitherRose)
{
    Item* witherRose = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wither_rose"));
    ASSERT_NE(witherRose, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(witherRose));
}

// ============================================================================
// FLOWERS 标签测试 - 大型花朵
// ============================================================================

TEST_F(ItemTagsTest, FlowersContainsSunflower)
{
    Item* sunflower = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "sunflower"));
    ASSERT_NE(sunflower, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(sunflower));
}

TEST_F(ItemTagsTest, FlowersContainsLilac)
{
    Item* lilac = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "lilac"));
    ASSERT_NE(lilac, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(lilac));
}

TEST_F(ItemTagsTest, FlowersContainsRoseBush)
{
    Item* roseBush = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "rose_bush"));
    ASSERT_NE(roseBush, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(roseBush));
}

TEST_F(ItemTagsTest, FlowersContainsPeony)
{
    Item* peony = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "peony"));
    ASSERT_NE(peony, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(peony));
}

// ============================================================================
// FLOWERS 标签测试 - 非花朵物品
// ============================================================================

TEST_F(ItemTagsTest, FlowersDoesNotContainStone)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(item::tag::ItemTags::FLOWERS().contains(stone));
}

TEST_F(ItemTagsTest, FlowersDoesNotContainDirt)
{
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(dirt, nullptr);
    EXPECT_FALSE(item::tag::ItemTags::FLOWERS().contains(dirt));
}

TEST_F(ItemTagsTest, FlowersDoesNotContainWheat)
{
    Item* wheat = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wheat"));
    ASSERT_NE(wheat, nullptr);
    EXPECT_FALSE(item::tag::ItemTags::FLOWERS().contains(wheat));
}

TEST_F(ItemTagsTest, FlowersDoesNotContainStick)
{
    Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stick"));
    ASSERT_NE(stick, nullptr);
    EXPECT_FALSE(item::tag::ItemTags::FLOWERS().contains(stick));
}

// ============================================================================
// FLOWERS 标签测试 - ItemStack 检查
// ============================================================================

TEST_F(ItemTagsTest, FlowersContainsItemStackWithDandelion)
{
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    ASSERT_NE(dandelion, nullptr);
    ItemStack stack(*dandelion, 1);
    EXPECT_TRUE(item::tag::ItemTags::FLOWERS().contains(stack));
}

TEST_F(ItemTagsTest, FlowersDoesNotContainItemStackWithStone)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemStack stack(*stone, 64);
    EXPECT_FALSE(item::tag::ItemTags::FLOWERS().contains(stack));
}

// ============================================================================
// FLOWERS 标签测试 - 标签属性
// ============================================================================

TEST_F(ItemTagsTest, FlowersTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::FLOWERS().getId(), ResourceLocation("minecraft", "flowers"));
}

TEST_F(ItemTagsTest, FlowersTagContainsMultipleItems)
{
    const auto& items = item::tag::ItemTags::FLOWERS().getItems();
    // 应该包含17种花朵（13种小型 + 4种大型）
    EXPECT_GE(items.size(), 17u);
}

// ============================================================================
// Item::isIn 测试
// ============================================================================

TEST_F(ItemTagsTest, DandelionIsInFlowersTag)
{
    Item* dandelion = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dandelion"));
    ASSERT_NE(dandelion, nullptr);
    EXPECT_TRUE(dandelion->isIn(item::tag::ItemTags::FLOWERS()));
}

TEST_F(ItemTagsTest, StoneIsNotInFlowersTag)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(stone->isIn(item::tag::ItemTags::FLOWERS()));
}

// ============================================================================
// 初始化测试
// ============================================================================

TEST_F(ItemTagsTest, InitializeCanBeCalledMultipleTimes)
{
    // 初始化应该可以多次调用（幂等性）
    EXPECT_NO_THROW(item::tag::ItemTags::initialize());
    EXPECT_NO_THROW(item::tag::ItemTags::initialize());
}

// ============================================================================
// DAMPENS_VIBRATIONS 标签测试 - 羊毛物品
// ============================================================================

TEST_F(ItemTagsTest, DampensVibrationsContainsWhiteWool)
{
    Item* whiteWool = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_wool"));
    ASSERT_NE(whiteWool, nullptr);
    EXPECT_TRUE(whiteWool->isIn(item::tag::ItemTags::DAMPENS_VIBRATIONS()));
}

TEST_F(ItemTagsTest, DampensVibrationsContainsBlackWool)
{
    Item* blackWool = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "black_wool"));
    ASSERT_NE(blackWool, nullptr);
    EXPECT_TRUE(blackWool->isIn(item::tag::ItemTags::DAMPENS_VIBRATIONS()));
}

// ============================================================================
// DAMPENS_VIBRATIONS 标签测试 - 地毯物品
// ============================================================================

TEST_F(ItemTagsTest, DampensVibrationsContainsWhiteCarpet)
{
    Item* whiteCarpet = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_carpet"));
    ASSERT_NE(whiteCarpet, nullptr);
    EXPECT_TRUE(whiteCarpet->isIn(item::tag::ItemTags::DAMPENS_VIBRATIONS()));
}

TEST_F(ItemTagsTest, DampensVibrationsContainsBlackCarpet)
{
    Item* blackCarpet = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "black_carpet"));
    ASSERT_NE(blackCarpet, nullptr);
    EXPECT_TRUE(blackCarpet->isIn(item::tag::ItemTags::DAMPENS_VIBRATIONS()));
}

// ============================================================================
// DAMPENS_VIBRATIONS 标签测试 - 非阻尼物品
// ============================================================================

TEST_F(ItemTagsTest, DampensVibrationsDoesNotContainStone)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(stone->isIn(item::tag::ItemTags::DAMPENS_VIBRATIONS()));
}

TEST_F(ItemTagsTest, DampensVibrationsDoesNotContainDirt)
{
    Item* dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    ASSERT_NE(dirt, nullptr);
    EXPECT_FALSE(dirt->isIn(item::tag::ItemTags::DAMPENS_VIBRATIONS()));
}

// ============================================================================
// DAMPENS_VIBRATIONS 标签属性
// ============================================================================

TEST_F(ItemTagsTest, DampensVibrationsTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::DAMPENS_VIBRATIONS().getId(), ResourceLocation("minecraft", "dampens_vibrations"));
}

TEST_F(ItemTagsTest, DampensVibrationsContains32Items)
{
    // 16 色羊毛 + 16 色地毯 = 32 项
    const auto& items = item::tag::ItemTags::DAMPENS_VIBRATIONS().getItems();
    EXPECT_EQ(items.size(), 32u);
}

// ============================================================================
// FIRE_RESISTANT 标签测试 - 下界合金物品
// ============================================================================

TEST_F(ItemTagsTest, FireResistantContainsNetheriteIngot)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemTagsTest, FireResistantContainsNetheriteScrap)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_scrap"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemTagsTest, FireResistantContainsAncientDebris)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "ancient_debris"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemTagsTest, FireResistantContainsNetherStar)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "nether_star"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemTagsTest, FireResistantContainsNetheriteBlock)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_block"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemTagsTest, FireResistantContainsNetheriteTools)
{
    // 所有下界合金工具都应在防火标签中
    const char* toolNames[] = {
        "netherite_sword", "netherite_shovel", "netherite_pickaxe", "netherite_axe", "netherite_hoe"};
    for (const char* name : toolNames) {
        Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Item not found: " << name;
        EXPECT_TRUE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT())) << "Not fire resistant: " << name;
    }
}

TEST_F(ItemTagsTest, FireResistantContainsNetheriteArmor)
{
    // 所有下界合金盔甲都应在防火标签中
    const char* armorNames[] = {"netherite_helmet", "netherite_chestplate", "netherite_leggings", "netherite_boots"};
    for (const char* name : armorNames) {
        Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Item not found: " << name;
        EXPECT_TRUE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT())) << "Not fire resistant: " << name;
    }
}

TEST_F(ItemTagsTest, FireResistantDoesNotContainIronItems)
{
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    EXPECT_FALSE(ironIngot->isIn(item::tag::ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemTagsTest, FireResistantDoesNotContainDiamondItems)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_FALSE(diamond->isIn(item::tag::ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemTagsTest, FireResistantTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::FIRE_RESISTANT().getId(), ResourceLocation("minecraft", "fire_resistant"));
}
