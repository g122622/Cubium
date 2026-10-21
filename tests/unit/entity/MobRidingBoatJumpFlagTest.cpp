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
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/world/IWorld.hpp"
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;

namespace {

/// 按实例 id 登记实体的测试世界（startRiding 经世界查询解析载具）
class RidingTestWorld final : public mc::test::BaseTestWorld {
public:
    void track(Entity& entity) { m_entities[entity.id()] = &entity; }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        const auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

private:
    std::unordered_map<u64, Entity*> m_entities;
};

} // namespace

class MobRidingBoatJumpFlagTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_registry = std::make_unique<ecs::EntityRegistry>(std::string("MobRidingBoatJumpFlagTest"));
        m_world = std::make_unique<RidingTestWorld>();

        m_mob = std::make_unique<PigEntity>(EntityInstanceId(1), *m_registry);
        m_mob->setId(EntityInstanceId(1));
        m_mob->setWorld(m_world.get());

        m_boat = std::make_unique<BoatEntity>(BoatEntity::Type::OAK, *m_registry);
        m_boat->setId(EntityInstanceId(2));
        m_boat->setWorld(m_world.get());
        m_world->track(*m_boat);

        m_otherPig = std::make_unique<PigEntity>(EntityInstanceId(3), *m_registry);
        m_otherPig->setId(EntityInstanceId(3));
        m_otherPig->setWorld(m_world.get());
        m_world->track(*m_otherPig);
    }

    void TearDown() override
    {
        m_mob->stopRiding();
        m_otherPig.reset();
        m_boat.reset();
        m_mob.reset();
        m_world.reset();
        m_registry.reset();
    }

    [[nodiscard]] bool jumpDisabled() const { return m_mob->goalSelector().isFlagDisabled(ai::GoalFlag::Jump); }
    [[nodiscard]] bool moveDisabled() const { return m_mob->goalSelector().isFlagDisabled(ai::GoalFlag::Move); }

    std::unique_ptr<ecs::EntityRegistry> m_registry;
    std::unique_ptr<RidingTestWorld> m_world;
    std::unique_ptr<PigEntity> m_mob;
    std::unique_ptr<BoatEntity> m_boat;
    std::unique_ptr<PigEntity> m_otherPig;
};

TEST_F(MobRidingBoatJumpFlagTest, NotRiding_JumpAndMoveEnabled)
{
    m_mob->updateMovementGoalFlags();
    EXPECT_FALSE(jumpDisabled());
    EXPECT_FALSE(moveDisabled());
}

TEST_F(MobRidingBoatJumpFlagTest, RidingBoatWithoutControllingPassenger_JumpDisabled)
{
    ASSERT_TRUE(m_mob->startRiding(*m_boat));
    ASSERT_TRUE(m_mob->isRiding());
    ASSERT_EQ(m_mob->getControllingPassenger(), INVALID_ENTITY_ID);

    m_mob->updateMovementGoalFlags();

    // 船特例：即使没有控制乘客，JUMP 也被禁用；MOVE 不受影响。
    EXPECT_TRUE(jumpDisabled());
    EXPECT_FALSE(moveDisabled());
}

TEST_F(MobRidingBoatJumpFlagTest, RidingNonBoat_JumpRemainsEnabled)
{
    ASSERT_TRUE(m_mob->startRiding(*m_otherPig));
    ASSERT_TRUE(m_mob->isRiding());

    m_mob->updateMovementGoalFlags();

    // 非船载具不触发船特例。
    EXPECT_FALSE(jumpDisabled());
    EXPECT_FALSE(moveDisabled());
}
