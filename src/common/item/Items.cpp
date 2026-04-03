#include "Items.hpp"

#include "items/block/BlockItem.hpp"
#include "items/potion/PotionItem.hpp"
#include "items/potion/SplashPotionItem.hpp"
#include "items/potion/LingeringPotionItem.hpp"
#include "tier/ItemTiers.hpp"
#include "items/tool/PickaxeItem.hpp"
#include "items/tool/AxeItem.hpp"
#include "items/tool/ShovelItem.hpp"
#include "items/tool/HoeItem.hpp"
#include "items/tool/SwordItem.hpp"
#include "../world/block/VanillaBlocks.hpp"

namespace {

mc::Item& registerBlockBackedItem(mc::ItemRegistry& registry,
                                  mc::Block* block,
                                  const char* path,
                                  mc::ItemProperties properties)
{
    const mc::ResourceLocation id("minecraft", path);
    if (block != nullptr) {
        return registry.registerItem<mc::BlockItem>(id, *block, std::move(properties));
    }

    return registry.registerItem(id, std::move(properties));
}

} // namespace

namespace mc {

bool Items::s_initialized = false;

// ============================================================================
// 静态成员初始化
// ============================================================================

Item* Items::AIR = nullptr;

// 木头和木板
Item* Items::OAK_LOG = nullptr;
Item* Items::SPRUCE_LOG = nullptr;
Item* Items::BIRCH_LOG = nullptr;
Item* Items::JUNGLE_LOG = nullptr;
Item* Items::ACACIA_LOG = nullptr;
Item* Items::DARK_OAK_LOG = nullptr;

Item* Items::OAK_PLANKS = nullptr;
Item* Items::SPRUCE_PLANKS = nullptr;
Item* Items::BIRCH_PLANKS = nullptr;
Item* Items::JUNGLE_PLANKS = nullptr;
Item* Items::ACACIA_PLANKS = nullptr;
Item* Items::DARK_OAK_PLANKS = nullptr;

// 石头
Item* Items::STONE = nullptr;
Item* Items::COBBLESTONE = nullptr;
Item* Items::MOSSY_COBBLESTONE = nullptr;

// 矿物和材料
Item* Items::DIAMOND = nullptr;
Item* Items::EMERALD = nullptr;
Item* Items::GOLD_INGOT = nullptr;
Item* Items::IRON_INGOT = nullptr;
Item* Items::COPPER_INGOT = nullptr;
Item* Items::NETHERITE_INGOT = nullptr;
Item* Items::NETHERITE_SCRAP = nullptr;

// 宝石碎片
Item* Items::DIAMOND_SHARD = nullptr;
Item* Items::EMERALD_SHARD = nullptr;

// 煤炭相关
Item* Items::COAL = nullptr;
Item* Items::CHARCOAL = nullptr;

// 红石相关
Item* Items::REDSTONE = nullptr;
Item* Items::LAPIS_LAZULI = nullptr;
Item* Items::QUARTZ = nullptr;
Item* Items::GLOWSTONE_DUST = nullptr;

// 矿物原矿（物品形式，通常是方块）
Item* Items::COAL_ORE = nullptr;
Item* Items::IRON_ORE = nullptr;
Item* Items::GOLD_ORE = nullptr;
Item* Items::DIAMOND_ORE = nullptr;
Item* Items::EMERALD_ORE = nullptr;
Item* Items::LAPIS_ORE = nullptr;
Item* Items::REDSTONE_ORE = nullptr;
Item* Items::COPPER_ORE = nullptr;
Item* Items::NETHER_QUARTZ_ORE = nullptr;
Item* Items::NETHER_GOLD_ORE = nullptr;
Item* Items::ANCIENT_DEBRIS = nullptr;

// 钻石工具
Item* Items::DIAMOND_PICKAXE = nullptr;
Item* Items::DIAMOND_AXE = nullptr;
Item* Items::DIAMOND_SHOVEL = nullptr;
Item* Items::DIAMOND_HOE = nullptr;
Item* Items::DIAMOND_SWORD = nullptr;

// 铁工具
Item* Items::IRON_PICKAXE = nullptr;
Item* Items::IRON_AXE = nullptr;
Item* Items::IRON_SHOVEL = nullptr;
Item* Items::IRON_HOE = nullptr;
Item* Items::IRON_SWORD = nullptr;

// 石工具
Item* Items::STONE_PICKAXE = nullptr;
Item* Items::STONE_AXE = nullptr;
Item* Items::STONE_SHOVEL = nullptr;
Item* Items::STONE_HOE = nullptr;
Item* Items::STONE_SWORD = nullptr;

// 木工具
Item* Items::WOODEN_PICKAXE = nullptr;
Item* Items::WOODEN_AXE = nullptr;
Item* Items::WOODEN_SHOVEL = nullptr;
Item* Items::WOODEN_HOE = nullptr;
Item* Items::WOODEN_SWORD = nullptr;

// 金工具
Item* Items::GOLDEN_PICKAXE = nullptr;
Item* Items::GOLDEN_AXE = nullptr;
Item* Items::GOLDEN_SHOVEL = nullptr;
Item* Items::GOLDEN_HOE = nullptr;
Item* Items::GOLDEN_SWORD = nullptr;

// 钻石护甲
Item* Items::DIAMOND_HELMET = nullptr;
Item* Items::DIAMOND_CHESTPLATE = nullptr;
Item* Items::DIAMOND_LEGGINGS = nullptr;
Item* Items::DIAMOND_BOOTS = nullptr;

// 铁护甲
Item* Items::IRON_HELMET = nullptr;
Item* Items::IRON_CHESTPLATE = nullptr;
Item* Items::IRON_LEGGINGS = nullptr;
Item* Items::IRON_BOOTS = nullptr;

// 金护甲
Item* Items::GOLDEN_HELMET = nullptr;
Item* Items::GOLDEN_CHESTPLATE = nullptr;
Item* Items::GOLDEN_LEGGINGS = nullptr;
Item* Items::GOLDEN_BOOTS = nullptr;

// 皮革护甲
Item* Items::LEATHER_HELMET = nullptr;
Item* Items::LEATHER_CHESTPLATE = nullptr;
Item* Items::LEATHER_LEGGINGS = nullptr;
Item* Items::LEATHER_BOOTS = nullptr;

// 食物
Item* Items::APPLE = nullptr;
Item* Items::GOLDEN_APPLE = nullptr;
Item* Items::ENCHANTED_GOLDEN_APPLE = nullptr;
Item* Items::BREAD = nullptr;
Item* Items::COOKED_BEEF = nullptr;
Item* Items::COOKED_PORKCHOP = nullptr;
Item* Items::COOKED_CHICKEN = nullptr;
Item* Items::COOKED_MUTTON = nullptr;
Item* Items::COOKED_RABBIT = nullptr;
Item* Items::COOKED_COD = nullptr;
Item* Items::COOKED_SALMON = nullptr;
Item* Items::BEEF = nullptr;
Item* Items::PORKCHOP = nullptr;
Item* Items::CHICKEN = nullptr;
Item* Items::MUTTON = nullptr;
Item* Items::RABBIT = nullptr;
Item* Items::COD = nullptr;
Item* Items::SALMON = nullptr;

// 木棍和骨头
Item* Items::STICK = nullptr;
Item* Items::BONE = nullptr;
Item* Items::BONE_MEAL = nullptr;

// 杂项
Item* Items::FLINT = nullptr;
Item* Items::FLINT_AND_STEEL = nullptr;
Item* Items::STRING = nullptr;
Item* Items::FEATHER = nullptr;
Item* Items::GUNPOWDER = nullptr;
Item* Items::LEATHER = nullptr;
Item* Items::SLIME_BALL = nullptr;
Item* Items::EGG = nullptr;
Item* Items::COMPASS = nullptr;
Item* Items::CLOCK = nullptr;
Item* Items::SPIDER_EYE = nullptr;
Item* Items::FERMENTED_SPIDER_EYE = nullptr;
Item* Items::BLAZE_ROD = nullptr;
Item* Items::BLAZE_POWDER = nullptr;
Item* Items::ENDER_PEARL = nullptr;
Item* Items::ENDER_EYE = nullptr;
Item* Items::NETHER_STAR = nullptr;
Item* Items::FIRE_CHARGE = nullptr;
Item* Items::FIREWORK_STAR = nullptr;
Item* Items::FIREWORK_ROCKET = nullptr;

// 染料
Item* Items::INK_SAC = nullptr;
Item* Items::RED_DYE = nullptr;
Item* Items::GREEN_DYE = nullptr;
Item* Items::COCOA_BEANS = nullptr;
Item* Items::LAPIS_LAZULI_DYE = nullptr;
Item* Items::PURPLE_DYE = nullptr;
Item* Items::CYAN_DYE = nullptr;
Item* Items::LIGHT_GRAY_DYE = nullptr;
Item* Items::GRAY_DYE = nullptr;
Item* Items::PINK_DYE = nullptr;
Item* Items::LIME_DYE = nullptr;
Item* Items::YELLOW_DYE = nullptr;
Item* Items::LIGHT_BLUE_DYE = nullptr;
Item* Items::MAGENTA_DYE = nullptr;
Item* Items::ORANGE_DYE = nullptr;
Item* Items::WHITE_DYE = nullptr;

// 种子
Item* Items::WHEAT_SEEDS = nullptr;
Item* Items::PUMPKIN_SEEDS = nullptr;
Item* Items::MELON_SEEDS = nullptr;
Item* Items::BEETROOT_SEEDS = nullptr;

// 农产品
Item* Items::WHEAT = nullptr;
Item* Items::PUMPKIN = nullptr;
Item* Items::MELON = nullptr;
Item* Items::MELON_SLICE = nullptr;
Item* Items::CARROT = nullptr;
Item* Items::POTATO = nullptr;
Item* Items::BEETROOT = nullptr;
Item* Items::SUGAR_CANE = nullptr;
Item* Items::SUGAR = nullptr;

// 水域更新材料
Item* Items::SCUTE = nullptr;
Item* Items::HEART_OF_THE_SEA = nullptr;
Item* Items::NAUTILUS_SHELL = nullptr;
Item* Items::PHANTOM_MEMBRANE = nullptr;
Item* Items::DRIED_KELP = nullptr;

// ============================================================================
// 酿造材料
// ============================================================================
Item* Items::NETHER_WART = nullptr;
Item* Items::GOLDEN_CARROT = nullptr;
Item* Items::GHAST_TEAR = nullptr;
Item* Items::RABBIT_FOOT = nullptr;
Item* Items::MAGMA_CREAM = nullptr;
Item* Items::DRAGON_BREATH = nullptr;
Item* Items::PUFFERFISH = nullptr;
Item* Items::TURTLE_HELMET = nullptr;
Item* Items::GLISTERING_MELON_SLICE = nullptr;

// ============================================================================
// 药水相关
// ============================================================================
Item* Items::GLASS_BOTTLE = nullptr;
Item* Items::POTION = nullptr;
Item* Items::SPLASH_POTION = nullptr;
Item* Items::LINGERING_POTION = nullptr;

// ============================================================================
// 初始化
// ============================================================================

void Items::initialize() {
    if (s_initialized) {
        return;
    }

    auto& registry = ItemRegistry::instance();

    // 空气物品（ID 0）
    AIR = &registry.registerItem(
        ResourceLocation("minecraft:air"),
        ItemProperties().maxStackSize(64)
    );

    registerMaterials();
    registerMisc();  // 提前注册，因为工具层级需要木板等作为修复材料

    // 初始化工具层级（需要在材料物品注册后）
    item::tier::ItemTiers::initialize();

    registerTools();
    registerArmor();
    registerFood();
    registerDyes();
    registerSeeds();
    registerCrops();
    registerAquaticMaterials();
    registerBrewingIngredients();
    registerPotions();

    s_initialized = true;
}

void Items::registerMaterials() {
    auto& registry = ItemRegistry::instance();

    // 宝石
    DIAMOND = &registry.registerItem(
        ResourceLocation("minecraft:diamond"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare)
    );

    EMERALD = &registry.registerItem(
        ResourceLocation("minecraft:emerald"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare)
    );

    // 锭
    GOLD_INGOT = &registry.registerItem(
        ResourceLocation("minecraft:gold_ingot"),
        ItemProperties().maxStackSize(64)
    );

    IRON_INGOT = &registry.registerItem(
        ResourceLocation("minecraft:iron_ingot"),
        ItemProperties().maxStackSize(64)
    );

    COPPER_INGOT = &registry.registerItem(
        ResourceLocation("minecraft:copper_ingot"),
        ItemProperties().maxStackSize(64)
    );

    NETHERITE_INGOT = &registry.registerItem(
        ResourceLocation("minecraft:netherite_ingot"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare)
    );

    NETHERITE_SCRAP = &registry.registerItem(
        ResourceLocation("minecraft:netherite_scrap"),
        ItemProperties().maxStackSize(64)
    );

    // 煤炭
    COAL = &registry.registerItem(
        ResourceLocation("minecraft:coal"),
        ItemProperties().maxStackSize(64)
    );

    CHARCOAL = &registry.registerItem(
        ResourceLocation("minecraft:charcoal"),
        ItemProperties().maxStackSize(64)
    );

    // 红石相关
    REDSTONE = &registry.registerItem(
        ResourceLocation("minecraft:redstone"),
        ItemProperties().maxStackSize(64)
    );

    LAPIS_LAZULI = &registry.registerItem(
        ResourceLocation("minecraft:lapis_lazuli"),
        ItemProperties().maxStackSize(64)
    );

    QUARTZ = &registry.registerItem(
        ResourceLocation("minecraft:quartz"),
        ItemProperties().maxStackSize(64)
    );

    GLOWSTONE_DUST = &registry.registerItem(
        ResourceLocation("minecraft:glowstone_dust"),
        ItemProperties().maxStackSize(64)
    );

    COAL_ORE = &registerBlockBackedItem(
        registry,
        VanillaBlocks::COAL_ORE,
        "coal_ore",
        ItemProperties().maxStackSize(64)
    );
    IRON_ORE = &registerBlockBackedItem(
        registry,
        VanillaBlocks::IRON_ORE,
        "iron_ore",
        ItemProperties().maxStackSize(64)
    );
    GOLD_ORE = &registerBlockBackedItem(
        registry,
        VanillaBlocks::GOLD_ORE,
        "gold_ore",
        ItemProperties().maxStackSize(64)
    );
    DIAMOND_ORE = &registerBlockBackedItem(
        registry,
        VanillaBlocks::DIAMOND_ORE,
        "diamond_ore",
        ItemProperties().maxStackSize(64)
    );
    EMERALD_ORE = &registerBlockBackedItem(
        registry,
        VanillaBlocks::EMERALD_ORE,
        "emerald_ore",
        ItemProperties().maxStackSize(64)
    );
    LAPIS_ORE = &registerBlockBackedItem(
        registry,
        VanillaBlocks::LAPIS_ORE,
        "lapis_ore",
        ItemProperties().maxStackSize(64)
    );
    REDSTONE_ORE = &registerBlockBackedItem(
        registry,
        VanillaBlocks::REDSTONE_ORE,
        "redstone_ore",
        ItemProperties().maxStackSize(64)
    );
}

void Items::registerTools() {
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 钻石工具
    // ========================================================================
    DIAMOND_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(
        ResourceLocation("minecraft:diamond_pickaxe"),
        item::tier::ItemTiers::DIAMOND(),  // tier
        1,      // attackDamage
        -2.8f,  // attackSpeed
        ItemProperties().rarity(ItemRarity::Common)
    );

    DIAMOND_AXE = &registry.registerItem<item::tool::AxeItem>(
        ResourceLocation("minecraft:diamond_axe"),
        item::tier::ItemTiers::DIAMOND(),  // tier
        5.0f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties().rarity(ItemRarity::Common)
    );

    DIAMOND_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(
        ResourceLocation("minecraft:diamond_shovel"),
        item::tier::ItemTiers::DIAMOND(),  // tier
        1.5f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties().rarity(ItemRarity::Common)
    );

    DIAMOND_HOE = &registry.registerItem<item::tool::HoeItem>(
        ResourceLocation("minecraft:diamond_hoe"),
        item::tier::ItemTiers::DIAMOND(),  // tier
        0,      // attackDamage
        -2.0f,  // attackSpeed
        ItemProperties().rarity(ItemRarity::Common)
    );

    DIAMOND_SWORD = &registry.registerItem<item::tool::SwordItem>(
        ResourceLocation("minecraft:diamond_sword"),
        item::tier::ItemTiers::DIAMOND(),  // tier
        3,      // attackDamage
        -2.4f,  // attackSpeed
        ItemProperties().rarity(ItemRarity::Common)
    );

    // ========================================================================
    // 铁工具
    // ========================================================================
    IRON_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(
        ResourceLocation("minecraft:iron_pickaxe"),
        item::tier::ItemTiers::IRON(),  // tier
        1,      // attackDamage
        -2.8f,  // attackSpeed
        ItemProperties()
    );

    IRON_AXE = &registry.registerItem<item::tool::AxeItem>(
        ResourceLocation("minecraft:iron_axe"),
        item::tier::ItemTiers::IRON(),  // tier
        4.0f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties()
    );

    IRON_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(
        ResourceLocation("minecraft:iron_shovel"),
        item::tier::ItemTiers::IRON(),  // tier
        1.5f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties()
    );

    IRON_HOE = &registry.registerItem<item::tool::HoeItem>(
        ResourceLocation("minecraft:iron_hoe"),
        item::tier::ItemTiers::IRON(),  // tier
        0,      // attackDamage
        -2.0f,  // attackSpeed
        ItemProperties()
    );

    IRON_SWORD = &registry.registerItem<item::tool::SwordItem>(
        ResourceLocation("minecraft:iron_sword"),
        item::tier::ItemTiers::IRON(),  // tier
        3,      // attackDamage
        -2.4f,  // attackSpeed
        ItemProperties()
    );

    // ========================================================================
    // 石工具
    // ========================================================================
    STONE_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(
        ResourceLocation("minecraft:stone_pickaxe"),
        item::tier::ItemTiers::STONE(),  // tier
        1,      // attackDamage
        -2.8f,  // attackSpeed
        ItemProperties()
    );

    STONE_AXE = &registry.registerItem<item::tool::AxeItem>(
        ResourceLocation("minecraft:stone_axe"),
        item::tier::ItemTiers::STONE(),  // tier
        4.0f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties()
    );

    STONE_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(
        ResourceLocation("minecraft:stone_shovel"),
        item::tier::ItemTiers::STONE(),  // tier
        1.5f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties()
    );

    STONE_HOE = &registry.registerItem<item::tool::HoeItem>(
        ResourceLocation("minecraft:stone_hoe"),
        item::tier::ItemTiers::STONE(),  // tier
        0,      // attackDamage
        -2.0f,  // attackSpeed
        ItemProperties()
    );

    STONE_SWORD = &registry.registerItem<item::tool::SwordItem>(
        ResourceLocation("minecraft:stone_sword"),
        item::tier::ItemTiers::STONE(),  // tier
        3,      // attackDamage
        -2.4f,  // attackSpeed
        ItemProperties()
    );

    // ========================================================================
    // 木工具
    // ========================================================================
    WOODEN_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(
        ResourceLocation("minecraft:wooden_pickaxe"),
        item::tier::ItemTiers::WOOD(),  // tier
        1,      // attackDamage
        -2.8f,  // attackSpeed
        ItemProperties()
    );

    WOODEN_AXE = &registry.registerItem<item::tool::AxeItem>(
        ResourceLocation("minecraft:wooden_axe"),
        item::tier::ItemTiers::WOOD(),  // tier
        3.0f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties()
    );

    WOODEN_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(
        ResourceLocation("minecraft:wooden_shovel"),
        item::tier::ItemTiers::WOOD(),  // tier
        1.0f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties()
    );

    WOODEN_HOE = &registry.registerItem<item::tool::HoeItem>(
        ResourceLocation("minecraft:wooden_hoe"),
        item::tier::ItemTiers::WOOD(),  // tier
        0,      // attackDamage
        -2.0f,  // attackSpeed
        ItemProperties()
    );

    WOODEN_SWORD = &registry.registerItem<item::tool::SwordItem>(
        ResourceLocation("minecraft:wooden_sword"),
        item::tier::ItemTiers::WOOD(),  // tier
        3,      // attackDamage
        -2.4f,  // attackSpeed
        ItemProperties()
    );

    // ========================================================================
    // 金工具
    // ========================================================================
    GOLDEN_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(
        ResourceLocation("minecraft:golden_pickaxe"),
        item::tier::ItemTiers::GOLD(),  // tier
        1,      // attackDamage
        -2.8f,  // attackSpeed
        ItemProperties()
    );

    GOLDEN_AXE = &registry.registerItem<item::tool::AxeItem>(
        ResourceLocation("minecraft:golden_axe"),
        item::tier::ItemTiers::GOLD(),  // tier
        3.0f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties()
    );

    GOLDEN_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(
        ResourceLocation("minecraft:golden_shovel"),
        item::tier::ItemTiers::GOLD(),  // tier
        1.0f,   // attackDamage
        -3.0f,  // attackSpeed
        ItemProperties()
    );

    GOLDEN_HOE = &registry.registerItem<item::tool::HoeItem>(
        ResourceLocation("minecraft:golden_hoe"),
        item::tier::ItemTiers::GOLD(),  // tier
        0,      // attackDamage
        -2.0f,  // attackSpeed
        ItemProperties()
    );

    GOLDEN_SWORD = &registry.registerItem<item::tool::SwordItem>(
        ResourceLocation("minecraft:golden_sword"),
        item::tier::ItemTiers::GOLD(),  // tier
        3,      // attackDamage
        -2.4f,  // attackSpeed
        ItemProperties()
    );
}

void Items::registerArmor() {
    auto& registry = ItemRegistry::instance();

    // 钻石护甲
    DIAMOND_HELMET = &registry.registerItem(
        ResourceLocation("minecraft:diamond_helmet"),
        ItemProperties().maxDamage(363)
    );

    DIAMOND_CHESTPLATE = &registry.registerItem(
        ResourceLocation("minecraft:diamond_chestplate"),
        ItemProperties().maxDamage(528)
    );

    DIAMOND_LEGGINGS = &registry.registerItem(
        ResourceLocation("minecraft:diamond_leggings"),
        ItemProperties().maxDamage(495)
    );

    DIAMOND_BOOTS = &registry.registerItem(
        ResourceLocation("minecraft:diamond_boots"),
        ItemProperties().maxDamage(396)
    );

    // 铁护甲
    IRON_HELMET = &registry.registerItem(
        ResourceLocation("minecraft:iron_helmet"),
        ItemProperties().maxDamage(165)
    );

    IRON_CHESTPLATE = &registry.registerItem(
        ResourceLocation("minecraft:iron_chestplate"),
        ItemProperties().maxDamage(240)
    );

    IRON_LEGGINGS = &registry.registerItem(
        ResourceLocation("minecraft:iron_leggings"),
        ItemProperties().maxDamage(225)
    );

    IRON_BOOTS = &registry.registerItem(
        ResourceLocation("minecraft:iron_boots"),
        ItemProperties().maxDamage(195)
    );

    // 金护甲
    GOLDEN_HELMET = &registry.registerItem(
        ResourceLocation("minecraft:golden_helmet"),
        ItemProperties().maxDamage(77)
    );

    GOLDEN_CHESTPLATE = &registry.registerItem(
        ResourceLocation("minecraft:golden_chestplate"),
        ItemProperties().maxDamage(112)
    );

    GOLDEN_LEGGINGS = &registry.registerItem(
        ResourceLocation("minecraft:golden_leggings"),
        ItemProperties().maxDamage(105)
    );

    GOLDEN_BOOTS = &registry.registerItem(
        ResourceLocation("minecraft:golden_boots"),
        ItemProperties().maxDamage(91)
    );

    // 皮革护甲
    LEATHER_HELMET = &registry.registerItem(
        ResourceLocation("minecraft:leather_helmet"),
        ItemProperties().maxDamage(55)
    );

    LEATHER_CHESTPLATE = &registry.registerItem(
        ResourceLocation("minecraft:leather_chestplate"),
        ItemProperties().maxDamage(80)
    );

    LEATHER_LEGGINGS = &registry.registerItem(
        ResourceLocation("minecraft:leather_leggings"),
        ItemProperties().maxDamage(75)
    );

    LEATHER_BOOTS = &registry.registerItem(
        ResourceLocation("minecraft:leather_boots"),
        ItemProperties().maxDamage(65)
    );
}

void Items::registerFood() {
    auto& registry = ItemRegistry::instance();

    APPLE = &registry.registerItem(
        ResourceLocation("minecraft:apple"),
        ItemProperties().maxStackSize(64)
    );

    GOLDEN_APPLE = &registry.registerItem(
        ResourceLocation("minecraft:golden_apple"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare)
    );

    ENCHANTED_GOLDEN_APPLE = &registry.registerItem(
        ResourceLocation("minecraft:enchanted_golden_apple"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Epic)
    );

    BREAD = &registry.registerItem(
        ResourceLocation("minecraft:bread"),
        ItemProperties().maxStackSize(64)
    );

    // 熟食
    COOKED_BEEF = &registry.registerItem(
        ResourceLocation("minecraft:cooked_beef"),
        ItemProperties().maxStackSize(64)
    );

    COOKED_PORKCHOP = &registry.registerItem(
        ResourceLocation("minecraft:cooked_porkchop"),
        ItemProperties().maxStackSize(64)
    );

    COOKED_CHICKEN = &registry.registerItem(
        ResourceLocation("minecraft:cooked_chicken"),
        ItemProperties().maxStackSize(64)
    );

    COOKED_MUTTON = &registry.registerItem(
        ResourceLocation("minecraft:cooked_mutton"),
        ItemProperties().maxStackSize(64)
    );

    COOKED_RABBIT = &registry.registerItem(
        ResourceLocation("minecraft:cooked_rabbit"),
        ItemProperties().maxStackSize(64)
    );

    COOKED_COD = &registry.registerItem(
        ResourceLocation("minecraft:cooked_cod"),
        ItemProperties().maxStackSize(64)
    );

    COOKED_SALMON = &registry.registerItem(
        ResourceLocation("minecraft:cooked_salmon"),
        ItemProperties().maxStackSize(64)
    );

    // 生食
    BEEF = &registry.registerItem(
        ResourceLocation("minecraft:beef"),
        ItemProperties().maxStackSize(64)
    );

    PORKCHOP = &registry.registerItem(
        ResourceLocation("minecraft:porkchop"),
        ItemProperties().maxStackSize(64)
    );

    CHICKEN = &registry.registerItem(
        ResourceLocation("minecraft:chicken"),
        ItemProperties().maxStackSize(64)
    );

    MUTTON = &registry.registerItem(
        ResourceLocation("minecraft:mutton"),
        ItemProperties().maxStackSize(64)
    );

    RABBIT = &registry.registerItem(
        ResourceLocation("minecraft:rabbit"),
        ItemProperties().maxStackSize(64)
    );

    COD = &registry.registerItem(
        ResourceLocation("minecraft:cod"),
        ItemProperties().maxStackSize(64)
    );

    SALMON = &registry.registerItem(
        ResourceLocation("minecraft:salmon"),
        ItemProperties().maxStackSize(64)
    );
}

void Items::registerMisc() {
    auto& registry = ItemRegistry::instance();

    // 木头和木板
    OAK_LOG = &registerBlockBackedItem(registry, VanillaBlocks::OAK_LOG, "oak_log", ItemProperties().maxStackSize(64));
    SPRUCE_LOG = &registerBlockBackedItem(registry, VanillaBlocks::SPRUCE_LOG, "spruce_log", ItemProperties().maxStackSize(64));
    BIRCH_LOG = &registerBlockBackedItem(registry, VanillaBlocks::BIRCH_LOG, "birch_log", ItemProperties().maxStackSize(64));
    JUNGLE_LOG = &registerBlockBackedItem(registry, VanillaBlocks::JUNGLE_LOG, "jungle_log", ItemProperties().maxStackSize(64));
    ACACIA_LOG = &registerBlockBackedItem(registry, VanillaBlocks::ACACIA_LOG, "acacia_log", ItemProperties().maxStackSize(64));
    DARK_OAK_LOG = &registerBlockBackedItem(registry, VanillaBlocks::DARK_OAK_LOG, "dark_oak_log", ItemProperties().maxStackSize(64));

    OAK_PLANKS = &registerBlockBackedItem(registry, VanillaBlocks::OAK_PLANKS, "oak_planks", ItemProperties().maxStackSize(64));
    SPRUCE_PLANKS = &registerBlockBackedItem(registry, VanillaBlocks::SPRUCE_PLANKS, "spruce_planks", ItemProperties().maxStackSize(64));
    BIRCH_PLANKS = &registerBlockBackedItem(registry, VanillaBlocks::BIRCH_PLANKS, "birch_planks", ItemProperties().maxStackSize(64));
    JUNGLE_PLANKS = &registerBlockBackedItem(registry, VanillaBlocks::JUNGLE_PLANKS, "jungle_planks", ItemProperties().maxStackSize(64));
    ACACIA_PLANKS = &registerBlockBackedItem(registry, VanillaBlocks::ACACIA_PLANKS, "acacia_planks", ItemProperties().maxStackSize(64));
    DARK_OAK_PLANKS = &registerBlockBackedItem(registry, VanillaBlocks::DARK_OAK_PLANKS, "dark_oak_planks", ItemProperties().maxStackSize(64));

    // 石头
    STONE = &registerBlockBackedItem(registry, VanillaBlocks::STONE, "stone", ItemProperties().maxStackSize(64));
    COBBLESTONE = &registerBlockBackedItem(registry, VanillaBlocks::COBBLESTONE, "cobblestone", ItemProperties().maxStackSize(64));
    MOSSY_COBBLESTONE = &registerBlockBackedItem(registry, VanillaBlocks::MOSSY_COBBLESTONE, "mossy_cobblestone", ItemProperties().maxStackSize(64));

    // 木棍和骨头
    STICK = &registry.registerItem(
        ResourceLocation("minecraft:stick"),
        ItemProperties().maxStackSize(64)
    );

    BONE = &registry.registerItem(
        ResourceLocation("minecraft:bone"),
        ItemProperties().maxStackSize(64)
    );

    BONE_MEAL = &registry.registerItem(
        ResourceLocation("minecraft:bone_meal"),
        ItemProperties().maxStackSize(64)
    );

    FLINT = &registry.registerItem(
        ResourceLocation("minecraft:flint"),
        ItemProperties().maxStackSize(64)
    );

    FLINT_AND_STEEL = &registry.registerItem(
        ResourceLocation("minecraft:flint_and_steel"),
        ItemProperties().maxDamage(64)
    );

    STRING = &registry.registerItem(
        ResourceLocation("minecraft:string"),
        ItemProperties().maxStackSize(64)
    );

    FEATHER = &registry.registerItem(
        ResourceLocation("minecraft:feather"),
        ItemProperties().maxStackSize(64)
    );

    GUNPOWDER = &registry.registerItem(
        ResourceLocation("minecraft:gunpowder"),
        ItemProperties().maxStackSize(64)
    );

    LEATHER = &registry.registerItem(
        ResourceLocation("minecraft:leather"),
        ItemProperties().maxStackSize(64)
    );

    SLIME_BALL = &registry.registerItem(
        ResourceLocation("minecraft:slime_ball"),
        ItemProperties().maxStackSize(64)
    );

    EGG = &registry.registerItem(
        ResourceLocation("minecraft:egg"),
        ItemProperties().maxStackSize(16)
    );

    COMPASS = &registry.registerItem(
        ResourceLocation("minecraft:compass"),
        ItemProperties().maxStackSize(64)
    );

    CLOCK = &registry.registerItem(
        ResourceLocation("minecraft:clock"),
        ItemProperties().maxStackSize(64)
    );

    SPIDER_EYE = &registry.registerItem(
        ResourceLocation("minecraft:spider_eye"),
        ItemProperties().maxStackSize(64)
    );

    FERMENTED_SPIDER_EYE = &registry.registerItem(
        ResourceLocation("minecraft:fermented_spider_eye"),
        ItemProperties().maxStackSize(64)
    );

    BLAZE_ROD = &registry.registerItem(
        ResourceLocation("minecraft:blaze_rod"),
        ItemProperties().maxStackSize(64)
    );

    BLAZE_POWDER = &registry.registerItem(
        ResourceLocation("minecraft:blaze_powder"),
        ItemProperties().maxStackSize(64)
    );

    ENDER_PEARL = &registry.registerItem(
        ResourceLocation("minecraft:ender_pearl"),
        ItemProperties().maxStackSize(16)
    );

    ENDER_EYE = &registry.registerItem(
        ResourceLocation("minecraft:ender_eye"),
        ItemProperties().maxStackSize(64)
    );

    NETHER_STAR = &registry.registerItem(
        ResourceLocation("minecraft:nether_star"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Uncommon)
    );

    FIRE_CHARGE = &registry.registerItem(
        ResourceLocation("minecraft:fire_charge"),
        ItemProperties().maxStackSize(64)
    );

    FIREWORK_STAR = &registry.registerItem(
        ResourceLocation("minecraft:firework_star"),
        ItemProperties().maxStackSize(64)
    );

    FIREWORK_ROCKET = &registry.registerItem(
        ResourceLocation("minecraft:firework_rocket"),
        ItemProperties().maxStackSize(64)
    );
}

void Items::registerDyes() {
    auto& registry = ItemRegistry::instance();

    INK_SAC = &registry.registerItem(
        ResourceLocation("minecraft:ink_sac"),
        ItemProperties().maxStackSize(64)
    );

    RED_DYE = &registry.registerItem(
        ResourceLocation("minecraft:red_dye"),
        ItemProperties().maxStackSize(64)
    );

    GREEN_DYE = &registry.registerItem(
        ResourceLocation("minecraft:green_dye"),
        ItemProperties().maxStackSize(64)
    );

    COCOA_BEANS = &registry.registerItem(
        ResourceLocation("minecraft:cocoa_beans"),
        ItemProperties().maxStackSize(64)
    );

    LAPIS_LAZULI_DYE = &registry.registerItem(
        ResourceLocation("minecraft:lapis_lazuli_dye"),
        ItemProperties().maxStackSize(64)
    );

    PURPLE_DYE = &registry.registerItem(
        ResourceLocation("minecraft:purple_dye"),
        ItemProperties().maxStackSize(64)
    );

    CYAN_DYE = &registry.registerItem(
        ResourceLocation("minecraft:cyan_dye"),
        ItemProperties().maxStackSize(64)
    );

    LIGHT_GRAY_DYE = &registry.registerItem(
        ResourceLocation("minecraft:light_gray_dye"),
        ItemProperties().maxStackSize(64)
    );

    GRAY_DYE = &registry.registerItem(
        ResourceLocation("minecraft:gray_dye"),
        ItemProperties().maxStackSize(64)
    );

    PINK_DYE = &registry.registerItem(
        ResourceLocation("minecraft:pink_dye"),
        ItemProperties().maxStackSize(64)
    );

    LIME_DYE = &registry.registerItem(
        ResourceLocation("minecraft:lime_dye"),
        ItemProperties().maxStackSize(64)
    );

    YELLOW_DYE = &registry.registerItem(
        ResourceLocation("minecraft:yellow_dye"),
        ItemProperties().maxStackSize(64)
    );

    LIGHT_BLUE_DYE = &registry.registerItem(
        ResourceLocation("minecraft:light_blue_dye"),
        ItemProperties().maxStackSize(64)
    );

    MAGENTA_DYE = &registry.registerItem(
        ResourceLocation("minecraft:magenta_dye"),
        ItemProperties().maxStackSize(64)
    );

    ORANGE_DYE = &registry.registerItem(
        ResourceLocation("minecraft:orange_dye"),
        ItemProperties().maxStackSize(64)
    );

    WHITE_DYE = &registry.registerItem(
        ResourceLocation("minecraft:white_dye"),
        ItemProperties().maxStackSize(64)
    );
}

void Items::registerSeeds() {
    auto& registry = ItemRegistry::instance();

    WHEAT_SEEDS = &registry.registerItem(
        ResourceLocation("minecraft:wheat_seeds"),
        ItemProperties().maxStackSize(64)
    );

    PUMPKIN_SEEDS = &registry.registerItem(
        ResourceLocation("minecraft:pumpkin_seeds"),
        ItemProperties().maxStackSize(64)
    );

    MELON_SEEDS = &registry.registerItem(
        ResourceLocation("minecraft:melon_seeds"),
        ItemProperties().maxStackSize(64)
    );

    BEETROOT_SEEDS = &registry.registerItem(
        ResourceLocation("minecraft:beetroot_seeds"),
        ItemProperties().maxStackSize(64)
    );
}

void Items::registerCrops() {
    auto& registry = ItemRegistry::instance();

    WHEAT = &registry.registerItem(
        ResourceLocation("minecraft:wheat"),
        ItemProperties().maxStackSize(64)
    );

    PUMPKIN = &registry.registerItem(
        ResourceLocation("minecraft:pumpkin"),
        ItemProperties().maxStackSize(64)
    );

    MELON = &registry.registerItem(
        ResourceLocation("minecraft:melon"),
        ItemProperties().maxStackSize(64)
    );

    MELON_SLICE = &registry.registerItem(
        ResourceLocation("minecraft:melon_slice"),
        ItemProperties().maxStackSize(64)
    );

    CARROT = &registry.registerItem(
        ResourceLocation("minecraft:carrot"),
        ItemProperties().maxStackSize(64)
    );

    POTATO = &registry.registerItem(
        ResourceLocation("minecraft:potato"),
        ItemProperties().maxStackSize(64)
    );

    BEETROOT = &registry.registerItem(
        ResourceLocation("minecraft:beetroot"),
        ItemProperties().maxStackSize(64)
    );

    SUGAR_CANE = &registry.registerItem(
        ResourceLocation("minecraft:sugar_cane"),
        ItemProperties().maxStackSize(64)
    );

    SUGAR = &registry.registerItem(
        ResourceLocation("minecraft:sugar"),
        ItemProperties().maxStackSize(64)
    );
}

void Items::registerAquaticMaterials() {
    auto& registry = ItemRegistry::instance();

    // 鳞甲 - 海龟掉落，用于合成海龟壳
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    SCUTE = &registry.registerItem(
        ResourceLocation("minecraft:scute"),
        ItemProperties().maxStackSize(64)
    );

    // 海洋之心 - 宝藏物品，用于合成潮涌核心
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS).rarity(Rarity.UNCOMMON))
    HEART_OF_THE_SEA = &registry.registerItem(
        ResourceLocation("minecraft:heart_of_the_sea"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Uncommon)
    );

    // 鹦鹉螺壳 - 溺尸掉落或钓鱼获得，用于合成潮涌核心
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    NAUTILUS_SHELL = &registry.registerItem(
        ResourceLocation("minecraft:nautilus_shell"),
        ItemProperties().maxStackSize(64)
    );

    // 幻翼膜 - 幻翼掉落，用于修复鞘翅和酿造缓降药水
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    PHANTOM_MEMBRANE = &registry.registerItem(
        ResourceLocation("minecraft:phantom_membrane"),
        ItemProperties().maxStackSize(64)
    );

    // 干海带 - 食物，快速食用（32tick）
    // 参考: new Item(new Item.Properties().group(ItemGroup.FOOD).food(Food.Builder().hunger(1).saturation(0.1F).fast().build()))
    DRIED_KELP = &registry.registerItem(
        ResourceLocation("minecraft:dried_kelp"),
        ItemProperties().maxStackSize(64)
    );
}

void Items::registerBrewingIngredients() {
    auto& registry = ItemRegistry::instance();

    // 地狱疣 - 酿造基础材料，用于制作尴尬的药水
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    NETHER_WART = &registry.registerItem(
        ResourceLocation("minecraft:nether_wart"),
        ItemProperties().maxStackSize(64)
    );

    // 金胡萝卜 - 夜视药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.FOOD).food(Food.Builder().hunger(6).saturation(8.1F).build()).rarity(Rarity.UNCOMMON))
    GOLDEN_CARROT = &registry.registerItem(
        ResourceLocation("minecraft:golden_carrot"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Uncommon)
    );

    // 恶魂之泪 - 生命恢复药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    GHAST_TEAR = &registry.registerItem(
        ResourceLocation("minecraft:ghast_tear"),
        ItemProperties().maxStackSize(64)
    );

    // 兔子脚 - 跳跃药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    RABBIT_FOOT = &registry.registerItem(
        ResourceLocation("minecraft:rabbit_foot"),
        ItemProperties().maxStackSize(64)
    );

    // 岩浆膏 - 防火药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    MAGMA_CREAM = &registry.registerItem(
        ResourceLocation("minecraft:magma_cream"),
        ItemProperties().maxStackSize(64)
    );

    // 龙息 - 滞留药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS).rarity(Rarity.UNCOMMON))
    DRAGON_BREATH = &registry.registerItem(
        ResourceLocation("minecraft:dragon_breath"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Uncommon)
    );

    // 河豚 - 水下呼吸药水材料（也是食物，有毒性）
    // 参考: new Item(new Item.Properties().group(ItemGroup.FOOD).food(Food.Builder().hunger(1).saturation(0.2F).effect(new EffectInstance(Effects.POISON, 60, 0), 1.0F).build()))
    PUFFERFISH = &registry.registerItem(
        ResourceLocation("minecraft:pufferfish"),
        ItemProperties().maxStackSize(64)
    );

    // 海龟壳 - 海龟大师药水材料（装备，但也可用于酿造）
    // 参考: new ArmorItem(ArmorMaterial.TURTLE, EquipmentSlotType.HEAD, new Item.Properties().group(ItemGroup.COMBAT))
    TURTLE_HELMET = &registry.registerItem(
        ResourceLocation("minecraft:turtle_helmet"),
        ItemProperties().maxStackSize(1).maxDamage(275)
    );

    // 闪烁的西瓜片 - 瞬间治疗药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    GLISTERING_MELON_SLICE = &registry.registerItem(
        ResourceLocation("minecraft:glistering_melon_slice"),
        ItemProperties().maxStackSize(64)
    );
}

void Items::registerPotions() {
    auto& registry = ItemRegistry::instance();

    // 玻璃瓶 - 用于装水和酿造
    // 参考: new GlassBottleItem(new Item.Properties().group(ItemGroup.BREWING))
    GLASS_BOTTLE = &registry.registerItem(
        ResourceLocation("minecraft:glass_bottle"),
        ItemProperties().maxStackSize(64)
    );

    // 药水 - 饮用药水
    // 参考: new PotionItem(new Item.Properties().group(ItemGroup.BREWING))
    POTION = &registry.registerItem<item::PotionItem>(
        ResourceLocation("minecraft:potion"),
        ItemProperties().maxStackSize(1)
    );

    // 喷溅药水 - 投掷药水
    // 参考: new SplashPotionItem(new Item.Properties().group(ItemGroup.BREWING))
    SPLASH_POTION = &registry.registerItem<item::SplashPotionItem>(
        ResourceLocation("minecraft:splash_potion"),
        ItemProperties().maxStackSize(1)
    );

    // 滞留药水 - 留下效果云
    // 参考: new LingeringPotionItem(new Item.Properties().group(ItemGroup.BREWING))
    LINGERING_POTION = &registry.registerItem<item::LingeringPotionItem>(
        ResourceLocation("minecraft:lingering_potion"),
        ItemProperties().maxStackSize(1)
    );
}

} // namespace mc
