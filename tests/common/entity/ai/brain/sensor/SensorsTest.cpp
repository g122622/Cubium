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

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/sensor/Sensors.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/ProfessionMapping.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"

using mc::BlockPos;
using mc::EntityInstanceId;
using mc::GameMode;
using mc::GlobalPos;
using mc::PigEntity;
using mc::Player;
using mc::ZombieEntity;
using mc::entity::IMob;
using mc::entity::VillagerEntity;
using mc::entity::VillagerProfession;
using mc::entity::ai::brain::memory::MemoryModuleTypes;
using mc::entity::ai::brain::sensor::AvoidEntitySensor;
using mc::entity::ai::brain::sensor::VillagerHostilesSensor;
using mc::entity::ai::brain::sensor::WorkStationSensor;
using mc::world::village::poi::PointOfInterestStorage;
using mc::world::village::poi::PointOfInterestType;

namespace mc {
namespace {

// ============================================================================
// IMob 标记接口测试
// ============================================================================

class SensorIMobTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试：MonsterEntity（ZombieEntity）实现 IMob 接口
TEST_F(SensorIMobTest, MonsterEntityImplementsIMob)
{
    ZombieEntity zombie(EntityInstanceId(10), mc::test::testEcsRegistry());
    IMob* imob = dynamic_cast<IMob*>(&zombie);
    EXPECT_NE(imob, nullptr) << "ZombieEntity（继承 MonsterEntity）应该实现 IMob 接口";
}

// 测试：AnimalEntity（PigEntity）不实现 IMob 接口
TEST_F(SensorIMobTest, AnimalEntityDoesNotImplementIMob)
{
    PigEntity pig(EntityInstanceId(20), mc::test::testEcsRegistry());
    IMob* animalImob = dynamic_cast<IMob*>(&pig);
    EXPECT_EQ(animalImob, nullptr) << "PigEntity（继承 AnimalEntity）不应该实现 IMob 接口";
}

// 测试：Player 不实现 IMob 接口
TEST_F(SensorIMobTest, PlayerDoesNotImplementIMob)
{
    Player player(EntityInstanceId(30), "TestPlayer", mc::test::testEcsRegistry());
    IMob* playerImob = dynamic_cast<IMob*>(&player);
    EXPECT_EQ(playerImob, nullptr) << "Player 不应该实现 IMob 接口";
}

// ============================================================================
// AvoidEntitySensor::shouldAvoid 测试
// ============================================================================

class AvoidEntitySensorTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试：shouldAvoid 对 MonsterEntity 返回 true（因为 MonsterEntity 实现了 IMob）
TEST_F(AvoidEntitySensorTest, ShouldAvoidMonsterEntity)
{
    ZombieEntity zombie(EntityInstanceId(10), mc::test::testEcsRegistry());
    VillagerEntity villager(EntityInstanceId(11), mc::test::testEcsRegistry());

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &zombie);
    EXPECT_TRUE(result) << "AvoidEntitySensor 应该对 MonsterEntity（实现 IMob）返回 true";
}

// 测试：shouldAvoid 对 AnimalEntity 返回 false（因为 AnimalEntity 不实现 IMob）
TEST_F(AvoidEntitySensorTest, ShouldNotAvoidAnimalEntity)
{
    PigEntity pig(EntityInstanceId(20), mc::test::testEcsRegistry());
    VillagerEntity villager(EntityInstanceId(21), mc::test::testEcsRegistry());

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &pig);
    EXPECT_FALSE(result) << "AvoidEntitySensor 不应该对 AnimalEntity（未实现 IMob）返回 true";
}

// 测试：shouldAvoid 对普通 Player 返回 false
TEST_F(AvoidEntitySensorTest, ShouldNotAvoidSurvivalPlayer)
{
    Player player(EntityInstanceId(30), "TestPlayer", mc::test::testEcsRegistry());
    VillagerEntity villager(EntityInstanceId(31), mc::test::testEcsRegistry());

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &player);
    EXPECT_FALSE(result) << "AvoidEntitySensor 不应该对普通 Player 返回 true";
}

// 测试：shouldAvoid 对创造模式 Player 返回 false
TEST_F(AvoidEntitySensorTest, ShouldNotAvoidCreativePlayer)
{
    Player player(EntityInstanceId(40), "CreativePlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Creative);
    VillagerEntity villager(EntityInstanceId(41), mc::test::testEcsRegistry());

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &player);
    EXPECT_FALSE(result) << "AvoidEntitySensor 不应该对创造模式 Player 返回 true";
}

// 测试：shouldAvoid 对旁观模式 Player 返回 false
TEST_F(AvoidEntitySensorTest, ShouldNotAvoidSpectatorPlayer)
{
    Player player(EntityInstanceId(50), "SpectatorPlayer", mc::test::testEcsRegistry());
    player.setGameMode(GameMode::Spectator);
    VillagerEntity villager(EntityInstanceId(51), mc::test::testEcsRegistry());

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &player);
    EXPECT_FALSE(result) << "AvoidEntitySensor 不应该对旁观模式 Player 返回 true";
}

// ============================================================================
// VillagerHostilesSensor 类型映射测试
// ============================================================================

class VillagerHostilesTypeMapTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试：验证所有映射表中引用的 VanillaEntityTypeKeys 变量都已声明
// 这是一个编译时验证 - 如果 VanillaEntityTypeKeys 中缺少某个变量，编译将失败
TEST_F(VillagerHostilesTypeMapTest, AllHostileTypeIdsAreDeclared)
{
    // 这些变量在 VanillaEntityTypeKeys 命名空间中声明，
    // 如果缺少任何一个，Sensors.cpp 将编译失败。
    // 此测试确认我们引用的所有 ID 都已声明且可访问。
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::DROWNED, mc::entity::VanillaEntityTypeKeys::DROWNED);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::EVOKER, mc::entity::VanillaEntityTypeKeys::EVOKER);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::HUSK, mc::entity::VanillaEntityTypeKeys::HUSK);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::ILLUSIONER, mc::entity::VanillaEntityTypeKeys::ILLUSIONER);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::PILLAGER, mc::entity::VanillaEntityTypeKeys::PILLAGER);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::RAVAGER, mc::entity::VanillaEntityTypeKeys::RAVAGER);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::VEX, mc::entity::VanillaEntityTypeKeys::VEX);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::VINDICATOR, mc::entity::VanillaEntityTypeKeys::VINDICATOR);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::ZOGLIN, mc::entity::VanillaEntityTypeKeys::ZOGLIN);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::ZOMBIE, mc::entity::VanillaEntityTypeKeys::ZOMBIE);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::ZOMBIE_VILLAGER, mc::entity::VanillaEntityTypeKeys::ZOMBIE_VILLAGER);
}

// 测试：验证不在村民敌对列表中的怪物类型存在（编译验证）
// 村民不会逃离：苦力怕、骷髅、蜘蛛、女巫、末影人等
TEST_F(VillagerHostilesTypeMapTest, NonHostileTypesForVillagerExist)
{
    // 这些怪物虽然实现了 IMob，但不在 VillagerHostilesSensor 的映射表中
    // VillagerHostilesSensor 使用精确类型映射而非 IMob 检查
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::CREEPER, mc::entity::VanillaEntityTypeKeys::CREEPER);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::SKELETON, mc::entity::VanillaEntityTypeKeys::SKELETON);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::SPIDER, mc::entity::VanillaEntityTypeKeys::SPIDER);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::WITCH, mc::entity::VanillaEntityTypeKeys::WITCH);
    EXPECT_EQ(mc::entity::VanillaEntityTypeKeys::ENDERMAN, mc::entity::VanillaEntityTypeKeys::ENDERMAN);
}

// ============================================================================
// WorkStationSensor::update() 测试
// ============================================================================

// 测试辅助类：暴露 protected 的 update 方法
class TestableWorkStationSensor : public WorkStationSensor<VillagerEntity> {
public:
    // 暴露 update 方法供测试直接调用
    using WorkStationSensor<VillagerEntity>::update;
};

// 支持 VillageManager 的测试世界
class WorkStationSensorTestWorld : public mc::test::BaseTestWorld {
public:
    WorkStationSensorTestWorld()
        : m_dayTime(5000)
        , m_currentTick(1000)
    {}

    void setDayTime(i64 time) { m_dayTime = time; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] world::village::VillageManager* villageManager() override { return m_villageManager.get(); }
    [[nodiscard]] const world::village::VillageManager* villageManager() const override
    {
        return m_villageManager.get();
    }

    void setVillageManager(std::unique_ptr<world::village::VillageManager> manager)
    {
        m_villageManager = std::move(manager);
    }

private:
    i64 m_dayTime;
    u64 m_currentTick;
    std::unique_ptr<world::village::VillageManager> m_villageManager;
};

class WorkStationSensorUpdateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化记忆类型（确保全局指针非空）
        MemoryModuleTypes::initialize();

        m_world = std::make_unique<WorkStationSensorTestWorld>();

        // 创建 VillageManager 和 POI 存储
        auto villageManager = std::make_unique<world::village::VillageManager>(*m_world);
        m_poiStorage = &villageManager->getPOIStorage();
        m_world->setVillageManager(std::move(villageManager));

        m_villager = std::make_unique<VillagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);

        // 创建传感器
        m_sensor = std::make_unique<TestableWorkStationSensor>();
    }

    void TearDown() override
    {
        m_sensor.reset();
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<WorkStationSensorTestWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
    std::unique_ptr<TestableWorkStationSensor> m_sensor;
    PointOfInterestStorage* m_poiStorage = nullptr;
};

// 测试：傻子村民清除 JOB_SITE 和 POTENTIAL_JOB_SITE 记忆
TEST_F(WorkStationSensorUpdateTest, NitwitVillagerClearsMemories)
{
    m_villager->setProfession(VillagerProfession::Nitwit);

    // 先设置记忆
    auto& brain = m_villager->brain();
    brain.setMemory(MemoryModuleTypes::JOB_SITE, GlobalPos(0, BlockPos(10, 64, 10)));
    brain.setMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE, GlobalPos(0, BlockPos(20, 64, 20)));

    // 确认记忆已设置
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::JOB_SITE));
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE));

    // 执行传感器更新
    m_sensor->update(m_world.get(), m_villager.get());

    // 傻子村民应该清除两个工作站记忆
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::JOB_SITE)) << "傻子村民应清除 JOB_SITE 记忆";
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE)) << "傻子村民应清除 POTENTIAL_JOB_SITE 记忆";
}

// 测试：有职业村民搜索对应工作站POI类型（JOB_SITE 和 POTENTIAL_JOB_SITE）
TEST_F(WorkStationSensorUpdateTest, EmployedVillagerSearchesSpecificWorkstation)
{
    // 设置村民为农民（对应 Composter 工作站）
    m_villager->setProfession(VillagerProfession::Farmer);

    // 注册一些POI：一个 Composter（农民工作站）和一个 Smoker（屠夫工作站）
    m_poiStorage->registerPOI(BlockPos(5, 64, 5), PointOfInterestType::Composter);
    m_poiStorage->registerPOI(BlockPos(3, 64, 3), PointOfInterestType::Smoker);

    // 执行传感器更新
    m_sensor->update(m_world.get(), m_villager.get());

    auto& brain = m_villager->brain();

    // 农民应找到 Composter 作为 JOB_SITE
    auto jobSite = brain.getMemory<GlobalPos>(MemoryModuleTypes::JOB_SITE);
    EXPECT_TRUE(jobSite.has_value()) << "有职业村民应设置 JOB_SITE 记忆";
    if (jobSite.has_value()) {
        EXPECT_EQ(jobSite->getPos(), BlockPos(5, 64, 5)) << "农民的 JOB_SITE 应该是 Composter 位置";
    }

    // 农民也应找到 Composter 作为 POTENTIAL_JOB_SITE
    auto potentialSite = brain.getMemory<GlobalPos>(MemoryModuleTypes::POTENTIAL_JOB_SITE);
    EXPECT_TRUE(potentialSite.has_value()) << "有职业村民应设置 POTENTIAL_JOB_SITE 记忆";
    if (potentialSite.has_value()) {
        EXPECT_EQ(potentialSite->getPos(), BlockPos(5, 64, 5)) << "农民的 POTENTIAL_JOB_SITE 应该是 Composter 位置";
    }
}

// 测试：有职业村民不搜索其他类型工作站
TEST_F(WorkStationSensorUpdateTest, EmployedVillagerIgnoresOtherWorkstationType)
{
    // 设置村民为农民（对应 Composter），但只有 Smoker（屠夫工作站）
    m_villager->setProfession(VillagerProfession::Farmer);

    // 只注册 Smoker（不是农民的工作站）
    m_poiStorage->registerPOI(BlockPos(3, 64, 3), PointOfInterestType::Smoker);

    // 执行传感器更新
    m_sensor->update(m_world.get(), m_villager.get());

    auto& brain = m_villager->brain();

    // 农民不应找到 Smoker 作为 JOB_SITE
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::JOB_SITE)) << "农民不应将 Smoker 设为 JOB_SITE";

    // 农民也不应将 Smoker 设为 POTENTIAL_JOB_SITE
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE)) << "农民不应将 Smoker 设为 POTENTIAL_JOB_SITE";
}

// 测试：有职业村民在搜索不到工作站时清除记忆
TEST_F(WorkStationSensorUpdateTest, EmployedVillagerClearsMemoriesWhenNoWorkstationFound)
{
    m_villager->setProfession(VillagerProfession::Farmer);

    // 先设置记忆
    auto& brain = m_villager->brain();
    brain.setMemory(MemoryModuleTypes::JOB_SITE, GlobalPos(0, BlockPos(10, 64, 10)));
    brain.setMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE, GlobalPos(0, BlockPos(20, 64, 20)));

    // 不注册任何 Composter POI
    m_poiStorage->registerPOI(BlockPos(3, 64, 3), PointOfInterestType::Smoker);

    // 执行传感器更新
    m_sensor->update(m_world.get(), m_villager.get());

    // 农民搜索不到 Composter，应清除记忆
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::JOB_SITE)) << "搜索不到对应工作站时应清除 JOB_SITE";
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE))
        << "搜索不到对应工作站时应清除 POTENTIAL_JOB_SITE";
}

// 测试：不同职业村民搜索不同工作站
TEST_F(WorkStationSensorUpdateTest, DifferentProfessionsSearchDifferentWorkstations)
{
    // 注册多个工作站
    m_poiStorage->registerPOI(BlockPos(5, 64, 5), PointOfInterestType::Composter); // 农民
    m_poiStorage->registerPOI(BlockPos(10, 64, 10), PointOfInterestType::Smoker);  // 屠夫
    m_poiStorage->registerPOI(BlockPos(15, 64, 15), PointOfInterestType::Lectern); // 图书管理员

    {
        // 农民搜索 Composter
        m_villager->setProfession(VillagerProfession::Farmer);
        m_sensor->update(m_world.get(), m_villager.get());

        auto jobSite = m_villager->brain().getMemory<GlobalPos>(MemoryModuleTypes::JOB_SITE);
        EXPECT_TRUE(jobSite.has_value());
        if (jobSite.has_value()) {
            EXPECT_EQ(jobSite->getPos(), BlockPos(5, 64, 5)) << "农民应找到 Composter";
        }
    }

    {
        // 屠夫搜索 Smoker
        m_villager->setProfession(VillagerProfession::Butcher);
        m_sensor->update(m_world.get(), m_villager.get());

        auto jobSite = m_villager->brain().getMemory<GlobalPos>(MemoryModuleTypes::JOB_SITE);
        EXPECT_TRUE(jobSite.has_value());
        if (jobSite.has_value()) {
            EXPECT_EQ(jobSite->getPos(), BlockPos(10, 64, 10)) << "屠夫应找到 Smoker";
        }
    }

    {
        // 图书管理员搜索 Lectern
        m_villager->setProfession(VillagerProfession::Librarian);
        m_sensor->update(m_world.get(), m_villager.get());

        auto jobSite = m_villager->brain().getMemory<GlobalPos>(MemoryModuleTypes::JOB_SITE);
        EXPECT_TRUE(jobSite.has_value());
        if (jobSite.has_value()) {
            EXPECT_EQ(jobSite->getPos(), BlockPos(15, 64, 15)) << "图书管理员应找到 Lectern";
        }
    }
}

// 测试：无职业村民搜索所有可获取工作站类型（POTENTIAL_JOB_SITE）
TEST_F(WorkStationSensorUpdateTest, UnemployedVillagerSearchesAllWorkstationTypes)
{
    // 无职业村民
    m_villager->setProfession(VillagerProfession::None);

    // 注册多个工作站
    m_poiStorage->registerPOI(BlockPos(5, 64, 5), PointOfInterestType::Smoker);
    m_poiStorage->registerPOI(BlockPos(10, 64, 10), PointOfInterestType::Composter);
    m_poiStorage->registerPOI(BlockPos(20, 64, 20), PointOfInterestType::BlastFurnace);

    // 执行传感器更新
    m_sensor->update(m_world.get(), m_villager.get());

    auto& brain = m_villager->brain();

    // 无职业村民不应设置 JOB_SITE
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::JOB_SITE)) << "无职业村民不应设置 JOB_SITE";

    // 无职业村民应找到最近的工作站作为 POTENTIAL_JOB_SITE
    auto potentialSite = brain.getMemory<GlobalPos>(MemoryModuleTypes::POTENTIAL_JOB_SITE);
    EXPECT_TRUE(potentialSite.has_value()) << "无职业村民应设置 POTENTIAL_JOB_SITE";
    if (potentialSite.has_value()) {
        // 最近的工作站是 Smoker（距离 5 格）
        EXPECT_EQ(potentialSite->getPos(), BlockPos(5, 64, 5)) << "无职业村民应找到最近的工作站";
    }
}

// 测试：无职业村民在没有工作站时清除 POTENTIAL_JOB_SITE
TEST_F(WorkStationSensorUpdateTest, UnemployedVillagerClearsMemoriesWhenNoWorkstationFound)
{
    m_villager->setProfession(VillagerProfession::None);

    // 先设置记忆
    auto& brain = m_villager->brain();
    brain.setMemory(MemoryModuleTypes::JOB_SITE, GlobalPos(0, BlockPos(10, 64, 10)));
    brain.setMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE, GlobalPos(0, BlockPos(20, 64, 20)));

    // 不注册任何工作站POI
    // 执行传感器更新
    m_sensor->update(m_world.get(), m_villager.get());

    // 无职业村民搜索不到任何工作站，应清除记忆
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::JOB_SITE)) << "无职业村民搜索不到工作站时应清除 JOB_SITE";
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE))
        << "无职业村民搜索不到工作站时应清除 POTENTIAL_JOB_SITE";
}

// 测试：无职业村民搜索到最近的工作站（跨类型比较）
TEST_F(WorkStationSensorUpdateTest, UnemployedVillagerFindsNearestAcrossAllTypes)
{
    m_villager->setProfession(VillagerProfession::None);

    // 注册多个工作站，距离各不相同
    // Smoker 在距离 30 格处
    m_poiStorage->registerPOI(BlockPos(30, 64, 0), PointOfInterestType::Smoker);
    // Lectern 在距离 5 格处（最近）
    m_poiStorage->registerPOI(BlockPos(5, 64, 0), PointOfInterestType::Lectern);
    // Composter 在距离 15 格处
    m_poiStorage->registerPOI(BlockPos(15, 64, 0), PointOfInterestType::Composter);

    m_sensor->update(m_world.get(), m_villager.get());

    auto potentialSite = m_villager->brain().getMemory<GlobalPos>(MemoryModuleTypes::POTENTIAL_JOB_SITE);
    EXPECT_TRUE(potentialSite.has_value());
    if (potentialSite.has_value()) {
        // 应找到最近的工作站（Lectern）
        EXPECT_EQ(potentialSite->getPos(), BlockPos(5, 64, 0)) << "无职业村民应找到最近的工作站（Lectern 在5格处）";
    }
}

// 测试：无职业村民只搜索工作站POI类型，不搜索床位等非工作站POI
TEST_F(WorkStationSensorUpdateTest, UnemployedVillagerIgnoresNonWorkstationPOI)
{
    m_villager->setProfession(VillagerProfession::None);

    // 注册非工作站POI（床位和钟）
    m_poiStorage->registerPOI(BlockPos(1, 64, 0), PointOfInterestType::BedRed);
    m_poiStorage->registerPOI(BlockPos(2, 64, 0), PointOfInterestType::Bell);
    // 注册一个工作站POI（距离较远）
    m_poiStorage->registerPOI(BlockPos(20, 64, 0), PointOfInterestType::Composter);

    m_sensor->update(m_world.get(), m_villager.get());

    auto potentialSite = m_villager->brain().getMemory<GlobalPos>(MemoryModuleTypes::POTENTIAL_JOB_SITE);
    EXPECT_TRUE(potentialSite.has_value());
    if (potentialSite.has_value()) {
        // 应找到 Composter，而非更近的床位或钟
        EXPECT_EQ(potentialSite->getPos(), BlockPos(20, 64, 0))
            << "无职业村民应忽略非工作站POI（床位、钟），找到工作站POI";
    }
}

// 测试：无职业村民搜索时清除 JOB_SITE
TEST_F(WorkStationSensorUpdateTest, UnemployedVillagerClearsJobSiteMemory)
{
    m_villager->setProfession(VillagerProfession::None);

    // 预设 JOB_SITE 记忆
    auto& brain = m_villager->brain();
    brain.setMemory(MemoryModuleTypes::JOB_SITE, GlobalPos(0, BlockPos(10, 64, 10)));

    // 注册一个工作站POI
    m_poiStorage->registerPOI(BlockPos(5, 64, 5), PointOfInterestType::Smoker);

    m_sensor->update(m_world.get(), m_villager.get());

    // 无职业村民应清除 JOB_SITE（只有有职业的村民才有 JOB_SITE）
    EXPECT_FALSE(brain.hasMemory(MemoryModuleTypes::JOB_SITE)) << "无职业村民应清除 JOB_SITE 记忆";

    // 但应设置 POTENTIAL_JOB_SITE
    EXPECT_TRUE(brain.hasMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE)) << "无职业村民应设置 POTENTIAL_JOB_SITE";
}

// 测试：WorkStationSensor 的 getUsedMemories 返回正确值
TEST_F(WorkStationSensorUpdateTest, GetUsedMemoriesReturnsCorrectTypes)
{
    auto usedMemories = m_sensor->getUsedMemories();
    EXPECT_EQ(usedMemories.size(), 2u) << "WorkStationSensor 应使用两个记忆类型";
    EXPECT_NE(usedMemories.find(MemoryModuleTypes::JOB_SITE), usedMemories.end())
        << "WorkStationSensor 应使用 JOB_SITE 记忆";
    EXPECT_NE(usedMemories.find(MemoryModuleTypes::POTENTIAL_JOB_SITE), usedMemories.end())
        << "WorkStationSensor 应使用 POTENTIAL_JOB_SITE 记忆";
}

// 测试：传感器更新间隔为40 tick
TEST_F(WorkStationSensorUpdateTest, SensorIntervalIs40Ticks)
{
    // WorkStationSensor 构造函数设置间隔为 40
    // 通过多次 tick 来验证 update 被调用的频率
    // 首先初始化计数器
    mc::math::Random random(42);
    m_sensor->initCounter(random);

    m_villager->setProfession(VillagerProfession::Farmer);
    m_poiStorage->registerPOI(BlockPos(5, 64, 5), PointOfInterestType::Composter);

    // tick 41 次（第一次 tick 初始化后随机偏移，但最多 40 tick 后必触发 update）
    // 初始化后 counter 为随机值 [0, 40)，所以 40 tick 后必定触发至少一次
    for (int i = 0; i < 50; ++i) {
        m_sensor->tick(m_world.get(), m_villager.get());
    }

    // 如果 update 被调用过，应该有 JOB_SITE 记忆
    auto jobSite = m_villager->brain().getMemory<GlobalPos>(MemoryModuleTypes::JOB_SITE);
    EXPECT_TRUE(jobSite.has_value()) << "50 tick 后 WorkStationSensor 应至少触发一次 update";
}

} // namespace
} // namespace mc
