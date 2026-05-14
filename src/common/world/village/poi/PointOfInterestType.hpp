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

#include "../../../core/Types.hpp"
#include <string>

namespace mc {
namespace world {
namespace village {
namespace poi {

/**
 * @brief POI（兴趣点）类型枚举
 *
 * 定义世界中可作为兴趣点的方块类型，包括床位、工作站和其他特殊方块。
 * 每种工作站类型对应一种村民职业。
 *
 * 参考 MC 1.16.5 PointOfInterestType
 */
enum class PointOfInterestType : u16 {
    // ========== 床位类型（用于村民睡眠和重生） ==========
    BedRed = 0,
    BedBlack,
    BedBlue,
    BedBrown,
    BedCyan,
    BedGray,
    BedGreen,
    BedLightBlue,
    BedLightGray,
    BedLime,
    BedMagenta,
    BedOrange,
    BedPink,
    BedPurple,
    BedWhite,
    BedYellow,

    // ========== 工作站类型（与村民职业对应） ==========
    /// 烟熏炉 - 屠夫(Butcher)
    Smoker,
    /// 高炉 - 盔甲匠(Armorer)
    BlastFurnace,
    /// 制图台 - 制图师(Cartographer)
    CartographyTable,
    /// 酿造台 - 牧师(Cleric)
    BrewingStand,
    /// 堆肥桶 - 农民(Farmer)
    Composter,
    /// 木桶 - 渔夫(Fisherman)
    Barrel,
    /// 制箭台 - 制箭师(Fletcher)
    FletchingTable,
    /// 炼药锅 - 皮革匠(Leatherworker)
    Cauldron,
    /// 讲台 - 图书管理员(Librarian)
    Lectern,
    /// 切石机 - 石匠(Mason)
    Stonecutter,
    /// 锻造台 - 工具匠(Toolsmith) / 武器匠(Weaponsmith)
    SmithingTable,
    /// 织布机 - 牧羊人(Shepherd)
    Loom,

    // ========== 其他POI类型 ==========
    /// 钟 - 村庄聚集点/会议点
    Bell,
    /// 下界传送门
    NetherPortal,
    /// 磁石
    Lodestone,
    /// 避雷针
    LightningRod,

    // ========== 特殊值 ==========
    /// 无效/未知类型
    None = 0xFFFF
};

/**
 * @brief POI类型工具类
 *
 * 提供POI类型的查询和转换方法
 */
class POITypeHelper {
public:
    /**
     * @brief 获取POI类型名称
     * @param type POI类型
     * @return 类型名称字符串
     */
    [[nodiscard]] static const char* getName(PointOfInterestType type);

    /**
     * @brief 检查是否为床位类型
     * @param type POI类型
     * @return 是否为床位
     */
    [[nodiscard]] static bool isBed(PointOfInterestType type);

    /**
     * @brief 检查是否为工作站类型
     * @param type POI类型
     * @return 是否为工作站
     */
    [[nodiscard]] static bool isWorkstation(PointOfInterestType type);

    /**
     * @brief 获取工作站对应的村民职业
     * @param type 工作站类型（必须是工作站类型）
     * @return 对应的村民职业，如果不是工作站返回None
     */
    [[nodiscard]] static PointOfInterestType getProfessionForWorkstation(PointOfInterestType type);

    /**
     * @brief 从方块ID获取POI类型
     * @param blockId 方块ID
     * @return 对应的POI类型，如果没有对应返回None
     */
    [[nodiscard]] static PointOfInterestType fromBlockId(u32 blockId);

    /**
     * @brief 获取POI类型的最大占用票据数
     * @param type POI类型
     * @return 最大票据数
     *
     * 床位通常为1，工作站为1，钟可以多个村民共享
     */
    [[nodiscard]] static i32 getMaxTickets(PointOfInterestType type);

    /**
     * @brief 获取POI类型的搜索范围
     * @param type POI类型
     * @return 村民搜索此类型POI的范围（方块）
     */
    [[nodiscard]] static f32 getSearchRange(PointOfInterestType type);
};

} // namespace poi
} // namespace village
} // namespace world
} // namespace mc
