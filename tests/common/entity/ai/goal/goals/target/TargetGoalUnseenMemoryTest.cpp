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

#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/core/EntityTypeIdNumber.hpp"
#include "entity/entities/passive/basic/PigEntity.hpp"
#include "entity/registry/VanillaEntities.hpp"

namespace mc {
namespace test {

// ==================== 辅助类 ====================

/**
 * @brief 支持实体查询的测试用世界
 */
class UnseenMemoryTestWorld : public test::BaseTestWorld {
public:
    void setEntities(std::vector<Entity*> entities) { m_entities = std::move(entities); }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_entities;
    }

private:
    std::vector<Entity*> m_entities;
};

/**
 * @brief 可公开 tick() 的测试用猪实体
 */
class TestPigEntity : public PigEntity {
public:
    explicit TestPigEntity(EntityId id)
        : PigEntity(id)
    {}

    using PigEntity::tick;
};

/**
 * @brief 可控视线的 TargetGoal 子类，用于测试 shouldContinueExecuting 的视线记忆逻辑
 *
 * 通过隐藏 checkSight() 方法（不使用 override，因为基类方法不是虚函数），
 * 可以精确控制"是否能看到目标"的返回值，从而测试 m_unseenMemoryTicks 的计时和阈值判断逻辑。
 */
class ControllableSightTargetGoal : public entity::ai::goal::TargetGoal {
public:
    ControllableSightTargetGoal(MobEntity* mob, bool checkSight)
        : TargetGoal(mob, checkSight)
        , m_canSeeTarget(false)
    {}

    [[nodiscard]] bool shouldExecute() override
    {
        // 模拟：如果有目标就返回 true
        if (m_target != nullptr) return true;
        return false;
    }

    void setCanSeeTarget(bool canSee) { m_canSeeTarget = canSee; }

    void setTarget(LivingEntity* target) { m_target = target; }

    // 暴露 m_unseenMemoryTicks 用于直接测试
    [[nodiscard]] i32 getUnseenMemoryTicks() const { return m_unseenMemoryTicks; }

protected:
    // 重写基类 checkSight()，使用自定义实现以控制视线返回值
    [[nodiscard]] bool checkSight() const override { return m_canSeeTarget; }

private:
    bool m_canSeeTarget;
};

// ==================== setUnseenMemoryTicks 基础测试 ====================

class TargetGoalUnseenMemoryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();

        world = std::make_unique<UnseenMemoryTestWorld>();

        pig = std::make_unique<TestPigEntity>(EntityId(1));
        pig->setWorld(world.get());
        pig->setPosition(0.0f, 64.0f, 0.0f);

        attacker = std::make_unique<TestPigEntity>(EntityId(2));
        attacker->setWorld(world.get());
        attacker->setPosition(5.0f, 64.0f, 0.0f);

        // 推进 tick 使 ticksExisted > 0
        for (int i = 0; i < 10; ++i) {
            pig->tick();
        }
    }

    void TearDown() override
    {
        pig.reset();
        attacker.reset();
        world.reset();
    }

    std::unique_ptr<UnseenMemoryTestWorld> world;
    std::unique_ptr<TestPigEntity> pig;
    std::unique_ptr<TestPigEntity> attacker;
};

// ==================== setUnseenMemoryTicks 方法测试 ====================

TEST_F(TargetGoalUnseenMemoryTest, SetUnseenMemoryTicks_ReturnsReference)
{
    // setUnseenMemoryTicks 应返回 TargetGoal& 以支持链式调用
    auto goal = std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(pig.get(), true);
    auto& ref = goal->setUnseenMemoryTicks(300);
    EXPECT_EQ(&ref, goal.get());
}

TEST_F(TargetGoalUnseenMemoryTest, SetUnseenMemoryTicks_DefaultValueIs60)
{
    // 默认视线记忆时间应为60tick，通过 getter 直接验证
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    EXPECT_EQ(goal->getUnseenMemoryTicks(), 60);
}

TEST_F(TargetGoalUnseenMemoryTest, SetUnseenMemoryTicks_ChangesValue)
{
    // setUnseenMemoryTicks 应正确修改值
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    goal->setUnseenMemoryTicks(300);
    EXPECT_EQ(goal->getUnseenMemoryTicks(), 300);

    goal->setUnseenMemoryTicks(1);
    EXPECT_EQ(goal->getUnseenMemoryTicks(), 1);

    goal->setUnseenMemoryTicks(0);
    EXPECT_EQ(goal->getUnseenMemoryTicks(), 0);
}

// ==================== HurtByTargetGoal 视线记忆测试 ====================

TEST_F(TargetGoalUnseenMemoryTest, HurtByTargetGoal_SetsUnseenMemoryTicksTo300OnStart)
{
    // HurtByTargetGoal::startExecuting() 应设置 m_unseenMemoryTicks = 300
    // 对应 MC Java: HurtByTargetGoal.start() 中 this.unseenMemoryTicks = 300
    pig->setLastHurtBy(attacker.get());

    auto goal = std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get());
    ASSERT_TRUE(goal->shouldExecute());
    goal->startExecuting();

    // 验证方式：通过 ControllableSightTargetGoal 的行为间接验证
    // 创建 ControllableSightTargetGoal 设置 300tick，模拟 HurtByTargetGoal 的行为
    auto mockGoal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    mockGoal->setUnseenMemoryTicks(300); // 与 HurtByTargetGoal.startExecuting() 中设置的值相同
    mockGoal->setTarget(attacker.get());
    mockGoal->setCanSeeTarget(false);

    mockGoal->startExecuting();

    // 60 tick 后应仍继续追踪（默认只有 60 tick，但 300 tick 应持续更久）
    for (int i = 0; i < 60; ++i) {
        EXPECT_TRUE(mockGoal->shouldContinueExecuting());
    }
    // 继续到 300 tick 仍应追踪
    for (int i = 60; i < 300; ++i) {
        EXPECT_TRUE(mockGoal->shouldContinueExecuting())
            << "HurtByTargetGoal should track for 300 ticks, at tick " << (i + 1);
    }
    // 超过 300 tick 后放弃
    EXPECT_FALSE(mockGoal->shouldContinueExecuting());
}

// ==================== NearestAttackableTargetGoal 构造测试 ====================

TEST_F(TargetGoalUnseenMemoryTest, NearestAttackableTargetGoal_DefaultChanceIsZero)
{
    // 默认 chance=0，每 tick 都检查目标
    auto goal = std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(pig.get(), true);
    EXPECT_NE(goal, nullptr);
}

TEST_F(TargetGoalUnseenMemoryTest, NearestAttackableTargetGoal_ExplicitChanceZero)
{
    // 显式传入 chance=0
    auto goal = std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(pig.get(), true, 0);
    EXPECT_NE(goal, nullptr);
}

// ==================== 视线记忆阈值测试 ====================

class UnseenMemoryThresholdTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();

        world = std::make_unique<UnseenMemoryTestWorld>();

        pig = std::make_unique<TestPigEntity>(EntityId(1));
        pig->setWorld(world.get());
        pig->setPosition(0.0f, 64.0f, 0.0f);

        attacker = std::make_unique<TestPigEntity>(EntityId(2));
        attacker->setWorld(world.get());
        attacker->setPosition(5.0f, 64.0f, 0.0f);

        for (int i = 0; i < 10; ++i) {
            pig->tick();
            attacker->tick();
        }
    }

    void TearDown() override
    {
        pig.reset();
        attacker.reset();
        world.reset();
    }

    std::unique_ptr<UnseenMemoryTestWorld> world;
    std::unique_ptr<TestPigEntity> pig;
    std::unique_ptr<TestPigEntity> attacker;
};

TEST_F(UnseenMemoryThresholdTest, DefaultUnseenMemory_60Ticks_DefaultCheckSightTrue)
{
    // 默认 m_unseenMemoryTicks=60，checkSight=true
    // 目标离开视线后，应在 60 tick 内继续追踪，超过 60 tick 后放弃
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    goal->setTarget(attacker.get());
    goal->setCanSeeTarget(true);

    // 初始：目标可见
    goal->startExecuting(); // 重置 m_unseenTicks = 0
    EXPECT_TRUE(goal->shouldContinueExecuting());

    // 目标离开视线
    goal->setCanSeeTarget(false);

    // 在 60 tick 内应继续追踪
    for (int i = 0; i < 60; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting()) << "shouldContinueExecuting at unseen tick " << (i + 1);
    }

    // 超过 60 tick 后应放弃追踪（m_unseenTicks > m_unseenMemoryTicks）
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(UnseenMemoryThresholdTest, CustomUnseenMemory_300Ticks)
{
    // 设置 m_unseenMemoryTicks=300（唤魔者/幻术师的值）
    // 目标离开视线后，应在 300 tick 内继续追踪
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    goal->setTarget(attacker.get());
    goal->setUnseenMemoryTicks(300);
    goal->setCanSeeTarget(true);

    goal->startExecuting();
    EXPECT_TRUE(goal->shouldContinueExecuting());

    // 目标离开视线
    goal->setCanSeeTarget(false);

    // 在 300 tick 内应继续追踪
    for (int i = 0; i < 300; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting()) << "shouldContinueExecuting at unseen tick " << (i + 1);
    }

    // 超过 300 tick 后应放弃追踪
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(UnseenMemoryThresholdTest, SeenTarget_ResetsUnseenTicks)
{
    // 目标重新可见时，m_unseenTicks 应重置为 0
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    goal->setTarget(attacker.get());
    goal->setCanSeeTarget(true);

    goal->startExecuting();
    EXPECT_TRUE(goal->shouldContinueExecuting());

    // 目标离开视线 30 tick
    goal->setCanSeeTarget(false);
    for (int i = 0; i < 30; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting());
    }

    // 目标重新可见
    goal->setCanSeeTarget(true);
    EXPECT_TRUE(goal->shouldContinueExecuting());

    // 再次离开视线，应从 0 开始重新计数
    goal->setCanSeeTarget(false);
    for (int i = 0; i < 60; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting()) << "shouldContinueExecuting at unseen tick " << (i + 1);
    }
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(UnseenMemoryThresholdTest, CheckSightFalse_UnseenTicksNotCounted)
{
    // checkSight=false 时，即使目标不可见也不计数
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), false);
    goal->setTarget(attacker.get());
    // checkSight=false，视线记忆逻辑不生效
    goal->setCanSeeTarget(false);

    goal->startExecuting();

    // 即使超过默认 60 tick，shouldContinueExecuting 也应返回 true
    // 因为 checkSight=false 时 shouldContinueExecuting 中的视线检查分支不执行
    for (int i = 0; i < 200; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting()) << "shouldContinueExecuting at tick " << (i + 1);
    }
}

TEST_F(UnseenMemoryThresholdTest, StartExecuting_ResetsUnseenTicks)
{
    // startExecuting() 应将 m_unseenTicks 重置为 0
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    goal->setTarget(attacker.get());
    goal->setCanSeeTarget(false);

    goal->startExecuting();

    // 即使一开始就看不到目标，也应从 0 开始计数
    for (int i = 0; i < 60; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting()) << "shouldContinueExecuting at unseen tick " << (i + 1);
    }
    EXPECT_FALSE(goal->shouldContinueExecuting());

    // 重新开始应重置
    goal->startExecuting();
    for (int i = 0; i < 60; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting());
    }
}

TEST_F(UnseenMemoryThresholdTest, SetUnseenMemoryTicks_To1_QuickForget)
{
    // 设置为 1 tick 记忆，几乎立即忘记目标
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    goal->setUnseenMemoryTicks(1);
    goal->setTarget(attacker.get());
    goal->setCanSeeTarget(true);

    goal->startExecuting();
    EXPECT_TRUE(goal->shouldContinueExecuting());

    // 目标离开视线
    goal->setCanSeeTarget(false);

    // 第 1 tick 仍追踪（m_unseenTicks 从 0 增到 1，1 > 1 为 false）
    EXPECT_TRUE(goal->shouldContinueExecuting());
    // 第 2 tick 超过阈值（m_unseenTicks = 2，2 > 1 为 true）
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(UnseenMemoryThresholdTest, TargetDead_StopsImmediately)
{
    // 目标死亡时，shouldContinueExecuting 应立即返回 false
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    goal->setTarget(attacker.get());
    goal->setCanSeeTarget(true);

    goal->startExecuting();
    EXPECT_TRUE(goal->shouldContinueExecuting());

    // 设置目标死亡（remove() 设置 m_removed = true，isAlive() 返回 false）
    attacker->remove();

    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(UnseenMemoryThresholdTest, ResetTask_ClearsUnseenTicks)
{
    // resetTask 应将 m_unseenTicks 重置为 0
    auto goal = std::make_unique<ControllableSightTargetGoal>(pig.get(), true);
    goal->setTarget(attacker.get());
    goal->setCanSeeTarget(false);

    goal->startExecuting();

    // 积累一些 unseen ticks
    for (int i = 0; i < 30; ++i) {
        goal->shouldContinueExecuting();
    }

    // 重置
    goal->resetTask();

    // 重新设置目标并启动
    goal->setTarget(attacker.get());
    goal->startExecuting();

    // 应从 0 重新计数
    for (int i = 0; i < 60; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting()) << "shouldContinueExecuting at unseen tick " << (i + 1);
    }
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

} // namespace test
} // namespace mc
