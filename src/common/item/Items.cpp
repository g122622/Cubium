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
#include "common/item/items/armor/NautilusArmorItem.hpp"
#include "common/item/items/armor/WolfArmorItem.hpp"
#include "common/item/items/block/BannerItem.hpp"
#include "common/item/items/block/BedItem.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/SeedsItem.hpp"
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
#include "common/item/items/special/BrushItem.hpp"
#include "common/item/items/special/BucketItem.hpp"
#include "common/item/items/special/EnchantedBookItem.hpp"
#include "common/item/items/special/FishBucketItem.hpp"
#include "common/item/items/special/FlintAndSteelItem.hpp"
#include "common/item/items/special/HarnessItem.hpp"
#include "common/item/items/special/HoneycombItem.hpp"
#include "common/item/items/special/KnowledgeBookItem.hpp"
#include "common/item/items/special/LeadItem.hpp"
#include "common/item/items/special/MilkBucketItem.hpp"
#include "common/item/items/special/MusicDiscItem.hpp"
#include "common/item/items/special/NameTagItem.hpp"
#include "common/item/items/special/PotterySherdItem.hpp"
#include "common/item/items/special/PowderSnowBucketItem.hpp"
#include "common/item/items/special/SaddleItem.hpp"
#include "common/item/items/special/SmithingTemplateItem.hpp"
#include "common/item/items/special/SpawnEggItem.hpp"
#include "common/item/items/special/StickItems.hpp"
#include "common/item/items/special/bundle/BundleItem.hpp"
#include "common/item/items/tool/AxeItem.hpp"
#include "common/item/items/tool/HoeItem.hpp"
#include "common/item/items/tool/PickaxeItem.hpp"
#include "common/item/items/tool/ShearsItem.hpp"
#include "common/item/items/tool/ShovelItem.hpp"
#include "common/item/items/tool/SwordItem.hpp"
#include "common/item/items/trial/BreezeRodItem.hpp"
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
#include "common/item/items/weapon/FireChargeItem.hpp"
#include "common/item/items/weapon/FireworkRocketItem.hpp"
#include "common/item/items/weapon/FishingRodItem.hpp"
#include "common/item/items/weapon/ShieldItem.hpp"
#include "common/item/items/weapon/SpearItem.hpp"
#include "common/item/items/weapon/SpectralArrowItem.hpp"
#include "common/item/items/weapon/ThrowableItem.hpp"
#include "common/item/items/weapon/ThrowableItems.hpp"
#include "common/item/items/weapon/TippedArrowItem.hpp"
#include "common/item/items/weapon/TridentItem.hpp"
#include "common/item/tier/ItemTiers.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/color/DyeColor.hpp"
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

/**
 * @brief 为刷怪蛋物品构造一个轻量级的 EntityType
 *
 * SpawnEggItem 持有 EntityType 副本（不可拷贝、可移动），其中存储刷怪蛋对应
 * 实体的注册名（如 "minecraft:pig"）。运行期生成实体时，MobEntity 会通过
 * EntityRegistry::getType(name) 查找真实的 EntityType 并调用其工厂，因此
 * SpawnEggItem 内部持有的 EntityType 不需要带工厂，仅作为名称载体即可。
 *
 * 这里使用 EntityType::Builder 构造一个最小化的 EntityType（工厂为空、可序列化），
 * 然后通过 const_cast 写入注册名（与 EntityRegistry::registerType 内部一致）。
 *
 * @param entityName 实体注册名（如 "minecraft:pig"）
 * @return 可移动的 EntityType 实例
 */
mc::entity::EntityType makeEntityTypeForSpawnEgg(const char* entityName)
{
    auto entityType = mc::entity::EntityType::Builder(
        [](mc::IWorld*) -> std::unique_ptr<mc::Entity> { return nullptr; }, mc::entity::EntityClassification::Creature)
                          .size(0.6f, 1.8f)
                          .trackingRange(5)
                          .updateInterval(3)
                          .canSummon(true)
                          .build();
    const_cast<std::string&>(entityType.name()) = entityName;
    return entityType;
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
Item* Items::BAMBOO_BLOCK = nullptr;
Item* Items::STRIPPED_BAMBOO_BLOCK = nullptr;

Item* Items::OAK_PLANKS = nullptr;
Item* Items::SPRUCE_PLANKS = nullptr;
Item* Items::BIRCH_PLANKS = nullptr;
Item* Items::JUNGLE_PLANKS = nullptr;
Item* Items::ACACIA_PLANKS = nullptr;
Item* Items::DARK_OAK_PLANKS = nullptr;
Item* Items::BAMBOO_PLANKS = nullptr;
Item* Items::BAMBOO_MOSAIC = nullptr;

// 其他木板变体
Item* Items::CRIMSON_PLANKS = nullptr;
Item* Items::WARPED_PLANKS = nullptr;
Item* Items::MANGROVE_PLANKS = nullptr;
Item* Items::CHERRY_PLANKS = nullptr;
Item* Items::PALE_OAK_PLANKS = nullptr;

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
Item* Items::BRICK = nullptr;
Item* Items::RESIN_BRICK = nullptr;
Item* Items::AMETHYST_SHARD = nullptr;

// 粗矿（Raw Ore）
Item* Items::RAW_IRON = nullptr;
Item* Items::RAW_COPPER = nullptr;
Item* Items::RAW_GOLD = nullptr;

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

// 深板岩矿物原矿
Item* Items::DEEPSLATE_COAL_ORE = nullptr;
Item* Items::DEEPSLATE_IRON_ORE = nullptr;
Item* Items::DEEPSLATE_COPPER_ORE = nullptr;
Item* Items::DEEPSLATE_GOLD_ORE = nullptr;
Item* Items::DEEPSLATE_DIAMOND_ORE = nullptr;
Item* Items::DEEPSLATE_LAPIS_ORE = nullptr;
Item* Items::DEEPSLATE_EMERALD_ORE = nullptr;
Item* Items::DEEPSLATE_REDSTONE_ORE = nullptr;

// 粗矿块（Raw Ore Block）
Item* Items::RAW_IRON_BLOCK = nullptr;
Item* Items::RAW_COPPER_BLOCK = nullptr;
Item* Items::RAW_GOLD_BLOCK = nullptr;

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

// 铜工具（MC 1.21.11 新增）
Item* Items::COPPER_PICKAXE = nullptr;
Item* Items::COPPER_AXE = nullptr;
Item* Items::COPPER_SHOVEL = nullptr;
Item* Items::COPPER_HOE = nullptr;
Item* Items::COPPER_SWORD = nullptr;

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

// 铜护甲
Item* Items::COPPER_HELMET = nullptr;
Item* Items::COPPER_CHESTPLATE = nullptr;
Item* Items::COPPER_LEGGINGS = nullptr;
Item* Items::COPPER_BOOTS = nullptr;

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
Item* Items::COPPER_HORSE_ARMOR = nullptr;
Item* Items::IRON_HORSE_ARMOR = nullptr;
Item* Items::GOLDEN_HORSE_ARMOR = nullptr;
Item* Items::DIAMOND_HORSE_ARMOR = nullptr;
Item* Items::NETHERITE_HORSE_ARMOR = nullptr;

// 狼铠
Item* Items::WOLF_ARMOR = nullptr;

// 鹦鹉螺铠甲
Item* Items::COPPER_NAUTILUS_ARMOR = nullptr;
Item* Items::IRON_NAUTILUS_ARMOR = nullptr;
Item* Items::GOLDEN_NAUTILUS_ARMOR = nullptr;
Item* Items::DIAMOND_NAUTILUS_ARMOR = nullptr;
Item* Items::NETHERITE_NAUTILUS_ARMOR = nullptr;

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
Item* Items::BRUSH = nullptr;
Item* Items::HONEYCOMB = nullptr;
Item* Items::BELL = nullptr;
Item* Items::SUSPICIOUS_SAND = nullptr;
Item* Items::SUSPICIOUS_GRAVEL = nullptr;
Item* Items::NAME_TAG = nullptr;
Item* Items::SADDLE = nullptr;
Item* Items::STRING = nullptr;
Item* Items::FEATHER = nullptr;
Item* Items::GUNPOWDER = nullptr;
Item* Items::LEATHER = nullptr;
Item* Items::RABBIT_HIDE = nullptr;
Item* Items::SLIME_BALL = nullptr;
Item* Items::EGG = nullptr;
Item* Items::BLUE_EGG = nullptr;
Item* Items::BROWN_EGG = nullptr;
Item* Items::SNOWBALL = nullptr;
Item* Items::COMPASS = nullptr;
Item* Items::RECOVERY_COMPASS = nullptr;
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
Item* Items::TORCHFLOWER_SEEDS = nullptr;
Item* Items::PITCHER_POD = nullptr;

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
Item* Items::TURTLE_SCUTE = nullptr;
Item* Items::ARMADILLO_SCUTE = nullptr;
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
// 长矛 - 按材质分层
// ============================================================================
Item* Items::WOODEN_SPEAR = nullptr;
Item* Items::STONE_SPEAR = nullptr;
Item* Items::COPPER_SPEAR = nullptr;
Item* Items::IRON_SPEAR = nullptr;
Item* Items::GOLDEN_SPEAR = nullptr;
Item* Items::DIAMOND_SPEAR = nullptr;
Item* Items::NETHERITE_SPEAR = nullptr;

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
Item* Items::POWDER_SNOW_BUCKET = nullptr;
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
Item* Items::KNOWLEDGE_BOOK = nullptr;

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
// 船（10种木材类型）
// ============================================================================
Item* Items::OAK_BOAT = nullptr;
Item* Items::SPRUCE_BOAT = nullptr;
Item* Items::BIRCH_BOAT = nullptr;
Item* Items::JUNGLE_BOAT = nullptr;
Item* Items::ACACIA_BOAT = nullptr;
Item* Items::DARK_OAK_BOAT = nullptr;
Item* Items::MANGROVE_BOAT = nullptr;
Item* Items::CHERRY_BOAT = nullptr;
Item* Items::PALE_OAK_BOAT = nullptr;
Item* Items::BAMBOO_RAFT = nullptr;

// ============================================================================
// 带箱子的船（10种木材类型）
// ============================================================================
Item* Items::OAK_CHEST_BOAT = nullptr;
Item* Items::SPRUCE_CHEST_BOAT = nullptr;
Item* Items::BIRCH_CHEST_BOAT = nullptr;
Item* Items::JUNGLE_CHEST_BOAT = nullptr;
Item* Items::ACACIA_CHEST_BOAT = nullptr;
Item* Items::DARK_OAK_CHEST_BOAT = nullptr;
Item* Items::MANGROVE_CHEST_BOAT = nullptr;
Item* Items::CHERRY_CHEST_BOAT = nullptr;
Item* Items::PALE_OAK_CHEST_BOAT = nullptr;
Item* Items::BAMBOO_CHEST_RAFT = nullptr;

// ============================================================================
// 悬挂实体物品
// ============================================================================
Item* Items::PAINTING = nullptr;
Item* Items::ITEM_FRAME = nullptr;
Item* Items::LEAD = nullptr;

// ============================================================================
// 告示牌物品（12种木材类型）
// ============================================================================
Item* Items::OAK_SIGN = nullptr;
Item* Items::SPRUCE_SIGN = nullptr;
Item* Items::BIRCH_SIGN = nullptr;
Item* Items::JUNGLE_SIGN = nullptr;
Item* Items::ACACIA_SIGN = nullptr;
Item* Items::DARK_OAK_SIGN = nullptr;
Item* Items::CRIMSON_SIGN = nullptr;
Item* Items::WARPED_SIGN = nullptr;
Item* Items::MANGROVE_SIGN = nullptr;
Item* Items::CHERRY_SIGN = nullptr;
Item* Items::BAMBOO_SIGN = nullptr;
Item* Items::PALE_OAK_SIGN = nullptr;

// ============================================================================
// 悬挂告示牌物品（12种木材类型）
// ============================================================================
Item* Items::OAK_HANGING_SIGN = nullptr;
Item* Items::SPRUCE_HANGING_SIGN = nullptr;
Item* Items::BIRCH_HANGING_SIGN = nullptr;
Item* Items::JUNGLE_HANGING_SIGN = nullptr;
Item* Items::ACACIA_HANGING_SIGN = nullptr;
Item* Items::DARK_OAK_HANGING_SIGN = nullptr;
Item* Items::CRIMSON_HANGING_SIGN = nullptr;
Item* Items::WARPED_HANGING_SIGN = nullptr;
Item* Items::MANGROVE_HANGING_SIGN = nullptr;
Item* Items::CHERRY_HANGING_SIGN = nullptr;
Item* Items::BAMBOO_HANGING_SIGN = nullptr;
Item* Items::PALE_OAK_HANGING_SIGN = nullptr;

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

// 木质书架变体（1.21.4+）
Item* Items::OAK_SHELF = nullptr;
Item* Items::SPRUCE_SHELF = nullptr;
Item* Items::BIRCH_SHELF = nullptr;
Item* Items::JUNGLE_SHELF = nullptr;
Item* Items::ACACIA_SHELF = nullptr;
Item* Items::DARK_OAK_SHELF = nullptr;
Item* Items::MANGROVE_SHELF = nullptr;
Item* Items::CHERRY_SHELF = nullptr;
Item* Items::PALE_OAK_SHELF = nullptr;
Item* Items::BAMBOO_SHELF = nullptr;
Item* Items::CRIMSON_SHELF = nullptr;
Item* Items::WARPED_SHELF = nullptr;

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
Item* Items::TORCH = nullptr;
Item* Items::SOUL_TORCH = nullptr;

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
Item* Items::SMOOTH_SANDSTONE = nullptr;
Item* Items::RED_SANDSTONE = nullptr;
Item* Items::CHISELED_RED_SANDSTONE = nullptr;
Item* Items::CUT_RED_SANDSTONE = nullptr;
Item* Items::SMOOTH_RED_SANDSTONE = nullptr;

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
Item* Items::GILDED_BLACKSTONE = nullptr;
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
Item* Items::TORCHFLOWER = nullptr;
Item* Items::PITCHER_PLANT = nullptr;
Item* Items::CACTUS_FLOWER = nullptr;
Item* Items::WILDFLOWERS = nullptr;
Item* Items::OPEN_EYEBLOSSOM = nullptr;
Item* Items::CLOSED_EYEBLOSSOM = nullptr;
Item* Items::BROWN_MUSHROOM = nullptr;
Item* Items::RED_MUSHROOM = nullptr;
Item* Items::BROWN_MUSHROOM_BLOCK = nullptr;
Item* Items::RED_MUSHROOM_BLOCK = nullptr;
Item* Items::MUSHROOM_STEM = nullptr;

// 花盆
Item* Items::FLOWER_POT = nullptr;

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

// 床 (16色)
Item* Items::WHITE_BED = nullptr;
Item* Items::ORANGE_BED = nullptr;
Item* Items::MAGENTA_BED = nullptr;
Item* Items::LIGHT_BLUE_BED = nullptr;
Item* Items::YELLOW_BED = nullptr;
Item* Items::LIME_BED = nullptr;
Item* Items::PINK_BED = nullptr;
Item* Items::GRAY_BED = nullptr;
Item* Items::LIGHT_GRAY_BED = nullptr;
Item* Items::CYAN_BED = nullptr;
Item* Items::PURPLE_BED = nullptr;
Item* Items::BLUE_BED = nullptr;
Item* Items::BROWN_BED = nullptr;
Item* Items::GREEN_BED = nullptr;
Item* Items::RED_BED = nullptr;
Item* Items::BLACK_BED = nullptr;

// 欢乐诡鬼装备 (Harness, 16色)
Item* Items::WHITE_HARNESS = nullptr;
Item* Items::ORANGE_HARNESS = nullptr;
Item* Items::MAGENTA_HARNESS = nullptr;
Item* Items::LIGHT_BLUE_HARNESS = nullptr;
Item* Items::YELLOW_HARNESS = nullptr;
Item* Items::LIME_HARNESS = nullptr;
Item* Items::PINK_HARNESS = nullptr;
Item* Items::GRAY_HARNESS = nullptr;
Item* Items::LIGHT_GRAY_HARNESS = nullptr;
Item* Items::CYAN_HARNESS = nullptr;
Item* Items::PURPLE_HARNESS = nullptr;
Item* Items::BLUE_HARNESS = nullptr;
Item* Items::BROWN_HARNESS = nullptr;
Item* Items::GREEN_HARNESS = nullptr;
Item* Items::RED_HARNESS = nullptr;
Item* Items::BLACK_HARNESS = nullptr;

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
Item* Items::WHITE_SHULKER_BOX = nullptr;
Item* Items::ORANGE_SHULKER_BOX = nullptr;
Item* Items::MAGENTA_SHULKER_BOX = nullptr;
Item* Items::LIGHT_BLUE_SHULKER_BOX = nullptr;
Item* Items::YELLOW_SHULKER_BOX = nullptr;
Item* Items::LIME_SHULKER_BOX = nullptr;
Item* Items::PINK_SHULKER_BOX = nullptr;
Item* Items::GRAY_SHULKER_BOX = nullptr;
Item* Items::LIGHT_GRAY_SHULKER_BOX = nullptr;
Item* Items::CYAN_SHULKER_BOX = nullptr;
Item* Items::PURPLE_SHULKER_BOX = nullptr;
Item* Items::BLUE_SHULKER_BOX = nullptr;
Item* Items::BROWN_SHULKER_BOX = nullptr;
Item* Items::GREEN_SHULKER_BOX = nullptr;
Item* Items::RED_SHULKER_BOX = nullptr;
Item* Items::BLACK_SHULKER_BOX = nullptr;
Item* Items::BEACON = nullptr;
Item* Items::LANTERN = nullptr;
Item* Items::SOUL_LANTERN = nullptr;
Item* Items::CAMPFIRE = nullptr;
Item* Items::SOUL_CAMPFIRE = nullptr;
Item* Items::JACK_O_LANTERN = nullptr;
Item* Items::CANDLE = nullptr;
Item* Items::WHITE_CANDLE = nullptr;
Item* Items::ORANGE_CANDLE = nullptr;
Item* Items::MAGENTA_CANDLE = nullptr;
Item* Items::LIGHT_BLUE_CANDLE = nullptr;
Item* Items::YELLOW_CANDLE = nullptr;
Item* Items::LIME_CANDLE = nullptr;
Item* Items::PINK_CANDLE = nullptr;
Item* Items::GRAY_CANDLE = nullptr;
Item* Items::LIGHT_GRAY_CANDLE = nullptr;
Item* Items::CYAN_CANDLE = nullptr;
Item* Items::PURPLE_CANDLE = nullptr;
Item* Items::BLUE_CANDLE = nullptr;
Item* Items::BROWN_CANDLE = nullptr;
Item* Items::GREEN_CANDLE = nullptr;
Item* Items::RED_CANDLE = nullptr;
Item* Items::BLACK_CANDLE = nullptr;
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
Item* Items::MANGROVE_BUTTON = nullptr;
Item* Items::CHERRY_BUTTON = nullptr;
Item* Items::BAMBOO_BUTTON = nullptr;
Item* Items::PALE_OAK_BUTTON = nullptr;
Item* Items::POLISHED_BLACKSTONE_BUTTON = nullptr;
Item* Items::STONE_PRESSURE_PLATE = nullptr;
Item* Items::OAK_PRESSURE_PLATE = nullptr;
Item* Items::SPRUCE_PRESSURE_PLATE = nullptr;
Item* Items::BIRCH_PRESSURE_PLATE = nullptr;
Item* Items::JUNGLE_PRESSURE_PLATE = nullptr;
Item* Items::ACACIA_PRESSURE_PLATE = nullptr;
Item* Items::DARK_OAK_PRESSURE_PLATE = nullptr;
Item* Items::CRIMSON_PRESSURE_PLATE = nullptr;
Item* Items::WARPED_PRESSURE_PLATE = nullptr;
Item* Items::MANGROVE_PRESSURE_PLATE = nullptr;
Item* Items::CHERRY_PRESSURE_PLATE = nullptr;
Item* Items::BAMBOO_PRESSURE_PLATE = nullptr;
Item* Items::PALE_OAK_PRESSURE_PLATE = nullptr;
Item* Items::POLISHED_BLACKSTONE_PRESSURE_PLATE = nullptr;
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
Item* Items::SPRUCE_DOOR = nullptr;
Item* Items::BIRCH_DOOR = nullptr;
Item* Items::JUNGLE_DOOR = nullptr;
Item* Items::ACACIA_DOOR = nullptr;
Item* Items::DARK_OAK_DOOR = nullptr;
Item* Items::MANGROVE_DOOR = nullptr;
Item* Items::CHERRY_DOOR = nullptr;
Item* Items::PALE_OAK_DOOR = nullptr;
Item* Items::BAMBOO_DOOR = nullptr;
Item* Items::CRIMSON_DOOR = nullptr;
Item* Items::WARPED_DOOR = nullptr;
Item* Items::IRON_DOOR = nullptr;
// 铜门
Item* Items::COPPER_DOOR = nullptr;
Item* Items::EXPOSED_COPPER_DOOR = nullptr;
Item* Items::WEATHERED_COPPER_DOOR = nullptr;
Item* Items::OXIDIZED_COPPER_DOOR = nullptr;
Item* Items::WAXED_COPPER_DOOR = nullptr;
Item* Items::WAXED_EXPOSED_COPPER_DOOR = nullptr;
Item* Items::WAXED_WEATHERED_COPPER_DOOR = nullptr;
Item* Items::WAXED_OXIDIZED_COPPER_DOOR = nullptr;
Item* Items::OAK_FENCE = nullptr;
Item* Items::SPRUCE_FENCE = nullptr;
Item* Items::BIRCH_FENCE = nullptr;
Item* Items::JUNGLE_FENCE = nullptr;
Item* Items::ACACIA_FENCE = nullptr;
Item* Items::DARK_OAK_FENCE = nullptr;
Item* Items::MANGROVE_FENCE = nullptr;
Item* Items::CHERRY_FENCE = nullptr;
Item* Items::PALE_OAK_FENCE = nullptr;
Item* Items::BAMBOO_FENCE = nullptr;
Item* Items::NETHER_BRICK_FENCE = nullptr;
Item* Items::OAK_FENCE_GATE = nullptr;
Item* Items::SPRUCE_FENCE_GATE = nullptr;
Item* Items::BIRCH_FENCE_GATE = nullptr;
Item* Items::JUNGLE_FENCE_GATE = nullptr;
Item* Items::ACACIA_FENCE_GATE = nullptr;
Item* Items::DARK_OAK_FENCE_GATE = nullptr;
Item* Items::MANGROVE_FENCE_GATE = nullptr;
Item* Items::CHERRY_FENCE_GATE = nullptr;
Item* Items::PALE_OAK_FENCE_GATE = nullptr;
Item* Items::BAMBOO_FENCE_GATE = nullptr;
Item* Items::OAK_TRAPDOOR = nullptr;
Item* Items::SPRUCE_TRAPDOOR = nullptr;
Item* Items::BIRCH_TRAPDOOR = nullptr;
Item* Items::JUNGLE_TRAPDOOR = nullptr;
Item* Items::ACACIA_TRAPDOOR = nullptr;
Item* Items::DARK_OAK_TRAPDOOR = nullptr;
Item* Items::MANGROVE_TRAPDOOR = nullptr;
Item* Items::CHERRY_TRAPDOOR = nullptr;
Item* Items::PALE_OAK_TRAPDOOR = nullptr;
Item* Items::BAMBOO_TRAPDOOR = nullptr;
Item* Items::CRIMSON_TRAPDOOR = nullptr;
Item* Items::WARPED_TRAPDOOR = nullptr;
Item* Items::IRON_TRAPDOOR = nullptr;
// 铜活板门
Item* Items::COPPER_TRAPDOOR = nullptr;
Item* Items::EXPOSED_COPPER_TRAPDOOR = nullptr;
Item* Items::WEATHERED_COPPER_TRAPDOOR = nullptr;
Item* Items::OXIDIZED_COPPER_TRAPDOOR = nullptr;
Item* Items::WAXED_COPPER_TRAPDOOR = nullptr;
Item* Items::WAXED_EXPOSED_COPPER_TRAPDOOR = nullptr;
Item* Items::WAXED_WEATHERED_COPPER_TRAPDOOR = nullptr;
Item* Items::WAXED_OXIDIZED_COPPER_TRAPDOOR = nullptr;

// 楼梯、台阶、墙
Item* Items::OAK_STAIRS = nullptr;
Item* Items::SPRUCE_STAIRS = nullptr;
Item* Items::BIRCH_STAIRS = nullptr;
Item* Items::JUNGLE_STAIRS = nullptr;
Item* Items::ACACIA_STAIRS = nullptr;
Item* Items::DARK_OAK_STAIRS = nullptr;
Item* Items::STONE_STAIRS = nullptr;
Item* Items::COBBLESTONE_STAIRS = nullptr;
Item* Items::SANDSTONE_STAIRS = nullptr;
Item* Items::SMOOTH_SANDSTONE_STAIRS = nullptr;
Item* Items::STONE_BRICK_STAIRS = nullptr;
Item* Items::MOSSY_STONE_BRICK_STAIRS = nullptr;
Item* Items::OAK_SLAB = nullptr;
Item* Items::SPRUCE_SLAB = nullptr;
Item* Items::BIRCH_SLAB = nullptr;
Item* Items::JUNGLE_SLAB = nullptr;
Item* Items::ACACIA_SLAB = nullptr;
Item* Items::DARK_OAK_SLAB = nullptr;
Item* Items::STONE_SLAB = nullptr;
Item* Items::COBBLESTONE_SLAB = nullptr;
Item* Items::SANDSTONE_SLAB = nullptr;
Item* Items::SMOOTH_SANDSTONE_SLAB = nullptr;
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
Item* Items::BREEZE_ROD = nullptr;
Item* Items::MACE = nullptr;
Item* Items::GUSTER_BANNER_PATTERN = nullptr;
Item* Items::FLOW_BANNER_PATTERN = nullptr;

// 锻造模板物品
Item* Items::NETHERITE_UPGRADE_SMITHING_TEMPLATE = nullptr;
Item* Items::SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::VEX_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::WILD_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::COAST_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::DUNE_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::WAYFINDER_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::RAISER_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::SHAPER_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::HOST_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::WARD_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::SILENCE_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::TIDE_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::SNOUT_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::RIB_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::EYE_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::SPIRE_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::FLOW_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;
Item* Items::BOLT_ARMOR_TRIM_SMITHING_TEMPLATE = nullptr;

// 陶片物品
Item* Items::ANGLER_POTTERY_SHERD = nullptr;
Item* Items::ARCHER_POTTERY_SHERD = nullptr;
Item* Items::ARMS_UP_POTTERY_SHERD = nullptr;
Item* Items::BLADE_POTTERY_SHERD = nullptr;
Item* Items::BREWER_POTTERY_SHERD = nullptr;
Item* Items::BURN_POTTERY_SHERD = nullptr;
Item* Items::DANGER_POTTERY_SHERD = nullptr;
Item* Items::EXPLORER_POTTERY_SHERD = nullptr;
Item* Items::FRIEND_POTTERY_SHERD = nullptr;
Item* Items::HEART_POTTERY_SHERD = nullptr;
Item* Items::HEARTBREAK_POTTERY_SHERD = nullptr;
Item* Items::HOWL_POTTERY_SHERD = nullptr;
Item* Items::MINER_POTTERY_SHERD = nullptr;
Item* Items::MOURNER_POTTERY_SHERD = nullptr;
Item* Items::PLENTY_POTTERY_SHERD = nullptr;
Item* Items::PRIZE_POTTERY_SHERD = nullptr;
Item* Items::SHEAF_POTTERY_SHERD = nullptr;
Item* Items::SHELTER_POTTERY_SHERD = nullptr;
Item* Items::SKULL_POTTERY_SHERD = nullptr;
Item* Items::SNORT_POTTERY_SHERD = nullptr;
Item* Items::FLOW_POTTERY_SHERD = nullptr;
Item* Items::GUSTER_POTTERY_SHERD = nullptr;
Item* Items::SCRAPE_POTTERY_SHERD = nullptr;

// 音乐唱片
Item* Items::MUSIC_DISC_13 = nullptr;
Item* Items::MUSIC_DISC_CAT = nullptr;
Item* Items::MUSIC_DISC_BLOCKS = nullptr;
Item* Items::MUSIC_DISC_CHIRP = nullptr;
Item* Items::MUSIC_DISC_FAR = nullptr;
Item* Items::MUSIC_DISC_MALL = nullptr;
Item* Items::MUSIC_DISC_MELLOHI = nullptr;
Item* Items::MUSIC_DISC_STAL = nullptr;
Item* Items::MUSIC_DISC_STRAD = nullptr;
Item* Items::MUSIC_DISC_WARD = nullptr;
Item* Items::MUSIC_DISC_11 = nullptr;
Item* Items::MUSIC_DISC_WAIT = nullptr;
Item* Items::MUSIC_DISC_PIGSTEP = nullptr;
Item* Items::MUSIC_DISC_OTHERSIDE = nullptr;
Item* Items::MUSIC_DISC_5 = nullptr;
Item* Items::MUSIC_DISC_RELIC = nullptr;
Item* Items::MUSIC_DISC_TEARS = nullptr;
Item* Items::MUSIC_DISC_CREATOR = nullptr;
Item* Items::MUSIC_DISC_CREATOR_MUSIC_BOX = nullptr;
Item* Items::MUSIC_DISC_PRECIPICE = nullptr;
Item* Items::MUSIC_DISC_LAVA_CHICKEN = nullptr;

// 头颅物品
Item* Items::SKELETON_SKULL = nullptr;
Item* Items::WITHER_SKELETON_SKULL = nullptr;
Item* Items::PLAYER_HEAD = nullptr;
Item* Items::ZOMBIE_HEAD = nullptr;
Item* Items::CREEPER_HEAD = nullptr;
Item* Items::DRAGON_HEAD = nullptr;
Item* Items::PIGLIN_HEAD = nullptr;

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

// 收纳袋（1 无色 + 16 色）
Item* Items::BUNDLE = nullptr;
Item* Items::WHITE_BUNDLE = nullptr;
Item* Items::ORANGE_BUNDLE = nullptr;
Item* Items::MAGENTA_BUNDLE = nullptr;
Item* Items::LIGHT_BLUE_BUNDLE = nullptr;
Item* Items::YELLOW_BUNDLE = nullptr;
Item* Items::LIME_BUNDLE = nullptr;
Item* Items::PINK_BUNDLE = nullptr;
Item* Items::GRAY_BUNDLE = nullptr;
Item* Items::LIGHT_GRAY_BUNDLE = nullptr;
Item* Items::CYAN_BUNDLE = nullptr;
Item* Items::PURPLE_BUNDLE = nullptr;
Item* Items::BLUE_BUNDLE = nullptr;
Item* Items::BROWN_BUNDLE = nullptr;
Item* Items::GREEN_BUNDLE = nullptr;
Item* Items::RED_BUNDLE = nullptr;
Item* Items::BLACK_BUNDLE = nullptr;

// ============================================================================
// 刷怪蛋物品
// ============================================================================

Item* Items::ALLAY_SPAWN_EGG = nullptr;
Item* Items::ARMADILLO_SPAWN_EGG = nullptr;
Item* Items::AXOLOTL_SPAWN_EGG = nullptr;
Item* Items::BAT_SPAWN_EGG = nullptr;
Item* Items::BEE_SPAWN_EGG = nullptr;
Item* Items::BLAZE_SPAWN_EGG = nullptr;
Item* Items::BOGGED_SPAWN_EGG = nullptr;
Item* Items::BREEZE_SPAWN_EGG = nullptr;
Item* Items::CAMEL_HUSK_SPAWN_EGG = nullptr;
Item* Items::CAT_SPAWN_EGG = nullptr;
Item* Items::CAMEL_SPAWN_EGG = nullptr;
Item* Items::CAVE_SPIDER_SPAWN_EGG = nullptr;
Item* Items::CHICKEN_SPAWN_EGG = nullptr;
Item* Items::COD_SPAWN_EGG = nullptr;
Item* Items::COPPER_GOLEM_SPAWN_EGG = nullptr;
Item* Items::COW_SPAWN_EGG = nullptr;
Item* Items::CREAKING_SPAWN_EGG = nullptr;
Item* Items::CREEPER_SPAWN_EGG = nullptr;
Item* Items::DOLPHIN_SPAWN_EGG = nullptr;
Item* Items::DONKEY_SPAWN_EGG = nullptr;
Item* Items::DROWNED_SPAWN_EGG = nullptr;
Item* Items::ELDER_GUARDIAN_SPAWN_EGG = nullptr;
Item* Items::ENDER_DRAGON_SPAWN_EGG = nullptr;
Item* Items::ENDERMAN_SPAWN_EGG = nullptr;
Item* Items::ENDERMITE_SPAWN_EGG = nullptr;
Item* Items::EVOKER_SPAWN_EGG = nullptr;
Item* Items::FOX_SPAWN_EGG = nullptr;
Item* Items::FROG_SPAWN_EGG = nullptr;
Item* Items::GHAST_SPAWN_EGG = nullptr;
Item* Items::GLOW_SQUID_SPAWN_EGG = nullptr;
Item* Items::GOAT_SPAWN_EGG = nullptr;
Item* Items::GUARDIAN_SPAWN_EGG = nullptr;
Item* Items::HAPPY_GHAST_SPAWN_EGG = nullptr;
Item* Items::HOGLIN_SPAWN_EGG = nullptr;
Item* Items::HORSE_SPAWN_EGG = nullptr;
Item* Items::HUSK_SPAWN_EGG = nullptr;
Item* Items::IRON_GOLEM_SPAWN_EGG = nullptr;
Item* Items::LLAMA_SPAWN_EGG = nullptr;
Item* Items::MAGMA_CUBE_SPAWN_EGG = nullptr;
Item* Items::MOOSHROOM_SPAWN_EGG = nullptr;
Item* Items::MULE_SPAWN_EGG = nullptr;
Item* Items::NAUTILUS_SPAWN_EGG = nullptr;
Item* Items::OCELOT_SPAWN_EGG = nullptr;
Item* Items::PANDA_SPAWN_EGG = nullptr;
Item* Items::PARROT_SPAWN_EGG = nullptr;
Item* Items::PARCHED_SPAWN_EGG = nullptr;
Item* Items::PHANTOM_SPAWN_EGG = nullptr;
Item* Items::PIG_SPAWN_EGG = nullptr;
Item* Items::PIGLIN_SPAWN_EGG = nullptr;
Item* Items::PIGLIN_BRUTE_SPAWN_EGG = nullptr;
Item* Items::PILLAGER_SPAWN_EGG = nullptr;
Item* Items::POLAR_BEAR_SPAWN_EGG = nullptr;
Item* Items::PUFFERFISH_SPAWN_EGG = nullptr;
Item* Items::RABBIT_SPAWN_EGG = nullptr;
Item* Items::RAVAGER_SPAWN_EGG = nullptr;
Item* Items::SALMON_SPAWN_EGG = nullptr;
Item* Items::SHEEP_SPAWN_EGG = nullptr;
Item* Items::SHULKER_SPAWN_EGG = nullptr;
Item* Items::SILVERFISH_SPAWN_EGG = nullptr;
Item* Items::SKELETON_SPAWN_EGG = nullptr;
Item* Items::SKELETON_HORSE_SPAWN_EGG = nullptr;
Item* Items::SLIME_SPAWN_EGG = nullptr;
Item* Items::SNIFFER_SPAWN_EGG = nullptr;
Item* Items::SNOW_GOLEM_SPAWN_EGG = nullptr;
Item* Items::SPIDER_SPAWN_EGG = nullptr;
Item* Items::SQUID_SPAWN_EGG = nullptr;
Item* Items::STRAY_SPAWN_EGG = nullptr;
Item* Items::STRIDER_SPAWN_EGG = nullptr;
Item* Items::TADPOLE_SPAWN_EGG = nullptr;
Item* Items::TRADER_LLAMA_SPAWN_EGG = nullptr;
Item* Items::TROPICAL_FISH_SPAWN_EGG = nullptr;
Item* Items::TURTLE_SPAWN_EGG = nullptr;
Item* Items::VEX_SPAWN_EGG = nullptr;
Item* Items::VILLAGER_SPAWN_EGG = nullptr;
Item* Items::VINDICATOR_SPAWN_EGG = nullptr;
Item* Items::WANDERING_TRADER_SPAWN_EGG = nullptr;
Item* Items::WARDEN_SPAWN_EGG = nullptr;
Item* Items::WITCH_SPAWN_EGG = nullptr;
Item* Items::WITHER_SPAWN_EGG = nullptr;
Item* Items::WITHER_SKELETON_SPAWN_EGG = nullptr;
Item* Items::WOLF_SPAWN_EGG = nullptr;
Item* Items::ZOGLIN_SPAWN_EGG = nullptr;
Item* Items::ZOMBIE_SPAWN_EGG = nullptr;
Item* Items::ZOMBIE_HORSE_SPAWN_EGG = nullptr;
Item* Items::ZOMBIE_NAUTILUS_SPAWN_EGG = nullptr;
Item* Items::ZOMBIFIED_PIGLIN_SPAWN_EGG = nullptr;
Item* Items::ZOMBIE_VILLAGER_SPAWN_EGG = nullptr;

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
    _registerChestBoats();   // 带箱子的船物品
    _registerHangingItems(); // 悬挂实体物品
    _registerSigns();        // 告示牌物品
    _registerBanners();      // 旗帜和图案物品
    _registerBuildingBlocks();
    _registerBeds();
    _registerShulkerBoxes();
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
    _registerMusicDiscs();        // 音乐唱片
    _registerSkulls();            // 头颅物品
    _registerHarnesses();         // 欢乐诡鬼装备 (16色)
    _registerBundles();           // 收纳袋 (1 无色 + 16 色)
    _registerSpawnEggs();         // 刷怪蛋 (67 种)

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

    // 砖（合成材料）
    BRICK = &registry.registerItem(ResourceLocation("minecraft:brick"), ItemProperties().maxStackSize(64));

    // 树脂砖（合成材料）
    RESIN_BRICK = &registry.registerItem(ResourceLocation("minecraft:resin_brick"), ItemProperties().maxStackSize(64));

    // 紫水晶碎片
    AMETHYST_SHARD =
        &registry.registerItem(ResourceLocation("minecraft:amethyst_shard"), ItemProperties().maxStackSize(64));

    // 粗矿（Raw Ore）- 铁矿石/铜矿石/金矿石的掉落物
    // 参考: net.minecraft.item.Items.RAW_IRON / RAW_COPPER / RAW_GOLD
    RAW_IRON = &registry.registerItem(ResourceLocation("minecraft:raw_iron"), ItemProperties().maxStackSize(64));

    RAW_COPPER = &registry.registerItem(ResourceLocation("minecraft:raw_copper"), ItemProperties().maxStackSize(64));

    RAW_GOLD = &registry.registerItem(ResourceLocation("minecraft:raw_gold"), ItemProperties().maxStackSize(64));

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

    // 深板岩矿物原矿
    DEEPSLATE_COAL_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DEEPSLATE_COAL_ORE, "deepslate_coal_ore", ItemProperties().maxStackSize(64));
    DEEPSLATE_IRON_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DEEPSLATE_IRON_ORE, "deepslate_iron_ore", ItemProperties().maxStackSize(64));
    DEEPSLATE_COPPER_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DEEPSLATE_COPPER_ORE, "deepslate_copper_ore", ItemProperties().maxStackSize(64));
    DEEPSLATE_GOLD_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DEEPSLATE_GOLD_ORE, "deepslate_gold_ore", ItemProperties().maxStackSize(64));
    DEEPSLATE_DIAMOND_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DEEPSLATE_DIAMOND_ORE, "deepslate_diamond_ore", ItemProperties().maxStackSize(64));
    DEEPSLATE_LAPIS_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DEEPSLATE_LAPIS_ORE, "deepslate_lapis_ore", ItemProperties().maxStackSize(64));
    DEEPSLATE_EMERALD_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DEEPSLATE_EMERALD_ORE, "deepslate_emerald_ore", ItemProperties().maxStackSize(64));
    DEEPSLATE_REDSTONE_ORE = &registerBlockBackedItem(
        registry, VanillaBlocks::DEEPSLATE_REDSTONE_ORE, "deepslate_redstone_ore", ItemProperties().maxStackSize(64));

    // 粗矿块（Raw Ore Block）
    // 参考: net.minecraft.item.Items.RAW_IRON_BLOCK / RAW_COPPER_BLOCK / RAW_GOLD_BLOCK
    RAW_IRON_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::RAW_IRON_BLOCK, "raw_iron_block", ItemProperties().maxStackSize(64));

    RAW_COPPER_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::RAW_COPPER_BLOCK, "raw_copper_block", ItemProperties().maxStackSize(64));

    RAW_GOLD_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::RAW_GOLD_BLOCK, "raw_gold_block", ItemProperties().maxStackSize(64));
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
    // 铜工具（MC 1.21.11 新增）
    // ========================================================================
    COPPER_PICKAXE = &registry.registerItem<item::tool::PickaxeItem>(ResourceLocation("minecraft:copper_pickaxe"),
        item::tier::ItemTiers::COPPER(), // tier
        2,                               // attackDamage
        -2.8f,                           // attackSpeed
        ItemProperties());

    COPPER_AXE = &registry.registerItem<item::tool::AxeItem>(ResourceLocation("minecraft:copper_axe"),
        item::tier::ItemTiers::COPPER(), // tier
        8.0f,                            // attackDamage
        -3.2f,                           // attackSpeed
        ItemProperties());

    COPPER_SHOVEL = &registry.registerItem<item::tool::ShovelItem>(ResourceLocation("minecraft:copper_shovel"),
        item::tier::ItemTiers::COPPER(), // tier
        2.5f,                            // attackDamage
        -3.0f,                           // attackSpeed
        ItemProperties());

    COPPER_HOE = &registry.registerItem<item::tool::HoeItem>(ResourceLocation("minecraft:copper_hoe"),
        item::tier::ItemTiers::COPPER(), // tier
        0,                               // attackDamage
        -2.0f,                           // attackSpeed
        ItemProperties());

    COPPER_SWORD = &registry.registerItem<item::tool::SwordItem>(ResourceLocation("minecraft:copper_sword"),
        item::tier::ItemTiers::COPPER(), // tier
        4,                               // attackDamage
        -2.4f,                           // attackSpeed
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
    // 铜护甲（MC 1.21.11 新增）
    // 铜护甲耐久介于皮革和锁链之间，防御值：头盔=2, 胸甲=4, 护腿=3, 靴子=1
    // 附魔能力 8，韧性 0，击退抗性 0
    // ========================================================================
    COPPER_HELMET = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:copper_helmet"),
        ArmorMaterials::COPPER,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(ArmorMaterials::COPPER.getDurability(item::armor::ArmorSlot::Head)));

    COPPER_CHESTPLATE = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:copper_chestplate"),
        ArmorMaterials::COPPER,
        item::armor::ArmorSlot::Chest,
        ItemProperties().maxDamage(ArmorMaterials::COPPER.getDurability(item::armor::ArmorSlot::Chest)));

    COPPER_LEGGINGS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:copper_leggings"),
        ArmorMaterials::COPPER,
        item::armor::ArmorSlot::Legs,
        ItemProperties().maxDamage(ArmorMaterials::COPPER.getDurability(item::armor::ArmorSlot::Legs)));

    COPPER_BOOTS = &registry.registerItem<item::items::ArmorItem>(ResourceLocation("minecraft:copper_boots"),
        ArmorMaterials::COPPER,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(ArmorMaterials::COPPER.getDurability(item::armor::ArmorSlot::Feet)));

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

    // 铜马铠 - +4 护甲
    COPPER_HORSE_ARMOR =
        &registry.registerItem<item::items::HorseArmorItem>(ResourceLocation("minecraft:copper_horse_armor"),
            ItemProperties().maxStackSize(1),
            4,
            ResourceLocation("minecraft", "textures/entity/horse/armor/horse_armor_copper.png"));

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

    // 下界合金马铠 - +19 护甲，防火（通过 FIRE_RESISTANT 标签实现）
    NETHERITE_HORSE_ARMOR =
        &registry.registerItem<item::items::HorseArmorItem>(ResourceLocation("minecraft:netherite_horse_armor"),
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare),
            19,
            ResourceLocation("minecraft", "textures/entity/horse/armor/horse_armor_netherite.png"));

    // ========================================================================
    // 狼铠 - MC 1.20.5+ 新增
    // 使用犰狳鳞甲材质，可染色，64点耐久，防御值11（Body槽位）
    // 参考: net.minecraft.item.WolfArmorItem
    // ========================================================================
    WOLF_ARMOR = &registry.registerItem<item::items::WolfArmorItem>(ResourceLocation("minecraft:wolf_armor"),
        ArmorMaterials::ARMADILLO_SCUTE,
        item::armor::ArmorSlot::Body,
        ItemProperties().maxDamage(ArmorMaterials::ARMADILLO_SCUTE.getDurability(item::armor::ArmorSlot::Body)));

    // ========================================================================
    // 鹦鹉螺铠甲 - MC 1.21.11 新增
    // 用于装备鹦鹉螺类实体，不可损坏，无耐久度
    // 护甲值由材质的 Body 槽位防御值提供（与 MC 1.21.11 Item.Properties.nautilusArmor
    // 通过 ArmorMaterial.createAttributes(ArmorType.BODY) 取护甲值的语义一致）：
    // 铜=4, 铁=5, 金=7, 钻石=11, 下界合金=19
    // 参考: net.minecraft.world.item.Item.Properties.nautilusArmor(ArmorMaterial) (MC 1.21.11)
    // ========================================================================
    // 铜鹦鹉螺铠甲 - +4 护甲
    COPPER_NAUTILUS_ARMOR = &registry.registerItem<item::items::NautilusArmorItem>(
        ResourceLocation("minecraft:copper_nautilus_armor"), ItemProperties().maxStackSize(1), ArmorMaterials::COPPER);

    // 铁鹦鹉螺铠甲 - +5 护甲
    IRON_NAUTILUS_ARMOR = &registry.registerItem<item::items::NautilusArmorItem>(
        ResourceLocation("minecraft:iron_nautilus_armor"), ItemProperties().maxStackSize(1), ArmorMaterials::IRON);

    // 金鹦鹉螺铠甲 - +7 护甲
    GOLDEN_NAUTILUS_ARMOR = &registry.registerItem<item::items::NautilusArmorItem>(
        ResourceLocation("minecraft:golden_nautilus_armor"), ItemProperties().maxStackSize(1), ArmorMaterials::GOLD);

    // 钻石鹦鹉螺铠甲 - +11 护甲
    DIAMOND_NAUTILUS_ARMOR =
        &registry.registerItem<item::items::NautilusArmorItem>(ResourceLocation("minecraft:diamond_nautilus_armor"),
            ItemProperties().maxStackSize(1),
            ArmorMaterials::DIAMOND);

    // 下界合金鹦鹉螺铠甲 - +19 护甲，防火
    NETHERITE_NAUTILUS_ARMOR =
        &registry.registerItem<item::items::NautilusArmorItem>(ResourceLocation("minecraft:netherite_nautilus_armor"),
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare),
            ArmorMaterials::NETHERITE);
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

    // 竹木原木
    BAMBOO_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_BLOCK, "bamboo_block", ItemProperties().maxStackSize(64));
    STRIPPED_BAMBOO_BLOCK = &registerBlockBackedItem(
        registry, VanillaBlocks::STRIPPED_BAMBOO_BLOCK, "stripped_bamboo_block", ItemProperties().maxStackSize(64));

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

    // 竹木木板和马赛克
    BAMBOO_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_PLANKS, "bamboo_planks", ItemProperties().maxStackSize(64));
    BAMBOO_MOSAIC = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_MOSAIC, "bamboo_mosaic", ItemProperties().maxStackSize(64));

    // 其他木板变体
    CRIMSON_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_PLANKS, "crimson_planks", ItemProperties().maxStackSize(64));
    WARPED_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_PLANKS, "warped_planks", ItemProperties().maxStackSize(64));
    MANGROVE_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::MANGROVE_PLANKS, "mangrove_planks", ItemProperties().maxStackSize(64));
    CHERRY_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::CHERRY_PLANKS, "cherry_planks", ItemProperties().maxStackSize(64));
    PALE_OAK_PLANKS = &registerBlockBackedItem(
        registry, VanillaBlocks::PALE_OAK_PLANKS, "pale_oak_planks", ItemProperties().maxStackSize(64));

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

    // 刷子 - 考古学工具，用于刷可疑方块和犰狳
    // 耐久度 64，附魔能力 1（仅耐久/经验修补/消失诅咒）
    // 参考: net.minecraft.world.item.BrushItem
    BRUSH = &registry.registerItem<item::BrushItem>(
        ResourceLocation("minecraft:brush"), ItemProperties().maxDamage(item::BrushItem::MAX_DURABILITY));

    // 蜜脾 - 右键铜方块涂蜡，阻止氧化
    HONEYCOMB = &registry.registerItem<item::items::HoneycombItem>(
        ResourceLocation("minecraft:honeycomb"), ItemProperties().maxStackSize(64));

    // 钟 - 可敲响的功能方块
    BELL = &registerBlockBackedItem(registry, VanillaBlocks::BELL, "bell", ItemProperties().maxStackSize(64));

    // 可疑的沙子/沙砾 - 考古学方块
    SUSPICIOUS_SAND = &registerBlockBackedItem(
        registry, VanillaBlocks::SUSPICIOUS_SAND, "suspicious_sand", ItemProperties().maxStackSize(64));
    SUSPICIOUS_GRAVEL = &registerBlockBackedItem(
        registry, VanillaBlocks::SUSPICIOUS_GRAVEL, "suspicious_gravel", ItemProperties().maxStackSize(64));

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

    RABBIT_HIDE = &registry.registerItem(ResourceLocation("minecraft:rabbit_hide"), ItemProperties().maxStackSize(64));

    SLIME_BALL = &registry.registerItem(ResourceLocation("minecraft:slime_ball"), ItemProperties().maxStackSize(64));

    // EGG 已在 registerThrowableItems() 中注册为 EggItem

    // 蓝蛋 - 寒带鸡变种产蛋，与白蛋行为相同
    BLUE_EGG = &registry.registerItem<item::EggItem>(
        ResourceLocation("minecraft:blue_egg"), ItemProperties().maxStackSize(16));

    // 棕蛋 - 暖色鸡变种产蛋，与白蛋行为相同
    BROWN_EGG = &registry.registerItem<item::EggItem>(
        ResourceLocation("minecraft:brown_egg"), ItemProperties().maxStackSize(16));

    COMPASS = &registry.registerItem(ResourceLocation("minecraft:compass"), ItemProperties().maxStackSize(64));

    // 追溯指南针 - 指向玩家上次死亡位置，8个回响碎片围绕1个指南针合成
    RECOVERY_COMPASS =
        &registry.registerItem(ResourceLocation("minecraft:recovery_compass"), ItemProperties().maxStackSize(1));

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

    FIRE_CHARGE = &registry.registerItem<item::FireChargeItem>(
        ResourceLocation("minecraft:fire_charge"), ItemProperties().maxStackSize(64));

    FIREWORK_STAR =
        &registry.registerItem(ResourceLocation("minecraft:firework_star"), ItemProperties().maxStackSize(64));

    FIREWORK_ROCKET = &registry.registerItem<item::FireworkRocketItem>(
        ResourceLocation("minecraft:firework_rocket"), ItemProperties().maxStackSize(64));
}

void Items::_registerDyes()
{
    auto& registry = ItemRegistry::instance();

    INK_SAC = &registry.registerItem(ResourceLocation("minecraft:ink_sac"), ItemProperties().maxStackSize(64));

    RED_DYE = &registry.registerItem(ResourceLocation("minecraft:red_dye"), ItemProperties().maxStackSize(64));

    GREEN_DYE = &registry.registerItem(ResourceLocation("minecraft:green_dye"), ItemProperties().maxStackSize(64));

    COCOA_BEANS =
        &registerBlockBackedItem(registry, VanillaBlocks::COCOA, "cocoa_beans", ItemProperties().maxStackSize(64));

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

    // 种子物品注册为 SeedsItem（BlockItem 子类），右键耕地时通过 BlockItem::tryPlace()
    // 放置对应的作物方块。作物方块的 isValidPosition() 会检查耕地和光照条件，
    // 因此种子只能在合法位置种植。
    //
    // 种子与作物方块的映射关系：
    //   WHEAT_SEEDS       -> WheatBlock (minecraft:wheat)
    //   PUMPKIN_SEEDS     -> PumpkinStemBlock (minecraft:pumpkin_stem)
    //   MELON_SEEDS       -> MelonStemBlock (minecraft:melon_stem)
    //   BEETROOT_SEEDS    -> BeetrootBlock (minecraft:beetroots)
    //   TORCHFLOWER_SEEDS -> TorchflowerCropBlock (minecraft:torchflower_crop)
    //   PITCHER_POD       -> PitcherCropBlock (minecraft:pitcher_crop)

    WHEAT_SEEDS = &registry.registerItem<item::items::SeedsItem>(
        ResourceLocation("minecraft:wheat_seeds"), *VanillaBlocks::WHEAT, ItemProperties().maxStackSize(64));

    PUMPKIN_SEEDS = &registry.registerItem<item::items::SeedsItem>(
        ResourceLocation("minecraft:pumpkin_seeds"), *VanillaBlocks::PUMPKIN_STEM, ItemProperties().maxStackSize(64));

    MELON_SEEDS = &registry.registerItem<item::items::SeedsItem>(
        ResourceLocation("minecraft:melon_seeds"), *VanillaBlocks::MELON_STEM, ItemProperties().maxStackSize(64));

    BEETROOT_SEEDS = &registry.registerItem<item::items::SeedsItem>(
        ResourceLocation("minecraft:beetroot_seeds"), *VanillaBlocks::BEETROOTS, ItemProperties().maxStackSize(64));

    // 火把花种子 - 嗅探兽挖掘获得，右键耕地种植火把花作物
    TORCHFLOWER_SEEDS = &registry.registerItem<item::items::SeedsItem>(ResourceLocation("minecraft:torchflower_seeds"),
        *VanillaBlocks::TORCHFLOWER_CROP,
        ItemProperties().maxStackSize(64));

    // 瓶草荚果 - 嗅探兽挖掘获得，右键耕地种植瓶草作物
    PITCHER_POD = &registry.registerItem<item::items::SeedsItem>(
        ResourceLocation("minecraft:pitcher_pod"), *VanillaBlocks::PITCHER_CROP, ItemProperties().maxStackSize(64));
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

    SUGAR_CANE =
        &registerBlockBackedItem(registry, VanillaBlocks::SUGAR_CANE, "sugar_cane", ItemProperties().maxStackSize(64));

    SUGAR = &registry.registerItem(ResourceLocation("minecraft:sugar"), ItemProperties().maxStackSize(64));

    // 竹子 - 熊猫食物
    // 参考: new BlockItem(Blocks.BAMBOO, new Item.Properties().group(ItemGroup.DECORATIONS))
    BAMBOO = &registerBlockBackedItem(registry, VanillaBlocks::BAMBOO, "bamboo", ItemProperties().maxStackSize(64));
}

void Items::_registerAquaticMaterials()
{
    auto& registry = ItemRegistry::instance();

    // 海龟鳞甲 - 海龟长大时掉落，用于合成海龟壳
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    // 注意: MC 1.20.5+ 将 scute 重命名为 turtle_scute
    TURTLE_SCUTE =
        &registry.registerItem(ResourceLocation("minecraft:turtle_scute"), ItemProperties().maxStackSize(64));

    // 犰狳鳞甲 - 刷犰狳获得，用于合成狼铠
    // 参考: new Item(new Item.Properties().group(ItemGroup.MATERIALS))
    ARMADILLO_SCUTE =
        &registry.registerItem(ResourceLocation("minecraft:armadillo_scute"), ItemProperties().maxStackSize(64));

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
    SPECTRAL_ARROW = &registry.registerItem<item::SpectralArrowItem>(
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

    // ========================================================================
    // 长矛 - 按材质分层（木/石/铜/铁/金/钻石/下界合金）
    // 长矛是近战和远程相结合的武器：
    // - 近战攻击伤害 = 基础值(3) + 层级加成（与剑一致）
    // - 攻击速度: -2.9（长杆武器，比剑慢）
    // - 投掷伤害: 8.0（固定，与三叉戟一致）
    // - 耐久消耗: 近战 1 / 破坏方块 2 / 投掷 1
    // ========================================================================

    // 木长矛
    WOODEN_SPEAR = &registry.registerItem<item::SpearItem>(ResourceLocation("minecraft:wooden_spear"),
        item::tier::ItemTiers::WOOD(), // tier
        3,                             // attackDamage（基础值）
        -2.9f,                         // attackSpeed
        ItemProperties().rarity(ItemRarity::Common));

    // 石长矛
    STONE_SPEAR = &registry.registerItem<item::SpearItem>(ResourceLocation("minecraft:stone_spear"),
        item::tier::ItemTiers::STONE(),
        3,
        -2.9f,
        ItemProperties().rarity(ItemRarity::Common));

    // 铜长矛
    COPPER_SPEAR = &registry.registerItem<item::SpearItem>(ResourceLocation("minecraft:copper_spear"),
        item::tier::ItemTiers::COPPER(),
        3,
        -2.9f,
        ItemProperties().rarity(ItemRarity::Common));

    // 铁长矛
    IRON_SPEAR = &registry.registerItem<item::SpearItem>(ResourceLocation("minecraft:iron_spear"),
        item::tier::ItemTiers::IRON(),
        3,
        -2.9f,
        ItemProperties().rarity(ItemRarity::Common));

    // 金长矛
    GOLDEN_SPEAR = &registry.registerItem<item::SpearItem>(ResourceLocation("minecraft:golden_spear"),
        item::tier::ItemTiers::GOLD(),
        3,
        -2.9f,
        ItemProperties().rarity(ItemRarity::Common));

    // 钻石长矛
    DIAMOND_SPEAR = &registry.registerItem<item::SpearItem>(ResourceLocation("minecraft:diamond_spear"),
        item::tier::ItemTiers::DIAMOND(),
        3,
        -2.9f,
        ItemProperties().rarity(ItemRarity::Common));

    // 下界合金长矛
    NETHERITE_SPEAR = &registry.registerItem<item::SpearItem>(ResourceLocation("minecraft:netherite_spear"),
        item::tier::ItemTiers::NETHERITE(),
        3,
        -2.9f,
        ItemProperties().rarity(ItemRarity::Rare));
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

    // 细雪桶 - 装有细雪的桶
    // 细雪不是流体，因此使用独立的 PowderSnowBucketItem（继承自 Item，非 BucketItem）。
    // 右键方块放置 PowderSnowBlock，使用后返回空桶。
    // 与炼药锅的交互由炼药锅自身的 onBlockActivated 处理。
    // 空桶从细雪方块舀取细雪由 PowderSnowBlock 实现 IBucketPickupHandler 接口处理。
    POWDER_SNOW_BUCKET = &registry.registerItem<item::PowderSnowBucketItem>(
        ResourceLocation("minecraft:powder_snow_bucket"), ItemProperties().maxStackSize(1).containerItem(BUCKET));

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

    // 知识之书 - 右键使用时解锁NBT中存储的配方列表
    KNOWLEDGE_BOOK = &registry.registerItem<item::items::KnowledgeBookItem>(
        ResourceLocation("minecraft:knowledge_book"), ItemProperties().maxStackSize(1));
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
    // 船物品（普通船，hasChest = false）
    // ========================================================================

    // 橡木船
    OAK_BOAT = &registry.registerItem<item::BoatItem>(
        ResourceLocation("minecraft:oak_boat"), entity::BoatEntity::Type::OAK, false, ItemProperties().maxStackSize(1));

    // 云杉木船
    SPRUCE_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:spruce_boat"),
        entity::BoatEntity::Type::SPRUCE,
        false,
        ItemProperties().maxStackSize(1));

    // 白桦木船
    BIRCH_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:birch_boat"),
        entity::BoatEntity::Type::BIRCH,
        false,
        ItemProperties().maxStackSize(1));

    // 丛林木船
    JUNGLE_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:jungle_boat"),
        entity::BoatEntity::Type::JUNGLE,
        false,
        ItemProperties().maxStackSize(1));

    // 金合欢木船
    ACACIA_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:acacia_boat"),
        entity::BoatEntity::Type::ACACIA,
        false,
        ItemProperties().maxStackSize(1));

    // 深色橡木船
    DARK_OAK_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:dark_oak_boat"),
        entity::BoatEntity::Type::DARK_OAK,
        false,
        ItemProperties().maxStackSize(1));

    // 红树木船
    MANGROVE_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:mangrove_boat"),
        entity::BoatEntity::Type::MANGROVE,
        false,
        ItemProperties().maxStackSize(1));

    // 樱花木船
    CHERRY_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:cherry_boat"),
        entity::BoatEntity::Type::CHERRY,
        false,
        ItemProperties().maxStackSize(1));

    // 苍白橡木船
    PALE_OAK_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:pale_oak_boat"),
        entity::BoatEntity::Type::PALE_OAK,
        false,
        ItemProperties().maxStackSize(1));

    // 竹筏
    BAMBOO_RAFT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:bamboo_raft"),
        entity::BoatEntity::Type::BAMBOO,
        false,
        ItemProperties().maxStackSize(1));
}

void Items::_registerChestBoats()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 带箱子的船物品（hasChest = true）
    // ========================================================================

    // 橡木箱子船
    OAK_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:oak_chest_boat"),
        entity::BoatEntity::Type::OAK,
        true,
        ItemProperties().maxStackSize(1));

    // 云杉木箱子船
    SPRUCE_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:spruce_chest_boat"),
        entity::BoatEntity::Type::SPRUCE,
        true,
        ItemProperties().maxStackSize(1));

    // 白桦木箱子船
    BIRCH_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:birch_chest_boat"),
        entity::BoatEntity::Type::BIRCH,
        true,
        ItemProperties().maxStackSize(1));

    // 丛林木箱子船
    JUNGLE_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:jungle_chest_boat"),
        entity::BoatEntity::Type::JUNGLE,
        true,
        ItemProperties().maxStackSize(1));

    // 金合欢木箱子船
    ACACIA_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:acacia_chest_boat"),
        entity::BoatEntity::Type::ACACIA,
        true,
        ItemProperties().maxStackSize(1));

    // 深色橡木箱子船
    DARK_OAK_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:dark_oak_chest_boat"),
        entity::BoatEntity::Type::DARK_OAK,
        true,
        ItemProperties().maxStackSize(1));

    // 红树木箱子船
    MANGROVE_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:mangrove_chest_boat"),
        entity::BoatEntity::Type::MANGROVE,
        true,
        ItemProperties().maxStackSize(1));

    // 樱花木箱子船
    CHERRY_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:cherry_chest_boat"),
        entity::BoatEntity::Type::CHERRY,
        true,
        ItemProperties().maxStackSize(1));

    // 苍白橡木箱子船
    PALE_OAK_CHEST_BOAT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:pale_oak_chest_boat"),
        entity::BoatEntity::Type::PALE_OAK,
        true,
        ItemProperties().maxStackSize(1));

    // 箱子竹筏
    BAMBOO_CHEST_RAFT = &registry.registerItem<item::BoatItem>(ResourceLocation("minecraft:bamboo_chest_raft"),
        entity::BoatEntity::Type::BAMBOO,
        true,
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
    LEAD = &registry.registerItem<item::items::LeadItem>(
        ResourceLocation("minecraft:lead"), ItemProperties().maxStackSize(16));
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

    // 红树木告示牌
    MANGROVE_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:mangrove_sign"),
        *VanillaBlocks::MANGROVE_SIGN,
        *VanillaBlocks::MANGROVE_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 樱花木告示牌
    CHERRY_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:cherry_sign"),
        *VanillaBlocks::CHERRY_SIGN,
        *VanillaBlocks::CHERRY_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 竹木告示牌
    BAMBOO_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:bamboo_sign"),
        *VanillaBlocks::BAMBOO_SIGN,
        *VanillaBlocks::BAMBOO_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // 苍白橡木告示牌
    PALE_OAK_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:pale_oak_sign"),
        *VanillaBlocks::PALE_OAK_SIGN,
        *VanillaBlocks::PALE_OAK_WALL_SIGN,
        ItemProperties().maxStackSize(16));

    // ========================================================================
    // 悬挂告示牌物品 - 使用 WallOrFloorItem（与普通告示牌相同的放置逻辑）
    // ========================================================================

    // 橡木悬挂告示牌
    OAK_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:oak_hanging_sign"),
        *VanillaBlocks::OAK_HANGING_SIGN,
        *VanillaBlocks::OAK_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 云杉木悬挂告示牌
    SPRUCE_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:spruce_hanging_sign"),
        *VanillaBlocks::SPRUCE_HANGING_SIGN,
        *VanillaBlocks::SPRUCE_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 白桦木悬挂告示牌
    BIRCH_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:birch_hanging_sign"),
        *VanillaBlocks::BIRCH_HANGING_SIGN,
        *VanillaBlocks::BIRCH_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 丛林木悬挂告示牌
    JUNGLE_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:jungle_hanging_sign"),
        *VanillaBlocks::JUNGLE_HANGING_SIGN,
        *VanillaBlocks::JUNGLE_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 金合欢木悬挂告示牌
    ACACIA_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:acacia_hanging_sign"),
        *VanillaBlocks::ACACIA_HANGING_SIGN,
        *VanillaBlocks::ACACIA_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 深色橡木悬挂告示牌
    DARK_OAK_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:dark_oak_hanging_sign"),
        *VanillaBlocks::DARK_OAK_HANGING_SIGN,
        *VanillaBlocks::DARK_OAK_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 绯红悬挂告示牌
    CRIMSON_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:crimson_hanging_sign"),
        *VanillaBlocks::CRIMSON_HANGING_SIGN,
        *VanillaBlocks::CRIMSON_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 诡异悬挂告示牌
    WARPED_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:warped_hanging_sign"),
        *VanillaBlocks::WARPED_HANGING_SIGN,
        *VanillaBlocks::WARPED_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 红树木悬挂告示牌
    MANGROVE_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:mangrove_hanging_sign"),
        *VanillaBlocks::MANGROVE_HANGING_SIGN,
        *VanillaBlocks::MANGROVE_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 樱花木悬挂告示牌
    CHERRY_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:cherry_hanging_sign"),
        *VanillaBlocks::CHERRY_HANGING_SIGN,
        *VanillaBlocks::CHERRY_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 竹木悬挂告示牌
    BAMBOO_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:bamboo_hanging_sign"),
        *VanillaBlocks::BAMBOO_HANGING_SIGN,
        *VanillaBlocks::BAMBOO_WALL_HANGING_SIGN,
        ItemProperties().maxStackSize(16));

    // 苍白橡木悬挂告示牌
    PALE_OAK_HANGING_SIGN = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:pale_oak_hanging_sign"),
        *VanillaBlocks::PALE_OAK_HANGING_SIGN,
        *VanillaBlocks::PALE_OAK_WALL_HANGING_SIGN,
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

    // 木质书架变体（1.21.4+）
    OAK_SHELF =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_SHELF, "oak_shelf", ItemProperties().maxStackSize(64));
    SPRUCE_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_SHELF, "spruce_shelf", ItemProperties().maxStackSize(64));
    BIRCH_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_SHELF, "birch_shelf", ItemProperties().maxStackSize(64));
    JUNGLE_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_SHELF, "jungle_shelf", ItemProperties().maxStackSize(64));
    ACACIA_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_SHELF, "acacia_shelf", ItemProperties().maxStackSize(64));
    DARK_OAK_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_SHELF, "dark_oak_shelf", ItemProperties().maxStackSize(64));
    MANGROVE_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::MANGROVE_SHELF, "mangrove_shelf", ItemProperties().maxStackSize(64));
    CHERRY_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::CHERRY_SHELF, "cherry_shelf", ItemProperties().maxStackSize(64));
    PALE_OAK_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::PALE_OAK_SHELF, "pale_oak_shelf", ItemProperties().maxStackSize(64));
    BAMBOO_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_SHELF, "bamboo_shelf", ItemProperties().maxStackSize(64));
    CRIMSON_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_SHELF, "crimson_shelf", ItemProperties().maxStackSize(64));
    WARPED_SHELF = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_SHELF, "warped_shelf", ItemProperties().maxStackSize(64));
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
        &registerBlockBackedItem(registry, VanillaBlocks::GRASS_PATH, "dirt_path", ItemProperties().maxStackSize(64));
    MYCELIUM =
        &registerBlockBackedItem(registry, VanillaBlocks::MYCELIUM, "mycelium", ItemProperties().maxStackSize(64));
    PACKED_ICE =
        &registerBlockBackedItem(registry, VanillaBlocks::PACKED_ICE, "packed_ice", ItemProperties().maxStackSize(64));
    BLUE_ICE =
        &registerBlockBackedItem(registry, VanillaBlocks::BLUE_ICE, "blue_ice", ItemProperties().maxStackSize(64));
    COARSE_DIRT = &registerBlockBackedItem(
        registry, VanillaBlocks::COARSE_DIRT, "coarse_dirt", ItemProperties().maxStackSize(64));
    PODZOL = &registerBlockBackedItem(registry, VanillaBlocks::PODZOL, "podzol", ItemProperties().maxStackSize(64));
    TORCH = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:torch"),
        *VanillaBlocks::TORCH,
        *VanillaBlocks::WALL_TORCH,
        ItemProperties().maxStackSize(64));

    SOUL_TORCH = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:soul_torch"),
        *VanillaBlocks::SOUL_TORCH,
        *VanillaBlocks::SOUL_WALL_TORCH,
        ItemProperties().maxStackSize(64));

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
    SMOOTH_SANDSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::SMOOTH_SANDSTONE, "smooth_sandstone", ItemProperties().maxStackSize(64));
    RED_SANDSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_SANDSTONE, "red_sandstone", ItemProperties().maxStackSize(64));
    CHISELED_RED_SANDSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::CHISELED_RED_SANDSTONE, "chiseled_red_sandstone", ItemProperties().maxStackSize(64));
    CUT_RED_SANDSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::CUT_RED_SANDSTONE, "cut_red_sandstone", ItemProperties().maxStackSize(64));
    SMOOTH_RED_SANDSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::SMOOTH_RED_SANDSTONE, "smooth_red_sandstone", ItemProperties().maxStackSize(64));

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
    GILDED_BLACKSTONE = &registerBlockBackedItem(
        registry, VanillaBlocks::GILDED_BLACKSTONE, "gilded_blackstone", ItemProperties().maxStackSize(64));
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
    SHULKER_BOX =
        &registerBlockBackedItem(registry, VanillaBlocks::SHULKER_BOX, "shulker_box", ItemProperties().maxStackSize(1));
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
    CHAIN = &registerBlockBackedItem(registry, VanillaBlocks::CHAIN, "iron_chain", ItemProperties().maxStackSize(64));
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

void Items::_registerBeds()
{
    auto& registry = ItemRegistry::instance();

    // 床使用 BedItem（自定义 BlockItem 子类），以支持根据朝向放置脚部方块
    // BedBlock::onBlockPlacedBy 会在脚部放置后自动放置头部方块
    WHITE_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:white_bed"), *VanillaBlocks::WHITE_BED, ItemProperties().maxStackSize(1));
    ORANGE_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:orange_bed"), *VanillaBlocks::ORANGE_BED, ItemProperties().maxStackSize(1));
    MAGENTA_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:magenta_bed"), *VanillaBlocks::MAGENTA_BED, ItemProperties().maxStackSize(1));
    LIGHT_BLUE_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:light_blue_bed"), *VanillaBlocks::LIGHT_BLUE_BED, ItemProperties().maxStackSize(1));
    YELLOW_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:yellow_bed"), *VanillaBlocks::YELLOW_BED, ItemProperties().maxStackSize(1));
    LIME_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:lime_bed"), *VanillaBlocks::LIME_BED, ItemProperties().maxStackSize(1));
    PINK_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:pink_bed"), *VanillaBlocks::PINK_BED, ItemProperties().maxStackSize(1));
    GRAY_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:gray_bed"), *VanillaBlocks::GRAY_BED, ItemProperties().maxStackSize(1));
    LIGHT_GRAY_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:light_gray_bed"), *VanillaBlocks::LIGHT_GRAY_BED, ItemProperties().maxStackSize(1));
    CYAN_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:cyan_bed"), *VanillaBlocks::CYAN_BED, ItemProperties().maxStackSize(1));
    PURPLE_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:purple_bed"), *VanillaBlocks::PURPLE_BED, ItemProperties().maxStackSize(1));
    BLUE_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:blue_bed"), *VanillaBlocks::BLUE_BED, ItemProperties().maxStackSize(1));
    BROWN_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:brown_bed"), *VanillaBlocks::BROWN_BED, ItemProperties().maxStackSize(1));
    GREEN_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:green_bed"), *VanillaBlocks::GREEN_BED, ItemProperties().maxStackSize(1));
    RED_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:red_bed"), *VanillaBlocks::RED_BED, ItemProperties().maxStackSize(1));
    BLACK_BED = &registry.registerItem<BedItem>(
        ResourceLocation("minecraft:black_bed"), *VanillaBlocks::BLACK_BED, ItemProperties().maxStackSize(1));
}

void Items::_registerShulkerBoxes()
{
    auto& registry = ItemRegistry::instance();

    // 潜影盒使用普通 BlockItem，堆叠上限为1（包含物品时不能堆叠）
    WHITE_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::WHITE_SHULKER_BOX, "white_shulker_box", ItemProperties().maxStackSize(1));
    ORANGE_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::ORANGE_SHULKER_BOX, "orange_shulker_box", ItemProperties().maxStackSize(1));
    MAGENTA_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::MAGENTA_SHULKER_BOX, "magenta_shulker_box", ItemProperties().maxStackSize(1));
    LIGHT_BLUE_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_BLUE_SHULKER_BOX, "light_blue_shulker_box", ItemProperties().maxStackSize(1));
    YELLOW_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::YELLOW_SHULKER_BOX, "yellow_shulker_box", ItemProperties().maxStackSize(1));
    LIME_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::LIME_SHULKER_BOX, "lime_shulker_box", ItemProperties().maxStackSize(1));
    PINK_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::PINK_SHULKER_BOX, "pink_shulker_box", ItemProperties().maxStackSize(1));
    GRAY_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::GRAY_SHULKER_BOX, "gray_shulker_box", ItemProperties().maxStackSize(1));
    LIGHT_GRAY_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::LIGHT_GRAY_SHULKER_BOX, "light_gray_shulker_box", ItemProperties().maxStackSize(1));
    CYAN_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::CYAN_SHULKER_BOX, "cyan_shulker_box", ItemProperties().maxStackSize(1));
    PURPLE_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::PURPLE_SHULKER_BOX, "purple_shulker_box", ItemProperties().maxStackSize(1));
    BLUE_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::BLUE_SHULKER_BOX, "blue_shulker_box", ItemProperties().maxStackSize(1));
    BROWN_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::BROWN_SHULKER_BOX, "brown_shulker_box", ItemProperties().maxStackSize(1));
    GREEN_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::GREEN_SHULKER_BOX, "green_shulker_box", ItemProperties().maxStackSize(1));
    RED_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::RED_SHULKER_BOX, "red_shulker_box", ItemProperties().maxStackSize(1));
    BLACK_SHULKER_BOX = &registerBlockBackedItem(
        registry, VanillaBlocks::BLACK_SHULKER_BOX, "black_shulker_box", ItemProperties().maxStackSize(1));
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

    // 火把花 - 嗅探兽挖掘种子种植后长成的花朵
    TORCHFLOWER = &registerBlockBackedItem(
        registry, VanillaBlocks::TORCHFLOWER, "torchflower", ItemProperties().maxStackSize(64));

    // 瓶草 - 嗅探兽挖掘荚果种植后长成的双层花朵
    PITCHER_PLANT = &registerBlockBackedItem(
        registry, VanillaBlocks::PITCHER_PLANT, "pitcher_plant", ItemProperties().maxStackSize(64));

    // 仙人掌花 - 生长在仙人掌上的花
    CACTUS_FLOWER = &registerBlockBackedItem(
        registry, VanillaBlocks::CACTUS_FLOWER, "cactus_flower", ItemProperties().maxStackSize(64));

    // 野花 - 地面装饰花，可堆叠放置
    WILDFLOWERS = &registerBlockBackedItem(
        registry, VanillaBlocks::WILDFLOWERS, "wildflowers", ItemProperties().maxStackSize(64));

    // 开放的眼眸花 - 苍白花园发光花朵
    OPEN_EYEBLOSSOM = &registerBlockBackedItem(
        registry, VanillaBlocks::OPEN_EYEBLOSSOM, "open_eyeblossom", ItemProperties().maxStackSize(64));

    // 闭合的眼眸花 - 苍白花园花朵
    CLOSED_EYEBLOSSOM = &registerBlockBackedItem(
        registry, VanillaBlocks::CLOSED_EYEBLOSSOM, "closed_eyeblossom", ItemProperties().maxStackSize(64));

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

    // 花盆（所有 potted_* 方块共用此物品，匹配 MC Java 设计）
    FLOWER_POT =
        &registerBlockBackedItem(registry, VanillaBlocks::FLOWER_POT, "flower_pot", ItemProperties().maxStackSize(64));
}

void Items::_registerRedstone()
{
    auto& registry = ItemRegistry::instance();

    // 注意：REDSTONE_WIRE 没有独立的物品，因为玩家持有的是 REDSTONE 物品
    // 红石粉放在地上时变成 REDSTONE_WIRE 方块
    REDSTONE_TORCH = &registry.registerItem<WallOrFloorItem>(ResourceLocation("minecraft:redstone_torch"),
        *VanillaBlocks::REDSTONE_TORCH,
        *VanillaBlocks::REDSTONE_WALL_TORCH,
        ItemProperties().maxStackSize(64));
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
    MANGROVE_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::MANGROVE_BUTTON, "mangrove_button", ItemProperties().maxStackSize(64));
    CHERRY_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::CHERRY_BUTTON, "cherry_button", ItemProperties().maxStackSize(64));
    BAMBOO_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_BUTTON, "bamboo_button", ItemProperties().maxStackSize(64));
    PALE_OAK_BUTTON = &registerBlockBackedItem(
        registry, VanillaBlocks::PALE_OAK_BUTTON, "pale_oak_button", ItemProperties().maxStackSize(64));
    POLISHED_BLACKSTONE_BUTTON = &registerBlockBackedItem(registry,
        VanillaBlocks::POLISHED_BLACKSTONE_BUTTON,
        "polished_blackstone_button",
        ItemProperties().maxStackSize(64));
    STONE_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_PRESSURE_PLATE, "stone_pressure_plate", ItemProperties().maxStackSize(64));
    OAK_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::OAK_PRESSURE_PLATE, "oak_pressure_plate", ItemProperties().maxStackSize(64));
    SPRUCE_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_PRESSURE_PLATE, "spruce_pressure_plate", ItemProperties().maxStackSize(64));
    BIRCH_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_PRESSURE_PLATE, "birch_pressure_plate", ItemProperties().maxStackSize(64));
    JUNGLE_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_PRESSURE_PLATE, "jungle_pressure_plate", ItemProperties().maxStackSize(64));
    ACACIA_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_PRESSURE_PLATE, "acacia_pressure_plate", ItemProperties().maxStackSize(64));
    DARK_OAK_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_PRESSURE_PLATE, "dark_oak_pressure_plate", ItemProperties().maxStackSize(64));
    CRIMSON_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_PRESSURE_PLATE, "crimson_pressure_plate", ItemProperties().maxStackSize(64));
    WARPED_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_PRESSURE_PLATE, "warped_pressure_plate", ItemProperties().maxStackSize(64));
    MANGROVE_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::MANGROVE_PRESSURE_PLATE, "mangrove_pressure_plate", ItemProperties().maxStackSize(64));
    CHERRY_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::CHERRY_PRESSURE_PLATE, "cherry_pressure_plate", ItemProperties().maxStackSize(64));
    BAMBOO_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_PRESSURE_PLATE, "bamboo_pressure_plate", ItemProperties().maxStackSize(64));
    PALE_OAK_PRESSURE_PLATE = &registerBlockBackedItem(
        registry, VanillaBlocks::PALE_OAK_PRESSURE_PLATE, "pale_oak_pressure_plate", ItemProperties().maxStackSize(64));
    POLISHED_BLACKSTONE_PRESSURE_PLATE = &registerBlockBackedItem(registry,
        VanillaBlocks::POLISHED_BLACKSTONE_PRESSURE_PLATE,
        "polished_blackstone_pressure_plate",
        ItemProperties().maxStackSize(64));
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

    // 门
    OAK_DOOR =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_DOOR, "oak_door", ItemProperties().maxStackSize(64));
    SPRUCE_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_DOOR, "spruce_door", ItemProperties().maxStackSize(64));
    BIRCH_DOOR =
        &registerBlockBackedItem(registry, VanillaBlocks::BIRCH_DOOR, "birch_door", ItemProperties().maxStackSize(64));
    JUNGLE_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_DOOR, "jungle_door", ItemProperties().maxStackSize(64));
    ACACIA_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_DOOR, "acacia_door", ItemProperties().maxStackSize(64));
    DARK_OAK_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_DOOR, "dark_oak_door", ItemProperties().maxStackSize(64));
    MANGROVE_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::MANGROVE_DOOR, "mangrove_door", ItemProperties().maxStackSize(64));
    CHERRY_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::CHERRY_DOOR, "cherry_door", ItemProperties().maxStackSize(64));
    PALE_OAK_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::PALE_OAK_DOOR, "pale_oak_door", ItemProperties().maxStackSize(64));
    BAMBOO_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_DOOR, "bamboo_door", ItemProperties().maxStackSize(64));
    CRIMSON_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_DOOR, "crimson_door", ItemProperties().maxStackSize(64));
    WARPED_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_DOOR, "warped_door", ItemProperties().maxStackSize(64));
    IRON_DOOR =
        &registerBlockBackedItem(registry, VanillaBlocks::IRON_DOOR, "iron_door", ItemProperties().maxStackSize(64));

    // 铜门（8 种氧化/涂蜡变种）
    COPPER_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::COPPER_DOOR, "copper_door", ItemProperties().maxStackSize(64));
    EXPOSED_COPPER_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::EXPOSED_COPPER_DOOR, "exposed_copper_door", ItemProperties().maxStackSize(64));
    WEATHERED_COPPER_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::WEATHERED_COPPER_DOOR, "weathered_copper_door", ItemProperties().maxStackSize(64));
    OXIDIZED_COPPER_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::OXIDIZED_COPPER_DOOR, "oxidized_copper_door", ItemProperties().maxStackSize(64));
    WAXED_COPPER_DOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::WAXED_COPPER_DOOR, "waxed_copper_door", ItemProperties().maxStackSize(64));
    WAXED_EXPOSED_COPPER_DOOR = &registerBlockBackedItem(registry,
        VanillaBlocks::WAXED_EXPOSED_COPPER_DOOR,
        "waxed_exposed_copper_door",
        ItemProperties().maxStackSize(64));
    WAXED_WEATHERED_COPPER_DOOR = &registerBlockBackedItem(registry,
        VanillaBlocks::WAXED_WEATHERED_COPPER_DOOR,
        "waxed_weathered_copper_door",
        ItemProperties().maxStackSize(64));
    WAXED_OXIDIZED_COPPER_DOOR = &registerBlockBackedItem(registry,
        VanillaBlocks::WAXED_OXIDIZED_COPPER_DOOR,
        "waxed_oxidized_copper_door",
        ItemProperties().maxStackSize(64));

    // 栅栏
    OAK_FENCE =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_FENCE, "oak_fence", ItemProperties().maxStackSize(64));
    SPRUCE_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_FENCE, "spruce_fence", ItemProperties().maxStackSize(64));
    BIRCH_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_FENCE, "birch_fence", ItemProperties().maxStackSize(64));
    JUNGLE_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_FENCE, "jungle_fence", ItemProperties().maxStackSize(64));
    ACACIA_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_FENCE, "acacia_fence", ItemProperties().maxStackSize(64));
    DARK_OAK_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_FENCE, "dark_oak_fence", ItemProperties().maxStackSize(64));
    MANGROVE_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::MANGROVE_FENCE, "mangrove_fence", ItemProperties().maxStackSize(64));
    CHERRY_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::CHERRY_FENCE, "cherry_fence", ItemProperties().maxStackSize(64));
    PALE_OAK_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::PALE_OAK_FENCE, "pale_oak_fence", ItemProperties().maxStackSize(64));
    BAMBOO_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_FENCE, "bamboo_fence", ItemProperties().maxStackSize(64));
    NETHER_BRICK_FENCE = &registerBlockBackedItem(
        registry, VanillaBlocks::NETHER_BRICK_FENCE, "nether_brick_fence", ItemProperties().maxStackSize(64));

    // 栅栏门
    OAK_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::OAK_FENCE_GATE, "oak_fence_gate", ItemProperties().maxStackSize(64));
    SPRUCE_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_FENCE_GATE, "spruce_fence_gate", ItemProperties().maxStackSize(64));
    BIRCH_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_FENCE_GATE, "birch_fence_gate", ItemProperties().maxStackSize(64));
    JUNGLE_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_FENCE_GATE, "jungle_fence_gate", ItemProperties().maxStackSize(64));
    ACACIA_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_FENCE_GATE, "acacia_fence_gate", ItemProperties().maxStackSize(64));
    DARK_OAK_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_FENCE_GATE, "dark_oak_fence_gate", ItemProperties().maxStackSize(64));
    MANGROVE_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::MANGROVE_FENCE_GATE, "mangrove_fence_gate", ItemProperties().maxStackSize(64));
    CHERRY_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::CHERRY_FENCE_GATE, "cherry_fence_gate", ItemProperties().maxStackSize(64));
    PALE_OAK_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::PALE_OAK_FENCE_GATE, "pale_oak_fence_gate", ItemProperties().maxStackSize(64));
    BAMBOO_FENCE_GATE = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_FENCE_GATE, "bamboo_fence_gate", ItemProperties().maxStackSize(64));

    // 活板门
    OAK_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::OAK_TRAPDOOR, "oak_trapdoor", ItemProperties().maxStackSize(64));
    SPRUCE_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_TRAPDOOR, "spruce_trapdoor", ItemProperties().maxStackSize(64));
    BIRCH_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_TRAPDOOR, "birch_trapdoor", ItemProperties().maxStackSize(64));
    JUNGLE_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_TRAPDOOR, "jungle_trapdoor", ItemProperties().maxStackSize(64));
    ACACIA_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_TRAPDOOR, "acacia_trapdoor", ItemProperties().maxStackSize(64));
    DARK_OAK_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_TRAPDOOR, "dark_oak_trapdoor", ItemProperties().maxStackSize(64));
    MANGROVE_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::MANGROVE_TRAPDOOR, "mangrove_trapdoor", ItemProperties().maxStackSize(64));
    CHERRY_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::CHERRY_TRAPDOOR, "cherry_trapdoor", ItemProperties().maxStackSize(64));
    PALE_OAK_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::PALE_OAK_TRAPDOOR, "pale_oak_trapdoor", ItemProperties().maxStackSize(64));
    BAMBOO_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::BAMBOO_TRAPDOOR, "bamboo_trapdoor", ItemProperties().maxStackSize(64));
    CRIMSON_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::CRIMSON_TRAPDOOR, "crimson_trapdoor", ItemProperties().maxStackSize(64));
    WARPED_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::WARPED_TRAPDOOR, "warped_trapdoor", ItemProperties().maxStackSize(64));
    IRON_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::IRON_TRAPDOOR, "iron_trapdoor", ItemProperties().maxStackSize(64));

    // 铜活板门（8 种氧化/涂蜡变种）
    COPPER_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::COPPER_TRAPDOOR, "copper_trapdoor", ItemProperties().maxStackSize(64));
    EXPOSED_COPPER_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::EXPOSED_COPPER_TRAPDOOR, "exposed_copper_trapdoor", ItemProperties().maxStackSize(64));
    WEATHERED_COPPER_TRAPDOOR = &registerBlockBackedItem(registry,
        VanillaBlocks::WEATHERED_COPPER_TRAPDOOR,
        "weathered_copper_trapdoor",
        ItemProperties().maxStackSize(64));
    OXIDIZED_COPPER_TRAPDOOR = &registerBlockBackedItem(registry,
        VanillaBlocks::OXIDIZED_COPPER_TRAPDOOR,
        "oxidized_copper_trapdoor",
        ItemProperties().maxStackSize(64));
    WAXED_COPPER_TRAPDOOR = &registerBlockBackedItem(
        registry, VanillaBlocks::WAXED_COPPER_TRAPDOOR, "waxed_copper_trapdoor", ItemProperties().maxStackSize(64));
    WAXED_EXPOSED_COPPER_TRAPDOOR = &registerBlockBackedItem(registry,
        VanillaBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR,
        "waxed_exposed_copper_trapdoor",
        ItemProperties().maxStackSize(64));
    WAXED_WEATHERED_COPPER_TRAPDOOR = &registerBlockBackedItem(registry,
        VanillaBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR,
        "waxed_weathered_copper_trapdoor",
        ItemProperties().maxStackSize(64));
    WAXED_OXIDIZED_COPPER_TRAPDOOR = &registerBlockBackedItem(registry,
        VanillaBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR,
        "waxed_oxidized_copper_trapdoor",
        ItemProperties().maxStackSize(64));

    // 楼梯、台阶、墙
    OAK_STAIRS =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_STAIRS, "oak_stairs", ItemProperties().maxStackSize(64));
    SPRUCE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_STAIRS, "spruce_stairs", ItemProperties().maxStackSize(64));
    BIRCH_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::BIRCH_STAIRS, "birch_stairs", ItemProperties().maxStackSize(64));
    JUNGLE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_STAIRS, "jungle_stairs", ItemProperties().maxStackSize(64));
    ACACIA_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_STAIRS, "acacia_stairs", ItemProperties().maxStackSize(64));
    DARK_OAK_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_STAIRS, "dark_oak_stairs", ItemProperties().maxStackSize(64));
    STONE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_STAIRS, "stone_stairs", ItemProperties().maxStackSize(64));
    COBBLESTONE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::COBBLESTONE_STAIRS, "cobblestone_stairs", ItemProperties().maxStackSize(64));
    SANDSTONE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::SANDSTONE_STAIRS, "sandstone_stairs", ItemProperties().maxStackSize(64));
    SMOOTH_SANDSTONE_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::SMOOTH_SANDSTONE_STAIRS, "smooth_sandstone_stairs", ItemProperties().maxStackSize(64));
    STONE_BRICK_STAIRS = &registerBlockBackedItem(
        registry, VanillaBlocks::STONE_BRICK_STAIRS, "stone_brick_stairs", ItemProperties().maxStackSize(64));
    MOSSY_STONE_BRICK_STAIRS = &registerBlockBackedItem(registry,
        VanillaBlocks::MOSSY_STONE_BRICK_STAIRS,
        "mossy_stone_brick_stairs",
        ItemProperties().maxStackSize(64));
    OAK_SLAB =
        &registerBlockBackedItem(registry, VanillaBlocks::OAK_SLAB, "oak_slab", ItemProperties().maxStackSize(64));
    SPRUCE_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::SPRUCE_SLAB, "spruce_slab", ItemProperties().maxStackSize(64));
    BIRCH_SLAB =
        &registerBlockBackedItem(registry, VanillaBlocks::BIRCH_SLAB, "birch_slab", ItemProperties().maxStackSize(64));
    JUNGLE_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::JUNGLE_SLAB, "jungle_slab", ItemProperties().maxStackSize(64));
    ACACIA_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::ACACIA_SLAB, "acacia_slab", ItemProperties().maxStackSize(64));
    DARK_OAK_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::DARK_OAK_SLAB, "dark_oak_slab", ItemProperties().maxStackSize(64));
    STONE_SLAB =
        &registerBlockBackedItem(registry, VanillaBlocks::STONE_SLAB, "stone_slab", ItemProperties().maxStackSize(64));
    COBBLESTONE_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::COBBLESTONE_SLAB, "cobblestone_slab", ItemProperties().maxStackSize(64));
    SANDSTONE_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::SANDSTONE_SLAB, "sandstone_slab", ItemProperties().maxStackSize(64));
    SMOOTH_SANDSTONE_SLAB = &registerBlockBackedItem(
        registry, VanillaBlocks::SMOOTH_SANDSTONE_SLAB, "smooth_sandstone_slab", ItemProperties().maxStackSize(64));
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

    // 狂风杖 - 旋风人掉落的材料物品
    // 可用于酿造风充药水、合成重锤和锻造模板
    BREEZE_ROD = &registry.registerItem<item::BreezeRodItem>(
        ResourceLocation("minecraft:breeze_rod"), ItemProperties().maxStackSize(64));

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

    // 下界合金升级锻造模板 - 用于在锻造台中升级钻石装备为下界合金装备
    // 可通过在工作台中用 1x 下界合金升级模板 + 1x 钻石 + 7x 下界合金锭 复制
    NETHERITE_UPGRADE_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:netherite_upgrade_smithing_template"),
        item::SmithingTemplateType::NetheriteUpgrade,
        "item.minecraft.smithing_template.netherite_upgrade.applies_to",
        "item.minecraft.smithing_template.netherite_upgrade.ingredients",
        "item.minecraft.smithing_template.netherite_upgrade.base_slot_description",
        "item.minecraft.smithing_template.netherite_upgrade.additions_slot_description",
        item::SmithingTemplateItem::netheriteUpgradeProperties());

    // 盔甲纹饰锻造模板 - 用于在锻造台中为盔甲添加纹饰
    // 18种纹饰模板，每种可通过工作台用 1x 模板 + 1x 钻石 + 对应材料 复制
    SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:sentry_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    VEX_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:vex_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    WILD_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:wild_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    COAST_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:coast_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    DUNE_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:dune_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    WAYFINDER_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:wayfinder_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    RAISER_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:raiser_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    SHAPER_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:shaper_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    HOST_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:host_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    WARD_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:ward_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    SILENCE_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:silence_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    TIDE_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:tide_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    SNOUT_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:snout_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    RIB_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:rib_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    EYE_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:eye_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    SPIRE_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:spire_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    FLOW_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:flow_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    BOLT_ARMOR_TRIM_SMITHING_TEMPLATE = &registry.registerItem<item::SmithingTemplateItem>(
        ResourceLocation("minecraft:bolt_armor_trim_smithing_template"),
        item::SmithingTemplateType::ArmorTrim,
        "item.minecraft.smithing_template.armor_trim.applies_to",
        "item.minecraft.smithing_template.armor_trim.ingredients",
        "item.minecraft.smithing_template.armor_trim.base_slot_description",
        "item.minecraft.smithing_template.armor_trim.additions_slot_description",
        item::SmithingTemplateItem::armorTrimProperties());

    // ========================================================================
    // 陶片物品
    // ========================================================================

    // 1.20 考古学陶片 - 通过考古发掘获取（遗迹废墟、沙漠井、沙漠神殿、平原考古点等）

    ANGLER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:angler_pottery_sherd"),
            blockentity::DecoratedPotPattern::Angler,
            ItemProperties().maxStackSize(64));

    ARCHER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:archer_pottery_sherd"),
            blockentity::DecoratedPotPattern::Archer,
            ItemProperties().maxStackSize(64));

    ARMS_UP_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:arms_up_pottery_sherd"),
            blockentity::DecoratedPotPattern::ArmsUp,
            ItemProperties().maxStackSize(64));

    BLADE_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:blade_pottery_sherd"),
            blockentity::DecoratedPotPattern::Blade,
            ItemProperties().maxStackSize(64));

    BREWER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:brewer_pottery_sherd"),
            blockentity::DecoratedPotPattern::Brewer,
            ItemProperties().maxStackSize(64));

    BURN_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:burn_pottery_sherd"),
            blockentity::DecoratedPotPattern::Burn,
            ItemProperties().maxStackSize(64));

    DANGER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:danger_pottery_sherd"),
            blockentity::DecoratedPotPattern::Danger,
            ItemProperties().maxStackSize(64));

    EXPLORER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:explorer_pottery_sherd"),
            blockentity::DecoratedPotPattern::Explorer,
            ItemProperties().maxStackSize(64));

    FRIEND_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:friend_pottery_sherd"),
            blockentity::DecoratedPotPattern::Friend,
            ItemProperties().maxStackSize(64));

    HEART_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:heart_pottery_sherd"),
            blockentity::DecoratedPotPattern::Heart,
            ItemProperties().maxStackSize(64));

    HEARTBREAK_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:heartbreak_pottery_sherd"),
            blockentity::DecoratedPotPattern::Heartbreak,
            ItemProperties().maxStackSize(64));

    HOWL_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:howl_pottery_sherd"),
            blockentity::DecoratedPotPattern::Howl,
            ItemProperties().maxStackSize(64));

    MINER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:miner_pottery_sherd"),
            blockentity::DecoratedPotPattern::Miner,
            ItemProperties().maxStackSize(64));

    MOURNER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:mourner_pottery_sherd"),
            blockentity::DecoratedPotPattern::Mourner,
            ItemProperties().maxStackSize(64));

    PLENTY_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:plenty_pottery_sherd"),
            blockentity::DecoratedPotPattern::Plenty,
            ItemProperties().maxStackSize(64));

    PRIZE_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:prize_pottery_sherd"),
            blockentity::DecoratedPotPattern::Prize,
            ItemProperties().maxStackSize(64));

    SHEAF_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:sheaf_pottery_sherd"),
            blockentity::DecoratedPotPattern::Sheaf,
            ItemProperties().maxStackSize(64));

    SHELTER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:shelter_pottery_sherd"),
            blockentity::DecoratedPotPattern::Shelter,
            ItemProperties().maxStackSize(64));

    SKULL_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:skull_pottery_sherd"),
            blockentity::DecoratedPotPattern::Skull,
            ItemProperties().maxStackSize(64));

    SNORT_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:snort_pottery_sherd"),
            blockentity::DecoratedPotPattern::Snort,
            ItemProperties().maxStackSize(64));

    // 1.21 试炼密室陶片 - 通过试炼密室饰纹陶罐掉落获取

    FLOW_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:flow_pottery_sherd"),
            blockentity::DecoratedPotPattern::Flow,
            ItemProperties().maxStackSize(64));

    GUSTER_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:guster_pottery_sherd"),
            blockentity::DecoratedPotPattern::Guster,
            ItemProperties().maxStackSize(64));

    SCRAPE_POTTERY_SHERD =
        &registry.registerItem<item::PotterySherdItem>(ResourceLocation("minecraft:scrape_pottery_sherd"),
            blockentity::DecoratedPotPattern::Scrape,
            ItemProperties().maxStackSize(64));
}

void Items::_registerMusicDiscs()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 音乐唱片
    // ========================================================================
    // 经典唱片 (1.16.5): 信号强度 1-12
    // 后续版本新增: 信号强度 13-15

    MUSIC_DISC_13 = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_13"),
        1,
        SoundEvents::MUSIC_DISC_13,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_CAT = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_cat"),
        2,
        SoundEvents::MUSIC_DISC_CAT,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_BLOCKS =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_blocks"),
            3,
            SoundEvents::MUSIC_DISC_BLOCKS,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_CHIRP =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_chirp"),
            4,
            SoundEvents::MUSIC_DISC_CHIRP,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_FAR = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_far"),
        5,
        SoundEvents::MUSIC_DISC_FAR,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_MALL = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_mall"),
        6,
        SoundEvents::MUSIC_DISC_MALL,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_MELLOHI =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_mellohi"),
            7,
            SoundEvents::MUSIC_DISC_MELLOHI,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_STAL = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_stal"),
        8,
        SoundEvents::MUSIC_DISC_STAL,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_STRAD =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_strad"),
            9,
            SoundEvents::MUSIC_DISC_STRAD,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_WARD = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_ward"),
        10,
        SoundEvents::MUSIC_DISC_WARD,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_11 = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_11"),
        11,
        SoundEvents::MUSIC_DISC_11,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_WAIT = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_wait"),
        12,
        SoundEvents::MUSIC_DISC_WAIT,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_PIGSTEP =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_pigstep"),
            13,
            SoundEvents::MUSIC_DISC_PIGSTEP,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    MUSIC_DISC_OTHERSIDE =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_otherside"),
            14,
            SoundEvents::MUSIC_DISC_OTHERSIDE,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    MUSIC_DISC_5 = &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_5"),
        15,
        SoundEvents::MUSIC_DISC_5,
        ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    MUSIC_DISC_RELIC =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_relic"),
            14,
            SoundEvents::MUSIC_DISC_RELIC,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    MUSIC_DISC_TEARS =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_tears"),
            10,
            SoundEvents::MUSIC_DISC_TEARS,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));

    MUSIC_DISC_CREATOR =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_creator"),
            12,
            SoundEvents::MUSIC_DISC_CREATOR,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    MUSIC_DISC_CREATOR_MUSIC_BOX =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_creator_music_box"),
            11,
            SoundEvents::MUSIC_DISC_CREATOR_MUSIC_BOX,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    MUSIC_DISC_PRECIPICE =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_precipice"),
            13,
            SoundEvents::MUSIC_DISC_PRECIPICE,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Rare));

    MUSIC_DISC_LAVA_CHICKEN =
        &registry.registerItem<item::items::MusicDiscItem>(ResourceLocation("minecraft:music_disc_lava_chicken"),
            9,
            SoundEvents::MUSIC_DISC_LAVA_CHICKEN,
            ItemProperties().maxStackSize(1).rarity(ItemRarity::Common));
}

void Items::_registerSkulls()
{
    auto& registry = ItemRegistry::instance();

    // ========================================================================
    // 头颅物品
    //
    // MC Java 中头颅物品使用 StandingAndWallBlockItem（本项目对应 WallOrFloorItem），
    // 因为头颅可以放置在地板上或墙壁上。当前 SkullBlock / WallSkullBlock 尚未实现，
    // TODO: 待 SkullBlock/WallSkullBlock 实现后，升级为 WallOrFloorItem 注册，
    // 格式如下：
    //   PLAYER_HEAD = &registry.registerItem<WallOrFloorItem>(
    //       ResourceLocation("minecraft:player_head"),
    //       *VanillaBlocks::PLAYER_HEAD,
    //       *VanillaBlocks::PLAYER_WALL_HEAD,
    //       ItemProperties().maxStackSize(64));
    //
    // 参考: net.minecraft.world.item.StandingAndWallBlockItem
    // 参考: net.minecraft.world.level.block.SkullBlock (7 种头颅类型)
    // ========================================================================

    SKELETON_SKULL =
        &registry.registerItem(ResourceLocation("minecraft:skeleton_skull"), ItemProperties().maxStackSize(64));

    WITHER_SKELETON_SKULL =
        &registry.registerItem(ResourceLocation("minecraft:wither_skeleton_skull"), ItemProperties().maxStackSize(64));

    PLAYER_HEAD = &registry.registerItem(ResourceLocation("minecraft:player_head"), ItemProperties().maxStackSize(64));

    ZOMBIE_HEAD = &registry.registerItem(ResourceLocation("minecraft:zombie_head"), ItemProperties().maxStackSize(64));

    CREEPER_HEAD =
        &registry.registerItem(ResourceLocation("minecraft:creeper_head"), ItemProperties().maxStackSize(64));

    DRAGON_HEAD = &registry.registerItem(ResourceLocation("minecraft:dragon_head"), ItemProperties().maxStackSize(64));

    PIGLIN_HEAD = &registry.registerItem(ResourceLocation("minecraft:piglin_head"), ItemProperties().maxStackSize(64));
}

void Items::_registerHarnesses()
{
    auto& registry = ItemRegistry::instance();

    // 欢乐诡鬼装备 (Harness) - MC 1.21.11 新增
    // 用于装备 HappyGhast 实体，无护甲值、无耐久、可染色合成
    // 每种颜色对应一个独立 Item 实例（颜色为物品固有属性，非 NBT 染色）
    WHITE_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:white_harness"), ItemProperties().maxStackSize(1), DyeColor::White);

    ORANGE_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:orange_harness"), ItemProperties().maxStackSize(1), DyeColor::Orange);

    MAGENTA_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:magenta_harness"), ItemProperties().maxStackSize(1), DyeColor::Magenta);

    LIGHT_BLUE_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:light_blue_harness"), ItemProperties().maxStackSize(1), DyeColor::LightBlue);

    YELLOW_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:yellow_harness"), ItemProperties().maxStackSize(1), DyeColor::Yellow);

    LIME_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:lime_harness"), ItemProperties().maxStackSize(1), DyeColor::Lime);

    PINK_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:pink_harness"), ItemProperties().maxStackSize(1), DyeColor::Pink);

    GRAY_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:gray_harness"), ItemProperties().maxStackSize(1), DyeColor::Gray);

    LIGHT_GRAY_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:light_gray_harness"), ItemProperties().maxStackSize(1), DyeColor::LightGray);

    CYAN_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:cyan_harness"), ItemProperties().maxStackSize(1), DyeColor::Cyan);

    PURPLE_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:purple_harness"), ItemProperties().maxStackSize(1), DyeColor::Purple);

    BLUE_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:blue_harness"), ItemProperties().maxStackSize(1), DyeColor::Blue);

    BROWN_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:brown_harness"), ItemProperties().maxStackSize(1), DyeColor::Brown);

    GREEN_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:green_harness"), ItemProperties().maxStackSize(1), DyeColor::Green);

    RED_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:red_harness"), ItemProperties().maxStackSize(1), DyeColor::Red);

    BLACK_HARNESS = &registry.registerItem<item::items::HarnessItem>(
        ResourceLocation("minecraft:black_harness"), ItemProperties().maxStackSize(1), DyeColor::Black);
}

void Items::_registerBundles()
{
    auto& registry = ItemRegistry::instance();

    // 收纳袋 (Bundle) - MC 1.21.11 实验性物品
    // 17 个变体：1 无色 + 16 色，颜色为物品固有属性
    // 通过 BundleContents 存储多个物品堆，权重系统限制总容量（MAX_WEIGHT=64）
    // 用法：右键开始使用 → 周期性丢出内容物；或在物品栏点击槽位插入/取出
    BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:bundle"), ItemProperties().maxStackSize(1), DyeColor::Count);

    WHITE_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:white_bundle"), ItemProperties().maxStackSize(1), DyeColor::White);

    ORANGE_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:orange_bundle"), ItemProperties().maxStackSize(1), DyeColor::Orange);

    MAGENTA_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:magenta_bundle"), ItemProperties().maxStackSize(1), DyeColor::Magenta);

    LIGHT_BLUE_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:light_blue_bundle"), ItemProperties().maxStackSize(1), DyeColor::LightBlue);

    YELLOW_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:yellow_bundle"), ItemProperties().maxStackSize(1), DyeColor::Yellow);

    LIME_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:lime_bundle"), ItemProperties().maxStackSize(1), DyeColor::Lime);

    PINK_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:pink_bundle"), ItemProperties().maxStackSize(1), DyeColor::Pink);

    GRAY_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:gray_bundle"), ItemProperties().maxStackSize(1), DyeColor::Gray);

    LIGHT_GRAY_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:light_gray_bundle"), ItemProperties().maxStackSize(1), DyeColor::LightGray);

    CYAN_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:cyan_bundle"), ItemProperties().maxStackSize(1), DyeColor::Cyan);

    PURPLE_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:purple_bundle"), ItemProperties().maxStackSize(1), DyeColor::Purple);

    BLUE_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:blue_bundle"), ItemProperties().maxStackSize(1), DyeColor::Blue);

    BROWN_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:brown_bundle"), ItemProperties().maxStackSize(1), DyeColor::Brown);

    GREEN_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:green_bundle"), ItemProperties().maxStackSize(1), DyeColor::Green);

    RED_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:red_bundle"), ItemProperties().maxStackSize(1), DyeColor::Red);

    BLACK_BUNDLE = &registry.registerItem<item::items::BundleItem>(
        ResourceLocation("minecraft:black_bundle"), ItemProperties().maxStackSize(1), DyeColor::Black);
}

void Items::_registerSpawnEggs()
{
    auto& registry = ItemRegistry::instance();

    // 刷怪蛋物品 - MC 1.21.11 net.minecraft.world.item.SpawnEggs
    // 颜色数据来自 SpawnEggs.DEFAULT_ENTITY_IDS_TO_EGGS 的 background/foreground (ARGB)
    // ItemType: primaryColor=背景色, secondaryColor=前景色（斑点色）
    // 注册名为 minecraft:<entity>_spawn_egg，maxStackSize=64
    // SpawnEggItem 内部持有的 EntityType 仅作为名称载体，实际实体生成
    // 通过 EntityRegistry::getType(name)->create() 完成（见 MobEntity::_spawnOffspringFromSpawnEgg
    // 与 SpawnEggItem::spawnEntity），因此工厂可为空。
    ALLAY_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:allay_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:allay"),
        0x0A96B4FF,
        0xCE8AC3FF,
        ItemProperties().maxStackSize(64));

    ARMADILLO_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:armadillo_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:armadillo"),
        0x625D5DFF,
        0xC5B8A0FF,
        ItemProperties().maxStackSize(64));

    AXOLOTL_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:axolotl_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:axolotl"),
        0x4838A3FF,
        0xA8F8E788,
        ItemProperties().maxStackSize(64));

    BAT_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:bat_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:bat"),
        0x4C3E4CFF,
        0x251627FF,
        ItemProperties().maxStackSize(64));

    BEE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:bee_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:bee"),
        0xEDC343FF,
        0xD68B26FF,
        ItemProperties().maxStackSize(64));

    BLAZE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:blaze_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:blaze"),
        0xF6B201FF,
        0xFFFFE8B0,
        ItemProperties().maxStackSize(64));

    BOGGED_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:bogged_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:bogged"),
        0x876B62FF,
        0x547E9BFF,
        ItemProperties().maxStackSize(64));

    BREEZE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:breeze_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:breeze"),
        0x757F7CFF,
        0xC6D6D2FF,
        ItemProperties().maxStackSize(64));

    CAT_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:cat_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:cat"),
        0x161616FF,
        0xDEDEDEFF,
        ItemProperties().maxStackSize(64));

    CAMEL_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:camel_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:camel"),
        0xC5C28EFF,
        0xE4DDC9FF,
        ItemProperties().maxStackSize(64));

    // TODO: camel_husk_spawn_egg 依赖 CamelHuskEntity 注册实现。当前 CamelHuskEntity 尚未在
    // EntityRegistry 中注册，MobEntity::_spawnOffspringFromSpawnEgg 通过
    // EntityRegistry::getType("minecraft:camel_husk") 查找实体会返回 nullptr，导致此刷怪蛋
    // 右键方块/右键生物时无法生成实体。待 CamelHuskEntity 实现并在 VanillaEntities 中注册后生效。
    CAMEL_HUSK_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:camel_husk_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:camel_husk"),
            0x383127FF,
            0x544937FF,
            ItemProperties().maxStackSize(64));

    CAVE_SPIDER_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:cave_spider_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:cave_spider"),
            0x0F2F0FFF,
            0x0C2E0CFF,
            ItemProperties().maxStackSize(64));

    CHICKEN_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:chicken_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:chicken"),
        0xA1A1A1FF,
        0xFFA31AFF,
        ItemProperties().maxStackSize(64));

    COD_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:cod_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:cod"),
        0xC4726CFF,
        0x465A82FF,
        ItemProperties().maxStackSize(64));

    COPPER_GOLEM_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:copper_golem_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:copper_golem"),
            0x783219FF,
            0xE3826CFF,
            ItemProperties().maxStackSize(64));

    COW_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:cow_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:cow"),
        0x45362DFF,
        0xC9A063FF,
        ItemProperties().maxStackSize(64));

    // TODO: creaking_spawn_egg 依赖 CreakingEntity 注册实现。当前 CreakingEntity 尚未在
    // EntityRegistry 中注册，MobEntity::_spawnOffspringFromSpawnEgg 通过
    // EntityRegistry::getType("minecraft:creaking") 查找实体会返回 nullptr，导致此刷怪蛋
    // 右键方块/右键生物时无法生成实体。待 CreakingEntity 实现并在 VanillaEntities 中注册后生效。
    CREAKING_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:creaking_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:creaking"),
        0x5A504EFF,
        0x8F968DFF,
        ItemProperties().maxStackSize(64));

    CREEPER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:creeper_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:creeper"),
        0x0DA70DFF,
        0x000000FF,
        ItemProperties().maxStackSize(64));

    DOLPHIN_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:dolphin_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:dolphin"),
        0x445D9BFF,
        0xF2F2F2FF,
        ItemProperties().maxStackSize(64));

    DONKEY_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:donkey_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:donkey"),
        0x34281FFF,
        0x4F3E2DFF,
        ItemProperties().maxStackSize(64));

    DROWNED_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:drowned_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:drowned"),
        0x899D9DFF,
        0x3030FFFF,
        ItemProperties().maxStackSize(64));

    ELDER_GUARDIAN_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:elder_guardian_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:elder_guardian"),
            0x4A7393FF,
            0x25D1C2FF,
            ItemProperties().maxStackSize(64));

    ENDER_DRAGON_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:ender_dragon_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:ender_dragon"),
            0x0F0F0FFF,
            0x9B6FCAFF,
            ItemProperties().maxStackSize(64));

    ENDERMAN_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:enderman_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:enderman"),
        0x111111FF,
        0x9B6FCAFF,
        ItemProperties().maxStackSize(64));

    ENDERMITE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:endermite_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:endermite"),
        0x161616FF,
        0x808071FF,
        ItemProperties().maxStackSize(64));

    EVOKER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:evoker_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:evoker"),
        0x959B9BFF,
        0x1B1616FF,
        ItemProperties().maxStackSize(64));

    FOX_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:fox_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:fox"),
        0xD58B2DFF,
        0xB5BBC0FF,
        ItemProperties().maxStackSize(64));

    FROG_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:frog_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:frog"),
        0x5D6A4DFF,
        0x9D9E0DFF,
        ItemProperties().maxStackSize(64));

    GHAST_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:ghast_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:ghast"),
        0xE9E9E9FF,
        0x4C4C68FF,
        ItemProperties().maxStackSize(64));

    GLOW_SQUID_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:glow_squid_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:glow_squid"),
            0x06223CFF,
            0x2BC6B3FF,
            ItemProperties().maxStackSize(64));

    GOAT_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:goat_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:goat"),
        0x8B7152FF,
        0x44522EFF,
        ItemProperties().maxStackSize(64));

    GUARDIAN_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:guardian_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:guardian"),
        0x4A7393FF,
        0x25D1C2FF,
        ItemProperties().maxStackSize(64));

    // TODO: happy_ghast_spawn_egg 依赖 HappyGhastEntity 注册实现。当前 HappyGhastEntity 尚未在
    // EntityRegistry 中注册，MobEntity::_spawnOffspringFromSpawnEgg 通过
    // EntityRegistry::getType("minecraft:happy_ghast") 查找实体会返回 nullptr，导致此刷怪蛋
    // 右键方块/右键生物时无法生成实体。待 HappyGhastEntity 实现并在 VanillaEntities 中注册后生效。
    HAPPY_GHAST_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:happy_ghast_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:happy_ghast"),
            0xF0F0F0FF,
            0xD8CCCCFF,
            ItemProperties().maxStackSize(64));

    HOGLIN_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:hoglin_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:hoglin"),
        0x5E2D29FF,
        0x6A4D44FF,
        ItemProperties().maxStackSize(64));

    HORSE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:horse_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:horse"),
        0x161616FF,
        0xC09E7DFF,
        ItemProperties().maxStackSize(64));

    HUSK_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:husk_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:husk"),
        0x303030FF,
        0x4B4A4BFF,
        ItemProperties().maxStackSize(64));

    IRON_GOLEM_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:iron_golem_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:iron_golem"),
            0xC9C9C9FF,
            0x6B6B6BFF,
            ItemProperties().maxStackSize(64));

    LLAMA_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:llama_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:llama"),
        0xC9BEBDFF,
        0x80705CFF,
        ItemProperties().maxStackSize(64));

    MAGMA_CUBE_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:magma_cube_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:magma_cube"),
            0x340000FF,
            0xFCFC00FF,
            ItemProperties().maxStackSize(64));

    MOOSHROOM_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:mooshroom_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:mooshroom"),
        0xA30505FF,
        0xE0CECEFF,
        ItemProperties().maxStackSize(64));

    MULE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:mule_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:mule"),
        0x34281FFF,
        0x4F3E2DFF,
        ItemProperties().maxStackSize(64));

    // 鹦鹉螺刷怪蛋：NautilusEntity 已在 VanillaEntities.hpp 注册为 WaterCreature
    NAUTILUS_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:nautilus_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:nautilus"),
        0xDB5A41FF,
        0x66452EFF,
        ItemProperties().maxStackSize(64));

    OCELOT_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:ocelot_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:ocelot"),
        0x161616FF,
        0xFFDE21FF,
        ItemProperties().maxStackSize(64));

    PANDA_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:panda_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:panda"),
        0x161616FF,
        0xDEDEDEFF,
        ItemProperties().maxStackSize(64));

    PARROT_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:parrot_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:parrot"),
        0x161616FF,
        0x15B7B7FF,
        ItemProperties().maxStackSize(64));

    // TODO: parched_spawn_egg 依赖 ParchedEntity 注册实现。当前 ParchedEntity 尚未在
    // EntityRegistry 中注册，MobEntity::_spawnOffspringFromSpawnEgg 通过
    // EntityRegistry::getType("minecraft:parched") 查找实体会返回 nullptr，导致此刷怪蛋
    // 右键方块/右键生物时无法生成实体。待 ParchedEntity 实现并在 VanillaEntities 中注册后生效。
    PARCHED_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:parched_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:parched"),
        0x746E66FF,
        0xAC986BFF,
        ItemProperties().maxStackSize(64));

    PHANTOM_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:phantom_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:phantom"),
        0x0F1111FF,
        0x4A4A4AFF,
        ItemProperties().maxStackSize(64));

    PIG_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:pig_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:pig"),
        0xF0A0A0FF,
        0xA05050FF,
        ItemProperties().maxStackSize(64));

    PIGLIN_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:piglin_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:piglin"),
        0xF0A0A0FF,
        0x9B6FCAFF,
        ItemProperties().maxStackSize(64));

    PIGLIN_BRUTE_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:piglin_brute_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:piglin_brute"),
            0xF0A0A0FF,
            0x5B2D2DFF,
            ItemProperties().maxStackSize(64));

    PILLAGER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:pillager_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:pillager"),
        0x5F7B7BFF,
        0x232323FF,
        ItemProperties().maxStackSize(64));

    POLAR_BEAR_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:polar_bear_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:polar_bear"),
            0xDEDEDEFF,
            0x7A7A7AFF,
            ItemProperties().maxStackSize(64));

    PUFFERFISH_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:pufferfish_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:pufferfish"),
            0x5F5F5FFF,
            0xFFFFFF88,
            ItemProperties().maxStackSize(64));

    RABBIT_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:rabbit_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:rabbit"),
        0x995F33FF,
        0x75482DFF,
        ItemProperties().maxStackSize(64));

    RAVAGER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:ravager_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:ravager"),
        0x8B6E49FF,
        0x3A3728FF,
        ItemProperties().maxStackSize(64));

    SALMON_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:salmon_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:salmon"),
        0x624F4FFF,
        0x9F8371FF,
        ItemProperties().maxStackSize(64));

    SHEEP_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:sheep_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:sheep"),
        0xDBD3B0FF,
        0xDEDEDEFF,
        ItemProperties().maxStackSize(64));

    SHULKER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:shulker_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:shulker"),
        0x9E6976FF,
        0x976D6DFF,
        ItemProperties().maxStackSize(64));

    SILVERFISH_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:silverfish_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:silverfish"),
            0x8C8C8CFF,
            0x636363FF,
            ItemProperties().maxStackSize(64));

    SKELETON_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:skeleton_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:skeleton"),
        0xC1C1C1FF,
        0x494949FF,
        ItemProperties().maxStackSize(64));

    SKELETON_HORSE_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:skeleton_horse_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:skeleton_horse"),
            0x161616FF,
            0x6A6A6AFF,
            ItemProperties().maxStackSize(64));

    SLIME_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:slime_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:slime"),
        0x518C5EFF,
        0x40A85AFF,
        ItemProperties().maxStackSize(64));

    SNIFFER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:sniffer_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:sniffer"),
        0xB67250FF,
        0xECB478FF,
        ItemProperties().maxStackSize(64));

    SNOW_GOLEM_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:snow_golem_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:snow_golem"),
            0xEEEEEEFF,
            0x6A6A6AFF,
            ItemProperties().maxStackSize(64));

    SPIDER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:spider_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:spider"),
        0x342D27FF,
        0x0F2F0FFF,
        ItemProperties().maxStackSize(64));

    SQUID_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:squid_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:squid"),
        0x223B4DFF,
        0x708899FF,
        ItemProperties().maxStackSize(64));

    STRAY_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:stray_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:stray"),
        0x0F2F2FFF,
        0x9CA5A5FF,
        ItemProperties().maxStackSize(64));

    STRIDER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:strider_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:strider"),
        0x9B4D38FF,
        0x4E4044FF,
        ItemProperties().maxStackSize(64));

    TADPOLE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:tadpole_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:tadpole"),
        0x103132FF,
        0x776E1DFF,
        ItemProperties().maxStackSize(64));

    TRADER_LLAMA_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:trader_llama_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:trader_llama"),
            0xC9BEBDFF,
            0x80705CFF,
            ItemProperties().maxStackSize(64));

    TROPICAL_FISH_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:tropical_fish_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:tropical_fish"),
            0x06223CFF,
            0xFFF8E9FF,
            ItemProperties().maxStackSize(64));

    TURTLE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:turtle_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:turtle"),
        0x307371FF,
        0xFFCC55FF,
        ItemProperties().maxStackSize(64));

    VEX_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:vex_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:vex"),
        0x6A7B8DFF,
        0x6E8B9EFF,
        ItemProperties().maxStackSize(64));

    VILLAGER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:villager_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:villager"),
        0x563C33FF,
        0x88B1BFFF,
        ItemProperties().maxStackSize(64));

    VINDICATOR_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:vindicator_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:vindicator"),
            0x959B9BFF,
            0x275E61FF,
            ItemProperties().maxStackSize(64));

    WANDERING_TRADER_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:wandering_trader_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:wandering_trader"),
            0x4A4A4AFF,
            0x9DB3D6FF,
            ItemProperties().maxStackSize(64));

    WARDEN_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:warden_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:warden"),
        0x0D0E26FF,
        0x116D9BFF,
        ItemProperties().maxStackSize(64));

    WITCH_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:witch_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:witch"),
        0x340000FF,
        0x51A7ADFF,
        ItemProperties().maxStackSize(64));

    WITHER_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:wither_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:wither"),
        0x161616FF,
        0x333333FF,
        ItemProperties().maxStackSize(64));

    WITHER_SKELETON_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:wither_skeleton_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:wither_skeleton"),
            0x181818FF,
            0x6B6B6BFF,
            ItemProperties().maxStackSize(64));

    WOLF_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:wolf_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:wolf"),
        0xD7D3D3FF,
        0xCEAFB1FF,
        ItemProperties().maxStackSize(64));

    ZOGLIN_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:zoglin_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:zoglin"),
        0x5E2D29FF,
        0x442E5DFF,
        ItemProperties().maxStackSize(64));

    ZOMBIE_SPAWN_EGG = &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:zombie_spawn_egg"),
        makeEntityTypeForSpawnEgg("minecraft:zombie"),
        0x00AFC1FF,
        0x006D76FF,
        ItemProperties().maxStackSize(64));

    ZOMBIE_HORSE_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:zombie_horse_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:zombie_horse"),
            0x161616FF,
            0x6A6A6AFF,
            ItemProperties().maxStackSize(64));

    // 僵尸鹦鹉螺刷怪蛋：ZombieNautilusEntity 已在 VanillaEntities.hpp 注册为 WaterCreature
    ZOMBIE_NAUTILUS_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:zombie_nautilus_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:zombie_nautilus"),
            0x776C49FF,
            0x615633FF,
            ItemProperties().maxStackSize(64));

    ZOMBIFIED_PIGLIN_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:zombified_piglin_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:zombified_piglin"),
            0xEA9393FF,
            0x4C7129FF,
            ItemProperties().maxStackSize(64));

    ZOMBIE_VILLAGER_SPAWN_EGG =
        &registry.registerItem<item::SpawnEggItem>(ResourceLocation("minecraft:zombie_villager_spawn_egg"),
            makeEntityTypeForSpawnEgg("minecraft:zombie_villager"),
            0x00AFC1FF,
            0x88B1BFFF,
            ItemProperties().maxStackSize(64));

    // TODO: 以下 6 种刷怪蛋对应的实体类型尚未在项目中注册，刷怪蛋物品本身已注册成功，
    // 但实际使用时（右键方块/右键生物生成幼体）MobEntity::_spawnOffspringFromSpawnEgg 或
    // SpawnEggItem::spawnEntity 通过 EntityRegistry::getType(name)->create() 查找实体会返回
    // nullptr，导致无法生成实体，功能不完整：
    //   - camel_husk_spawn_egg     → 依赖 CamelHuskEntity
    //   - creaking_spawn_egg       → 依赖 CreakingEntity
    //   - happy_ghast_spawn_egg    → 依赖 HappyGhastEntity
    //   - nautilus_spawn_egg       → 依赖 NautilusEntity
    //   - parched_spawn_egg        → 依赖 ParchedEntity
    //   - zombie_nautilus_spawn_egg → 依赖 ZombieNautilusEntity
    // 待上述实体类型实现并在 VanillaEntities::registerAll() 中注册后，刷怪蛋功能自动生效。
}

} // namespace mc
