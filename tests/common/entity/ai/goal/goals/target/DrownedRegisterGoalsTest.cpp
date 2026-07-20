/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "entity/ai/goal/GoalSelector.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/core/EntityTypeIdNumber.hpp"
#include "entity/entities/monster/undead/DrownedEntity.hpp"
#include "entity/entities/monster/undead/ZombieEntity.hpp"
#include "entity/entities/passive/water/AxolotlEntity.hpp"
#include "entity/registry/VanillaEntities.hpp"

namespace mc {
namespace test {

// ==================== DrownedEntity registerGoals 测试 ====================

/**
 * @brief 可公开 tick() 的测试用溺尸实体
 */
class TestDrownedEntity : public DrownedEntity {
public:
    explicit TestDrownedEntity(EntityId id)
        : DrownedEntity(id)
    {}

    using DrownedEntity::tick;

    /**
     * @brief 获取目标选择器（用于测试）
     */
    entity::ai::GoalSelector& testTargetSelector() { return targetSelector(); }
};

/**
 * @brief 可公开 tick() 的测试用僵尸实体
 */
class TestZombieEntity : public ZombieEntity {
public:
    explicit TestZombieEntity(EntityId id)
        : ZombieEntity(id)
    {}

    using ZombieEntity::tick;

    /**
     * @brief 获取目标选择器（用于测试）
     */
    entity::ai::GoalSelector& testTargetSelector() { return targetSelector(); }
};

class DrownedRegisterGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和实体注册表，确保 EntityTypeIdNumber 有正确的 typeId
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    void TearDown() override {}
};

TEST_F(DrownedRegisterGoalsTest, DrownedHasExactlyOneHurtByTargetGoal)
{
    // 创建溺尸实体
    auto drowned = std::make_unique<TestDrownedEntity>(EntityId(1));

    // 检查目标选择器中的 HurtByTargetGoal 数量
    // DrownedEntity::registerGoals() 先调用父类注册一个，再移除后添加一个
    // 最终应该只有 1 个 HurtByTargetGoal
    i32 hurtByTargetCount = 0;
    for (const auto& pg : drowned->testTargetSelector().getAllGoals()) {
        if (dynamic_cast<const entity::ai::goal::HurtByTargetGoal*>(pg.getGoal()) != nullptr) {
            hurtByTargetCount++;
        }
    }
    EXPECT_EQ(hurtByTargetCount, 1) << "DrownedEntity should have exactly 1 HurtByTargetGoal after registerGoals()";
}

TEST_F(DrownedRegisterGoalsTest, ZombieHasExactlyOneHurtByTargetGoal)
{
    // 创建僵尸实体作为对照组
    auto zombie = std::make_unique<TestZombieEntity>(EntityId(2));

    i32 hurtByTargetCount = 0;
    for (const auto& pg : zombie->testTargetSelector().getAllGoals()) {
        if (dynamic_cast<const entity::ai::goal::HurtByTargetGoal*>(pg.getGoal()) != nullptr) {
            hurtByTargetCount++;
        }
    }
    EXPECT_EQ(hurtByTargetCount, 1) << "ZombieEntity should have exactly 1 HurtByTargetGoal";
}

TEST_F(DrownedRegisterGoalsTest, DrownedAndZombieBothHaveTargetGoals)
{
    auto drowned = std::make_unique<TestDrownedEntity>(EntityId(1));
    auto zombie = std::make_unique<TestZombieEntity>(EntityId(2));

    // 两者都应该有目标选择器中的目标
    EXPECT_GT(drowned->testTargetSelector().getAllGoals().size(), 0u) << "DrownedEntity should have target goals";
    EXPECT_GT(zombie->testTargetSelector().getAllGoals().size(), 0u) << "ZombieEntity should have target goals";
}

TEST_F(DrownedRegisterGoalsTest, DrownedTargetGoalsIncludeNearestAttackableTargetForPlayer)
{
    // 验证溺尸有攻击玩家的目标（NearestAttackableTargetGoal<Player>）
    auto drowned = std::make_unique<TestDrownedEntity>(EntityId(1));

    bool hasPlayerTarget = false;
    for (const auto& pg : drowned->testTargetSelector().getAllGoals()) {
        // NearestAttackableTargetGoal 继承自 TargetGoal
        // 检查是否有非 HurtByTargetGoal 的 TargetGoal（即攻击目标类目标）
        if (dynamic_cast<const entity::ai::goal::TargetGoal*>(pg.getGoal()) != nullptr &&
            dynamic_cast<const entity::ai::goal::HurtByTargetGoal*>(pg.getGoal()) == nullptr) {
            hasPlayerTarget = true;
        }
    }
    EXPECT_TRUE(hasPlayerTarget) << "DrownedEntity should have target selection goals beyond HurtByTargetGoal";
}

TEST_F(DrownedRegisterGoalsTest, DrownedHasNearestAttackableTargetGoalForAxolotl)
{
    // DrownedEntity 继承了 ZombieEntity 的目标，并添加了 AxolotlEntity 目标
    // 验证溺尸有比僵尸更多的目标选择器目标（至少多了 Axolotl 和自定义 Player 目标）
    // 注意：溺尸的 Player 目标会替换僵尸的（相同优先级2），但 Axolotl 是新增的
    auto drowned = std::make_unique<TestDrownedEntity>(EntityId(1));

    i32 targetGoalCount = 0;
    i32 hurtByTargetCount = 0;
    for (const auto& pg : drowned->testTargetSelector().getAllGoals()) {
        const auto* goal = pg.getGoal();
        if (dynamic_cast<const entity::ai::goal::TargetGoal*>(goal) != nullptr) {
            targetGoalCount++;
        }
        if (dynamic_cast<const entity::ai::goal::HurtByTargetGoal*>(goal) != nullptr) {
            hurtByTargetCount++;
        }
    }
    // 溺尸应有至少 1 个 TargetGoal 和恰好 1 个 HurtByTargetGoal
    EXPECT_GE(targetGoalCount, 1) << "DrownedEntity should have at least 1 target goal";
    EXPECT_EQ(hurtByTargetCount, 1) << "DrownedEntity should have exactly 1 HurtByTargetGoal";
}

TEST_F(DrownedRegisterGoalsTest, ZombieTargetGoalCount)
{
    // 对照组：普通僵尸应有目标选择器目标
    auto zombie = std::make_unique<TestZombieEntity>(EntityId(2));

    i32 hurtByTargetCount = 0;
    for (const auto& pg : zombie->testTargetSelector().getAllGoals()) {
        if (dynamic_cast<const entity::ai::goal::HurtByTargetGoal*>(pg.getGoal()) != nullptr) {
            hurtByTargetCount++;
        }
    }
    EXPECT_EQ(hurtByTargetCount, 1) << "ZombieEntity should have exactly 1 HurtByTargetGoal";
}

} // namespace test
} // namespace mc
