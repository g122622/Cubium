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

    /// 火标签
    static BlockTag& FIRE();

    /// 灵魂火基座方块标签（灵魂沙、灵魂土）
    /// 在这些方块上可以点燃灵魂火
    static BlockTag& SOUL_FIRE_BASE_BLOCKS();

    /// 羊毛标签
    static BlockTag& WOOL();

    /// 羊毛地毯标签（所有颜色的地毯方块）
    /// 参考: net.minecraft.tags.BlockTags.WOOL_CARPETS
    /// 运行时消费场景：
    /// 1. DAMPENS_VIBRATIONS 标签的组成项（地毯方块阻尼振动）— 已在 VibrationSystemServer 中消费
    /// 2. COMBINATION_STEP_SOUND_BLOCKS 标签的组成项（组合脚步声）— 待实现
    /// 3. 羊驼装备判定（MC 1.21+ 使用 Equippable 数据组件，非标签判断）— 已在 LlamaEntity::isValidArmorForSlot 中使用
    /// ItemTags::CARPETS
    static BlockTag& WOOL_CARPETS();

    /// 木质栅栏标签（所有木质栅栏，不含下界砖栅栏）
    static BlockTag& WOODEN_FENCES();

    /// 栅栏标签（所有木质栅栏 + 下界砖栅栏）
    static BlockTag& FENCES();

    /// 栅栏门标签
    static BlockTag& FENCE_GATES();

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

    // ========== 蜜蜂相关标签 ==========

    /// 小花朵标签（蒲公英、虞美人等）
    /// 蜜蜂可以采集这些花朵
    static BlockTag& SMALL_FLOWERS();

    /// 高花朵标签（向日葵、丁香等）
    /// 蜜蜂可以采集这些花朵
    static BlockTag& TALL_FLOWERS();

    /// 蜂巢/蜂箱标签
    /// 蜜蜂可以进入的方块
    static BlockTag& BEEHIVES();

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

    /// 水晶声音方块标签（紫水晶块、紫水晶母岩）
    static BlockTag& CRYSTAL_SOUND_BLOCKS();

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

    // ========== 1.21 棘巧试炼 标签 ==========

    /// 不可被特性替换方块标签（基岩、刷怪笼、箱子等）
    static BlockTag& FEATURES_CANNOT_REPLACE();

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
