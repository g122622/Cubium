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
    static Item* BRICK;          // 砖（合成材料）
    static Item* RESIN_BRICK;    // 树脂砖（合成材料）
    static Item* AMETHYST_SHARD; // 紫水晶碎片

    // ========================================================================
    // 粗矿（Raw Ore）
    // ========================================================================
    static Item* RAW_IRON;   // 粗铁 - 铁矿石掉落物
    static Item* RAW_COPPER; // 粗铜 - 铜矿石掉落物
    static Item* RAW_GOLD;   // 粗金 - 金矿石掉落物

    // ========================================================================
    // 宝石碎片
    // ========================================================================
    static Item* DIAMOND_SHARD; // 自定义：钻石碎片（暂用）
    static Item* EMERALD_SHARD; // 自定义：绿宝石碎片（暂用）

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
    // 深板岩矿物原矿
    // ========================================================================
    static Item* DEEPSLATE_COAL_ORE;
    static Item* DEEPSLATE_IRON_ORE;
    static Item* DEEPSLATE_COPPER_ORE;
    static Item* DEEPSLATE_GOLD_ORE;
    static Item* DEEPSLATE_DIAMOND_ORE;
    static Item* DEEPSLATE_LAPIS_ORE;
    static Item* DEEPSLATE_EMERALD_ORE;
    static Item* DEEPSLATE_REDSTONE_ORE;

    // ========================================================================
    // 粗矿块（Raw Ore Block）
    // ========================================================================
    static Item* RAW_IRON_BLOCK;   // 粗铁块
    static Item* RAW_COPPER_BLOCK; // 粗铜块
    static Item* RAW_GOLD_BLOCK;   // 粗金块

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
    // 工具 - 铜（MC 1.21.11 新增）
    // ========================================================================
    static Item* COPPER_PICKAXE;
    static Item* COPPER_AXE;
    static Item* COPPER_SHOVEL;
    static Item* COPPER_HOE;
    static Item* COPPER_SWORD;

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
    // 护甲 - 铜（MC 1.21.11 新增）
    // ========================================================================
    static Item* COPPER_HELMET;
    static Item* COPPER_CHESTPLATE;
    static Item* COPPER_LEGGINGS;
    static Item* COPPER_BOOTS;

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
    static Item* TURTLE_HELMET; // 海龟壳 - 在酿造材料部分已声明
    static Item* ELYTRA;        // 鞘翅

    // ========================================================================
    // 马铠 - 用于装备马提供护甲
    // ========================================================================
    static Item* LEATHER_HORSE_ARMOR;   // 皮革马铠 - +3 护甲
    static Item* COPPER_HORSE_ARMOR;    // 铜马铠 - +4 护甲
    static Item* IRON_HORSE_ARMOR;      // 铁马铠 - +5 护甲
    static Item* GOLDEN_HORSE_ARMOR;    // 金马铠 - +7 护甲
    static Item* DIAMOND_HORSE_ARMOR;   // 钻石马铠 - +11 护甲
    static Item* NETHERITE_HORSE_ARMOR; // 下界合金马铠 - +19 护甲，防火

    // ========================================================================
    // 狼铠 - 用于装备狼提供护甲，可染色、可修复
    // ========================================================================
    static Item* WOLF_ARMOR; // 狼铠 - 犰狳鳞甲材质，可染色，64点耐久

    // ========================================================================
    // 鹦鹉螺铠甲 - 用于装备鹦鹉螺类实体，不可损坏
    // ========================================================================
    static Item* COPPER_NAUTILUS_ARMOR;    // 铜鹦鹉螺铠甲 - +4 护甲
    static Item* IRON_NAUTILUS_ARMOR;      // 铁鹦鹉螺铠甲 - +5 护甲
    static Item* GOLDEN_NAUTILUS_ARMOR;    // 金鹦鹉螺铠甲 - +7 护甲
    static Item* DIAMOND_NAUTILUS_ARMOR;   // 钻石鹦鹉螺铠甲 - +11 护甲
    static Item* NETHERITE_NAUTILUS_ARMOR; // 下界合金鹦鹉螺铠甲 - +19 护甲

    // ========================================================================
    // 欢乐诡鬼装备 (Harness) - 16色，用于装备 HappyGhast 实体，无护甲值、无耐久
    // ========================================================================
    static Item* WHITE_HARNESS;
    static Item* ORANGE_HARNESS;
    static Item* MAGENTA_HARNESS;
    static Item* LIGHT_BLUE_HARNESS;
    static Item* YELLOW_HARNESS;
    static Item* LIME_HARNESS;
    static Item* PINK_HARNESS;
    static Item* GRAY_HARNESS;
    static Item* LIGHT_GRAY_HARNESS;
    static Item* CYAN_HARNESS;
    static Item* PURPLE_HARNESS;
    static Item* BLUE_HARNESS;
    static Item* BROWN_HARNESS;
    static Item* GREEN_HARNESS;
    static Item* RED_HARNESS;
    static Item* BLACK_HARNESS;

    // ========================================================================
    // 收纳袋 (Bundle) - 1 无色 + 16 色，可收纳多组物品的容器
    // ========================================================================
    static Item* BUNDLE;            // 收纳袋（无色）
    static Item* WHITE_BUNDLE;      // 白色收纳袋
    static Item* ORANGE_BUNDLE;     // 橙色收纳袋
    static Item* MAGENTA_BUNDLE;    // 品红色收纳袋
    static Item* LIGHT_BLUE_BUNDLE; // 淡蓝色收纳袋
    static Item* YELLOW_BUNDLE;     // 黄色收纳袋
    static Item* LIME_BUNDLE;       // 黄绿色收纳袋
    static Item* PINK_BUNDLE;       // 粉色收纳袋
    static Item* GRAY_BUNDLE;       // 灰色收纳袋
    static Item* LIGHT_GRAY_BUNDLE; // 淡灰色收纳袋
    static Item* CYAN_BUNDLE;       // 青色收纳袋
    static Item* PURPLE_BUNDLE;     // 紫色收纳袋
    static Item* BLUE_BUNDLE;       // 蓝色收纳袋
    static Item* BROWN_BUNDLE;      // 棕色收纳袋
    static Item* GREEN_BUNDLE;      // 绿色收纳袋
    static Item* RED_BUNDLE;        // 红色收纳袋
    static Item* BLACK_BUNDLE;      // 黑色收纳袋

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
    static Item* CAKE;
    static Item* RABBIT_STEW;
    static Item* ROTTEN_FLESH;
    static Item* SPIDER_EYE;
    static Item* SUSPICIOUS_STEW;
    static Item* SWEET_BERRIES;
    static Item* GLOW_BERRIES;
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

    // 竹木原木（1.20 竹木系列）
    static Item* BAMBOO_BLOCK;          // 竹木块
    static Item* STRIPPED_BAMBOO_BLOCK; // 去皮竹木块

    static Item* OAK_PLANKS;
    static Item* SPRUCE_PLANKS;
    static Item* BIRCH_PLANKS;
    static Item* JUNGLE_PLANKS;
    static Item* ACACIA_PLANKS;
    static Item* DARK_OAK_PLANKS;

    // 竹木木板和马赛克（1.20 竹木系列）
    static Item* BAMBOO_PLANKS; // 竹木板
    static Item* BAMBOO_MOSAIC; // 竹木马赛克

    // 其他木板变体
    static Item* CRIMSON_PLANKS;  // 绯红木板
    static Item* WARPED_PLANKS;   // 诡异木板
    static Item* MANGROVE_PLANKS; // 红树木板
    static Item* CHERRY_PLANKS;   // 樱花木板
    static Item* PALE_OAK_PLANKS; // 苍白橡木木板

    // ========================================================================
    // 木棍、骨头和碗
    // ========================================================================
    static Item* STICK;
    static Item* BONE;
    static Item* BONE_MEAL;
    static Item* BOWL;

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
    static Item* SHEARS;            // 剪刀
    static Item* BRUSH;             // 刷子（考古学工具）
    static Item* HONEYCOMB;         // 蜜脾
    static Item* BELL;              // 钟
    static Item* SUSPICIOUS_SAND;   // 可疑的沙子
    static Item* SUSPICIOUS_GRAVEL; // 可疑的沙砾
    static Item* NAME_TAG;          // 命名牌
    static Item* SADDLE;            // 鞍
    static Item* STRING;
    static Item* FEATHER;
    static Item* GUNPOWDER;
    static Item* LEATHER;
    static Item* RABBIT_HIDE; // 兔子皮 - 可合成皮革
    static Item* SLIME_BALL;
    static Item* EGG;
    static Item* BLUE_EGG;  // 蓝蛋 - 寒带鸡变种产蛋
    static Item* BROWN_EGG; // 棕蛋 - 暖色鸡变种产蛋
    static Item* SNOWBALL;  // 雪球
    static Item* COMPASS;
    static Item* RECOVERY_COMPASS; // 追溯指南针 - 指向玩家上次死亡位置
    static Item* CLOCK;
    static Item* MAP;        // 空地图
    static Item* FILLED_MAP; // 已填充地图
    static Item* PAPER;      // 纸
    static Item* FERMENTED_SPIDER_EYE;
    static Item* BLAZE_ROD;
    static Item* BLAZE_POWDER;
    static Item* ENDER_PEARL;
    static Item* ENDER_EYE;
    static Item* NETHER_STAR;
    static Item* FIRE_CHARGE;
    static Item* FIREWORK_STAR;
    static Item* FIREWORK_ROCKET;
    static Item* EXPERIENCE_BOTTLE; // 附魔之瓶

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
    static Item* TORCHFLOWER_SEEDS; // 火把花种子 - 嗅探兽食物
    static Item* PITCHER_POD;       // 瓶草荚果 - 可种植瓶草

    // ========================================================================
    // 农产品
    // ========================================================================
    static Item* WHEAT;
    static Item* HAY_BLOCK; // 干草块 - 用于喂养马属动物
    static Item* PUMPKIN;
    static Item* MELON;
    // MELON_SLICE 在食物部分声明
    // CARROT 在食物部分声明
    // POTATO 在食物部分声明
    // BEETROOT 在食物部分声明
    static Item* CACTUS;   // 仙人掌 - 方块物品
    static Item* LILY_PAD; // 睡莲 - 方块物品
    static Item* VINE;     // 藤蔓 - 方块物品
    static Item* SUGAR_CANE;
    static Item* SUGAR;
    static Item* BAMBOO; // 竹子 - 熊猫食物

    // ========================================================================
    // 下界材料
    // ========================================================================
    static Item* CRIMSON_FUNGUS; // 绯红菌 - 可用于某些合成
    static Item* WARPED_FUNGUS;  // 诡异菌 - 炽足兽食物

    // ========================================================================
    // 水域更新材料
    // ========================================================================
    static Item* TURTLE_SCUTE;     // 海龟鳞甲 - 海龟长大时掉落
    static Item* ARMADILLO_SCUTE;  // 犰狳鳞甲 - 刷犰狳获得
    static Item* HEART_OF_THE_SEA; // 海洋之心 - 宝藏物品
    static Item* NAUTILUS_SHELL;   // 鹦鹉螺壳 - 溺尸掉落/钓鱼
    static Item* PHANTOM_MEMBRANE; // 幻翼膜 - 幻翼掉落
    static Item* DRIED_KELP_BLOCK; // 干海带块 - 方块物品
    static Item* SEA_PICKLE;       // 海泡菜 - 方块物品
    static Item* KELP;             // 海带 - 水下植物方块物品
    static Item* SEAGRASS;         // 海草 - 海龟食物
    // DRIED_KELP 在食物部分声明

    // ========================================================================
    // 酿造材料
    // ========================================================================
    static Item* NETHER_WART; // 地狱疣 - 酿造基础材料
    // GOLDEN_CARROT 在食物部分声明
    static Item* GHAST_TEAR;    // 恶魂之泪 - 生命恢复药水
    static Item* RABBIT_FOOT;   // 兔子脚 - 跳跃药水
    static Item* MAGMA_CREAM;   // 岩浆膏 - 防火药水
    static Item* DRAGON_BREATH; // 龙息 - 滞留药水
    // PUFFERFISH 在食物部分声明
    // TURTLE_HELMET 在特殊护甲部分声明
    static Item* GLISTERING_MELON_SLICE; // 闪烁的西瓜片 - 瞬间治疗药水

    // ========================================================================
    // 药水相关
    // ========================================================================
    static Item* GLASS_BOTTLE;     // 玻璃瓶
    static Item* POTION;           // 药水
    static Item* SPLASH_POTION;    // 喷溅药水
    static Item* LINGERING_POTION; // 滞留药水

    // ========================================================================
    // 武器和弹药
    // ========================================================================
    static Item* BOW;            // 弓
    static Item* ARROW;          // 箭矢
    static Item* SPECTRAL_ARROW; // 光灵箭
    static Item* TIPPED_ARROW;   // 药水箭
    static Item* CROSSBOW;       // 弩
    static Item* TRIDENT;        // 三叉戟
    static Item* SHIELD;         // 盾牌
    static Item* FISHING_ROD;    // 钓鱼竿

    // ========================================================================
    // 长矛 - 按材质分层（木/石/铜/铁/金/钻石/下界合金）
    // ========================================================================
    static Item* WOODEN_SPEAR;    // 木长矛
    static Item* STONE_SPEAR;     // 石长矛
    static Item* COPPER_SPEAR;    // 铜长矛
    static Item* IRON_SPEAR;      // 铁长矛
    static Item* GOLDEN_SPEAR;    // 金长矛
    static Item* DIAMOND_SPEAR;   // 钻石长矛
    static Item* NETHERITE_SPEAR; // 下界合金长矛

    // ========================================================================
    // 骑乘控制物品
    // ========================================================================
    static Item* CARROT_ON_A_STICK;        // 胡萝卜钓竿 - 控制猪
    static Item* WARPED_FUNGUS_ON_A_STICK; // 诡异菌钓竿 - 控制炽足兽

    // ========================================================================
    // 桶类
    // ========================================================================
    static Item* BUCKET;               // 空桶
    static Item* WATER_BUCKET;         // 水桶
    static Item* LAVA_BUCKET;          // 岩浆桶
    static Item* POWDER_SNOW_BUCKET;   // 细雪桶
    static Item* COD_BUCKET;           // 鳕鱼桶
    static Item* SALMON_BUCKET;        // 鲑鱼桶
    static Item* PUFFERFISH_BUCKET;    // 河豚桶
    static Item* TROPICAL_FISH_BUCKET; // 热带鱼桶
    static Item* AXOLOTL_BUCKET;       // 美西螈桶
    static Item* MILK_BUCKET;          // 牛奶桶

    // ========================================================================
    // 书本类物品
    // ========================================================================
    static Item* BOOK;           // 书
    static Item* ENCHANTED_BOOK; // 附魔书
    static Item* WRITABLE_BOOK;  // 书与笔
    static Item* WRITTEN_BOOK;   // 成书
    static Item* KNOWLEDGE_BOOK; // 知识之书 - 右键解锁配方

    // ========================================================================
    // 海绵
    // ========================================================================
    static Item* SPONGE;     // 海绵（干燥）
    static Item* WET_SPONGE; // 湿海绵

    // ========================================================================
    // 矿车
    // ========================================================================
    static Item* MINECART;               // 普通矿车
    static Item* CHEST_MINECART;         // 箱子矿车
    static Item* FURNACE_MINECART;       // 熔炉矿车
    static Item* TNT_MINECART;           // TNT矿车
    static Item* HOPPER_MINECART;        // 漏斗矿车
    static Item* COMMAND_BLOCK_MINECART; // 命令方块矿车

    // ========================================================================
    // 船（10种木材类型）
    // ========================================================================
    static Item* OAK_BOAT;      // 橡木船
    static Item* SPRUCE_BOAT;   // 云杉木船
    static Item* BIRCH_BOAT;    // 白桦木船
    static Item* JUNGLE_BOAT;   // 丛林木船
    static Item* ACACIA_BOAT;   // 金合欢木船
    static Item* DARK_OAK_BOAT; // 深色橡木船
    static Item* MANGROVE_BOAT; // 红树木船
    static Item* CHERRY_BOAT;   // 樱花木船
    static Item* PALE_OAK_BOAT; // 苍白橡木船
    static Item* BAMBOO_RAFT;   // 竹筏

    // ========================================================================
    // 带箱子的船（10种木材类型）
    // ========================================================================
    static Item* OAK_CHEST_BOAT;      // 橡木箱子船
    static Item* SPRUCE_CHEST_BOAT;   // 云杉木箱子船
    static Item* BIRCH_CHEST_BOAT;    // 白桦木箱子船
    static Item* JUNGLE_CHEST_BOAT;   // 丛林木箱子船
    static Item* ACACIA_CHEST_BOAT;   // 金合欢木箱子船
    static Item* DARK_OAK_CHEST_BOAT; // 深色橡木箱子船
    static Item* MANGROVE_CHEST_BOAT; // 红树木箱子船
    static Item* CHERRY_CHEST_BOAT;   // 樱花木箱子船
    static Item* PALE_OAK_CHEST_BOAT; // 苍白橡木箱子船
    static Item* BAMBOO_CHEST_RAFT;   // 箱子竹筏

    // ========================================================================
    // 悬挂实体物品
    // ========================================================================
    static Item* PAINTING;   // 画作
    static Item* ITEM_FRAME; // 物品展示框
    static Item* LEAD;       // 拴绳

    // ========================================================================
    // 告示牌物品（12种木材类型）
    // ========================================================================
    static Item* OAK_SIGN;      // 橡木告示牌
    static Item* SPRUCE_SIGN;   // 云杉木告示牌
    static Item* BIRCH_SIGN;    // 白桦木告示牌
    static Item* JUNGLE_SIGN;   // 丛林木告示牌
    static Item* ACACIA_SIGN;   // 金合欢木告示牌
    static Item* DARK_OAK_SIGN; // 深色橡木告示牌
    static Item* CRIMSON_SIGN;  // 绯红告示牌
    static Item* WARPED_SIGN;   // 诡异告示牌
    static Item* MANGROVE_SIGN; // 红树木告示牌
    static Item* CHERRY_SIGN;   // 樱花木告示牌
    static Item* BAMBOO_SIGN;   // 竹木告示牌
    static Item* PALE_OAK_SIGN; // 苍白橡木告示牌

    // ========================================================================
    // 悬挂告示牌物品（12种木材类型）
    // ========================================================================
    static Item* OAK_HANGING_SIGN;      // 橡木悬挂告示牌
    static Item* SPRUCE_HANGING_SIGN;   // 云杉木悬挂告示牌
    static Item* BIRCH_HANGING_SIGN;    // 白桦木悬挂告示牌
    static Item* JUNGLE_HANGING_SIGN;   // 丛林木悬挂告示牌
    static Item* ACACIA_HANGING_SIGN;   // 金合欢木悬挂告示牌
    static Item* DARK_OAK_HANGING_SIGN; // 深色橡木悬挂告示牌
    static Item* CRIMSON_HANGING_SIGN;  // 绯红悬挂告示牌
    static Item* WARPED_HANGING_SIGN;   // 诡异悬挂告示牌
    static Item* MANGROVE_HANGING_SIGN; // 红树木悬挂告示牌
    static Item* CHERRY_HANGING_SIGN;   // 樱花木悬挂告示牌
    static Item* BAMBOO_HANGING_SIGN;   // 竹木悬挂告示牌
    static Item* PALE_OAK_HANGING_SIGN; // 苍白橡木悬挂告示牌

    // ========================================================================
    // 旗帜物品 (16色)
    // ========================================================================
    static Item* WHITE_BANNER;
    static Item* ORANGE_BANNER;
    static Item* MAGENTA_BANNER;
    static Item* LIGHT_BLUE_BANNER;
    static Item* YELLOW_BANNER;
    static Item* LIME_BANNER;
    static Item* PINK_BANNER;
    static Item* GRAY_BANNER;
    static Item* LIGHT_GRAY_BANNER;
    static Item* CYAN_BANNER;
    static Item* PURPLE_BANNER;
    static Item* BLUE_BANNER;
    static Item* BROWN_BANNER;
    static Item* GREEN_BANNER;
    static Item* RED_BANNER;
    static Item* BLACK_BANNER;

    // ========================================================================
    // 旗帜图案物品
    // ========================================================================
    static Item* FLOWER_BANNER_PATTERN;  // 花朵图案
    static Item* CREEPER_BANNER_PATTERN; // 苦力怕图案
    static Item* SKULL_BANNER_PATTERN;   // 骷髅图案
    static Item* MOJANG_BANNER_PATTERN;  // Mojang标志图案
    static Item* GLOBE_BANNER_PATTERN;   // 地球图案
    static Item* PIGLIN_BANNER_PATTERN;  // 猪灵图案

    // ========================================================================
    // 基础建筑方块
    // ========================================================================
    static Item* DIRT;        // 泥土
    static Item* GRASS_BLOCK; // 草方块
    static Item* SAND;        // 沙子
    static Item* GRAVEL;      // 沙砾
    static Item* BEDROCK;     // 基岩
    static Item* OBSIDIAN;    // 黑曜石
    static Item* NETHERRACK;  // 下界岩
    static Item* GLOWSTONE;   // 荧石
    static Item* END_STONE;   // 末地石
    static Item* ICE;         // 冰
    static Item* CLAY;        // 黏土块
    static Item* SNOW;        // 雪
    static Item* SNOW_BLOCK;  // 雪块
    static Item* TERRACOTTA;  // 陶瓦
    static Item* BRICKS;      // 砖块
    static Item* BOOKSHELF;   // 书架

    // 木质书架变体（1.21.4+）
    static Item* OAK_SHELF;      // 橡木书架
    static Item* SPRUCE_SHELF;   // 云杉木书架
    static Item* BIRCH_SHELF;    // 白桦木书架
    static Item* JUNGLE_SHELF;   // 丛林木书架
    static Item* ACACIA_SHELF;   // 金合欢木书架
    static Item* DARK_OAK_SHELF; // 深色橡木书架
    static Item* MANGROVE_SHELF; // 红树木书架
    static Item* CHERRY_SHELF;   // 樱花木书架
    static Item* PALE_OAK_SHELF; // 苍白橡木书架
    static Item* BAMBOO_SHELF;   // 竹木书架
    static Item* CRIMSON_SHELF;  // 绯红木书架
    static Item* WARPED_SHELF;   // 诡异木书架

    static Item* BONE_BLOCK;  // 骨块
    static Item* SLIME_BLOCK; // 史莱姆块
    static Item* HONEY_BLOCK; // 蜂蜜块
    static Item* RED_SAND;    // 红沙
    static Item* COBWEB;      // 蛛网
    static Item* FARMLAND;    // 耕地
    static Item* GRASS_PATH;  // 草径
    static Item* MYCELIUM;    // 菌丝
    static Item* PACKED_ICE;  // 浮冰
    static Item* BLUE_ICE;    // 蓝冰
    static Item* COARSE_DIRT; // 砂土
    static Item* PODZOL;      // 灰化土
    static Item* TORCH;       // 火把
    static Item* SOUL_TORCH;  // 灵魂火把

    // ========================================================================
    // 石头变种
    // ========================================================================
    static Item* GRANITE;           // 花岗岩
    static Item* POLISHED_GRANITE;  // 磨制花岗岩
    static Item* DIORITE;           // 闪长岩
    static Item* POLISHED_DIORITE;  // 磨制闪长岩
    static Item* ANDESITE;          // 安山岩
    static Item* POLISHED_ANDESITE; // 磨制安山岩

    // ========================================================================
    // 砂岩
    // ========================================================================
    static Item* SANDSTONE;              // 砂岩
    static Item* CHISELED_SANDSTONE;     // 錾制砂岩
    static Item* CUT_SANDSTONE;          // 切制砂岩
    static Item* SMOOTH_SANDSTONE;       // 平滑砂岩
    static Item* RED_SANDSTONE;          // 红砂岩
    static Item* CHISELED_RED_SANDSTONE; // 錾制红砂岩
    static Item* CUT_RED_SANDSTONE;      // 切制红砂岩
    static Item* SMOOTH_RED_SANDSTONE;   // 平滑红砂岩

    // ========================================================================
    // 矿物方块
    // ========================================================================
    static Item* DIAMOND_BLOCK;   // 钻石块
    static Item* COAL_BLOCK;      // 煤炭块
    static Item* GOLD_BLOCK;      // 金块
    static Item* IRON_BLOCK;      // 铁块
    static Item* LAPIS_BLOCK;     // 青金石块
    static Item* EMERALD_BLOCK;   // 绿宝石块
    static Item* REDSTONE_BLOCK;  // 红石块
    static Item* NETHERITE_BLOCK; // 下界合金块

    // ========================================================================
    // 下界方块
    // ========================================================================
    static Item* SOUL_SAND;               // 灵魂沙
    static Item* SOUL_SOIL;               // 灵魂土
    static Item* BASALT;                  // 玄武岩
    static Item* POLISHED_BASALT;         // 磨制玄武岩
    static Item* BLACKSTONE;              // 黑石
    static Item* POLISHED_BLACKSTONE;     // 磨制黑石
    static Item* GILDED_BLACKSTONE;       // 镶金黑石
    static Item* CRYING_OBSIDIAN;         // 哭泣的黑曜石
    static Item* MAGMA;                   // 岩浆块
    static Item* NETHER_WART_BLOCK;       // 地狱疣块
    static Item* WARPED_WART_BLOCK;       // 诡异疣块
    static Item* CRIMSON_STEM;            // 绯红菌柄
    static Item* WARPED_STEM;             // 诡异菌柄
    static Item* CRIMSON_NYLIUM;          // 绯红菌岩
    static Item* WARPED_NYLIUM;           // 诡异菌岩
    static Item* CRIMSON_HYPHAE;          // 绯红菌丝体
    static Item* WARPED_HYPHAE;           // 诡异菌丝体
    static Item* STRIPPED_CRIMSON_STEM;   // 去皮绯红菌柄
    static Item* STRIPPED_WARPED_STEM;    // 去皮诡异菌柄
    static Item* STRIPPED_CRIMSON_HYPHAE; // 去皮绯红菌丝体
    static Item* STRIPPED_WARPED_HYPHAE;  // 去皮诡异菌丝体
    static Item* SHROOMLIGHT;             // 荧光菇
    static Item* WEEPING_VINES;           // 垂泪藤
    static Item* TWISTING_VINES;          // 缠怨藤
    static Item* CRIMSON_ROOTS;           // 绯红菌索
    static Item* WARPED_ROOTS;            // 诡异菌索
    static Item* NETHER_SPROUTS;          // 下界苗
    static Item* DEAD_BUSH;               // 枯萎的灌木

    // ========================================================================
    // 木材和去皮原木
    // ========================================================================
    static Item* OAK_WOOD;               // 橡木
    static Item* SPRUCE_WOOD;            // 云杉木
    static Item* BIRCH_WOOD;             // 白桦木
    static Item* JUNGLE_WOOD;            // 丛林木
    static Item* ACACIA_WOOD;            // 金合欢木
    static Item* DARK_OAK_WOOD;          // 深色橡木
    static Item* STRIPPED_OAK_LOG;       // 去皮橡木原木
    static Item* STRIPPED_SPRUCE_LOG;    // 去皮云杉木原木
    static Item* STRIPPED_BIRCH_LOG;     // 去皮白桦木原木
    static Item* STRIPPED_JUNGLE_LOG;    // 去皮丛林木原木
    static Item* STRIPPED_ACACIA_LOG;    // 去皮金合欢木原木
    static Item* STRIPPED_DARK_OAK_LOG;  // 去皮深色橡木原木
    static Item* STRIPPED_OAK_WOOD;      // 去皮橡木
    static Item* STRIPPED_SPRUCE_WOOD;   // 去皮云杉木
    static Item* STRIPPED_BIRCH_WOOD;    // 去皮白桦木
    static Item* STRIPPED_JUNGLE_WOOD;   // 去皮丛林木
    static Item* STRIPPED_ACACIA_WOOD;   // 去皮金合欢木
    static Item* STRIPPED_DARK_OAK_WOOD; // 去皮深色橡木

    // ========================================================================
    // 树叶
    // ========================================================================
    static Item* OAK_LEAVES;      // 橡树树叶
    static Item* SPRUCE_LEAVES;   // 云杉树叶
    static Item* BIRCH_LEAVES;    // 白桦树叶
    static Item* JUNGLE_LEAVES;   // 丛林树叶
    static Item* ACACIA_LEAVES;   // 金合欢树叶
    static Item* DARK_OAK_LEAVES; // 深色橡树树叶

    // ========================================================================
    // 树苗
    // ========================================================================
    static Item* OAK_SAPLING;      // 橡树树苗
    static Item* SPRUCE_SAPLING;   // 云杉树苗
    static Item* BIRCH_SAPLING;    // 白桦树苗
    static Item* JUNGLE_SAPLING;   // 丛林树苗
    static Item* ACACIA_SAPLING;   // 金合欢树苗
    static Item* DARK_OAK_SAPLING; // 深色橡树树苗

    // ========================================================================
    // 植被和花
    // ========================================================================
    static Item* SHORT_GRASS;          // 矮草
    static Item* TALL_GRASS;           // 高草
    static Item* FERN;                 // 蕨
    static Item* LARGE_FERN;           // 大型蕨
    static Item* DANDELION;            // 蒲公英
    static Item* POPPY;                // 虞美人
    static Item* BLUE_ORCHID;          // 蓝花美耳草
    static Item* ALLIUM;               // 绒球葱
    static Item* AZURE_BLUET;          // 蓝花美耳草
    static Item* RED_TULIP;            // 红色郁金香
    static Item* ORANGE_TULIP;         // 橙色郁金香
    static Item* WHITE_TULIP;          // 白色郁金香
    static Item* PINK_TULIP;           // 粉色郁金香
    static Item* OXEYE_DAISY;          // 滨菊
    static Item* LILY_OF_THE_VALLEY;   // 铃兰
    static Item* SUNFLOWER;            // 向日葵
    static Item* LILAC;                // 紫丁香
    static Item* ROSE_BUSH;            // 玫瑰丛
    static Item* PEONY;                // 牡丹
    static Item* CORNFLOWER;           // 矢车菊
    static Item* WITHER_ROSE;          // 凋零玫瑰
    static Item* TORCHFLOWER;          // 火把花
    static Item* PITCHER_PLANT;        // 瓶草
    static Item* CACTUS_FLOWER;        // 仙人掌花
    static Item* WILDFLOWERS;          // 野花
    static Item* OPEN_EYEBLOSSOM;      // 开放的眼眸花
    static Item* CLOSED_EYEBLOSSOM;    // 闭合的眼眸花
    static Item* BROWN_MUSHROOM;       // 棕色蘑菇
    static Item* RED_MUSHROOM;         // 红色蘑菇
    static Item* BROWN_MUSHROOM_BLOCK; // 棕色蘑菇方块
    static Item* RED_MUSHROOM_BLOCK;   // 红色蘑菇方块
    static Item* MUSHROOM_STEM;        // 蘑菇柄

    // ========================================================================
    // 花盆
    // ========================================================================
    static Item* FLOWER_POT; // 花盆（所有 potted_* 方块共用此物品）

    // ========================================================================
    // 羊毛 (16色)
    // ========================================================================
    static Item* WHITE_WOOL;
    static Item* ORANGE_WOOL;
    static Item* MAGENTA_WOOL;
    static Item* LIGHT_BLUE_WOOL;
    static Item* YELLOW_WOOL;
    static Item* LIME_WOOL;
    static Item* PINK_WOOL;
    static Item* GRAY_WOOL;
    static Item* LIGHT_GRAY_WOOL;
    static Item* CYAN_WOOL;
    static Item* PURPLE_WOOL;
    static Item* BLUE_WOOL;
    static Item* BROWN_WOOL;
    static Item* GREEN_WOOL;
    static Item* RED_WOOL;
    static Item* BLACK_WOOL;

    // ========================================================================
    // 床 (16色)
    // ========================================================================
    static Item* WHITE_BED;
    static Item* ORANGE_BED;
    static Item* MAGENTA_BED;
    static Item* LIGHT_BLUE_BED;
    static Item* YELLOW_BED;
    static Item* LIME_BED;
    static Item* PINK_BED;
    static Item* GRAY_BED;
    static Item* LIGHT_GRAY_BED;
    static Item* CYAN_BED;
    static Item* PURPLE_BED;
    static Item* BLUE_BED;
    static Item* BROWN_BED;
    static Item* GREEN_BED;
    static Item* RED_BED;
    static Item* BLACK_BED;

    // ========================================================================
    // 地毯 (16色)
    // ========================================================================
    static Item* WHITE_CARPET;
    static Item* ORANGE_CARPET;
    static Item* MAGENTA_CARPET;
    static Item* LIGHT_BLUE_CARPET;
    static Item* YELLOW_CARPET;
    static Item* LIME_CARPET;
    static Item* PINK_CARPET;
    static Item* GRAY_CARPET;
    static Item* LIGHT_GRAY_CARPET;
    static Item* CYAN_CARPET;
    static Item* PURPLE_CARPET;
    static Item* BLUE_CARPET;
    static Item* BROWN_CARPET;
    static Item* GREEN_CARPET;
    static Item* RED_CARPET;
    static Item* BLACK_CARPET;

    // ========================================================================
    // 染色玻璃 (16色)
    // ========================================================================
    static Item* WHITE_STAINED_GLASS;
    static Item* ORANGE_STAINED_GLASS;
    static Item* MAGENTA_STAINED_GLASS;
    static Item* LIGHT_BLUE_STAINED_GLASS;
    static Item* YELLOW_STAINED_GLASS;
    static Item* LIME_STAINED_GLASS;
    static Item* PINK_STAINED_GLASS;
    static Item* GRAY_STAINED_GLASS;
    static Item* LIGHT_GRAY_STAINED_GLASS;
    static Item* CYAN_STAINED_GLASS;
    static Item* PURPLE_STAINED_GLASS;
    static Item* BLUE_STAINED_GLASS;
    static Item* BROWN_STAINED_GLASS;
    static Item* GREEN_STAINED_GLASS;
    static Item* RED_STAINED_GLASS;
    static Item* BLACK_STAINED_GLASS;

    // ========================================================================
    // 混凝土 (16色)
    // ========================================================================
    static Item* WHITE_CONCRETE;
    static Item* ORANGE_CONCRETE;
    static Item* MAGENTA_CONCRETE;
    static Item* LIGHT_BLUE_CONCRETE;
    static Item* YELLOW_CONCRETE;
    static Item* LIME_CONCRETE;
    static Item* PINK_CONCRETE;
    static Item* GRAY_CONCRETE;
    static Item* LIGHT_GRAY_CONCRETE;
    static Item* CYAN_CONCRETE;
    static Item* PURPLE_CONCRETE;
    static Item* BLUE_CONCRETE;
    static Item* BROWN_CONCRETE;
    static Item* GREEN_CONCRETE;
    static Item* RED_CONCRETE;
    static Item* BLACK_CONCRETE;

    // ========================================================================
    // 混凝土粉末 (16色)
    // ========================================================================
    static Item* WHITE_CONCRETE_POWDER;
    static Item* ORANGE_CONCRETE_POWDER;
    static Item* MAGENTA_CONCRETE_POWDER;
    static Item* LIGHT_BLUE_CONCRETE_POWDER;
    static Item* YELLOW_CONCRETE_POWDER;
    static Item* LIME_CONCRETE_POWDER;
    static Item* PINK_CONCRETE_POWDER;
    static Item* GRAY_CONCRETE_POWDER;
    static Item* LIGHT_GRAY_CONCRETE_POWDER;
    static Item* CYAN_CONCRETE_POWDER;
    static Item* PURPLE_CONCRETE_POWDER;
    static Item* BLUE_CONCRETE_POWDER;
    static Item* BROWN_CONCRETE_POWDER;
    static Item* GREEN_CONCRETE_POWDER;
    static Item* RED_CONCRETE_POWDER;
    static Item* BLACK_CONCRETE_POWDER;

    // ========================================================================
    // 陶瓦 (16色)
    // ========================================================================
    static Item* WHITE_TERRACOTTA;
    static Item* ORANGE_TERRACOTTA;
    static Item* MAGENTA_TERRACOTTA;
    static Item* LIGHT_BLUE_TERRACOTTA;
    static Item* YELLOW_TERRACOTTA;
    static Item* LIME_TERRACOTTA;
    static Item* PINK_TERRACOTTA;
    static Item* GRAY_TERRACOTTA;
    static Item* LIGHT_GRAY_TERRACOTTA;
    static Item* CYAN_TERRACOTTA;
    static Item* PURPLE_TERRACOTTA;
    static Item* BLUE_TERRACOTTA;
    static Item* BROWN_TERRACOTTA;
    static Item* GREEN_TERRACOTTA;
    static Item* RED_TERRACOTTA;
    static Item* BLACK_TERRACOTTA;

    // ========================================================================
    // 功能方块
    // ========================================================================
    static Item* CRAFTING_TABLE;   // 合成台
    static Item* CHEST;            // 箱子
    static Item* TRAPPED_CHEST;    // 陷阱箱
    static Item* BREWING_STAND;    // 酿造台
    static Item* ENCHANTING_TABLE; // 附魔台
    static Item* CAULDRON;         // 炼药锅
    static Item* ENDER_CHEST;      // 末影箱
    static Item* SHULKER_BOX;      // 潜影盒
    // 潜影盒 (16色)
    static Item* WHITE_SHULKER_BOX;
    static Item* ORANGE_SHULKER_BOX;
    static Item* MAGENTA_SHULKER_BOX;
    static Item* LIGHT_BLUE_SHULKER_BOX;
    static Item* YELLOW_SHULKER_BOX;
    static Item* LIME_SHULKER_BOX;
    static Item* PINK_SHULKER_BOX;
    static Item* GRAY_SHULKER_BOX;
    static Item* LIGHT_GRAY_SHULKER_BOX;
    static Item* CYAN_SHULKER_BOX;
    static Item* PURPLE_SHULKER_BOX;
    static Item* BLUE_SHULKER_BOX;
    static Item* BROWN_SHULKER_BOX;
    static Item* GREEN_SHULKER_BOX;
    static Item* RED_SHULKER_BOX;
    static Item* BLACK_SHULKER_BOX;
    static Item* BEACON;            // 信标
    static Item* LANTERN;           // 灯笼
    static Item* SOUL_LANTERN;      // 灵魂灯笼
    static Item* CAMPFIRE;          // 营火
    static Item* SOUL_CAMPFIRE;     // 灵魂营火
    static Item* JACK_O_LANTERN;    // 南瓜灯
    static Item* CANDLE;            // 蜡烛
    static Item* WHITE_CANDLE;      // 白色蜡烛
    static Item* ORANGE_CANDLE;     // 橙色蜡烛
    static Item* MAGENTA_CANDLE;    // 品红色蜡烛
    static Item* LIGHT_BLUE_CANDLE; // 淡蓝色蜡烛
    static Item* YELLOW_CANDLE;     // 黄色蜡烛
    static Item* LIME_CANDLE;       // 黄绿色蜡烛
    static Item* PINK_CANDLE;       // 粉色蜡烛
    static Item* GRAY_CANDLE;       // 灰色蜡烛
    static Item* LIGHT_GRAY_CANDLE; // 淡灰色蜡烛
    static Item* CYAN_CANDLE;       // 青色蜡烛
    static Item* PURPLE_CANDLE;     // 紫色蜡烛
    static Item* BLUE_CANDLE;       // 蓝色蜡烛
    static Item* BROWN_CANDLE;      // 棕色蜡烛
    static Item* GREEN_CANDLE;      // 绿色蜡烛
    static Item* RED_CANDLE;        // 红色蜡烛
    static Item* BLACK_CANDLE;      // 黑色蜡烛
    static Item* CONDUIT;           // 潮涌核心
    static Item* LOOM;              // 织布机
    static Item* BARREL;            // 木桶
    static Item* CARTOGRAPHY_TABLE; // 制图台
    static Item* FLETCHING_TABLE;   // 制箭台
    static Item* SMITHING_TABLE;    // 锻造台
    static Item* COMPOSTER;         // 堆肥桶
    static Item* LECTERN;           // 讲台
    static Item* JUKEBOX;           // 唱片机
    static Item* RESPAWN_ANCHOR;    // 重生锚

    // ========================================================================
    // 装饰/实用方块
    // ========================================================================
    static Item* LADDER;           // 梯子
    static Item* SCAFFOLDING;      // 脚手架
    static Item* CHAIN;            // 锁链
    static Item* IRON_BARS;        // 铁栏杆
    static Item* GLASS_PANE;       // 玻璃板
    static Item* CARVED_PUMPKIN;   // 雕刻过的南瓜
    static Item* END_ROD;          // 末地烛
    static Item* END_PORTAL_FRAME; // 末地传送门框架
    static Item* DRAGON_EGG;       // 龙蛋
    static Item* TURTLE_EGG;       // 海龟蛋
    static Item* CHORUS_FLOWER;    // 紫颂花

    // ========================================================================
    // 红石方块
    // ========================================================================
    // 注意：REDSTONE_WIRE 没有独立物品，红石粉物品（REDSTONE）放在地上时变成 REDSTONE_WIRE 方块
    static Item* REDSTONE_TORCH;                     // 红石火把
    static Item* REDSTONE_LAMP;                      // 红石灯
    static Item* REDSTONE_REPEATER;                  // 红石中继器
    static Item* REDSTONE_COMPARATOR;                // 红石比较器
    static Item* OBSERVER;                           // 观察者
    static Item* LEVER;                              // 拉杆
    static Item* STONE_BUTTON;                       // 石头按钮
    static Item* OAK_BUTTON;                         // 橡木按钮
    static Item* SPRUCE_BUTTON;                      // 云杉木按钮
    static Item* BIRCH_BUTTON;                       // 白桦木按钮
    static Item* JUNGLE_BUTTON;                      // 丛林木按钮
    static Item* ACACIA_BUTTON;                      // 金合欢木按钮
    static Item* DARK_OAK_BUTTON;                    // 深色橡木按钮
    static Item* CRIMSON_BUTTON;                     // 绯红木按钮
    static Item* WARPED_BUTTON;                      // 诡异木按钮
    static Item* MANGROVE_BUTTON;                    // 红树木按钮
    static Item* CHERRY_BUTTON;                      // 樱花木按钮
    static Item* BAMBOO_BUTTON;                      // 竹木按钮
    static Item* PALE_OAK_BUTTON;                    // 苍白橡木按钮
    static Item* POLISHED_BLACKSTONE_BUTTON;         // 磨制黑石按钮
    static Item* STONE_PRESSURE_PLATE;               // 石头压力板
    static Item* OAK_PRESSURE_PLATE;                 // 橡木压力板
    static Item* SPRUCE_PRESSURE_PLATE;              // 云杉木压力板
    static Item* BIRCH_PRESSURE_PLATE;               // 白桦木压力板
    static Item* JUNGLE_PRESSURE_PLATE;              // 丛林木压力板
    static Item* ACACIA_PRESSURE_PLATE;              // 金合欢木压力板
    static Item* DARK_OAK_PRESSURE_PLATE;            // 深色橡木压力板
    static Item* CRIMSON_PRESSURE_PLATE;             // 绯红木压力板
    static Item* WARPED_PRESSURE_PLATE;              // 诡异木压力板
    static Item* MANGROVE_PRESSURE_PLATE;            // 红树木压力板
    static Item* CHERRY_PRESSURE_PLATE;              // 樱花木压力板
    static Item* BAMBOO_PRESSURE_PLATE;              // 竹木压力板
    static Item* PALE_OAK_PRESSURE_PLATE;            // 苍白橡木压力板
    static Item* POLISHED_BLACKSTONE_PRESSURE_PLATE; // 磨制黑石压力板
    static Item* LIGHT_WEIGHTED_PRESSURE_PLATE;      // 轻质测重压力板
    static Item* HEAVY_WEIGHTED_PRESSURE_PLATE;      // 重质测重压力板
    static Item* DAYLIGHT_DETECTOR;                  // 阳光探测器
    static Item* PISTON;                             // 活塞
    static Item* STICKY_PISTON;                      // 粘性活塞
    static Item* DISPENSER;                          // 发射器
    static Item* DROPPER;                            // 投掷器
    static Item* NOTE_BLOCK;                         // 音符盒
    static Item* TNT;                                // TNT
    static Item* TARGET;                             // 标靶
    static Item* TRIPWIRE_HOOK;                      // 绊线钩

    // ========================================================================
    // 铁轨
    // ========================================================================
    static Item* RAIL;           // 铁轨
    static Item* POWERED_RAIL;   // 充能铁轨
    static Item* DETECTOR_RAIL;  // 探测铁轨
    static Item* ACTIVATOR_RAIL; // 激活铁轨

    // ========================================================================
    // 门、栅栏、活板门
    // ========================================================================
    static Item* OAK_DOOR;      // 橡木门
    static Item* SPRUCE_DOOR;   // 云杉木门
    static Item* BIRCH_DOOR;    // 白桦木门
    static Item* JUNGLE_DOOR;   // 丛林木门
    static Item* ACACIA_DOOR;   // 金合欢木门
    static Item* DARK_OAK_DOOR; // 深色橡木门
    static Item* MANGROVE_DOOR; // 红树木门
    static Item* CHERRY_DOOR;   // 樱花木门
    static Item* PALE_OAK_DOOR; // 苍白橡木门
    static Item* BAMBOO_DOOR;   // 竹木门
    static Item* CRIMSON_DOOR;  // 绯红木门
    static Item* WARPED_DOOR;   // 诡异木门
    static Item* IRON_DOOR;     // 铁门
    // 铜门（8 种氧化/涂蜡变种）
    static Item* COPPER_DOOR;                 // 铜门
    static Item* EXPOSED_COPPER_DOOR;         // 斑驳铜门
    static Item* WEATHERED_COPPER_DOOR;       // 锈蚀铜门
    static Item* OXIDIZED_COPPER_DOOR;        // 氧化铜门
    static Item* WAXED_COPPER_DOOR;           // 涂蜡铜门
    static Item* WAXED_EXPOSED_COPPER_DOOR;   // 涂蜡斑驳铜门
    static Item* WAXED_WEATHERED_COPPER_DOOR; // 涂蜡锈蚀铜门
    static Item* WAXED_OXIDIZED_COPPER_DOOR;  // 涂蜡氧化铜门
    static Item* OAK_FENCE;                   // 橡木栅栏
    static Item* SPRUCE_FENCE;                // 云杉木栅栏
    static Item* BIRCH_FENCE;                 // 白桦木栅栏
    static Item* JUNGLE_FENCE;                // 丛林木栅栏
    static Item* ACACIA_FENCE;                // 金合欢木栅栏
    static Item* DARK_OAK_FENCE;              // 深色橡木栅栏
    static Item* MANGROVE_FENCE;              // 红树木栅栏
    static Item* CHERRY_FENCE;                // 樱花木栅栏
    static Item* PALE_OAK_FENCE;              // 苍白橡木栅栏
    static Item* BAMBOO_FENCE;                // 竹木栅栏
    static Item* NETHER_BRICK_FENCE;          // 下界砖栅栏
    static Item* OAK_FENCE_GATE;              // 橡木栅栏门
    static Item* SPRUCE_FENCE_GATE;           // 云杉木栅栏门
    static Item* BIRCH_FENCE_GATE;            // 白桦木栅栏门
    static Item* JUNGLE_FENCE_GATE;           // 丛林木栅栏门
    static Item* ACACIA_FENCE_GATE;           // 金合欢木栅栏门
    static Item* DARK_OAK_FENCE_GATE;         // 深色橡木栅栏门
    static Item* MANGROVE_FENCE_GATE;         // 红树木栅栏门
    static Item* CHERRY_FENCE_GATE;           // 樱花木栅栏门
    static Item* PALE_OAK_FENCE_GATE;         // 苍白橡木栅栏门
    static Item* BAMBOO_FENCE_GATE;           // 竹木栅栏门
    static Item* OAK_TRAPDOOR;                // 橡木活板门
    static Item* SPRUCE_TRAPDOOR;             // 云杉木活板门
    static Item* BIRCH_TRAPDOOR;              // 白桦木活板门
    static Item* JUNGLE_TRAPDOOR;             // 丛林木活板门
    static Item* ACACIA_TRAPDOOR;             // 金合欢木活板门
    static Item* DARK_OAK_TRAPDOOR;           // 深色橡木活板门
    static Item* MANGROVE_TRAPDOOR;           // 红树木活板门
    static Item* CHERRY_TRAPDOOR;             // 樱花木活板门
    static Item* PALE_OAK_TRAPDOOR;           // 苍白橡木活板门
    static Item* BAMBOO_TRAPDOOR;             // 竹木活板门
    static Item* CRIMSON_TRAPDOOR;            // 绯红木活板门
    static Item* WARPED_TRAPDOOR;             // 诡异木活板门
    static Item* IRON_TRAPDOOR;               // 铁活板门
    // 铜活板门（8 种氧化/涂蜡变种）
    static Item* COPPER_TRAPDOOR;                 // 铜活板门
    static Item* EXPOSED_COPPER_TRAPDOOR;         // 斑驳铜活板门
    static Item* WEATHERED_COPPER_TRAPDOOR;       // 锈蚀铜活板门
    static Item* OXIDIZED_COPPER_TRAPDOOR;        // 氧化铜活板门
    static Item* WAXED_COPPER_TRAPDOOR;           // 涂蜡铜活板门
    static Item* WAXED_EXPOSED_COPPER_TRAPDOOR;   // 涂蜡斑驳铜活板门
    static Item* WAXED_WEATHERED_COPPER_TRAPDOOR; // 涂蜡锈蚀铜活板门
    static Item* WAXED_OXIDIZED_COPPER_TRAPDOOR;  // 涂蜡氧化铜活板门

    // ========================================================================
    // 楼梯、台阶、墙
    // ========================================================================
    static Item* OAK_STAIRS;               // 橡木楼梯
    static Item* SPRUCE_STAIRS;            // 云杉木楼梯
    static Item* BIRCH_STAIRS;             // 白桦木楼梯
    static Item* JUNGLE_STAIRS;            // 丛林木楼梯
    static Item* ACACIA_STAIRS;            // 金合欢木楼梯
    static Item* DARK_OAK_STAIRS;          // 深色橡木楼梯
    static Item* STONE_STAIRS;             // 石头楼梯
    static Item* COBBLESTONE_STAIRS;       // 圆石楼梯
    static Item* SANDSTONE_STAIRS;         // 砂岩楼梯
    static Item* SMOOTH_SANDSTONE_STAIRS;  // 平滑砂岩楼梯
    static Item* STONE_BRICK_STAIRS;       // 石砖楼梯
    static Item* MOSSY_STONE_BRICK_STAIRS; // 苔石砖楼梯
    static Item* OAK_SLAB;                 // 橡木台阶
    static Item* SPRUCE_SLAB;              // 云杉木台阶
    static Item* BIRCH_SLAB;               // 白桦木台阶
    static Item* JUNGLE_SLAB;              // 丛林木台阶
    static Item* ACACIA_SLAB;              // 金合欢木台阶
    static Item* DARK_OAK_SLAB;            // 深色橡木台阶
    static Item* STONE_SLAB;               // 石头台阶
    static Item* COBBLESTONE_SLAB;         // 圆石台阶
    static Item* SANDSTONE_SLAB;           // 砂岩台阶
    static Item* SMOOTH_SANDSTONE_SLAB;    // 平滑砂岩台阶
    static Item* STONE_BRICK_SLAB;         // 石砖台阶
    static Item* MOSSY_STONE_BRICK_SLAB;   // 苔石砖台阶
    static Item* COBBLESTONE_WALL;         // 圆石墙
    static Item* STONE_BRICK_WALL;         // 石砖墙
    static Item* MOSSY_STONE_BRICK_WALL;   // 苔石砖墙

    // ========================================================================
    // 末地方块
    // ========================================================================
    static Item* END_STONE_BRICKS; // 末地石砖
    static Item* PURPUR_BLOCK;     // 紫珀块
    static Item* PURPUR_PILLAR;    // 紫珀柱

    // ========================================================================
    // 海晶方块
    // ========================================================================
    static Item* PRISMARINE;              // 海晶石
    static Item* PRISMARINE_BRICKS;       // 海晶砖
    static Item* DARK_PRISMARINE;         // 暗海晶石
    static Item* PRISMARINE_STAIRS;       // 海晶石楼梯
    static Item* PRISMARINE_BRICK_STAIRS; // 海晶砖楼梯
    static Item* DARK_PRISMARINE_STAIRS;  // 暗海晶石楼梯
    static Item* PRISMARINE_SLAB;         // 海晶石台阶
    static Item* PRISMARINE_BRICK_SLAB;   // 海晶砖台阶
    static Item* DARK_PRISMARINE_SLAB;    // 暗海晶石台阶
    static Item* SEA_LANTERN;             // 海晶灯

    // ========================================================================
    // 石砖系列
    // ========================================================================
    static Item* STONE_BRICKS;          // 石砖
    static Item* MOSSY_STONE_BRICKS;    // 苔石砖
    static Item* CRACKED_STONE_BRICKS;  // 裂纹石砖
    static Item* CHISELED_STONE_BRICKS; // 錾制石砖

    // ========================================================================
    // 虫蚀方块
    // ========================================================================
    static Item* INFESTED_STONE;                 // 虫蚀石头
    static Item* INFESTED_COBBLESTONE;           // 虫蚀圆石
    static Item* INFESTED_STONE_BRICKS;          // 虫蚀石砖
    static Item* INFESTED_MOSSY_STONE_BRICKS;    // 虫蚀苔石砖
    static Item* INFESTED_CRACKED_STONE_BRICKS;  // 虫蚀裂纹石砖
    static Item* INFESTED_CHISELED_STONE_BRICKS; // 虫蚀錾制石砖

    // ========================================================================
    // 石英系列
    // ========================================================================
    static Item* QUARTZ_BLOCK;          // 石英块
    static Item* CHISELED_QUARTZ_BLOCK; // 錾制石英块
    static Item* QUARTZ_PILLAR;         // 石英柱

    // ========================================================================
    // 珊瑚方块 - 活
    // ========================================================================
    static Item* TUBE_CORAL_BLOCK;   // 管珊瑚方块
    static Item* BRAIN_CORAL_BLOCK;  // 脑珊瑚方块
    static Item* BUBBLE_CORAL_BLOCK; // 气泡珊瑚方块
    static Item* FIRE_CORAL_BLOCK;   // 火珊瑚方块
    static Item* HORN_CORAL_BLOCK;   // 角珊瑚方块

    // ========================================================================
    // 珊瑚方块 - 死
    // ========================================================================
    static Item* DEAD_TUBE_CORAL_BLOCK;   // 死管珊瑚方块
    static Item* DEAD_BRAIN_CORAL_BLOCK;  // 死脑珊瑚方块
    static Item* DEAD_BUBBLE_CORAL_BLOCK; // 死气泡珊瑚方块
    static Item* DEAD_FIRE_CORAL_BLOCK;   // 死火珊瑚方块
    static Item* DEAD_HORN_CORAL_BLOCK;   // 死角珊瑚方块

    // ========================================================================
    // 珊瑚扇 - 活
    // ========================================================================
    static Item* TUBE_CORAL_FAN;   // 管珊瑚扇
    static Item* BRAIN_CORAL_FAN;  // 脑珊瑚扇
    static Item* BUBBLE_CORAL_FAN; // 气泡珊瑚扇
    static Item* FIRE_CORAL_FAN;   // 火珊瑚扇
    static Item* HORN_CORAL_FAN;   // 角珊瑚扇

    // ========================================================================
    // 珊瑚扇 - 死
    // ========================================================================
    static Item* DEAD_TUBE_CORAL_FAN;   // 死管珊瑚扇
    static Item* DEAD_BRAIN_CORAL_FAN;  // 死脑珊瑚扇
    static Item* DEAD_BUBBLE_CORAL_FAN; // 死气泡珊瑚扇
    static Item* DEAD_FIRE_CORAL_FAN;   // 死火珊瑚扇
    static Item* DEAD_HORN_CORAL_FAN;   // 死角珊瑚扇

    // ========================================================================
    // 试炼密室 (Trial Chambers)
    // ========================================================================
    static Item* TRIAL_KEY;             // 试炼钥匙
    static Item* OMINOUS_TRIAL_KEY;     // 不祥试炼钥匙
    static Item* OMINOUS_BOTTLE;        // 不祥之瓶
    static Item* WIND_CHARGE;           // 风弹
    static Item* BREEZE_ROD;            // 狂风杖
    static Item* MACE;                  // 重锤
    static Item* GUSTER_BANNER_PATTERN; // 旋风旗帜图案
    static Item* FLOW_BANNER_PATTERN;   // 涡流旗帜图案

    // ========================================================================
    // 锻造模板物品（盔甲纹饰 + 下界合金升级）
    // ========================================================================
    static Item* NETHERITE_UPGRADE_SMITHING_TEMPLATE;    // 下界合金升级锻造模板
    static Item* SENTRY_ARMOR_TRIM_SMITHING_TEMPLATE;    // 哨兵盔甲纹饰
    static Item* VEX_ARMOR_TRIM_SMITHING_TEMPLATE;       // 恼鬼盔甲纹饰
    static Item* WILD_ARMOR_TRIM_SMITHING_TEMPLATE;      // 荒野盔甲纹饰
    static Item* COAST_ARMOR_TRIM_SMITHING_TEMPLATE;     // 海岸盔甲纹饰
    static Item* DUNE_ARMOR_TRIM_SMITHING_TEMPLATE;      // 沙丘盔甲纹饰
    static Item* WAYFINDER_ARMOR_TRIM_SMITHING_TEMPLATE; // 寻路者盔甲纹饰
    static Item* RAISER_ARMOR_TRIM_SMITHING_TEMPLATE;    // 升起者盔甲纹饰
    static Item* SHAPER_ARMOR_TRIM_SMITHING_TEMPLATE;    // 塑造者盔甲纹饰
    static Item* HOST_ARMOR_TRIM_SMITHING_TEMPLATE;      // 宿主盔甲纹饰
    static Item* WARD_ARMOR_TRIM_SMITHING_TEMPLATE;      // 监守者盔甲纹饰
    static Item* SILENCE_ARMOR_TRIM_SMITHING_TEMPLATE;   // 寂静盔甲纹饰
    static Item* TIDE_ARMOR_TRIM_SMITHING_TEMPLATE;      // 潮汐盔甲纹饰
    static Item* SNOUT_ARMOR_TRIM_SMITHING_TEMPLATE;     // 猪鼻盔甲纹饰
    static Item* RIB_ARMOR_TRIM_SMITHING_TEMPLATE;       // 镶铆盔甲纹饰
    static Item* EYE_ARMOR_TRIM_SMITHING_TEMPLATE;       // 眼睛盔甲纹饰
    static Item* SPIRE_ARMOR_TRIM_SMITHING_TEMPLATE;     // 尖塔盔甲纹饰
    static Item* FLOW_ARMOR_TRIM_SMITHING_TEMPLATE;      // 涡流盔甲纹饰
    static Item* BOLT_ARMOR_TRIM_SMITHING_TEMPLATE;      // 闪电盔甲纹饰

    // ========================================================================
    // 陶片物品（1.20 考古学 + 1.21 试炼密室）
    // ========================================================================
    static Item* ANGLER_POTTERY_SHERD;     // 钓鱼者陶片
    static Item* ARCHER_POTTERY_SHERD;     // 射手陶片
    static Item* ARMS_UP_POTTERY_SHERD;    // 举手陶片
    static Item* BLADE_POTTERY_SHERD;      // 刀刃陶片
    static Item* BREWER_POTTERY_SHERD;     // 酿造者陶片
    static Item* BURN_POTTERY_SHERD;       // 燃烧陶片
    static Item* DANGER_POTTERY_SHERD;     // 危险陶片
    static Item* EXPLORER_POTTERY_SHERD;   // 探险者陶片
    static Item* FRIEND_POTTERY_SHERD;     // 朋友陶片
    static Item* HEART_POTTERY_SHERD;      // 心陶片
    static Item* HEARTBREAK_POTTERY_SHERD; // 碎心陶片
    static Item* HOWL_POTTERY_SHERD;       // 嚎叫陶片
    static Item* MINER_POTTERY_SHERD;      // 矿工陶片
    static Item* MOURNER_POTTERY_SHERD;    // 哀悼者陶片
    static Item* PLENTY_POTTERY_SHERD;     // 丰饶陶片
    static Item* PRIZE_POTTERY_SHERD;      // 奖赏陶片
    static Item* SHEAF_POTTERY_SHERD;      // 麦捆陶片
    static Item* SHELTER_POTTERY_SHERD;    // 庇护所陶片
    static Item* SKULL_POTTERY_SHERD;      // 骷髅陶片
    static Item* SNORT_POTTERY_SHERD;      // 喷鼻陶片
    static Item* FLOW_POTTERY_SHERD;       // 涡流陶片
    static Item* GUSTER_POTTERY_SHERD;     // 旋风陶片
    static Item* SCRAPE_POTTERY_SHERD;     // 刮削陶片

    // ========================================================================
    // 音乐唱片
    // ========================================================================
    static Item* MUSIC_DISC_13;                // 音乐唱片 - 13
    static Item* MUSIC_DISC_CAT;               // 音乐唱片 - cat
    static Item* MUSIC_DISC_BLOCKS;            // 音乐唱片 - blocks
    static Item* MUSIC_DISC_CHIRP;             // 音乐唱片 - chirp
    static Item* MUSIC_DISC_FAR;               // 音乐唱片 - far
    static Item* MUSIC_DISC_MALL;              // 音乐唱片 - mall
    static Item* MUSIC_DISC_MELLOHI;           // 音乐唱片 - mellohi
    static Item* MUSIC_DISC_STAL;              // 音乐唱片 - stal
    static Item* MUSIC_DISC_STRAD;             // 音乐唱片 - strad
    static Item* MUSIC_DISC_WARD;              // 音乐唱片 - ward
    static Item* MUSIC_DISC_11;                // 音乐唱片 - 11
    static Item* MUSIC_DISC_WAIT;              // 音乐唱片 - wait
    static Item* MUSIC_DISC_PIGSTEP;           // 音乐唱片 - Pigstep
    static Item* MUSIC_DISC_OTHERSIDE;         // 音乐唱片 - otherside
    static Item* MUSIC_DISC_5;                 // 音乐唱片 - 5
    static Item* MUSIC_DISC_RELIC;             // 音乐唱片 - relic
    static Item* MUSIC_DISC_TEARS;             // 音乐唱片 - Tears
    static Item* MUSIC_DISC_CREATOR;           // 音乐唱片 - Creator
    static Item* MUSIC_DISC_CREATOR_MUSIC_BOX; // 音乐唱片 - Creator (八音盒)
    static Item* MUSIC_DISC_PRECIPICE;         // 音乐唱片 - Precipice
    static Item* MUSIC_DISC_LAVA_CHICKEN;      // 音乐唱片 - Lava Chicken

    // ========================================================================
    // 头颅物品
    // ========================================================================
    static Item* SKELETON_SKULL;        // 骷髅头颅
    static Item* WITHER_SKELETON_SKULL; // 凋灵骷髅头颅
    static Item* PLAYER_HEAD;           // 玩家头颅
    static Item* ZOMBIE_HEAD;           // 僵尸头
    static Item* CREEPER_HEAD;          // 苦力怕头
    static Item* DRAGON_HEAD;           // 龙首
    static Item* PIGLIN_HEAD;           // 猪灵头

    // ========================================================================
    // 刷怪蛋物品
    //
    // 对应 MC 1.21.11 的 SpawnEggItem，共 87 种刷怪蛋（含 Allay/Camel_Husk/Creaking/
    // Happy_Ghast/Nautilus/Parched 等 1.21 新增实体）。颜色数据来源：
    //   - 历史型刷怪蛋（1.16.5 之前）：MC Java 中 SpawnEggItem 内置的 background/
    //     foreground（ARGB）常量，该数据在各版本间保持稳定。
    //   - 新增实体刷怪蛋（1.17+ 实验性/未实现实体）：从原版资源包纹理
    //     (assets/minecraft/textures/item/xxx_spawn_egg.png) 提取的主色/次色。
    // MC 1.21.11 已将颜色从 Java 代码迁移至客户端纹理，本项目中颜色仅作为 API
    // 字段保留（SpawnEggItem::getPrimaryColor/getSecondaryColor），不参与服务端逻辑。
    // 物品注册名为 minecraft:xxx_spawn_egg。
    // 使用方式：
    //   - 右键方块：SpawnEggItem::onItemUse 在方块面上方生成对应实体
    //   - 右键玩家对应生物（如手持猪刷怪蛋右键猪）：MobEntity::processInitialInteract
    //     走 _spawnOffspringFromSpawnEgg 路径，生成幼体（仅 AgeableEntity 子类）
    // ========================================================================
    static Item* ALLAY_SPAWN_EGG;            // 悦灵刷怪蛋
    static Item* ARMADILLO_SPAWN_EGG;        // 犰狳刷怪蛋
    static Item* AXOLOTL_SPAWN_EGG;          // 美西螈刷怪蛋
    static Item* BAT_SPAWN_EGG;              // 蝙蝠刷怪蛋
    static Item* BEE_SPAWN_EGG;              // 蜜蜂刷怪蛋
    static Item* BLAZE_SPAWN_EGG;            // 烈焰人刷怪蛋
    static Item* BOGGED_SPAWN_EGG;           // 沼骸刷怪蛋
    static Item* BREEZE_SPAWN_EGG;           // 旋风刷怪蛋
    static Item* CAMEL_HUSK_SPAWN_EGG;       // 骆驼尸壳刷怪蛋
    static Item* CAMEL_SPAWN_EGG;            // 骆驼刷怪蛋
    static Item* CAT_SPAWN_EGG;              // 猫刷怪蛋
    static Item* CAVE_SPIDER_SPAWN_EGG;      // 洞穴蜘蛛刷怪蛋
    static Item* CHICKEN_SPAWN_EGG;          // 鸡刷怪蛋
    static Item* COD_SPAWN_EGG;              // 鳕鱼刷怪蛋
    static Item* COPPER_GOLEM_SPAWN_EGG;     // 铜傀儡刷怪蛋
    static Item* COW_SPAWN_EGG;              // 牛刷怪蛋
    static Item* CREAKING_SPAWN_EGG;         // 嘎吱刷怪蛋
    static Item* CREEPER_SPAWN_EGG;          // 苦力怕刷怪蛋
    static Item* DOLPHIN_SPAWN_EGG;          // 海豚刷怪蛋
    static Item* DONKEY_SPAWN_EGG;           // 驴刷怪蛋
    static Item* DROWNED_SPAWN_EGG;          // 溺尸刷怪蛋
    static Item* ELDER_GUARDIAN_SPAWN_EGG;   // 远古守卫者刷怪蛋
    static Item* ENDER_DRAGON_SPAWN_EGG;     // 末影龙刷怪蛋
    static Item* ENDERMAN_SPAWN_EGG;         // 末影人刷怪蛋
    static Item* ENDERMITE_SPAWN_EGG;        // 末影螨刷怪蛋
    static Item* EVOKER_SPAWN_EGG;           // 唤魔者刷怪蛋
    static Item* FOX_SPAWN_EGG;              // 狐狸刷怪蛋
    static Item* FROG_SPAWN_EGG;             // 青蛙刷怪蛋
    static Item* GHAST_SPAWN_EGG;            // 恶魂刷怪蛋
    static Item* GLOW_SQUID_SPAWN_EGG;       // 发光鱿鱼刷怪蛋
    static Item* GOAT_SPAWN_EGG;             // 山羊刷怪蛋
    static Item* GUARDIAN_SPAWN_EGG;         // 守卫者刷怪蛋
    static Item* HAPPY_GHAST_SPAWN_EGG;      // 欢乐恶魂刷怪蛋
    static Item* HOGLIN_SPAWN_EGG;           // 疣猪兽刷怪蛋
    static Item* HORSE_SPAWN_EGG;            // 马刷怪蛋
    static Item* HUSK_SPAWN_EGG;             // 尸壳刷怪蛋
    static Item* IRON_GOLEM_SPAWN_EGG;       // 铁傀儡刷怪蛋
    static Item* LLAMA_SPAWN_EGG;            // 羊驼刷怪蛋
    static Item* MAGMA_CUBE_SPAWN_EGG;       // 岩浆怪刷怪蛋
    static Item* MOOSHROOM_SPAWN_EGG;        // 哞菇刷怪蛋
    static Item* MULE_SPAWN_EGG;             // 骡刷怪蛋
    static Item* NAUTILUS_SPAWN_EGG;         // 鹦鹉螺刷怪蛋
    static Item* OCELOT_SPAWN_EGG;           // 豹猫刷怪蛋
    static Item* PANDA_SPAWN_EGG;            // 熊猫刷怪蛋
    static Item* PARROT_SPAWN_EGG;           // 鹦鹉刷怪蛋
    static Item* PARCHED_SPAWN_EGG;          // 干涸者刷怪蛋
    static Item* PHANTOM_SPAWN_EGG;          // 幻翼刷怪蛋
    static Item* PIG_SPAWN_EGG;              // 猪刷怪蛋
    static Item* PIGLIN_SPAWN_EGG;           // 猪灵刷怪蛋
    static Item* PIGLIN_BRUTE_SPAWN_EGG;     // 猪灵蛮兵刷怪蛋
    static Item* PILLAGER_SPAWN_EGG;         // 掠夺者刷怪蛋
    static Item* POLAR_BEAR_SPAWN_EGG;       // 北极熊刷怪蛋
    static Item* PUFFERFISH_SPAWN_EGG;       // 河豚刷怪蛋
    static Item* RABBIT_SPAWN_EGG;           // 兔子刷怪蛋
    static Item* RAVAGER_SPAWN_EGG;          // 劫掠兽刷怪蛋
    static Item* SALMON_SPAWN_EGG;           // 鲑鱼刷怪蛋
    static Item* SHEEP_SPAWN_EGG;            // 羊刷怪蛋
    static Item* SHULKER_SPAWN_EGG;          // 潜影贝刷怪蛋
    static Item* SILVERFISH_SPAWN_EGG;       // 蠹虫刷怪蛋
    static Item* SKELETON_SPAWN_EGG;         // 骷髅刷怪蛋
    static Item* SKELETON_HORSE_SPAWN_EGG;   // 骷髅马刷怪蛋
    static Item* SLIME_SPAWN_EGG;            // 史莱姆刷怪蛋
    static Item* SNIFFER_SPAWN_EGG;          // 嗅探兽刷怪蛋
    static Item* SNOW_GOLEM_SPAWN_EGG;       // 雪傀儡刷怪蛋
    static Item* SPIDER_SPAWN_EGG;           // 蜘蛛刷怪蛋
    static Item* SQUID_SPAWN_EGG;            // 鱿鱼刷怪蛋
    static Item* STRAY_SPAWN_EGG;            // 流髑刷怪蛋
    static Item* STRIDER_SPAWN_EGG;          // 炽足兽刷怪蛋
    static Item* TADPOLE_SPAWN_EGG;          // 蝌蚪刷怪蛋
    static Item* TRADER_LLAMA_SPAWN_EGG;     // 行商羊驼刷怪蛋
    static Item* TROPICAL_FISH_SPAWN_EGG;    // 热带鱼刷怪蛋
    static Item* TURTLE_SPAWN_EGG;           // 海龟刷怪蛋
    static Item* VEX_SPAWN_EGG;              // 恼鬼刷怪蛋
    static Item* VILLAGER_SPAWN_EGG;         // 村民刷怪蛋
    static Item* VINDICATOR_SPAWN_EGG;       // 卫道士刷怪蛋
    static Item* WANDERING_TRADER_SPAWN_EGG; // 流浪商人刷怪蛋
    static Item* WARDEN_SPAWN_EGG;           // 监守者刷怪蛋
    static Item* WITCH_SPAWN_EGG;            // 女巫刷怪蛋
    static Item* WITHER_SPAWN_EGG;           // 凋灵刷怪蛋
    static Item* WITHER_SKELETON_SPAWN_EGG;  // 凋灵骷髅刷怪蛋
    static Item* WOLF_SPAWN_EGG;             // 狼刷怪蛋
    static Item* ZOGLIN_SPAWN_EGG;           // 僵尸疣猪兽刷怪蛋
    static Item* ZOMBIE_SPAWN_EGG;           // 僵尸刷怪蛋
    static Item* ZOMBIE_HORSE_SPAWN_EGG;     // 僵尸马刷怪蛋
    static Item* ZOMBIE_NAUTILUS_SPAWN_EGG;  // 僵尸鹦鹉螺刷怪蛋
    static Item* ZOMBIFIED_PIGLIN_SPAWN_EGG; // 僵尸猪灵刷怪蛋
    static Item* ZOMBIE_VILLAGER_SPAWN_EGG;  // 僵尸村民刷怪蛋

private:
    static bool s_initialized;

    static void _registerMaterials();
    static void _registerTools();
    static void _registerArmor();
    static void _registerFood();
    static void _registerMisc();
    static void _registerDyes();
    static void _registerSeeds();
    static void _registerCrops();
    static void _registerAquaticMaterials();
    static void _registerBrewingIngredients();
    static void _registerPotions();
    static void _registerWeapons();
    static void _registerThrowables();
    static void _registerBuckets();
    static void _registerBooks();
    static void _registerSponges();
    static void _registerMinecarts();
    static void _registerBoats();
    static void _registerChestBoats();
    static void _registerHangingItems();
    static void _registerSigns();
    static void _registerBanners();
    static void _registerBuildingBlocks();
    static void _registerBeds();
    static void _registerShulkerBoxes();
    static void _registerWool();
    static void _registerCarpets();
    static void _registerStainedGlass();
    static void _registerConcrete();
    static void _registerTerracotta();
    static void _registerVegetation();
    static void _registerRedstone();
    static void _registerCoral();
    static void _registerDoorsFencesStairs();
    static void _registerTrialChamberItems();
    static void _registerMusicDiscs();
    static void _registerSkulls();
    static void _registerHarnesses();
    static void _registerBundles();
    static void _registerSpawnEggs();
};

} // namespace mc
