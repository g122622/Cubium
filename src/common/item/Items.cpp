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

#include "Items.hpp"

#include "common/entity/core/EntityRegistry.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/food/Foods.hpp"
#include "common/item/items/BannerPatternItem.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/item/items/armor/HorseArmorItem.hpp"
#include "common/item/items/block/BannerItem.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/WallOrFloorItem.hpp"
#include "common/item/items/food/ChorusFruitItem.hpp"
#include "common/item/items/food/FoodItem.hpp"
#include "common/item/items/food/GoldenAppleItem.hpp"
#include "common/item/items/food/HoneyBottleItem.hpp"
#include "common/item/items/map/EmptyMapItem.hpp"
#include "common/item/items/map/FilledMapItem.hpp"
#include "common/item/items/potion/LingeringPotionItem.hpp"
#include "common/item/items/potion/PotionItem.hpp"
#include "common/item/items/potion/SplashPotionItem.hpp"
#include "common/item/items/special/BoneMealItem.hpp"
#include "common/item/items/special/BucketItem.hpp"
#include "common/item/items/special/EnchantedBookItem.hpp"
#include "common/item/items/special/FishBucketItem.hpp"
#include "common/item/items/special/FlintAndSteelItem.hpp"
#include "common/item/items/special/MilkBucketItem.hpp"
#include "common/item/items/special/NameTagItem.hpp"
#include "common/item/items/special/SaddleItem.hpp"
#include "common/item/items/special/StickItems.hpp"
#include "common/item/items/tool/AxeItem.hpp"
#include "common/item/items/tool/HoeItem.hpp"
#include "common/item/items/tool/PickaxeItem.hpp"
#include "common/item/items/tool/ShearsItem.hpp"
#include "common/item/items/tool/ShovelItem.hpp"
#include "common/item/items/tool/SwordItem.hpp"
#include "common/item/items/trial/MaceItem.hpp"
#include "common/item/items/trial/OminousBottleItem.hpp"
#include "common/item/items/trial/OminousTrialKeyItem.hpp"
#include "common/item/items/trial/TrialKeyItem.hpp"
#include "common/item/items/trial/WindChargeItem.hpp"
#include "common/item/items/vehicle/BoatItem.hpp"
#include "common/item/items/vehicle/MinecartItem.hpp"
#include "common/item/items/weapon/ArrowItem.hpp"
#include "common/item/items/weapon/BowItem.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/item/items/weapon/FishingRodItem.hpp"
#include "common/item/items/weapon/ShieldItem.hpp"
#include "common/item/items/weapon/ThrowableItem.hpp"
#include "common/item/items/weapon/ThrowableItems.hpp"
#include "common/item/items/weapon/TippedArrowItem.hpp"
#include "common/item/items/weapon/TridentItem.hpp"
#include "common/item/tier/ItemTiers.hpp"
#include "common/world/block/blocks/functional/CompostableItems.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"

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

// 马铠
Item* Items::LEATHER_HORSE_ARMOR = nullptr;
Item* Items::IRON_HORSE_ARMOR = nullptr;
Item* Items::GOLDEN_HORSE_ARMOR = nullptr;
Item* Items::DIAMOND_HORSE_ARMOR = nullptr;

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
Item* Items::CAKE = nullptr;
Item* Items::RABBIT_STEW = nullptr;
Item* Items::ROTTEN_FLESH = nullptr;
Item* Items::SPIDER_EYE = nullptr;
Item* Items::SUSPICIOUS_STEW = nullptr;
Item* Items::SWEET_BERRIES = nullptr;
Item* Items::GLOW_BERRIES = nullptr;
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
Item* Items::MAP = nullptr;
Item* Items::FILLED_MAP = nullptr;
Item* Items::PAPER = nullptr;
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
Item* Items::HAY_BLOCK = nullptr;
Item* Items::PUMPKIN = nullptr;
Item* Items::MELON = nullptr;
// MELON_SLICE 已在食物部分声明
// CARROT 已在食物部分声明
// POTATO 已在食物部分声明
// BEETROOT 已在食物部分声明
Item* Items::CACTUS = nullptr;
Item* Items::LILY_PAD = nullptr;
Item* Items::VINE = nullptr;
Item* Items::SUGAR_CANE = nullptr;
Item* Items::SUGAR = nullptr;
Item* Items::BAMBOO = nullptr;

// 下界材料
Item* Items::CRIMSON_FUNGUS = nullptr;
Item* Items::WARPED_FUNGUS = nullptr;

// 水域更新材料
Item* Items::SCUTE = nullptr;
Item* Items::HEART_OF_THE_SEA = nullptr;
Item* Items::NAUTILUS_SHELL = nullptr;
Item* Items::PHANTOM_MEMBRANE = nullptr;
Item* Items::DRIED_KELP_BLOCK = nullptr;
Item* Items::SEA_PICKLE = nullptr;
Item* Items::KELP = nullptr;
Item* Items::SEAGRASS = nullptr;
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
// 骑乘控制物品
// ============================================================================
Item* Items::CARROT_ON_A_STICK = nullptr;
Item* Items::WARPED_FUNGUS_ON_A_STICK = nullptr;

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
Item* Items::AXOLOTL_BUCKET = nullptr;
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
// 船（6种木材类型）
// ============================================================================
Item* Items::OAK_BOAT = nullptr;
Item* Items::SPRUCE_BOAT = nullptr;
Item* Items::BIRCH_BOAT = nullptr;
Item* Items::JUNGLE_BOAT = nullptr;
Item* Items::ACACIA_BOAT = nullptr;
Item* Items::DARK_OAK_BOAT = nullptr;

// ============================================================================
// 悬挂实体物品
// ============================================================================
Item* Items::PAINTING = nullptr;
Item* Items::ITEM_FRAME = nullptr;
Item* Items::LEAD = nullptr;

// ============================================================================
// 告示牌物品（8种木材类型）
// ============================================================================
Item* Items::OAK_SIGN = nullptr;
Item* Items::SPRUCE_SIGN = nullptr;
Item* Items::BIRCH_SIGN = nullptr;
Item* Items::JUNGLE_SIGN = nullptr;
Item* Items::ACACIA_SIGN = nullptr;
Item* Items::DARK_OAK_SIGN = nullptr;
Item* Items::CRIMSON_SIGN = nullptr;
Item* Items::WARPED_SIGN = nullptr;

// 基础建筑方块
Item* Items::DIRT = nullptr;
Item* Items::GRASS_BLOCK = nullptr;
Item* Items::SAND = nullptr;
Item* Items::GRAVEL = nullptr;
Item* Items::BEDROCK = nullptr;
Item* Items::OBSIDIAN = nullptr;
Item* Items::NETHERRACK = nullptr;
Item* Items::GLOWSTONE = nullptr;
Item* Items::END_STONE = nullptr;
Item* Items::ICE = nullptr;
Item* Items::CLAY = nullptr;
Item* Items::SNOW = nullptr;
Item* Items::SNOW_BLOCK = nullptr;
Item* Items::TERRACOTTA = nullptr;
Item* Items::BRICKS = nullptr;
Item* Items::BOOKSHELF = nullptr;
Item* Items::BONE_BLOCK = nullptr;
Item* Items::SLIME_BLOCK = nullptr;
Item* Items::HONEY_BLOCK = nullptr;
Item* Items::RED_SAND = nullptr;
Item* Items::COBWEB = nullptr;
Item* Items::FARMLAND = nullptr;
Item* Items::GRASS_PATH = nullptr;
Item* Items::MYCELIUM = nullptr;
Item* Items::PACKED_ICE = nullptr;
Item* Items::BLUE_ICE = nullptr;
Item* Items::COARSE_DIRT = nullptr;
Item* Items::PODZOL = nullptr;

// 石头变种
Item* Items::GRANITE = nullptr;
Item* Items::POLISHED_GRANITE = nullptr;
Item* Items::DIORITE = nullptr;
Item* Items::POLISHED_DIORITE = nullptr;
Item* Items::ANDESITE = nullptr;
Item* Items::POLISHED_ANDESITE = nullptr;

// 砂岩
Item* Items::SANDSTONE = nullptr;
Item* Items::CHISELED_SANDSTONE = nullptr;
Item* Items::CUT_SANDSTONE = nullptr;
Item* Items::RED_SANDSTONE = nullptr;

// 矿物方块
Item* Items::DIAMOND_BLOCK = nullptr;
Item* Items::COAL_BLOCK = nullptr;
Item* Items::GOLD_BLOCK = nullptr;
Item* Items::IRON_BLOCK = nullptr;
Item* Items::LAPIS_BLOCK = nullptr;
Item* Items::EMERALD_BLOCK = nullptr;
Item* Items::REDSTONE_BLOCK = nullptr;
Item* Items::NETHERITE_BLOCK = nullptr;

// 下界方块
Item* Items::SOUL_SAND = nullptr;
Item* Items::SOUL_SOIL = nullptr;
Item* Items::BASALT = nullptr;
Item* Items::POLISHED_BASALT = nullptr;
Item* Items::BLACKSTONE = nullptr;
Item* Items::POLISHED_BLACKSTONE = nullptr;
Item* Items::CRYING_OBSIDIAN = nullptr;
Item* Items::MAGMA = nullptr;
Item* Items::NETHER_WART_BLOCK = nullptr;
Item* Items::WARPED_WART_BLOCK = nullptr;
Item* Items::CRIMSON_STEM = nullptr;
Item* Items::WARPED_STEM = nullptr;
Item* Items::CRIMSON_NYLIUM = nullptr;
Item* Items::WARPED_NYLIUM = nullptr;
Item* Items::CRIMSON_HYPHAE = nullptr;
Item* Items::WARPED_HYPHAE = nullptr;
Item* Items::STRIPPED_CRIMSON_STEM = nullptr;
Item* Items::STRIPPED_WARPED_STEM = nullptr;
Item* Items::STRIPPED_CRIMSON_HYPHAE = nullptr;
Item* Items::STRIPPED_WARPED_HYPHAE = nullptr;
Item* Items::SHROOMLIGHT = nullptr;
Item* Items::WEEPING_VINES = nullptr;
Item* Items::TWISTING_VINES = nullptr;
Item* Items::CRIMSON_ROOTS = nullptr;
Item* Items::WARPED_ROOTS = nullptr;
Item* Items::NETHER_SPROUTS = nullptr;
Item* Items::DEAD_BUSH = nullptr;

// 木材和去皮原木
Item* Items::OAK_WOOD = nullptr;
Item* Items::SPRUCE_WOOD = nullptr;
Item* Items::BIRCH_WOOD = nullptr;
Item* Items::JUNGLE_WOOD = nullptr;
Item* Items::ACACIA_WOOD = nullptr;
Item* Items::DARK_OAK_WOOD = nullptr;
Item* Items::STRIPPED_OAK_LOG = nullptr;
Item* Items::STRIPPED_SPRUCE_LOG = nullptr;
Item* Items::STRIPPED_BIRCH_LOG = nullptr;
Item* Items::STRIPPED_JUNGLE_LOG = nullptr;
Item* Items::STRIPPED_ACACIA_LOG = nullptr;
Item* Items::STRIPPED_DARK_OAK_LOG = nullptr;
Item* Items::STRIPPED_OAK_WOOD = nullptr;
Item* Items::STRIPPED_SPRUCE_WOOD = nullptr;
Item* Items::STRIPPED_BIRCH_WOOD = nullptr;
Item* Items::STRIPPED_JUNGLE_WOOD = nullptr;
Item* Items::STRIPPED_ACACIA_WOOD = nullptr;
Item* Items::STRIPPED_DARK_OAK_WOOD = nullptr;

// 树叶
Item* Items::OAK_LEAVES = nullptr;
Item* Items::SPRUCE_LEAVES = nullptr;
Item* Items::BIRCH_LEAVES = nullptr;
Item* Items::JUNGLE_LEAVES = nullptr;
Item* Items::ACACIA_LEAVES = nullptr;
Item* Items::DARK_OAK_LEAVES = nullptr;

// 树苗
Item* Items::OAK_SAPLING = nullptr;
Item* Items::SPRUCE_SAPLING = nullptr;
Item* Items::BIRCH_SAPLING = nullptr;
Item* Items::JUNGLE_SAPLING = nullptr;
Item* Items::ACACIA_SAPLING = nullptr;
Item* Items::DARK_OAK_SAPLING = nullptr;

// 植被和花
Item* Items::SHORT_GRASS = nullptr;
Item* Items::TALL_GRASS = nullptr;
Item* Items::FERN = nullptr;
Item* Items::LARGE_FERN = nullptr;
Item* Items::DANDELION = nullptr;
Item* Items::POPPY = nullptr;
Item* Items::BLUE_ORCHID = nullptr;
Item* Items::ALLIUM = nullptr;
Item* Items::AZURE_BLUET = nullptr;
Item* Items::RED_TULIP = nullptr;
Item* Items::ORANGE_TULIP = nullptr;
Item* Items::WHITE_TULIP = nullptr;
Item* Items::PINK_TULIP = nullptr;
Item* Items::OXEYE_DAISY = nullptr;
Item* Items::LILY_OF_THE_VALLEY = nullptr;
Item* Items::SUNFLOWER = nullptr;
Item* Items::LILAC = nullptr;
Item* Items::ROSE_BUSH = nullptr;
Item* Items::PEONY = nullptr;
Item* Items::CORNFLOWER = nullptr;
Item* Items::WITHER_ROSE = nullptr;
Item* Items::BROWN_MUSHROOM = nullptr;
Item* Items::RED_MUSHROOM = nullptr;
Item* Items::BROWN_MUSHROOM_BLOCK = nullptr;
Item* Items::RED_MUSHROOM_BLOCK = nullptr;
Item* Items::MUSHROOM_STEM = nullptr;

// 羊毛 (16色)
Item* Items::WHITE_WOOL = nullptr;
Item* Items::ORANGE_WOOL = nullptr;
Item* Items::MAGENTA_WOOL = nullptr;
Item* Items::LIGHT_BLUE_WOOL = nullptr;
Item* Items::YELLOW_WOOL = nullptr;
Item* Items::LIME_WOOL = nullptr;
Item* Items::PINK_WOOL = nullptr;
Item* Items::GRAY_WOOL = nullptr;
Item* Items::LIGHT_GRAY_WOOL = nullptr;
Item* Items::CYAN_WOOL = nullptr;
Item* Items::PURPLE_WOOL = nullptr;
Item* Items::BLUE_WOOL = nullptr;
Item* Items::BROWN_WOOL = nullptr;
Item* Items::GREEN_WOOL = nullptr;
Item* Items::RED_WOOL = nullptr;
Item* Items::BLACK_WOOL = nullptr;

// 地毯 (16色)
Item* Items::WHITE_CARPET = nullptr;
Item* Items::ORANGE_CARPET = nullptr;
Item* Items::MAGENTA_CARPET = nullptr;
Item* Items::LIGHT_BLUE_CARPET = nullptr;
Item* Items::YELLOW_CARPET = nullptr;
Item* Items::LIME_CARPET = nullptr;
Item* Items::PINK_CARPET = nullptr;
Item* Items::GRAY_CARPET = nullptr;
Item* Items::LIGHT_GRAY_CARPET = nullptr;
Item* Items::CYAN_CARPET = nullptr;
Item* Items::PURPLE_CARPET = nullptr;
Item* Items::BLUE_CARPET = nullptr;
Item* Items::BROWN_CARPET = nullptr;
Item* Items::GREEN_CARPET = nullptr;
Item* Items::RED_CARPET = nullptr;
Item* Items::BLACK_CARPET = nullptr;

// 染色玻璃 (16色)
Item* Items::WHITE_STAINED_GLASS = nullptr;
Item* Items::ORANGE_STAINED_GLASS = nullptr;
Item* Items::MAGENTA_STAINED_GLASS = nullptr;
Item* Items::LIGHT_BLUE_STAINED_GLASS = nullptr;
Item* Items::YELLOW_STAINED_GLASS = nullptr;
Item* Items::LIME_STAINED_GLASS = nullptr;
Item* Items::PINK_STAINED_GLASS = nullptr;
Item* Items::GRAY_STAINED_GLASS = nullptr;
Item* Items::LIGHT_GRAY_STAINED_GLASS = nullptr;
Item* Items::CYAN_STAINED_GLASS = nullptr;
Item* Items::PURPLE_STAINED_GLASS = nullptr;
Item* Items::BLUE_STAINED_GLASS = nullptr;
Item* Items::BROWN_STAINED_GLASS = nullptr;
Item* Items::GREEN_STAINED_GLASS = nullptr;
Item* Items::RED_STAINED_GLASS = nullptr;
Item* Items::BLACK_STAINED_GLASS = nullptr;

// 混凝土 (16色)
Item* Items::WHITE_CONCRETE = nullptr;
Item* Items::ORANGE_CONCRETE = nullptr;
Item* Items::MAGENTA_CONCRETE = nullptr;
Item* Items::LIGHT_BLUE_CONCRETE = nullptr;
Item* Items::YELLOW_CONCRETE = nullptr;
Item* Items::LIME_CONCRETE = nullptr;
Item* Items::PINK_CONCRETE = nullptr;
Item* Items::GRAY_CONCRETE = nullptr;
Item* Items::LIGHT_GRAY_CONCRETE = nullptr;
Item* Items::CYAN_CONCRETE = nullptr;
Item* Items::PURPLE_CONCRETE = nullptr;
Item* Items::BLUE_CONCRETE = nullptr;
Item* Items::BROWN_CONCRETE = nullptr;
Item* Items::GREEN_CONCRETE = nullptr;
Item* Items::RED_CONCRETE = nullptr;
Item* Items::BLACK_CONCRETE = nullptr;

// 混凝土粉末 (16色)
Item* Items::WHITE_CONCRETE_POWDER = nullptr;
Item* Items::ORANGE_CONCRETE_POWDER = nullptr;
Item* Items::MAGENTA_CONCRETE_POWDER = nullptr;
Item* Items::LIGHT_BLUE_CONCRETE_POWDER = nullptr;
Item* Items::YELLOW_CONCRETE_POWDER = nullptr;
Item* Items::LIME_CONCRETE_POWDER = nullptr;
Item* Items::PINK_CONCRETE_POWDER = nullptr;
Item* Items::GRAY_CONCRETE_POWDER = nullptr;
Item* Items::LIGHT_GRAY_CONCRETE_POWDER = nullptr;
Item* Items::CYAN_CONCRETE_POWDER = nullptr;
Item* Items::PURPLE_CONCRETE_POWDER = nullptr;
Item* Items::BLUE_CONCRETE_POWDER = nullptr;
Item* Items::BROWN_CONCRETE_POWDER = nullptr;
Item* Items::GREEN_CONCRETE_POWDER = nullptr;
Item* Items::RED_CONCRETE_POWDER = nullptr;
Item* Items::BLACK_CONCRETE_POWDER = nullptr;

// 陶瓦 (16色)
Item* Items::WHITE_TERRACOTTA = nullptr;
Item* Items::ORANGE_TERRACOTTA = nullptr;
Item* Items::MAGENTA_TERRACOTTA = nullptr;
Item* Items::LIGHT_BLUE_TERRACOTTA = nullptr;
Item* Items::YELLOW_TERRACOTTA = nullptr;
Item* Items::LIME_TERRACOTTA = nullptr;
Item* Items::PINK_TERRACOTTA = nullptr;
Item* Items::GRAY_TERRACOTTA = nullptr;
Item* Items::LIGHT_GRAY_TERRACOTTA = nullptr;
Item* Items::CYAN_TERRACOTTA = nullptr;
Item* Items::PURPLE_TERRACOTTA = nullptr;
Item* Items::BLUE_TERRACOTTA = nullptr;
Item* Items::BROWN_TERRACOTTA = nullptr;
Item* Items::GREEN_TERRACOTTA = nullptr;
Item* Items::RED_TERRACOTTA = nullptr;
Item* Items::BLACK_TERRACOTTA = nullptr;

// 功能方块
Item* Items::CRAFTING_TABLE = nullptr;
Item* Items::CHEST = nullptr;
Item* Items::TRAPPED_CHEST = nullptr;
Item* Items::BREWING_STAND = nullptr;
Item* Items::ENCHANTING_TABLE = nullptr;
Item* Items::CAULDRON = nullptr;
Item* Items::ENDER_CHEST = nullptr;
Item* Items::SHULKER_BOX = nullptr;
Item* Items::BEACON = nullptr;
Item* Items::LANTERN = nullptr;
Item* Items::SOUL_LANTERN = nullptr;
Item* Items::CAMPFIRE = nullptr;
Item* Items::SOUL_CAMPFIRE = nullptr;
Item* Items::JACK_O_LANTERN = nullptr;
Item* Items::CONDUIT = nullptr;
Item* Items::LOOM = nullptr;
Item* Items::BARREL = nullptr;
Item* Items::CARTOGRAPHY_TABLE = nullptr;
Item* Items::FLETCHING_TABLE = nullptr;
Item* Items::SMITHING_TABLE = nullptr;
Item* Items::COMPOSTER = nullptr;
Item* Items::LECTERN = nullptr;
Item* Items::JUKEBOX = nullptr;
Item* Items::RESPAWN_ANCHOR = nullptr;

// 装饰/实用方块
Item* Items::LADDER = nullptr;
Item* Items::SCAFFOLDING = nullptr;
Item* Items::CHAIN = nullptr;
Item* Items::IRON_BARS = nullptr;
Item* Items::GLASS_PANE = nullptr;
Item* Items::CARVED_PUMPKIN = nullptr;
Item* Items::END_ROD = nullptr;
Item* Items::END_PORTAL_FRAME = nullptr;
Item* Items::DRAGON_EGG = nullptr;
Item* Items::TURTLE_EGG = nullptr;
Item* Items::CHORUS_FLOWER = nullptr;

// 红石方块
// 注意：REDSTONE_WIRE 没有独立物品，红石粉物品（REDSTONE）放在地上时变成 REDSTONE_WIRE 方块
Item* Items::REDSTONE_TORCH = nullptr;
Item* Items::REDSTONE_LAMP = nullptr;
Item* Items::REDSTONE_REPEATER = nullptr;
Item* Items::REDSTONE_COMPARATOR = nullptr;
Item* Items::OBSERVER = nullptr;
Item* Items::LEVER = nullptr;
Item* Items::STONE_BUTTON = nullptr;
Item* Items::OAK_BUTTON = nullptr;
Item* Items::SPRUCE_BUTTON = nullptr;
Item* Items::BIRCH_BUTTON = nullptr;
Item* Items::JUNGLE_BUTTON = nullptr;
Item* Items::ACACIA_BUTTON = nullptr;
Item* Items::DARK_OAK_BUTTON = nullptr;
Item* Items::CRIMSON_BUTTON = nullptr;
Item* Items::WARPED_BUTTON = nullptr;
Item* Items::STONE_PRESSURE_PLATE = nullptr;
Item* Items::OAK_PRESSURE_PLATE = nullptr;
Item* Items::LIGHT_WEIGHTED_PRESSURE_PLATE = nullptr;
Item* Items::HEAVY_WEIGHTED_PRESSURE_PLATE = nullptr;
Item* Items::DAYLIGHT_DETECTOR = nullptr;
Item* Items::PISTON = nullptr;
Item* Items::STICKY_PISTON = nullptr;
Item* Items::DISPENSER = nullptr;
Item* Items::DROPPER = nullptr;
Item* Items::NOTE_BLOCK = nullptr;
Item* Items::TNT = nullptr;
Item* Items::TARGET = nullptr;
Item* Items::TRIPWIRE = nullptr;
Item* Items::TRIPWIRE_HOOK = nullptr;

// 铁轨
Item* Items::RAIL = nullptr;
Item* Items::POWERED_RAIL = nullptr;
Item* Items::DETECTOR_RAIL = nullptr;
Item* Items::ACTIVATOR_RAIL = nullptr;

// 门、栅栏、活板门
Item* Items::OAK_DOOR = nullptr;
Item* Items::IRON_DOOR = nullptr;
Item* Items::OAK_FENCE = nullptr;
Item* Items::OAK_FENCE_GATE = nullptr;
Item* Items::OAK_TRAPDOOR = nullptr;
Item* Items::IRON_TRAPDOOR = nullptr;

// 楼梯、台阶、墙
Item* Items::OAK_STAIRS = nullptr;
Item* Items::STONE_STAIRS = nullptr;
Item* Items::COBBLESTONE_STAIRS = nullptr;
Item* Items::STONE_BRICK_STAIRS = nullptr;
Item* Items::MOSSY_STONE_BRICK_STAIRS = nullptr;
Item* Items::OAK_SLAB = nullptr;
Item* Items::STONE_SLAB = nullptr;
Item* Items::COBBLESTONE_SLAB = nullptr;
Item* Items::STONE_BRICK_SLAB = nullptr;
Item* Items::MOSSY_STONE_BRICK_SLAB = nullptr;
Item* Items::COBBLESTONE_WALL = nullptr;
Item* Items::STONE_BRICK_WALL = nullptr;
Item* Items::MOSSY_STONE_BRICK_WALL = nullptr;

// 末地方块
Item* Items::END_STONE_BRICKS = nullptr;
Item* Items::PURPUR_BLOCK = nullptr;
Item* Items::PURPUR_PILLAR = nullptr;

// 海晶方块
Item* Items::PRISMARINE = nullptr;
Item* Items::PRISMARINE_BRICKS = nullptr;
Item* Items::DARK_PRISMARINE = nullptr;
Item* Items::PRISMARINE_STAIRS = nullptr;
Item* Items::PRISMARINE_BRICK_STAIRS = nullptr;
Item* Items::DARK_PRISMARINE_STAIRS = nullptr;
Item* Items::PRISMARINE_SLAB = nullptr;
Item* Items::PRISMARINE_BRICK_SLAB = nullptr;
Item* Items::DARK_PRISMARINE_SLAB = nullptr;
Item* Items::SEA_LANTERN = nullptr;

// 石砖系列
Item* Items::STONE_BRICKS = nullptr;
Item* Items::MOSSY_STONE_BRICKS = nullptr;
Item* Items::CRACKED_STONE_BRICKS = nullptr;
Item* Items::CHISELED_STONE_BRICKS = nullptr;

// 虫蚀方块
Item* Items::INFESTED_STONE = nullptr;
Item* Items::INFESTED_COBBLESTONE = nullptr;
Item* Items::INFESTED_STONE_BRICKS = nullptr;
Item* Items::INFESTED_MOSSY_STONE_BRICKS = nullptr;
Item* Items::INFESTED_CRACKED_STONE_BRICKS = nullptr;
Item* Items::INFESTED_CHISELED_STONE_BRICKS = nullptr;

// 石英系列
Item* Items::QUARTZ_BLOCK = nullptr;
Item* Items::CHISELED_QUARTZ_BLOCK = nullptr;
Item* Items::QUARTZ_PILLAR = nullptr;

// 珊瑚方块 - 活
Item* Items::TUBE_CORAL_BLOCK = nullptr;
Item* Items::BRAIN_CORAL_BLOCK = nullptr;
Item* Items::BUBBLE_CORAL_BLOCK = nullptr;
Item* Items::FIRE_CORAL_BLOCK = nullptr;
Item* Items::HORN_CORAL_BLOCK = nullptr;

// 珊瑚方块 - 死
Item* Items::DEAD_TUBE_CORAL_BLOCK = nullptr;
Item* Items::DEAD_BRAIN_CORAL_BLOCK = nullptr;
Item* Items::DEAD_BUBBLE_CORAL_BLOCK = nullptr;
Item* Items::DEAD_FIRE_CORAL_BLOCK = nullptr;
Item* Items::DEAD_HORN_CORAL_BLOCK = nullptr;

// 珊瑚扇 - 活
Item* Items::TUBE_CORAL_FAN = nullptr;
Item* Items::BRAIN_CORAL_FAN = nullptr;
Item* Items::BUBBLE_CORAL_FAN = nullptr;
Item* Items::FIRE_CORAL_FAN = nullptr;
Item* Items::HORN_CORAL_FAN = nullptr;

// 珊瑚扇 - 死
Item* Items::DEAD_TUBE_CORAL_FAN = nullptr;
Item* Items::DEAD_BRAIN_CORAL_FAN = nullptr;
Item* Items::DEAD_BUBBLE_CORAL_FAN = nullptr;
Item* Items::DEAD_FIRE_CORAL_FAN = nullptr;
Item* Items::DEAD_HORN_CORAL_FAN = nullptr;

// 试炼密室物品
Item* Items::TRIAL_KEY = nullptr;
Item* Items::OMINOUS_TRIAL_KEY = nullptr;
Item* Items::OMINOUS_BOTTLE = nullptr;
Item* Items::WIND_CHARGE = nullptr;
Item* Items::MACE = nullptr;
Item* Items::GUSTER_BANNER_PATTERN = nullptr;
Item* Items::FLOW_BANNER_PATTERN = nullptr;
Item* Items::RIB_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::FLOW_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::FLOW_POTTERY_SHERD = nullptr;
Item* Items::GUSTER_POTTERY_SHERD = nullptr;
Item* Items::SCRAPE_POTTERY_SHERD = nullptr;
Item* Items::MUSIC_DISC_CREATOR = nullptr;
Item* Items::MUSIC_DISC_CREATOR_MUSIC_BOX = nullptr;
Item* Items::MUSIC_DISC_PRECIPICE = nullptr;

// 旗帜物品（16色）
Item* Items::WHITE_BANNER = nullptr;
Item* Items::ORANGE_BANNER = nullptr;
Item* Items::MAGENTA_BANNER = nullptr;
Item* Items::LIGHT_BLUE_BANNER = nullptr;
Item* Items::YELLOW_BANNER = nullptr;
Item* Items::LIME_BANNER = nullptr;
Item* Items::PINK_BANNER = nullptr;
Item* Items::GRAY_BANNER = nullptr;
Item* Items::LIGHT_GRAY_BANNER = nullptr;
Item* Items::CYAN_BANNER = nullptr;
Item* Items::PURPLE_BANNER = nullptr;
Item* Items::BLUE_BANNER = nullptr;
Item* Items::BROWN_BANNER = nullptr;
Item* Items::GREEN_BANNER = nullptr;
Item* Items::RED_BANNER = nullptr;
Item* Items::BLACK_BANNER = nullptr;

// 旗帜图案物品
Item* Items::FLOWER_BANNER_PATTERN = nullptr;
Item* Items::CREEPER_BANNER_PATTERN = nullptr;
Item* Items::SKULL_BANNER_PATTERN = nullptr;
Item* Items::MOJANG_BANNER_PATTERN = nullptr;
Item* Items::GLOBE_BANNER_PATTERN = nullptr;
Item* Items::PIGLIN_BANNER_PATTERN = nullptr;

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

    _registerMaterials();
    _registerMisc(); // 提前注册，因为工具层级需要木板等作为修复材料

    // 初始化工具层级（需要在材料物品注册后）
    item::tier::ItemTiers::initialize();

    _registerTools();
    _registerArmor();
    _registerFood();
    _registerDyes();
    _registerSeeds();
    _registerCrops();
    _registerAquaticMaterials();
    _registerBrewingIngredients();
    _registerPotions();
    _registerWeapons();      // 武器和弹药
    _registerThrowables();   // 投掷物品
    _registerBuckets();      // 桶类物品（需要 BUCKET 在 WATER_BUCKET/LAVA_BUCKET 之前注册）
    _registerBooks();        // 书本类物品
    _registerSponges();      // 海绵物品
    _registerMinecarts();    // 矿车物品
    _registerBoats();        // 船物品
    _registerHangingItems(); // 悬挂实体物品
    _registerSigns();        // 告示牌物品
    _registerBanners();      // 旗帜和图案物品
    _registerBuildingBlocks();
    _registerWool();
    _registerCarpets();
    _registerStainedGlass();
    _registerConcrete();
    _registerTerracotta();
    _registerVegetation();
    _registerRedstone();
    _registerCoral();
    _registerDoorsFencesStairs();
    _registerTrialChamberItems(); // 试炼密室物品

    // 初始化堆肥物品注册表（必须在 Items 注册完成后）
    blocks::CompostableItems::initialize();

    s_initialized = true;
}

void Items::_registerMaterials()
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

void Items::_registerTools()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 钻石工具
    // ========================================================================
    DIAMOND_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:diamond_pickaxe"),
        item::tier::ItemTiers::DIAMOND(), // tier
        1,                                // attackDamage
        -2.8f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    DIAMOND_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:diamond_axe"),
        item::tier::ItemTiers::DIAMOND(), // tier
        7.0f,                             // attackDamage
        -3.0f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    DIAMOND_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:diamond_shovel"),
        item::tier::ItemTiers::DIAMOND(), // tier
        1.5f,                             // attackDamage
        -3.0f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    DIAMOND_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:diamond_hoe"),
        item::tier::ItemTiers::DIAMOND(), // tier
        0,                                // attackDamage
        -3.0f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    DIAMOND_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:diamond_sword"),
        item::tier::ItemTiers::DIAMOND(), // tier
        3,                                // attackDamage
        -2.4f,                            // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    // ========================================================================
    // 铁工具
    // ========================================================================
    IRON_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:iron_pickaxe"),
        item::tier::ItemTiers::IRON(), // tier
        1,                             // attackDamage
        -2.8f,                         // attackSpeed
        ItemProperties());

    IRON_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:iron_axe"),
        item::tier::ItemTiers::IRON(), // tier
        7.0f,                          // attackDamage
        -3.1f,                         // attackSpeed
        ItemProperties());

    IRON_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:iron_shovel"),
        item::tier::ItemTiers::IRON(), // tier
        1.5f,                          // attackDamage
        -3.0f,                         // attackSpeed
        ItemProperties());

    IRON_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:iron_hoe"),
        item::tier::ItemTiers::IRON(), // tier
        0,                             // attackDamage
        -2.0f,                         // attackSpeed
        ItemProperties());

    IRON_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:iron_sword"),
        item::tier::ItemTiers::IRON(), // tier
        3,                             // attackDamage
        -2.4f,                         // attackSpeed
        ItemProperties());

    // ========================================================================
    // 石工具
    // ========================================================================
    STONE_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:stone_pickaxe"),
        item::tier::ItemTiers::STONE(), // tier
        1,                              // attackDamage
        -2.8f,                          // attackSpeed
        ItemProperties());

    STONE_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:stone_axe"),
        item::tier::ItemTiers::STONE(), // tier
        8.0f,                           // attackDamage
        -3.2f,                          // attackSpeed
        ItemProperties());

    STONE_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:stone_shovel"),
        item::tier::ItemTiers::STONE(), // tier
        1.5f,                           // attackDamage
        -3.0f,                          // attackSpeed
        ItemProperties());

    STONE_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:stone_hoe"),
        item::tier::ItemTiers::STONE(), // tier
        0,                              // attackDamage
        -1.0f,                          // attackSpeed
        ItemProperties());

    STONE_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:stone_sword"),
        item::tier::ItemTiers::STONE(), // tier
        3,                              // attackDamage
        -2.4f,                          // attackSpeed
        ItemProperties());

    // ========================================================================
    // 木工具
    // ========================================================================
    WOODEN_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:wooden_pickaxe"),
        item::tier::ItemTiers::WOOD(), // tier
        1,                             // attackDamage
        -2.8f,                         // attackSpeed
        ItemProperties());

    WOODEN_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:wooden_axe"),
        item::tier::ItemTiers::WOOD(), // tier
        7.0f,                          // attackDamage
        -3.2f,                         // attackSpeed
        ItemProperties());

    WOODEN_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:wooden_shovel"),
        item::tier::ItemTiers::WOOD(), // tier
        1.5f,                          // attackDamage
        -3.0f,                         // attackSpeed
        ItemProperties());

    WOODEN_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:wooden_hoe"),
        item::tier::ItemTiers::WOOD(), // tier
        0,                             // attackDamage
        0.0f,                          // attackSpeed
        ItemProperties());

    WOODEN_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:wooden_sword"),
        item::tier::ItemTiers::WOOD(), // tier
        3,                             // attackDamage
        -2.4f,                         // attackSpeed
        ItemProperties());

    // ========================================================================
    // 金工具
    // ========================================================================
    GOLDEN_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:golden_pickaxe"),
        item::tier::ItemTiers::GOLD(), // tier
        1,                             // attackDamage
        -2.8f,                         // attackSpeed
        ItemProperties());

    GOLDEN_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:golden_axe"),
        item::tier::ItemTiers::GOLD(), // tier
        7.0f,                          // attackDamage
        -3.0f,                         // attackSpeed
        ItemProperties());

    GOLDEN_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:golden_shovel"),
        item::tier::ItemTiers::GOLD(), // tier
        1.5f,                          // attackDamage
        -3.0f,                         // attackSpeed
        ItemProperties());

    GOLDEN_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:golden_hoe"),
        item::tier::ItemTiers::GOLD(), // tier
        0,                             // attackDamage
        0.0f,                          // attackSpeed
        ItemProperties());

    GOLDEN_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:golden_sword"),
        item::tier::ItemTiers::GOLD(), // tier
        3,                             // attackDamage
        -2.4f,                         // attackSpeed
        ItemProperties());

    // ========================================================================
    // 下界合金工具
    // ========================================================================
    NETHERITE_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:netherite_pickaxe"),
        item::tier::ItemTiers::NETHERITE(), // tier
        1,                                  // attackDamage
        -2.8f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));

    NETHERITE_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:netherite_axe"),
        item::tier::ItemTiers::NETHERITE(), // tier
        6.0f,                               // attackDamage
        -3.0f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));

    NETHERITE_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:netherite_shovel"),
        item::tier::ItemTiers::NETHERITE(), // tier
        1.5f,                               // attackDamage
        -3.0f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));

    NETHERITE_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:netherite_hoe"),
        item::tier::ItemTiers::NETHERITE(), // tier
        0,                                  // attackDamage
        -4.0f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));

    NETHERITE_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:netherite_sword"),
        item::tier::ItemTiers::NETHERITE(), // tier
        3,                                  // attackDamage
        -2.4f,                              // attackSpeed
        ItemProperties().rarity(ItemRarity::Rare));
}

void Items::_registerArmor()
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

    // ========================================================================
    // 马铠 - 用于装备马提供护甲
    // ========================================================================
    // 皮革马铠 - +3 护甲
    LEATHER_HORSE_ARMOR =
        &registry.registerItem<item::items::HorseArmorItem>(ResourceLocation("minecraft:leather_horse_armor"),
            ItemProperties().maxStackSize(1),
            3,
            ResourceLocation("minecraft", "textures/entity/horse/armor/horse_armor_leather.png"));

    // 铁马铠 - +5 护甲
    IRON_HORSE_ARMOR =
        &registry.registerItem<item::items::HorseArmorItem>(ResourceLocation("minecraft:iron_horse_armor"),
            ItemProperties().maxStackSize(1),
            5,
            ResourceLocation("minecraft", "textures/entity/horse/armor/horse_armor_iron.png"));

    // 金马铠 - +7 护甲
    GOLDEN_HORSE_ARMOR =
        &registry.registerItem<item::items::HorseArmorItem>(ResourceLocation("minecraft:golden_horse_armor"),
            ItemProperties().maxStackSize(1),
            7,
            ResourceLocation("minecraft", "textures/entity/horse/armor/horse_armor_gold.png"));

    // 钻石马铠 - +11 护甲
    DIAMOND_HORSE_ARMOR =
        &registry.registerItem<item::items::HorseArmorItem>(ResourceLocation("minecraft:diamond_horse_armor"),
            ItemProperties().maxStackSize(1),
            11,
            ResourceLocation("minecraft", "textures/entity/horse/armor/horse_armor_diamond.png"));
}

void Items::_registerFood()
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

    // 蛋糕 - BlockItem，最大堆叠数为1
    CAKE = &registerBlockBackedItem(registry, VanillaBlocks::CAKE, "cake", ItemProperties().maxStackSize(1));

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

    GLOW_BERRIES = &registry.registerItem(
        ResourceLocation("minecraft:glow_berries"), ItemProperties().maxStackSize(64).food(&Foods::GLOW_BERRIES));

    TROPICAL_FISH = &registry.registerItem(
        ResourceLocation("minecraft:tropical_fish"), ItemProperties().maxStackSize(64).food(&Foods::TROPICAL_FISH));
}

void Items::_registerMisc()
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

    // 剪刀 - 耐久度 238
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

    // 地图物品
    MAP = &registry.registerItem<item::items::EmptyMapItem>(
        ResourceLocation("minecraft:map"), ItemProperties().maxStackSize(64));
    FILLED_MAP = &registry.registerItem<item::items::FilledMapItem>(
        ResourceLocation("minecraft:filled_map"), ItemProperties().maxStackSize(1));
    PAPER = &registry.registerItem(ResourceLocation("minecraft:paper"), ItemProperties().maxStackSize(64));

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

void Items::_registerDyes()
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

void Items::_registerSeeds()
{
    auto& registry = ItemRegistry::instance();

    WHEAT_SEEDS = &registry.registerItem(ResourceLocation("minecraft:wheat_seeds"), ItemProperties().maxStackSize(64));

    PUMPKIN_SEEDS =
        &registry.registerItem(ResourceLocation("minecraft:pumpkin_seeds"), ItemProperties().maxStackSize(64));

    MELON_SEEDS = &registry.registerItem(ResourceLocation("minecraft:melon_seeds"), ItemProperties().maxStackSize(64));

    BEETROOT_SEEDS =
        &registry.registerItem(ResourceLocation("minecraft:beetroot_seeds"), ItemProperties().maxStackSize(64));
}

void Items::_registerCrops()
{
    auto& registry = ItemRegistry::instance();

    WHEAT = &registry.registerItem(ResourceLocation("minecraft:wheat"), ItemProperties().maxStackSize(64));

    // 干草块 - 用于喂养马属动物，恢复大量生命值
    // 参考: new BlockItem(Blocks.HAY_BLOCK, new Item.Properties().group(ItemGroup.DECORATIONS))
    HAY_BLOCK =
        &registerBlockBackedItem(registry, VanillaBlocks::HAY_BLOCK, "hay_block", ItemProperties().maxStackSize(64));

    PUMPKIN = &registry.registerItem(ResourceLocation("minecraft:pumpkin"), ItemProperties().maxStackSize(64));

    MELON = &registry.registerItem(ResourceLocation("minecraft:melon"), ItemProperties().maxStackSize(64));

    // 注意：MELON_SLICE, CARROT, POTATO, BEETROOT 已在 registerFood() 中注册为食物
    // 这里不再重复注册
    CACTUS = &registerBlockBackedItem(registry, VanillaBlocks::CACTUS, "cactus", ItemProperties().maxStackSize(64));

    LILY_PAD =
        &registerBlockBackedItem(registry, VanillaBlocks::LILY_PAD, "lily_pad", ItemProperties().maxStackSize(64));

    VINE = &registerBlockBackedItem(registry, VanillaBlocks::VINE, "vine", ItemProperties().maxStackSize(64));

    SUGAR_CANE = &registry.registerItem(ResourceLocation("minecraft:sugar_cane"), ItemProperties().maxStackSize(64));

    SUGAR = &registry.registerItem(ResourceLocation("minecraft:sugar"), ItemProperties().maxStackSize(64));

    // 竹子 - 熊猫食物
    // 参考: new BlockItem(Blocks.BAMBOO, new Item.Properties().group(ItemGroup.DECORATIONS))
    BAMBOO = &registerBlockBackedItem(registry, VanillaBlocks::BAMBOO, "bamboo", ItemProperties().maxStackSize(64));
}

void Items::_registerAquaticMaterials()
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

    // 干海带块 - 可作为方块放置
    DRIED_KELP_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::DRIED_KELP_BLOCK, "dried_kelp_block", ItemProperties().maxStackSize(64));

    // 海泡菜 - 水下装饰与照明方块
    SEA_PICKLE =
        &registerBlockBackedItem(registry, VanillaBlocks::SEA_PICKLE, "sea_pickle", ItemProperties().maxStackSize(64));

    // 海带 - 方块掉落直接产出该物品，也是可放置的水下植物方块物品
    // 先按 block-backed item 注册，确保 loot table 能解析到 minecraft:kelp
    KELP = &registerBlockBackedItem(registry, VanillaBlocks::KELP, "kelp", ItemProperties().maxStackSize(64));

    // 海草 - 海龟食物，也是水下装饰方块
    // 参考: new BlockItem(Blocks.SEAGRASS, new Item.Properties().group(ItemGroup.DECORATIONS))
    // 注意：海草物品从方块获取，使用 registerBlockBackedItem
    SEAGRASS =
        &registerBlockBackedItem(registry, VanillaBlocks::SEAGRASS, "seagrass", ItemProperties().maxStackSize(64));

    // 注意：DRIED_KELP 已在 registerFood() 中注册为食物

    // 下界真菌
    // 绯红菌 - 可用于某些合成
    // 参考: new BlockItem(Blocks.CRIMSON_FUNGUS, new Item.Properties().group(ItemGroup.DECORATIONS))
    CRIMSON_FUNGUS = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_FUNGUS, "crimson_fungus", ItemProperties().maxStackSize(64));

    // 诡异菌 - 炽足兽食物
    // 参考: new BlockItem(Blocks.WARPED_FUNGUS, new Item.Properties().group(ItemGroup.DECORATIONS))
    WARPED_FUNGUS = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_FUNGUS, "warped_fungus", ItemProperties().maxStackSize(64));
}

void Items::_registerBrewingIngredients()
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

void Items::_registerPotions()
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

void Items::_registerWeapons()
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

    // ========================================================================
    // 骑乘控制物品 (OnAStickItem)
    // ========================================================================

    // 胡萝卜钓竿 - 控制猪
    // 耐久度25，每次加速消耗7耐久
    CARROT_ON_A_STICK =
        &registry.registerItem<item::CarrotOnAStickItem>(ResourceLocation("minecraft:carrot_on_a_stick"),
            ItemProperties().maxDamage(item::CarrotOnAStickItem::MAX_DAMAGE));

    // 诡异菌钓竿 - 控制炽足兽
    // 耐久度100，每次加速消耗1耐久
    WARPED_FUNGUS_ON_A_STICK =
        &registry.registerItem<item::WarpedFungusOnAStickItem>(ResourceLocation("minecraft:warped_fungus_on_a_stick"),
            ItemProperties().maxDamage(item::WarpedFungusOnAStickItem::MAX_DAMAGE));
}

void Items::_registerThrowables()
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

void Items::_registerBuckets()
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

    // 鱼桶
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

    // 美西螈桶
    AXOLOTL_BUCKET = &registry.registerItem<item::FishBucketItem>(ResourceLocation("minecraft:axolotl_bucket"),
        mc::entity::EntityTypes::AXOLOTL,
        ItemProperties().maxStackSize(1).containerItem(BUCKET));

    // 牛奶桶 - 清除所有药水效果
    // 参考: new MilkBucketItem(new Item.Properties().containerItem(BUCKET))
    MILK_BUCKET = &registry.registerItem<item::special::MilkBucketItem>(
        ResourceLocation("minecraft:milk_bucket"), ItemProperties().maxStackSize(1).containerItem(BUCKET));
}

void Items::_registerBooks()
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

void Items::_registerSponges()
{
    auto& registry = ItemRegistry::instance();

    // 海绵（干燥）- 吸水后变成湿海绵
    SPONGE = &registerBlockBackedItem(registry, VanillaBlocks::SPONGE, "sponge", ItemProperties().maxStackSize(64));

    // 湿海绵 - 在熔炉中干燥后返回海绵
    WET_SPONGE =
        &registerBlockBackedItem(registry, VanillaBlocks::WET_SPONGE, "wet_sponge", ItemProperties().maxStackSize(64));
}

void Items::_registerMinecarts()
{
    auto& registry = ItemRegistry::instance();

    // 普通矿车
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

void Items::_registerBoats()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 船物品
    // ========================================================================

    // 橡木船
    OAK_BOAT = &registry.registerItem<item::BoatItem>(
        ResourceLocation("minecraft:oak_boat"), entity::BoatEntity::Type::OAK, ItemProperties().maxStackSize(1));

    // 云杉木船
    SPRUCE_BOAT = &registry.registerItem<item::BoatItem>(
        ResourceLocation("minecraft:spruce_boat"), entity::BoatEntity::Type::SPRUCE, ItemProperties().maxStackSize(1));

    // 白桦木船
    BIRCH_BOAT = &registry.registerItem<item::BoatItem>(
        ResourceLocation("minecraft:birch_boat"), entity::BoatEntity::Type::BIRCH, ItemProperties().maxStackSize(1));

    // 丛林木船
    JUNGLE_BOAT = &registry.registerItem<item::BoatItem>(
        ResourceLocation("minecraft:jungle_boat"), entity::BoatEntity::Type::JUNGLE, ItemProperties().maxStackSize(1));

    // 金合欢木船
    ACACIA_BOAT = &registry.registerItem<item::BoatItem>(
        ResourceLocation("minecraft:acacia_boat"), entity::BoatEntity::Type::ACACIA, ItemProperties().maxStackSize(1));

    // 深色橡木船
    DARK_OAK_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:dark_oak_boat"),
        entity::BoatEntity::Type::DARK_OAK,
        ItemProperties().maxStackSize(1));
}

void Items::_registerHangingItems()
{
    auto& registry = ItemRegistry::instance();

    // 画作
    PAINTING = &registry.registerItem(ResourceLocation("minecraft:painting"), ItemProperties().maxStackSize(16));

    // 物品展示框
    ITEM_FRAME = &registry.registerItem(ResourceLocation("minecraft:item_frame"), ItemProperties().maxStackSize(16));

    // 拴绳
    LEAD = &registry.registerItem(ResourceLocation("minecraft:lead"), ItemProperties().maxStackSize(16));
}

void Items::_registerSigns()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 告示牌物品 - 使用 WallOrFloorItem 注册
    // ========================================================================

    // 橡木告示牌
    OAK_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:oak_sign"),
        *VanillaBlocks::OAK_SIGN,
        *VanillaBlocks::OAK_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 云杉木告示牌
    SPRUCE_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:spruce_sign"),
        *VanillaBlocks::SPRUCE_SIGN,
        *VanillaBlocks::SPRUCE_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 白桦木告示牌
    BIRCH_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:birch_sign"),
        *VanillaBlocks::BIRCH_SIGN,
        *VanillaBlocks::BIRCH_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 丛林木告示牌
    JUNGLE_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:jungle_sign"),
        *VanillaBlocks::JUNGLE_SIGN,
        *VanillaBlocks::JUNGLE_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 金合欢木告示牌
    ACACIA_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:acacia_sign"),
        *VanillaBlocks::ACACIA_SIGN,
        *VanillaBlocks::ACACIA_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 深色橡木告示牌
    DARK_OAK_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:dark_oak_sign"),
        *VanillaBlocks::DARK_OAK_SIGN,
        *VanillaBlocks::DARK_OAK_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 绯红告示牌（下界木材）
    CRIMSON_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:crimson_sign"),
        *VanillaBlocks::CRIMSON_SIGN,
        *VanillaBlocks::CRIMSON_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 诡异告示牌（下界木材）
    WARPED_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:warped_sign"),
        *VanillaBlocks::WARPED_SIGN,
        *VanillaBlocks::WARPED_WALL_SIGN,
        ItemProperties().maxStackSize(16));
}

void Items::_registerBanners()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 旗帜物品 - 使用 BannerItem (继承自 WallOrFloorItem)
    // ========================================================================

    WHITE_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:white_banner"),
        *VanillaBlocks::WHITE_BANNER,
        *VanillaBlocks::WHITE_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    ORANGE_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:orange_banner"),
        *VanillaBlocks::ORANGE_BANNER,
        *VanillaBlocks::ORANGE_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    MAGENTA_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:magenta_banner"),
        *VanillaBlocks::MAGENTA_BANNER,
        *VanillaBlocks::MAGENTA_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    LIGHT_BLUE_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:light_blue_banner"),
        *VanillaBlocks::LIGHT_BLUE_BANNER,
        *VanillaBlocks::LIGHT_BLUE_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    YELLOW_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:yellow_banner"),
        *VanillaBlocks::YELLOW_BANNER,
        *VanillaBlocks::YELLOW_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    LIME_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:lime_banner"),
        *VanillaBlocks::LIME_BANNER,
        *VanillaBlocks::LIME_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    PINK_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:pink_banner"),
        *VanillaBlocks::PINK_BANNER,
        *VanillaBlocks::PINK_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    GRAY_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:gray_banner"),
        *VanillaBlocks::GRAY_BANNER,
        *VanillaBlocks::GRAY_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    LIGHT_GRAY_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:light_gray_banner"),
        *VanillaBlocks::LIGHT_GRAY_BANNER,
        *VanillaBlocks::LIGHT_GRAY_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    CYAN_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:cyan_banner"),
        *VanillaBlocks::CYAN_BANNER,
        *VanillaBlocks::CYAN_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    PURPLE_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:purple_banner"),
        *VanillaBlocks::PURPLE_BANNER,
        *VanillaBlocks::PURPLE_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    BLUE_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:blue_banner"),
        *VanillaBlocks::BLUE_BANNER,
        *VanillaBlocks::BLUE_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    BROWN_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:brown_banner"),
        *VanillaBlocks::BROWN_BANNER,
        *VanillaBlocks::BROWN_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    GREEN_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:green_banner"),
        *VanillaBlocks::GREEN_BANNER,
        *VanillaBlocks::GREEN_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    RED_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:red_banner"),
        *VanillaBlocks::RED_BANNER,
        *VanillaBlocks::RED_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    BLACK_BANNER = &registry.registerItem<item::BannerItem>(ResourceLocation("minecraft:black_banner"),
        *VanillaBlocks::BLACK_BANNER,
        *VanillaBlocks::BLACK_WALL_BANNER,
        ItemProperties().maxStackSize(16));

    // ========================================================================
    // 旗帜图案物品
    // ========================================================================

    FLOWER_BANNER_PATTERN =
        &registry.registerItem<item::BannerPatternItem>(ResourceLocation("minecraft:flower_banner_pattern"),
            blockentity::BannerPatternType::Flower,
            ItemProperties().maxStackSize(1));

    CREEPER_BANNER_PATTERN =
        &registry.registerItem<item::BannerPatternItem>(ResourceLocation("minecraft:creeper_banner_pattern"),
            blockentity::BannerPatternType::Creeper,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Uncommon));

    SKULL_BANNER_PATTERN =
        &registry.registerItem<item::BannerPatternItem>(ResourceLocation("minecraft:skull_banner_pattern"),
            blockentity::BannerPatternType::Skull,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Uncommon));

    MOJANG_BANNER_PATTERN =
        &registry.registerItem<item::BannerPatternItem>(ResourceLocation("minecraft:mojang_banner_pattern"),
            blockentity::BannerPatternType::Mojang,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Epic));

    GLOBE_BANNER_PATTERN =
        &registry.registerItem<item::BannerPatternItem>(ResourceLocation("minecraft:globe_banner_pattern"),
            blockentity::BannerPatternType::Globe,
            ItemProperties().maxStackSize(1));

    PIGLIN_BANNER_PATTERN =
        &registry.registerItem<item::BannerPatternItem>(ResourceLocation("minecraft:piglin_banner_pattern"),
            blockentity::BannerPatternType::Piglin,
            ItemProperties().maxStackSize(1));
}

void Items::_registerBuildingBlocks()
{
    auto& registry = ItemRegistry::instance();

    // 基础建筑方块
    DIRT = &registerBlockBackedItem(registry, VanillaBlocks::DIRT, "dirt", ItemProperties().maxStackSize(64));
    GRASS_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::GRASS_BLOCK, "grass_block", ItemProperties().maxStackSize(64));
    SAND = &registerBlockBackedItem(registry, VanillaBlocks::SAND, "sand", ItemProperties().maxStackSize(64));
    GRAVEL = &registerBlockBackedItem(registry, VanillaBlocks::GRAVEL, "gravel", ItemProperties().maxStackSize(64));
    BEDROCK = &registerBlockBackedItem(registry, VanillaBlocks::BEDROCK, "bedrock", ItemProperties().maxStackSize(64));
    OBSIDIAN =
        &registerBlockBackedItem(registry, VanillaBlocks::OBSIDIAN, "obsidian", ItemProperties().maxStackSize(64));
    NETHERRACK =
        &registerBlockBackedItem(registry, VanillaBlocks::NETHERRACK, "netherrack", ItemProperties().maxStackSize(64));
    GLOWSTONE =
        &registerBlockBackedItem(registry, VanillaBlocks::GLOWSTONE, "glowstone", ItemProperties().maxStackSize(64));
    END_STONE =
        &registerBlockBackedItem(registry, VanillaBlocks::END_STONE, "end_stone", ItemProperties().maxStackSize(64));
    ICE = &registerBlockBackedItem(registry, VanillaBlocks::ICE, "ice", ItemProperties().maxStackSize(64));
    CLAY = &registerBlockBackedItem(registry, VanillaBlocks::CLAY, "clay", ItemProperties().maxStackSize(64));
    SNOW = &registerBlockBackedItem(registry, VanillaBlocks::SNOW, "snow", ItemProperties().maxStackSize(64));
    SNOW_BLOCK =
        &registerBlockBackedItem(registry, VanillaBlocks::SNOW_BLOCK, "snow_block", ItemProperties().maxStackSize(64));
    TERRACOTTA =
        &registerBlockBackedItem(registry, VanillaBlocks::TERRACOTTA, "terracotta", ItemProperties().maxStackSize(64));
    BRICKS = &registerBlockBackedItem(registry, VanillaBlocks::BRICKS, "bricks", ItemProperties().maxStackSize(64));
    BOOKSHELF =
        &registerBlockBackedItem(registry, VanillaBlocks::BOOKSHELF, "bookshelf", ItemProperties().maxStackSize(64));
    BONE_BLOCK =
        &registerBlockBackedItem(registry, VanillaBlocks::BONE_BLOCK, "bone_block", ItemProperties().maxStackSize(64));
    SLIME_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::SLIME_BLOCK, "slime_block", ItemProperties().maxStackSize(64));
    HONEY_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::HONEY_BLOCK, "honey_block", ItemProperties().maxStackSize(64));
    RED_SAND =
        &registerBlockBackedItem(registry, VanillaBlocks::RED_SAND, "red_sand", ItemProperties().maxStackSize(64));
    COBWEB = &registerBlockBackedItem(registry, VanillaBlocks::COBWEB, "cobweb", ItemProperties().maxStackSize(64));
    FARMLAND =
        &registerBlockBackedItem(registry, VanillaBlocks::FARMLAND, "farmland", ItemProperties().maxStackSize(64));
    GRASS_PATH =
        &registerBlockBackedItem(registry, VanillaBlocks::GRASS_PATH, "grass_path", ItemProperties().maxStackSize(64));
    MYCELIUM =
        &registerBlockBackedItem(registry, VanillaBlocks::MYCELIUM, "mycelium", ItemProperties().maxStackSize(64));
    PACKED_ICE =
        &registerBlockBackedItem(registry, VanillaBlocks::PACKED_ICE, "packed_ice", ItemProperties().maxStackSize(64));
    BLUE_ICE =
        &registerBlockBackedItem(registry, VanillaBlocks::BLUE_ICE, "blue_ice", ItemProperties().maxStackSize(64));
    COARSE_DIRT = &registerBlockBackedItem(
        registry, VanillaBlocks::COARSE_DIRT, "coarse_dirt", ItemProperties().maxStackSize(64));
    PODZOL = &registerBlockBackedItem(registry, VanillaBlocks::PODZOL, "podzol", ItemProperties().maxStackSize(64));

    // 石头变种
    GRANITE = &registerBlockBackedItem(registry, VanillaBlocks::GRANITE, "granite", ItemProperties().maxStackSize(64));
    POLISHED_GRANITE = &registerBlockBackedItem(
        registry, VanillaBlocks::POLISHED_GRANITE, "polished_granite", ItemProperties().maxStackSize(64));
    DIORITE = &registerBlockBackedItem(registry, VanillaBlocks::DIORITE, "diorite", ItemProperties().maxStackSize(64));
    POLISHED_DIORITE = &registerBlockBackedItem(
        registry, VanillaBlocks::POLISHED_DIORITE, "polished_diorite", ItemProperties().maxStackSize(64));
    ANDESITE =
        &registerBlockBackedItem(registry, VanillaBlocks::ANDESITE, "andesite", ItemProperties().maxStackSize(64));
    POLISHED_ANDESITE = &registerBlockBackedItem(
        registry, VanillaBlocks::POLISHED_ANDESITE, "polished_andesite", ItemProperties().maxStackSize(64));

    // 砂岩
    SANDSTONE =
        &registerBlockBackedItem(registry, VanillaBlocks::SANDSTONE, "sandstone", ItemProperties().maxStackSize(64));
    CHISELED_SANDSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::CHISELED_SANDSTONE, "chiseled_sandstone", ItemProperties().maxStackSize(64));
    CUT_SANDSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::CUT_SANDSTONE, "cut_sandstone", ItemProperties().maxStackSize(64));
    RED_SANDSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_SANDSTONE, "red_sandstone", ItemProperties().maxStackSize(64));

    // 矿物方块
    DIAMOND_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::DIAMOND_BLOCK, "diamond_block", ItemProperties().maxStackSize(64));
    COAL_BLOCK =
        &registerBlockBackedItem(registry, VanillaBlocks::COAL_BLOCK, "coal_block", ItemProperties().maxStackSize(64));
    GOLD_BLOCK =
        &registerBlockBackedItem(registry, VanillaBlocks::GOLD_BLOCK, "gold_block", ItemProperties().maxStackSize(64));
    IRON_BLOCK =
        &registerBlockBackedItem(registry, VanillaBlocks::IRON_BLOCK, "iron_block", ItemProperties().maxStackSize(64));
    LAPIS_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::LAPIS_BLOCK, "lapis_block", ItemProperties().maxStackSize(64));
    EMERALD_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::EMERALD_BLOCK, "emerald_block", ItemProperties().maxStackSize(64));
    REDSTONE_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::REDSTONE_BLOCK, "redstone_block", ItemProperties().maxStackSize(64));
    NETHERITE_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::NETHERITE_BLOCK, "netherite_block", ItemProperties().maxStackSize(64));

    // 下界方块
    SOUL_SAND =
        &registerBlockBackedItem(registry, VanillaBlocks::SOUL_SAND, "soul_sand", ItemProperties().maxStackSize(64));
    SOUL_SOIL =
        &registerBlockBackedItem(registry, VanillaBlocks::SOUL_SOIL, "soul_soil", ItemProperties().maxStackSize(64));
    BASALT = &registerBlockBackedItem(registry, VanillaBlocks::BASALT, "basalt", ItemProperties().maxStackSize(64));
    POLISHED_BASALT = &registerBlockBackedItem(
        registry, VanillaBlocks::POLISHED_BASALT, "polished_basalt", ItemProperties().maxStackSize(64));
    BLACKSTONE =
        &registerBlockBackedItem(registry, VanillaBlocks::BLACKSTONE, "blackstone", ItemProperties().maxStackSize(64));
    POLISHED_BLACKSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::POLISHED_BLACKSTONE, "polished_blackstone", ItemProperties().maxStackSize(64));
    CRYING_OBSIDIAN = &registerBlockBackedItem(
        registry, VanillaBlocks::CRYING_OBSIDIAN, "crying_obsidian", ItemProperties().maxStackSize(64));
    MAGMA = &registerBlockBackedItem(registry, VanillaBlocks::MAGMA, "magma_block", ItemProperties().maxStackSize(64));
    NETHER_WART_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::NETHER_WART_BLOCK, "nether_wart_block", ItemProperties().maxStackSize(64));
    WARPED_WART_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_WART_BLOCK, "warped_wart_block", ItemProperties().maxStackSize(64));
    CRIMSON_STEM = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_STEM, "crimson_stem", ItemProperties().maxStackSize(64));
    WARPED_STEM = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_STEM, "warped_stem", ItemProperties().maxStackSize(64));
    CRIMSON_NYLIUM = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_NYLIUM, "crimson_nylium", ItemProperties().maxStackSize(64));
    WARPED_NYLIUM = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_NYLIUM, "warped_nylium", ItemProperties().maxStackSize(64));
    CRIMSON_HYPHAE = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_HYPHAE, "crimson_hyphae", ItemProperties().maxStackSize(64));
    WARPED_HYPHAE = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_HYPHAE, "warped_hyphae", ItemProperties().maxStackSize(64));
    STRIPPED_CRIMSON_STEM = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_CRIMSON_STEM, "stripped_crimson_stem", ItemProperties().maxStackSize(64));
    STRIPPED_WARPED_STEM = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_WARPED_STEM, "stripped_warped_stem", ItemProperties().maxStackSize(64));
    STRIPPED_CRIMSON_HYPHAE = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_CRIMSON_HYPHAE, "stripped_crimson_hyphae", ItemProperties().maxStackSize(64));
    STRIPPED_WARPED_HYPHAE = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_WARPED_HYPHAE, "stripped_warped_hyphae", ItemProperties().maxStackSize(64));
    SHROOMLIGHT = &registerBlockBackedItem(
        registry, VanillaBlocks::SHROOMLIGHT, "shroomlight", ItemProperties().maxStackSize(64));
    WEEPING_VINES = &registerBlockBackedItem(
        registry, VanillaBlocks::WEEPING_VINES, "weeping_vines", ItemProperties().maxStackSize(64));
    TWISTING_VINES = &registerBlockBackedItem(
        registry, VanillaBlocks::TWISTING_VINES, "twisting_vines", ItemProperties().maxStackSize(64));
    CRIMSON_ROOTS = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_ROOTS, "crimson_roots", ItemProperties().maxStackSize(64));
    WARPED_ROOTS = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_ROOTS, "warped_roots", ItemProperties().maxStackSize(64));
    NETHER_SPROUTS = &registerBlockBackedItem(
        registry, VanillaBlocks::NETHER_SPROUTS, "nether_sprouts", ItemProperties().maxStackSize(64));
    DEAD_BUSH =
        &registerBlockBackedItem(registry, VanillaBlocks::DEAD_BUSH, "dead_bush", ItemProperties().maxStackSize(64));

    // 木材和去皮原木
    OAK_WOOD =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_WOOD, "oak_wood", ItemProperties().maxStackSize(64));
    SPRUCE_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_WOOD, "spruce_wood", ItemProperties().maxStackSize(64));
    BIRCH_WOOD =
        &registerBlockBackedItem(registry, VanillaBlocks::BIRCH_WOOD, "birch_wood", ItemProperties().maxStackSize(64));
    JUNGLE_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_WOOD, "jungle_wood", ItemProperties().maxStackSize(64));
    ACACIA_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_WOOD, "acacia_wood", ItemProperties().maxStackSize(64));
    DARK_OAK_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_WOOD, "dark_oak_wood", ItemProperties().maxStackSize(64));
    STRIPPED_OAK_LOG = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_OAK_LOG, "stripped_oak_log", ItemProperties().maxStackSize(64));
    STRIPPED_SPRUCE_LOG = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_SPRUCE_LOG, "stripped_spruce_log", ItemProperties().maxStackSize(64));
    STRIPPED_BIRCH_LOG = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_BIRCH_LOG, "stripped_birch_log", ItemProperties().maxStackSize(64));
    STRIPPED_JUNGLE_LOG = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_JUNGLE_LOG, "stripped_jungle_log", ItemProperties().maxStackSize(64));
    STRIPPED_ACACIA_LOG = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_ACACIA_LOG, "stripped_acacia_log", ItemProperties().maxStackSize(64));
    STRIPPED_DARK_OAK_LOG = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_DARK_OAK_LOG, "stripped_dark_oak_log", ItemProperties().maxStackSize(64));
    STRIPPED_OAK_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_OAK_WOOD, "stripped_oak_wood", ItemProperties().maxStackSize(64));
    STRIPPED_SPRUCE_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_SPRUCE_WOOD, "stripped_spruce_wood", ItemProperties().maxStackSize(64));
    STRIPPED_BIRCH_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_BIRCH_WOOD, "stripped_birch_wood", ItemProperties().maxStackSize(64));
    STRIPPED_JUNGLE_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_JUNGLE_WOOD, "stripped_jungle_wood", ItemProperties().maxStackSize(64));
    STRIPPED_ACACIA_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_ACACIA_WOOD, "stripped_acacia_wood", ItemProperties().maxStackSize(64));
    STRIPPED_DARK_OAK_WOOD = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_DARK_OAK_WOOD, "stripped_dark_oak_wood", ItemProperties().maxStackSize(64));

    // 树叶
    OAK_LEAVES =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_LEAVES, "oak_leaves", ItemProperties().maxStackSize(64));
    SPRUCE_LEAVES = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_LEAVES, "spruce_leaves", ItemProperties().maxStackSize(64));
    BIRCH_LEAVES = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_LEAVES, "birch_leaves", ItemProperties().maxStackSize(64));
    JUNGLE_LEAVES = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_LEAVES, "jungle_leaves", ItemProperties().maxStackSize(64));
    ACACIA_LEAVES = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_LEAVES, "acacia_leaves", ItemProperties().maxStackSize(64));
    DARK_OAK_LEAVES = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_LEAVES, "dark_oak_leaves", ItemProperties().maxStackSize(64));

    // 树苗
    OAK_SAPLING = &registerBlockBackedItem(
        registry, VanillaBlocks::OAK_SAPLING, "oak_sapling", ItemProperties().maxStackSize(64));
    SPRUCE_SAPLING = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_SAPLING, "spruce_sapling", ItemProperties().maxStackSize(64));
    BIRCH_SAPLING = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_SAPLING, "birch_sapling", ItemProperties().maxStackSize(64));
    JUNGLE_SAPLING = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_SAPLING, "jungle_sapling", ItemProperties().maxStackSize(64));
    ACACIA_SAPLING = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_SAPLING, "acacia_sapling", ItemProperties().maxStackSize(64));
    DARK_OAK_SAPLING = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_SAPLING, "dark_oak_sapling", ItemProperties().maxStackSize(64));

    // 功能方块
    CRAFTING_TABLE = &registerBlockBackedItem(
        registry, VanillaBlocks::CRAFTING_TABLE, "crafting_table", ItemProperties().maxStackSize(64));
    CHEST = &registerBlockBackedItem(registry, VanillaBlocks::CHEST, "chest", ItemProperties().maxStackSize(64));
    TRAPPED_CHEST = &registerBlockBackedItem(
        registry, VanillaBlocks::TRAPPED_CHEST, "trapped_chest", ItemProperties().maxStackSize(64));
    BREWING_STAND = &registerBlockBackedItem(
        registry, VanillaBlocks::BREWING_STAND, "brewing_stand", ItemProperties().maxStackSize(64));
    ENCHANTING_TABLE = &registerBlockBackedItem(
        registry, VanillaBlocks::ENCHANTING_TABLE, "enchanting_table", ItemProperties().maxStackSize(64));
    CAULDRON =
        &registerBlockBackedItem(registry, VanillaBlocks::CAULDRON, "cauldron", ItemProperties().maxStackSize(64));
    ENDER_CHEST = &registerBlockBackedItem(
        registry, VanillaBlocks::ENDER_CHEST, "ender_chest", ItemProperties().maxStackSize(64));
    SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::SHULKER_BOX, "shulker_box", ItemProperties().maxStackSize(64));
    BEACON = &registerBlockBackedItem(registry, VanillaBlocks::BEACON, "beacon", ItemProperties().maxStackSize(64));
    LANTERN = &registerBlockBackedItem(registry, VanillaBlocks::LANTERN, "lantern", ItemProperties().maxStackSize(64));
    SOUL_LANTERN = &registerBlockBackedItem(
        registry, VanillaBlocks::SOUL_LANTERN, "soul_lantern", ItemProperties().maxStackSize(64));
    CAMPFIRE =
        &registerBlockBackedItem(registry, VanillaBlocks::CAMPFIRE, "campfire", ItemProperties().maxStackSize(64));
    SOUL_CAMPFIRE = &registerBlockBackedItem(
        registry, VanillaBlocks::SOUL_CAMPFIRE, "soul_campfire", ItemProperties().maxStackSize(64));
    JACK_O_LANTERN = &registerBlockBackedItem(
        registry, VanillaBlocks::JACK_O_LANTERN, "jack_o_lantern", ItemProperties().maxStackSize(64));
    CONDUIT = &registerBlockBackedItem(registry, VanillaBlocks::CONDUIT, "conduit", ItemProperties().maxStackSize(64));
    LOOM = &registerBlockBackedItem(registry, VanillaBlocks::LOOM, "loom", ItemProperties().maxStackSize(64));
    BARREL = &registerBlockBackedItem(registry, VanillaBlocks::BARREL, "barrel", ItemProperties().maxStackSize(64));
    CARTOGRAPHY_TABLE = &registerBlockBackedItem(
        registry, VanillaBlocks::CARTOGRAPHY_TABLE, "cartography_table", ItemProperties().maxStackSize(64));
    FLETCHING_TABLE = &registerBlockBackedItem(
        registry, VanillaBlocks::FLETCHING_TABLE, "fletching_table", ItemProperties().maxStackSize(64));
    SMITHING_TABLE = &registerBlockBackedItem(
        registry, VanillaBlocks::SMITHING_TABLE, "smithing_table", ItemProperties().maxStackSize(64));
    COMPOSTER =
        &registerBlockBackedItem(registry, VanillaBlocks::COMPOSTER, "composter", ItemProperties().maxStackSize(64));
    LECTERN = &registerBlockBackedItem(registry, VanillaBlocks::LECTERN, "lectern", ItemProperties().maxStackSize(64));
    JUKEBOX = &registerBlockBackedItem(registry, VanillaBlocks::JUKEBOX, "jukebox", ItemProperties().maxStackSize(64));
    RESPAWN_ANCHOR = &registerBlockBackedItem(
        registry, VanillaBlocks::RESPAWN_ANCHOR, "respawn_anchor", ItemProperties().maxStackSize(64));

    // 装饰/实用方块
    LADDER = &registerBlockBackedItem(registry, VanillaBlocks::LADDER, "ladder", ItemProperties().maxStackSize(64));
    SCAFFOLDING = &registerBlockBackedItem(
        registry, VanillaBlocks::SCAFFOLDING, "scaffolding", ItemProperties().maxStackSize(64));
    CHAIN = &registerBlockBackedItem(registry, VanillaBlocks::CHAIN, "chain", ItemProperties().maxStackSize(64));
    IRON_BARS =
        &registerBlockBackedItem(registry, VanillaBlocks::IRON_BARS, "iron_bars", ItemProperties().maxStackSize(64));
    GLASS_PANE =
        &registerBlockBackedItem(registry, VanillaBlocks::GLASS_PANE, "glass_pane", ItemProperties().maxStackSize(64));
    CARVED_PUMPKIN = &registerBlockBackedItem(
        registry, VanillaBlocks::CARVED_PUMPKIN, "carved_pumpkin", ItemProperties().maxStackSize(64));
    END_ROD = &registerBlockBackedItem(registry, VanillaBlocks::END_ROD, "end_rod", ItemProperties().maxStackSize(64));
    END_PORTAL_FRAME = &registerBlockBackedItem(
        registry, VanillaBlocks::END_PORTAL_FRAME, "end_portal_frame", ItemProperties().maxStackSize(64));
    DRAGON_EGG =
        &registerBlockBackedItem(registry, VanillaBlocks::DRAGON_EGG, "dragon_egg", ItemProperties().maxStackSize(64));
    TURTLE_EGG =
        &registerBlockBackedItem(registry, VanillaBlocks::TURTLE_EGG, "turtle_egg", ItemProperties().maxStackSize(64));
    CHORUS_FLOWER = &registerBlockBackedItem(
        registry, VanillaBlocks::CHORUS_FLOWER, "chorus_flower", ItemProperties().maxStackSize(64));

    // 末地方块
    END_STONE_BRICKS = &registerBlockBackedItem(
        registry, VanillaBlocks::END_STONE_BRICKS, "end_stone_bricks", ItemProperties().maxStackSize(64));
    PURPUR_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPUR_BLOCK, "purpur_block", ItemProperties().maxStackSize(64));
    PURPUR_PILLAR = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPUR_PILLAR, "purpur_pillar", ItemProperties().maxStackSize(64));

    // 海晶方块
    PRISMARINE =
        &registerBlockBackedItem(registry, VanillaBlocks::PRISMARINE, "prismarine", ItemProperties().maxStackSize(64));
    PRISMARINE_BRICKS = &registerBlockBackedItem(
        registry, VanillaBlocks::PRISMARINE_BRICKS, "prismarine_bricks", ItemProperties().maxStackSize(64));
    DARK_PRISMARINE = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_PRISMARINE, "dark_prismarine", ItemProperties().maxStackSize(64));
    PRISMARINE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::PRISMARINE_STAIRS, "prismarine_stairs", ItemProperties().maxStackSize(64));
    PRISMARINE_BRICK_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::PRISMARINE_BRICK_STAIRS, "prismarine_brick_stairs", ItemProperties().maxStackSize(64));
    DARK_PRISMARINE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_PRISMARINE_STAIRS, "dark_prismarine_stairs", ItemProperties().maxStackSize(64));
    PRISMARINE_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::PRISMARINE_SLAB, "prismarine_slab", ItemProperties().maxStackSize(64));
    PRISMARINE_BRICK_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::PRISMARINE_BRICK_SLAB, "prismarine_brick_slab", ItemProperties().maxStackSize(64));
    DARK_PRISMARINE_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_PRISMARINE_SLAB, "dark_prismarine_slab", ItemProperties().maxStackSize(64));
    SEA_LANTERN = &registerBlockBackedItem(
        registry, VanillaBlocks::SEA_LANTERN, "sea_lantern", ItemProperties().maxStackSize(64));

    // 石砖系列
    STONE_BRICKS = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_BRICKS, "stone_bricks", ItemProperties().maxStackSize(64));
    MOSSY_STONE_BRICKS = &registerBlockBackedItem(
        registry, VanillaBlocks::MOSSY_STONE_BRICKS, "mossy_stone_bricks", ItemProperties().maxStackSize(64));
    CRACKED_STONE_BRICKS = &registerBlockBackedItem(
        registry, VanillaBlocks::CRACKED_STONE_BRICKS, "cracked_stone_bricks", ItemProperties().maxStackSize(64));
    CHISELED_STONE_BRICKS = &registerBlockBackedItem(
        registry, VanillaBlocks::CHISELED_STONE_BRICKS, "chiseled_stone_bricks", ItemProperties().maxStackSize(64));

    // 虫蚀方块
    INFESTED_STONE = &registerBlockBackedItem(
        registry, VanillaBlocks::INFESTED_STONE, "infested_stone", ItemProperties().maxStackSize(64));
    INFESTED_COBBLESTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::INFESTED_COBBLESTONE, "infested_cobblestone", ItemProperties().maxStackSize(64));
    INFESTED_STONE_BRICKS = &registerBlockBackedItem(
        registry, VanillaBlocks::INFESTED_STONE_BRICKS, "infested_stone_bricks", ItemProperties().maxStackSize(64));
    INFESTED_MOSSY_STONE_BRICKS = &registerBlockBackedItem(registry,
        VanillaBlocks::INFESTED_MOSSY_STONE_BRICKS,
        "infested_mossy_stone_bricks",
        ItemProperties().maxStackSize(64));
    INFESTED_CRACKED_STONE_BRICKS = &registerBlockBackedItem(registry,
        VanillaBlocks::INFESTED_CRACKED_STONE_BRICKS,
        "infested_cracked_stone_bricks",
        ItemProperties().maxStackSize(64));
    INFESTED_CHISELED_STONE_BRICKS = &registerBlockBackedItem(registry,
        VanillaBlocks::INFESTED_CHISELED_STONE_BRICKS,
        "infested_chiseled_stone_bricks",
        ItemProperties().maxStackSize(64));

    // 石英系列
    QUARTZ_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::QUARTZ_BLOCK, "quartz_block", ItemProperties().maxStackSize(64));
    CHISELED_QUARTZ_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::CHISELED_QUARTZ_BLOCK, "chiseled_quartz_block", ItemProperties().maxStackSize(64));
    QUARTZ_PILLAR = &registerBlockBackedItem(
        registry, VanillaBlocks::QUARTZ_PILLAR, "quartz_pillar", ItemProperties().maxStackSize(64));
}

void Items::_registerWool()
{
    auto& registry = ItemRegistry::instance();

    WHITE_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::WHITE_WOOL, "white_wool", ItemProperties().maxStackSize(64));
    ORANGE_WOOL = &registerBlockBackedItem(
        registry, VanillaBlocks::ORANGE_WOOL, "orange_wool", ItemProperties().maxStackSize(64));
    MAGENTA_WOOL = &registerBlockBackedItem(
        registry, VanillaBlocks::MAGENTA_WOOL, "magenta_wool", ItemProperties().maxStackSize(64));
    LIGHT_BLUE_WOOL = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_BLUE_WOOL, "light_blue_wool", ItemProperties().maxStackSize(64));
    YELLOW_WOOL = &registerBlockBackedItem(
        registry, VanillaBlocks::YELLOW_WOOL, "yellow_wool", ItemProperties().maxStackSize(64));
    LIME_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::LIME_WOOL, "lime_wool", ItemProperties().maxStackSize(64));
    PINK_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::PINK_WOOL, "pink_wool", ItemProperties().maxStackSize(64));
    GRAY_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::GRAY_WOOL, "gray_wool", ItemProperties().maxStackSize(64));
    LIGHT_GRAY_WOOL = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_GRAY_WOOL, "light_gray_wool", ItemProperties().maxStackSize(64));
    CYAN_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::CYAN_WOOL, "cyan_wool", ItemProperties().maxStackSize(64));
    PURPLE_WOOL = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPLE_WOOL, "purple_wool", ItemProperties().maxStackSize(64));
    BLUE_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::BLUE_WOOL, "blue_wool", ItemProperties().maxStackSize(64));
    BROWN_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::BROWN_WOOL, "brown_wool", ItemProperties().maxStackSize(64));
    GREEN_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::GREEN_WOOL, "green_wool", ItemProperties().maxStackSize(64));
    RED_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::RED_WOOL, "red_wool", ItemProperties().maxStackSize(64));
    BLACK_WOOL =
        &registerBlockBackedItem(registry, VanillaBlocks::BLACK_WOOL, "black_wool", ItemProperties().maxStackSize(64));
}

void Items::_registerCarpets()
{
    auto& registry = ItemRegistry::instance();

    WHITE_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::WHITE_CARPET, "white_carpet", ItemProperties().maxStackSize(64));
    ORANGE_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::ORANGE_CARPET, "orange_carpet", ItemProperties().maxStackSize(64));
    MAGENTA_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::MAGENTA_CARPET, "magenta_carpet", ItemProperties().maxStackSize(64));
    LIGHT_BLUE_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_BLUE_CARPET, "light_blue_carpet", ItemProperties().maxStackSize(64));
    YELLOW_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::YELLOW_CARPET, "yellow_carpet", ItemProperties().maxStackSize(64));
    LIME_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::LIME_CARPET, "lime_carpet", ItemProperties().maxStackSize(64));
    PINK_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::PINK_CARPET, "pink_carpet", ItemProperties().maxStackSize(64));
    GRAY_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::GRAY_CARPET, "gray_carpet", ItemProperties().maxStackSize(64));
    LIGHT_GRAY_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_GRAY_CARPET, "light_gray_carpet", ItemProperties().maxStackSize(64));
    CYAN_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::CYAN_CARPET, "cyan_carpet", ItemProperties().maxStackSize(64));
    PURPLE_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPLE_CARPET, "purple_carpet", ItemProperties().maxStackSize(64));
    BLUE_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::BLUE_CARPET, "blue_carpet", ItemProperties().maxStackSize(64));
    BROWN_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::BROWN_CARPET, "brown_carpet", ItemProperties().maxStackSize(64));
    GREEN_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::GREEN_CARPET, "green_carpet", ItemProperties().maxStackSize(64));
    RED_CARPET =
        &registerBlockBackedItem(registry, VanillaBlocks::RED_CARPET, "red_carpet", ItemProperties().maxStackSize(64));
    BLACK_CARPET = &registerBlockBackedItem(
        registry, VanillaBlocks::BLACK_CARPET, "black_carpet", ItemProperties().maxStackSize(64));
}

void Items::_registerStainedGlass()
{
    auto& registry = ItemRegistry::instance();

    WHITE_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::WHITE_STAINED_GLASS, "white_stained_glass", ItemProperties().maxStackSize(64));
    ORANGE_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::ORANGE_STAINED_GLASS, "orange_stained_glass", ItemProperties().maxStackSize(64));
    MAGENTA_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::MAGENTA_STAINED_GLASS, "magenta_stained_glass", ItemProperties().maxStackSize(64));
    LIGHT_BLUE_STAINED_GLASS = &registerBlockBackedItem(registry,
        VanillaBlocks::LIGHT_BLUE_STAINED_GLASS,
        "light_blue_stained_glass",
        ItemProperties().maxStackSize(64));
    YELLOW_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::YELLOW_STAINED_GLASS, "yellow_stained_glass", ItemProperties().maxStackSize(64));
    LIME_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::LIME_STAINED_GLASS, "lime_stained_glass", ItemProperties().maxStackSize(64));
    PINK_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::PINK_STAINED_GLASS, "pink_stained_glass", ItemProperties().maxStackSize(64));
    GRAY_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::GRAY_STAINED_GLASS, "gray_stained_glass", ItemProperties().maxStackSize(64));
    LIGHT_GRAY_STAINED_GLASS = &registerBlockBackedItem(registry,
        VanillaBlocks::LIGHT_GRAY_STAINED_GLASS,
        "light_gray_stained_glass",
        ItemProperties().maxStackSize(64));
    CYAN_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::CYAN_STAINED_GLASS, "cyan_stained_glass", ItemProperties().maxStackSize(64));
    PURPLE_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPLE_STAINED_GLASS, "purple_stained_glass", ItemProperties().maxStackSize(64));
    BLUE_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::BLUE_STAINED_GLASS, "blue_stained_glass", ItemProperties().maxStackSize(64));
    BROWN_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::BROWN_STAINED_GLASS, "brown_stained_glass", ItemProperties().maxStackSize(64));
    GREEN_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::GREEN_STAINED_GLASS, "green_stained_glass", ItemProperties().maxStackSize(64));
    RED_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_STAINED_GLASS, "red_stained_glass", ItemProperties().maxStackSize(64));
    BLACK_STAINED_GLASS = &registerBlockBackedItem(
        registry, VanillaBlocks::BLACK_STAINED_GLASS, "black_stained_glass", ItemProperties().maxStackSize(64));
}

void Items::_registerConcrete()
{
    auto& registry = ItemRegistry::instance();

    // 混凝土 (16色)
    WHITE_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::WHITE_CONCRETE, "white_concrete", ItemProperties().maxStackSize(64));
    ORANGE_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::ORANGE_CONCRETE, "orange_concrete", ItemProperties().maxStackSize(64));
    MAGENTA_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::MAGENTA_CONCRETE, "magenta_concrete", ItemProperties().maxStackSize(64));
    LIGHT_BLUE_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_BLUE_CONCRETE, "light_blue_concrete", ItemProperties().maxStackSize(64));
    YELLOW_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::YELLOW_CONCRETE, "yellow_concrete", ItemProperties().maxStackSize(64));
    LIME_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::LIME_CONCRETE, "lime_concrete", ItemProperties().maxStackSize(64));
    PINK_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::PINK_CONCRETE, "pink_concrete", ItemProperties().maxStackSize(64));
    GRAY_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::GRAY_CONCRETE, "gray_concrete", ItemProperties().maxStackSize(64));
    LIGHT_GRAY_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_GRAY_CONCRETE, "light_gray_concrete", ItemProperties().maxStackSize(64));
    CYAN_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::CYAN_CONCRETE, "cyan_concrete", ItemProperties().maxStackSize(64));
    PURPLE_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPLE_CONCRETE, "purple_concrete", ItemProperties().maxStackSize(64));
    BLUE_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::BLUE_CONCRETE, "blue_concrete", ItemProperties().maxStackSize(64));
    BROWN_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::BROWN_CONCRETE, "brown_concrete", ItemProperties().maxStackSize(64));
    GREEN_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::GREEN_CONCRETE, "green_concrete", ItemProperties().maxStackSize(64));
    RED_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_CONCRETE, "red_concrete", ItemProperties().maxStackSize(64));
    BLACK_CONCRETE = &registerBlockBackedItem(
        registry, VanillaBlocks::BLACK_CONCRETE, "black_concrete", ItemProperties().maxStackSize(64));

    // 混凝土粉末 (16色)
    WHITE_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::WHITE_CONCRETE_POWDER, "white_concrete_powder", ItemProperties().maxStackSize(64));
    ORANGE_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::ORANGE_CONCRETE_POWDER, "orange_concrete_powder", ItemProperties().maxStackSize(64));
    MAGENTA_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::MAGENTA_CONCRETE_POWDER, "magenta_concrete_powder", ItemProperties().maxStackSize(64));
    LIGHT_BLUE_CONCRETE_POWDER = &registerBlockBackedItem(registry,
        VanillaBlocks::LIGHT_BLUE_CONCRETE_POWDER,
        "light_blue_concrete_powder",
        ItemProperties().maxStackSize(64));
    YELLOW_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::YELLOW_CONCRETE_POWDER, "yellow_concrete_powder", ItemProperties().maxStackSize(64));
    LIME_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::LIME_CONCRETE_POWDER, "lime_concrete_powder", ItemProperties().maxStackSize(64));
    PINK_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::PINK_CONCRETE_POWDER, "pink_concrete_powder", ItemProperties().maxStackSize(64));
    GRAY_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::GRAY_CONCRETE_POWDER, "gray_concrete_powder", ItemProperties().maxStackSize(64));
    LIGHT_GRAY_CONCRETE_POWDER = &registerBlockBackedItem(registry,
        VanillaBlocks::LIGHT_GRAY_CONCRETE_POWDER,
        "light_gray_concrete_powder",
        ItemProperties().maxStackSize(64));
    CYAN_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::CYAN_CONCRETE_POWDER, "cyan_concrete_powder", ItemProperties().maxStackSize(64));
    PURPLE_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPLE_CONCRETE_POWDER, "purple_concrete_powder", ItemProperties().maxStackSize(64));
    BLUE_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::BLUE_CONCRETE_POWDER, "blue_concrete_powder", ItemProperties().maxStackSize(64));
    BROWN_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::BROWN_CONCRETE_POWDER, "brown_concrete_powder", ItemProperties().maxStackSize(64));
    GREEN_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::GREEN_CONCRETE_POWDER, "green_concrete_powder", ItemProperties().maxStackSize(64));
    RED_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_CONCRETE_POWDER, "red_concrete_powder", ItemProperties().maxStackSize(64));
    BLACK_CONCRETE_POWDER = &registerBlockBackedItem(
        registry, VanillaBlocks::BLACK_CONCRETE_POWDER, "black_concrete_powder", ItemProperties().maxStackSize(64));
}

void Items::_registerTerracotta()
{
    auto& registry = ItemRegistry::instance();

    WHITE_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::WHITE_TERRACOTTA, "white_terracotta", ItemProperties().maxStackSize(64));
    ORANGE_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::ORANGE_TERRACOTTA, "orange_terracotta", ItemProperties().maxStackSize(64));
    MAGENTA_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::MAGENTA_TERRACOTTA, "magenta_terracotta", ItemProperties().maxStackSize(64));
    LIGHT_BLUE_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_BLUE_TERRACOTTA, "light_blue_terracotta", ItemProperties().maxStackSize(64));
    YELLOW_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::YELLOW_TERRACOTTA, "yellow_terracotta", ItemProperties().maxStackSize(64));
    LIME_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::LIME_TERRACOTTA, "lime_terracotta", ItemProperties().maxStackSize(64));
    PINK_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::PINK_TERRACOTTA, "pink_terracotta", ItemProperties().maxStackSize(64));
    GRAY_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::GRAY_TERRACOTTA, "gray_terracotta", ItemProperties().maxStackSize(64));
    LIGHT_GRAY_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_GRAY_TERRACOTTA, "light_gray_terracotta", ItemProperties().maxStackSize(64));
    CYAN_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::CYAN_TERRACOTTA, "cyan_terracotta", ItemProperties().maxStackSize(64));
    PURPLE_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPLE_TERRACOTTA, "purple_terracotta", ItemProperties().maxStackSize(64));
    BLUE_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::BLUE_TERRACOTTA, "blue_terracotta", ItemProperties().maxStackSize(64));
    BROWN_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::BROWN_TERRACOTTA, "brown_terracotta", ItemProperties().maxStackSize(64));
    GREEN_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::GREEN_TERRACOTTA, "green_terracotta", ItemProperties().maxStackSize(64));
    RED_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_TERRACOTTA, "red_terracotta", ItemProperties().maxStackSize(64));
    BLACK_TERRACOTTA = &registerBlockBackedItem(
        registry, VanillaBlocks::BLACK_TERRACOTTA, "black_terracotta", ItemProperties().maxStackSize(64));
}

void Items::_registerVegetation()
{
    auto& registry = ItemRegistry::instance();

    // 植被和花
    SHORT_GRASS =
        &registerBlockBackedItem(registry, VanillaBlocks::SHORT_GRASS, "grass", ItemProperties().maxStackSize(64));
    TALL_GRASS =
        &registerBlockBackedItem(registry, VanillaBlocks::TALL_GRASS, "tall_grass", ItemProperties().maxStackSize(64));
    FERN = &registerBlockBackedItem(registry, VanillaBlocks::FERN, "fern", ItemProperties().maxStackSize(64));
    LARGE_FERN =
        &registerBlockBackedItem(registry, VanillaBlocks::LARGE_FERN, "large_fern", ItemProperties().maxStackSize(64));
    DANDELION =
        &registerBlockBackedItem(registry, VanillaBlocks::DANDELION, "dandelion", ItemProperties().maxStackSize(64));
    POPPY = &registerBlockBackedItem(registry, VanillaBlocks::POPPY, "poppy", ItemProperties().maxStackSize(64));
    BLUE_ORCHID = &registerBlockBackedItem(
        registry, VanillaBlocks::BLUE_ORCHID, "blue_orchid", ItemProperties().maxStackSize(64));
    ALLIUM = &registerBlockBackedItem(registry, VanillaBlocks::ALLIUM, "allium", ItemProperties().maxStackSize(64));
    AZURE_BLUET = &registerBlockBackedItem(
        registry, VanillaBlocks::AZURE_BLUET, "azure_bluet", ItemProperties().maxStackSize(64));
    RED_TULIP =
        &registerBlockBackedItem(registry, VanillaBlocks::RED_TULIP, "red_tulip", ItemProperties().maxStackSize(64));
    ORANGE_TULIP = &registerBlockBackedItem(
        registry, VanillaBlocks::ORANGE_TULIP, "orange_tulip", ItemProperties().maxStackSize(64));
    WHITE_TULIP = &registerBlockBackedItem(
        registry, VanillaBlocks::WHITE_TULIP, "white_tulip", ItemProperties().maxStackSize(64));
    PINK_TULIP =
        &registerBlockBackedItem(registry, VanillaBlocks::PINK_TULIP, "pink_tulip", ItemProperties().maxStackSize(64));
    OXEYE_DAISY = &registerBlockBackedItem(
        registry, VanillaBlocks::OXEYE_DAISY, "oxeye_daisy", ItemProperties().maxStackSize(64));
    LILY_OF_THE_VALLEY = &registerBlockBackedItem(
        registry, VanillaBlocks::LILY_OF_THE_VALLEY, "lily_of_the_valley", ItemProperties().maxStackSize(64));
    SUNFLOWER =
        &registerBlockBackedItem(registry, VanillaBlocks::SUNFLOWER, "sunflower", ItemProperties().maxStackSize(64));
    LILAC = &registerBlockBackedItem(registry, VanillaBlocks::LILAC, "lilac", ItemProperties().maxStackSize(64));
    ROSE_BUSH =
        &registerBlockBackedItem(registry, VanillaBlocks::ROSE_BUSH, "rose_bush", ItemProperties().maxStackSize(64));
    PEONY = &registerBlockBackedItem(registry, VanillaBlocks::PEONY, "peony", ItemProperties().maxStackSize(64));
    CORNFLOWER =
        &registerBlockBackedItem(registry, VanillaBlocks::CORNFLOWER, "cornflower", ItemProperties().maxStackSize(64));
    WITHER_ROSE = &registerBlockBackedItem(
        registry, VanillaBlocks::WITHER_ROSE, "wither_rose", ItemProperties().maxStackSize(64));
    BROWN_MUSHROOM = &registerBlockBackedItem(
        registry, VanillaBlocks::BROWN_MUSHROOM, "brown_mushroom", ItemProperties().maxStackSize(64));
    RED_MUSHROOM = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_MUSHROOM, "red_mushroom", ItemProperties().maxStackSize(64));
    BROWN_MUSHROOM_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::BROWN_MUSHROOM_BLOCK, "brown_mushroom_block", ItemProperties().maxStackSize(64));
    RED_MUSHROOM_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_MUSHROOM_BLOCK, "red_mushroom_block", ItemProperties().maxStackSize(64));
    MUSHROOM_STEM = &registerBlockBackedItem(
        registry, VanillaBlocks::MUSHROOM_STEM, "mushroom_stem", ItemProperties().maxStackSize(64));
}

void Items::_registerRedstone()
{
    auto& registry = ItemRegistry::instance();

    // 注意：REDSTONE_WIRE 没有独立的物品，因为玩家持有的是 REDSTONE 物品
    // 红石粉放在地上时变成 REDSTONE_WIRE 方块
    REDSTONE_TORCH = &registerBlockBackedItem(
        registry, VanillaBlocks::REDSTONE_TORCH, "redstone_torch", ItemProperties().maxStackSize(64));
    REDSTONE_LAMP = &registerBlockBackedItem(
        registry, VanillaBlocks::REDSTONE_LAMP, "redstone_lamp", ItemProperties().maxStackSize(64));
    REDSTONE_REPEATER = &registerBlockBackedItem(
        registry, VanillaBlocks::REDSTONE_REPEATER, "repeater", ItemProperties().maxStackSize(64));
    REDSTONE_COMPARATOR = &registerBlockBackedItem(
        registry, VanillaBlocks::REDSTONE_COMPARATOR, "comparator", ItemProperties().maxStackSize(64));
    OBSERVER =
        &registerBlockBackedItem(registry, VanillaBlocks::OBSERVER, "observer", ItemProperties().maxStackSize(64));
    LEVER = &registerBlockBackedItem(registry, VanillaBlocks::LEVER, "lever", ItemProperties().maxStackSize(64));
    STONE_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_BUTTON, "stone_button", ItemProperties().maxStackSize(64));
    OAK_BUTTON =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_BUTTON, "oak_button", ItemProperties().maxStackSize(64));
    SPRUCE_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_BUTTON, "spruce_button", ItemProperties().maxStackSize(64));
    BIRCH_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_BUTTON, "birch_button", ItemProperties().maxStackSize(64));
    JUNGLE_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_BUTTON, "jungle_button", ItemProperties().maxStackSize(64));
    ACACIA_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_BUTTON, "acacia_button", ItemProperties().maxStackSize(64));
    DARK_OAK_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_BUTTON, "dark_oak_button", ItemProperties().maxStackSize(64));
    CRIMSON_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_BUTTON, "crimson_button", ItemProperties().maxStackSize(64));
    WARPED_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_BUTTON, "warped_button", ItemProperties().maxStackSize(64));
    STONE_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_PRESSURE_PLATE, "stone_pressure_plate", ItemProperties().maxStackSize(64));
    OAK_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::OAK_PRESSURE_PLATE, "oak_pressure_plate", ItemProperties().maxStackSize(64));
    LIGHT_WEIGHTED_PRESSURE_PLATE = &registerBlockBackedItem(registry,
        VanillaBlocks::LIGHT_WEIGHTED_PRESSURE_PLATE,
        "light_weighted_pressure_plate",
        ItemProperties().maxStackSize(64));
    HEAVY_WEIGHTED_PRESSURE_PLATE = &registerBlockBackedItem(registry,
        VanillaBlocks::HEAVY_WEIGHTED_PRESSURE_PLATE,
        "heavy_weighted_pressure_plate",
        ItemProperties().maxStackSize(64));
    DAYLIGHT_DETECTOR = &registerBlockBackedItem(
        registry, VanillaBlocks::DAYLIGHT_DETECTOR, "daylight_detector", ItemProperties().maxStackSize(64));
    PISTON = &registerBlockBackedItem(registry, VanillaBlocks::PISTON, "piston", ItemProperties().maxStackSize(64));
    STICKY_PISTON = &registerBlockBackedItem(
        registry, VanillaBlocks::STICKY_PISTON, "sticky_piston", ItemProperties().maxStackSize(64));
    DISPENSER =
        &registerBlockBackedItem(registry, VanillaBlocks::DISPENSER, "dispenser", ItemProperties().maxStackSize(64));
    DROPPER = &registerBlockBackedItem(registry, VanillaBlocks::DROPPER, "dropper", ItemProperties().maxStackSize(64));
    NOTE_BLOCK =
        &registerBlockBackedItem(registry, VanillaBlocks::NOTE_BLOCK, "note_block", ItemProperties().maxStackSize(64));
    TNT = &registerBlockBackedItem(registry, VanillaBlocks::TNT, "tnt", ItemProperties().maxStackSize(64));
    TARGET = &registerBlockBackedItem(registry, VanillaBlocks::TARGET, "target", ItemProperties().maxStackSize(64));
    TRIPWIRE =
        &registerBlockBackedItem(registry, VanillaBlocks::TRIPWIRE, "tripwire", ItemProperties().maxStackSize(64));
    TRIPWIRE_HOOK = &registerBlockBackedItem(
        registry, VanillaBlocks::TRIPWIRE_HOOK, "tripwire_hook", ItemProperties().maxStackSize(64));

    // 铁轨
    RAIL = &registerBlockBackedItem(registry, VanillaBlocks::RAIL, "rail", ItemProperties().maxStackSize(64));
    POWERED_RAIL = &registerBlockBackedItem(
        registry, VanillaBlocks::POWERED_RAIL, "powered_rail", ItemProperties().maxStackSize(64));
    DETECTOR_RAIL = &registerBlockBackedItem(
        registry, VanillaBlocks::DETECTOR_RAIL, "detector_rail", ItemProperties().maxStackSize(64));
    ACTIVATOR_RAIL = &registerBlockBackedItem(
        registry, VanillaBlocks::ACTIVATOR_RAIL, "activator_rail", ItemProperties().maxStackSize(64));
}

void Items::_registerCoral()
{
    auto& registry = ItemRegistry::instance();

    // 珊瑚方块 - 活
    TUBE_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::TUBE_CORAL_BLOCK, "tube_coral_block", ItemProperties().maxStackSize(64));
    BRAIN_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::BRAIN_CORAL_BLOCK, "brain_coral_block", ItemProperties().maxStackSize(64));
    BUBBLE_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::BUBBLE_CORAL_BLOCK, "bubble_coral_block", ItemProperties().maxStackSize(64));
    FIRE_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::FIRE_CORAL_BLOCK, "fire_coral_block", ItemProperties().maxStackSize(64));
    HORN_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::HORN_CORAL_BLOCK, "horn_coral_block", ItemProperties().maxStackSize(64));

    // 珊瑚方块 - 死
    DEAD_TUBE_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_TUBE_CORAL_BLOCK, "dead_tube_coral_block", ItemProperties().maxStackSize(64));
    DEAD_BRAIN_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_BRAIN_CORAL_BLOCK, "dead_brain_coral_block", ItemProperties().maxStackSize(64));
    DEAD_BUBBLE_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_BUBBLE_CORAL_BLOCK, "dead_bubble_coral_block", ItemProperties().maxStackSize(64));
    DEAD_FIRE_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_FIRE_CORAL_BLOCK, "dead_fire_coral_block", ItemProperties().maxStackSize(64));
    DEAD_HORN_CORAL_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_HORN_CORAL_BLOCK, "dead_horn_coral_block", ItemProperties().maxStackSize(64));

    // 珊瑚扇 - 活
    TUBE_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::TUBE_CORAL_FAN, "tube_coral_fan", ItemProperties().maxStackSize(64));
    BRAIN_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::BRAIN_CORAL_FAN, "brain_coral_fan", ItemProperties().maxStackSize(64));
    BUBBLE_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::BUBBLE_CORAL_FAN, "bubble_coral_fan", ItemProperties().maxStackSize(64));
    FIRE_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::FIRE_CORAL_FAN, "fire_coral_fan", ItemProperties().maxStackSize(64));
    HORN_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::HORN_CORAL_FAN, "horn_coral_fan", ItemProperties().maxStackSize(64));

    // 珊瑚扇 - 死
    DEAD_TUBE_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_TUBE_CORAL_FAN, "dead_tube_coral_fan", ItemProperties().maxStackSize(64));
    DEAD_BRAIN_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_BRAIN_CORAL_FAN, "dead_brain_coral_fan", ItemProperties().maxStackSize(64));
    DEAD_BUBBLE_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_BUBBLE_CORAL_FAN, "dead_bubble_coral_fan", ItemProperties().maxStackSize(64));
    DEAD_FIRE_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_FIRE_CORAL_FAN, "dead_fire_coral_fan", ItemProperties().maxStackSize(64));
    DEAD_HORN_CORAL_FAN = &registerBlockBackedItem(
        registry, VanillaBlocks::DEAD_HORN_CORAL_FAN, "dead_horn_coral_fan", ItemProperties().maxStackSize(64));
}

void Items::_registerDoorsFencesStairs()
{
    auto& registry = ItemRegistry::instance();

    // 门、栅栏、活板门
    OAK_DOOR =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_DOOR, "oak_door", ItemProperties().maxStackSize(64));
    IRON_DOOR =
        &registerBlockBackedItem(registry, VanillaBlocks::IRON_DOOR, "iron_door", ItemProperties().maxStackSize(64));
    OAK_FENCE =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_FENCE, "oak_fence", ItemProperties().maxStackSize(64));
    OAK_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::OAK_FENCE_GATE, "oak_fence_gate", ItemProperties().maxStackSize(64));
    OAK_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::OAK_TRAPDOOR, "oak_trapdoor", ItemProperties().maxStackSize(64));
    IRON_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::IRON_TRAPDOOR, "iron_trapdoor", ItemProperties().maxStackSize(64));

    // 楼梯、台阶、墙
    OAK_STAIRS =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_STAIRS, "oak_stairs", ItemProperties().maxStackSize(64));
    STONE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_STAIRS, "stone_stairs", ItemProperties().maxStackSize(64));
    COBBLESTONE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::COBBLESTONE_STAIRS, "cobblestone_stairs", ItemProperties().maxStackSize(64));
    STONE_BRICK_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_BRICK_STAIRS, "stone_brick_stairs", ItemProperties().maxStackSize(64));
    MOSSY_STONE_BRICK_STAIRS = &registerBlockBackedItem(registry,
        VanillaBlocks::MOSSY_STONE_BRICK_STAIRS,
        "mossy_stone_brick_stairs",
        ItemProperties().maxStackSize(64));
    OAK_SLAB =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_SLAB, "oak_slab", ItemProperties().maxStackSize(64));
    STONE_SLAB =
        &registerBlockBackedItem(registry, VanillaBlocks::STONE_SLAB, "stone_slab", ItemProperties().maxStackSize(64));
    COBBLESTONE_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::COBBLESTONE_SLAB, "cobblestone_slab", ItemProperties().maxStackSize(64));
    STONE_BRICK_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_BRICK_SLAB, "stone_brick_slab", ItemProperties().maxStackSize(64));
    MOSSY_STONE_BRICK_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::MOSSY_STONE_BRICK_SLAB, "mossy_stone_brick_slab", ItemProperties().maxStackSize(64));
    COBBLESTONE_WALL = &registerBlockBackedItem(
        registry, VanillaBlocks::COBBLESTONE_WALL, "cobblestone_wall", ItemProperties().maxStackSize(64));
    STONE_BRICK_WALL = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_BRICK_WALL, "stone_brick_wall", ItemProperties().maxStackSize(64));
    MOSSY_STONE_BRICK_WALL = &registerBlockBackedItem(
        registry, VanillaBlocks::MOSSY_STONE_BRICK_WALL, "mossy_stone_brick_wall", ItemProperties().maxStackSize(64));
}

void Items::_registerTrialChamberItems()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 试炼钥匙和不祥试炼钥匙
    // ========================================================================

    // 试炼钥匙 - 右键宝库时消耗，解锁普通宝库战利品
    // 试炼刷怪笼击杀所有怪物后50%概率弹出
    TRIAL_KEY = &registry.registerItem<item::TrialKeyItem>(
        ResourceLocation("minecraft:trial_key"), ItemProperties().maxStackSize(64));

    // 不祥试炼钥匙 - 右键不祥宝库时消耗，解锁不祥宝库战利品
    // 不祥试炼刷怪笼击杀所有怪物后30%概率弹出
    OMINOUS_TRIAL_KEY = &registry.registerItem<item::OminousTrialKeyItem>(
        ResourceLocation("minecraft:ominous_trial_key"), ItemProperties().maxStackSize(64));

    // ========================================================================
    // 不祥之瓶
    // ========================================================================

    // 不祥之瓶 - 可饮用，饮用后给予不祥之兆效果（等级I-V由damage值0-4决定）
    // 饮用后返还玻璃瓶
    OMINOUS_BOTTLE = &registry.registerItem<item::OminousBottleItem>(ResourceLocation("minecraft:ominous_bottle"),
        ItemProperties().maxStackSize(64).maxDamage(item::OminousBottleItem::MAX_DAMAGE));

    // ========================================================================
    // 风弹
    // ========================================================================

    // 风弹 - 右键投掷风弹弹射物实体，命中时产生风爆效果
    // 旋风人掉落（0-1，受抢夺影响），试炼刷怪笼补给
    WIND_CHARGE = &registry.registerItem<item::WindChargeItem>(
        ResourceLocation("minecraft:wind_charge"), ItemProperties().maxStackSize(64));

    // ========================================================================
    // 重锤
    // ========================================================================

    // 重锤 - MC 1.21 新增重型近战武器
    // 攻击伤害5，攻击速度-2.4，下落攻击加成3/格，最大额外伤害40
    // 支持魔咒：破甲、致密、风爆
    MACE = &registry.registerItem<item::MaceItem>(ResourceLocation("minecraft:mace"),
        ItemProperties().maxDamage(item::MaceItem::MAX_DURABILITY).rarity(ItemRarity::Rare));

    // ========================================================================
    // 旗帜图案物品
    // ========================================================================

    // 旋风旗帜图案 - 试炼密室宝库独有战利品
    GUSTER_BANNER_PATTERN =
        &registry.registerItem<item::BannerPatternItem>(ResourceLocation("minecraft:guster_banner_pattern"),
            blockentity::BannerPatternType::Guster,
            ItemProperties().maxStackSize(1));

    // 涡流旗帜图案 - 不祥宝库独有战利品
    FLOW_BANNER_PATTERN =
        &registry.registerItem<item::BannerPatternItem>(ResourceLocation("minecraft:flow_banner_pattern"),
            blockentity::BannerPatternType::Flow,
            ItemProperties().maxStackSize(1));

    // ========================================================================
    // 锻造模板物品
    // ========================================================================

    // 镶铆盔甲纹饰锻造模板 - 试炼密室宝库独有战利品
    // TODO(trial_chambers): 实现锻造模板系统后替换为专用SmithingTemplateItem类
    RIB_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem(
        ResourceLocation("minecraft:rib_armor_trim_smithing_template"), ItemProperties().maxStackSize(64));

    // 涡流盔甲纹饰锻造模板 - 不祥宝库独有战利品
    // TODO(trial_chambers): 实现锻造模板系统后替换为专用SmithingTemplateItem类
    FLOW_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem(
        ResourceLocation("minecraft:flow_armor_trim_smithing_template"), ItemProperties().maxStackSize(64));

    // ========================================================================
    // 陶片物品
    // ========================================================================

    // 涡流纹样陶片 - 试炼密室饰纹陶罐掉落
    // TODO(trial_chambers): 实现陶片/饰纹陶罐系统后替换为专用PotterySherdItem类
    FLOW_POTTERY_SHERD =
        &registry.registerItem(ResourceLocation("minecraft:flow_pottery_sherd"), ItemProperties().maxStackSize(64));

    // 旋风纹样陶片 - 试炼密室饰纹陶罐掉落
    // TODO(trial_chambers): 实现陶片/饰纹陶罐系统后替换为专用PotterySherdItem类
    GUSTER_POTTERY_SHERD =
        &registry.registerItem(ResourceLocation("minecraft:guster_pottery_sherd"), ItemProperties().maxStackSize(64));

    // 刮削纹样陶片 - 试炼密室饰纹陶罐掉落
    // TODO(trial_chambers): 实现陶片/饰纹陶罐系统后替换为专用PotterySherdItem类
    SCRAPE_POTTERY_SHERD =
        &registry.registerItem(ResourceLocation("minecraft:scrape_pottery_sherd"), ItemProperties().maxStackSize(64));

    // ========================================================================
    // 音乐唱片
    // ========================================================================

    // 音乐唱片 (Creator) - 不祥宝库独有战利品
    // TODO(trial_chambers): 实现音乐唱片播放逻辑
    MUSIC_DISC_CREATOR = &registry.registerItem(
        ResourceLocation("minecraft:music_disc_creator"), ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    // 音乐唱片 (Creator 八音盒) - 试炼密室柱廊陶罐掉落
    // TODO(trial_chambers): 实现音乐唱片播放逻辑
    MUSIC_DISC_CREATOR_MUSIC_BOX = &registry.registerItem(ResourceLocation("minecraft:music_disc_creator_music_box"),
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    // 音乐唱片 (Precipice) - 试炼密室宝库独有战利品
    // TODO(trial_chambers): 实现音乐唱片播放逻辑
    MUSIC_DISC_PRECIPICE = &registry.registerItem(
        ResourceLocation("minecraft:music_disc_precipice"), ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));
}

} // namespace mc
