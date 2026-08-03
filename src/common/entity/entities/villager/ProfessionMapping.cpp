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

#include "ProfessionMapping.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"

#include <cstring>
#include <unordered_map>
#include <vector>

namespace mc {
namespace entity {
namespace villager {

// 静态成员初始化
bool ProfessionMapping::s_initialized = false;
std::unordered_map<VillagerProfession, world::village::poi::PointOfInterestType> ProfessionMapping::s_professionToPOI;
std::unordered_map<world::village::poi::PointOfInterestType, VillagerProfession> ProfessionMapping::s_poiToProfession;
std::vector<world::village::poi::PointOfInterestType> ProfessionMapping::s_acquirableWorkstations;

void ProfessionMapping::_initializeMappings()
{
    if (s_initialized) return;
    s_initialized = true;

    // 职业到POI工作站的映射
    s_professionToPOI[VillagerProfession::Armorer] = world::village::poi::PointOfInterestType::BlastFurnace;
    s_professionToPOI[VillagerProfession::Butcher] = world::village::poi::PointOfInterestType::Smoker;
    s_professionToPOI[VillagerProfession::Cartographer] = world::village::poi::PointOfInterestType::CartographyTable;
    s_professionToPOI[VillagerProfession::Cleric] = world::village::poi::PointOfInterestType::BrewingStand;
    s_professionToPOI[VillagerProfession::Farmer] = world::village::poi::PointOfInterestType::Composter;
    s_professionToPOI[VillagerProfession::Fisherman] = world::village::poi::PointOfInterestType::Barrel;
    s_professionToPOI[VillagerProfession::Fletcher] = world::village::poi::PointOfInterestType::FletchingTable;
    s_professionToPOI[VillagerProfession::Leatherworker] = world::village::poi::PointOfInterestType::Cauldron;
    s_professionToPOI[VillagerProfession::Librarian] = world::village::poi::PointOfInterestType::Lectern;
    s_professionToPOI[VillagerProfession::Mason] = world::village::poi::PointOfInterestType::Stonecutter;
    s_professionToPOI[VillagerProfession::Shepherd] = world::village::poi::PointOfInterestType::Loom;
    s_professionToPOI[VillagerProfession::Toolsmith] = world::village::poi::PointOfInterestType::SmithingTable;
    s_professionToPOI[VillagerProfession::Weaponsmith] = world::village::poi::PointOfInterestType::SmithingTable;
    // None 和 Nitwit 没有工作站

    // 反向映射：POI工作站到职业
    for (const auto& [profession, poi] : s_professionToPOI) {
        s_poiToProfession[poi] = profession;
    }

    // 初始化可获取工作站列表（无职业村民可搜索的所有工作站POI类型）
    // 对应 MC 原版 PoiTypeTags.ACQUIRABLE_JOB_SITE
    s_acquirableWorkstations = {
        world::village::poi::PointOfInterestType::Smoker,
        world::village::poi::PointOfInterestType::BlastFurnace,
        world::village::poi::PointOfInterestType::CartographyTable,
        world::village::poi::PointOfInterestType::BrewingStand,
        world::village::poi::PointOfInterestType::Composter,
        world::village::poi::PointOfInterestType::Barrel,
        world::village::poi::PointOfInterestType::FletchingTable,
        world::village::poi::PointOfInterestType::Cauldron,
        world::village::poi::PointOfInterestType::Lectern,
        world::village::poi::PointOfInterestType::Stonecutter,
        world::village::poi::PointOfInterestType::SmithingTable,
        world::village::poi::PointOfInterestType::Loom,
    };
}

world::village::poi::PointOfInterestType ProfessionMapping::getWorkstationPOI(VillagerProfession profession)
{
    _initializeMappings();

    auto it = s_professionToPOI.find(profession);
    if (it != s_professionToPOI.end()) {
        return it->second;
    }
    return world::village::poi::PointOfInterestType::None;
}

VillagerProfession ProfessionMapping::getProfessionFromPOI(world::village::poi::PointOfInterestType poiType)
{
    _initializeMappings();

    auto it = s_poiToProfession.find(poiType);
    if (it != s_poiToProfession.end()) {
        return it->second;
    }
    return VillagerProfession::None;
}

bool ProfessionMapping::isValidProfession(VillagerProfession profession) noexcept
{
    return profession != VillagerProfession::None;
}

bool ProfessionMapping::hasWorkstation(VillagerProfession profession) noexcept
{
    // None和Nitwit没有工作站
    if (profession == VillagerProfession::None || profession == VillagerProfession::Nitwit) {
        return false;
    }
    return isValidProfession(profession);
}

const char* ProfessionMapping::getProfessionName(VillagerProfession profession) noexcept
{
    switch (profession) {
        case VillagerProfession::None:
            return "none";
        case VillagerProfession::Armorer:
            return "armorer";
        case VillagerProfession::Butcher:
            return "butcher";
        case VillagerProfession::Cartographer:
            return "cartographer";
        case VillagerProfession::Cleric:
            return "cleric";
        case VillagerProfession::Farmer:
            return "farmer";
        case VillagerProfession::Fisherman:
            return "fisherman";
        case VillagerProfession::Fletcher:
            return "fletcher";
        case VillagerProfession::Leatherworker:
            return "leatherworker";
        case VillagerProfession::Librarian:
            return "librarian";
        case VillagerProfession::Mason:
            return "mason";
        case VillagerProfession::Nitwit:
            return "nitwit";
        case VillagerProfession::Shepherd:
            return "shepherd";
        case VillagerProfession::Toolsmith:
            return "toolsmith";
        case VillagerProfession::Weaponsmith:
            return "weaponsmith";
        default:
            return "none";
    }
}

VillagerProfession ProfessionMapping::getProfessionFromName(const char* name) noexcept
{
    if (name == nullptr) return VillagerProfession::None;

    // 使用字符串比较
    if (strcmp(name, "armorer") == 0) return VillagerProfession::Armorer;
    if (strcmp(name, "butcher") == 0) return VillagerProfession::Butcher;
    if (strcmp(name, "cartographer") == 0) return VillagerProfession::Cartographer;
    if (strcmp(name, "cleric") == 0) return VillagerProfession::Cleric;
    if (strcmp(name, "farmer") == 0) return VillagerProfession::Farmer;
    if (strcmp(name, "fisherman") == 0) return VillagerProfession::Fisherman;
    if (strcmp(name, "fletcher") == 0) return VillagerProfession::Fletcher;
    if (strcmp(name, "leatherworker") == 0) return VillagerProfession::Leatherworker;
    if (strcmp(name, "librarian") == 0) return VillagerProfession::Librarian;
    if (strcmp(name, "mason") == 0) return VillagerProfession::Mason;
    if (strcmp(name, "nitwit") == 0) return VillagerProfession::Nitwit;
    if (strcmp(name, "shepherd") == 0) return VillagerProfession::Shepherd;
    if (strcmp(name, "toolsmith") == 0) return VillagerProfession::Toolsmith;
    if (strcmp(name, "weaponsmith") == 0) return VillagerProfession::Weaponsmith;

    return VillagerProfession::None;
}

i32 ProfessionMapping::getMaxLevel(VillagerProfession /*profession*/) noexcept
{
    // 所有职业最大等级都是5
    return 5;
}

i32 ProfessionMapping::getExperienceForLevel(i32 level) noexcept
{
    // 升级经验需求
    // 等级 1->2: 10 经验
    // 等级 2->3: 70 经验
    // 等级 3->4: 150 经验
    // 等级 4->5: 250 经验
    switch (level) {
        case 1:
            return 10;
        case 2:
            return 70;
        case 3:
            return 150;
        case 4:
            return 250;
        default:
            return 0;
    }
}

const std::vector<world::village::poi::PointOfInterestType>& ProfessionMapping::getAcquirableWorkstations()
{
    _initializeMappings();
    return s_acquirableWorkstations;
}

} // namespace villager
} // namespace entity
} // namespace mc
