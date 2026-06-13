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

#include "common/core/Types.hpp"
#include "common/entity/ai/brain/sensor/Sensors.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/interfaces/IMob.hpp"

using mc::EntityId;
using mc::GameMode;
using mc::PigEntity;
using mc::Player;
using mc::ZombieEntity;
using mc::entity::IMob;
using mc::entity::VillagerEntity;
using mc::entity::ai::brain::sensor::AvoidEntitySensor;
using mc::entity::ai::brain::sensor::VillagerHostilesSensor;

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
    ZombieEntity zombie(EntityId(10));
    IMob* imob = dynamic_cast<IMob*>(&zombie);
    EXPECT_NE(imob, nullptr) << "ZombieEntity（继承 MonsterEntity）应该实现 IMob 接口";
}

// 测试：AnimalEntity（PigEntity）不实现 IMob 接口
TEST_F(SensorIMobTest, AnimalEntityDoesNotImplementIMob)
{
    PigEntity pig(EntityId(20));
    IMob* animalImob = dynamic_cast<IMob*>(&pig);
    EXPECT_EQ(animalImob, nullptr) << "PigEntity（继承 AnimalEntity）不应该实现 IMob 接口";
}

// 测试：Player 不实现 IMob 接口
TEST_F(SensorIMobTest, PlayerDoesNotImplementIMob)
{
    Player player(EntityId(30), "TestPlayer");
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
    ZombieEntity zombie(EntityId(10));
    VillagerEntity villager(EntityId(11));

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &zombie);
    EXPECT_TRUE(result) << "AvoidEntitySensor 应该对 MonsterEntity（实现 IMob）返回 true";
}

// 测试：shouldAvoid 对 AnimalEntity 返回 false（因为 AnimalEntity 不实现 IMob）
TEST_F(AvoidEntitySensorTest, ShouldNotAvoidAnimalEntity)
{
    PigEntity pig(EntityId(20));
    VillagerEntity villager(EntityId(21));

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &pig);
    EXPECT_FALSE(result) << "AvoidEntitySensor 不应该对 AnimalEntity（未实现 IMob）返回 true";
}

// 测试：shouldAvoid 对普通 Player 返回 false
TEST_F(AvoidEntitySensorTest, ShouldNotAvoidSurvivalPlayer)
{
    Player player(EntityId(30), "TestPlayer");
    VillagerEntity villager(EntityId(31));

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &player);
    EXPECT_FALSE(result) << "AvoidEntitySensor 不应该对普通 Player 返回 true";
}

// 测试：shouldAvoid 对创造模式 Player 返回 false
TEST_F(AvoidEntitySensorTest, ShouldNotAvoidCreativePlayer)
{
    Player player(EntityId(40), "CreativePlayer");
    player.setGameMode(GameMode::Creative);
    VillagerEntity villager(EntityId(41));

    bool result = AvoidEntitySensor<VillagerEntity>::shouldAvoid(&villager, &player);
    EXPECT_FALSE(result) << "AvoidEntitySensor 不应该对创造模式 Player 返回 true";
}

// 测试：shouldAvoid 对旁观模式 Player 返回 false
TEST_F(AvoidEntitySensorTest, ShouldNotAvoidSpectatorPlayer)
{
    Player player(EntityId(50), "SpectatorPlayer");
    player.setGameMode(GameMode::Spectator);
    VillagerEntity villager(EntityId(51));

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

// 测试：验证所有映射表中引用的 EntityTypeIdNumber 变量都已声明
// 这是一个编译时验证 - 如果 EntityTypeIdNumber 中缺少某个变量，编译将失败
TEST_F(VillagerHostilesTypeMapTest, AllHostileTypeIdsAreDeclared)
{
    // 这些变量在 EntityTypeIdNumber 命名空间中声明，
    // 如果缺少任何一个，Sensors.cpp 将编译失败。
    // 此测试确认我们引用的所有 ID 都已声明且可访问。
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::DROWNED, mc::entity::EntityTypeIdNumber::DROWNED);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::EVOKER, mc::entity::EntityTypeIdNumber::EVOKER);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::HUSK, mc::entity::EntityTypeIdNumber::HUSK);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::ILLUSIONER, mc::entity::EntityTypeIdNumber::ILLUSIONER);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::PILLAGER, mc::entity::EntityTypeIdNumber::PILLAGER);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::RAVAGER, mc::entity::EntityTypeIdNumber::RAVAGER);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::VEX, mc::entity::EntityTypeIdNumber::VEX);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::VINDICATOR, mc::entity::EntityTypeIdNumber::VINDICATOR);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::ZOGLIN, mc::entity::EntityTypeIdNumber::ZOGLIN);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::ZOMBIE, mc::entity::EntityTypeIdNumber::ZOMBIE);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::ZOMBIE_VILLAGER, mc::entity::EntityTypeIdNumber::ZOMBIE_VILLAGER);
}

// 测试：验证不在村民敌对列表中的怪物类型存在（编译验证）
// 村民不会逃离：苦力怕、骷髅、蜘蛛、女巫、末影人等
TEST_F(VillagerHostilesTypeMapTest, NonHostileTypesForVillagerExist)
{
    // 这些怪物虽然实现了 IMob，但不在 VillagerHostilesSensor 的映射表中
    // VillagerHostilesSensor 使用精确类型映射而非 IMob 检查
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::CREEPER, mc::entity::EntityTypeIdNumber::CREEPER);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::SKELETON, mc::entity::EntityTypeIdNumber::SKELETON);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::SPIDER, mc::entity::EntityTypeIdNumber::SPIDER);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::WITCH, mc::entity::EntityTypeIdNumber::WITCH);
    EXPECT_EQ(mc::entity::EntityTypeIdNumber::ENDERMAN, mc::entity::EntityTypeIdNumber::ENDERMAN);
}

} // namespace
} // namespace mc
