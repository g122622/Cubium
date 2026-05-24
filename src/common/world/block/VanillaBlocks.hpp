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

#include "Block.hpp"
#include "BlockRegistry.hpp"
#include "blocks/AirBlock.hpp"
#include "blocks/RotatedPillarBlock.hpp"
#include "blocks/SignBlock.hpp"
#include "blocks/SimpleBlock.hpp"

namespace mc {

/**
 * @brief 原版方块静态引用
 *
 * 提供所有原版方块的静态指针，便于快速访问。
 * 在游戏初始化时调用 VanillaBlocks::initialize() 进行注册。
 *
 * 参考: net.minecraft.block.Blocks
 */
class VanillaBlocks {
public:
    /**
     * @brief 初始化所有原版方块
     *
     * 必须在使用任何方块前调用。
     */
    static void initialize();

    // ========================================================================
    // 基础方块
    // ========================================================================
    static Block* AIR;
    static Block* CAVE_AIR; // 洞穴空气 - 用于洞穴生成
    static Block* VOID_AIR; // 虚空空气 - 用于世界边界外
    static Block* STONE;
    static Block* GRASS_BLOCK;
    static Block* DIRT;
    static Block* COBBLESTONE;
    static Block* OAK_PLANKS;
    static Block* WATER;
    static Block* LAVA;
    static Block* BEDROCK;
    static Block* SAND;
    static Block* GRAVEL;

    // ========================================================================
    // 石头变种
    // ========================================================================
    static Block* GRANITE;
    static Block* POLISHED_GRANITE;
    static Block* DIORITE;
    static Block* POLISHED_DIORITE;
    static Block* ANDESITE;
    static Block* POLISHED_ANDESITE;

    // ========================================================================
    // 泥土变种
    // ========================================================================
    static Block* COARSE_DIRT;
    static Block* PODZOL;

    // ========================================================================
    // 砂岩系列
    // ========================================================================
    static Block* SANDSTONE;
    static Block* CHISELED_SANDSTONE;
    static Block* CUT_SANDSTONE;
    static Block* RED_SANDSTONE;

    // ========================================================================
    // 矿石方块
    // ========================================================================
    static Block* GOLD_ORE;
    static Block* IRON_ORE;
    static Block* COAL_ORE;
    static Block* DIAMOND_ORE;
    static Block* DIAMOND_BLOCK;
    static Block* EMERALD_ORE;
    static Block* LAPIS_ORE;
    static Block* REDSTONE_ORE;
    static Block* COPPER_ORE; // 铜矿 (1.17+)

    // ========================================================================
    // 下界矿石
    // ========================================================================
    static Block* NETHER_QUARTZ_ORE; // 下界石英矿
    static Block* NETHER_GOLD_ORE;   // 下界金矿
    static Block* ANCIENT_DEBRIS;    // 远古残骸

    // ========================================================================
    // 矿物方块
    // ========================================================================
    static Block* COAL_BLOCK; // 煤炭块
    static Block* GOLD_BLOCK;
    static Block* IRON_BLOCK;
    static Block* LAPIS_BLOCK;
    static Block* EMERALD_BLOCK;
    static Block* REDSTONE_BLOCK;
    static Block* NETHERITE_BLOCK; // 下界合金块

    // ========================================================================
    // 建筑方块
    // ========================================================================
    static Block* BRICKS;
    static Block* MOSSY_COBBLESTONE;
    static Block* BOOKSHELF;
    static Block* TNT;
    static Block* SPONGE;
    static Block* WET_SPONGE;

    // ========================================================================
    // 功能方块
    // ========================================================================
    static Block* CRAFTING_TABLE;
    static Block* CAULDRON;
    static Block* ENCHANTING_TABLE;
    static Block* CHEST;             // 箱子（含水）
    static Block* TRAPPED_CHEST;     // 陷阱箱（含水）
    static Block* SHULKER_BOX;       // 潜影盒
    static Block* LOOM;              // 织布机
    static Block* BARREL;            // 木桶
    static Block* CARTOGRAPHY_TABLE; // 制图台
    static Block* FLETCHING_TABLE;   // 制箭台
    static Block* SMITHING_TABLE;    // 锻造台
    static Block* COMPOSTER;         // 堆肥桶
    static Block* LECTERN;           // 讲台
    static Block* JUKEBOX;           // 唱片机
    static Block* SPAWNER;           // 刷怪笼
    static Block* STRUCTURE_BLOCK;   // 结构方块
    static Block* STRUCTURE_VOID;    // 结构空位
    static Block* JIGSAW;            // 拼图方块
    static Block* BARRIER;           // 屏障

    // ========================================================================
    // 含水方块
    // ========================================================================
    static Block* LADDER;      // 梯子（含水）
    static Block* CHAIN;       // 锁链（含水）
    static Block* SCAFFOLDING; // 脚手架（含水）
    static Block* GLASS_PANE;  // 玻璃板（含水）
    static Block* IRON_BARS;   // 铁栏杆（含水）

    // ========================================================================
    // 门和栅栏门
    // ========================================================================
    static Block* OAK_DOOR;
    static Block* IRON_DOOR;
    static Block* OAK_FENCE_GATE;

    // ========================================================================
    // 楼梯（示例）
    // ========================================================================
    static Block* OAK_STAIRS;
    static Block* STONE_STAIRS;
    static Block* COBBLESTONE_STAIRS;

    // ========================================================================
    // 台阶（示例）
    // ========================================================================
    static Block* OAK_SLAB;
    static Block* STONE_SLAB;
    static Block* COBBLESTONE_SLAB;

    // ========================================================================
    // 墙（示例）
    // ========================================================================
    static Block* COBBLESTONE_WALL;
    static Block* STONE_BRICK_WALL;

    // ========================================================================
    // 栅栏（示例）
    // ========================================================================
    static Block* OAK_FENCE;

    // ========================================================================
    // 活板门（示例）
    // ========================================================================
    static Block* OAK_TRAPDOOR;
    static Block* IRON_TRAPDOOR;

    // ========================================================================
    // 羊毛 (16色)
    // ========================================================================
    static Block* WHITE_WOOL;
    static Block* ORANGE_WOOL;
    static Block* MAGENTA_WOOL;
    static Block* LIGHT_BLUE_WOOL;
    static Block* YELLOW_WOOL;
    static Block* LIME_WOOL;
    static Block* PINK_WOOL;
    static Block* GRAY_WOOL;
    static Block* LIGHT_GRAY_WOOL;
    static Block* CYAN_WOOL;
    static Block* PURPLE_WOOL;
    static Block* BLUE_WOOL;
    static Block* BROWN_WOOL;
    static Block* GREEN_WOOL;
    static Block* RED_WOOL;
    static Block* BLACK_WOOL;

    // ========================================================================
    // 地毯 (16色)
    // ========================================================================
    static Block* WHITE_CARPET;
    static Block* ORANGE_CARPET;
    static Block* MAGENTA_CARPET;
    static Block* LIGHT_BLUE_CARPET;
    static Block* YELLOW_CARPET;
    static Block* LIME_CARPET;
    static Block* PINK_CARPET;
    static Block* GRAY_CARPET;
    static Block* LIGHT_GRAY_CARPET;
    static Block* CYAN_CARPET;
    static Block* PURPLE_CARPET;
    static Block* BLUE_CARPET;
    static Block* BROWN_CARPET;
    static Block* GREEN_CARPET;
    static Block* RED_CARPET;
    static Block* BLACK_CARPET;

    // ========================================================================
    // 木板变种
    // ========================================================================
    static Block* SPRUCE_PLANKS;
    static Block* BIRCH_PLANKS;
    static Block* JUNGLE_PLANKS;
    static Block* ACACIA_PLANKS;
    static Block* DARK_OAK_PLANKS;

    // ========================================================================
    // 原木和树叶
    // ========================================================================
    static Block* OAK_LOG;
    static Block* OAK_WOOD;
    static Block* OAK_LEAVES;
    static Block* SPRUCE_LOG;
    static Block* SPRUCE_WOOD;
    static Block* BIRCH_LOG;
    static Block* BIRCH_WOOD;
    static Block* JUNGLE_LOG;
    static Block* JUNGLE_WOOD;
    static Block* ACACIA_LOG;
    static Block* ACACIA_WOOD;
    static Block* DARK_OAK_LOG;
    static Block* DARK_OAK_WOOD;
    static Block* STRIPPED_OAK_LOG;
    static Block* STRIPPED_SPRUCE_LOG;
    static Block* STRIPPED_BIRCH_LOG;
    static Block* STRIPPED_JUNGLE_LOG;
    static Block* STRIPPED_ACACIA_LOG;
    static Block* STRIPPED_DARK_OAK_LOG;
    static Block* STRIPPED_OAK_WOOD;
    static Block* STRIPPED_SPRUCE_WOOD;
    static Block* STRIPPED_BIRCH_WOOD;
    static Block* STRIPPED_JUNGLE_WOOD;
    static Block* STRIPPED_ACACIA_WOOD;
    static Block* STRIPPED_DARK_OAK_WOOD;
    static Block* SPRUCE_LEAVES;
    static Block* BIRCH_LEAVES;
    static Block* JUNGLE_LEAVES;
    static Block* ACACIA_LEAVES;
    static Block* DARK_OAK_LEAVES;

    // ========================================================================
    // 植被方块
    // ========================================================================
    static Block* SHORT_GRASS;
    static Block* TALL_GRASS;
    static Block* FERN;
    static Block* DANDELION;
    static Block* POPPY;
    static Block* BLUE_ORCHID;
    static Block* ALLIUM;
    static Block* AZURE_BLUET;
    static Block* RED_TULIP;
    static Block* ORANGE_TULIP;
    static Block* WHITE_TULIP;
    static Block* PINK_TULIP;
    static Block* OXEYE_DAISY;
    static Block* LILY_OF_THE_VALLEY; // 铃兰
    static Block* SUNFLOWER;          // 向日葵
    static Block* LILAC;              // 丁香
    static Block* ROSE_BUSH;          // 玫瑰丛
    static Block* PEONY;              // 牡丹
    static Block* CORNFLOWER;         // 矢车菊
    static Block* WITHER_ROSE;        // 凋零玫瑰
    static Block* BROWN_MUSHROOM;
    static Block* RED_MUSHROOM;
    static Block* BROWN_MUSHROOM_BLOCK; // 棕色蘑菇方块
    static Block* RED_MUSHROOM_BLOCK;   // 红色蘑菇方块
    static Block* MUSHROOM_STEM;        // 蘑菇柄

    // ========================================================================
    // 树苗
    // ========================================================================
    static Block* OAK_SAPLING;
    static Block* SPRUCE_SAPLING;
    static Block* BIRCH_SAPLING;
    static Block* JUNGLE_SAPLING;
    static Block* ACACIA_SAPLING;
    static Block* DARK_OAK_SAPLING;

    // ========================================================================
    // 石砖系列
    // ========================================================================
    static Block* STONE_BRICKS;
    static Block* MOSSY_STONE_BRICKS;
    static Block* CRACKED_STONE_BRICKS;
    static Block* CHISELED_STONE_BRICKS;
    // 石砖楼梯和台阶
    static Block* STONE_BRICK_STAIRS;
    static Block* STONE_BRICK_SLAB;
    // 苔藓石砖变种
    static Block* MOSSY_STONE_BRICK_STAIRS;
    static Block* MOSSY_STONE_BRICK_SLAB;
    static Block* MOSSY_STONE_BRICK_WALL;

    // ========================================================================
    // 虫蚀方块系列 (Infested Blocks / Monster Eggs)
    // ========================================================================
    static Block* INFESTED_STONE;                 // 虫蚀石头
    static Block* INFESTED_COBBLESTONE;           // 虫蚀圆石
    static Block* INFESTED_STONE_BRICKS;          // 虫蚀石砖
    static Block* INFESTED_MOSSY_STONE_BRICKS;    // 虫蚀苔藓石砖
    static Block* INFESTED_CRACKED_STONE_BRICKS;  // 虫蚀裂纹石砖
    static Block* INFESTED_CHISELED_STONE_BRICKS; // 虫蚀錾制石砖

    // ========================================================================
    // 石英系列
    // ========================================================================
    static Block* QUARTZ_BLOCK;
    static Block* CHISELED_QUARTZ_BLOCK;
    static Block* QUARTZ_PILLAR;
    // 注：NETHER_QUARTZ_ORE 在"下界矿石"部分定义

    // ========================================================================
    // 海晶系列
    // ========================================================================
    static Block* PRISMARINE;
    static Block* PRISMARINE_BRICKS;
    static Block* DARK_PRISMARINE;
    static Block* PRISMARINE_STAIRS;
    static Block* PRISMARINE_BRICK_STAIRS;
    static Block* DARK_PRISMARINE_STAIRS;
    static Block* PRISMARINE_SLAB;
    static Block* PRISMARINE_BRICK_SLAB;
    static Block* DARK_PRISMARINE_SLAB;
    static Block* SEA_LANTERN;

    // ========================================================================
    // 紫珀系列
    // ========================================================================
    static Block* PURPUR_BLOCK;
    static Block* PURPUR_PILLAR;

    // ========================================================================
    // 末地系列
    // ========================================================================
    static Block* END_STONE_BRICKS;
    static Block* END_ROD;
    static Block* CHORUS_PLANT;  // 紫颂植物
    static Block* CHORUS_FLOWER; // 紫颂花
    static Block* DRAGON_EGG;    // 龙蛋

    // ========================================================================
    // 骨块与干草块
    // ========================================================================
    static Block* BONE_BLOCK;
    static Block* HAY_BLOCK;

    // ========================================================================
    // 其他方块
    // ========================================================================
    static Block* SNOW;       // 雪层（可堆叠）
    static Block* SNOW_BLOCK; // 雪块（固体方块，用于冰刺等）
    static Block* ICE;
    static Block* GLASS; // 玻璃（透明，不传播天空光）

    // ========================================================================
    // 南瓜和西瓜系列
    // ========================================================================
    static Block* MELON;                 // 西瓜方块
    static Block* PUMPKIN;               // 南瓜方块（可被剪刀雕刻）
    static Block* CARVED_PUMPKIN;        // 雕刻南瓜（可生成傀儡）
    static Block* MELON_STEM;            // 西瓜茎
    static Block* PUMPKIN_STEM;          // 南瓜茎
    static Block* ATTACHED_MELON_STEM;   // 连接西瓜茎（西瓜生成后茎变成的方块）
    static Block* ATTACHED_PUMPKIN_STEM; // 连接南瓜茎（南瓜生成后茎变成的方块）
    // 注意：JACK_O_LANTERN 在其他方块部分定义

    static Block* NETHERRACK;
    static Block* GLOWSTONE;
    static Block* END_STONE;
    static Block* OBSIDIAN;
    static Block* NETHER_PORTAL;    // 下界传送门
    static Block* END_PORTAL;       // 末地传送门
    static Block* END_PORTAL_FRAME; // 末地传送门框架
    static Block* END_GATEWAY;      // 末地折跃门
    static Block* BEACON;           // 信标
    static Block* BREWING_STAND;    // 酿造台
    static Block* ENDER_CHEST;      // 末影箱
    static Block* LANTERN;          // 灯笼
    static Block* SOUL_LANTERN;     // 灵魂灯笼
    static Block* CAMPFIRE;         // 营火
    static Block* SOUL_CAMPFIRE;    // 灵魂营火
    static Block* JACK_O_LANTERN;   // 南瓜灯

    // ========================================================================
    // 红石方块
    // ========================================================================
    static Block* REDSTONE_WIRE;                 // 红石线
    static Block* REDSTONE_TORCH;                // 红石火把
    static Block* REDSTONE_WALL_TORCH;           // 墙上的红石火把
    static Block* REDSTONE_LAMP;                 // 红石灯
    static Block* REDSTONE_REPEATER;             // 红石中继器
    static Block* REDSTONE_COMPARATOR;           // 红石比较器
    static Block* OBSERVER;                      // 侦测器
    static Block* LEVER;                         // 拉杆
    static Block* STONE_BUTTON;                  // 石头按钮
    static Block* OAK_BUTTON;                    // 橡木按钮
    static Block* SPRUCE_BUTTON;                 // 云杉木按钮
    static Block* BIRCH_BUTTON;                  // 白桦木按钮
    static Block* JUNGLE_BUTTON;                 // 丛林木按钮
    static Block* ACACIA_BUTTON;                 // 金合欢木按钮
    static Block* DARK_OAK_BUTTON;               // 深色橡木按钮
    static Block* CRIMSON_BUTTON;                // 绯红按钮
    static Block* WARPED_BUTTON;                 // 诡异按钮
    static Block* STONE_PRESSURE_PLATE;          // 石头压力板
    static Block* OAK_PRESSURE_PLATE;            // 橡木压力板
    static Block* LIGHT_WEIGHTED_PRESSURE_PLATE; // 轻质测重压力板
    static Block* HEAVY_WEIGHTED_PRESSURE_PLATE; // 重质测重压力板
    static Block* DAYLIGHT_DETECTOR;             // 日光探测器
    static Block* PISTON;                        // 活塞
    static Block* STICKY_PISTON;                 // 粘性活塞
    static Block* PISTON_HEAD;                   // 活塞头
    static Block* MOVING_PISTON;                 // 移动中的活塞
    static Block* DISPENSER;                     // 发射器
    static Block* DROPPER;                       // 投掷器
    static Block* NOTE_BLOCK;                    // 音符盒
    static Block* TRIPWIRE;                      // 绊线
    static Block* TRIPWIRE_HOOK;                 // 绊线钩
    static Block* TARGET;                        // 标靶

    // ========================================================================
    // 铁轨方块
    // ========================================================================
    static Block* RAIL;           // 普通铁轨
    static Block* POWERED_RAIL;   // 动力铁轨
    static Block* DETECTOR_RAIL;  // 探测铁轨
    static Block* ACTIVATOR_RAIL; // 激活铁轨

    // ========================================================================
    // 染色玻璃 (16色)
    // ========================================================================
    static Block* WHITE_STAINED_GLASS;
    static Block* ORANGE_STAINED_GLASS;
    static Block* MAGENTA_STAINED_GLASS;
    static Block* LIGHT_BLUE_STAINED_GLASS;
    static Block* YELLOW_STAINED_GLASS;
    static Block* LIME_STAINED_GLASS;
    static Block* PINK_STAINED_GLASS;
    static Block* GRAY_STAINED_GLASS;
    static Block* LIGHT_GRAY_STAINED_GLASS;
    static Block* CYAN_STAINED_GLASS;
    static Block* PURPLE_STAINED_GLASS;
    static Block* BLUE_STAINED_GLASS;
    static Block* BROWN_STAINED_GLASS;
    static Block* GREEN_STAINED_GLASS;
    static Block* RED_STAINED_GLASS;
    static Block* BLACK_STAINED_GLASS;

    // ========================================================================
    // 混凝土 (16色)
    // ========================================================================
    static Block* WHITE_CONCRETE;
    static Block* ORANGE_CONCRETE;
    static Block* MAGENTA_CONCRETE;
    static Block* LIGHT_BLUE_CONCRETE;
    static Block* YELLOW_CONCRETE;
    static Block* LIME_CONCRETE;
    static Block* PINK_CONCRETE;
    static Block* GRAY_CONCRETE;
    static Block* LIGHT_GRAY_CONCRETE;
    static Block* CYAN_CONCRETE;
    static Block* PURPLE_CONCRETE;
    static Block* BLUE_CONCRETE;
    static Block* BROWN_CONCRETE;
    static Block* GREEN_CONCRETE;
    static Block* RED_CONCRETE;
    static Block* BLACK_CONCRETE;

    // ========================================================================
    // 混凝土粉末 (16色)
    // ========================================================================
    static Block* WHITE_CONCRETE_POWDER;
    static Block* ORANGE_CONCRETE_POWDER;
    static Block* MAGENTA_CONCRETE_POWDER;
    static Block* LIGHT_BLUE_CONCRETE_POWDER;
    static Block* YELLOW_CONCRETE_POWDER;
    static Block* LIME_CONCRETE_POWDER;
    static Block* PINK_CONCRETE_POWDER;
    static Block* GRAY_CONCRETE_POWDER;
    static Block* LIGHT_GRAY_CONCRETE_POWDER;
    static Block* CYAN_CONCRETE_POWDER;
    static Block* PURPLE_CONCRETE_POWDER;
    static Block* BLUE_CONCRETE_POWDER;
    static Block* BROWN_CONCRETE_POWDER;
    static Block* GREEN_CONCRETE_POWDER;
    static Block* RED_CONCRETE_POWDER;
    static Block* BLACK_CONCRETE_POWDER;

    // ========================================================================
    // 陶瓦 (16色)
    // ========================================================================
    static Block* WHITE_TERRACOTTA;
    static Block* ORANGE_TERRACOTTA;
    static Block* MAGENTA_TERRACOTTA;
    static Block* LIGHT_BLUE_TERRACOTTA;
    static Block* YELLOW_TERRACOTTA;
    static Block* LIME_TERRACOTTA;
    static Block* PINK_TERRACOTTA;
    static Block* GRAY_TERRACOTTA;
    static Block* LIGHT_GRAY_TERRACOTTA;
    static Block* CYAN_TERRACOTTA;
    static Block* PURPLE_TERRACOTTA;
    static Block* BLUE_TERRACOTTA;
    static Block* BROWN_TERRACOTTA;
    static Block* GREEN_TERRACOTTA;
    static Block* RED_TERRACOTTA;
    static Block* BLACK_TERRACOTTA;
    static Block* TERRACOTTA; // 普通陶瓦

    // ========================================================================
    // 下界方块
    // ========================================================================
    static Block* SOUL_SAND;
    static Block* SOUL_SOIL;
    static Block* BASALT;
    static Block* POLISHED_BASALT;
    static Block* BLACKSTONE;
    static Block* POLISHED_BLACKSTONE;
    static Block* CRYING_OBSIDIAN;
    static Block* RESPAWN_ANCHOR;    // 重生锚
    static Block* MAGMA;             // 岩浆块 (发光)
    static Block* NETHER_WART_BLOCK; // 地狱疣块
    static Block* WARPED_WART_BLOCK; // 诡异疣块
    static Block* FIRE;              // 火
    static Block* SOUL_FIRE;         // 灵魂火
    static Block* NETHER_WART;       // 下界疣（作物）

    // ========================================================================
    // 自然方块扩展
    // ========================================================================
    static Block* CLAY;             // 粘土
    static Block* MYCELIUM;         // 菌丝
    static Block* GRASS_PATH;       // 草径
    static Block* PACKED_ICE;       // 浮冰
    static Block* BLUE_ICE;         // 蓝冰
    static Block* FROSTED_ICE;      // 霜冰
    static Block* SLIME_BLOCK;      // 粘液块
    static Block* HONEY_BLOCK;      // 蜂蜜块
    static Block* CACTUS;           // 仙人掌
    static Block* DEAD_BUSH;        // 枯萎灌木
    static Block* LILY_PAD;         // 睡莲
    static Block* VINE;             // 藤蔓
    static Block* COBWEB;           // 蜘蛛网
    static Block* SUGAR_CANE;       // 甘蔗
    static Block* FARMLAND;         // 耕地
    static Block* RED_SAND;         // 红沙
    static Block* DRIED_KELP_BLOCK; // 干海带块
    static Block* SEA_PICKLE;       // 海泡菜
    static Block* KELP;             // 海带顶部
    static Block* KELP_PLANT;       // 海带茎
    static Block* SEAGRASS;         // 海草
    static Block* TALL_SEAGRASS;    // 高海草
    static Block* BUBBLE_COLUMN;    // 气泡柱
    static Block* TURTLE_EGG;       // 海龟蛋
    static Block* BAMBOO;           // 竹子
    static Block* BAMBOO_SAPLING;   // 竹子幼苗

    // ========================================================================
    // 珊瑚方块
    // ========================================================================
    static Block* DEAD_TUBE_CORAL_BLOCK;
    static Block* DEAD_BRAIN_CORAL_BLOCK;
    static Block* DEAD_BUBBLE_CORAL_BLOCK;
    static Block* DEAD_FIRE_CORAL_BLOCK;
    static Block* DEAD_HORN_CORAL_BLOCK;

    static Block* DEAD_TUBE_CORAL_FAN;
    static Block* DEAD_BRAIN_CORAL_FAN;
    static Block* DEAD_BUBBLE_CORAL_FAN;
    static Block* DEAD_FIRE_CORAL_FAN;
    static Block* DEAD_HORN_CORAL_FAN;

    static Block* DEAD_TUBE_CORAL_WALL_FAN;
    static Block* DEAD_BRAIN_CORAL_WALL_FAN;
    static Block* DEAD_BUBBLE_CORAL_WALL_FAN;
    static Block* DEAD_FIRE_CORAL_WALL_FAN;
    static Block* DEAD_HORN_CORAL_WALL_FAN;

    static Block* TUBE_CORAL_BLOCK;
    static Block* BRAIN_CORAL_BLOCK;
    static Block* BUBBLE_CORAL_BLOCK;
    static Block* FIRE_CORAL_BLOCK;
    static Block* HORN_CORAL_BLOCK;

    static Block* TUBE_CORAL_FAN;
    static Block* BRAIN_CORAL_FAN;
    static Block* BUBBLE_CORAL_FAN;
    static Block* FIRE_CORAL_FAN;
    static Block* HORN_CORAL_FAN;

    static Block* TUBE_CORAL_WALL_FAN;
    static Block* BRAIN_CORAL_WALL_FAN;
    static Block* BUBBLE_CORAL_WALL_FAN;
    static Block* FIRE_CORAL_WALL_FAN;
    static Block* HORN_CORAL_WALL_FAN;

    static Block* CONDUIT; // 潮涌核心

    // ========================================================================
    // 告示牌（含水）
    // ========================================================================
    static Block* OAK_SIGN;           // 橡木告示牌（站立）
    static Block* OAK_WALL_SIGN;      // 橡木告示牌（墙面）
    static Block* SPRUCE_SIGN;        // 云杉木告示牌（站立）
    static Block* SPRUCE_WALL_SIGN;   // 云杉木告示牌（墙面）
    static Block* BIRCH_SIGN;         // 白桦木告示牌（站立）
    static Block* BIRCH_WALL_SIGN;    // 白桦木告示牌（墙面）
    static Block* JUNGLE_SIGN;        // 丛林木告示牌（站立）
    static Block* JUNGLE_WALL_SIGN;   // 丛林木告示牌（墙面）
    static Block* ACACIA_SIGN;        // 金合欢木告示牌（站立）
    static Block* ACACIA_WALL_SIGN;   // 金合欢木告示牌（墙面）
    static Block* DARK_OAK_SIGN;      // 深色橡木告示牌（站立）
    static Block* DARK_OAK_WALL_SIGN; // 深色橡木告示牌（墙面）
    static Block* CRIMSON_SIGN;       // 绯红告示牌（站立）
    static Block* CRIMSON_WALL_SIGN;  // 绯红告示牌（墙面）
    static Block* WARPED_SIGN;        // 诡异告示牌（站立）
    static Block* WARPED_WALL_SIGN;   // 诡异告示牌（墙面）

    // ========================================================================
    // 下界扩展植物方块
    // ========================================================================
    static Block* CRIMSON_STEM;            // 绯红菌柄
    static Block* WARPED_STEM;             // 诡异菌柄
    static Block* STRIPPED_CRIMSON_STEM;   // 去皮绯红菌柄
    static Block* STRIPPED_WARPED_STEM;    // 去皮诡异菌柄
    static Block* CRIMSON_HYPHAE;          // 绯红菌核
    static Block* WARPED_HYPHAE;           // 诡异菌核
    static Block* STRIPPED_CRIMSON_HYPHAE; // 去皮绯红菌核
    static Block* STRIPPED_WARPED_HYPHAE;  // 去皮诡异菌核
    static Block* CRIMSON_NYLIUM;          // 绯红菌岩
    static Block* WARPED_NYLIUM;           // 诡异菌岩
    static Block* SHROOMLIGHT;             // 菌光体
    static Block* CRIMSON_FUNGUS;          // 绯红菌
    static Block* WARPED_FUNGUS;           // 诡异菌
    static Block* WEEPING_VINES;           // 垂泪藤
    static Block* TWISTING_VINES;          // 扭曲藤

    // ========================================================================
    // 辅助函数
    // ========================================================================

    /**
     * @brief 安全获取方块默认状态
     *
     * 用于在初始化阶段可能尚未注册方块时安全获取默认状态。
     * 如果方块为空指针，返回 nullptr。
     *
     * @param block 方块指针（可能为 nullptr）
     * @return 方块默认状态指针，如果方块为空则返回 nullptr
     */
    [[nodiscard]] static const BlockState* getState(Block* block) { return block ? &block->defaultState() : nullptr; }

private:
    static bool s_initialized;

    static void registerBaseBlocks();
    static void registerOreBlocks();
    static void registerLogBlocks();
    static void registerStoneVariants();
    static void registerDirtVariants();
    static void registerSandstones();
    static void registerMineralBlocks();
    static void registerBuildingBlocks();
    static void registerFunctionalBlocks();
    static void registerWoolBlocks();
    static void registerCarpetBlocks(); // 地毯
    static void registerPlanksVariants();
    static void registerNetherBlocks();
    static void registerTreeVariants();
    static void registerVegetationBlocks();
    static void registerColoredBlocks();         // 染色玻璃、混凝土、陶瓦等
    static void registerStoneBricks();           // 石砖系列
    static void registerInfestedBlocks();        // 虫蚀方块系列
    static void registerQuartzBlocks();          // 石英系列
    static void registerPrismarineBlocks();      // 海晶系列
    static void registerSignBlocks();            // 告示牌
    static void registerPurpurBlocks();          // 紫珀系列
    static void registerEndBlocks();             // 末地方块
    static void registerBoneAndHayBlocks();      // 骨块和干草块
    static void registerNetherExtensionBlocks(); // 下界扩展方块（岩浆块等）
    static void registerNaturalBlocks();         // 自然扩展方块
    static void registerPumpkinMelonBlocks();    // 南瓜和西瓜系列
    static void registerRedstoneBlocks();        // 红石方块
    static void registerStairsSlabsWalls();      // 楼梯、台阶、墙、栅栏、活板门
    static void registerSpecialBlocks();         // 特殊方块（刷怪笼、结构方块、屏障等）
};

} // namespace mc
