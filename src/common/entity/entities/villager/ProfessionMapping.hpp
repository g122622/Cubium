#pragma once

#include "../../../core/Types.hpp"
#include "../../../world/village/poi/PointOfInterestType.hpp"
#include "AbstractVillagerEntity.hpp"
#include <unordered_map>

namespace mc {
namespace entity {
namespace villager {

/**
 * @brief 职业映射工具类
 *
 * 提供村民职业(VillagerProfession)、工作站(Workstation/PointOfInterestType)
 * 之间的统一映射关系。
 *
 * 统一说明：
 * - VillagerProfession: 村民的职业类型，决定交易列表和外观
 * - PointOfInterestType: POI类型，包含工作站定义，是世界中的兴趣点
 * - Workstation枚举已废弃，统一使用PointOfInterestType中的工作站类型
 *
 * 参考 MC 1.16.5 VillagerProfession + PointOfInterestType
 */
class ProfessionMapping {
public:
    /**
     * @brief 获取职业对应的工作站POI类型
     * @param profession 村民职业
     * @return 对应的工作站POI类型，如果无职业返回None
     */
    [[nodiscard]] static world::village::poi::PointOfInterestType
    getWorkstationPOI(VillagerProfession profession);

    /**
     * @brief 从工作站POI类型获取职业
     * @param poiType 工作站POI类型
     * @return 对应的村民职业，如果不是工作站返回None
     */
    [[nodiscard]] static VillagerProfession
    getProfessionFromPOI(world::village::poi::PointOfInterestType poiType);

    /**
     * @brief 检查职业是否有效（非None且非Nitwit）
     * @param profession 村民职业
     * @return 是否有效
     */
    [[nodiscard]] static bool isValidProfession(VillagerProfession profession);

    /**
     * @brief 检查职业是否有对应的工作站
     * @param profession 村民职业
     * @return 是否有工作站
     *
     * 注意：None和Nitwit没有工作站
     */
    [[nodiscard]] static bool hasWorkstation(VillagerProfession profession);

    /**
     * @brief 获取职业名称
     * @param profession 村民职业
     * @return 职业名称字符串
     */
    [[nodiscard]] static const char* getProfessionName(VillagerProfession profession);

    /**
     * @brief 从名称获取职业
     * @param name 职业名称
     * @return 村民职业，如果不存在返回None
     */
    [[nodiscard]] static VillagerProfession getProfessionFromName(const char* name);

    /**
     * @brief 获取职业的最大等级
     * @param profession 村民职业
     * @return 最大等级（通常为5）
     */
    [[nodiscard]] static i32 getMaxLevel(VillagerProfession profession);

    /**
     * @brief 获取升级所需经验
     * @param level 当前等级（1-5）
     * @return 升级所需经验
     */
    [[nodiscard]] static i32 getExperienceForLevel(i32 level);

private:
    // 初始化映射表
    static void initializeMappings();

    static bool s_initialized;
    static std::unordered_map<VillagerProfession, world::village::poi::PointOfInterestType> s_professionToPOI;
    static std::unordered_map<world::village::poi::PointOfInterestType, VillagerProfession> s_poiToProfession;
};

} // namespace villager
} // namespace entity
} // namespace mc
