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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace mc {

class Block;
class BlockState;

/**
 * @brief 方块标签
 *
 * 用于将方块分组以便功能判断。
 *
 * 用法示例:
 * @code
 * // 检查方块状态是否在标签中
 * if (BlockTags::JUNGLE_LOGS.contains(state)) {
 *     // 方块是丛林原木
 * }
 * @endcode
 */
class BlockTag {
public:
    /**
     * @brief 构造方块标签
     * @param id 标签资源位置
     */
    explicit BlockTag(ResourceLocation id) noexcept;

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& getId() const { return m_id; }

    /**
     * @brief 添加方块到标签
     * @param blockId 方块资源位置
     */
    void add(const ResourceLocation& blockId);

    /**
     * @brief 批量添加方块
     * @param blockIds 方块资源位置列表
     */
    void addAll(const std::vector<ResourceLocation>& blockIds);

    /**
     * @brief 检查方块是否在标签中
     * @param blockId 方块资源位置
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const ResourceLocation& blockId) const noexcept;

    /**
     * @brief 检查方块是否在标签中
     * @param block 方块指针
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const Block* block) const;

    /**
     * @brief 检查方块是否在标签中
     * @param block 方块引用
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const Block& block) const;

    /**
     * @brief 检查方块状态是否在标签中
     * @param state 方块状态引用
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const BlockState& state) const;

    /**
     * @brief 获取标签中的所有方块ID
     */
    [[nodiscard]] const std::unordered_set<ResourceLocation>& getBlockIds() const { return m_blockIds; }

private:
    ResourceLocation m_id;
    std::unordered_set<ResourceLocation> m_blockIds;
};

/**
 * @brief 内置方块标签集合
 */
class BlockTags {
public:
    // ========== 原木标签 ==========

    /// 原木标签（所有原木）
    static BlockTag& LOGS();

    /// 丛林原木标签（丛林原木、丛林木、去皮丛林原木、去皮丛林木）
    static BlockTag& JUNGLE_LOGS();

    /// 橡木原木标签
    static BlockTag& OAK_LOGS();

    /// 云杉原木标签
    static BlockTag& SPRUCE_LOGS();

    /// 白桦原木标签
    static BlockTag& BIRCH_LOGS();

    /// 金合欢原木标签
    static BlockTag& ACACIA_LOGS();

    /// 深色橡木原木标签
    static BlockTag& DARK_OAK_LOGS();

    /// 绯红原木标签
    static BlockTag& CRIMSON_STEMS();

    /// 诡异原木标签
    static BlockTag& WARPED_STEMS();

    // ========== 其他常用标签 ==========

    /// 叶子标签
    static BlockTag& LEAVES();

    /// 木板标签
    static BlockTag& PLANKS();

    /// 土壤标签（可以种植的土地）
    static BlockTag& DIRT();

    /// 沙子标签
    static BlockTag& SAND();

    /// 石头标签
    static BlockTag& STONE();

    /// 陶瓦标签（原色陶瓦 + 16 色陶瓦）
    /// 运行时消费场景：DryVegetationBlock 判定干草类是否可放置在陶瓦上。
    static BlockTag& TERRACOTTA();

    /// 干草类可种植标签（= SAND + TERRACOTTA + DIRT + FARMLAND，是 #dirt 的超集）
    /// 运行时消费场景：DryVegetationBlock（short_dry_grass / tall_dry_grass）判定下方是否可支撑。
    static BlockTag& DRY_VEGETATION_MAY_PLACE_ON();

    /// 菌丝岩标签（crimson_nylium / warped_nylium）
    /// 运行时消费场景：NetherForestVegetationFeature 判断 origin 下方是否为菌丝岩。
    static BlockTag& NYLIUM();

    /// 火标签
    static BlockTag& FIRE();

    /// 灵魂火基座方块标签（灵魂沙、灵魂土）
    /// 在这些方块上可以点燃灵魂火
    static BlockTag& SOUL_FIRE_BASE_BLOCKS();

    /// 营火标签（营火、灵魂营火）
    /// 运行时消费场景：
    /// 1. CampfireBlock::isLitCampfire() — 判断是否为点燃的营火方块
    /// 2. CampfireBlock::isSmokeyPos() — 判断蜂巢下方是否有营火（烟雾信号）
    static BlockTag& CAMPFIRES();

    /// 蜡烛标签
    /// 包含所有蜡烛方块（普通蜡烛 + 16种染色蜡烛）
    /// 运行时消费场景：
    /// 1. AbstractCandleBlock::isLit() — 判断是否为点燃的蜡烛
    /// 2. CandleBlock::canLight() — 判断是否可点燃
    static BlockTag& CANDLES();

    /// 蜡烛蛋糕标签
    /// 包含所有蜡烛蛋糕方块（普通蜡烛蛋糕 + 16种染色蜡烛蛋糕）
    /// 运行时消费场景：
    /// 1. AbstractCandleBlock::isLit() — 判断是否为点燃的蜡烛蛋糕
    static BlockTag& CANDLE_CAKES();

    /// 羊毛标签
    static BlockTag& WOOL();

    /// 羊毛地毯标签（所有颜色的地毯方块）
    /// 运行时消费场景：
    /// 1. DAMPENS_VIBRATIONS 标签的组成项（地毯方块阻尼振动）— 已在 VibrationSystemServer 中消费
    /// 2. COMBINATION_STEP_SOUND_BLOCKS 标签的组成项（组合脚步声）— 已在 Entity/Player 步声逻辑中消费
    /// 3. 羊驼装备判定（MC 1.21+ 使用 Equippable 数据组件，非标签判断）— 已在 LlamaEntity::isValidArmorForSlot 中使用
    static BlockTag& WOOL_CARPETS();

    /// 床标签（所有颜色的床方块）
    /// 运行时消费场景：
    /// 1. 村民睡眠目标判定 — 已在 VillagerEntity/GoToBedGoal 中消费
    /// 2. 猫咪坐上方块判定 — 已在 CatGoals 中消费
    static BlockTag& BEDS();

    /// 木质栅栏标签（所有木质栅栏，不含下界砖栅栏）
    static BlockTag& WOODEN_FENCES();

    /// 栅栏标签（所有木质栅栏 + 下界砖栅栏）
    static BlockTag& FENCES();

    /// 栅栏门标签
    static BlockTag& FENCE_GATES();

    /// 不稳定底部中心标签（栅栏门等）
    /// 运行时消费场景：
    /// 1. Block.canSupportCenter 判定 — 栅栏门虽占完整方块，但顶部无法提供 Center 支撑
    ///    用于钟、灯笼、火把、孢子花、蜡烛等悬挂类方块的支撑判定
    /// 对应 MC 1.21.11 #minecraft:unstable_bottom_center，数据包内容为 #minecraft:fence_gates
    static BlockTag& UNSTABLE_BOTTOM_CENTER();

    /// 木质书架标签（所有12种木质/下界木质书架变体）
    /// 运行时消费场景：
    /// 1. ShelfBlock 侧链连接判定 — 判断相邻书架是否可连接
    /// 2. 可被斧头快速挖掘（mineable_with_axe 标签的组成项）
    /// 3. 作为熔炉燃料（燃烧时间 = 基础木质 × 1.5）
    static BlockTag& WOODEN_SHELVES();

    /// 竹子可种植标签（草、泥土、沙子、沙砾、竹林土）
    static BlockTag& BAMBOO_PLANTABLE_ON();

    /// 蘑菇可生长方块标签（菌丝、灰化土、绯红菌岩、诡异菌岩）
    /// 蘑菇在这些方块上放置时不受光照限制
    static BlockTag& MUSHROOM_GROW_BLOCK();

    /// 甜浆果丛可种植标签（草方块、泥土、粗糙泥土、灰化土、耕地）
    static BlockTag& VALID_SWEET_BERRY_BUSH_GROUND();

    // ========== 珊瑚和水下骨粉标签 ==========

    /// 墙珊瑚标签（所有墙珊瑚扇，包括死的和活的）
    static BlockTag& WALL_CORALS();

    /// 水下骨粉标签（骨粉可以在水下催熟的方块）
    static BlockTag& UNDERWATER_BONEMEALS();

    // ========== 炽足兽标签 ==========

    /// 炽足兽温暖方块标签（熔岩方块）
    /// 炽足兽在这些方块上不会感到寒冷
    static BlockTag& STRIDER_WARM_BLOCKS();

    // ========== 疣猪兽/猪灵排斥物标签 ==========

    /// 疣猪兽排斥物标签
    /// 疣猪兽在这些方块附近会逃跑，getPathWeight 返回 -1.0
    /// 包含: 诡异菌(warped_fungus)、盆栽诡异菌(potted_warped_fungus)、下界传送门(nether_portal)、重生锚(respawn_anchor)
    /// MC 1.21.11: BlockTags.HOGLIN_REPELLENTS
    static BlockTag& HOGLIN_REPELLENTS();

    /// 猪灵排斥物标签
    /// 猪灵在这些方块附近会逃跑
    /// 包含: 灵魂火(soul_fire)、灵魂火把(soul_torch)、灵魂墙火把(soul_wall_torch)、
    ///       灵魂灯笼(soul_lantern)、灵魂营火(soul_campfire，需点燃)
    /// 注意: MC 1.21.11 中 warped_fungus 不在 PIGLIN_REPELLENTS 中，仅存在于 HOGLIN_REPELLENTS
    /// MC 1.21.11: BlockTags.PIGLIN_REPELLENTS
    static BlockTag& PIGLIN_REPELLENTS();

    // ========== 漏斗标签 ==========

    /// 漏斗不阻挡标签
    /// 漏斗上方为此标签中的方块时，即使碰撞形状为完整方块，漏斗仍可吸取上方物品实体。
    /// MC Java 中仅包含 BEEHIVES 标签（蜂巢 bee_nest、蜂箱 beehive），
    /// 因为蜂巢/蜂箱虽碰撞形状为完整方块，但漏斗应能从中吸取蜂蜜瓶/空瓶。
    /// MC 1.21.11: BlockTags.DOES_NOT_BLOCK_HOPPERS
    static BlockTag& DOES_NOT_BLOCK_HOPPERS();

    // ========== 蜜蜂相关标签 ==========

    /// 小花朵标签（蒲公英、虞美人等）
    /// 蜜蜂可以采集这些花朵
    static BlockTag& SMALL_FLOWERS();

    /// 高花朵标签（向日葵、丁香等）
    /// 蜜蜂可以采集这些花朵
    /// 注意: MC 1.21.2+ 已移除 tall_flowers 标签，高花朵直接包含在 FLOWERS 标签中
    static BlockTag& TALL_FLOWERS();

    /// 花朵标签（所有花朵，包含小花朵、高花朵和其他花类方块）
    /// MC 1.21.11: BlockTags.FLOWERS
    static BlockTag& FLOWERS();

    /// 花盆标签（空花盆 + 所有 potted_* 盆栽方块）
    /// MC 1.21.11: BlockTags.FLOWER_POTS
    static BlockTag& FLOWER_POTS();

    /// 树苗标签（所有树苗，包含杜鹃花丛和红树胎生苗）
    /// MC 1.21.11: BlockTags.SAPLINGS
    static BlockTag& SAPLINGS();

    /// 蜂巢/蜂箱标签
    /// 蜜蜂可以进入的方块
    static BlockTag& BEEHIVES();

    /// 蜜蜂吸引物标签
    /// 蜜蜂被这些方块吸引（用于授粉目标判定、眼眸花中毒触发等）。
    /// 包含蒲公英、开放眼眸花、虞美人、郁金香、向日葵、丁香、牡丹等 29 个方块，
    /// 闭合眼眸花不在此标签中（蜜蜂不被闭合眼眸花吸引）。
    /// 含水的可水合花朵会被排除（由 attractsBees 工具函数处理），向日葵仅上半部分生效。
    /// MC 1.21.11: BlockTags.BEE_ATTRACTIVE
    static BlockTag& BEE_ATTRACTIVE();

    /// 蜜蜂可授粉作物标签
    /// 小麦、胡萝卜、马铃薯、甜菜根、西瓜茎、南瓜茎、甜浆果丛
    static BlockTag& BEE_GROWABLES();

    // ========== 末影人标签 ==========

    /// 末影人可拾取方块标签
    /// 草方块、泥土、沙子、沙砾、蘑菇、花、仙人掌、南瓜、西瓜、TNT等
    static BlockTag& ENDERMAN_HOLDABLE();

    // ========== 凋灵标签 ==========

    /// 凋灵免疫方块标签
    /// 凋灵无法破坏的方块：基岩、屏障、末地传送门、命令方块等
    static BlockTag& WITHER_IMMUNE();

    // ========== 末影龙标签 ==========

    /// 末影龙免疫方块标签
    /// 末影龙无法破坏的方块：基岩、末地传送门、黑曜石、末地石、铁栏杆等
    static BlockTag& DRAGON_IMMUNE();

    /// 末影龙透明方块标签
    /// 末影龙穿过时不破坏的方块：光照方块、火等
    static BlockTag& DRAGON_TRANSPARENT();

    // ========== 1.17 洞穴与山崖 标签 ==========

    /// 铜矿石标签（铜矿石、深板岩铜矿石）
    static BlockTag& COPPER_ORES();

    /// 深板岩矿石可替换方块标签（深板岩、凝灰岩）
    static BlockTag& DEEPSLATE_ORE_REPLACEABLES();

    /// 主世界基础石头标签（石头、花岗岩、闪长岩、安山岩、凝灰岩、深板岩）
    static BlockTag& BASE_STONE_OVERWORLD();

    /// 可被滴水石块替换的方块标签（DripstoneUtils 依赖）
    static BlockTag& DRIPSTONE_REPLACEABLE();

    /// 水晶声音方块标签（紫水晶块、紫水晶母岩）
    static BlockTag& CRYSTAL_SOUND_BLOCKS();

    /// 组合脚步声方块标签（羊毛地毯、苔藓地毯、苍白苔藓地毯、雪层、下界苗、诡异菌索、绯红菌索、树脂团）
    /// 当实体站在这些方块上时，脚步声会同时播放上方方块的正常步声和下方方块的沉闷步声
    static BlockTag& COMBINATION_STEP_SOUND_BLOCKS();

    /// 内部脚步声方块标签（细雪、幽匿脉络、发光地衣、睡莲、小型紫水晶芽、粉红色花瓣、野花、落叶层）
    /// 当实体站在这些方块上时，脚步声只播放上方方块自身的步声（替代脚下方块的步声）
    static BlockTag& INSIDE_STEP_SOUND_BLOCKS();

    /// 洞穴藤蔓标签
    static BlockTag& CAVE_VINES();

    /// 苔藓可替换方块标签
    static BlockTag& MOSS_REPLACEABLE();

    /// 繁茂洞穴地面可替换方块标签（黏土、石头、沙砾、泥土等）
    static BlockTag& LUSH_GROUND_REPLACEABLE();

    /// 杜鹃根系可替换方块标签（泥土、石头、沙砾等）
    static BlockTag& AZALEA_ROOT_REPLACEABLE();

    /// 铜块标签（所有铜质方块，含氧化和涂蜡变种）
    static BlockTag& COPPER();

    /// 铜傀儡雕像标签（所有 8 个变体：未涂蜡/涂蜡 × 4 个氧化等级）
    /// 参考: net.minecraft.tags.BlockTags.COPPER_GOLEM_STATUES (MC 1.21.11)
    /// 用于 shouldChangedStateKeepBlockEntity 判断：斧头刮削/去蜡时保留方块实体
    static BlockTag& COPPER_GOLEM_STATUES();

    /// 铜箱子标签（所有 8 个变体：未涂蜡/涂蜡 × 4 个氧化等级）
    /// 参考: net.minecraft.tags.BlockTags.COPPER_CHESTS (MC 1.21.11)
    /// 用于 chestCanConnectTo 判断：双箱合并允许跨氧化等级与涂蜡状态连接
    /// 以及 shouldChangedStateKeepBlockEntity 判断：斧头刮削/去蜡时保留方块实体
    static BlockTag& COPPER_CHESTS();

    /// 避雷针标签
    static BlockTag& LIGHTNING_RODS();

    /// 减振方块标签（羊毛等，可阻挡幽匿感测体振动）
    static BlockTag& DAMPENS_VIBRATIONS();

    /// 遮挡振动信号方块标签
    static BlockTag& OCCLUDES_VIBRATION_SIGNALS();

    /// 主世界自然原木标签
    static BlockTag& OVERWORLD_NATURAL_LOGS();

    /// 雪标签（雪、雪块、细雪）
    static BlockTag& SNOW();

    /// 细雪可放置标签
    static BlockTag& POWDER_SNOW_WALKABLE_MOVED();

    // ========== 1.19 荒野更新 标签 ==========

    /// 幽匿可替换方块标签
    static BlockTag& SCULK_REPLACEABLE();

    /// 幽匿世界生成可替换方块标签
    static BlockTag& SCULK_REPLACEABLE_WORLD_GEN();

    /// 远古城市可替换方块标签
    static BlockTag& ANCIENT_CITY_REPLACEABLE();

    /// 振动共振方块标签（紫水晶块）
    static BlockTag& VIBRATION_RESONATORS();

    /// 青蛙可生成标签
    static BlockTag& FROGS_SPAWNABLE_ON();

    /// 可转化为泥巴方块标签（泥土、粗泥、缠根泥土）
    static BlockTag& CONVERTABLE_TO_MUD();

    /// 红树林原木可生长标签
    static BlockTag& MANGROVE_LOGS_CAN_GROW_THROUGH();

    /// 红树林根可生长标签
    static BlockTag& MANGROVE_ROOTS_CAN_GROW_THROUGH();

    /// 红树林原木标签
    static BlockTag& MANGROVE_LOGS();

    // ========== 1.20 足迹与故事 标签 ==========

    /// 樱花原木标签
    static BlockTag& CHERRY_LOGS();

    /// 竹木方块标签（竹块、去皮竹块）
    static BlockTag& BAMBOO_BLOCKS();

    /// 嗅探兽可挖掘方块标签
    static BlockTag& SNIFFER_DIGGABLE_BLOCK();

    /// 嗅探兽蛋孵化加速方块标签（下方为此标签方块时孵化时间减半）
    static BlockTag& SNIFFER_EGG_HATCH_BOOST();

    // ========== 1.21 棘巧试炼 标签 ==========

    /// 不可被特性替换方块标签（基岩、刷怪笼、箱子等）
    static BlockTag& FEATURES_CANNOT_REPLACE();

    /// 熔岩湖边界石不可替换标签（引用 features_cannot_replace + leaves + logs）
    /// 用于 LakeFeature 边界方块放置判定
    static BlockTag& LAVA_POOL_STONE_CANNOT_REPLACE();

    /// 附魔力量提供者标签（书架）
    static BlockTag& ENCHANTMENT_POWER_PROVIDER();

    /// 附魔力量传输者标签（空气等，允许附魔力量穿过的方块）
    static BlockTag& ENCHANTMENT_POWER_TRANSMITTER();

    /// 维持耕地标签
    static BlockTag& MAINTAINS_FARMLAND();

    // ========== 1.21.2+ 苍白花园 标签 ==========

    /// 苍白橡木原木标签
    static BlockTag& PALE_OAK_LOGS();

    /// 可被树替换方块标签
    static BlockTag& REPLACEABLE_BY_TREES();

    // ========== 雕刻器标签 ==========

    /// 主世界可雕刻方块标签（石头变种、泥土类、深板岩、凝灰岩、方解石、沙砾等）
    static BlockTag& OVERWORLD_CARVER_REPLACEABLES();

    /// 下界可雕刻方块标签（下界岩、灵魂沙、灵魂土、玄武岩、黑石等）
    static BlockTag& NETHER_CARVER_REPLACEABLES();

    // ========== 铁砧标签 ==========

    /// 铁砧标签（包含 anvil、chipped_anvil、damaged_anvil）
    /// 用于下落铁砧损坏判定
    static BlockTag& ANVIL();

    // ========== 雪层放置标签 ==========

    /// 雪层不可放置标签（冰、浮冰、屏障）
    /// 雪层不能在这些方块上方存活
    static BlockTag& SNOW_LAYER_CANNOT_SURVIVE_ON();

    /// 雪层可放置标签（蜂蜜块、灵魂沙、泥巴）
    /// 雪层可以在这些方块上方存活（即使它们没有完整的上表面碰撞箱）
    static BlockTag& SNOW_LAYER_CAN_SURVIVE_ON();

    // ========== 滴叶标签 ==========

    /// 小滴叶可放置标签（黏土、苔藓块）
    /// 参考: net.minecraft.tags.BlockTags.SMALL_DRIPLEAF_PLACEABLE
    static BlockTag& SMALL_DRIPLEAF_PLACEABLE();

    /// 大滴叶可放置标签（黏土、泥土、砂土、灰化土、耕地、苔藓块、缠根泥土、泥巴、泥泞红树林根、草方块、菌丝、沙子、小滴叶等）
    /// 参考: net.minecraft.tags.BlockTags.BIG_DRIPLEAF_PLACEABLE
    static BlockTag& BIG_DRIPLEAF_PLACEABLE();

    // ========== 建筑方块形状标签 ==========

    /// 楼梯方块标签（所有楼梯方块）
    /// 参考: net.minecraft.tags.BlockTags.STAIRS
    static BlockTag& STAIRS();

    /// 台阶方块标签（所有台阶方块）
    /// 参考: net.minecraft.tags.BlockTags.SLABS
    static BlockTag& SLABS();

    /// 墙壁方块标签（所有墙壁方块，不含墙上的告示牌/旗帜等）
    /// 参考: net.minecraft.tags.BlockTags.WALLS
    static BlockTag& WALLS();

    /// 猪灵守护的方块标签（破坏时激怒附近猪灵）
    static BlockTag& GUARDED_BY_PIGLINS();

    /// 栏杆方块标签（铁栏杆等，用于墙壁和玻璃板连接判断）
    static BlockTag& BARS();

    /// 锁链方块标签（铁锁链和铜锁链，含氧化和涂蜡变种）
    /// 参考: net.minecraft.tags.BlockTags.CHAINS
    static BlockTag& CHAINS();

    /// 潜影盒标签（所有潜影盒变体，用于连接例外判断）
    static BlockTag& SHULKER_BOXES();

    /// 墙柱覆盖标签（火把、告示牌、旗帜、压力板等，放置在墙上时强制显示墙柱）
    static BlockTag& WALL_POST_OVERRIDE();

    /// 炼药锅标签（所有炼药锅变体：空炼药锅、水炼药锅、岩浆炼药锅、细雪炼药锅）
    /// 参考: net.minecraft.tags.BlockTags.CAULDRONS
    static BlockTag& CAULDRONS();

    /// 木门方块标签（所有木门方块）
    /// 包含所有材质的木门方块（橡木、云杉、白桦、丛林、金合欢、深色橡木、苍白橡木、
    /// 绯红木、诡异木、红树木、竹木、樱花木）。
    /// 参考: net.minecraft.tags.BlockTags.WOODEN_DOORS
    static BlockTag& WOODEN_DOORS();

    /// 门方块标签（所有门方块）
    /// 包含所有木门 + 铁门 + 铜门（含氧化和涂蜡变种）。
    /// 参考: net.minecraft.tags.BlockTags.DOORS
    static BlockTag& DOORS();

    /// 木活板门方块标签（所有木活板门方块）
    /// 包含所有材质的木活板门方块（橡木、云杉、白桦、丛林、金合欢、深色橡木、苍白橡木、
    /// 绯红木、诡异木、红树木、竹木、樱花木）。
    /// 参考: net.minecraft.tags.BlockTags.WOODEN_TRAPDOORS
    static BlockTag& WOODEN_TRAPDOORS();

    /// 活板门方块标签（所有活板门方块）
    /// 包含所有木活板门 + 铁活板门 + 铜活板门（含氧化和涂蜡变种）。
    /// 参考: net.minecraft.tags.BlockTags.TRAPDOORS
    static BlockTag& TRAPDOORS();

    /// 不可燃木材方块标签（所有不可燃烧的木材方块）
    /// 包含绯红木和诡异木系列的所有方块（原木、菌柄、木板、台阶、楼梯、
    /// 栅栏、栅栏门、门、活板门、按钮、压力板、告示牌等）。
    /// 参考: net.minecraft.tags.BlockTags.NON_FLAMMABLE_WOOD
    static BlockTag& NON_FLAMMABLE_WOOD();

    /**
     * @brief 初始化所有内置标签
     *
     * 在 BlockRegistry::initialize() 之后调用
     */
    static void initialize();

    /**
     * @brief 根据ID获取标签
     *
     * @param id 标签资源位置
     * @return 标签指针，如果不存在返回 nullptr
     */
    [[nodiscard]] static BlockTag* getTag(const ResourceLocation& id);

    /**
     * @brief 遍历所有标签
     */
    static void forEachTag(std::function<void(BlockTag&)> callback);

private:
    BlockTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<BlockTag>>& _getTags();
    static bool s_initialized;
};

} // namespace mc
