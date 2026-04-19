#include <gtest/gtest.h>
#include "entity/entities/player/Player.hpp"
#include "entity/entities/player/GameModeUtils.hpp"
#include "common/world/IWorld.hpp"
#include "physics/PhysicsEngine.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/fluid/Fluid.hpp"
#include <cmath>
#include "common/resource/ResourceLocation.hpp"

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
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        // 创建玩家
        m_player = std::make_unique<Player>(static_cast<EntityId>(1), "TestPlayer");

        // 设置创造模式（允许飞行）
        m_player->setGameMode(GameMode::Creative);
    }

    void TearDown() override {
        m_player.reset();
    }

    std::unique_ptr<Player> m_player;
};

class GroundSupportWorld final : public IWorld {
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

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        if (m_supportEnabled && x == 0 && y == 0 && z == 0) {
            return &VanillaBlocks::STONE->defaultState();
        }

        return nullptr;
    }

    bool setBlock(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return fluid::Fluid::getFluidState(0); }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override {
        if (!m_supportEnabled) {
            return false;
        }

        return box.maxX > 0.0f && box.minX < 1.0f &&
               box.maxY > 0.0f && box.minY < 1.0f &&
               box.maxZ > 0.0f && box.minZ < 1.0f;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override {
        if (!hasBlockCollision(box)) {
            return {};
        }

        return {AxisAlignedBB(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= 0 && y < 256; }
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

    void playSound(const ResourceLocation& soundEventId,
                   sound::SoundCategory category,
                   const Vector3& position,
                   f32 volume,
                   f32 pitch) override {
        m_lastSound = SoundRecord{soundEventId, category, position, volume, pitch};
    }

private:
    bool m_supportEnabled = true;
    Optional<SoundRecord> m_lastSound;
};

// ============================================================================
// 飞行速度测试
// ============================================================================

TEST_F(PlayerMovementTest, FlySpeedDefaultValue_IsCorrect) {
    // MC中flySpeed默认值是0.05F
    EXPECT_FLOAT_EQ(m_player->abilities().flySpeed, 0.05f);
}

TEST_F(PlayerMovementTest, WalkSpeedDefaultValue_IsCorrect) {
    // MC中walkSpeed默认值是0.1F
    EXPECT_FLOAT_EQ(m_player->abilities().walkSpeed, 0.1f);
}

TEST_F(PlayerMovementTest, CreativeMode_HasFlyAbility) {
    EXPECT_TRUE(m_player->abilities().canFly);
    EXPECT_TRUE(m_player->abilities().creativeMode);
}

TEST_F(PlayerMovementTest, SurvivalMode_NoFlyAbility) {
    m_player->setGameMode(GameMode::Survival);
    EXPECT_FALSE(m_player->abilities().canFly);
    EXPECT_FALSE(m_player->abilities().creativeMode);
}

TEST_F(PlayerMovementTest, DamagePlaysHurtSound) {
    GroundSupportWorld world;
    m_player->setWorld(&world);
    m_player->setGameMode(GameMode::Survival);
    m_player->setHealth(20.0f);

    m_player->damage(5.0f);

    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.player.hurt");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Players);
    EXPECT_FLOAT_EQ(world.lastSound().volume, 1.0f);
}

TEST_F(PlayerMovementTest, LethalDamagePlaysDeathSound) {
    GroundSupportWorld world;
    m_player->setWorld(&world);
    m_player->setGameMode(GameMode::Survival);
    m_player->setHealth(5.0f);

    m_player->damage(10.0f);

    ASSERT_TRUE(world.hasSoundRecord());
    EXPECT_EQ(world.lastSound().soundEventId.toString(), "minecraft:entity.player.death");
    EXPECT_EQ(world.lastSound().category, sound::SoundCategory::Players);
}

// ============================================================================
// 飞行水平移动测试
// ============================================================================

TEST_F(PlayerMovementTest, Flying_HorizontalMovement_AddsToVelocity) {
    // 开启飞行
    m_player->abilities().flying = true;

    // 初始速度应为零
    EXPECT_FLOAT_EQ(m_player->velocity().x, 0.0f);
    EXPECT_FLOAT_EQ(m_player->velocity().z, 0.0f);

    // 向前移动 (yaw=0 时，forward=1 会影响 Z 方向)
    m_player->handleMovementInput(1.0f, 0.0f, false, false);

    // 飞行速度 = flySpeed * 1.0 = 0.05
    // 速度应该被添加（不是替换）
    // 由于yaw=0时forward影响Z方向，我们应该检查总速度不为零
    f32 speed = std::sqrt(
        m_player->velocity().x * m_player->velocity().x +
        m_player->velocity().z * m_player->velocity().z
    );
    EXPECT_GT(speed, 0.0f);
}

TEST_F(PlayerMovementTest, Flying_Sprint_DoublesSpeed) {
    m_player->abilities().flying = true;
    m_player->setSprinting(true);

    // 冲刺时速度应该是两倍
    m_player->handleMovementInput(1.0f, 0.0f, false, false);

    // 飞行速度 = flySpeed * 2.0 (冲刺) = 0.1
    // 速度应该大于非冲刺情况
    f32 sprintSpeed = std::sqrt(
        m_player->velocity().x * m_player->velocity().x +
        m_player->velocity().z * m_player->velocity().z
    );

    // 重置
    m_player->setSprinting(false);
    m_player->setVelocity(Vector3(0.0f, 0.0f, 0.0f));

    // 非冲刺
    m_player->handleMovementInput(1.0f, 0.0f, false, false);
    f32 normalSpeed = std::sqrt(
        m_player->velocity().x * m_player->velocity().x +
        m_player->velocity().z * m_player->velocity().z
    );

    // 冲刺速度应该约为非冲刺的两倍
    EXPECT_NEAR(sprintSpeed, normalSpeed * 2.0f, 0.001f);
}

// ============================================================================
// 飞行垂直移动测试
// ============================================================================

TEST_F(PlayerMovementTest, Flying_Jump_IncreasesVerticalVelocity) {
    m_player->abilities().flying = true;

    // 初始Y速度为零
    EXPECT_FLOAT_EQ(m_player->velocity().y, 0.0f);

    // 按跳跃键上升
    m_player->handleMovementInput(0.0f, 0.0f, true, false);

    // Y速度应该增加
    // MC中：verticalSpeed = flySpeed * 3.0 = 0.05 * 3.0 = 0.15
    // 注意：速度是添加的
    EXPECT_NEAR(m_player->velocity().y, 0.15f, 0.001f);
}

TEST_F(PlayerMovementTest, Flying_Sneak_DecreasesVerticalVelocity) {
    m_player->abilities().flying = true;

    // 初始Y速度为零
    EXPECT_FLOAT_EQ(m_player->velocity().y, 0.0f);

    // 按潜行键下降
    m_player->handleMovementInput(0.0f, 0.0f, false, true);

    // Y速度应该减少（下降）
    // MC中：verticalSpeed = flySpeed * 3.0 = 0.05 * 3.0 = 0.15
    EXPECT_NEAR(m_player->velocity().y, -0.15f, 0.001f);
}

TEST_F(PlayerMovementTest, Flying_JumpAndSneak_CancelOut) {
    m_player->abilities().flying = true;

    // 同时按跳跃和潜行应该抵消
    m_player->handleMovementInput(0.0f, 0.0f, true, true);

    // Y速度应该保持接近零
    EXPECT_NEAR(m_player->velocity().y, 0.0f, 0.001f);
}

TEST_F(PlayerMovementTest, Flying_VerticalSpeed_SprintDoubles) {
    m_player->abilities().flying = true;
    m_player->setSprinting(true);

    // 冲刺时上升速度应该是两倍
    m_player->handleMovementInput(0.0f, 0.0f, true, false);

    // verticalSpeed = flySpeed * 3.0 * 2.0 (冲刺) = 0.3
    EXPECT_NEAR(m_player->velocity().y, 0.3f, 0.001f);
}

// ============================================================================
// 飞行物理更新测试
// ============================================================================

TEST_F(PlayerMovementTest, Flying_YVelocityDrag_AppliedInUpdatePhysics) {
    m_player->abilities().flying = true;
    m_player->setVelocity(Vector3(0.0f, 1.0f, 0.0f));

    // 不设置物理引擎，只测试阻力
    m_player->updatePhysics();

    // 飞行时Y方向阻力是0.6
    // 参考 MC PlayerEntity.travel() line 1451:
    // this.setMotion(vector3d.x, d5 * 0.6D, vector3d.z);
    EXPECT_NEAR(m_player->velocity().y, 0.6f, 0.001f);
}

TEST_F(PlayerMovementTest, Flying_HorizontalDrag_AppliedInUpdatePhysics) {
    m_player->abilities().flying = true;
    m_player->setVelocity(Vector3(1.0f, 0.0f, 1.0f));

    m_player->updatePhysics();

    // 飞行时水平阻力是0.91 (DRAG_GROUND)
    EXPECT_NEAR(m_player->velocity().x, 0.91f, 0.001f);
    EXPECT_NEAR(m_player->velocity().z, 0.91f, 0.001f);
}

TEST_F(PlayerMovementTest, NotFlying_AirDrag_AppliedInUpdatePhysics) {
    m_player->abilities().flying = false;
    m_player->setVelocity(Vector3(1.0f, 1.0f, 1.0f));
    m_player->setOnGround(false);

    m_player->updatePhysics();

    // 非飞行时阻力是0.98 (DRAG_AIR)
    EXPECT_NEAR(m_player->velocity().x, 0.98f, 0.001f);
    EXPECT_NEAR(m_player->velocity().z, 0.98f, 0.001f);
}

// ============================================================================
// 非飞行移动测试
// ============================================================================

TEST_F(PlayerMovementTest, Walking_HorizontalMovement_AddsToVelocity) {
    m_player->abilities().flying = false;
    m_player->setOnGround(true);

    // 行走速度 = walkSpeed = 0.1
    m_player->handleMovementInput(1.0f, 0.0f, false, false);

    // 应该有水平速度
    f32 speed = std::sqrt(
        m_player->velocity().x * m_player->velocity().x +
        m_player->velocity().z * m_player->velocity().z
    );

    // 速度应该接近 walkSpeed
    EXPECT_NEAR(speed, 0.1f, 0.01f);
}

TEST_F(PlayerMovementTest, Sneaking_ReducesSpeed) {
    m_player->abilities().flying = false;
    m_player->setOnGround(true);

    // 潜行速度 = walkSpeed * 0.3 = 0.03
    m_player->handleMovementInput(1.0f, 0.0f, false, true);

    f32 sneakSpeed = std::sqrt(
        m_player->velocity().x * m_player->velocity().x +
        m_player->velocity().z * m_player->velocity().z
    );

    // 重置
    m_player->setVelocity(Vector3(0.0f, 0.0f, 0.0f));

    // 正常行走
    m_player->handleMovementInput(1.0f, 0.0f, false, false);
    f32 normalSpeed = std::sqrt(
        m_player->velocity().x * m_player->velocity().x +
        m_player->velocity().z * m_player->velocity().z
    );

    // 潜行速度应该是正常速度的约30%
    EXPECT_NEAR(sneakSpeed, normalSpeed * 0.3f, 0.01f);
}

// ============================================================================
// 移动距离累计测试
// ============================================================================

TEST_F(PlayerMovementTest, UpdateMoveDistance_ResamplesCurrentPosition) {
    m_player->setOnGround(true);

    m_player->move(2.0f, 0.0f, 0.0f);
    m_player->updateMoveDistance();

    EXPECT_TRUE(m_player->shouldPlayStepSound());
    EXPECT_FLOAT_EQ(m_player->moveDistanceWalked(), 2.0f);
    EXPECT_FLOAT_EQ(m_player->prevMoveDistanceWalked(), 0.0f);

    m_player->updateMoveDistance();

    EXPECT_FALSE(m_player->shouldPlayStepSound());
    EXPECT_FLOAT_EQ(m_player->moveDistanceWalked(), 2.0f);
    EXPECT_FLOAT_EQ(m_player->prevMoveDistanceWalked(), 2.0f);

    m_player->setPosition(10.0f, 64.0f, 10.0f);
    m_player->updateMoveDistance();

    EXPECT_FALSE(m_player->shouldPlayStepSound());
    EXPECT_FLOAT_EQ(m_player->moveDistanceWalked(), 0.0f);
    EXPECT_FLOAT_EQ(m_player->prevMoveDistanceWalked(), 0.0f);
}

// ============================================================================
// 跳跃测试
// ============================================================================

TEST_F(PlayerMovementTest, Jump_OnGround_SetsJumpVelocity) {
    m_player->abilities().flying = false;
    m_player->setOnGround(true);

    // 跳跃速度应该是 JUMP_VELOCITY = 0.42
    m_player->jump();

    EXPECT_NEAR(m_player->velocity().y, 0.42f, 0.001f);
    EXPECT_FALSE(m_player->onGround());
}

TEST_F(PlayerMovementTest, Jump_InAir_DoesNothing) {
    m_player->abilities().flying = false;
    m_player->setOnGround(false);

    f32 prevY = m_player->velocity().y;
    m_player->jump();

    // 在空中不能跳跃
    EXPECT_FLOAT_EQ(m_player->velocity().y, prevY);
}

TEST_F(PlayerMovementTest, Jump_WhileFlying_UsesFlyUpInstead) {
    // 飞行模式下在地面上
    m_player->abilities().flying = true;
    m_player->setOnGround(true);

    // 飞行时按跳跃键应该触发飞行上升，而不是普通跳跃
    // handleMovementInput 中处理飞行上升
    m_player->handleMovementInput(0.0f, 0.0f, true, false);

    // 飞行上升速度 = flySpeed * 3.0 = 0.15
    // 而不是普通跳跃速度 0.42
    EXPECT_NEAR(m_player->velocity().y, 0.15f, 0.001f);
}

TEST_F(PlayerMovementTest, FallingAfterSupportRemovalRefreshesGroundState) {
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

// ============================================================================
// 阻力衰减测试
// ============================================================================

TEST_F(PlayerMovementTest, VelocityDecays_WithDrag) {
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

TEST_F(PlayerMovementTest, GameModeUtils_Creative_HasAllAbilities) {
    auto abilities = entity::GameModeUtils::getAbilitiesForGameMode(GameMode::Creative);

    EXPECT_TRUE(abilities.creativeMode);
    EXPECT_TRUE(abilities.canFly);
    EXPECT_TRUE(abilities.invulnerable);
    EXPECT_TRUE(abilities.allowEdit);
    EXPECT_FLOAT_EQ(abilities.flySpeed, 0.05f);
    EXPECT_FLOAT_EQ(abilities.walkSpeed, 0.1f);
}

TEST_F(PlayerMovementTest, GameModeUtils_Survival_NoFlyNoInvulnerable) {
    auto abilities = entity::GameModeUtils::getAbilitiesForGameMode(GameMode::Survival);

    EXPECT_FALSE(abilities.creativeMode);
    EXPECT_FALSE(abilities.canFly);
    EXPECT_FALSE(abilities.invulnerable);
    EXPECT_TRUE(abilities.allowEdit);
}

TEST_F(PlayerMovementTest, GameModeUtils_Spectator_CanFlyFlying) {
    auto abilities = entity::GameModeUtils::getAbilitiesForGameMode(GameMode::Spectator);

    EXPECT_FALSE(abilities.creativeMode);
    EXPECT_TRUE(abilities.canFly);
    EXPECT_TRUE(abilities.flying); // 观察者模式默认飞行
    EXPECT_TRUE(abilities.invulnerable);
    EXPECT_FALSE(abilities.allowEdit);
}

} // namespace
} // namespace mc
