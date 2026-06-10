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

#include <functional>
#include <memory>
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

    /**
     * @brief 初始化所有内置标签
     *
     * 在 BiomeRegistry::initialize() 之后调用
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
    static bool s_initialized;
};

} // namespace mc::world::biome
