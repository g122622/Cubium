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

#include "AbstractVillagerEntity.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "core/Types.hpp"
#include "world/village/poi/PointOfInterestType.hpp"
#include <unordered_map>
#include <vector>

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
 */
class ProfessionMapping {
public:
    /**
     * @brief 获取职业对应的工作站POI类型
     * @param profession 村民职业
     * @return 对应的工作站POI类型，如果无职业返回None
     */
    [[nodiscard]] static world::village::poi::PointOfInterestType getWorkstationPOI(VillagerProfession profession);

    /**
     * @brief 从工作站POI类型获取职业
     * @param poiType 工作站POI类型
     * @return 对应的村民职业，如果不是工作站返回None
     */
    [[nodiscard]] static VillagerProfession getProfessionFromPOI(world::village::poi::PointOfInterestType poiType);

    /**
     * @brief 检查职业是否有效（非None且非Nitwit）
     * @param profession 村民职业
     * @return 是否有效
     */
    [[nodiscard]] static bool isValidProfession(VillagerProfession profession) noexcept;

    /**
     * @brief 检查职业是否有对应的工作站
     * @param profession 村民职业
     * @return 是否有工作站
     *
     * 注意：None和Nitwit没有工作站
     */
    [[nodiscard]] static bool hasWorkstation(VillagerProfession profession) noexcept;

    /**
     * @brief 获取职业名称
     * @param profession 村民职业
     * @return 职业名称字符串
     */
    [[nodiscard]] static const char* getProfessionName(VillagerProfession profession) noexcept;

    /**
     * @brief 从名称获取职业
     * @param name 职业名称
     * @return 村民职业，如果不存在返回None
     */
    [[nodiscard]] static VillagerProfession getProfessionFromName(const char* name) noexcept;

    /**
     * @brief 获取职业的最大等级
     * @param profession 村民职业
     * @return 最大等级（通常为5）
     */
    [[nodiscard]] static i32 getMaxLevel(VillagerProfession profession) noexcept;

    /**
     * @brief 获取升级所需经验
     * @param level 当前等级（1-5）
     * @return 升级所需经验
     */
    [[nodiscard]] static i32 getExperienceForLevel(i32 level) noexcept;

    /**
     * @brief 获取所有可获取的工作站POI类型列表
     *
     * 无职业村民使用此列表搜索工作站。对应MC原版的 acquirableJobSite 标签。
     * @return 所有人类可获取的工作站POI类型列表
     */
    [[nodiscard]] static const std::vector<world::village::poi::PointOfInterestType>& getAcquirableWorkstations();

private:
    // 初始化映射表
    static void _initializeMappings();

    static bool s_initialized;
    static std::unordered_map<VillagerProfession, world::village::poi::PointOfInterestType> s_professionToPOI;
    static std::unordered_map<world::village::poi::PointOfInterestType, VillagerProfession> s_poiToProfession;
    static std::vector<world::village::poi::PointOfInterestType> s_acquirableWorkstations;
};

} // namespace villager
} // namespace entity
} // namespace mc
