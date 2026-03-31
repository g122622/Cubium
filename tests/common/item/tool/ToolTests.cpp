#include <gtest/gtest.h>
#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"
#include "item/Items.hpp"
#include "item/tier/ItemTiers.hpp"
#include "item/items/tool/ToolType.hpp"
#include "item/items/tool/PickaxeItem.hpp"
#include "item/items/tool/AxeItem.hpp"
#include "item/items/tool/ShovelItem.hpp"
#include "item/items/tool/HoeItem.hpp"
#include "item/items/tool/SwordItem.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"

using namespace mc;
using namespace mc::item;
using namespace mc::item::tier;
using namespace mc::item::tool;

// ============================================================================
// ItemTier Tests
// ============================================================================

class ItemTierTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Initialize items first (needed for repair materials)
        Items::initialize();
        ItemTiers::initialize();
    }
};

TEST_F(ItemTierTest, WoodTierValues) {
    const auto& tier = ItemTiers::WOOD();
    EXPECT_EQ(tier.getMaxUses(), 59);
    EXPECT_FLOAT_EQ(tier.getEfficiency(), 2.0f);
    EXPECT_FLOAT_EQ(tier.getAttackDamage(), 0.0f);
    EXPECT_EQ(tier.getHarvestLevel(), 0);
    EXPECT_EQ(tier.getEnchantability(), 15);
}

TEST_F(ItemTierTest, StoneTierValues) {
    const auto& tier = ItemTiers::STONE();
    EXPECT_EQ(tier.getMaxUses(), 131);
    EXPECT_FLOAT_EQ(tier.getEfficiency(), 4.0f);
    EXPECT_FLOAT_EQ(tier.getAttackDamage(), 1.0f);
    EXPECT_EQ(tier.getHarvestLevel(), 1);
    EXPECT_EQ(tier.getEnchantability(), 5);
}

TEST_F(ItemTierTest, IronTierValues) {
    const auto& tier = ItemTiers::IRON();
    EXPECT_EQ(tier.getMaxUses(), 250);
    EXPECT_FLOAT_EQ(tier.getEfficiency(), 6.0f);
    EXPECT_FLOAT_EQ(tier.getAttackDamage(), 2.0f);
    EXPECT_EQ(tier.getHarvestLevel(), 2);
    EXPECT_EQ(tier.getEnchantability(), 14);
}

TEST_F(ItemTierTest, DiamondTierValues) {
    const auto& tier = ItemTiers::DIAMOND();
    EXPECT_EQ(tier.getMaxUses(), 1561);
    EXPECT_FLOAT_EQ(tier.getEfficiency(), 8.0f);
    EXPECT_FLOAT_EQ(tier.getAttackDamage(), 3.0f);
    EXPECT_EQ(tier.getHarvestLevel(), 3);
    EXPECT_EQ(tier.getEnchantability(), 10);
}

TEST_F(ItemTierTest, GoldTierValues) {
    const auto& tier = ItemTiers::GOLD();
    EXPECT_EQ(tier.getMaxUses(), 32);  // Very low durability
    EXPECT_FLOAT_EQ(tier.getEfficiency(), 12.0f);  // Highest efficiency
    EXPECT_FLOAT_EQ(tier.getAttackDamage(), 0.0f);
    EXPECT_EQ(tier.getHarvestLevel(), 0);  // Same as wood
    EXPECT_EQ(tier.getEnchantability(), 22);  // Highest enchantability
}

TEST_F(ItemTierTest, NetheriteTierValues) {
    const auto& tier = ItemTiers::NETHERITE();
    EXPECT_EQ(tier.getMaxUses(), 2031);  // Highest durability
    EXPECT_FLOAT_EQ(tier.getEfficiency(), 9.0f);
    EXPECT_FLOAT_EQ(tier.getAttackDamage(), 4.0f);
    EXPECT_EQ(tier.getHarvestLevel(), 4);  // Highest harvest level
    EXPECT_EQ(tier.getEnchantability(), 15);
}

// ============================================================================
// Tool Item Tests
// ============================================================================

class ToolItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        Items::initialize();
        ItemTiers::initialize();
        VanillaBlocks::initialize();
    }
};

TEST_F(ToolItemTest, DiamondPickaxeDurability) {
    auto* pickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    // Diamond pickaxe should have 1561 durability
    EXPECT_EQ(pickaxe->maxDamage(), 1561);
}

TEST_F(ToolItemTest, IronPickaxeDurability) {
    auto* pickaxe = Items::IRON_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    // Iron pickaxe should have 250 durability
    EXPECT_EQ(pickaxe->maxDamage(), 250);
}

TEST_F(ToolItemTest, StonePickaxeDurability) {
    auto* pickaxe = Items::STONE_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    // Stone pickaxe should have 131 durability
    EXPECT_EQ(pickaxe->maxDamage(), 131);
}

TEST_F(ToolItemTest, WoodenPickaxeDurability) {
    auto* pickaxe = Items::WOODEN_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    // Wooden pickaxe should have 59 durability
    EXPECT_EQ(pickaxe->maxDamage(), 59);
}

TEST_F(ToolItemTest, GoldenPickaxeDurability) {
    auto* pickaxe = Items::GOLDEN_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    // Golden pickaxe should have 32 durability
    EXPECT_EQ(pickaxe->maxDamage(), 32);
}

TEST_F(ToolItemTest, PickaxeIsTieredItem) {
    auto* pickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    // Should have enchantability from tier
    EXPECT_EQ(pickaxe->getItemEnchantability(), 10);  // Diamond enchantability
}

TEST_F(ToolItemTest, SwordDamage) {
    auto* sword = Items::DIAMOND_SWORD;
    ASSERT_NE(sword, nullptr);

    // Diamond sword: base 3 + tier 3 = 6 damage (stored as attackDamage)
    // Max damage is durability
    EXPECT_EQ(sword->maxDamage(), 1561);  // Diamond durability
}

TEST_F(ToolItemTest, PickaxeEnchantability) {
    // Gold tools have highest enchantability (22)
    EXPECT_EQ(Items::GOLDEN_PICKAXE->getItemEnchantability(), 22);

    // Diamond tools have enchantability 10
    EXPECT_EQ(Items::DIAMOND_PICKAXE->getItemEnchantability(), 10);

    // Iron tools have enchantability 14
    EXPECT_EQ(Items::IRON_PICKAXE->getItemEnchantability(), 14);
}

TEST_F(ToolItemTest, ToolTypeConstants) {
    EXPECT_EQ(TOOL_TYPE_NONE, 0);
    EXPECT_EQ(TOOL_TYPE_PICKAXE, 1);
    EXPECT_EQ(TOOL_TYPE_AXE, 2);
    EXPECT_EQ(TOOL_TYPE_SHOVEL, 3);
    EXPECT_EQ(TOOL_TYPE_HOE, 4);
    EXPECT_EQ(TOOL_TYPE_SWORD, 5);
    EXPECT_EQ(TOOL_TYPE_SHEARS, 6);
}

// ============================================================================
// Tool Harvest Tests
// ============================================================================

class ToolHarvestTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 必须先初始化方块，因为工具注册时需要有效的方块指针
        VanillaBlocks::initialize();
        Items::initialize();
        // ItemTiers::initialize() 已经在 Items::initialize() 中调用
    }
};

TEST_F(ToolHarvestTest, PickaxeSpeedOnStone) {
    auto* pickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    ItemStack stack(*pickaxe, 1);
    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    const BlockState& state = stone->defaultState();

    f32 speed = pickaxe->getDestroySpeed(stack, state);
    // Diamond pickaxe should have 8.0 efficiency on stone
    EXPECT_FLOAT_EQ(speed, 8.0f);
}

TEST_F(ToolHarvestTest, WoodenPickaxeCannotHarvestDiamondOre) {
    auto* pickaxe = Items::WOODEN_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    auto* diamondOre = VanillaBlocks::DIAMOND_ORE;
    if (diamondOre == nullptr) {
        GTEST_SKIP() << "DIAMOND_ORE not registered yet";
    }

    const BlockState& state = diamondOre->defaultState();

    // Wooden pickaxe harvest level 0, diamond ore needs level 2
    EXPECT_FALSE(pickaxe->canHarvestBlock(state));
}

TEST_F(ToolHarvestTest, IronPickaxeCanHarvestDiamondOre) {
    auto* pickaxe = Items::IRON_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    auto* diamondOre = VanillaBlocks::DIAMOND_ORE;
    if (diamondOre == nullptr) {
        GTEST_SKIP() << "DIAMOND_ORE not registered yet";
    }

    const BlockState& state = diamondOre->defaultState();

    // Iron pickaxe harvest level 2, diamond ore needs level 2
    EXPECT_TRUE(pickaxe->canHarvestBlock(state));
}

TEST_F(ToolHarvestTest, DiamondPickaxeCanHarvestDiamondOre) {
    auto* pickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    auto* diamondOre = VanillaBlocks::DIAMOND_ORE;
    if (diamondOre == nullptr) {
        GTEST_SKIP() << "DIAMOND_ORE not registered yet";
    }

    const BlockState& state = diamondOre->defaultState();

    // Diamond pickaxe harvest level 3, diamond ore needs level 2
    EXPECT_TRUE(pickaxe->canHarvestBlock(state));
}

TEST_F(ToolHarvestTest, PickaxeSpeedOnDirt) {
    auto* pickaxe = Items::DIAMOND_PICKAXE;
    ASSERT_NE(pickaxe, nullptr);

    ItemStack stack(*pickaxe, 1);
    auto* dirt = VanillaBlocks::DIRT;
    ASSERT_NE(dirt, nullptr);

    const BlockState& state = dirt->defaultState();

    f32 speed = pickaxe->getDestroySpeed(stack, state);
    // Pickaxe is not effective on dirt (earth material)
    EXPECT_FLOAT_EQ(speed, 1.0f);
}

TEST_F(ToolHarvestTest, ShovelSpeedOnDirt) {
    auto* shovel = Items::DIAMOND_SHOVEL;
    ASSERT_NE(shovel, nullptr);

    ItemStack stack(*shovel, 1);
    auto* dirt = VanillaBlocks::DIRT;
    ASSERT_NE(dirt, nullptr);

    const BlockState& state = dirt->defaultState();

    f32 speed = shovel->getDestroySpeed(stack, state);
    // Diamond shovel should have 8.0 efficiency on dirt
    EXPECT_FLOAT_EQ(speed, 8.0f);
}

TEST_F(ToolHarvestTest, AxeSpeedOnOakLog) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* log = VanillaBlocks::OAK_LOG;
    ASSERT_NE(log, nullptr);

    const BlockState& state = log->defaultState();

    f32 speed = axe->getDestroySpeed(stack, state);
    // Diamond axe should have 8.0 efficiency on wood
    EXPECT_FLOAT_EQ(speed, 8.0f);
}

// ============================================================================
// BlockState Harvest Tests
// ============================================================================

TEST_F(ToolHarvestTest, BlockStateHarvestProperties) {
    // Check that stone block has correct harvest properties
    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    const BlockState& state = stone->defaultState();

    // Stone should require pickaxe
    EXPECT_EQ(state.getHarvestTool(), TOOL_TYPE_PICKAXE);
    // Stone should require harvest level 0 (can be mined with wood)
    EXPECT_EQ(state.getHarvestLevel(), 0);
}

TEST_F(ToolHarvestTest, BlockStateRequiresTool) {
    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    const BlockState& state = stone->defaultState();

    // Stone requires pickaxe to get cobblestone drop
    EXPECT_TRUE(state.requiresTool());
}

TEST_F(ToolHarvestTest, ToolEffectiveCheck) {
    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    const BlockState& state = stone->defaultState();

    // Wooden pickaxe should be effective (level 0 >= level 0)
    EXPECT_TRUE(state.isToolEffective(TOOL_TYPE_PICKAXE, 0));

    // Stone pickaxe should be effective (level 1 >= level 0)
    EXPECT_TRUE(state.isToolEffective(TOOL_TYPE_PICKAXE, 1));

    // Shovel should not be effective
    EXPECT_FALSE(state.isToolEffective(TOOL_TYPE_SHOVEL, 3));
}
