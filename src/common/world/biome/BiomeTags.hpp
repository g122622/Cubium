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

#include "BiomeTag.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace mc::world::biome {

/**
 * @brief 内置生物群系标签集合
 *
 * 包含所有原版 has_structure 生物群系标签，
 * 用于判断某个生物群系是否可以生成特定结构。
 *
 * 使用方法：
 * @code
 * // 初始化（在 BiomeRegistry::initialize() 之后调用）
 * BiomeTags::initialize();
 *
 * // 检查生物群系是否可以生成平原村庄
 * if (BiomeTags::HAS_STRUCTURE_VILLAGE_PLAINS().contains(biomeId)) {
 *     // 生成平原村庄
 * }
 *
 // 便捷方法
 * if (BiomeTags::hasStructure(biomeId, ResourceLocation("minecraft", "has_structure/village_plains"))) {
 *     // 生成平原村庄
 * }
 * @endcode
 */
class BiomeTags {
public:
    // ========== 村庄标签 ==========

    /// 沙漠村庄标签
    static BiomeTag& HAS_STRUCTURE_VILLAGE_DESERT();
    /// 平原村庄标签
    static BiomeTag& HAS_STRUCTURE_VILLAGE_PLAINS();
    /// 热带草原村庄标签
    static BiomeTag& HAS_STRUCTURE_VILLAGE_SAVANNA();
    /// 雪地村庄标签
    static BiomeTag& HAS_STRUCTURE_VILLAGE_SNOWY();
    /// 针叶林村庄标签
    static BiomeTag& HAS_STRUCTURE_VILLAGE_TAIGA();

    // ========== 下界结构标签 ==========

    /// 堡垒遗迹标签
    static BiomeTag& HAS_STRUCTURE_BASTION_REMNANT();
    /// 下界要塞标签
    static BiomeTag& HAS_STRUCTURE_FORTRESS();
    /// 下界化石标签
    static BiomeTag& HAS_STRUCTURE_NETHER_FOSSIL();

    // ========== 主世界结构标签 ==========

    /// 埋藏宝藏标签
    static BiomeTag& HAS_STRUCTURE_BURIED_TREASURE();
    /// 沙漠神殿标签
    static BiomeTag& HAS_STRUCTURE_DESERT_PYRAMID();
    /// 末地城标签
    static BiomeTag& HAS_STRUCTURE_END_CITY();
    /// 雪屋标签
    static BiomeTag& HAS_STRUCTURE_IGLOO();
    /// 丛林神殿标签
    static BiomeTag& HAS_STRUCTURE_JUNGLE_PYRAMID();
    /// 林地 Mansion 标签
    static BiomeTag& HAS_STRUCTURE_MANSION();
    /// 矿井标签
    static BiomeTag& HAS_STRUCTURE_MINESHAFT();
    /// 恶地矿井标签
    static BiomeTag& HAS_STRUCTURE_MINESHAFT_MESA();
    /// 海底神殿标签
    static BiomeTag& HAS_STRUCTURE_MONUMENT();
    /// 冷水海底废墟标签
    static BiomeTag& HAS_STRUCTURE_OCEAN_RUIN_COLD();
    /// 温水海底废墟标签
    static BiomeTag& HAS_STRUCTURE_OCEAN_RUIN_WARM();
    /// 掠夺者前哨站标签
    static BiomeTag& HAS_STRUCTURE_PILLAGER_OUTPOST();

    // ========== 废弃传送门标签（按生物群系分类）==========

    /// 废弃传送门（沙漠）标签
    static BiomeTag& HAS_STRUCTURE_RUINED_PORTAL_DESERT();
    /// 废弃传送门（丛林）标签
    static BiomeTag& HAS_STRUCTURE_RUINED_PORTAL_JUNGLE();
    /// 废弃传送门（山地）标签
    static BiomeTag& HAS_STRUCTURE_RUINED_PORTAL_MOUNTAIN();
    /// 废弃传送门（下界）标签
    static BiomeTag& HAS_STRUCTURE_RUINED_PORTAL_NETHER();
    /// 废弃传送门（海洋）标签
    static BiomeTag& HAS_STRUCTURE_RUINED_PORTAL_OCEAN();
    /// 废弃传送门（标准）标签
    static BiomeTag& HAS_STRUCTURE_RUINED_PORTAL_STANDARD();
    /// 废弃传送门（沼泽）标签
    static BiomeTag& HAS_STRUCTURE_RUINED_PORTAL_SWAMP();

    // ========== 船只结构标签 ==========

    /// 沉船标签
    static BiomeTag& HAS_STRUCTURE_SHIPWRECK();
    /// 搁浅沉船标签
    static BiomeTag& HAS_STRUCTURE_SHIPWRECK_BEACHED();

    // ========== 其他结构标签 ==========

    /// 要塞标签
    static BiomeTag& HAS_STRUCTURE_STRONGHOLD();
    /// 女巫小屋标签
    static BiomeTag& HAS_STRUCTURE_SWAMP_HUT();
    /// 古迹废墟标签
    static BiomeTag& HAS_STRUCTURE_TRAIL_RUINS();
    /// 试炼密室标签
    static BiomeTag& HAS_STRUCTURE_TRIAL_CHAMBERS();
    /// 远古城市标签
    static BiomeTag& HAS_STRUCTURE_ANCIENT_CITY();

    // ========== 游戏玩法标签 ==========

    /// 允许地表史莱姆生成的生物群系（沼泽、红树林沼泽）
    static BiomeTag& ALLOWS_SURFACE_SLIME_SPAWNS();

    // ========== 生物群系类型标签 ==========

    /// 海洋生物群系标签（#minecraft:is_ocean）
    static BiomeTag& IS_OCEAN();
    /// 河流生物群系标签（#minecraft:is_river）
    static BiomeTag& IS_RIVER();

    // ========== 维度标签 ==========

    /// 主世界生物群系标签（#minecraft:is_overworld）
    static BiomeTag& IS_OVERWORLD();
    /// 下界生物群系标签（#minecraft:is_nether）
    static BiomeTag& IS_NETHER();
    /// 末地生物群系标签（#minecraft:is_end）
    static BiomeTag& IS_END();

    // ========== 地形类型标签 ==========

    /// 深海生物群系标签（#minecraft:is_deep_ocean）
    static BiomeTag& IS_DEEP_OCEAN();
    /// 海滩生物群系标签（#minecraft:is_beach）
    static BiomeTag& IS_BEACH();
    /// 山地生物群系标签（#minecraft:is_mountain）
    static BiomeTag& IS_MOUNTAIN();
    /// 丘陵生物群系标签（#minecraft:is_hill）
    static BiomeTag& IS_HILL();
    /// 针叶林生物群系标签（#minecraft:is_taiga）
    static BiomeTag& IS_TAIGA();
    /// 丛林生物群系标签（#minecraft:is_jungle）
    static BiomeTag& IS_JUNGLE();
    /// 森林生物群系标签（#minecraft:is_forest）
    static BiomeTag& IS_FOREST();
    /// 热带草原生物群系标签（#minecraft:is_savanna）
    static BiomeTag& IS_SAVANNA();
    /// 恶地生物群系标签（#minecraft:is_badlands）
    static BiomeTag& IS_BADLANDS();
    /// 蘑菇岛生物群系标签（#minecraft:is_mushroom）
    static BiomeTag& IS_MUSHROOM();

    // ========== 更多游戏玩法标签 ==========

    /// 生成寒冷变体青蛙的生物群系
    static BiomeTag& SPAWNS_COLD_VARIANT_FROGS();
    /// 生成温暖变体青蛙的生物群系
    static BiomeTag& SPAWNS_WARM_VARIANT_FROGS();
    /// 无僵尸围城的生物群系
    static BiomeTag& WITHOUT_ZOMBIE_SIEGES();
    /// 无流浪商人生成的生物群系
    static BiomeTag& WITHOUT_WANDERING_TRADER_SPAWNS();
    /// 无巡逻队生成的生物群系
    static BiomeTag& WITHOUT_PATROL_SPAWNS();
    /// 偏向要塞生成的生物群系
    static BiomeTag& STRONGHOLD_BIASED_TO();
    /// 需要海底神殿周围生成的生物群系
    static BiomeTag& REQUIRED_OCEAN_MONUMENT_SURROUNDING();
    /// 阻止矿井生成的生物群系
    static BiomeTag& MINESHAFT_BLOCKING();
    /// 地图上显示水体的生物群系
    static BiomeTag& WATER_ON_MAP_OUTLINES();
    /// 骨粉可以生成珊瑚的生物群系
    static BiomeTag& PRODUCES_CORALS_FROM_BONEMEAL();
    /// 虚空生物群系标签
    static BiomeTag& IS_VOID();

    /**
     * @brief 初始化所有内置标签
     *
     * 在 BiomeRegistry::initialize() 之后调用。线程安全：内部用 std::call_once 保证
     * 全进程只执行一次，多 worker 并发首次调用时只有一个线程填充 _getTags()，其余阻塞等待。
     * 仍应在区块生成 worker 启动前调用一次，避免首个区块生成时多 worker 竞争 call_once 造成的串行化。
     */
    static void initialize();

    /**
     * @brief 根据ID获取标签
     *
     * @param id 标签资源位置
     * @return 标签指针，如果不存在返回 nullptr
     */
    [[nodiscard]] static BiomeTag* getTag(const ResourceLocation& id);

    /**
     * @brief 检查生物群系是否在指定结构标签中
     *
     * @param biomeId 生物群系ID
     * @param tagId 标签资源位置
     * @return 是否在标签中
     */
    [[nodiscard]] static bool hasStructure(BiomeId biomeId, const ResourceLocation& tagId);

    /**
     * @brief 遍历所有标签
     */
    static void forEachTag(std::function<void(const BiomeTag&)> callback);

private:
    BiomeTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<BiomeTag>>& _getTags();
    static std::once_flag& _getInitOnce();
};

} // namespace mc::world::biome

// 旧命名空间兼容别名
namespace mc {
using BiomeTags = ::mc::world::biome::BiomeTags;
} // namespace mc
