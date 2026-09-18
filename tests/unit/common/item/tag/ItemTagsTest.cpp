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

TEST_F(ItemTagsTest, FireResistantContainsNetheriteAnimalArmor)
{
    // 下界合金马铠与下界合金鹦鹉螺铠甲均应在防火标签中
    const char* armorNames[] = {"netherite_horse_armor", "netherite_nautilus_armor"};
    for (const char* name : armorNames) {
        Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Item not found: " << name;
        EXPECT_TRUE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT())) << "Not fire resistant: " << name;
    }
}

TEST_F(ItemTagsTest, FireResistantDoesNotContainNonNetheriteNautilusArmor)
{
    // 非下界合金的鹦鹉螺铠甲不应在防火标签中
    const char* armorNames[] = {
        "copper_nautilus_armor", "iron_nautilus_armor", "golden_nautilus_armor", "diamond_nautilus_armor"};
    for (const char* name : armorNames) {
        Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Item not found: " << name;
        EXPECT_FALSE(item->isIn(item::tag::ItemTags::FIRE_RESISTANT())) << "Should not be fire resistant: " << name;
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

// ============================================================================
// CHAINS 标签测试 - 铁锁链和铜锁链物品
// 参考: net.minecraft.tags.ItemTags.CHAINS
// ============================================================================

TEST_F(ItemTagsTest, ChainsContainsIronChain)
{
    Item* ironChain = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_chain"));
    ASSERT_NE(ironChain, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(ironChain));
}

TEST_F(ItemTagsTest, ChainsContainsCopperChain)
{
    Item* copperChain = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_chain"));
    ASSERT_NE(copperChain, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(copperChain));
}

TEST_F(ItemTagsTest, ChainsContainsExposedCopperChain)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "exposed_copper_chain"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(item));
}

TEST_F(ItemTagsTest, ChainsContainsWeatheredCopperChain)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "weathered_copper_chain"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(item));
}

TEST_F(ItemTagsTest, ChainsContainsOxidizedCopperChain)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oxidized_copper_chain"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(item));
}

TEST_F(ItemTagsTest, ChainsContainsWaxedCopperChain)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_copper_chain"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(item));
}

TEST_F(ItemTagsTest, ChainsContainsWaxedExposedCopperChain)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_exposed_copper_chain"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(item));
}

TEST_F(ItemTagsTest, ChainsContainsWaxedWeatheredCopperChain)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_weathered_copper_chain"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(item));
}

TEST_F(ItemTagsTest, ChainsContainsWaxedOxidizedCopperChain)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_oxidized_copper_chain"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item::tag::ItemTags::CHAINS().contains(item));
}

TEST_F(ItemTagsTest, ChainsTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::CHAINS().getId(), ResourceLocation("minecraft", "chains"));
}

TEST_F(ItemTagsTest, ChainsContainsAllNineItems)
{
    // CHAINS 标签应包含铁锁链 + 4个铜锁链氧化变种 + 4个涂蜡铜锁链变种 = 9项
    const auto& items = item::tag::ItemTags::CHAINS().getItems();
    EXPECT_EQ(items.size(), 9u);
}

TEST_F(ItemTagsTest, ChainsDoesNotContainIronBars)
{
    // 铁栏杆不属于锁链标签
    Item* ironBars = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_bars"));
    ASSERT_NE(ironBars, nullptr);
    EXPECT_FALSE(item::tag::ItemTags::CHAINS().contains(ironBars));
}

TEST_F(ItemTagsTest, ChainsDoesNotContainIronIngot)
{
    // 铁锭不属于锁链标签
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    EXPECT_FALSE(item::tag::ItemTags::CHAINS().contains(ironIngot));
}

TEST_F(ItemTagsTest, IronChainIsInChainsTag)
{
    // 使用 Item::isIn() 方法检查
    Item* ironChain = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_chain"));
    ASSERT_NE(ironChain, nullptr);
    EXPECT_TRUE(ironChain->isIn(item::tag::ItemTags::CHAINS()));
}

// ============================================================================
// WOODEN_DOORS 标签测试
// 参考: net.minecraft.tags.ItemTags.WOODEN_DOORS
// ============================================================================

TEST_F(ItemTagsTest, WoodenDoorsContainsOakDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_DOORS()));
}

TEST_F(ItemTagsTest, WoodenDoorsContainsCrimsonDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_DOORS()));
}

TEST_F(ItemTagsTest, WoodenDoorsContainsWarpedDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_DOORS()));
}

TEST_F(ItemTagsTest, WoodenDoorsDoesNotContainIronDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIn(item::tag::ItemTags::WOODEN_DOORS()));
}

TEST_F(ItemTagsTest, WoodenDoorsTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::WOODEN_DOORS().getId(), ResourceLocation("minecraft", "wooden_doors"));
}

TEST_F(ItemTagsTest, WoodenDoorsContainsAll12Doors)
{
    // 12种木门: oak, spruce, birch, jungle, acacia, dark_oak, mangrove, cherry, bamboo, pale_oak, crimson, warped
    const auto& items = item::tag::ItemTags::WOODEN_DOORS().getItems();
    EXPECT_EQ(items.size(), 12u);
}

// ============================================================================
// DOORS 标签测试
// 参考: net.minecraft.tags.ItemTags.DOORS
// ============================================================================

TEST_F(ItemTagsTest, DoorsContainsOakDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsIronDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsCrimsonDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsDoesNotContainStone)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(stone->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::DOORS().getId(), ResourceLocation("minecraft", "doors"));
}

// ========== 铜门在 DOORS 标签中 ==========

TEST_F(ItemTagsTest, DoorsContainsCopperDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsExposedCopperDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "exposed_copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsWeatheredCopperDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "weathered_copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsOxidizedCopperDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oxidized_copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsWaxedCopperDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsWaxedExposedCopperDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_exposed_copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsWaxedWeatheredCopperDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_weathered_copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, DoorsContainsWaxedOxidizedCopperDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_oxidized_copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::DOORS()));
}

TEST_F(ItemTagsTest, CopperDoorsNotInWoodenDoors)
{
    // 铜门不应属于 WOODEN_DOORS 标签
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIn(item::tag::ItemTags::WOODEN_DOORS()));
}

// ========== 铜活板门在 TRAPDOORS 标签中 ==========

TEST_F(ItemTagsTest, TrapdoorsContainsCopperTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsExposedCopperTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "exposed_copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsWeatheredCopperTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "weathered_copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsOxidizedCopperTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oxidized_copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsWaxedCopperTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsWaxedExposedCopperTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_exposed_copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsWaxedWeatheredCopperTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_weathered_copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsWaxedOxidizedCopperTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "waxed_oxidized_copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, CopperTrapdoorsNotInWoodenTrapdoors)
{
    // 铜活板门不应属于 WOODEN_TRAPDOORS 标签
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "copper_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIn(item::tag::ItemTags::WOODEN_TRAPDOORS()));
}

// ============================================================================
// WOODEN_TRAPDOORS 标签测试
// 参考: net.minecraft.tags.ItemTags.WOODEN_TRAPDOORS
// ============================================================================

TEST_F(ItemTagsTest, WoodenTrapdoorsContainsOakTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_TRAPDOORS()));
}

TEST_F(ItemTagsTest, WoodenTrapdoorsContainsCrimsonTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_TRAPDOORS()));
}

TEST_F(ItemTagsTest, WoodenTrapdoorsContainsWarpedTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_TRAPDOORS()));
}

TEST_F(ItemTagsTest, WoodenTrapdoorsDoesNotContainIronTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIn(item::tag::ItemTags::WOODEN_TRAPDOORS()));
}

TEST_F(ItemTagsTest, WoodenTrapdoorsTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::WOODEN_TRAPDOORS().getId(), ResourceLocation("minecraft", "wooden_trapdoors"));
}

TEST_F(ItemTagsTest, WoodenTrapdoorsContainsAll12Trapdoors)
{
    // 12种木活板门: oak, spruce, birch, jungle, acacia, dark_oak, mangrove, cherry, bamboo, pale_oak, crimson, warped
    const auto& items = item::tag::ItemTags::WOODEN_TRAPDOORS().getItems();
    EXPECT_EQ(items.size(), 12u);
}

// ============================================================================
// TRAPDOORS 标签测试
// 参考: net.minecraft.tags.ItemTags.TRAPDOORS
// ============================================================================

TEST_F(ItemTagsTest, TrapdoorsContainsOakTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsIronTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsContainsWarpedTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::TRAPDOORS()));
}

TEST_F(ItemTagsTest, TrapdoorsTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::TRAPDOORS().getId(), ResourceLocation("minecraft", "trapdoors"));
}

// ============================================================================
// NON_FLAMMABLE_WOOD 标签测试
// 参考: net.minecraft.tags.ItemTags.NON_FLAMMABLE_WOOD
// ============================================================================

TEST_F(ItemTagsTest, NonFlammableWoodContainsCrimsonStem)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_stem"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodContainsWarpedPlanks)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_planks"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodContainsCrimsonDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodContainsWarpedTrapdoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_trapdoor"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodContainsCrimsonSign)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_sign"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodContainsWarpedHangingSign)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_hanging_sign"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodDoesNotContainOakPlanks)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_planks"));
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodDoesNotContainIronDoor)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_door"));
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::NON_FLAMMABLE_WOOD().getId(), ResourceLocation("minecraft", "non_flammable_wood"));
}

TEST_F(ItemTagsTest, NonFlammableWoodContainsCrimsonShelf)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_shelf"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodContainsWarpedShelf)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_shelf"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

TEST_F(ItemTagsTest, NonFlammableWoodDoesNotContainOakShelf)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_shelf"));
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIn(item::tag::ItemTags::NON_FLAMMABLE_WOOD()));
}

// ============================================================================
// WOODEN_SHELVES 标签测试
// 参考: net.minecraft.tags.ItemTags.WOODEN_SHELVES
// ============================================================================

TEST_F(ItemTagsTest, WoodenShelvesContainsOakShelf)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "oak_shelf"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_SHELVES()));
}

TEST_F(ItemTagsTest, WoodenShelvesContainsCrimsonShelf)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_shelf"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_SHELVES()));
}

TEST_F(ItemTagsTest, WoodenShelvesContainsWarpedShelf)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_shelf"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::WOODEN_SHELVES()));
}

TEST_F(ItemTagsTest, WoodenShelvesDoesNotContainBookshelf)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "bookshelf"));
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIn(item::tag::ItemTags::WOODEN_SHELVES()));
}

TEST_F(ItemTagsTest, WoodenShelvesTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::WOODEN_SHELVES().getId(), ResourceLocation("minecraft", "wooden_shelves"));
}

TEST_F(ItemTagsTest, WoodenShelvesContainsAll12Shelves)
{
    const auto& items = item::tag::ItemTags::WOODEN_SHELVES().getItems();
    EXPECT_EQ(items.size(), 12u);
}

// ============================================================================
// BEDS 标签测试
// ============================================================================

TEST_F(ItemTagsTest, BedsContainsWhiteBed)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_bed"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::BEDS()));
}

TEST_F(ItemTagsTest, BedsContainsAll16Colors)
{
    // minecraft:beds 标签应包含全部 16 色床物品
    const char* bedNames[] = {"white_bed",
        "orange_bed",
        "magenta_bed",
        "light_blue_bed",
        "yellow_bed",
        "lime_bed",
        "pink_bed",
        "gray_bed",
        "light_gray_bed",
        "cyan_bed",
        "purple_bed",
        "blue_bed",
        "brown_bed",
        "green_bed",
        "red_bed",
        "black_bed"};

    for (const char* name : bedNames) {
        Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Item minecraft:" << name << " should be registered";
        EXPECT_TRUE(item->isIn(item::tag::ItemTags::BEDS())) << "minecraft:" << name << " should be in beds tag";
    }
}

TEST_F(ItemTagsTest, BedsDoesNotContainNonBedItems)
{
    // 床标签不应包含非床物品
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(stone->isIn(item::tag::ItemTags::BEDS()));

    Item* whiteWool = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "white_wool"));
    if (whiteWool != nullptr) {
        EXPECT_FALSE(whiteWool->isIn(item::tag::ItemTags::BEDS()));
    }
}

TEST_F(ItemTagsTest, BedsTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::BEDS().getId(), ResourceLocation("minecraft", "beds"));
}

TEST_F(ItemTagsTest, BedsTagContains16Items)
{
    const auto& items = item::tag::ItemTags::BEDS().getItems();
    EXPECT_EQ(items.size(), 16u);
}

// ============================================================================
// SHULKER_BOXES 标签测试
// 参考: net.minecraft.tags.ItemTags.SHULKER_BOXES
// ============================================================================

TEST_F(ItemTagsTest, ShulkerBoxesContainsUncoloredShulkerBox)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "shulker_box"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIn(item::tag::ItemTags::SHULKER_BOXES()));
}

TEST_F(ItemTagsTest, ShulkerBoxesContainsAll16ColoredVariants)
{
    // minecraft:shulker_boxes 标签应包含全部 16 色潜影盒物品
    const char* shulkerBoxNames[] = {"white_shulker_box",
        "orange_shulker_box",
        "magenta_shulker_box",
        "light_blue_shulker_box",
        "yellow_shulker_box",
        "lime_shulker_box",
        "pink_shulker_box",
        "gray_shulker_box",
        "light_gray_shulker_box",
        "cyan_shulker_box",
        "purple_shulker_box",
        "blue_shulker_box",
        "brown_shulker_box",
        "green_shulker_box",
        "red_shulker_box",
        "black_shulker_box"};

    for (const char* name : shulkerBoxNames) {
        Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Item minecraft:" << name << " should be registered";
        EXPECT_TRUE(item->isIn(item::tag::ItemTags::SHULKER_BOXES()))
            << "minecraft:" << name << " should be in shulker_boxes tag";
    }
}

TEST_F(ItemTagsTest, ShulkerBoxesDoesNotContainNonShulkerBoxItems)
{
    // 潜影盒标签不应包含非潜影盒物品
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(stone->isIn(item::tag::ItemTags::SHULKER_BOXES()));

    Item* chest = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "chest"));
    if (chest != nullptr) {
        EXPECT_FALSE(chest->isIn(item::tag::ItemTags::SHULKER_BOXES()));
    }
}

TEST_F(ItemTagsTest, ShulkerBoxesTagIdIsCorrect)
{
    EXPECT_EQ(item::tag::ItemTags::SHULKER_BOXES().getId(), ResourceLocation("minecraft", "shulker_boxes"));
}

TEST_F(ItemTagsTest, ShulkerBoxesTagContains17Items)
{
    // 无色潜影盒 + 16 色潜影盒 = 17 个物品
    const auto& items = item::tag::ItemTags::SHULKER_BOXES().getItems();
    EXPECT_EQ(items.size(), 17u);
}
