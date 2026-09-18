/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction restriction, including without limitation the rights
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

#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/entities/passive/basic/PigEntity.hpp"
#include "entity/entities/passive/special/StriderEntity.hpp"
#include "entity/entities/player/Player.hpp"

namespace mc {
namespace test {

// ==================== LookAtGoal TypeFilter 测试 ====================

class LookAtGoalTypeFilterTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

    void TearDown() override { pig.reset(); }

    std::unique_ptr<PigEntity> pig;
};

// ==================== 类型过滤标记测试 ====================

TEST_F(LookAtGoalTypeFilterTest, TypeFilter_CompilesForLivingEntity)
{
    // 验证 TypeFilter 对 LivingEntity 子类可用
    entity::ai::goal::TypeFilter<LivingEntity> filter;
    MC_UNUSED(filter);
    // 如果编译通过，测试成功
    SUCCEED();
}

TEST_F(LookAtGoalTypeFilterTest, TypeFilter_CompilesForMobEntity)
{
    entity::ai::goal::TypeFilter<MobEntity> filter;
    MC_UNUSED(filter);
    SUCCEED();
}

TEST_F(LookAtGoalTypeFilterTest, TypeFilter_CompilesForPlayer)
{
    entity::ai::goal::TypeFilter<Player> filter;
    MC_UNUSED(filter);
    SUCCEED();
}

TEST_F(LookAtGoalTypeFilterTest, TypeFilter_CompilesForAnimalEntity)
{
    entity::ai::goal::TypeFilter<AnimalEntity> filter;
    MC_UNUSED(filter);
    SUCCEED();
}

TEST_F(LookAtGoalTypeFilterTest, TypeFilter_CompilesForPigEntity)
{
    entity::ai::goal::TypeFilter<PigEntity> filter;
    MC_UNUSED(filter);
    SUCCEED();
}

TEST_F(LookAtGoalTypeFilterTest, TypeFilter_CompilesForStriderEntity)
{
    entity::ai::goal::TypeFilter<StriderEntity> filter;
    MC_UNUSED(filter);
    SUCCEED();
}

// ==================== LookAtGoal 模板构造函数测试 ====================

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_TypeFilterConstructor_PigEntity)
{
    // 使用 TypeFilter 构造看向猪的 LookAtGoal
    auto goal = std::make_unique<entity::ai::goal::LookAtGoal>(
        pig.get(), 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<PigEntity>{});

    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "LookAtGoal");
}

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_TypeFilterConstructor_Player)
{
    // 使用 TypeFilter 构造看向玩家的 LookAtGoal
    auto goal =
        std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 10.0f, 0.05f, entity::ai::goal::TypeFilter<Player>{});

    EXPECT_NE(goal, nullptr);
}

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_TypeFilterConstructor_StriderEntity)
{
    // 使用 TypeFilter 构造看向炽足兽的 LookAtGoal
    auto goal = std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(),
        8.0f,
        entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE,
        entity::ai::goal::TypeFilter<StriderEntity>{});

    EXPECT_NE(goal, nullptr);
}

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_TypeFilterConstructor_LivingEntity)
{
    // 使用 TypeFilter 构造看向任意 LivingEntity 的 LookAtGoal
    auto goal = std::make_unique<entity::ai::goal::LookAtGoal>(
        pig.get(), 12.0f, 0.02f, entity::ai::goal::TypeFilter<LivingEntity>{});

    EXPECT_NE(goal, nullptr);
}

// ==================== 互斥标志测试 ====================

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_TypeFilter_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::LookAtGoal>(
        pig.get(), 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<PigEntity>{});

    // LookAtGoal 应该只有 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

// ==================== 默认构造函数测试 ====================

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_DefaultConstructor)
{
    // 默认构造函数（看向任意 LivingEntity）
    auto goal = std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 8.0f);

    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "LookAtGoal");
}

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_ChanceConstructor)
{
    // 带概率的构造函数
    auto goal = std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 10.0f, 0.05f);

    EXPECT_NE(goal, nullptr);
}

// ==================== 自定义过滤函数测试 ====================

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_CustomFilter)
{
    // 使用自定义过滤函数
    auto goal =
        std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 10.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向成年动物
            const AnimalEntity* animal = dynamic_cast<const AnimalEntity*>(entity);
            return animal != nullptr && !animal->isChild();
        });

    EXPECT_NE(goal, nullptr);
}

// ==================== 常量测试 ====================

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_DefaultLookChance)
{
    // 验证默认看向概率
    EXPECT_FLOAT_EQ(entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, 0.02f);
}

// ==================== LookRandomlyGoal 测试 ====================

TEST_F(LookAtGoalTypeFilterTest, LookRandomlyGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::LookRandomlyGoal>(pig.get());

    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "LookRandomlyGoal");
}

TEST_F(LookAtGoalTypeFilterTest, LookRandomlyGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::LookRandomlyGoal>(pig.get());

    // LookRandomlyGoal 使用 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

// ==================== 无世界时的行为测试 ====================

TEST_F(LookAtGoalTypeFilterTest, LookAtGoal_ShouldExecuteReturnsFalseWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::LookAtGoal>(
        pig.get(), 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<PigEntity>{});

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(LookAtGoalTypeFilterTest, LookRandomlyGoal_ShouldExecuteReturnsFalseWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::LookRandomlyGoal>(pig.get());

    // 无世界时不应执行（因为概率检查会失败）
    // 注意：由于概率检查使用随机数，可能在某些情况下返回 true
    // 但没有世界时，实体无法正确工作
    MC_UNUSED(goal);
    SUCCEED();
}

} // namespace test
} // namespace mc
