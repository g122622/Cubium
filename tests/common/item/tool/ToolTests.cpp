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

// ============================================================================
// Tool Special Function Tests (MC 1.16.5)
// ============================================================================

class ToolSpecialFunctionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        VanillaBlocks::initialize();
        Items::initialize();

        // 注意：使用"construct on first use"模式，静态映射表在第一次调用静态方法时自动初始化
        // 无需在此创建工具实例
    }
};

// ========== AxeItem Stripping Tests ==========

TEST_F(ToolSpecialFunctionTest, AxeGetStrippedBlockOakLog) {
    auto* oakLog = VanillaBlocks::OAK_LOG;
    auto* strippedOakLog = VanillaBlocks::STRIPPED_OAK_LOG;

    ASSERT_NE(oakLog, nullptr) << "OAK_LOG should be registered";
    ASSERT_NE(strippedOakLog, nullptr) << "STRIPPED_OAK_LOG should be registered";

    // Check that axe can strip oak log
    const Block* result = AxeItem::getStrippedBlock(oakLog);
    EXPECT_EQ(result, strippedOakLog) << "Axe should strip OAK_LOG to STRIPPED_OAK_LOG";
}

TEST_F(ToolSpecialFunctionTest, AxeGetStrippedBlockOakWood) {
    auto* oakWood = VanillaBlocks::OAK_WOOD;
    auto* strippedOakWood = VanillaBlocks::STRIPPED_OAK_WOOD;

    ASSERT_NE(oakWood, nullptr) << "OAK_WOOD should be registered";
    ASSERT_NE(strippedOakWood, nullptr) << "STRIPPED_OAK_WOOD should be registered";

    const Block* result = AxeItem::getStrippedBlock(oakWood);
    EXPECT_EQ(result, strippedOakWood) << "Axe should strip OAK_WOOD to STRIPPED_OAK_WOOD";
}

TEST_F(ToolSpecialFunctionTest, AxeGetStrippedBlockAllWoodTypes) {
    // Test all 6 wood types
    struct WoodPair {
        const Block* log;
        const Block* strippedLog;
        const char* name;
    };

    WoodPair woodTypes[] = {
        {VanillaBlocks::SPRUCE_LOG, VanillaBlocks::STRIPPED_SPRUCE_LOG, "SPRUCE"},
        {VanillaBlocks::BIRCH_LOG, VanillaBlocks::STRIPPED_BIRCH_LOG, "BIRCH"},
        {VanillaBlocks::JUNGLE_LOG, VanillaBlocks::STRIPPED_JUNGLE_LOG, "JUNGLE"},
        {VanillaBlocks::ACACIA_LOG, VanillaBlocks::STRIPPED_ACACIA_LOG, "ACACIA"},
        {VanillaBlocks::DARK_OAK_LOG, VanillaBlocks::STRIPPED_DARK_OAK_LOG, "DARK_OAK"},
    };

    for (const auto& pair : woodTypes) {
        if (pair.log == nullptr || pair.strippedLog == nullptr) {
            continue;  // Skip if not registered
        }
        const Block* result = AxeItem::getStrippedBlock(pair.log);
        EXPECT_EQ(result, pair.strippedLog)
            << "Axe should strip " << pair.name << "_LOG to STRIPPED_" << pair.name << "_LOG";
    }
}

TEST_F(ToolSpecialFunctionTest, AxeCannotStripStone) {
    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    const Block* result = AxeItem::getStrippedBlock(stone);
    EXPECT_EQ(result, nullptr) << "Axe should not be able to strip stone";
}

TEST_F(ToolSpecialFunctionTest, AxeCannotStripDirt) {
    auto* dirt = VanillaBlocks::DIRT;
    ASSERT_NE(dirt, nullptr);

    const Block* result = AxeItem::getStrippedBlock(dirt);
    EXPECT_EQ(result, nullptr) << "Axe should not be able to strip dirt";
}

TEST_F(ToolSpecialFunctionTest, AxeGetStrippedBlockNullInput) {
    const Block* result = AxeItem::getStrippedBlock(nullptr);
    EXPECT_EQ(result, nullptr) << "getStrippedBlock should return nullptr for null input";
}

// ========== ShovelItem Path Creation Tests ==========

TEST_F(ToolSpecialFunctionTest, ShovelGetPathBlockGrassBlock) {
    auto* grassBlock = VanillaBlocks::GRASS_BLOCK;
    auto* grassPath = VanillaBlocks::GRASS_PATH;

    ASSERT_NE(grassBlock, nullptr) << "GRASS_BLOCK should be registered";
    ASSERT_NE(grassPath, nullptr) << "GRASS_PATH should be registered";

    const Block* result = ShovelItem::getPathBlock(grassBlock);
    EXPECT_EQ(result, grassPath) << "Shovel should convert GRASS_BLOCK to GRASS_PATH";
}

TEST_F(ToolSpecialFunctionTest, ShovelCannotPathDirt) {
    auto* dirt = VanillaBlocks::DIRT;
    ASSERT_NE(dirt, nullptr);

    const Block* result = ShovelItem::getPathBlock(dirt);
    EXPECT_EQ(result, nullptr) << "Shovel should not be able to create path from dirt";
}

TEST_F(ToolSpecialFunctionTest, ShovelCannotPathStone) {
    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    const Block* result = ShovelItem::getPathBlock(stone);
    EXPECT_EQ(result, nullptr) << "Shovel should not be able to create path from stone";
}

TEST_F(ToolSpecialFunctionTest, ShovelCannotPathGrassPath) {
    auto* grassPath = VanillaBlocks::GRASS_PATH;
    if (grassPath == nullptr) {
        GTEST_SKIP() << "GRASS_PATH not registered yet";
    }

    const Block* result = ShovelItem::getPathBlock(grassPath);
    EXPECT_EQ(result, nullptr) << "Shovel should not be able to convert GRASS_PATH to anything";
}

TEST_F(ToolSpecialFunctionTest, ShovelGetPathBlockNullInput) {
    const Block* result = ShovelItem::getPathBlock(nullptr);
    EXPECT_EQ(result, nullptr) << "getPathBlock should return nullptr for null input";
}

// ========== HoeItem Tilling Tests ==========

TEST_F(ToolSpecialFunctionTest, HoeGetTilledBlockGrassBlock) {
    auto* grassBlock = VanillaBlocks::GRASS_BLOCK;
    auto* farmland = VanillaBlocks::FARMLAND;

    ASSERT_NE(grassBlock, nullptr) << "GRASS_BLOCK should be registered";
    ASSERT_NE(farmland, nullptr) << "FARMLAND should be registered";

    const Block* result = HoeItem::getTilledBlock(grassBlock);
    EXPECT_EQ(result, farmland) << "Hoe should convert GRASS_BLOCK to FARMLAND";
}

TEST_F(ToolSpecialFunctionTest, HoeGetTilledBlockDirt) {
    auto* dirt = VanillaBlocks::DIRT;
    auto* farmland = VanillaBlocks::FARMLAND;

    ASSERT_NE(dirt, nullptr) << "DIRT should be registered";
    ASSERT_NE(farmland, nullptr) << "FARMLAND should be registered";

    const Block* result = HoeItem::getTilledBlock(dirt);
    EXPECT_EQ(result, farmland) << "Hoe should convert DIRT to FARMLAND";
}

TEST_F(ToolSpecialFunctionTest, HoeGetTilledBlockGrassPath) {
    auto* grassPath = VanillaBlocks::GRASS_PATH;
    auto* farmland = VanillaBlocks::FARMLAND;

    if (grassPath == nullptr || farmland == nullptr) {
        GTEST_SKIP() << "GRASS_PATH or FARMLAND not registered yet";
    }

    const Block* result = HoeItem::getTilledBlock(grassPath);
    EXPECT_EQ(result, farmland) << "Hoe should convert GRASS_PATH to FARMLAND";
}

TEST_F(ToolSpecialFunctionTest, HoeGetTilledBlockCoarseDirtToDirt) {
    auto* coarseDirt = VanillaBlocks::COARSE_DIRT;
    auto* dirt = VanillaBlocks::DIRT;

    if (coarseDirt == nullptr || dirt == nullptr) {
        GTEST_SKIP() << "COARSE_DIRT or DIRT not registered yet";
    }

    // MC 1.16.5: Coarse dirt -> Dirt (not farmland!)
    const Block* result = HoeItem::getTilledBlock(coarseDirt);
    EXPECT_EQ(result, dirt) << "Hoe should convert COARSE_DIRT to DIRT (not FARMLAND)";
}

TEST_F(ToolSpecialFunctionTest, HoeCannotTillStone) {
    auto* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    const Block* result = HoeItem::getTilledBlock(stone);
    EXPECT_EQ(result, nullptr) << "Hoe should not be able to till stone";
}

TEST_F(ToolSpecialFunctionTest, HoeCannotTillFarmland) {
    auto* farmland = VanillaBlocks::FARMLAND;
    if (farmland == nullptr) {
        GTEST_SKIP() << "FARMLAND not registered yet";
    }

    const Block* result = HoeItem::getTilledBlock(farmland);
    EXPECT_EQ(result, nullptr) << "Hoe should not be able to convert FARMLAND to anything";
}

TEST_F(ToolSpecialFunctionTest, HoeGetTilledBlockNullInput) {
    const Block* result = HoeItem::getTilledBlock(nullptr);
    EXPECT_EQ(result, nullptr) << "getTilledBlock should return nullptr for null input";
}

// ========== Tool Enchantability Tests ==========

TEST_F(ToolItemTest, AxeEnchantability) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);
    EXPECT_EQ(axe->getItemEnchantability(), 10);  // Diamond enchantability
}

TEST_F(ToolItemTest, ShovelEnchantability) {
    auto* shovel = Items::DIAMOND_SHOVEL;
    ASSERT_NE(shovel, nullptr);
    EXPECT_EQ(shovel->getItemEnchantability(), 10);  // Diamond enchantability
}

TEST_F(ToolItemTest, HoeEnchantability) {
    auto* hoe = Items::DIAMOND_HOE;
    ASSERT_NE(hoe, nullptr);
    EXPECT_EQ(hoe->getItemEnchantability(), 10);  // Diamond enchantability
}

// ============================================================================
// Axe Effective Blocks Tests - Wood Buttons
// ============================================================================

TEST_F(ToolHarvestTest, AxeEffectiveOnOakButton) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* button = VanillaBlocks::OAK_BUTTON;
    ASSERT_NE(button, nullptr);

    const BlockState& state = button->defaultState();
    f32 speed = axe->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 8.0f) << "Diamond axe should be effective on OAK_BUTTON";
}

TEST_F(ToolHarvestTest, AxeEffectiveOnSpruceButton) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* button = VanillaBlocks::SPRUCE_BUTTON;
    if (button == nullptr) {
        GTEST_SKIP() << "SPRUCE_BUTTON not registered yet";
    }

    const BlockState& state = button->defaultState();
    f32 speed = axe->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 8.0f) << "Diamond axe should be effective on SPRUCE_BUTTON";
}

TEST_F(ToolHarvestTest, AxeEffectiveOnBirchButton) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* button = VanillaBlocks::BIRCH_BUTTON;
    if (button == nullptr) {
        GTEST_SKIP() << "BIRCH_BUTTON not registered yet";
    }

    const BlockState& state = button->defaultState();
    f32 speed = axe->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 8.0f) << "Diamond axe should be effective on BIRCH_BUTTON";
}

TEST_F(ToolHarvestTest, AxeEffectiveOnJungleButton) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* button = VanillaBlocks::JUNGLE_BUTTON;
    if (button == nullptr) {
        GTEST_SKIP() << "JUNGLE_BUTTON not registered yet";
    }

    const BlockState& state = button->defaultState();
    f32 speed = axe->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 8.0f) << "Diamond axe should be effective on JUNGLE_BUTTON";
}

TEST_F(ToolHarvestTest, AxeEffectiveOnAcaciaButton) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* button = VanillaBlocks::ACACIA_BUTTON;
    if (button == nullptr) {
        GTEST_SKIP() << "ACACIA_BUTTON not registered yet";
    }

    const BlockState& state = button->defaultState();
    f32 speed = axe->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 8.0f) << "Diamond axe should be effective on ACACIA_BUTTON";
}

TEST_F(ToolHarvestTest, AxeEffectiveOnDarkOakButton) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* button = VanillaBlocks::DARK_OAK_BUTTON;
    if (button == nullptr) {
        GTEST_SKIP() << "DARK_OAK_BUTTON not registered yet";
    }

    const BlockState& state = button->defaultState();
    f32 speed = axe->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 8.0f) << "Diamond axe should be effective on DARK_OAK_BUTTON";
}

TEST_F(ToolHarvestTest, AxeEffectiveOnCrimsonButton) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* button = VanillaBlocks::CRIMSON_BUTTON;
    if (button == nullptr) {
        GTEST_SKIP() << "CRIMSON_BUTTON not registered yet";
    }

    const BlockState& state = button->defaultState();
    f32 speed = axe->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 8.0f) << "Diamond axe should be effective on CRIMSON_BUTTON";
}

TEST_F(ToolHarvestTest, AxeEffectiveOnWarpedButton) {
    auto* axe = Items::DIAMOND_AXE;
    ASSERT_NE(axe, nullptr);

    ItemStack stack(*axe, 1);
    auto* button = VanillaBlocks::WARPED_BUTTON;
    if (button == nullptr) {
        GTEST_SKIP() << "WARPED_BUTTON not registered yet";
    }

    const BlockState& state = button->defaultState();
    f32 speed = axe->getDestroySpeed(stack, state);
    EXPECT_FLOAT_EQ(speed, 8.0f) << "Diamond axe should be effective on WARPED_BUTTON";
}
