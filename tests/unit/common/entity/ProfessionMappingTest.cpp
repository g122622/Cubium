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

#include <gtest/gtest.h>

#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/entities/villager/ProfessionMapping.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::villager;
using namespace mc::world::village::poi;

// ============================================================================
// ProfessionMapping::getWorkstationPOI 测试
// ============================================================================

class ProfessionMappingGetWorkstationTest : public ::testing::Test {
protected:
    // 映射应在首次调用时延迟初始化
};

TEST_F(ProfessionMappingGetWorkstationTest, ArmorerMapsToBlastFurnace)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Armorer), PointOfInterestType::BlastFurnace);
}

TEST_F(ProfessionMappingGetWorkstationTest, ButcherMapsToSmoker)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Butcher), PointOfInterestType::Smoker);
}

TEST_F(ProfessionMappingGetWorkstationTest, CartographerMapsToCartographyTable)
{
    EXPECT_EQ(
        ProfessionMapping::getWorkstationPOI(VillagerProfession::Cartographer), PointOfInterestType::CartographyTable);
}

TEST_F(ProfessionMappingGetWorkstationTest, ClericMapsToBrewingStand)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Cleric), PointOfInterestType::BrewingStand);
}

TEST_F(ProfessionMappingGetWorkstationTest, FarmerMapsToComposter)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Farmer), PointOfInterestType::Composter);
}

TEST_F(ProfessionMappingGetWorkstationTest, FishermanMapsToBarrel)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Fisherman), PointOfInterestType::Barrel);
}

TEST_F(ProfessionMappingGetWorkstationTest, FletcherMapsToFletchingTable)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Fletcher), PointOfInterestType::FletchingTable);
}

TEST_F(ProfessionMappingGetWorkstationTest, LeatherworkerMapsToCauldron)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Leatherworker), PointOfInterestType::Cauldron);
}

TEST_F(ProfessionMappingGetWorkstationTest, LibrarianMapsToLectern)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Librarian), PointOfInterestType::Lectern);
}

TEST_F(ProfessionMappingGetWorkstationTest, MasonMapsToStonecutter)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Mason), PointOfInterestType::Stonecutter);
}

TEST_F(ProfessionMappingGetWorkstationTest, ShepherdMapsToLoom)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Shepherd), PointOfInterestType::Loom);
}

TEST_F(ProfessionMappingGetWorkstationTest, ToolsmithMapsToSmithingTable)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Toolsmith), PointOfInterestType::SmithingTable);
}

TEST_F(ProfessionMappingGetWorkstationTest, WeaponsmithMapsToSmithingTable)
{
    EXPECT_EQ(
        ProfessionMapping::getWorkstationPOI(VillagerProfession::Weaponsmith), PointOfInterestType::SmithingTable);
}

TEST_F(ProfessionMappingGetWorkstationTest, NoneReturnsNone)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::None), PointOfInterestType::None);
}

TEST_F(ProfessionMappingGetWorkstationTest, NitwitReturnsNone)
{
    EXPECT_EQ(ProfessionMapping::getWorkstationPOI(VillagerProfession::Nitwit), PointOfInterestType::None);
}

// ============================================================================
// ProfessionMapping::getProfessionFromPOI 测试
// ============================================================================

class ProfessionMappingGetProfessionTest : public ::testing::Test {
protected:
    // 测试 POI 到职业的反向映射
};

TEST_F(ProfessionMappingGetProfessionTest, SmokerMapsToButcher)
{
    EXPECT_EQ(ProfessionMapping::getProfessionFromPOI(PointOfInterestType::Smoker), VillagerProfession::Butcher);
}

TEST_F(ProfessionMappingGetProfessionTest, BlastFurnaceMapsToArmorer)
{
    EXPECT_EQ(ProfessionMapping::getProfessionFromPOI(PointOfInterestType::BlastFurnace), VillagerProfession::Armorer);
}

TEST_F(ProfessionMappingGetProfessionTest, ComposterMapsToFarmer)
{
    EXPECT_EQ(ProfessionMapping::getProfessionFromPOI(PointOfInterestType::Composter), VillagerProfession::Farmer);
}

TEST_F(ProfessionMappingGetProfessionTest, NonWorkstationReturnsNone)
{
    // 床位不是工作站，应返回 None
    EXPECT_EQ(ProfessionMapping::getProfessionFromPOI(PointOfInterestType::BedRed), VillagerProfession::None);
    // 钟不是工作站，应返回 None
    EXPECT_EQ(ProfessionMapping::getProfessionFromPOI(PointOfInterestType::Bell), VillagerProfession::None);
}

// ============================================================================
// ProfessionMapping::hasWorkstation 测试
// ============================================================================

class ProfessionMappingHasWorkstationTest : public ::testing::Test {
protected:
    // 测试是否有工作站
};

TEST_F(ProfessionMappingHasWorkstationTest, NoneHasNoWorkstation)
{
    EXPECT_FALSE(ProfessionMapping::hasWorkstation(VillagerProfession::None));
}

TEST_F(ProfessionMappingHasWorkstationTest, NitwitHasNoWorkstation)
{
    EXPECT_FALSE(ProfessionMapping::hasWorkstation(VillagerProfession::Nitwit));
}

TEST_F(ProfessionMappingHasWorkstationTest, ArmorerHasWorkstation)
{
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Armorer));
}

TEST_F(ProfessionMappingHasWorkstationTest, AllProfessionsWithWorkstation)
{
    // 所有有工作站的职业
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Armorer));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Butcher));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Cartographer));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Cleric));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Farmer));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Fisherman));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Fletcher));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Leatherworker));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Librarian));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Mason));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Shepherd));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Toolsmith));
    EXPECT_TRUE(ProfessionMapping::hasWorkstation(VillagerProfession::Weaponsmith));
}

// ============================================================================
// ProfessionMapping::isValidProfession 测试
// ============================================================================

TEST_F(ProfessionMappingHasWorkstationTest, NoneIsNotValidProfession)
{
    EXPECT_FALSE(ProfessionMapping::isValidProfession(VillagerProfession::None));
}

TEST_F(ProfessionMappingHasWorkstationTest, NitwitIsValidProfession)
{
    // Nitwit 是有效职业，只是没有工作站
    EXPECT_TRUE(ProfessionMapping::isValidProfession(VillagerProfession::Nitwit));
}

TEST_F(ProfessionMappingHasWorkstationTest, FarmerIsValidProfession)
{
    EXPECT_TRUE(ProfessionMapping::isValidProfession(VillagerProfession::Farmer));
}

// ============================================================================
// ProfessionMapping::getAcquirableWorkstations 测试
// ============================================================================

class ProfessionMappingAcquirableWorkstationsTest : public ::testing::Test {
protected:
    // 测试可获取工作站列表
};

TEST_F(ProfessionMappingAcquirableWorkstationsTest, ListNotEmpty)
{
    const auto& workstations = ProfessionMapping::getAcquirableWorkstations();
    EXPECT_FALSE(workstations.empty()) << "可获取工作站列表不应为空";
}

TEST_F(ProfessionMappingAcquirableWorkstationsTest, ListContains12Workstations)
{
    // 12 种工作站POI类型（对应13种有工作站的职业，其中Toolsmith和Weaponsmith共享SmithingTable）
    const auto& workstations = ProfessionMapping::getAcquirableWorkstations();
    EXPECT_EQ(workstations.size(), 12u) << "应该有12种工作站POI类型";
}

TEST_F(ProfessionMappingAcquirableWorkstationsTest, ListContainsAllWorkstationTypes)
{
    const auto& workstations = ProfessionMapping::getAcquirableWorkstations();

    // 验证列表包含所有工作站类型
    EXPECT_NE(std::find(workstations.begin(), workstations.end(), PointOfInterestType::Smoker), workstations.end());
    EXPECT_NE(
        std::find(workstations.begin(), workstations.end(), PointOfInterestType::BlastFurnace), workstations.end());
    EXPECT_NE(
        std::find(workstations.begin(), workstations.end(), PointOfInterestType::CartographyTable), workstations.end());
    EXPECT_NE(
        std::find(workstations.begin(), workstations.end(), PointOfInterestType::BrewingStand), workstations.end());
    EXPECT_NE(std::find(workstations.begin(), workstations.end(), PointOfInterestType::Composter), workstations.end());
    EXPECT_NE(std::find(workstations.begin(), workstations.end(), PointOfInterestType::Barrel), workstations.end());
    EXPECT_NE(
        std::find(workstations.begin(), workstations.end(), PointOfInterestType::FletchingTable), workstations.end());
    EXPECT_NE(std::find(workstations.begin(), workstations.end(), PointOfInterestType::Cauldron), workstations.end());
    EXPECT_NE(std::find(workstations.begin(), workstations.end(), PointOfInterestType::Lectern), workstations.end());
    EXPECT_NE(
        std::find(workstations.begin(), workstations.end(), PointOfInterestType::Stonecutter), workstations.end());
    EXPECT_NE(
        std::find(workstations.begin(), workstations.end(), PointOfInterestType::SmithingTable), workstations.end());
    EXPECT_NE(std::find(workstations.begin(), workstations.end(), PointOfInterestType::Loom), workstations.end());
}

TEST_F(ProfessionMappingAcquirableWorkstationsTest, ListDoesNotContainNonWorkstations)
{
    const auto& workstations = ProfessionMapping::getAcquirableWorkstations();

    // 不应包含非工作站类型
    EXPECT_EQ(std::find(workstations.begin(), workstations.end(), PointOfInterestType::BedRed), workstations.end());
    EXPECT_EQ(std::find(workstations.begin(), workstations.end(), PointOfInterestType::Bell), workstations.end());
    EXPECT_EQ(
        std::find(workstations.begin(), workstations.end(), PointOfInterestType::NetherPortal), workstations.end());
    EXPECT_EQ(std::find(workstations.begin(), workstations.end(), PointOfInterestType::None), workstations.end());
}

TEST_F(ProfessionMappingAcquirableWorkstationsTest, AllWorkstationsAreWorkstationType)
{
    // 验证列表中所有类型都通过 isWorkstation 检查
    const auto& workstations = ProfessionMapping::getAcquirableWorkstations();
    for (auto wsType : workstations) {
        EXPECT_TRUE(POITypeHelper::isWorkstation(wsType))
            << "POI type " << static_cast<int>(wsType) << " should be a workstation type";
    }
}

TEST_F(ProfessionMappingAcquirableWorkstationsTest, EachWorkstationMapsToValidProfession)
{
    // 验证列表中每个工作站都能映射到有效职业
    const auto& workstations = ProfessionMapping::getAcquirableWorkstations();
    for (auto wsType : workstations) {
        VillagerProfession profession = ProfessionMapping::getProfessionFromPOI(wsType);
        EXPECT_TRUE(ProfessionMapping::isValidProfession(profession))
            << "Workstation POI type " << static_cast<int>(wsType) << " should map to a valid profession";
    }
}

// ============================================================================
// ProfessionMapping::getProfessionName 测试
// ============================================================================

TEST_F(ProfessionMappingHasWorkstationTest, GetProfessionNameReturnsCorrectStrings)
{
    EXPECT_STREQ(ProfessionMapping::getProfessionName(VillagerProfession::None), "none");
    EXPECT_STREQ(ProfessionMapping::getProfessionName(VillagerProfession::Armorer), "armorer");
    EXPECT_STREQ(ProfessionMapping::getProfessionName(VillagerProfession::Butcher), "butcher");
    EXPECT_STREQ(ProfessionMapping::getProfessionName(VillagerProfession::Farmer), "farmer");
    EXPECT_STREQ(ProfessionMapping::getProfessionName(VillagerProfession::Nitwit), "nitwit");
}

TEST_F(ProfessionMappingHasWorkstationTest, GetProfessionFromNameRoundTrip)
{
    // 测试 name -> profession -> name 往返
    EXPECT_EQ(ProfessionMapping::getProfessionFromName("armorer"), VillagerProfession::Armorer);
    EXPECT_EQ(ProfessionMapping::getProfessionFromName("butcher"), VillagerProfession::Butcher);
    EXPECT_EQ(ProfessionMapping::getProfessionFromName("farmer"), VillagerProfession::Farmer);
    EXPECT_EQ(ProfessionMapping::getProfessionFromName("none"), VillagerProfession::None);
    EXPECT_EQ(ProfessionMapping::getProfessionFromName("nitwit"), VillagerProfession::Nitwit);
    EXPECT_EQ(ProfessionMapping::getProfessionFromName("unknown"), VillagerProfession::None);
    EXPECT_EQ(ProfessionMapping::getProfessionFromName(nullptr), VillagerProfession::None);
}
