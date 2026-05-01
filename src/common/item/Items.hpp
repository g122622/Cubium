#pragma once

#include "core/Item.hpp"
#include "core/ItemRegistry.hpp"

namespace mc {

/**
 * @brief 原版物品静态引用
 *
 * 提供所有原版物品的静态指针，便于快速访问。
 * 在游戏初始化时调用 Items::initialize() 进行注册。
 *
 * 参考: net.minecraft.item.Items
 */
class Items {
public:
    /**
     * @brief 初始化所有原版物品
     *
     * 必须在使用任何物品前调用。
     */
    static void initialize();

    // ========================================================================
    // 空气
    // ========================================================================
    static Item* AIR;

    // ========================================================================
    // 矿物和材料
    // ========================================================================
    static Item* DIAMOND;
    static Item* EMERALD;
    static Item* GOLD_INGOT;
    static Item* IRON_INGOT;
    static Item* COPPER_INGOT;
    static Item* NETHERITE_INGOT;
    static Item* NETHERITE_SCRAP;

    // ========================================================================
    // 宝石碎片
    // ========================================================================
    static Item* DIAMOND_SHARD;        // 自定义：钻石碎片（暂用）
    static Item* EMERALD_SHARD;        // 自定义：绿宝石碎片（暂用）

    // ========================================================================
    // 煤炭相关
    // ========================================================================
    static Item* COAL;
    static Item* CHARCOAL;

    // ========================================================================
    // 红石相关
    // ========================================================================
    static Item* REDSTONE;
    static Item* LAPIS_LAZULI;
    static Item* QUARTZ;
    static Item* GLOWSTONE_DUST;

    // ========================================================================
    // 矿物原矿
    // ========================================================================
    static Item* COAL_ORE;
    static Item* IRON_ORE;
    static Item* GOLD_ORE;
    static Item* DIAMOND_ORE;
    static Item* EMERALD_ORE;
    static Item* LAPIS_ORE;
    static Item* REDSTONE_ORE;
    static Item* COPPER_ORE;
    static Item* NETHER_QUARTZ_ORE;
    static Item* NETHER_GOLD_ORE;
    static Item* ANCIENT_DEBRIS;

    // ========================================================================
    // 工具 - 钻石
    // ========================================================================
    static Item* DIAMOND_PICKAXE;
    static Item* DIAMOND_AXE;
    static Item* DIAMOND_SHOVEL;
    static Item* DIAMOND_HOE;
    static Item* DIAMOND_SWORD;

    // ========================================================================
    // 工具 - 铁
    // ========================================================================
    static Item* IRON_PICKAXE;
    static Item* IRON_AXE;
    static Item* IRON_SHOVEL;
    static Item* IRON_HOE;
    static Item* IRON_SWORD;

    // ========================================================================
    // 工具 - 石
    // ========================================================================
    static Item* STONE_PICKAXE;
    static Item* STONE_AXE;
    static Item* STONE_SHOVEL;
    static Item* STONE_HOE;
    static Item* STONE_SWORD;

    // ========================================================================
    // 工具 - 木
    // ========================================================================
    static Item* WOODEN_PICKAXE;
    static Item* WOODEN_AXE;
    static Item* WOODEN_SHOVEL;
    static Item* WOODEN_HOE;
    static Item* WOODEN_SWORD;

    // ========================================================================
    // 工具 - 金
    // ========================================================================
    static Item* GOLDEN_PICKAXE;
    static Item* GOLDEN_AXE;
    static Item* GOLDEN_SHOVEL;
    static Item* GOLDEN_HOE;
    static Item* GOLDEN_SWORD;

    // ========================================================================
    // 工具 - 下界合金
    // ========================================================================
    static Item* NETHERITE_PICKAXE;
    static Item* NETHERITE_AXE;
    static Item* NETHERITE_SHOVEL;
    static Item* NETHERITE_HOE;
    static Item* NETHERITE_SWORD;

    // ========================================================================
    // 护甲 - 钻石
    // ========================================================================
    static Item* DIAMOND_HELMET;
    static Item* DIAMOND_CHESTPLATE;
    static Item* DIAMOND_LEGGINGS;
    static Item* DIAMOND_BOOTS;

    // ========================================================================
    // 护甲 - 铁
    // ========================================================================
    static Item* IRON_HELMET;
    static Item* IRON_CHESTPLATE;
    static Item* IRON_LEGGINGS;
    static Item* IRON_BOOTS;

    // ========================================================================
    // 护甲 - 金
    // ========================================================================
    static Item* GOLDEN_HELMET;
    static Item* GOLDEN_CHESTPLATE;
    static Item* GOLDEN_LEGGINGS;
    static Item* GOLDEN_BOOTS;

    // ========================================================================
    // 护甲 - 皮革
    // ========================================================================
    static Item* LEATHER_HELMET;
    static Item* LEATHER_CHESTPLATE;
    static Item* LEATHER_LEGGINGS;
    static Item* LEATHER_BOOTS;

    // ========================================================================
    // 护甲 - 锁链
    // ========================================================================
    static Item* CHAINMAIL_HELMET;
    static Item* CHAINMAIL_CHESTPLATE;
    static Item* CHAINMAIL_LEGGINGS;
    static Item* CHAINMAIL_BOOTS;

    // ========================================================================
    // 护甲 - 下界合金
    // ========================================================================
    static Item* NETHERITE_HELMET;
    static Item* NETHERITE_CHESTPLATE;
    static Item* NETHERITE_LEGGINGS;
    static Item* NETHERITE_BOOTS;

    // ========================================================================
    // 特殊护甲
    // ========================================================================
    static Item* TURTLE_HELMET;    // 海龟壳 - 在酿造材料部分已声明
    static Item* ELYTRA;           // 鞘翅

    // ========================================================================
    // 食物
    // ========================================================================
    static Item* APPLE;
    static Item* GOLDEN_APPLE;
    static Item* ENCHANTED_GOLDEN_APPLE;
    static Item* BREAD;
    static Item* COOKED_BEEF;
    static Item* COOKED_PORKCHOP;
    static Item* COOKED_CHICKEN;
    static Item* COOKED_MUTTON;
    static Item* COOKED_RABBIT;
    static Item* COOKED_COD;
    static Item* COOKED_SALMON;
    static Item* BEEF;
    static Item* PORKCHOP;
    static Item* CHICKEN;
    static Item* MUTTON;
    static Item* RABBIT;
    static Item* COD;
    static Item* SALMON;
    // 缺失的食物
    static Item* BAKED_POTATO;
    static Item* BEETROOT;
    static Item* BEETROOT_SOUP;
    static Item* CARROT;
    static Item* CHORUS_FRUIT;
    static Item* COOKIE;
    static Item* DRIED_KELP;
    static Item* GOLDEN_CARROT;
    static Item* HONEY_BOTTLE;
    static Item* MELON_SLICE;
    static Item* MUSHROOM_STEW;
    static Item* POISONOUS_POTATO;
    static Item* POTATO;
    static Item* PUFFERFISH;
    static Item* PUMPKIN_PIE;
    static Item* RABBIT_STEW;
    static Item* ROTTEN_FLESH;
    static Item* SPIDER_EYE;
    static Item* SUSPICIOUS_STEW;
    static Item* SWEET_BERRIES;
    static Item* TROPICAL_FISH;

    // ========================================================================
    // 木头和木板（合成基础材料）
    // ========================================================================
    static Item* OAK_LOG;
    static Item* SPRUCE_LOG;
    static Item* BIRCH_LOG;
    static Item* JUNGLE_LOG;
    static Item* ACACIA_LOG;
    static Item* DARK_OAK_LOG;

    static Item* OAK_PLANKS;
    static Item* SPRUCE_PLANKS;
    static Item* BIRCH_PLANKS;
    static Item* JUNGLE_PLANKS;
    static Item* ACACIA_PLANKS;
    static Item* DARK_OAK_PLANKS;

    // ========================================================================
    // 木棍和骨头
    // ========================================================================
    static Item* STICK;
    static Item* BONE;
    static Item* BONE_MEAL;

    // ========================================================================
    // 石头相关
    // ========================================================================
    static Item* STONE;
    static Item* COBBLESTONE;
    static Item* MOSSY_COBBLESTONE;

    // ========================================================================
    // 杂项
    // ========================================================================
    static Item* FLINT;
    static Item* FLINT_AND_STEEL;
    static Item* STRING;
    static Item* FEATHER;
    static Item* GUNPOWDER;
    static Item* LEATHER;
    static Item* SLIME_BALL;
    static Item* EGG;
    static Item* SNOWBALL;              // 雪球
    static Item* COMPASS;
    static Item* CLOCK;
    static Item* FERMENTED_SPIDER_EYE;
    static Item* BLAZE_ROD;
    static Item* BLAZE_POWDER;
    static Item* ENDER_PEARL;
    static Item* ENDER_EYE;
    static Item* NETHER_STAR;
    static Item* FIRE_CHARGE;
    static Item* FIREWORK_STAR;
    static Item* FIREWORK_ROCKET;
    static Item* EXPERIENCE_BOTTLE;     // 附魔之瓶

    // ========================================================================
    // 染料 (16色)
    // ========================================================================
    static Item* INK_SAC;
    static Item* RED_DYE;
    static Item* GREEN_DYE;
    static Item* COCOA_BEANS;
    static Item* LAPIS_LAZULI_DYE;
    static Item* PURPLE_DYE;
    static Item* CYAN_DYE;
    static Item* LIGHT_GRAY_DYE;
    static Item* GRAY_DYE;
    static Item* PINK_DYE;
    static Item* LIME_DYE;
    static Item* YELLOW_DYE;
    static Item* LIGHT_BLUE_DYE;
    static Item* MAGENTA_DYE;
    static Item* ORANGE_DYE;
    static Item* WHITE_DYE;

    // ========================================================================
    // 种子
    // ========================================================================
    static Item* WHEAT_SEEDS;
    static Item* PUMPKIN_SEEDS;
    static Item* MELON_SEEDS;
    static Item* BEETROOT_SEEDS;

    // ========================================================================
    // 农产品
    // ========================================================================
    static Item* WHEAT;
    static Item* PUMPKIN;
    static Item* MELON;
    // MELON_SLICE 在食物部分声明
    // CARROT 在食物部分声明
    // POTATO 在食物部分声明
    // BEETROOT 在食物部分声明
    static Item* SUGAR_CANE;
    static Item* SUGAR;

    // ========================================================================
    // 水域更新材料
    // ========================================================================
    static Item* SCUTE;                  // 鳞甲 - 海龟掉落
    static Item* HEART_OF_THE_SEA;       // 海洋之心 - 宝藏物品
    static Item* NAUTILUS_SHELL;         // 鹦鹉螺壳 - 溺尸掉落/钓鱼
    static Item* PHANTOM_MEMBRANE;       // 幻翼膜 - 幻翼掉落
    // DRIED_KELP 在食物部分声明

    // ========================================================================
    // 酿造材料
    // ========================================================================
    static Item* NETHER_WART;            // 地狱疣 - 酿造基础材料
    // GOLDEN_CARROT 在食物部分声明
    static Item* GHAST_TEAR;             // 恶魂之泪 - 生命恢复药水
    static Item* RABBIT_FOOT;            // 兔子脚 - 跳跃药水
    static Item* MAGMA_CREAM;            // 岩浆膏 - 防火药水
    static Item* DRAGON_BREATH;          // 龙息 - 滞留药水
    // PUFFERFISH 在食物部分声明
    // TURTLE_HELMET 在特殊护甲部分声明
    static Item* GLISTERING_MELON_SLICE; // 闪烁的西瓜片 - 瞬间治疗药水

    // ========================================================================
    // 药水相关
    // ========================================================================
    static Item* GLASS_BOTTLE;           // 玻璃瓶
    static Item* POTION;                 // 药水
    static Item* SPLASH_POTION;          // 喷溅药水
    static Item* LINGERING_POTION;       // 滞留药水

    // ========================================================================
    // 武器和弹药
    // ========================================================================
    static Item* BOW;                    // 弓
    static Item* ARROW;                  // 箭矢
    static Item* SPECTRAL_ARROW;         // 光灵箭
    static Item* TIPPED_ARROW;           // 药水箭
    static Item* CROSSBOW;               // 弩
    static Item* TRIDENT;                // 三叉戟
    static Item* SHIELD;                 // 盾牌
    static Item* FISHING_ROD;            // 钓鱼竿

    // ========================================================================
    // 桶类
    // ========================================================================
    static Item* BUCKET;                 // 空桶
    static Item* WATER_BUCKET;           // 水桶
    static Item* LAVA_BUCKET;            // 岩浆桶

    // ========================================================================
    // 海绵
    // ========================================================================
    static Item* SPONGE;                 // 海绵（干燥）
    static Item* WET_SPONGE;             // 湿海绵

private:
    static bool s_initialized;

    static void registerMaterials();
    static void registerTools();
    static void registerArmor();
    static void registerFood();
    static void registerMisc();
    static void registerDyes();
    static void registerSeeds();
    static void registerCrops();
    static void registerAquaticMaterials();
    static void registerBrewingIngredients();
    static void registerPotions();
    static void registerWeapons();
    static void registerThrowables();
    static void registerBuckets();
    static void registerSponges();
};

} // namespace mc
