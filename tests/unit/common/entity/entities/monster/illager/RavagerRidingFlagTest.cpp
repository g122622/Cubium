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

// RAIDERS 标签 Ravager 骑乘门控修复测试。
//
// 验证 RavagerEntity::updateMovementGoalFlags()（RavagerEntity.cpp）对齐 vanilla 1.21.11
// Ravager.updateControlFlags（Ravager.java:88-96）：
//   boolean flag = !(getControllingPassenger() instanceof Mob)
//                  || getControllingPassenger().getType().is(EntityTypeTags.RAIDERS);
//   goalSelector.setControlFlag(MOVE, flag);
//   goalSelector.setControlFlag(JUMP, flag && !(getVehicle() instanceof AbstractBoat));
//   goalSelector.setControlFlag(LOOK, flag);
//   goalSelector.setControlFlag(TARGET, flag);
//
// 此前缺陷：RavagerEntity 未重写 updateMovementGoalFlags，落到基类 MobEntity 的
// `entityType() != PLAYER` 硬编码——灾厄村民（RAIDERS 标签成员）骑 Ravager 时 controllingIsMob=true
// → canMove=false → Ravager 的 MOVE/JUMP/LOOK flag 全关，Ravager 停摆不自主寻路，与 vanilla
// 相反（vanilla 灾厄村民骑乘时 flag=true，Ravager 保持自主 AI，骑手仅随乘）。改查 RAIDERS 标签
// 对齐 vanilla 析取语义。
//
// 测试设计（4 例，含正反对照）：
//   - RaiderRidingKeepsMoveFlag：掠夺者（RAIDERS 标签内）骑 Ravager → MOVE/LOOK/TARGET flag 保持开
//     （isFlagDisabled==false），对齐 vanilla（修复前 flag 被关 = isFlagDisabled==true）
//   - NonRaiderMobRidingDisablesMoveFlag：僵尸（非 RAIDERS 的 Mob）骑 Ravager → MOVE flag 关
//     （isFlagDisabled==true），对齐 vanilla 非 Raider Mob 骑 Mob 载具停摆
//   - PlayerRidingKeepsMoveFlag：玩家骑 Ravager → MOVE flag 保持开（玩家不接管 Ravager AI）
//   - NoPassengerKeepsMoveFlag：无乘客 → MOVE flag 保持开（Ravager 自主行动）
//
// 骑乘建立：passenger->startRiding(*ravager) 经 Entity::addPassenger 把 passenger.id 塞进
// ravager.m_passengers，getControllingPassenger() 返回首名乘客 id。测试世界重写 getEntity 按 id
// 查回实体指针（updateMovementGoalFlags 经 m_world->getEntity 解引用控制乘客）。
//
// Ref: vanilla Ravager.java:88-96（updateControlFlags 析取 RAIDERS 标签）
// Ref: vanilla Mob.java:333-339（基类 updateControlFlags 仅 !instanceof Mob）
// Ref: RavagerEntity.cpp（updateMovementGoalFlags 重写查 RAIDERS + 设 TARGET flag）
// Ref: EntityTypeTags.cpp:553-560（RAIDERS 成员含 pillager/evoker/vindicator/illusioner/witch/ravager）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/entities/monster/illager/IllagerEntities.hpp"
#include "common/entity/entities/monster/illager/RavagerEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/world/IWorld.hpp"

#include <map>
#include <memory>
#include <vector>

namespace mc {
namespace {

// 支持实体注册和按 id 查找的测试世界。
// updateMovementGoalFlags 经 m_world->getEntity(controllingId) 解引用控制乘客，BaseTestWorld 默认
// getEntity 返 nullptr，故须重写以查回骑乘者实体。
class RavagerRidingTestWorld final : public mc::test::BaseTestWorld {
public:
    RavagerRidingTestWorld() = default;

    void addTestEntity(Entity* entity) { m_testEntities[entity->id()] = entity; }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_testEntities.find(id);
        return it != m_testEntities.end() ? it->second : nullptr;
    }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_testEntities.find(id);
        return it != m_testEntities.end() ? it->second : nullptr;
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        EntityInstanceId id = entity->id();
        m_testEntities[id] = entity.get();
        m_ownedEntities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override { throw std::runtime_error("not implemented"); }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("not implemented");
    }

private:
    std::map<EntityInstanceId, Entity*> m_testEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
};

} // namespace

class RavagerRidingFlagTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 注册所有实体类型以使 VanillaEntityTypeKeys 常量有效（entityType() 查表）。
        entity::VanillaEntities::registerAll();
        // 实体类型标签初始化（进程级幂等）。RAIDERS 成员集在 initialize() 注册。
        EntityTypeTags::initialize();
    }

    void SetUp() override { m_world = std::make_unique<RavagerRidingTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<RavagerRidingTestWorld> m_world;
};

// 掠夺者（RAIDERS 标签内）骑 Ravager → Ravager 的 MOVE/LOOK/TARGET flag 保持开（对齐 vanilla）。
//
// vanilla Ravager.updateControlFlags:90：控制乘客为 Mob 且在 RAIDERS 标签内 → flag=true。
// 修复前：基类硬编码 !=PLAYER → controllingIsMob=true → canMove=false → MOVE flag 关（停摆）。
// 修复后：查 RAIDERS 标签命中 pillager → canMove=true → flag 保持开。
TEST_F(RavagerRidingFlagTest, RaiderRidingKeepsMoveFlag)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());
    ravager->setTypeId("minecraft:ravager");

    auto pillager = std::make_unique<PillagerEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    pillager->setWorld(m_world.get());
    pillager->setTypeId("minecraft:pillager");

    m_world->addTestEntity(ravager.get());
    m_world->addTestEntity(pillager.get());

    // 掠夺者骑上劫掠兽（成为控制乘客）
    ASSERT_TRUE(pillager->startRiding(*ravager));
    ASSERT_EQ(ravager->getControllingPassenger(), pillager->id());

    // 触发 flag 更新（对齐 tick 中每 5 tick 调用，此处直接调）
    ravager->updateMovementGoalFlags();

    // RAIDERS 标签内 → flag 保持开（isFlagDisabled==false）
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Jump));
    // 对齐 vanilla Ravager.updateControlFlags 设 TARGET flag（基类未设，Ravager 重写补齐）
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Target));
}

// 僵尸（非 RAIDERS 标签的 Mob）骑 Ravager → Ravager 的 MOVE flag 关（对齐 vanilla 非 Raider Mob 停摆）。
//
// vanilla：控制乘客为 Mob 且不在 RAIDERS 标签 → flag=false（载具 AI 关停交骑手控制）。
// 此场景对齐"骷髅骑马"式 Mob 骑 Mob：载具停摆。修复前后均 flag=false（对照组，验证未误伤）。
TEST_F(RavagerRidingFlagTest, NonRaiderMobRidingDisablesMoveFlag)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());
    ravager->setTypeId("minecraft:ravager");

    auto zombie = std::make_unique<ZombieEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    zombie->setWorld(m_world.get());
    zombie->setTypeId("minecraft:zombie");

    m_world->addTestEntity(ravager.get());
    m_world->addTestEntity(zombie.get());

    ASSERT_TRUE(zombie->startRiding(*ravager));
    ASSERT_EQ(ravager->getControllingPassenger(), zombie->id());

    ravager->updateMovementGoalFlags();

    // 非 RAIDERS 的 Mob 骑乘 → flag 关（isFlagDisabled==true）
    EXPECT_TRUE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Look));
    EXPECT_TRUE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Target));
}

// 玩家骑 Ravager → MOVE flag 保持开（玩家不接管 Ravager AI，对齐 vanilla）。
//
// vanilla：控制乘客为 Player（非 Mob）→ !instanceof Mob=true → flag=true。
// 修复前后均 flag=true（对照组，验证玩家骑乘未回归）。
TEST_F(RavagerRidingFlagTest, PlayerRidingKeepsMoveFlag)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());
    ravager->setTypeId("minecraft:ravager");

    auto player = std::make_unique<Player>(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(m_world.get());
    player->setTypeId("minecraft:player");

    m_world->addTestEntity(ravager.get());
    m_world->addTestEntity(player.get());

    ASSERT_TRUE(player->startRiding(*ravager));
    ASSERT_EQ(ravager->getControllingPassenger(), player->id());

    ravager->updateMovementGoalFlags();

    // 玩家骑乘 → flag 保持开
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Look));
}

// 无乘客 → MOVE flag 保持开（Ravager 自主行动，对齐 vanilla）。
TEST_F(RavagerRidingFlagTest, NoPassengerKeepsMoveFlag)
{
    auto ravager = std::make_unique<RavagerEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    ravager->setWorld(m_world.get());
    ravager->setTypeId("minecraft:ravager");

    m_world->addTestEntity(ravager.get());

    ASSERT_EQ(ravager->getControllingPassenger(), INVALID_ENTITY_ID);

    ravager->updateMovementGoalFlags();

    // 无乘客 → flag 保持开
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(ravager->goalSelector().isFlagDisabled(entity::ai::GoalFlag::Target));
}

} // namespace mc
