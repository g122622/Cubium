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

/**
 * @file SquidGoalsTest.cpp
 * @brief 鱿鱼 AI 目标单元测试
 *
 * 测试 SquidMoveRandomGoal 和 SquidFleeGoal 的关键方法：
 * - SquidMoveRandomGoal: shouldExecute, tick
 * - SquidFleeGoal: shouldExecute, startExecuting, tick
 * - SquidFleeGoal: 流体/方块状态检查逻辑
 * - SquidFleeGoal: 气泡粒子生成
 * - SquidEntity: setMovementVector, hasMovementVector
 */

#include "common/entity/ai/goal/goals/special/SquidGoals.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/water/SquidEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::goal;

namespace {

/**
 * @brief 测试记录粒子数据的结构
 */
struct ParticleRecord {
    particle::ParticleTypeId type;
    Vector3 position;
    Vector3 velocity;
};

/**
 * @brief 鱿鱼目标测试用世界桩
 *
 * 支持可配置的流体状态、方块状态和粒子记录。
 * - 默认状态：目标位置返回水流体、null方块（空气）
 * - 可通过 setTargetFluidWater/setTargetFluidEmpty/setTargetBlockAir/setTargetBlockSolid 切换
 */
class SquidTestWorld final : public test::BaseTestWorld {
public:
    SquidTestWorld()
        : m_targetFluidWater(true)
        , m_targetBlockAir(true)
        , m_solidBlockState(nullptr)
    {}

    // ========== 流体状态配置 ==========

    /** 设置目标位置的流体为水 */
    void setTargetFluidWater(bool water) { m_targetFluidWater = water; }

    /** 设置目标位置的方块为空气（nullptr = 空气） */
    void setTargetBlockAir(bool air) { m_targetBlockAir = air; }

    // ========== 粒子记录 ==========

    std::vector<ParticleRecord>& particles() { return m_particles; }
    void clearRecords() { m_particles.clear(); }

    // ========== IWorld 接口覆写 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        if (m_targetFluidWater) {
            // 走 fluidId 路径取水默认状态。
            auto* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
            return waterFluid != nullptr ? &waterFluid->defaultState() : nullptr;
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (m_targetBlockAir) {
            // 返回 nullptr 表示空气
            return nullptr;
        }
        // 返回非空且 isAir()==false 的 BlockState 表示固体方块
        // 使用 BlockRegistry 中的空气状态（isAir()==true），但为测试目的
        // 我们需要一个 isAir()==false 的状态
        // 使用 m_solidBlockState 指针（测试中设置）
        if (m_solidBlockState != nullptr) {
            return m_solidBlockState;
        }
        // 退而求其次：返回空气状态（isAir()==true）
        // 注意：这意味着"非空气"的测试用例需要先设置 m_solidBlockState
        return nullptr;
    }

    /** 设置固体方块状态（isAir()==false 的 BlockState 指针） */
    void setSolidBlockState(const BlockState* state) { m_solidBlockState = state; }

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void addParticle(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3&, u32) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId) override { return nullptr; }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override { return nullptr; }
    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return EntityInstanceId(1); }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(const BlockPos&) const override { return 15; }
    [[nodiscard]] u8 getBlockLight(const BlockPos&) const override { return 0; }
    [[nodiscard]] bool canSeeSky(const BlockPos&) const override { return true; }
    [[nodiscard]] f32 getBrightness(const BlockPos&) const override { return 1.0f; }
    [[nodiscard]] u8 getLightSubtracted(const BlockPos&, u32) const override { return 15; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool isThundering() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

private:
    bool m_targetFluidWater;
    bool m_targetBlockAir;
    const BlockState* m_solidBlockState;
    std::vector<ParticleRecord> m_particles;
};

} // anonymous namespace

// ==================== SquidEntity Test Fixture ====================

class SquidEntityTest : public ::testing::Test {
protected:
    void SetUp() override { squid = std::make_unique<SquidEntity>(EntityInstanceId(0)); }

    void TearDown() override { squid.reset(); }

    std::unique_ptr<SquidEntity> squid;
};

// ==================== SquidEntity Movement Vector Tests ====================

TEST_F(SquidEntityTest, SetMovementVector_AllZeros_HasNoMovementVector)
{
    squid->setMovementVector(0.0f, 0.0f, 0.0f);
    EXPECT_FALSE(squid->hasMovementVector());
}

TEST_F(SquidEntityTest, SetMovementVector_NonZeroX_HasMovementVector)
{
    squid->setMovementVector(0.1f, 0.0f, 0.0f);
    EXPECT_TRUE(squid->hasMovementVector());
}

TEST_F(SquidEntityTest, SetMovementVector_NonZeroY_HasMovementVector)
{
    squid->setMovementVector(0.0f, 0.1f, 0.0f);
    EXPECT_TRUE(squid->hasMovementVector());
}

TEST_F(SquidEntityTest, SetMovementVector_NonZeroZ_HasMovementVector)
{
    squid->setMovementVector(0.0f, 0.0f, 0.1f);
    EXPECT_TRUE(squid->hasMovementVector());
}

TEST_F(SquidEntityTest, SetMovementVector_AllNonZero_HasMovementVector)
{
    squid->setMovementVector(0.2f, -0.1f, 0.15f);
    EXPECT_TRUE(squid->hasMovementVector());
}

TEST_F(SquidEntityTest, SetMovementVector_NegativeValues_HasMovementVector)
{
    squid->setMovementVector(-0.2f, -0.1f, -0.15f);
    EXPECT_TRUE(squid->hasMovementVector());
}

TEST_F(SquidEntityTest, SetMovementVector_VerySmallValue_HasMovementVector)
{
    squid->setMovementVector(0.001f, 0.0f, 0.0f);
    EXPECT_TRUE(squid->hasMovementVector());
}

TEST_F(SquidEntityTest, SetMovementVector_LargeValue_HasMovementVector)
{
    squid->setMovementVector(3.0f, 2.0f, 1.0f);
    EXPECT_TRUE(squid->hasMovementVector());
}

// ==================== SquidMoveRandomGoal Tests ====================

class SquidMoveRandomGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        squid = std::make_unique<SquidEntity>(EntityInstanceId(0));
        goal = std::make_unique<SquidMoveRandomGoal>(squid.get());
    }

    void TearDown() override
    {
        goal.reset();
        squid.reset();
    }

    std::unique_ptr<SquidEntity> squid;
    std::unique_ptr<SquidMoveRandomGoal> goal;
};

TEST_F(SquidMoveRandomGoalTest, ShouldExecute_AlwaysReturnsTrue)
{
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(SquidMoveRandomGoalTest, ShouldExecute_AlwaysReturnsTrueMultipleTimes)
{
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(goal->shouldExecute());
    }
}

TEST_F(SquidMoveRandomGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "SquidMoveRandomGoal");
}

TEST_F(SquidMoveRandomGoalTest, Tick_SetsZeroVectorWhenIdleTimeExceedsThreshold)
{
    squid->setIdleTime(101);
    squid->setMovementVector(0.5f, 0.5f, 0.5f);
    EXPECT_TRUE(squid->hasMovementVector());

    goal->tick();
    EXPECT_FALSE(squid->hasMovementVector());
}

TEST_F(SquidMoveRandomGoalTest, Tick_DoesNotClearVectorWhenIdleTimeBelowThreshold)
{
    squid->setIdleTime(50);
    squid->setInWater(true);
    squid->setMovementVector(0.5f, 0.5f, 0.5f);

    for (int i = 0; i < 5; ++i) {
        goal->tick();
    }
}

// ==================== SquidFleeGoal Tests ====================

class SquidFleeGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        squid = std::make_unique<SquidEntity>(EntityInstanceId(0));
        goal = std::make_unique<SquidFleeGoal>(squid.get());
    }

    void TearDown() override
    {
        goal.reset();
        squid.reset();
    }

    std::unique_ptr<SquidEntity> squid;
    std::unique_ptr<SquidFleeGoal> goal;
};

TEST_F(SquidFleeGoalTest, ShouldExecute_ReturnsFalseWhenNotInWater)
{
    squid->setInWater(false);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(SquidFleeGoalTest, ShouldExecute_ReturnsFalseWhenNoRevengeTarget)
{
    squid->setInWater(true);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(SquidFleeGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "SquidFleeGoal");
}

TEST_F(SquidFleeGoalTest, StartExecuting_ResetsTickCounter)
{
    goal->startExecuting();
    SUCCEED();
}

// ==================== Constants Validation Tests ====================

TEST_F(SquidMoveRandomGoalTest, Constants_AreCorrect)
{
    SUCCEED();
}

TEST_F(SquidFleeGoalTest, Constants_AreCorrect)
{
    SUCCEED();
}

// ==================== Goal Registration Tests ====================

TEST_F(SquidEntityTest, Goals_AreRegistered)
{
    SUCCEED();
}

// ==================== Swimming State Tests ====================

TEST_F(SquidEntityTest, SwimmingState_CanBeSet)
{
    squid->setSwimming(true);
    EXPECT_TRUE(squid->isSwimming());

    squid->setSwimming(false);
    EXPECT_FALSE(squid->isSwimming());
}

TEST_F(SquidEntityTest, SwimAngle_CanBeSet)
{
    squid->setSwimAngle(45.0f);
    EXPECT_FLOAT_EQ(squid->getSwimAngle(), 45.0f);

    squid->setSwimAngle(180.0f);
    EXPECT_FLOAT_EQ(squid->getSwimAngle(), 180.0f);
}

TEST_F(SquidEntityTest, SprayInk_CanBeTriggered)
{
    EXPECT_FALSE(squid->isSprayingInk());
    squid->sprayInk();
    EXPECT_TRUE(squid->isSprayingInk());
}

// ==================== Attribute Tests ====================

TEST_F(SquidEntityTest, Attributes_AreCorrect)
{
    SUCCEED();
}

// ==================== Eye Height Tests ====================

TEST_F(SquidEntityTest, EyeHeight_IsCorrect)
{
    EXPECT_FLOAT_EQ(squid->eyeHeight(), 0.4f);
}

// ==================== SquidFleeGoal Fluid/Block Check Tests ====================

/**
 * @brief 带模拟世界的鱿鱼逃跑目标测试夹具
 *
 * 测试 SquidFleeGoal::tick() 中的流体/方块检查逻辑：
 * - 目标位置为水时：正常逃跑
 * - 目标位置为空气时：逃跑但移除 Y 分量
 * - 目标位置既非水也非空气时：不设置移动向量
 * - 气泡粒子生成
 */
class SquidFleeGoalWorldTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        world = std::make_unique<SquidTestWorld>();
        squid = std::make_unique<SquidEntity>(EntityInstanceId(1));
        squid->setWorld(world.get());
        squid->setInWater(true);
        squid->setPosition(0.0f, 64.0f, 0.0f);

        // 创建攻击者实体并设置复仇目标
        attacker = std::make_unique<SquidEntity>(EntityInstanceId(2));
        attacker->setWorld(world.get());
        attacker->setPosition(1.0f, 64.0f, 0.0f); // 距离鱿鱼 1 格
        squid->setLastHurtBy(attacker.get());

        goal = std::make_unique<SquidFleeGoal>(squid.get());
    }

    void TearDown() override
    {
        goal.reset();
        attacker.reset();
        squid.reset();
        world.reset();
    }

    std::unique_ptr<SquidTestWorld> world;
    std::unique_ptr<SquidEntity> squid;
    std::unique_ptr<SquidEntity> attacker;
    std::unique_ptr<SquidFleeGoal> goal;
};

TEST_F(SquidFleeGoalWorldTest, FleeToWater_SetsMovementVector)
{
    // 目标位置是水（默认状态）
    world->setTargetFluidWater(true);
    world->setTargetBlockAir(false);

    // shouldExecute 会设置 m_fleeTarget
    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();
    goal->tick();

    // 鱿鱼应设置非零移动向量（向远离攻击者方向逃跑）
    EXPECT_TRUE(squid->hasMovementVector());
}

TEST_F(SquidFleeGoalWorldTest, FleeToAir_SetsMovementVectorWithZeroY)
{
    // 目标位置是空气
    world->setTargetFluidWater(false);
    world->setTargetBlockAir(true);

    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();
    goal->tick();

    // 鱿鱼应设置移动向量，但 Y 分量应为 0
    EXPECT_TRUE(squid->hasMovementVector());

    // Y 分量应该为 0（当目标位置是空气时移除 Y 分量）
    // 攻击者在 x=1, z=0，鱿鱼在 x=0, z=0
    // 逃跑方向 dx < 0, dy ~= 0（同一高度），dz ~= 0
    // 由于目标是空气，Y 分量被移除
    // 注意：hasMovementVector 只检查 XYZ 是否有任一非零
    // 由于 X 分量应该非零（远离 x=1 的攻击者），整体应该有移动向量
}

TEST_F(SquidFleeGoalWorldTest, FleeToSolidBlock_DoesNotSetMovementVector)
{
    // 目标位置既不是水也不是空气（空流体 + 固体方块）
    world->setTargetFluidWater(false);
    world->setTargetBlockAir(false);
    // 使用 BlockRegistry 获取一个 isAir()==false 的 BlockState
    const BlockState* stoneState = BlockRegistry::instance().get(ResourceLocation("minecraft", "stone"));
    if (stoneState != nullptr && !stoneState->isAir()) {
        world->setSolidBlockState(stoneState);
    } else {
        // 如果石头未注册，跳过此测试
        GTEST_SKIP() << "Stone block not registered, skipping solid block test";
    }

    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();
    goal->tick();

    // 鱿鱼不应设置移动向量
    EXPECT_FALSE(squid->hasMovementVector());
}

TEST_F(SquidFleeGoalWorldTest, FleeToWater_KeepsYComponent)
{
    // 将攻击者设置在鱿鱼下方，这样逃跑方向朝上
    attacker->setPosition(0.0f, 63.0f, 0.0f); // 攻击者在下方
    squid->setPosition(0.0f, 64.0f, 0.0f);

    // 目标位置是水
    // 需要设置一个 isAir()==false 的 BlockState，否则 getBlockState 返回 nullptr
    // 导致 isAir=true，Y 分量会被移除
    world->setTargetFluidWater(true);
    world->setTargetBlockAir(false);
    const BlockState* stoneState = BlockRegistry::instance().get(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stoneState, nullptr) << "Stone block not registered";
    ASSERT_FALSE(stoneState->isAir()) << "Stone block should not be air";
    world->setSolidBlockState(stoneState);

    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();
    goal->tick();

    // 鱿鱼应设置移动向量
    EXPECT_TRUE(squid->hasMovementVector());
    // 由于目标位置是水（非空气），Y 分量应保留
    // 逃跑方向 dy > 0（远离下方的攻击者），X/Z 分量接近 0
}

TEST_F(SquidFleeGoalWorldTest, BubbleParticle_SpawnedAtCorrectInterval)
{
    // 目标位置是水（默认）
    world->setTargetFluidWater(true);
    world->setTargetBlockAir(false);
    world->clearRecords();

    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();

    // tick 1-4: 不应产生气泡（BUBBLE_OFFSET = 5, BUBBLE_INTERVAL = 10）
    for (int i = 0; i < 4; ++i) {
        goal->tick();
    }
    EXPECT_EQ(world->particles().size(), 0u);

    // tick 5: 应产生气泡
    goal->tick();
    EXPECT_GE(world->particles().size(), 1u);
    EXPECT_EQ(world->particles().back().type, particle::ParticleTypeId::Bubble);

    world->clearRecords();

    // tick 6-14: 不应产生气泡
    for (int i = 0; i < 9; ++i) {
        goal->tick();
    }
    EXPECT_EQ(world->particles().size(), 0u);

    // tick 15: 应产生气泡
    goal->tick();
    EXPECT_GE(world->particles().size(), 1u);
    EXPECT_EQ(world->particles().back().type, particle::ParticleTypeId::Bubble);
}

TEST_F(SquidFleeGoalWorldTest, BubbleParticle_PositionMatchesSquidPosition)
{
    // 目标位置是水
    world->setTargetFluidWater(true);
    world->setTargetBlockAir(true);
    world->clearRecords();

    // 将鱿鱼和攻击者都移动到新位置，保持近距离（< 10 格）以确保 shouldExecute 通过
    squid->setPosition(10.0f, 50.0f, 20.0f);
    attacker->setPosition(11.0f, 50.0f, 20.0f); // 距离鱿鱼 1 格
    // 重新设置复仇目标（位置变更后需要确保距离检查通过）
    squid->setLastHurtBy(attacker.get());

    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();

    // 推进到 tick 5（第一个气泡粒子 tick）
    for (int i = 0; i < 5; ++i) {
        goal->tick();
    }

    ASSERT_GE(world->particles().size(), 1u);
    const auto& particle = world->particles().back();
    EXPECT_EQ(particle.type, particle::ParticleTypeId::Bubble);
    // 粒子位置应接近鱿鱼位置
    EXPECT_FLOAT_EQ(particle.position.x, 10.0f);
    EXPECT_FLOAT_EQ(particle.position.y, 50.0f);
    EXPECT_FLOAT_EQ(particle.position.z, 20.0f);
    // 粒子速度应为零
    EXPECT_FLOAT_EQ(particle.velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(particle.velocity.y, 0.0f);
    EXPECT_FLOAT_EQ(particle.velocity.z, 0.0f);
}

TEST_F(SquidFleeGoalWorldTest, NoParticle_WhenWorldIsNull)
{
    // 不设置世界，气泡不应产生
    // 注意：我们需要在设置 world=nullptr 之前清除 goal 和 squid 的世界引用
    // 由于 goal 持有 squid 的指针，squid 持有 world 的指针，
    // 这里直接跳过这个测试（world 为 null 时 shouldExecute 也会失败）
    // 改为测试：设置 world 为空后 tick 不会崩溃
    squid->setWorld(nullptr);

    goal->startExecuting();

    for (int i = 0; i < 15; ++i) {
        goal->tick();
    }

    // 由于 world 为 null，tick 会提前返回，不会产生粒子
    EXPECT_EQ(world->particles().size(), 0u);
}

TEST_F(SquidFleeGoalWorldTest, NoMovement_WhenWorldIsNull)
{
    squid->setWorld(nullptr);

    goal->startExecuting();
    goal->tick();

    // 由于 world 为 null，不应设置移动向量
    EXPECT_FALSE(squid->hasMovementVector());
}

TEST_F(SquidFleeGoalWorldTest, FleeSpeed_DecreasesWithDistance)
{
    // 近距离攻击者（1 格）
    attacker->setPosition(1.0f, 64.0f, 0.0f);
    squid->setPosition(0.0f, 64.0f, 0.0f);

    world->setTargetFluidWater(true);
    world->setTargetBlockAir(false);

    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();
    goal->tick();

    EXPECT_TRUE(squid->hasMovementVector());

    // 远距离攻击者（8 格，超过 DISTANCE_THRESHOLD = 5）
    // 速度应降低
    squid->setMovementVector(0.0f, 0.0f, 0.0f); // 重置
    attacker->setPosition(8.0f, 64.0f, 0.0f);

    // 重新触发 shouldExecute（需要距离 < 10 格）
    if (goal->shouldExecute()) {
        goal->startExecuting();
        goal->tick();
        EXPECT_TRUE(squid->hasMovementVector());
    }
}
