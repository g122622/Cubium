/**
 * @brief 船内生物的 JUMP flag 门控测试
 *
 * 对齐 vanilla Mob.updateControlFlags（Mob.java:333-339）：
 *   flag  = !(getControllingPassenger() instanceof Mob)
 *   flag1 = !(getVehicle() instanceof AbstractBoat)
 *   MOVE/LOOK 用 flag，JUMP 用 flag && flag1。
 * 即生物骑乘船时无论是否有控制乘客，JUMP flag 都应被禁用。
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/world/IWorld.hpp"
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

/**
 * @brief 记录载具实体的测试世界
 *
 * startRiding 走世界查询读取载具，故测试世界必须能按实例 id 返回已登记实体。
 */
class RidingTestWorld final : public mc::test::BaseTestWorld {
public:
    void track(Entity& entity) { m_entities[entity.id()] = &entity; }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        const auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

private:
    std::unordered_map<u32, Entity*> m_entities;
};

class MobRidingBoatJumpFlagTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个用例使用独立 registry：共享的 testEcsRegistry 会跨用例累积实体，
        // 与显式指定的实例 id 冲突。
        m_registry = std::make_unique<ecs::EntityRegistry>(std::string("MobRidingBoatJumpFlagTest"));
        m_mob = std::make_unique<PigEntity>(EntityInstanceId(1), *m_registry);
        m_boat = std::make_unique<BoatEntity>(BoatEntity::Type::OAK, *m_registry);
        // 部分实体构造器固定传 EntityInstanceId(0)，此处显式赋互不相同的实例 id
        // （0 是 INVALID_ENTITY_ID，载具与乘客不可同为 0）。
        m_mob->setId(EntityInstanceId(1));
        m_boat->setId(EntityInstanceId(2));

        m_world = std::make_unique<RidingTestWorld>();
        m_world->track(*m_boat);
        m_mob->setWorld(m_world.get());
        m_boat->setWorld(m_world.get());
    }

    void TearDown() override
    {
        m_boat.reset();
        m_mob.reset();
    }

    [[nodiscard]] bool jumpDisabled() const { return m_mob->goalSelector().isFlagDisabled(ai::GoalFlag::Jump); }

    [[nodiscard]] bool moveDisabled() const { return m_mob->goalSelector().isFlagDisabled(ai::GoalFlag::Move); }

    std::unique_ptr<ecs::EntityRegistry> m_registry;
    std::unique_ptr<RidingTestWorld> m_world;
    std::unique_ptr<PigEntity> m_mob;
    std::unique_ptr<BoatEntity> m_boat;
};

TEST_F(MobRidingBoatJumpFlagTest, NotRiding_JumpAndMoveEnabled)
{
    m_mob->updateMovementGoalFlags();
    EXPECT_FALSE(jumpDisabled());
    EXPECT_FALSE(moveDisabled());
}

TEST_F(MobRidingBoatJumpFlagTest, RidingBoatWithoutControllingPassenger_JumpDisabled)
{
    EXPECT_TRUE(m_boat->couldAcceptPassenger());
    EXPECT_TRUE(m_boat->canBeRidden(*m_mob));
    EXPECT_TRUE(m_boat->canAddPassenger(*m_mob));
    ASSERT_TRUE(m_mob->startRiding(*m_boat));
    ASSERT_TRUE(m_mob->isRiding());
    ASSERT_EQ(m_mob->getVehicle(), m_boat->id());

    EXPECT_EQ(m_mob->getVehicle(), m_boat->id());
    EXPECT_NE(m_world->getEntity(m_mob->getVehicle()), nullptr);

    m_mob->updateMovementGoalFlags();

    // 船特例：即使没有控制乘客，JUMP 也被禁用；MOVE/LOOK 不受影响。
    EXPECT_TRUE(jumpDisabled());
    EXPECT_FALSE(moveDisabled());
}

TEST_F(MobRidingBoatJumpFlagTest, RidingNonBoat_JumpRemainsEnabled)
{
    PigEntity otherPig(EntityInstanceId(3), *m_registry);
    otherPig.setId(EntityInstanceId(3));
    otherPig.setWorld(m_world.get());
    m_world->track(otherPig);
    ASSERT_TRUE(m_mob->startRiding(otherPig));

    m_mob->updateMovementGoalFlags();

    // 非船载具不触发船特例。
    EXPECT_FALSE(jumpDisabled());
    EXPECT_FALSE(moveDisabled());
}