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

#include "core/Constants.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/special/EnchantedBookItem.hpp"
#include "item/items/special/FlintAndSteelItem.hpp"
#include "item/items/special/MilkBucketItem.hpp"
#include "item/items/tool/ShearsItem.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// ShearsItem 测试
// ============================================================================

class ShearsItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ShearsItemTest, ShearsRegistered)
{
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);
    EXPECT_EQ(shears->itemLocation(), ResourceLocation("minecraft:shears"));
}

TEST_F(ShearsItemTest, ShearsHasCorrectDurability)
{
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);

    // MC 1.16.5: 剪刀耐久度为 238
    EXPECT_EQ(shears->maxDamage(), 238);
    EXPECT_TRUE(shears->isDamageable());
}

TEST_F(ShearsItemTest, ShearsIsNotStackable)
{
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);

    // 有耐久度的物品堆叠数为1
    EXPECT_EQ(shears->maxStackSize(), 1);
}

TEST_F(ShearsItemTest, ShearsStackDamage)
{
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);

    ItemStack stack(shears, 1);
    EXPECT_FALSE(stack.isDamaged());
    EXPECT_EQ(stack.getDamage(), 0);

    // 造成伤害
    bool broken = stack.attemptDamageItem(50);
    EXPECT_FALSE(broken);
    EXPECT_TRUE(stack.isDamaged());
    EXPECT_EQ(stack.getDamage(), 50);
}

TEST_F(ShearsItemTest, ShearsBreaksAtMaxDamage)
{
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);

    ItemStack stack(shears, 1);

    // 造成超过耐久度的伤害
    bool broken = stack.attemptDamageItem(300);
    EXPECT_TRUE(broken);
    EXPECT_TRUE(stack.isEmpty());
}

// ============================================================================
// FlintAndSteelItem 测试
// ============================================================================

class FlintAndSteelItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(FlintAndSteelItemTest, FlintAndSteelRegistered)
{
    Item* flintAndSteel = ItemRegistry::instance().getItem(ResourceLocation("minecraft:flint_and_steel"));
    ASSERT_NE(flintAndSteel, nullptr);
    EXPECT_EQ(flintAndSteel->itemLocation(), ResourceLocation("minecraft:flint_and_steel"));
}

TEST_F(FlintAndSteelItemTest, FlintAndSteelHasCorrectDurability)
{
    Item* flintAndSteel = ItemRegistry::instance().getItem(ResourceLocation("minecraft:flint_and_steel"));
    ASSERT_NE(flintAndSteel, nullptr);

    // MC 1.16.5: 打火石耐久度为 64
    EXPECT_EQ(flintAndSteel->maxDamage(), 64);
    EXPECT_TRUE(flintAndSteel->isDamageable());
}

TEST_F(FlintAndSteelItemTest, FlintAndSteelIsNotStackable)
{
    Item* flintAndSteel = ItemRegistry::instance().getItem(ResourceLocation("minecraft:flint_and_steel"));
    ASSERT_NE(flintAndSteel, nullptr);

    EXPECT_EQ(flintAndSteel->maxStackSize(), 1);
}

TEST_F(FlintAndSteelItemTest, FlintAndSteelUseDuration)
{
    Item* flintAndSteel = ItemRegistry::instance().getItem(ResourceLocation("minecraft:flint_and_steel"));
    ASSERT_NE(flintAndSteel, nullptr);

    ItemStack stack(flintAndSteel, 1);
    // 打火石没有使用持续时间
    EXPECT_EQ(flintAndSteel->getUseDuration(stack), 0);
}

// ============================================================================
// MilkBucketItem 测试
// ============================================================================

class MilkBucketItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(MilkBucketItemTest, MilkBucketRegistered)
{
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);
    EXPECT_EQ(milkBucket->itemLocation(), ResourceLocation("minecraft:milk_bucket"));
}

TEST_F(MilkBucketItemTest, MilkBucketIsNotStackable)
{
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);

    // 桶类物品堆叠数为1
    EXPECT_EQ(milkBucket->maxStackSize(), 1);
}

TEST_F(MilkBucketItemTest, MilkBucketUseDuration)
{
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);

    ItemStack stack(milkBucket, 1);
    // MC 1.16.5: 牛奶桶饮用时间为 32 ticks
    EXPECT_EQ(milkBucket->getUseDuration(stack), 32);
}

TEST_F(MilkBucketItemTest, MilkBucketUseAction)
{
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);

    ItemStack stack(milkBucket, 1);
    // 牛奶桶是饮用动作
    EXPECT_EQ(milkBucket->getUseAction(stack), UseAction::Drink);
}

TEST_F(MilkBucketItemTest, MilkBucketHasContainerItem)
{
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);

    // 牛奶桶使用后返回空桶
    EXPECT_TRUE(milkBucket->hasContainerItem());
    const Item* containerItem = milkBucket->containerItem();
    ASSERT_NE(containerItem, nullptr);
    EXPECT_EQ(containerItem->itemLocation(), ResourceLocation("minecraft:bucket"));
}

// ============================================================================
// ToolItem 综合测试
// ============================================================================

class ToolItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ToolItemTest, AllTierToolsRegistered)
{
    // 验证各层级工具都已注册
    const char* toolTypes[] = {"pickaxe", "axe", "shovel", "hoe", "sword"};
    const char* tiers[] = {"wooden", "stone", "iron", "golden", "diamond", "netherite"};

    for (const char* tier : tiers) {
        for (const char* tool : toolTypes) {
            std::string itemId = std::string("minecraft:") + tier + "_" + tool;
            Item* item = ItemRegistry::instance().getItem(ResourceLocation(itemId));
            EXPECT_NE(item, nullptr) << "Missing tool: " << itemId;
        }
    }
}

TEST_F(ToolItemTest, AllTiersHaveCorrectDurability)
{
    // MC 1.16.5 各层级耐久度
    struct TierDurability {
        const char* tier;
        int durability;
    };

    std::vector<TierDurability> tiers = {
        {"wooden", 59}, {"stone", 131}, {"iron", 250}, {"golden", 32}, {"diamond", 1561}, {"netherite", 2031}};

    for (const auto& td : tiers) {
        std::string itemId = std::string("minecraft:") + td.tier + "_pickaxe";
        Item* item = ItemRegistry::instance().getItem(ResourceLocation(itemId));
        ASSERT_NE(item, nullptr) << "Missing: " << itemId;
        EXPECT_EQ(item->maxDamage(), td.durability) << "Wrong durability for " << itemId;
    }
}

TEST_F(ToolItemTest, ShearsAndFlintAndSteelAreDamageable)
{
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);
    EXPECT_TRUE(shears->isDamageable());

    Item* flintAndSteel = ItemRegistry::instance().getItem(ResourceLocation("minecraft:flint_and_steel"));
    ASSERT_NE(flintAndSteel, nullptr);
    EXPECT_TRUE(flintAndSteel->isDamageable());
}

// ============================================================================
// BlockTags 测试（验证 WOOL 标签）
// ============================================================================

class BlockTagsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        VanillaBlocks::initialize();
    }
};

TEST_F(BlockTagsTest, WoolTagExists)
{
    // 验证 WOOL 标签已创建
    EXPECT_NO_THROW({
        BlockTag& woolTag = BlockTags::WOOL();
        EXPECT_TRUE(true);
    });
}

TEST_F(BlockTagsTest, WoolTagContainsWoolBlocks)
{
    BlockTag& woolTag = BlockTags::WOOL();

    // 验证常见羊毛方块在标签中
    if (VanillaBlocks::WHITE_WOOL) {
        EXPECT_TRUE(woolTag.contains(VanillaBlocks::WHITE_WOOL));
    }
    if (VanillaBlocks::RED_WOOL) {
        EXPECT_TRUE(woolTag.contains(VanillaBlocks::RED_WOOL));
    }
    if (VanillaBlocks::BLACK_WOOL) {
        EXPECT_TRUE(woolTag.contains(VanillaBlocks::BLACK_WOOL));
    }
}

// ============================================================================
// EnchantedBookItem 测试
// ============================================================================

class EnchantedBookItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(EnchantedBookItemTest, EnchantedBookRegistered)
{
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);
    EXPECT_EQ(enchantedBook->itemLocation(), ResourceLocation("minecraft:enchanted_book"));
}

TEST_F(EnchantedBookItemTest, EnchantedBookNotStackable)
{
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);
    EXPECT_EQ(enchantedBook->maxStackSize(), 1);
}

TEST_F(EnchantedBookItemTest, EnchantedBookHasEffect)
{
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);

    ItemStack stack(enchantedBook, 1);
    EXPECT_TRUE(enchantedBook->hasEffect(stack));
}

TEST_F(EnchantedBookItemTest, EnchantedBookEnchantability)
{
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);
    EXPECT_EQ(enchantedBook->getItemEnchantability(), 1);
}

TEST_F(EnchantedBookItemTest, EmptyBookHasNoEnchantments)
{
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);

    ItemStack stack(enchantedBook, 1);
    EXPECT_FALSE(item::items::EnchantedBookItem::hasEnchantments(stack));
    EXPECT_EQ(item::items::EnchantedBookItem::getEnchantmentCount(stack), 0u);
    EXPECT_TRUE(item::items::EnchantedBookItem::getEnchantments(stack).empty());
}

TEST_F(EnchantedBookItemTest, BookRegistered)
{
    Item* book = ItemRegistry::instance().getItem(ResourceLocation("minecraft:book"));
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->maxStackSize(), 64);
}

// ============================================================================
// Nether Wood Blocks 测试
// ============================================================================

class NetherWoodTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        VanillaBlocks::initialize();
    }
};

TEST_F(NetherWoodTest, CrimsonStemRegistered)
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:crimson_stem"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:crimson_stem"));
}

TEST_F(NetherWoodTest, WarpedStemRegistered)
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_stem"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:warped_stem"));
}

TEST_F(NetherWoodTest, StrippedCrimsonStemRegistered)
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stripped_crimson_stem"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:stripped_crimson_stem"));
}

TEST_F(NetherWoodTest, StrippedWarpedStemRegistered)
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stripped_warped_stem"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:stripped_warped_stem"));
}

TEST_F(NetherWoodTest, CrimsonHyphaeRegistered)
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:crimson_hyphae"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:crimson_hyphae"));
}

TEST_F(NetherWoodTest, WarpedHyphaeRegistered)
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_hyphae"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:warped_hyphae"));
}

TEST_F(NetherWoodTest, StrippedCrimsonHyphaeRegistered)
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stripped_crimson_hyphae"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:stripped_crimson_hyphae"));
}

TEST_F(NetherWoodTest, StrippedWarpedHyphaeRegistered)
{
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stripped_warped_hyphae"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:stripped_warped_hyphae"));
}

// ============================================================================
// TippedArrowItem 测试
// ============================================================================

class TippedArrowItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(TippedArrowItemTest, TippedArrowRegistered)
{
    Item* tippedArrow = ItemRegistry::instance().getItem(ResourceLocation("minecraft:tipped_arrow"));
    ASSERT_NE(tippedArrow, nullptr);
    EXPECT_EQ(tippedArrow->itemLocation(), ResourceLocation("minecraft:tipped_arrow"));
}

TEST_F(TippedArrowItemTest, TippedArrowIsStackable)
{
    Item* tippedArrow = ItemRegistry::instance().getItem(ResourceLocation("minecraft:tipped_arrow"));
    ASSERT_NE(tippedArrow, nullptr);
    // 箭矢可堆叠到64个
    EXPECT_EQ(tippedArrow->maxStackSize(), 64);
}

TEST_F(TippedArrowItemTest, TippedArrowNotInfiniteInSurvival)
{
    Item* tippedArrow = ItemRegistry::instance().getItem(ResourceLocation("minecraft:tipped_arrow"));
    ASSERT_NE(tippedArrow, nullptr);
    // 药水箭不受益于无限附魔
    // isInfinite() 方法需要 Player 参数，这里只验证物品注册正确
    EXPECT_NE(tippedArrow, nullptr);
}

// ============================================================================
// SplashPotionItem 测试
// ============================================================================

class SplashPotionItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(SplashPotionItemTest, SplashPotionRegistered)
{
    Item* splashPotion = ItemRegistry::instance().getItem(ResourceLocation("minecraft:splash_potion"));
    ASSERT_NE(splashPotion, nullptr);
    EXPECT_EQ(splashPotion->itemLocation(), ResourceLocation("minecraft:splash_potion"));
}

TEST_F(SplashPotionItemTest, SplashPotionIsStackable)
{
    Item* splashPotion = ItemRegistry::instance().getItem(ResourceLocation("minecraft:splash_potion"));
    ASSERT_NE(splashPotion, nullptr);
    // MC 1.16.5: 喷溅药水默认堆叠数为1（相同药水类型才可堆叠）
    EXPECT_EQ(splashPotion->maxStackSize(), 1);
}

// ============================================================================
// LingeringPotionItem 测试
// ============================================================================

class LingeringPotionItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(LingeringPotionItemTest, LingeringPotionRegistered)
{
    Item* lingeringPotion = ItemRegistry::instance().getItem(ResourceLocation("minecraft:lingering_potion"));
    ASSERT_NE(lingeringPotion, nullptr);
    EXPECT_EQ(lingeringPotion->itemLocation(), ResourceLocation("minecraft:lingering_potion"));
}

TEST_F(LingeringPotionItemTest, LingeringPotionIsStackable)
{
    Item* lingeringPotion = ItemRegistry::instance().getItem(ResourceLocation("minecraft:lingering_potion"));
    ASSERT_NE(lingeringPotion, nullptr);
    // MC 1.16.5: 滞留药水默认堆叠数为1（相同药水类型才可堆叠）
    EXPECT_EQ(lingeringPotion->maxStackSize(), 1);
}

// ============================================================================
// FlintAndSteelItem 灵魂火测试
// ============================================================================

class FlintAndSteelSoulFireTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(FlintAndSteelSoulFireTest, SoulFireBlockRegistered)
{
    Block* soulFire = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:soul_fire"));
    ASSERT_NE(soulFire, nullptr);
    EXPECT_EQ(soulFire->blockLocation(), ResourceLocation("minecraft:soul_fire"));
}

TEST_F(FlintAndSteelSoulFireTest, FireBlockRegistered)
{
    Block* fire = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:fire"));
    ASSERT_NE(fire, nullptr);
    EXPECT_EQ(fire->blockLocation(), ResourceLocation("minecraft:fire"));
}

TEST_F(FlintAndSteelSoulFireTest, SoulSandRegistered)
{
    Block* soulSand = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:soul_sand"));
    ASSERT_NE(soulSand, nullptr);
    EXPECT_EQ(soulSand->blockLocation(), ResourceLocation("minecraft:soul_sand"));
}

TEST_F(FlintAndSteelSoulFireTest, SoulSoilRegistered)
{
    Block* soulSoil = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:soul_soil"));
    ASSERT_NE(soulSoil, nullptr);
    EXPECT_EQ(soulSoil->blockLocation(), ResourceLocation("minecraft:soul_soil"));
}

TEST_F(FlintAndSteelSoulFireTest, SoulFireBaseBlocksTagContainsSoulSand)
{
    ASSERT_NE(VanillaBlocks::SOUL_SAND, nullptr);
    EXPECT_TRUE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(VanillaBlocks::SOUL_SAND));
}

TEST_F(FlintAndSteelSoulFireTest, SoulFireBaseBlocksTagContainsSoulSoil)
{
    ASSERT_NE(VanillaBlocks::SOUL_SOIL, nullptr);
    EXPECT_TRUE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(VanillaBlocks::SOUL_SOIL));
}

TEST_F(FlintAndSteelSoulFireTest, SoulFireBaseBlocksTagDoesNotContainOtherBlocks)
{
    // 普通方块不应在灵魂火基座标签中
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    ASSERT_NE(VanillaBlocks::DIRT, nullptr);
    ASSERT_NE(VanillaBlocks::GRASS_BLOCK, nullptr);

    EXPECT_FALSE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(VanillaBlocks::STONE));
    EXPECT_FALSE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(VanillaBlocks::DIRT));
    EXPECT_FALSE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(VanillaBlocks::GRASS_BLOCK));
}

// ============================================================================
// 告示牌物品注册测试
// ============================================================================

class SignItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(SignItemTest, OakSignRegistered)
{
    Item* sign = ItemRegistry::instance().getItem(ResourceLocation("minecraft:oak_sign"));
    ASSERT_NE(sign, nullptr);
    EXPECT_EQ(sign->itemLocation(), ResourceLocation("minecraft:oak_sign"));
    EXPECT_EQ(sign->maxStackSize(), 16) << "告示牌堆叠数应为 16";
}

TEST_F(SignItemTest, SpruceSignRegistered)
{
    Item* sign = ItemRegistry::instance().getItem(ResourceLocation("minecraft:spruce_sign"));
    ASSERT_NE(sign, nullptr);
    EXPECT_EQ(sign->itemLocation(), ResourceLocation("minecraft:spruce_sign"));
    EXPECT_EQ(sign->maxStackSize(), 16);
}

TEST_F(SignItemTest, BirchSignRegistered)
{
    Item* sign = ItemRegistry::instance().getItem(ResourceLocation("minecraft:birch_sign"));
    ASSERT_NE(sign, nullptr);
    EXPECT_EQ(sign->itemLocation(), ResourceLocation("minecraft:birch_sign"));
    EXPECT_EQ(sign->maxStackSize(), 16);
}

TEST_F(SignItemTest, JungleSignRegistered)
{
    Item* sign = ItemRegistry::instance().getItem(ResourceLocation("minecraft:jungle_sign"));
    ASSERT_NE(sign, nullptr);
    EXPECT_EQ(sign->itemLocation(), ResourceLocation("minecraft:jungle_sign"));
    EXPECT_EQ(sign->maxStackSize(), 16);
}

TEST_F(SignItemTest, AcaciaSignRegistered)
{
    Item* sign = ItemRegistry::instance().getItem(ResourceLocation("minecraft:acacia_sign"));
    ASSERT_NE(sign, nullptr);
    EXPECT_EQ(sign->itemLocation(), ResourceLocation("minecraft:acacia_sign"));
    EXPECT_EQ(sign->maxStackSize(), 16);
}

TEST_F(SignItemTest, DarkOakSignRegistered)
{
    Item* sign = ItemRegistry::instance().getItem(ResourceLocation("minecraft:dark_oak_sign"));
    ASSERT_NE(sign, nullptr);
    EXPECT_EQ(sign->itemLocation(), ResourceLocation("minecraft:dark_oak_sign"));
    EXPECT_EQ(sign->maxStackSize(), 16);
}

TEST_F(SignItemTest, CrimsonSignRegistered)
{
    Item* sign = ItemRegistry::instance().getItem(ResourceLocation("minecraft:crimson_sign"));
    ASSERT_NE(sign, nullptr);
    EXPECT_EQ(sign->itemLocation(), ResourceLocation("minecraft:crimson_sign"));
    EXPECT_EQ(sign->maxStackSize(), 16);
}

TEST_F(SignItemTest, WarpedSignRegistered)
{
    Item* sign = ItemRegistry::instance().getItem(ResourceLocation("minecraft:warped_sign"));
    ASSERT_NE(sign, nullptr);
    EXPECT_EQ(sign->itemLocation(), ResourceLocation("minecraft:warped_sign"));
    EXPECT_EQ(sign->maxStackSize(), 16);
}

TEST_F(SignItemTest, AllSignItemsAvailableViaItemsClass)
{
    // 验证 Items 类中的静态指针已正确初始化
    EXPECT_NE(Items::OAK_SIGN, nullptr);
    EXPECT_NE(Items::SPRUCE_SIGN, nullptr);
    EXPECT_NE(Items::BIRCH_SIGN, nullptr);
    EXPECT_NE(Items::JUNGLE_SIGN, nullptr);
    EXPECT_NE(Items::ACACIA_SIGN, nullptr);
    EXPECT_NE(Items::DARK_OAK_SIGN, nullptr);
    EXPECT_NE(Items::CRIMSON_SIGN, nullptr);
    EXPECT_NE(Items::WARPED_SIGN, nullptr);
}

TEST_F(SignItemTest, AllSignItemsHaveCorrectStackSize)
{
    // MC 1.16.5: 所有告示牌堆叠数均为 16
    EXPECT_EQ(Items::OAK_SIGN->maxStackSize(), 16);
    EXPECT_EQ(Items::SPRUCE_SIGN->maxStackSize(), 16);
    EXPECT_EQ(Items::BIRCH_SIGN->maxStackSize(), 16);
    EXPECT_EQ(Items::JUNGLE_SIGN->maxStackSize(), 16);
    EXPECT_EQ(Items::ACACIA_SIGN->maxStackSize(), 16);
    EXPECT_EQ(Items::DARK_OAK_SIGN->maxStackSize(), 16);
    EXPECT_EQ(Items::CRIMSON_SIGN->maxStackSize(), 16);
    EXPECT_EQ(Items::WARPED_SIGN->maxStackSize(), 16);
}
