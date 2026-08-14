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

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/player/GameModeUtils.hpp"
#include "entity/entities/player/Player.hpp"
#include "physics/PhysicsEngine.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace mc {
namespace {

/**
 * @brief Player移动测试固件
 *
 * 测试飞行速度、移动输入处理等核心移动逻辑。
 * 参考 MC 1.16.5 PlayerEntity.travel() 和 ClientPlayerEntity.livingTick()
 */
class PlayerMovementTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和流体注册表
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();

        // 创建玩家
        m_player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer", mc::test::testEcsRegistry());

        // 设置创造模式（允许飞行）
        m_player->setGameMode(GameMode::Creative);
    }

    void TearDown() override { m_player.reset(); }

    std::unique_ptr<Player> m_player;
};

class GroundSupportWorld final : public mc::test::BaseTestWorld {
public:
    void setSupportEnabled(bool enabled) { m_supportEnabled = enabled; }

    struct SoundRecord {
        ResourceLocation soundEventId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    [[nodiscard]] bool hasSoundRecord() const { return m_lastSound.has_value(); }
    [[nodiscard]] const SoundRecord& lastSound() const { return *m_lastSound; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (m_supportEnabled && y == 0 && x >= -1 && x <= 1 && z >= -1 && z <= 1) {
            return &VanillaBlocks::STONE->defaultState();
        }

        return nullptr;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        if (!m_supportEnabled) {
            return false;
        }

        return box.maxX > -1.0f && box.minX < 2.0f && box.maxY > 0.0f && box.minY < 1.0f && box.maxZ > -1.0f &&
            box.minZ < 2.0f;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        if (!hasBlockCollision(box)) {
            return {};
        }

        return {AxisAlignedBB(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSound = SoundRecord{soundEventId, category, position, volume, pitch};
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("GroundSupportWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("GroundSupportWorld::tickManager not implemented");
    }

private:
    bool m_supportEnabled = true;
    std::optional<SoundRecord> m_lastSound;
};

class EmptyCollisionWorld final : public ICollisionWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] i32 getMinBuildHeight() const override { return mc::world::MIN_BUILD_HEIGHT; }
    [[nodiscard]] i32 getMaxBuildHeight() const override { return mc::world::MAX_BUILD_HEIGHT; }
};

// ============================================================================
// 飞行速度测试
// ============================================================================

TEST_F(PlayerMovementTest, FlySpeedDefaultValue_IsCorrect)
{
    // MC中flySpeed默认值是0.05F
    EXPECT_FLOAT_EQ(m_player->abilities().flySpeed, physics::FLY_SPEED);
}

TEST_F(PlayerMovementTest, WalkSpeedDefaultValue_IsCorrect)
{
    // MC中walkSpeed默认值是0.1F
    EXPECT_FLOAT_EQ(m_player->abilities().walkSpeed, physics::WALK_SPEED);
}

TEST_F(PlayerMovementTest, CreativeMode_HasFlyAbility)
{
    EXPECT_TRUE(m_player->abilities().canFly);
    EXPECT_TRUE(m_player->abilities().creativeMode);
}

TEST_F(PlayerMovementTest, SurvivalMode_NoFlyAbility)
{
    m_player->setGameMode(GameMode::Survival);
    EXPECT_FALSE(m_player->abilities().canFly);
    EXPECT_FALSE(m_player->abilities().creativeMode);
}

TEST_F(PlayerMovementTest, DamagePlaysHurtSound)
{
    GroundSupportWorld world;
    m_player->setWorld(&world);
    m_player->setGameMode(GameMode::Survival);
    m_player->setHealth(20.0f);

    auto genericSource = DamageSources::generic();
    m_player->hurt(genericSource, 5.0f);

    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.player.hurt");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Players);
    EXPECT_FLOAT_EQ(world.lastSound().volume, 1.0f);
}

TEST_F(PlayerMovementTest, LethalDamagePlaysDeathSound)
{
    GroundSupportWorld world;
    m_player->setWorld(&world);
    m_player->setGameMode(GameMode::Survival);
    m_player->setHealth(5.0f);

    auto genericSource = DamageSources::generic();
    m_player->hurt(genericSource, 10.0f);

    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.player.death");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Players);
}

// ============================================================================
// 飞行水平移动测试
// ============================================================================

TEST_F(PlayerMovementTest, Flying_HorizontalMovement_AddsToVelocity)
{
    // 开启飞行
    m_player->abilities().flying = true;

    // 初始速度应为零
    EXPECT_FLOAT_EQ(m_player->velocity().x, 0.0f);
    EXPECT_FLOAT_EQ(m_player->velocity().z, 0.0f);

    // 向前移动 (yaw=0 时，forward=1 会影响 Z 方向)
    m_player->handleMovementInput(1.0f, 0.0f, false, false);
    m_player->updatePhysics();

    // 飞行速度 = flySpeed * 1.0 = 0.05，随后应用水平阻力 0.91
    f32 speed =
        std::sqrt(m_player->velocity().x * m_player->velocity().x + m_player->velocity().z * m_player->velocity().z);
    EXPECT_NEAR(speed, physics::FLY_SPEED * physics::FLY_HORIZONTAL_DRAG, 0.001f);
}

TEST_F(PlayerMovementTest, Flying_Sprint_DoublesSpeed)
{
    m_player->abilities().flying = true;
    m_player->setSprinting(true);

    // 冲刺时速度应该是两倍
    m_player->handleMovementInput(1.0f, 0.0f, false, false);
    m_player->updatePhysics();

    f32 sprintSpeed =
        std::sqrt(m_player->velocity().x * m_player->velocity().x + m_player->velocity().z * m_player->velocity().z);

    // 重置
    m_player->setSprinting(false);
    m_player->setVelocity(Vector3(0.0f, 0.0f, 0.0f));

    // 非冲刺
    m_player->handleMovementInput(1.0f, 0.0f, false, false);
    m_player->updatePhysics();
    f32 normalSpeed =
        std::sqrt(m_player->velocity().x * m_player->velocity().x + m_player->velocity().z * m_player->velocity().z);

    // 冲刺速度应该约为非冲刺的两倍
    EXPECT_NEAR(sprintSpeed, normalSpeed * 2.0f, 0.001f);
}

// ============================================================================
// 飞行垂直移动测试
// ============================================================================

TEST_F(PlayerMovementTest, Flying_Jump_IncreasesVerticalVelocity)
{
    m_player->abilities().flying = true;

    // 初始Y速度为零
    EXPECT_FLOAT_EQ(m_player->velocity().y, 0.0f);

    // 按跳跃键上升
    m_player->handleMovementInput(0.0f, 0.0f, true, false);
    m_player->updatePhysics();

    // Y速度应该增加后应用飞行垂直阻力 0.6
    EXPECT_NEAR(m_player->velocity().y,
        physics::FLY_SPEED * physics::FLY_VERTICAL_INPUT_MULTIPLIER * physics::FLY_VERTICAL_DRAG,
        0.001f);
}

TEST_F(PlayerMovementTest, Flying_Sneak_DecreasesVerticalVelocity)
{
    m_player->abilities().flying = true;

    // 初始Y速度为零
    EXPECT_FLOAT_EQ(m_player->velocity().y, 0.0f);

    // 按潜行键下降
    m_player->handleMovementInput(0.0f, 0.0f, false, true);
    m_player->updatePhysics();

    // Y速度应该减少后应用飞行垂直阻力 0.6
    EXPECT_NEAR(m_player->velocity().y,
        -physics::FLY_SPEED * physics::FLY_VERTICAL_INPUT_MULTIPLIER * physics::FLY_VERTICAL_DRAG,
        0.001f);
}

TEST_F(PlayerMovementTest, Flying_JumpAndSneak_CancelOut)
{
    m_player->abilities().flying = true;

    // 同时按跳跃和潜行应该抵消
    m_player->handleMovementInput(0.0f, 0.0f, true, true);
    m_player->updatePhysics();

    // Y速度应该保持接近零
    EXPECT_NEAR(m_player->velocity().y, 0.0f, 0.001f);
}

TEST_F(PlayerMovementTest, Flying_VerticalSpeed_SprintDoubles)
{
    m_player->abilities().flying = true;
    m_player->setSprinting(true);

    // 冲刺时上升速度应该是两倍
    m_player->handleMovementInput(0.0f, 0.0f, true, false);
    m_player->updatePhysics();

    // verticalSpeed = flySpeed * 3.0 * 2.0 (冲刺) = 0.3，再乘飞行垂直阻力 0.6
    EXPECT_NEAR(m_player->velocity().y,
        physics::FLY_SPEED * physics::FLY_VERTICAL_INPUT_MULTIPLIER * physics::SPRINT_FLY_MULTIPLIER *
            physics::FLY_VERTICAL_DRAG,
        0.001f);
}

// ============================================================================
// 飞行物理更新测试
// ============================================================================

TEST_F(PlayerMovementTest, Flying_YVelocityDrag_AppliedInUpdatePhysics)
{
    m_player->abilities().flying = true;
    m_player->setVelocity(Vector3(0.0f, 1.0f, 0.0f));

    // 不设置物理引擎，只测试阻力
    m_player->updatePhysics();

    // 飞行时Y方向阻力是0.6
    // 参考 MC PlayerEntity.travel() line 1451:
    // this.setMotion(vector3d.x, d5 * 0.6D, vector3d.z);
    EXPECT_NEAR(m_player->velocity().y, 0.6f, 0.001f);
}

TEST_F(PlayerMovementTest, Flying_HorizontalDrag_AppliedInUpdatePhysics)
{
    m_player->abilities().flying = true;
    m_player->setVelocity(Vector3(1.0f, 0.0f, 1.0f));

    m_player->updatePhysics();

    // 飞行时水平阻力是0.91 (DRAG_GROUND)
    EXPECT_NEAR(m_player->velocity().x, 0.91f, 0.001f);
    EXPECT_NEAR(m_player->velocity().z, 0.91f, 0.001f);
}

TEST_F(PlayerMovementTest, NotFlying_AirDrag_AppliedInUpdatePhysics)
{
    m_player->abilities().flying = false;
    m_player->setVelocity(Vector3(1.0f, 1.0f, 1.0f));
    m_player->setOnGround(false);

    m_player->updatePhysics();

    // MC 空中水平阻力为 0.91，垂直阻力为 0.98
    EXPECT_NEAR(m_player->velocity().x, physics::DRAG_GROUND, 0.001f);
    EXPECT_NEAR(m_player->velocity().z, physics::DRAG_GROUND, 0.001f);
    EXPECT_NEAR(m_player->velocity().y, (1.0f - physics::GRAVITY) * physics::DRAG_AIR, 0.001f);
}

// ============================================================================
// 非飞行移动测试
// ============================================================================

TEST_F(PlayerMovementTest, Walking_HorizontalMovement_AddsToVelocity)
{
    GroundSupportWorld world;
    m_player->setWorld(&world);
    m_player->abilities().flying = false;
    m_player->setPosition(0.3f, 1.0f, 0.3f);
    m_player->setOnGround(true);

    m_player->handleMovementInput(1.0f, 0.0f, false, false);
    m_player->updatePhysics();

    f32 speed =
        std::sqrt(m_player->velocity().x * m_player->velocity().x + m_player->velocity().z * m_player->velocity().z);

    // 地面行走单 tick 水平速度（yaw=0，forward=1，STONE 默认 slipperiness=0.6、speedFactor=1.0）：
    //   speedFactor = getGroundMoveFactor(MOVEMENT_SPEED=0.1, slip=0.6) * getBlockSpeedFactor()
    //               = (0.1 * 0.21600002 / 0.6^3) * 1.0 = 0.1
    //   m_velocity.z += 0.1 后，水平阻力 = slip * DRAG_GROUND = 0.6 * 0.91 = 0.546
    //   speed = 0.1 * 0.546 = 0.0546
    // 对齐 MC Java LivingEntity.travelInAir：地面分支末段衰减 slipperiness*0.91，
    // 速度标量经 getFrictionInfluencedSpeed = speed*0.216/slip^3（默认 slip 下退化为 speed）。
    EXPECT_NEAR(speed, 0.0546f, 0.001f);
}

TEST_F(PlayerMovementTest, Sneaking_ReducesSpeed)
{
    GroundSupportWorld world;
    m_player->setWorld(&world);
    m_player->abilities().flying = false;
    m_player->setPosition(0.3f, 1.0f, 0.3f);
    m_player->setOnGround(true);

    // 潜行速度 = 地面输入速度 * 0.3
    m_player->handleMovementInput(1.0f, 0.0f, false, true);
    m_player->updatePhysics();

    f32 sneakSpeed =
        std::sqrt(m_player->velocity().x * m_player->velocity().x + m_player->velocity().z * m_player->velocity().z);

    // 重置
    m_player->setVelocity(Vector3(0.0f, 0.0f, 0.0f));
    m_player->setPosition(0.3f, 1.0f, 0.3f);
    m_player->setOnGround(true);

    // 正常行走
    m_player->handleMovementInput(1.0f, 0.0f, false, false);
    m_player->updatePhysics();
    f32 normalSpeed =
        std::sqrt(m_player->velocity().x * m_player->velocity().x + m_player->velocity().z * m_player->velocity().z);

    // 潜行速度应该是正常速度的约30%
    EXPECT_NEAR(sneakSpeed, normalSpeed * physics::SNEAK_SPEED_MULTIPLIER, 0.01f);
}

// ============================================================================
// 移动距离累计测试
// ============================================================================

TEST_F(PlayerMovementTest, UpdateMoveDistance_ResamplesCurrentPosition)
{
    m_player->setOnGround(true);
    m_player->setVelocity(Vector3(0.2f, 0.0f, 0.0f));

    m_player->move(2.0f, 0.0f, 0.0f);
    m_player->updateMoveDistance();

    EXPECT_TRUE(m_player->shouldPlayStepSound());
    EXPECT_NEAR(m_player->moveDistanceWalked(), 1.2f, 0.0001f);
    EXPECT_FLOAT_EQ(m_player->prevMoveDistanceWalked(), 0.0f);
    EXPECT_GT(m_player->cameraYaw(), 0.0f);
    EXPECT_FLOAT_EQ(m_player->prevCameraYaw(), 0.0f);

    m_player->updateMoveDistance();

    EXPECT_FALSE(m_player->shouldPlayStepSound());
    EXPECT_NEAR(m_player->moveDistanceWalked(), 1.2f, 0.0001f);
    EXPECT_NEAR(m_player->prevMoveDistanceWalked(), 1.2f, 0.0001f);

    // 重置位置与速度：setPosition 复位 moveDistance/cameraYaw，但不复位 velocity；
    // _updateCameraYaw 基于 m_velocity 计算 targetCameraYaw，故需同时清零速度才能让
    // cameraYaw 在 updateMoveDistance 后保持 0（模拟完全静止状态）。
    m_player->setVelocity(Vector3(0.0f, 0.0f, 0.0f));
    m_player->setPosition(10.0f, 64.0f, 10.0f);
    m_player->updateMoveDistance();

    EXPECT_FALSE(m_player->shouldPlayStepSound());
    EXPECT_FLOAT_EQ(m_player->moveDistanceWalked(), 0.0f);
    EXPECT_FLOAT_EQ(m_player->prevMoveDistanceWalked(), 0.0f);
    EXPECT_FLOAT_EQ(m_player->cameraYaw(), 0.0f);
    EXPECT_FLOAT_EQ(m_player->prevCameraYaw(), 0.0f);
}

TEST_F(PlayerMovementTest, UpdateMoveDistance_DecaysCameraYawWhenAirborne)
{
    m_player->setOnGround(true);
    m_player->setVelocity(Vector3(0.2f, 0.0f, 0.0f));
    m_player->move(2.0f, 0.0f, 0.0f);
    m_player->updateMoveDistance();
    const f32 walkingCameraYaw = m_player->cameraYaw();
    ASSERT_GT(walkingCameraYaw, 0.0f);

    m_player->setOnGround(false);
    m_player->updateMoveDistance();

    EXPECT_LT(m_player->cameraYaw(), walkingCameraYaw);
    EXPECT_FLOAT_EQ(m_player->prevCameraYaw(), walkingCameraYaw);
}

// ============================================================================
// 跳跃测试
// ============================================================================

TEST_F(PlayerMovementTest, Jump_OnGround_SetsJumpVelocity)
{
    m_player->abilities().flying = false;
    m_player->setOnGround(true);

    // 跳跃速度应该是 JUMP_VELOCITY = 0.42
    m_player->jump();

    EXPECT_NEAR(m_player->velocity().y, 0.42f, 0.001f);
    EXPECT_FALSE(m_player->onGround());
}

TEST_F(PlayerMovementTest, Jump_InAir_DoesNothing)
{
    m_player->abilities().flying = false;
    m_player->setOnGround(false);

    f32 prevY = m_player->velocity().y;
    m_player->jump();

    // 在空中不能跳跃
    EXPECT_FLOAT_EQ(m_player->velocity().y, prevY);
}

TEST_F(PlayerMovementTest, Jump_WhileFlying_UsesFlyUpInstead)
{
    // 飞行模式下在地面上
    m_player->abilities().flying = true;
    m_player->setOnGround(true);

    // 飞行时按跳跃键应该触发飞行上升，而不是普通跳跃
    // handleMovementInput 缓存输入，updatePhysics 在固定 tick 中处理飞行上升
    m_player->handleMovementInput(0.0f, 0.0f, true, false);
    m_player->updatePhysics();

    // 飞行上升速度 = flySpeed * 3.0 * 飞行垂直阻力 0.6，而不是普通跳跃速度 0.42
    EXPECT_NEAR(m_player->velocity().y,
        physics::FLY_SPEED * physics::FLY_VERTICAL_INPUT_MULTIPLIER * physics::FLY_VERTICAL_DRAG,
        0.001f);
}

TEST_F(PlayerMovementTest, HandleMovementInput_WithPhysicsWorldOnly_DoesNotCrash)
{
    EmptyCollisionWorld collisionWorld;
    PhysicsEngine physicsEngine(collisionWorld);

    m_player->setPhysicsEngine(&physicsEngine);
    m_player->abilities().flying = true;
    m_player->setPosition(0.0f, 64.0f, 0.0f);

    m_player->handleMovementInput(0.0f, 0.0f, false, false);

    EXPECT_FALSE(m_player->isInWater());
    EXPECT_FALSE(m_player->isInLava());
}

TEST_F(PlayerMovementTest, FallingAfterSupportRemovalRefreshesGroundState)
{
    GroundSupportWorld world;
    m_player->setWorld(&world);
    m_player->setGameMode(GameMode::Survival);
    m_player->setPosition(0.3f, 1.0f, 0.3f);
    m_player->setVelocity(Vector3(0.0f, 0.0f, 0.0f));

    world.setSupportEnabled(true);
    m_player->updatePhysics();
    ASSERT_TRUE(m_player->isOnGround());

    f32 supportedY = m_player->y();

    world.setSupportEnabled(false);
    m_player->updatePhysics();

    EXPECT_FALSE(m_player->isOnGround());
}

TEST_F(PlayerMovementTest, SetPosition_ResetsInterpolationHistory)
{
    m_player->setPosition(0.0f, 64.0f, 0.0f);
    m_player->move(2.0f, 0.0f, 0.0f);
    ASSERT_FLOAT_EQ(m_player->prevX(), 0.0f);
    ASSERT_FLOAT_EQ(m_player->x(), 2.0f);

    m_player->setPosition(10.0f, 70.0f, -5.0f);

    EXPECT_FLOAT_EQ(m_player->prevX(), 10.0f);
    EXPECT_FLOAT_EQ(m_player->prevY(), 70.0f);
    EXPECT_FLOAT_EQ(m_player->prevZ(), -5.0f);
    EXPECT_FLOAT_EQ(m_player->x(), 10.0f);
    EXPECT_FLOAT_EQ(m_player->y(), 70.0f);
    EXPECT_FLOAT_EQ(m_player->z(), -5.0f);
}

TEST_F(PlayerMovementTest, UpdatePhysics_SnapshotsPreviousTickPosition)
{
    m_player->abilities().flying = true;
    m_player->setPosition(0.0f, 64.0f, 0.0f);
    m_player->handleMovementInput(1.0f, 0.0f, false, false);

    m_player->updatePhysics();
    const Vector3 firstTickPosition = m_player->position();
    ASSERT_GT(firstTickPosition.z, 0.0f);
    EXPECT_FLOAT_EQ(m_player->prevX(), 0.0f);
    EXPECT_FLOAT_EQ(m_player->prevY(), 64.0f);
    EXPECT_FLOAT_EQ(m_player->prevZ(), 0.0f);

    m_player->handleMovementInput(1.0f, 0.0f, false, false);
    m_player->updatePhysics();

    EXPECT_FLOAT_EQ(m_player->prevX(), firstTickPosition.x);
    EXPECT_FLOAT_EQ(m_player->prevY(), firstTickPosition.y);
    EXPECT_FLOAT_EQ(m_player->prevZ(), firstTickPosition.z);
    EXPECT_GT(m_player->z(), firstTickPosition.z);
}

// ============================================================================
// 阻力衰减测试
// ============================================================================

TEST_F(PlayerMovementTest, VelocityDecays_WithDrag)
{
    m_player->abilities().flying = true;
    m_player->setVelocity(Vector3(1.0f, 1.0f, 1.0f));

    // 多次应用阻力
    for (int i = 0; i < 10; i++) {
        m_player->updatePhysics();
    }

    // 速度应该逐渐衰减
    EXPECT_LT(m_player->velocity().x, 0.5f);
    EXPECT_LT(m_player->velocity().y, 0.01f); // Y衰减更快（0.6^n）
    EXPECT_LT(m_player->velocity().z, 0.5f);
}

// ============================================================================
// 游戏模式能力测试
// ============================================================================

TEST_F(PlayerMovementTest, GameModeUtils_Creative_HasAllAbilities)
{
    auto abilities = entity::GameModeUtils::getAbilitiesForGameMode(GameMode::Creative);

    EXPECT_TRUE(abilities.creativeMode);
    EXPECT_TRUE(abilities.canFly);
    EXPECT_TRUE(abilities.invulnerable);
    EXPECT_TRUE(abilities.allowEdit);
    EXPECT_FLOAT_EQ(abilities.flySpeed, physics::FLY_SPEED);
    EXPECT_FLOAT_EQ(abilities.walkSpeed, physics::WALK_SPEED);
}

TEST_F(PlayerMovementTest, GameModeUtils_Survival_NoFlyNoInvulnerable)
{
    auto abilities = entity::GameModeUtils::getAbilitiesForGameMode(GameMode::Survival);

    EXPECT_FALSE(abilities.creativeMode);
    EXPECT_FALSE(abilities.canFly);
    EXPECT_FALSE(abilities.invulnerable);
    EXPECT_TRUE(abilities.allowEdit);
}

TEST_F(PlayerMovementTest, GameModeUtils_Spectator_CanFlyFlying)
{
    auto abilities = entity::GameModeUtils::getAbilitiesForGameMode(GameMode::Spectator);

    EXPECT_FALSE(abilities.creativeMode);
    EXPECT_TRUE(abilities.canFly);
    EXPECT_TRUE(abilities.flying); // 观察者模式默认飞行
    EXPECT_TRUE(abilities.invulnerable);
    EXPECT_FALSE(abilities.allowEdit);
}

} // namespace
} // namespace mc
