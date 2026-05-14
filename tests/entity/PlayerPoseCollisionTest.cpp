#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace {

class PlayerPoseCollisionWorld : public test::BaseTestWorld {
public:
    void setLowCeiling(bool enabled) { m_lowCeiling = enabled; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        return m_lowCeiling && box.intersects(m_ceilingBox);
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        if (!hasBlockCollision(box)) {
            return {};
        }

        return {m_ceilingBox};
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PlayerPoseCollisionWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PlayerPoseCollisionWorld::tickManager not implemented");
    }

private:
    bool m_lowCeiling = false;
    AxisAlignedBB m_ceilingBox{-5.0f, 1.5f, -5.0f, 5.0f, 2.5f, 5.0f};
};

class PlayerPoseCollisionTest : public ::testing::Test {
protected:
    PlayerPoseCollisionWorld m_world;
};

TEST_F(PlayerPoseCollisionTest, SetSneakingFalseStandsUpInClearSpace)
{
    Player player(1, "Steve");
    player.setWorld(&m_world);
    player.setPosition(0.0f, 0.0f, 0.0f);

    player.setSneaking(true);
    EXPECT_EQ(player.pose(), EntityPose::Crouching);

    player.setSneaking(false);

    EXPECT_FALSE(player.isSneaking());
    EXPECT_EQ(player.pose(), EntityPose::Standing);
    EXPECT_FLOAT_EQ(player.boundingBox().height(), Player::PLAYER_HEIGHT);
}

TEST_F(PlayerPoseCollisionTest, SetSneakingFalseKeepsCrouchWhenCeilingBlocksStanding)
{
    m_world.setLowCeiling(true);

    Player player(2, "Alex");
    player.setWorld(&m_world);
    player.setPosition(0.0f, 0.0f, 0.0f);

    player.setSneaking(true);
    EXPECT_EQ(player.pose(), EntityPose::Crouching);

    player.setSneaking(false);

    EXPECT_TRUE(player.isSneaking());
    EXPECT_EQ(player.pose(), EntityPose::Crouching);
    EXPECT_FLOAT_EQ(player.boundingBox().height(), Player::PLAYER_CROUCH_HEIGHT);
}

TEST_F(PlayerPoseCollisionTest, SetSwimmingFalseFallsBackToCrouchWhenCeilingBlocksStanding)
{
    m_world.setLowCeiling(true);

    Player player(3, "Steve");
    player.setWorld(&m_world);
    player.setPosition(0.0f, 0.0f, 0.0f);

    player.setSwimming(true);
    EXPECT_EQ(player.pose(), EntityPose::Swimming);

    player.setSwimming(false);

    EXPECT_TRUE(player.isSneaking());
    EXPECT_FALSE(player.isSwimming());
    EXPECT_EQ(player.pose(), EntityPose::Crouching);
}

TEST_F(PlayerPoseCollisionTest, SetSleepingFalseFallsBackToCrouchWhenCeilingBlocksStanding)
{
    m_world.setLowCeiling(true);

    Player player(4, "Steve");
    player.setWorld(&m_world);
    player.setPosition(0.0f, 0.0f, 0.0f);

    player.setSleeping(true);
    EXPECT_EQ(player.pose(), EntityPose::Sleeping);

    player.setSleeping(false);

    EXPECT_TRUE(player.isSneaking());
    EXPECT_EQ(player.pose(), EntityPose::Crouching);
}

} // namespace
} // namespace mc
