#include <gtest/gtest.h>

#include "common/entity/entities/player/Player.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/core/Constants.hpp"

namespace mc {
namespace {

class PlayerPoseCollisionWorld : public IWorld {
public:
    void setLowCeiling(bool enabled) { m_lowCeiling = enabled; }

    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlock(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return fluid::Fluid::getFluidState(0); }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 0; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

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

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }

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