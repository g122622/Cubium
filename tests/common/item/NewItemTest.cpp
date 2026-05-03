#include <gtest/gtest.h>
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/tool/ShearsItem.hpp"
#include "item/items/special/FlintAndSteelItem.hpp"
#include "item/items/special/MilkBucketItem.hpp"
#include "item/items/special/EnchantedBookItem.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/VanillaBlocks.hpp"

using namespace mc;

// ============================================================================
// ShearsItem 测试
// ============================================================================

class ShearsItemTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
    }
};

TEST_F(ShearsItemTest, ShearsRegistered) {
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);
    EXPECT_EQ(shears->itemLocation(), ResourceLocation("minecraft:shears"));
}

TEST_F(ShearsItemTest, ShearsHasCorrectDurability) {
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);

    // MC 1.16.5: 剪刀耐久度为 238
    EXPECT_EQ(shears->maxDamage(), 238);
    EXPECT_TRUE(shears->isDamageable());
}

TEST_F(ShearsItemTest, ShearsIsNotStackable) {
    Item* shears = ItemRegistry::instance().getItem(ResourceLocation("minecraft:shears"));
    ASSERT_NE(shears, nullptr);

    // 有耐久度的物品堆叠数为1
    EXPECT_EQ(shears->maxStackSize(), 1);
}

TEST_F(ShearsItemTest, ShearsStackDamage) {
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

TEST_F(ShearsItemTest, ShearsBreaksAtMaxDamage) {
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
    void SetUp() override {
        Items::initialize();
    }
};

TEST_F(FlintAndSteelItemTest, FlintAndSteelRegistered) {
    Item* flintAndSteel = ItemRegistry::instance().getItem(ResourceLocation("minecraft:flint_and_steel"));
    ASSERT_NE(flintAndSteel, nullptr);
    EXPECT_EQ(flintAndSteel->itemLocation(), ResourceLocation("minecraft:flint_and_steel"));
}

TEST_F(FlintAndSteelItemTest, FlintAndSteelHasCorrectDurability) {
    Item* flintAndSteel = ItemRegistry::instance().getItem(ResourceLocation("minecraft:flint_and_steel"));
    ASSERT_NE(flintAndSteel, nullptr);

    // MC 1.16.5: 打火石耐久度为 64
    EXPECT_EQ(flintAndSteel->maxDamage(), 64);
    EXPECT_TRUE(flintAndSteel->isDamageable());
}

TEST_F(FlintAndSteelItemTest, FlintAndSteelIsNotStackable) {
    Item* flintAndSteel = ItemRegistry::instance().getItem(ResourceLocation("minecraft:flint_and_steel"));
    ASSERT_NE(flintAndSteel, nullptr);

    EXPECT_EQ(flintAndSteel->maxStackSize(), 1);
}

TEST_F(FlintAndSteelItemTest, FlintAndSteelUseDuration) {
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
    void SetUp() override {
        Items::initialize();
    }
};

TEST_F(MilkBucketItemTest, MilkBucketRegistered) {
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);
    EXPECT_EQ(milkBucket->itemLocation(), ResourceLocation("minecraft:milk_bucket"));
}

TEST_F(MilkBucketItemTest, MilkBucketIsNotStackable) {
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);

    // 桶类物品堆叠数为1
    EXPECT_EQ(milkBucket->maxStackSize(), 1);
}

TEST_F(MilkBucketItemTest, MilkBucketUseDuration) {
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);

    ItemStack stack(milkBucket, 1);
    // MC 1.16.5: 牛奶桶饮用时间为 32 ticks
    EXPECT_EQ(milkBucket->getUseDuration(stack), 32);
}

TEST_F(MilkBucketItemTest, MilkBucketUseAction) {
    Item* milkBucket = ItemRegistry::instance().getItem(ResourceLocation("minecraft:milk_bucket"));
    ASSERT_NE(milkBucket, nullptr);

    ItemStack stack(milkBucket, 1);
    // 牛奶桶是饮用动作
    EXPECT_EQ(milkBucket->getUseAction(stack), UseAction::Drink);
}

TEST_F(MilkBucketItemTest, MilkBucketHasContainerItem) {
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
    void SetUp() override {
        Items::initialize();
    }
};

TEST_F(ToolItemTest, AllTierToolsRegistered) {
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

TEST_F(ToolItemTest, AllTiersHaveCorrectDurability) {
    // MC 1.16.5 各层级耐久度
    struct TierDurability {
        const char* tier;
        int durability;
    };

    std::vector<TierDurability> tiers = {
        {"wooden", 59},
        {"stone", 131},
        {"iron", 250},
        {"golden", 32},
        {"diamond", 1561},
        {"netherite", 2031}
    };

    for (const auto& td : tiers) {
        std::string itemId = std::string("minecraft:") + td.tier + "_pickaxe";
        Item* item = ItemRegistry::instance().getItem(ResourceLocation(itemId));
        ASSERT_NE(item, nullptr) << "Missing: " << itemId;
        EXPECT_EQ(item->maxDamage(), td.durability)
            << "Wrong durability for " << itemId;
    }
}

TEST_F(ToolItemTest, ShearsAndFlintAndSteelAreDamageable) {
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
    void SetUp() override {
        Items::initialize();
        VanillaBlocks::initialize();
    }
};

TEST_F(BlockTagsTest, WoolTagExists) {
    // 验证 WOOL 标签已创建
    EXPECT_NO_THROW({
        BlockTag& woolTag = BlockTags::WOOL();
        EXPECT_TRUE(true);
    });
}

TEST_F(BlockTagsTest, WoolTagContainsWoolBlocks) {
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
    void SetUp() override {
        Items::initialize();
    }
};

TEST_F(EnchantedBookItemTest, EnchantedBookRegistered) {
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);
    EXPECT_EQ(enchantedBook->itemLocation(), ResourceLocation("minecraft:enchanted_book"));
}

TEST_F(EnchantedBookItemTest, EnchantedBookNotStackable) {
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);
    EXPECT_EQ(enchantedBook->maxStackSize(), 1);
}

TEST_F(EnchantedBookItemTest, EnchantedBookHasEffect) {
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);

    ItemStack stack(enchantedBook, 1);
    EXPECT_TRUE(enchantedBook->hasEffect(stack));
}

TEST_F(EnchantedBookItemTest, EnchantedBookEnchantability) {
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);
    EXPECT_EQ(enchantedBook->getItemEnchantability(), 1);
}

TEST_F(EnchantedBookItemTest, EmptyBookHasNoEnchantments) {
    Item* enchantedBook = ItemRegistry::instance().getItem(ResourceLocation("minecraft:enchanted_book"));
    ASSERT_NE(enchantedBook, nullptr);

    ItemStack stack(enchantedBook, 1);
    EXPECT_FALSE(item::items::EnchantedBookItem::hasEnchantments(stack));
    EXPECT_EQ(item::items::EnchantedBookItem::getEnchantmentCount(stack), 0u);
    EXPECT_TRUE(item::items::EnchantedBookItem::getEnchantments(stack).empty());
}

TEST_F(EnchantedBookItemTest, BookRegistered) {
    Item* book = ItemRegistry::instance().getItem(ResourceLocation("minecraft:book"));
    ASSERT_NE(book, nullptr);
    EXPECT_EQ(book->maxStackSize(), 64);
}

// ============================================================================
// Nether Wood Blocks 测试
// ============================================================================

class NetherWoodTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        VanillaBlocks::initialize();
    }
};

TEST_F(NetherWoodTest, CrimsonStemRegistered) {
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:crimson_stem"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:crimson_stem"));
}

TEST_F(NetherWoodTest, WarpedStemRegistered) {
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_stem"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:warped_stem"));
}

TEST_F(NetherWoodTest, StrippedCrimsonStemRegistered) {
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stripped_crimson_stem"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:stripped_crimson_stem"));
}

TEST_F(NetherWoodTest, StrippedWarpedStemRegistered) {
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stripped_warped_stem"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:stripped_warped_stem"));
}

TEST_F(NetherWoodTest, CrimsonHyphaeRegistered) {
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:crimson_hyphae"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:crimson_hyphae"));
}

TEST_F(NetherWoodTest, WarpedHyphaeRegistered) {
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:warped_hyphae"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:warped_hyphae"));
}

TEST_F(NetherWoodTest, StrippedCrimsonHyphaeRegistered) {
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stripped_crimson_hyphae"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:stripped_crimson_hyphae"));
}

TEST_F(NetherWoodTest, StrippedWarpedHyphaeRegistered) {
    Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:stripped_warped_hyphae"));
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft:stripped_warped_hyphae"));
}
