#include "Items.hpp"

#include "../entity/core/EntityRegistry.hpp"
#include "../world/block/VanillaBlocks.hpp"
#include "../world/fluid/FluidRegistry.hpp"
#include "armor/ArmorMaterial.hpp"
#include "food/Foods.hpp"
#include "items/armor/ArmorItem.hpp"
#include "items/armor/DyeableArmorItem.hpp"
#include "items/block/BlockItem.hpp"
#include "items/food/ChorusFruitItem.hpp"
#include "items/food/FoodItem.hpp"
#include "items/food/GoldenAppleItem.hpp"
#include "items/food/HoneyBottleItem.hpp"
#include "items/potion/LingeringPotionItem.hpp"
#include "items/potion/PotionItem.hpp"
#include "items/potion/SplashPotionItem.hpp"
#include "items/special/BoneMealItem.hpp"
#include "items/special/BucketItem.hpp"
#include "items/special/EnchantedBookItem.hpp"
#include "items/special/FishBucketItem.hpp"
#include "items/special/FlintAndSteelItem.hpp"
#include "items/special/MilkBucketItem.hpp"
#include "items/special/NameTagItem.hpp"
#include "items/special/SaddleItem.hpp"
#include "items/tool/AxeItem.hpp"
#include "items/tool/HoeItem.hpp"
#include "items/tool/PickaxeItem.hpp"
#include "items/tool/ShearsItem.hpp"
#include "items/tool/ShovelItem.hpp"
#include "items/tool/SwordItem.hpp"
#include "items/vehicle/MinecartItem.hpp"
#include "items/weapon/ArrowItem.hpp"
#include "items/weapon/BowItem.hpp"
#include "items/weapon/CrossbowItem.hpp"
#include "items/weapon/FishingRodItem.hpp"
#include "items/weapon/ShieldItem.hpp"
#include "items/weapon/ThrowableItem.hpp"
#include "items/weapon/ThrowableItems.hpp"
#include "items/weapon/TippedArrowItem.hpp"
#include "items/weapon/TridentItem.hpp"
#include "tier/ItemTiers.hpp"

namespace {

mc::Item& registerBlockBackedItem(
    mc::ItemRegistry& registry, mc::Block* block, const char* path, mc::ItemProperties properties)
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

// 下界合金工具
Item* Items::NETHERITE_PICKAXE = nullptr;
Item* Items::NETHERITE_AXE = nullptr;
Item* Items::NETHERITE_SHOVEL = nullptr;
Item* Items::NETHERITE_HOE = nullptr;
Item* Items::NETHERITE_SWORD = nullptr;

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

// 锁链护甲
Item* Items::CHAINMAIL_HELMET = nullptr;
Item* Items::CHAINMAIL_CHESTPLATE = nullptr;
Item* Items::CHAINMAIL_LEGGINGS = nullptr;
Item* Items::CHAINMAIL_BOOTS = nullptr;

// 下界合金护甲
Item* Items::NETHERITE_HELMET = nullptr;
Item* Items::NETHERITE_CHESTPLATE = nullptr;
Item* Items::NETHERITE_LEGGINGS = nullptr;
Item* Items::NETHERITE_BOOTS = nullptr;

// 特殊护甲
Item* Items::ELYTRA = nullptr;

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
// 缺失的食物
Item* Items::BAKED_POTATO = nullptr;
Item* Items::BEETROOT = nullptr;
Item* Items::BEETROOT_SOUP = nullptr;
Item* Items::CARROT = nullptr;
Item* Items::CHORUS_FRUIT = nullptr;
Item* Items::COOKIE = nullptr;
Item* Items::DRIED_KELP = nullptr;
Item* Items::GOLDEN_CARROT = nullptr;
Item* Items::HONEY_BOTTLE = nullptr;
Item* Items::MELON_SLICE = nullptr;
Item* Items::MUSHROOM_STEW = nullptr;
Item* Items::POISONOUS_POTATO = nullptr;
Item* Items::POTATO = nullptr;
Item* Items::PUFFERFISH = nullptr;
Item* Items::PUMPKIN_PIE = nullptr;
Item* Items::RABBIT_STEW = nullptr;
Item* Items::ROTTEN_FLESH = nullptr;
Item* Items::SPIDER_EYE = nullptr;
Item* Items::SUSPICIOUS_STEW = nullptr;
Item* Items::SWEET_BERRIES = nullptr;
Item* Items::TROPICAL_FISH = nullptr;

// 木棍、骨头和碗
Item* Items::STICK = nullptr;
Item* Items::BONE = nullptr;
Item* Items::BONE_MEAL = nullptr;
Item* Items::BOWL = nullptr;

// 杂项
Item* Items::FLINT = nullptr;
Item* Items::FLINT_AND_STEEL = nullptr;
Item* Items::SHEARS = nullptr;
Item* Items::NAME_TAG = nullptr;
Item* Items::SADDLE = nullptr;
Item* Items::STRING = nullptr;
Item* Items::FEATHER = nullptr;
Item* Items::GUNPOWDER = nullptr;
Item* Items::LEATHER = nullptr;
Item* Items::SLIME_BALL = nullptr;
Item* Items::EGG = nullptr;
Item* Items::SNOWBALL = nullptr;
Item* Items::COMPASS = nullptr;
Item* Items::CLOCK = nullptr;
// SPIDER_EYE 已在食物部分声明
Item* Items::FERMENTED_SPIDER_EYE = nullptr;
Item* Items::BLAZE_ROD = nullptr;
Item* Items::BLAZE_POWDER = nullptr;
Item* Items::ENDER_PEARL = nullptr;
Item* Items::ENDER_EYE = nullptr;
Item* Items::NETHER_STAR = nullptr;
Item* Items::FIRE_CHARGE = nullptr;
Item* Items::FIREWORK_STAR = nullptr;
Item* Items::FIREWORK_ROCKET = nullptr;
Item* Items::EXPERIENCE_BOTTLE = nullptr;

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
// MELON_SLICE 已在食物部分声明
// CARROT 已在食物部分声明
// POTATO 已在食物部分声明
// BEETROOT 已在食物部分声明
Item* Items::SUGAR_CANE = nullptr;
Item* Items::SUGAR = nullptr;

// 水域更新材料
Item* Items::SCUTE = nullptr;
Item* Items::HEART_OF_THE_SEA = nullptr;
Item* Items::NAUTILUS_SHELL = nullptr;
Item* Items::PHANTOM_MEMBRANE = nullptr;
// DRIED_KELP 已在食物部分声明

// ============================================================================
// 酿造材料
// ============================================================================
Item* Items::NETHER_WART = nullptr;
// GOLDEN_CARROT 已在食物部分声明
Item* Items::GHAST_TEAR = nullptr;
Item* Items::RABBIT_FOOT = nullptr;
Item* Items::MAGMA_CREAM = nullptr;
Item* Items::DRAGON_BREATH = nullptr;
// PUFFERFISH 已在食物部分声明
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
// 武器和弹药
// ============================================================================
Item* Items::BOW = nullptr;
Item* Items::ARROW = nullptr;
Item* Items::SPECTRAL_ARROW = nullptr;
Item* Items::TIPPED_ARROW = nullptr;
Item* Items::CROSSBOW = nullptr;
Item* Items::TRIDENT = nullptr;
Item* Items::SHIELD = nullptr;
Item* Items::FISHING_ROD = nullptr;

// ============================================================================
// 桶类
// ============================================================================
Item* Items::BUCKET = nullptr;
Item* Items::WATER_BUCKET = nullptr;
Item* Items::LAVA_BUCKET = nullptr;
Item* Items::COD_BUCKET = nullptr;
Item* Items::SALMON_BUCKET = nullptr;
Item* Items::PUFFERFISH_BUCKET = nullptr;
Item* Items::TROPICAL_FISH_BUCKET = nullptr;
Item* Items::MILK_BUCKET = nullptr;

// ============================================================================
// 书本类物品
// ============================================================================
Item* Items::BOOK = nullptr;
Item* Items::ENCHANTED_BOOK = nullptr;
Item* Items::WRITABLE_BOOK = nullptr;
Item* Items::WRITTEN_BOOK = nullptr;

// ============================================================================
// 海绵
// ============================================================================
Item* Items::SPONGE = nullptr;
Item* Items::WET_SPONGE = nullptr;

// ============================================================================
// 矿车
// ============================================================================
Item* Items::MINECART = nullptr;
Item* Items::CHEST_MINECART = nullptr;
Item* Items::FURNACE_MINECART = nullptr;
Item* Items::TNT_MINECART = nullptr;
Item* Items::HOPPER_MINECART = nullptr;
Item* Items::COMMAND_BLOCK_MINECART = nullptr;

// ============================================================================
// 悬挂实体物品
// ============================================================================
Item* Items::PAINTING = nullptr;
Item* Items::ITEM_FRAME = nullptr;
Item* Items::LEAD = nullptr;

// ============================================================================
// 初始化
// ============================================================================

void Items::initialize()
{
    if (s_initialized) {
        return;
    }

    // 初始化食物属性（必须在注册食物物品前）
    item::food::Foods::initialize();

    // 初始化盔甲材质（必须在注册盔甲物品前）
    item::armor::ArmorMaterials::initialize();

    auto& registry = ItemRegistry::instance();

    // 空气物品（ID 0）
    AIR = &registry.registerItem(ResourceLocation("minecraft:air"), ItemProperties().maxStackSize(64));

    registerMaterials();
    registerMisc(); // 提前注册，因为工具层级需要木板等作为修复材料

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
    registerWeapons();      // 武器和弹药
    registerThrowables();   // 投掷物品
    registerBuckets();      // 桶类物品（需要 BUCKET 在 WATER_BUCKET/LAVA_BUCKET 之前注册）
    registerBooks();        // 书本类物品
    registerSponges();      // 海绵物品
    registerMinecarts();    // 矿车物品
    registerHangingItems(); // 悬挂实体物品

    s_initialized = true;
}

void Items::registerMaterials()
{
    auto& registry = ItemRegistry::instance();

    // 宝石
    DIAMOND = &registry.registerItem(
        ResourceLocation("minecraft:diamond"), ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare));

    EMERALD = &registry.registerItem(
        ResourceLocation("minecraft:emerald"), ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare));

    // 锭
    GOLD_INGOT = &registry.registerItem(ResourceLocation("minecraft:gold_ingot"), ItemProperties().maxStackSize(64));

    IRON_INGOT = &registry.registerItem(ResourceLocation("minecraft:iron_ingot"), ItemProperties().maxStackSize(64));

    COPPER_INGOT =
        &registry.registerItem(ResourceLocation("minecraft:copper_ingot"), ItemProperties().maxStackSize(64));

    NETHERITE_INGOT = &registry.registerItem(
        ResourceLocation("minecraft:netherite_ingot"), ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare));

    NETHERITE_SCRAP =
        &registry.registerItem(ResourceLocation("minecraft:netherite_scrap"), ItemProperties().maxStackSize(64));

    // 煤炭
    COAL = &registry.registerItem(ResourceLocation("minecraft:coal"), ItemProperties().maxStackSize(64));

    CHARCOAL = &registry.registerItem(ResourceLocation("minecraft:charcoal"), ItemProperties().maxStackSize(64));

    // 红石相关
    REDSTONE = &registry.registerItem(ResourceLocation("minecraft:redstone"), ItemProperties().maxStackSize(64));

    LAPIS_LAZULI =
        &registry.registerItem(ResourceLocation("minecraft:lapis_lazuli"), ItemProperties().maxStackSize(64));

    QUARTZ = &registry.registerItem(ResourceLocation("minecraft:quartz"), ItemProperties().maxStackSize(64));

    GLOWSTONE_DUST =
        &registry.registerItem(ResourceLocation("minecraft:glowstone_dust"), ItemProperties().maxStackSize(64));

    COAL_ORE =
        &registerBlockBackedItem(registry, VanillaBlocks::COAL_ORE, "coal_ore", ItemProperties().maxStackSize(64));
    IRON_ORE =
        &registerBlockBackedItem(registry, VanillaBlocks::IRON_ORE, "iron_ore", ItemProperties().maxStackSize(64));
    GOLD_ORE =
        &registerBlockBackedItem(registry, VanillaBlocks::GOLD_ORE, "gold_ore", ItemProperties().maxStackSize(64));
    DIAMOND_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DIAMOND_ORE, "diamond_ore", ItemProperties().maxStackSize(64));
    EMERALD_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::EMERALD_ORE, "emerald_ore", ItemProperties().maxStackSize(64));
    LAPIS_ORE =
        &registerBlockBackedItem(registry, VanillaBlocks::LAPIS_ORE, "lapis_ore", ItemProperties().maxStackSize(64));
    REDSTONE_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::REDSTONE_ORE, "redstone_ore", ItemProperties().maxStackSize(64));
}

void Items::registerTools()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 钻石工具
    // MC 1.16.5: 斧基础伤害7.0 + tier(3.0) = 总伤害10.0
    //            锄基础伤害0，攻击速度-3.0
    // ========================================================================
    DIAMOND_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:diamond_pickaxe"),
        item::tier::ItemTiers::DIAMOND(), // tier
        1,                                // attackDamage
        -2.8f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    DIAMOND_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:diamond_axe"),
        item::tier::ItemTiers::DIAMOND(), // tier
        7.0f,                             // attackDamage (MC 1.16.5: 7.0, 总伤害=7.0+3.0=10.0)
        -3.0f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    DIAMOND_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:diamond_shovel"),
        item::tier::ItemTiers::DIAMOND(), // tier
        1.5f,                             // attackDamage
        -3.0f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    DIAMOND_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:diamond_hoe"),
        item::tier::ItemTiers::DIAMOND(), // tier
        0,                                // attackDamage (MC 1.16.5: 0)
        -3.0f,                            // attackSpeed (MC 1.16.5: -3.0)
        ItemProperties().rarity(ItemRarity::Common));

    DIAMOND_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:diamond_sword"),
        item::tier::ItemTiers::DIAMOND(), // tier
        3,                                // attackDamage
        -2.4f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    // ========================================================================
    // 铁工具
    // MC 1.16.5: 斧基础伤害7.0 + tier(2.0) = 总伤害9.0，攻击速度-3.1
    //            锄基础伤害0，攻击速度-2.0
    // ========================================================================
    IRON_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:iron_pickaxe"),
        item::tier::ItemTiers::IRON(), // tier
        1,                             // attackDamage
        -2.8f,                         // attackSpeed
        ItemProperties());

    IRON_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:iron_axe"),
        item::tier::ItemTiers::IRON(), // tier
        7.0f,                          // attackDamage (MC 1.16.5: 7.0, 总伤害=7.0+2.0=9.0)
        -3.1f,                         // attackSpeed (MC 1.16.5: -3.1)
        ItemProperties());

    IRON_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:iron_shovel"),
        item::tier::ItemTiers::IRON(), // tier
        1.5f,                          // attackDamage
        -3.0f,                         // attackSpeed
        ItemProperties());

    IRON_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:iron_hoe"),
        item::tier::ItemTiers::IRON(), // tier
        0,                             // attackDamage (MC 1.16.5: 0)
        -2.0f,                         // attackSpeed (MC 1.16.5: -2.0)
        ItemProperties());

    IRON_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:iron_sword"),
        item::tier::ItemTiers::IRON(), // tier
        3,                             // attackDamage
        -2.4f,                         // attackSpeed
        ItemProperties());

    // ========================================================================
    // 石工具
    // MC 1.16.5: 斧基础伤害8.0 + tier(1.0) = 总伤害9.0，攻击速度-3.2
    //            锄基础伤害0，攻击速度-1.0
    // ========================================================================
    STONE_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:stone_pickaxe"),
        item::tier::ItemTiers::STONE(), // tier
        1,                              // attackDamage
        -2.8f,                          // attackSpeed
        ItemProperties());

    STONE_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:stone_axe"),
        item::tier::ItemTiers::STONE(), // tier
        8.0f,                           // attackDamage (MC 1.16.5: 8.0, 总伤害=8.0+1.0=9.0)
        -3.2f,                          // attackSpeed (MC 1.16.5: -3.2)
        ItemProperties());

    STONE_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:stone_shovel"),
        item::tier::ItemTiers::STONE(), // tier
        1.5f,                           // attackDamage
        -3.0f,                          // attackSpeed
        ItemProperties());

    STONE_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:stone_hoe"),
        item::tier::ItemTiers::STONE(), // tier
        0,                              // attackDamage (MC 1.16.5: 0)
        -1.0f,                          // attackSpeed (MC 1.16.5: -1.0)
        ItemProperties());

    STONE_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:stone_sword"),
        item::tier::ItemTiers::STONE(), // tier
        3,                              // attackDamage
        -2.4f,                          // attackSpeed
        ItemProperties());

    // ========================================================================
    // 木工具
    // MC 1.16.5: 斧基础伤害7.0 + tier(0.0) = 总伤害7.0，攻击速度-3.2
    //            锄基础伤害0，攻击速度0.0
    // ========================================================================
    WOODEN_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:wooden_pickaxe"),
        item::tier::ItemTiers::WOOD(), // tier
        1,                             // attackDamage
        -2.8f,                         // attackSpeed
        ItemProperties());

    WOODEN_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:wooden_axe"),
        item::tier::ItemTiers::WOOD(), // tier
        7.0f,                          // attackDamage (MC 1.16.5: 7.0, 总伤害=7.0+0.0=7.0)
        -3.2f,                         // attackSpeed (MC 1.16.5: -3.2)
        ItemProperties());

    WOODEN_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:wooden_shovel"),
        item::tier::ItemTiers::WOOD(), // tier
        1.5f,                          // attackDamage (MC 1.16.5: 1.5)
        -3.0f,                         // attackSpeed
        ItemProperties());

    WOODEN_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:wooden_hoe"),
        item::tier::ItemTiers::WOOD(), // tier
        0,                             // attackDamage (MC 1.16.5: 0)
        0.0f,                          // attackSpeed (MC 1.16.5: 0.0)
        ItemProperties());

    WOODEN_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:wooden_sword"),
        item::tier::ItemTiers::WOOD(), // tier
        3,                             // attackDamage
        -2.4f,                         // attackSpeed
        ItemProperties());

    // ========================================================================
    // 金工具
    // MC 1.16.5: 斧基础伤害7.0 + tier(0.0) = 总伤害7.0
    //            锄基础伤害0，攻击速度0.0
    // ========================================================================
    GOLDEN_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:golden_pickaxe"),
        item::tier::ItemTiers::GOLD(), // tier
        1,                             // attackDamage
        -2.8f,                         // attackSpeed
        ItemProperties());

    GOLDEN_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:golden_axe"),
        item::tier::ItemTiers::GOLD(), // tier
        7.0f,                          // attackDamage (MC 1.16.5: 7.0, 总伤害=7.0+0.0=7.0)
        -3.0f,                         // attackSpeed
        ItemProperties());

    GOLDEN_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:golden_shovel"),
        item::tier::ItemTiers::GOLD(), // tier
        1.5f,                          // attackDamage (MC 1.16.5: 1.5)
        -3.0f,                         // attackSpeed
        ItemProperties());

    GOLDEN_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:golden_hoe"),
        item::tier::ItemTiers::GOLD(), // tier
        0,                             // attackDamage (MC 1.16.5: 0)
        0.0f,                          // attackSpeed (MC 1.16.5: 0.0)
        ItemProperties());

    GOLDEN_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:golden_sword"),
        item::tier::ItemTiers::GOLD(), // tier
        3,                             // attackDamage
        -2.4f,                         // attackSpeed
        ItemProperties());

    // ========================================================================
    // 下界合金工具
    // MC 1.16.5: 斧基础伤害6.0 + tier(4.0) = 总伤害10.0
    //            锄基础伤害0，攻击速度-4.0
    // ========================================================================
    NETHERITE_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:netherite_pickaxe"),
        item::tier::ItemTiers::NETHERITE(), // tier
        1,                                  // attackDamage
        -2.8f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));

    NETHERITE_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:netherite_axe"),
        item::tier::ItemTiers::NETHERITE(), // tier
        6.0f,                               // attackDamage (MC 1.16.5: 6.0, 总伤害=6.0+4.0=10.0)
        -3.0f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));

    NETHERITE_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:netherite_shovel"),
        item::tier::ItemTiers::NETHERITE(), // tier
        1.5f,                               // attackDamage
        -3.0f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));

    NETHERITE_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:netherite_hoe"),
        item::tier::ItemTiers::NETHERITE(), // tier
        0,                                  // attackDamage (MC 1.16.5: 0)
        -4.0f,                              // attackSpeed (MC 1.16.5: -4.0)
        ItemProperties().rarity(ItemRarity::Rare));

    NETHERITE_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:netherite_sword"),
        item::tier::ItemTiers::NETHERITE(), // tier
        3,                                  // attackDamage
        -2.4f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));
}

void Items::registerArmor()
{
    auto& registry = ItemRegistry::instance();
    using namespace item::armor;

    // ========================================================================
    // 钻石护甲
    // ========================================================================
    DIAMOND_HELMET = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:diamond_helmet"),
        ArmorMaterials::DIAMOND,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(ArmorMaterials::DIAMOND.getDurability(item::armor::ArmorSlot::Head)));

    DIAMOND_CHESTPLATE =
        &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:diamond_chestplate"),
            ArmorMaterials::DIAMOND,
            item::armor::ArmorSlot::Chest,
            ItemProperties().maxDamage(ArmorMaterials::DIAMOND.getDurability(item::armor::ArmorSlot::Chest)));

    DIAMOND_LEGGINGS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:diamond_leggings"),
        ArmorMaterials::DIAMOND,
        item::armor::ArmorSlot::Legs,
        ItemProperties().maxDamage(ArmorMaterials::DIAMOND.getDurability(item::armor::ArmorSlot::Legs)));

    DIAMOND_BOOTS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:diamond_boots"),
        ArmorMaterials::DIAMOND,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(ArmorMaterials::DIAMOND.getDurability(item::armor::ArmorSlot::Feet)));

    // ========================================================================
    // 铁护甲
    // ========================================================================
    IRON_HELMET = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:iron_helmet"),
        ArmorMaterials::IRON,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Head)));

    IRON_CHESTPLATE = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:iron_chestplate"),
        ArmorMaterials::IRON,
        item::armor::ArmorSlot::Chest,
        ItemProperties().maxDamage(ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Chest)));

    IRON_LEGGINGS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:iron_leggings"),
        ArmorMaterials::IRON,
        item::armor::ArmorSlot::Legs,
        ItemProperties().maxDamage(ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Legs)));

    IRON_BOOTS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:iron_boots"),
        ArmorMaterials::IRON,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Feet)));

    // ========================================================================
    // 金护甲
    // ========================================================================
    GOLDEN_HELMET = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:golden_helmet"),
        ArmorMaterials::GOLD,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(ArmorMaterials::GOLD.getDurability(item::armor::ArmorSlot::Head)));

    GOLDEN_CHESTPLATE = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:golden_chestplate"),
        ArmorMaterials::GOLD,
        item::armor::ArmorSlot::Chest,
        ItemProperties().maxDamage(ArmorMaterials::GOLD.getDurability(item::armor::ArmorSlot::Chest)));

    GOLDEN_LEGGINGS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:golden_leggings"),
        ArmorMaterials::GOLD,
        item::armor::ArmorSlot::Legs,
        ItemProperties().maxDamage(ArmorMaterials::GOLD.getDurability(item::armor::ArmorSlot::Legs)));

    GOLDEN_BOOTS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:golden_boots"),
        ArmorMaterials::GOLD,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(ArmorMaterials::GOLD.getDurability(item::armor::ArmorSlot::Feet)));

    // ========================================================================
    // 皮革护甲（可染色）
    // ========================================================================
    LEATHER_HELMET = &registry.registerItem<item::items::DyeableArmorItem>(ResourceLocation("minecraft:leather_helmet"),
        ArmorMaterials::LEATHER,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(ArmorMaterials::LEATHER.getDurability(item::armor::ArmorSlot::Head)));

    LEATHER_CHESTPLATE =
        &registry.registerItem<item::items::DyeableArmorItem>(ResourceLocation("minecraft:leather_chestplate"),
            ArmorMaterials::LEATHER,
            item::armor::ArmorSlot::Chest,
            ItemProperties().maxDamage(ArmorMaterials::LEATHER.getDurability(item::armor::ArmorSlot::Chest)));

    LEATHER_LEGGINGS =
        &registry.registerItem<item::items::DyeableArmorItem>(ResourceLocation("minecraft:leather_leggings"),
            ArmorMaterials::LEATHER,
            item::armor::ArmorSlot::Legs,
            ItemProperties().maxDamage(ArmorMaterials::LEATHER.getDurability(item::armor::ArmorSlot::Legs)));

    LEATHER_BOOTS = &registry.registerItem<item::items::DyeableArmorItem>(ResourceLocation("minecraft:leather_boots"),
        ArmorMaterials::LEATHER,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(ArmorMaterials::LEATHER.getDurability(item::armor::ArmorSlot::Feet)));

    // ========================================================================
    // 锁链护甲
    // ========================================================================
    CHAINMAIL_HELMET = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:chainmail_helmet"),
        ArmorMaterials::CHAIN,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(ArmorMaterials::CHAIN.getDurability(item::armor::ArmorSlot::Head)));

    CHAINMAIL_CHESTPLATE =
        &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:chainmail_chestplate"),
            ArmorMaterials::CHAIN,
            item::armor::ArmorSlot::Chest,
            ItemProperties().maxDamage(ArmorMaterials::CHAIN.getDurability(item::armor::ArmorSlot::Chest)));

    CHAINMAIL_LEGGINGS =
        &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:chainmail_leggings"),
            ArmorMaterials::CHAIN,
            item::armor::ArmorSlot::Legs,
            ItemProperties().maxDamage(ArmorMaterials::CHAIN.getDurability(item::armor::ArmorSlot::Legs)));

    CHAINMAIL_BOOTS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:chainmail_boots"),
        ArmorMaterials::CHAIN,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(ArmorMaterials::CHAIN.getDurability(item::armor::ArmorSlot::Feet)));

    // ========================================================================
    // 下界合金护甲
    // ========================================================================
    NETHERITE_HELMET = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:netherite_helmet"),
        ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Head,
        ItemProperties()
            .maxDamage(ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Head))
            .rarity(ItemRarity::Rare));

    NETHERITE_CHESTPLATE =
        &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:netherite_chestplate"),
            ArmorMaterials::NETHERITE,
            item::armor::ArmorSlot::Chest,
            ItemProperties()
                .maxDamage(ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Chest))
                .rarity(ItemRarity::Rare));

    NETHERITE_LEGGINGS =
        &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:netherite_leggings"),
            ArmorMaterials::NETHERITE,
            item::armor::ArmorSlot::Legs,
            ItemProperties()
                .maxDamage(ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Legs))
                .rarity(ItemRarity::Rare));

    NETHERITE_BOOTS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:netherite_boots"),
        ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Feet,
        ItemProperties()
            .maxDamage(ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Feet))
            .rarity(ItemRarity::Rare));

    // ========================================================================
    // 鞘翅
    // ========================================================================
    ELYTRA = &registry.registerItem(
        ResourceLocation("minecraft:elytra"), ItemProperties().maxDamage(432).rarity(ItemRarity::Uncommon));
}

void Items::registerFood()
{
    auto& registry = ItemRegistry::instance();
    using namespace item::food;

    // 基础食物
    APPLE = &registry.registerItem(
        ResourceLocation("minecraft:apple"), ItemProperties().maxStackSize(64).food(&Foods::APPLE));

    // 金苹果 - 使用自定义 GoldenAppleItem 以支持僵尸村民治愈
    GOLDEN_APPLE = &registry.registerItem<item::items::GoldenAppleItem>(ResourceLocation("minecraft:golden_apple"),
        &Foods::GOLDEN_APPLE,
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare));

    // 附魔金苹果 - 使用自定义 GoldenAppleItem
    ENCHANTED_GOLDEN_APPLE =
        &registry.registerItem<item::items::GoldenAppleItem>(ResourceLocation("minecraft:enchanted_golden_apple"),
            &Foods::ENCHANTED_GOLDEN_APPLE,
            ItemProperties().maxStackSize(64).rarity(ItemRarity::Epic));

    BREAD = &registry.registerItem(
        ResourceLocation("minecraft:bread"), ItemProperties().maxStackSize(64).food(&Foods::BREAD));

    // 熟食
    COOKED_BEEF = &registry.registerItem(
        ResourceLocation("minecraft:cooked_beef"), ItemProperties().maxStackSize(64).food(&Foods::COOKED_BEEF));

    COOKED_PORKCHOP = &registry.registerItem(
        ResourceLocation("minecraft:cooked_porkchop"), ItemProperties().maxStackSize(64).food(&Foods::COOKED_PORKCHOP));

    COOKED_CHICKEN = &registry.registerItem(
        ResourceLocation("minecraft:cooked_chicken"), ItemProperties().maxStackSize(64).food(&Foods::COOKED_CHICKEN));

    COOKED_MUTTON = &registry.registerItem(
        ResourceLocation("minecraft:cooked_mutton"), ItemProperties().maxStackSize(64).food(&Foods::COOKED_MUTTON));

    COOKED_RABBIT = &registry.registerItem(
        ResourceLocation("minecraft:cooked_rabbit"), ItemProperties().maxStackSize(64).food(&Foods::COOKED_RABBIT));

    COOKED_COD = &registry.registerItem(
        ResourceLocation("minecraft:cooked_cod"), ItemProperties().maxStackSize(64).food(&Foods::COOKED_COD));

    COOKED_SALMON = &registry.registerItem(
        ResourceLocation("minecraft:cooked_salmon"), ItemProperties().maxStackSize(64).food(&Foods::COOKED_SALMON));

    // 生食
    BEEF = &registry.registerItem(
        ResourceLocation("minecraft:beef"), ItemProperties().maxStackSize(64).food(&Foods::BEEF));

    PORKCHOP = &registry.registerItem(
        ResourceLocation("minecraft:porkchop"), ItemProperties().maxStackSize(64).food(&Foods::PORKCHOP));

    CHICKEN = &registry.registerItem(
        ResourceLocation("minecraft:chicken"), ItemProperties().maxStackSize(64).food(&Foods::CHICKEN));

    MUTTON = &registry.registerItem(
        ResourceLocation("minecraft:mutton"), ItemProperties().maxStackSize(64).food(&Foods::MUTTON));

    RABBIT = &registry.registerItem(
        ResourceLocation("minecraft:rabbit"), ItemProperties().maxStackSize(64).food(&Foods::RABBIT));

    COD =
        &registry.registerItem(ResourceLocation("minecraft:cod"), ItemProperties().maxStackSize(64).food(&Foods::COD));

    SALMON = &registry.registerItem(
        ResourceLocation("minecraft:salmon"), ItemProperties().maxStackSize(64).food(&Foods::SALMON));

    // 缺失的食物
    BAKED_POTATO = &registry.registerItem(
        ResourceLocation("minecraft:baked_potato"), ItemProperties().maxStackSize(64).food(&Foods::BAKED_POTATO));

    BEETROOT = &registry.registerItem(
        ResourceLocation("minecraft:beetroot"), ItemProperties().maxStackSize(64).food(&Foods::BEETROOT));

    BEETROOT_SOUP = &registry.registerItem<item::items::FoodItem>(ResourceLocation("minecraft:beetroot_soup"),
        &Foods::BEETROOT_SOUP,
        ItemProperties().maxStackSize(1).containerItem(BOWL));

    CARROT = &registry.registerItem(
        ResourceLocation("minecraft:carrot"), ItemProperties().maxStackSize(64).food(&Foods::CARROT));

    // 紫颂果 - 食用后随机传送
    // 参考: net.minecraft.item.ChorusFruitItem
    CHORUS_FRUIT = &registry.registerItem<item::items::ChorusFruitItem>(
        ResourceLocation("minecraft:chorus_fruit"), &Foods::CHORUS_FRUIT, ItemProperties().maxStackSize(64));

    COOKIE = &registry.registerItem(
        ResourceLocation("minecraft:cookie"), ItemProperties().maxStackSize(64).food(&Foods::COOKIE));

    DRIED_KELP = &registry.registerItem(
        ResourceLocation("minecraft:dried_kelp"), ItemProperties().maxStackSize(64).food(&Foods::DRIED_KELP));

    GOLDEN_CARROT = &registry.registerItem(ResourceLocation("minecraft:golden_carrot"),
        ItemProperties().maxStackSize(64).rarity(ItemRarity::Rare).food(&Foods::GOLDEN_CARROT));

    // 蜂蜜瓶 - 清除中毒效果，返回玻璃瓶
    // 参考: net.minecraft.item.HoneyBottleItem
    HONEY_BOTTLE = &registry.registerItem<item::items::HoneyBottleItem>(
        ResourceLocation("minecraft:honey_bottle"), &Foods::HONEY_BOTTLE, ItemProperties().maxStackSize(16));

    MELON_SLICE = &registry.registerItem(
        ResourceLocation("minecraft:melon_slice"), ItemProperties().maxStackSize(64).food(&Foods::MELON_SLICE));

    MUSHROOM_STEW = &registry.registerItem<item::items::FoodItem>(ResourceLocation("minecraft:mushroom_stew"),
        &Foods::MUSHROOM_STEW,
        ItemProperties().maxStackSize(1).containerItem(BOWL));

    POISONOUS_POTATO = &registry.registerItem(ResourceLocation("minecraft:poisonous_potato"),
        ItemProperties().maxStackSize(64).food(&Foods::POISONOUS_POTATO));

    POTATO = &registry.registerItem(
        ResourceLocation("minecraft:potato"), ItemProperties().maxStackSize(64).food(&Foods::POTATO));

    PUFFERFISH = &registry.registerItem(
        ResourceLocation("minecraft:pufferfish"), ItemProperties().maxStackSize(64).food(&Foods::PUFFERFISH));

    PUMPKIN_PIE = &registry.registerItem(
        ResourceLocation("minecraft:pumpkin_pie"), ItemProperties().maxStackSize(64).food(&Foods::PUMPKIN_PIE));

    RABBIT_STEW = &registry.registerItem<item::items::FoodItem>(ResourceLocation("minecraft:rabbit_stew"),
        &Foods::RABBIT_STEW,
        ItemProperties().maxStackSize(1).containerItem(BOWL));

    ROTTEN_FLESH = &registry.registerItem(
        ResourceLocation("minecraft:rotten_flesh"), ItemProperties().maxStackSize(64).food(&Foods::ROTTEN_FLESH));

    SPIDER_EYE = &registry.registerItem(
        ResourceLocation("minecraft:spider_eye"), ItemProperties().maxStackSize(64).food(&Foods::SPIDER_EYE));

    SUSPICIOUS_STEW = &registry.registerItem<item::items::FoodItem>(ResourceLocation("minecraft:suspicious_stew"),
        &Foods::SUSPICIOUS_STEW,
        ItemProperties().maxStackSize(1).containerItem(BOWL));

    SWEET_BERRIES = &registry.registerItem(
        ResourceLocation("minecraft:sweet_berries"), ItemProperties().maxStackSize(64).food(&Foods::SWEET_BERRIES));

    TROPICAL_FISH = &registry.registerItem(
        ResourceLocation("minecraft:tropical_fish"), ItemProperties().maxStackSize(64).food(&Foods::TROPICAL_FISH));
}

void Items::registerMisc()
{
    auto& registry = ItemRegistry::instance();

    // 木头和木板
    OAK_LOG = &registerBlockBackedItem(registry, VanillaBlocks::OAK_LOG, "oak_log", ItemProperties().maxStackSize(64));
    SPRUCE_LOG =
        &registerBlockBackedItem(registry, VanillaBlocks::SPRUCE_LOG, "spruce_log", ItemProperties().maxStackSize(64));
    BIRCH_LOG =
        &registerBlockBackedItem(registry, VanillaBlocks::BIRCH_LOG, "birch_log", ItemProperties().maxStackSize(64));
    JUNGLE_LOG =
        &registerBlockBackedItem(registry, VanillaBlocks::JUNGLE_LOG, "jungle_log", ItemProperties().maxStackSize(64));
    ACACIA_LOG =
        &registerBlockBackedItem(registry, VanillaBlocks::ACACIA_LOG, "acacia_log", ItemProperties().maxStackSize(64));
    DARK_OAK_LOG = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_LOG, "dark_oak_log", ItemProperties().maxStackSize(64));

    OAK_PLANKS =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_PLANKS, "oak_planks", ItemProperties().maxStackSize(64));
    SPRUCE_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_PLANKS, "spruce_planks", ItemProperties().maxStackSize(64));
    BIRCH_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_PLANKS, "birch_planks", ItemProperties().maxStackSize(64));
    JUNGLE_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_PLANKS, "jungle_planks", ItemProperties().maxStackSize(64));
    ACACIA_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_PLANKS, "acacia_planks", ItemProperties().maxStackSize(64));
    DARK_OAK_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_PLANKS, "dark_oak_planks", ItemProperties().maxStackSize(64));

    // 石头
    STONE = &registerBlockBackedItem(registry, VanillaBlocks::STONE, "stone", ItemProperties().maxStackSize(64));
    COBBLESTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::COBBLESTONE, "cobblestone", ItemProperties().maxStackSize(64));
    MOSSY_COBBLESTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::MOSSY_COBBLESTONE, "mossy_cobblestone", ItemProperties().maxStackSize(64));

    // 木棍和骨头
    STICK = &registry.registerItem(ResourceLocation("minecraft:stick"), ItemProperties().maxStackSize(64));

    BONE = &registry.registerItem(ResourceLocation("minecraft:bone"), ItemProperties().maxStackSize(64));

    BONE_MEAL = &registry.registerItem<item::items::BoneMealItem>(
        ResourceLocation("minecraft:bone_meal"), ItemProperties().maxStackSize(64));

    BOWL = &registry.registerItem(ResourceLocation("minecraft:bowl"), ItemProperties().maxStackSize(64));

    FLINT = &registry.registerItem(ResourceLocation("minecraft:flint"), ItemProperties().maxStackSize(64));

    FLINT_AND_STEEL = &registry.registerItem<item::tool::FlintAndSteelItem>(
        ResourceLocation("minecraft:flint_and_steel"), ItemProperties().maxDamage(64));

    // 剪刀 - MC 1.16.5: 耐久度 238
    SHEARS = &registry.registerItem<item::tool::ShearsItem>(
        ResourceLocation("minecraft:shears"), ItemProperties().maxDamage(238));

    // 命名牌 - 给生物命名，使其持久化
    NAME_TAG = &registry.registerItem<item::items::NameTagItem>(
        ResourceLocation("minecraft:name_tag"), ItemProperties().maxStackSize(64));

    // 鞍 - 用于装备可骑乘实体（猪、炽足兽、马等）
    SADDLE = &registry.registerItem<item::items::SaddleItem>(
        ResourceLocation("minecraft:saddle"), ItemProperties().maxStackSize(64));

    STRING = &registry.registerItem(ResourceLocation("minecraft:string"), ItemProperties().maxStackSize(64));

    FEATHER = &registry.registerItem(ResourceLocation("minecraft:feather"), ItemProperties().maxStackSize(64));

    GUNPOWDER = &registry.registerItem(ResourceLocation("minecraft:gunpowder"), ItemProperties().maxStackSize(64));

    LEATHER = &registry.registerItem(ResourceLocation("minecraft:leather"), ItemProperties().maxStackSize(64));

    SLIME_BALL = &registry.registerItem(ResourceLocation("minecraft:slime_ball"), ItemProperties().maxStackSize(64));

    // EGG 已在 registerThrowableItems() 中注册为 EggItem

    COMPASS = &registry.registerItem(ResourceLocation("minecraft:compass"), ItemProperties().maxStackSize(64));

    CLOCK = &registry.registerItem(ResourceLocation("minecraft:clock"), ItemProperties().maxStackSize(64));

    // SPIDER_EYE 已在 registerFood() 中注册

    FERMENTED_SPIDER_EYE =
        &registry.registerItem(ResourceLocation("minecraft:fermented_spider_eye"), ItemProperties().maxStackSize(64));

    BLAZE_ROD = &registry.registerItem(ResourceLocation("minecraft:blaze_rod"), ItemProperties().maxStackSize(64));

    BLAZE_POWDER =
        &registry.registerItem(ResourceLocation("minecraft:blaze_powder"), ItemProperties().maxStackSize(64));

    // ENDER_PEARL 已在 registerThrowableItems() 中注册为 EnderPearlItem

    ENDER_EYE = &registry.registerItem(ResourceLocation("minecraft:ender_eye"), ItemProperties().maxStackSize(64));

    NETHER_STAR = &registry.registerItem(
        ResourceLocation("minecraft:nether_star"), ItemProperties().maxStackSize(64).rarity(ItemRarity::Uncommon));

    FIRE_CHARGE = &registry.registerItem(ResourceLocation("minecraft:fire_charge"), ItemProperties().maxStackSize(64));

    FIREWORK_STAR =
        &registry.registerItem(ResourceLocation("minecraft:firework_star"), ItemProperties().maxStackSize(64));

    FIREWORK_ROCKET =
        &registry.registerItem(ResourceLocation("minecraft:firework_rocket"), ItemProperties().maxStackSize(64));
}

void Items::registerDyes()
{
    auto& registry = ItemRegistry::instance();

    INK_SAC = &registry.registerItem(ResourceLocation("minecraft:ink_sac"), ItemProperties().maxStackSize(64));

    RED_DYE = &registry.registerItem(ResourceLocation("minecraft:red_dye"), ItemProperties().maxStackSize(64));

    GREEN_DYE = &registry.registerItem(ResourceLocation("minecraft:green_dye"), ItemProperties().maxStackSize(64));

    COCOA_BEANS = &registry.registerItem(ResourceLocation("minecraft:cocoa_beans"), ItemProperties().maxStackSize(64));

    LAPIS_LAZULI_DYE =
        &registry.registerItem(ResourceLocation("minecraft:lapis_lazuli_dye"), ItemProperties().maxStackSize(64));

    PURPLE_DYE = &registry.registerItem(ResourceLocation("minecraft:purple_dye"), ItemProperties().maxStackSize(64));

    CYAN_DYE = &registry.registerItem(ResourceLocation("minecraft:cyan_dye"), ItemProperties().maxStackSize(64));

    LIGHT_GRAY_DYE =
        &registry.registerItem(ResourceLocation("minecraft:light_gray_dye"), ItemProperties().maxStackSize(64));

    GRAY_DYE = &registry.registerItem(ResourceLocation("minecraft:gray_dye"), ItemProperties().maxStackSize(64));

    PINK_DYE = &registry.registerItem(ResourceLocation("minecraft:pink_dye"), ItemProperties().maxStackSize(64));

    LIME_DYE = &registry.registerItem(ResourceLocation("minecraft:lime_dye"), ItemProperties().maxStackSize(64));

    YELLOW_DYE = &registry.registerItem(ResourceLocation("minecraft:yellow_dye"), ItemProperties().maxStackSize(64));

    LIGHT_BLUE_DYE =
        &registry.registerItem(ResourceLocation("minecraft:light_blue_dye"), ItemProperties().maxStackSize(64));

    MAGENTA_DYE = &registry.registerItem(ResourceLocation("minecraft:magenta_dye"), ItemProperties().maxStackSize(64));

    ORANGE_DYE = &registry.registerItem(ResourceLocation("minecraft:orange_dye"), ItemProperties().maxStackSize(64));

    WHITE_DYE = &registry.registerItem(ResourceLocation("minecraft:white_dye"), ItemProperties().maxStackSize(64));
}

void Items::registerSeeds()
{
    auto& registry = ItemRegistry::instance();

    WHEAT_SEEDS = &registry.registerItem(ResourceLocation("minecraft:wheat_seeds"), ItemProperties().maxStackSize(64));

    PUMPKIN_SEEDS =
        &registry.registerItem(ResourceLocation("minecraft:pumpkin_seeds"), ItemProperties().maxStackSize(64));

    MELON_SEEDS = &registry.registerItem(ResourceLocation("minecraft:melon_seeds"), ItemProperties().maxStackSize(64));

    BEETROOT_SEEDS =
        &registry.registerItem(ResourceLocation("minecraft:beetroot_seeds"), ItemProperties().maxStackSize(64));
}

void Items::registerCrops()
{
    auto& registry = ItemRegistry::instance();

    WHEAT = &registry.registerItem(ResourceLocation("minecraft:wheat"), ItemProperties().maxStackSize(64));

    PUMPKIN = &registry.registerItem(ResourceLocation("minecraft:pumpkin"), ItemProperties().maxStackSize(64));

    MELON = &registry.registerItem(ResourceLocation("minecraft:melon"), ItemProperties().maxStackSize(64));

    // 注意：MELON_SLICE, CARROT, POTATO, BEETROOT 已在 registerFood() 中注册为食物
    // 这里不再重复注册

    SUGAR_CANE = &registry.registerItem(ResourceLocation("minecraft:sugar_cane"), ItemProperties().maxStackSize(64));

    SUGAR = &registry.registerItem(ResourceLocation("minecraft:sugar"), ItemProperties().maxStackSize(64));
}

void Items::registerAquaticMaterials()
{
    auto& registry = ItemRegistry::instance();

    // 鳞甲 - 海龟掉落，用于合成海龟壳
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    SCUTE = &registry.registerItem(ResourceLocation("minecraft:scute"), ItemProperties().maxStackSize(64));

    // 海洋之心 - 宝藏物品，用于合成潮涌核心
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS).rarity(Rarity.UNCOMMON))
    HEART_OF_THE_SEA = &registry.registerItem(
        ResourceLocation("minecraft:heart_of_the_sea"), ItemProperties().maxStackSize(64).rarity(ItemRarity::Uncommon));

    // 鹦鹉螺壳 - 溺尸掉落或钓鱼获得，用于合成潮涌核心
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    NAUTILUS_SHELL =
        &registry.registerItem(ResourceLocation("minecraft:nautilus_shell"), ItemProperties().maxStackSize(64));

    // 幻翼膜 - 幻翼掉落，用于修复鞘翅和酿造缓降药水
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    PHANTOM_MEMBRANE =
        &registry.registerItem(ResourceLocation("minecraft:phantom_membrane"), ItemProperties().maxStackSize(64));

    // 注意：DRIED_KELP 已在 registerFood() 中注册为食物
}

void Items::registerBrewingIngredients()
{
    auto& registry = ItemRegistry::instance();

    // 地狱疣 - 酿造基础材料，用于制作尴尬的药水
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    NETHER_WART = &registry.registerItem(ResourceLocation("minecraft:nether_wart"), ItemProperties().maxStackSize(64));

    // 注意：GOLDEN_CARROT 已在 registerFood() 中注册为食物
    // 注意：PUFFERFISH 已在 registerFood() 中注册为食物

    // 恶魂之泪 - 生命恢复药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    GHAST_TEAR = &registry.registerItem(ResourceLocation("minecraft:ghast_tear"), ItemProperties().maxStackSize(64));

    // 兔子脚 - 跳跃药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    RABBIT_FOOT = &registry.registerItem(ResourceLocation("minecraft:rabbit_foot"), ItemProperties().maxStackSize(64));

    // 岩浆膏 - 防火药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    MAGMA_CREAM = &registry.registerItem(ResourceLocation("minecraft:magma_cream"), ItemProperties().maxStackSize(64));

    // 龙息 - 滞留药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS).rarity(Rarity.UNCOMMON))
    DRAGON_BREATH = &registry.registerItem(
        ResourceLocation("minecraft:dragon_breath"), ItemProperties().maxStackSize(64).rarity(ItemRarity::Uncommon));

    // 海龟壳 - 海龟大师药水材料（装备，但也可用于酿造）
    // 参考: new ArmorItem(ArmorMaterial.TURTLE, EquipmentSlotType.HEAD, new Item.Properties().group(ItemGroup.COMBAT))
    TURTLE_HELMET = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:turtle_helmet"),
        item::armor::ArmorMaterials::TURTLE,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::TURTLE.getDurability(item::armor::ArmorSlot::Head)));

    // 闪烁的西瓜片 - 瞬间治疗药水材料
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    GLISTERING_MELON_SLICE =
        &registry.registerItem(ResourceLocation("minecraft:glistering_melon_slice"), ItemProperties().maxStackSize(64));
}

void Items::registerPotions()
{
    auto& registry = ItemRegistry::instance();

    // 玻璃瓶 - 用于装水和酿造
    // 参考: new GlassBottleItem(new Item.Properties().group(ItemGroup.BREWING))
    GLASS_BOTTLE =
        &registry.registerItem(ResourceLocation("minecraft:glass_bottle"), ItemProperties().maxStackSize(64));

    // 药水 - 饮用药水
    // 参考: new PotionItem(new Item.Properties().group(ItemGroup.BREWING))
    POTION = &registry.registerItem<item::PotionItem>(
        ResourceLocation("minecraft:potion"), ItemProperties().maxStackSize(1));

    // 喷溅药水 - 投掷药水
    // 参考: new SplashPotionItem(new Item.Properties().group(ItemGroup.BREWING))
    SPLASH_POTION = &registry.registerItem<item::SplashPotionItem>(
        ResourceLocation("minecraft:splash_potion"), ItemProperties().maxStackSize(1));

    // 滞留药水 - 留下效果云
    // 参考: new LingeringPotionItem(new Item.Properties().group(ItemGroup.BREWING))
    LINGERING_POTION = &registry.registerItem<item::LingeringPotionItem>(
        ResourceLocation("minecraft:lingering_potion"), ItemProperties().maxStackSize(1));
}

void Items::registerWeapons()
{
    auto& registry = ItemRegistry::instance();

    // 弓 - 远程武器
    // 参考: new BowItem(new Item.Properties().maxDamage(384))
    BOW = &registry.registerItem<item::BowItem>(ResourceLocation("minecraft:bow"), ItemProperties().maxDamage(384));

    // 箭矢 - 弹药
    // 参考: new ArrowItem(new Item.Properties().maxStackSize(64))
    ARROW =
        &registry.registerItem<item::ArrowItem>(ResourceLocation("minecraft:arrow"), ItemProperties().maxStackSize(64));

    // 光灵箭 - 带发光效果（仅创造模式）
    // 参考: new SpectralArrowItem(new Item.Properties().maxStackSize(64))
    SPECTRAL_ARROW = &registry.registerItem<item::ArrowItem>(
        ResourceLocation("minecraft:spectral_arrow"), ItemProperties().maxStackSize(64));

    // 药水箭 - 带药水效果
    // 参考: new TippedArrowItem(new Item.Properties().maxStackSize(64))
    TIPPED_ARROW = &registry.registerItem<item::TippedArrowItem>(
        ResourceLocation("minecraft:tipped_arrow"), ItemProperties().maxStackSize(64));

    // 弩 - 可装填的远程武器
    // 参考: new CrossbowItem(new Item.Properties().maxDamage(326))
    CROSSBOW = &registry.registerItem<item::CrossbowItem>(
        ResourceLocation("minecraft:crossbow"), ItemProperties().maxDamage(326));

    // 三叉戟 - 近战和远程结合的武器
    // 参考: new TridentItem(new Item.Properties().maxDamage(250))
    TRIDENT = &registry.registerItem<item::TridentItem>(
        ResourceLocation("minecraft:trident"), ItemProperties().maxDamage(250));

    // 盾牌 - 格挡武器
    // 参考: new ShieldItem(new Item.Properties().maxDamage(336))
    SHIELD =
        &registry.registerItem<item::ShieldItem>(ResourceLocation("minecraft:shield"), ItemProperties().maxDamage(336));

    // 钓鱼竿 - 钓鱼工具
    // 参考: new FishingRodItem(new Item.Properties().maxDamage(64))
    FISHING_ROD = &registry.registerItem<item::FishingRodItem>(
        ResourceLocation("minecraft:fishing_rod"), ItemProperties().maxDamage(64));
}

void Items::registerThrowables()
{
    auto& registry = ItemRegistry::instance();

    // 雪球 - 投掷物品
    // 参考: new SnowballItem(new Item.Properties().maxStackSize(16))
    SNOWBALL = &registry.registerItem<item::SnowballItem>(
        ResourceLocation("minecraft:snowball"), ItemProperties().maxStackSize(16));

    // 鸡蛋 - 投掷物品，有概率孵化小鸡
    // 参考: new EggItem(new Item.Properties().maxStackSize(16))
    EGG = &registry.registerItem<item::EggItem>(ResourceLocation("minecraft:egg"), ItemProperties().maxStackSize(16));

    // 末影珍珠 - 投掷后传送
    // 参考: new EnderPearlItem(new Item.Properties().maxStackSize(16))
    ENDER_PEARL = &registry.registerItem<item::EnderPearlItem>(
        ResourceLocation("minecraft:ender_pearl"), ItemProperties().maxStackSize(16));

    // 附魔之瓶 - 投掷后释放经验
    // 参考: new ExperienceBottleItem(new Item.Properties().maxStackSize(64))
    EXPERIENCE_BOTTLE = &registry.registerItem<item::ExperienceBottleItem>(
        ResourceLocation("minecraft:experience_bottle"), ItemProperties().maxStackSize(64));
}

void Items::registerBuckets()
{
    auto& registry = ItemRegistry::instance();

    // 空桶 - 用于装水/岩浆/牛奶
    // 参考: new BucketItem((Fluid)null, new Item.Properties().maxStackSize(16))
    BUCKET = &registry.registerItem<BucketItem>(ResourceLocation("minecraft:bucket"),
        nullptr, // 空桶没有流体
        ItemProperties().maxStackSize(16));

    // 水桶 - 装满水的桶
    // 参考: new BucketItem(Fluids.WATER, new Item.Properties().maxStackSize(1).containerItem(BUCKET))
    // 水桶使用后返回空桶，所以 containerItem 设为 BUCKET
    WATER_BUCKET = &registry.registerItem<BucketItem>(ResourceLocation("minecraft:water_bucket"),
        fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID),
        ItemProperties().maxStackSize(1).containerItem(BUCKET));

    // 岩浆桶 - 装满岩浆的桶
    // 参考: new BucketItem(Fluids.LAVA, new Item.Properties().maxStackSize(1).containerItem(BUCKET))
    // 岩浆桶作为燃料使用后返回空桶
    LAVA_BUCKET = &registry.registerItem<BucketItem>(ResourceLocation("minecraft:lava_bucket"),
        fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::LAVA_ID),
        ItemProperties().maxStackSize(1).containerItem(BUCKET));

    // 鱼桶 - MC 1.16.5
    // 鳕鱼桶 - 可以装鳕鱼的水桶，使用后返回空桶
    COD_BUCKET = &registry.registerItem<item::FishBucketItem>(ResourceLocation("minecraft:cod_bucket"),
        mc::entity::EntityTypes::COD,
        ItemProperties().maxStackSize(1).containerItem(BUCKET));

    // 鲑鱼桶
    SALMON_BUCKET = &registry.registerItem<item::FishBucketItem>(ResourceLocation("minecraft:salmon_bucket"),
        mc::entity::EntityTypes::SALMON,
        ItemProperties().maxStackSize(1).containerItem(BUCKET));

    // 河豚桶
    PUFFERFISH_BUCKET = &registry.registerItem<item::FishBucketItem>(ResourceLocation("minecraft:pufferfish_bucket"),
        mc::entity::EntityTypes::PUFFERFISH,
        ItemProperties().maxStackSize(1).containerItem(BUCKET));

    // 热带鱼桶
    TROPICAL_FISH_BUCKET =
        &registry.registerItem<item::FishBucketItem>(ResourceLocation("minecraft:tropical_fish_bucket"),
            mc::entity::EntityTypes::TROPICAL_FISH,
            ItemProperties().maxStackSize(1).containerItem(BUCKET));

    // 牛奶桶 - 清除所有药水效果
    // 参考: new MilkBucketItem(new Item.Properties().containerItem(BUCKET))
    MILK_BUCKET = &registry.registerItem<item::special::MilkBucketItem>(
        ResourceLocation("minecraft:milk_bucket"), ItemProperties().maxStackSize(1).containerItem(BUCKET));
}

void Items::registerBooks()
{
    auto& registry = ItemRegistry::instance();

    // 书 - 可用于合成书架、附魔台
    BOOK = &registry.registerItem(ResourceLocation("minecraft:book"), ItemProperties().maxStackSize(64));

    // 附魔书 - 存储附魔，可在铁砧中应用到物品
    ENCHANTED_BOOK = &registry.registerItem<item::items::EnchantedBookItem>(
        ResourceLocation("minecraft:enchanted_book"), ItemProperties().maxStackSize(1));

    // 书与笔 - 可写入内容
    WRITABLE_BOOK =
        &registry.registerItem(ResourceLocation("minecraft:writable_book"), ItemProperties().maxStackSize(1));

    // 成书 - 已完成的书
    WRITTEN_BOOK =
        &registry.registerItem(ResourceLocation("minecraft:written_book"), ItemProperties().maxStackSize(16));
}

void Items::registerSponges()
{
    auto& registry = ItemRegistry::instance();

    // 海绵（干燥）- 吸水后变成湿海绵
    SPONGE = &registerBlockBackedItem(registry, VanillaBlocks::SPONGE, "sponge", ItemProperties().maxStackSize(64));

    // 湿海绵 - 在熔炉中干燥后返回海绵
    WET_SPONGE =
        &registerBlockBackedItem(registry, VanillaBlocks::WET_SPONGE, "wet_sponge", ItemProperties().maxStackSize(64));
}

void Items::registerMinecarts()
{
    auto& registry = ItemRegistry::instance();

    // 普通矿车 - MC 1.16.5: maxStackSize = 1
    MINECART = &registry.registerItem<item::MinecartItem>(ResourceLocation("minecraft:minecart"),
        entity::AbstractMinecartEntity::Type::Rideable,
        ItemProperties().maxStackSize(1));

    // 箱子矿车
    CHEST_MINECART = &registry.registerItem<item::MinecartItem>(ResourceLocation("minecraft:chest_minecart"),
        entity::AbstractMinecartEntity::Type::Chest,
        ItemProperties().maxStackSize(1));

    // 熔炉矿车
    FURNACE_MINECART = &registry.registerItem<item::MinecartItem>(ResourceLocation("minecraft:furnace_minecart"),
        entity::AbstractMinecartEntity::Type::Furnace,
        ItemProperties().maxStackSize(1));

    // TNT矿车
    TNT_MINECART = &registry.registerItem<item::MinecartItem>(ResourceLocation("minecraft:tnt_minecart"),
        entity::AbstractMinecartEntity::Type::TNT,
        ItemProperties().maxStackSize(1));

    // 漏斗矿车
    HOPPER_MINECART = &registry.registerItem<item::MinecartItem>(ResourceLocation("minecraft:hopper_minecart"),
        entity::AbstractMinecartEntity::Type::Hopper,
        ItemProperties().maxStackSize(1));

    // 命令方块矿车
    COMMAND_BLOCK_MINECART =
        &registry.registerItem<item::MinecartItem>(ResourceLocation("minecraft:command_block_minecart"),
            entity::AbstractMinecartEntity::Type::CommandBlock,
            ItemProperties().maxStackSize(1));
}

void Items::registerHangingItems()
{
    auto& registry = ItemRegistry::instance();

    // 画作 - MC 1.16.5: maxStackSize = 16
    PAINTING = &registry.registerItem(ResourceLocation("minecraft:painting"), ItemProperties().maxStackSize(16));

    // 物品展示框 - MC 1.16.5: maxStackSize = 16
    ITEM_FRAME = &registry.registerItem(ResourceLocation("minecraft:item_frame"), ItemProperties().maxStackSize(16));

    // 拴绳 - MC 1.16.5: maxStackSize = 16
    LEAD = &registry.registerItem(ResourceLocation("minecraft:lead"), ItemProperties().maxStackSize(16));
}

} // namespace mc
