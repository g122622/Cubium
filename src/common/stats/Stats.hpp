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

#include "common/resource/ResourceLocation.hpp"

namespace mc {
namespace stats {

/**
 * @brief 自定义统计常量
 *
 * 定义 Minecraft 1.16.5 中所有自定义统计的资源位置常量，
 * 对应 MC Java 版 Stats 类中的 Custom 统计字段。
 * 这些统计在 StatRegistry 中注册，通过 StatisticsManager::incrementCustom() 增量。
 *
 * 命名规则：与 MC Java 版 Stats.java 中的常量名一致，使用 UPPER_SNAKE_CASE。
 * 资源位置格式：minecraft:{snake_case_name}
 */

// ========== 容器交互统计 ==========

/// 打开木桶
inline constexpr const char* OPEN_BARREL = "minecraft:open_barrel";
/// 打开箱子
inline constexpr const char* OPEN_CHEST = "minecraft:open_chest";
/// 打开末影箱
inline constexpr const char* OPEN_ENDERCHEST = "minecraft:open_enderchest";
/// 打开潜影盒
inline constexpr const char* OPEN_SHULKER_BOX = "minecraft:open_shulker_box";

// ========== 方块交互统计 ==========

/// 与铁砧交互
inline constexpr const char* INTERACT_WITH_ANVIL = "minecraft:interact_with_anvil";
/// 与信标交互
inline constexpr const char* INTERACT_WITH_BEACON = "minecraft:interact_with_beacon";
/// 与高炉交互
inline constexpr const char* INTERACT_WITH_BLAST_FURNACE = "minecraft:interact_with_blast_furnace";
/// 与酿造台交互
inline constexpr const char* INTERACT_WITH_BREWINGSTAND = "minecraft:interact_with_brewingstand";
/// 与营火交互
inline constexpr const char* INTERACT_WITH_CAMPFIRE = "minecraft:interact_with_campfire";
/// 与制图台交互
inline constexpr const char* INTERACT_WITH_CARTOGRAPHY_TABLE = "minecraft:interact_with_cartography_table";
/// 与合成台交互
inline constexpr const char* INTERACT_WITH_CRAFTING_TABLE = "minecraft:interact_with_crafting_table";
/// 与堆肥桶交互
inline constexpr const char* INTERACT_WITH_COMPOSTER = "minecraft:interact_with_composter";
/// 与熔炉交互
inline constexpr const char* INTERACT_WITH_FURNACE = "minecraft:interact_with_furnace";
/// 与砂轮交互
inline constexpr const char* INTERACT_WITH_GRINDSTONE = "minecraft:interact_with_grindstone";
/// 与讲台交互
inline constexpr const char* INTERACT_WITH_LECTERN = "minecraft:interact_with_lectern";
/// 与织布机交互
inline constexpr const char* INTERACT_WITH_LOOM = "minecraft:interact_with_loom";
/// 与烟熏炉交互
inline constexpr const char* INTERACT_WITH_SMOKER = "minecraft:interact_with_smoker";
/// 与锻造台交互
inline constexpr const char* INTERACT_WITH_SMITHING_TABLE = "minecraft:interact_with_smithing_table";
/// 与切石机交互
inline constexpr const char* INTERACT_WITH_STONECUTTER = "minecraft:interact_with_stonecutter";

// ========== 物品使用统计 ==========

/// 末影珍珠使用次数
inline constexpr const char* ENDER_PEARLS_USED = "minecraft:ender_pearls_used";
/// 烟花火箭使用次数
inline constexpr const char* FIREWORK_ROCKET_USED = "minecraft:firework_rocket_used";

// ========== 其他统计 ==========

/// 敲钟
inline constexpr const char* BELL_RING = "minecraft:bell_ring";
/// 填充炼药锅
inline constexpr const char* FILL_CAULDRON = "minecraft:fill_cauldron";
/// 使用炼药锅
inline constexpr const char* USE_CAULDRON = "minecraft:use_cauldron";
/// 清洗盔甲
inline constexpr const char* CLEAN_ARMOR = "minecraft:clean_armor";
/// 清洗旗帜
inline constexpr const char* CLEAN_BANNER = "minecraft:clean_banner";
/// 清洗潜影盒
inline constexpr const char* CLEAN_SHULKER_BOX = "minecraft:clean_shulker_box";
/// 播放唱片
inline constexpr const char* RECORD_PLAYED = "minecraft:record_played";
/// 演奏音符盒
inline constexpr const char* NOTEBLOCK_PLAYED = "minecraft:noteblock_played";
/// 调整音符盒
inline constexpr const char* NOTEBLOCK_TUNED = "minecraft:noteblock_tuned";
/// 种花盆
inline constexpr const char* FLOWER_POTTED = "minecraft:flower_potted";
/// 触发陷阱箱
inline constexpr const char* TRAPPED_CHEST_TRIGGERED = "minecraft:trapped_chest_triggered";
/// 检查发射器
inline constexpr const char* INSPECT_DISPENSER = "minecraft:inspect_dispenser";
/// 检查投掷器
inline constexpr const char* INSPECT_DROPPER = "minecraft:inspect_dropper";
/// 检查漏斗
inline constexpr const char* INSPECT_HOPPER = "minecraft:inspect_hopper";
/// 信标交互（旧名称，与 INTERACT_WITH_BEACON 相同）
inline constexpr const char* BEACON_INTERACTION = "minecraft:beacon_interaction";

// ========== 游戏事件统计 ==========

/// 触发袭击
inline constexpr const char* RAID_TRIGGER = "minecraft:raid_trigger";
/// 赢得袭击
inline constexpr const char* RAID_WIN = "minecraft:raid_win";
/// 在床上睡觉
inline constexpr const char* SLEEP_IN_BED = "minecraft:sleep_in_bed";
/// 击中标靶
inline constexpr const char* TARGET_HIT = "minecraft:target_hit";
/// 离开游戏
inline constexpr const char* LEAVE_GAME = "minecraft:leave_game";

// ========== 时间/距离统计 ==========

/// 自上次死亡以来的时间
inline constexpr const char* TIME_SINCE_DEATH = "minecraft:time_since_death";
/// 自上次休息以来的时间
inline constexpr const char* TIME_SINCE_REST = "minecraft:time_since_rest";
/// 潜行时间
inline constexpr const char* SNEAK_TIME = "minecraft:sneak_time";
/// 水下行走距离（厘米）
inline constexpr const char* WALK_UNDER_WATER_ONE_CM = "minecraft:walk_under_water_one_cm";
/// 水上行走距离（厘米）
inline constexpr const char* WALK_ON_WATER_ONE_CM = "minecraft:walk_on_water_one_cm";
/// 船行距离（厘米）
inline constexpr const char* BOAT_ONE_CM = "minecraft:boat_one_cm";
/// 鞘翅飞行距离（厘米）
inline constexpr const char* AVIATE_ONE_CM = "minecraft:aviate_one_cm";
/// 骑马距离（厘米）
inline constexpr const char* HORSE_ONE_CM = "minecraft:horse_one_cm";
/// 骑羊驼距离（厘米）
inline constexpr const char* LLAMA_ONE_CM = "minecraft:llama_one_cm";
/// 骑猪距离（厘米）
inline constexpr const char* PIG_ONE_CM = "minecraft:pig_one_cm";
/// 矿车距离（厘米）
inline constexpr const char* MINECART_ONE_CM = "minecraft:minecart_one_cm";

} // namespace stats
} // namespace mc
