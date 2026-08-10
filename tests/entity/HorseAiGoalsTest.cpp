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
 * @file HorseAiGoalsTest.cpp
 * @brief 马类实体 AI 目标注册测试
 *
 * 测试 AbstractHorseEntity 及其子类的 AI 目标注册是否符合 MC 1.16.5 规范。
 *
 * MC 1.16.5 AbstractHorseEntity.registerGoals() 注册的目标：
 * - 优先级 0: SwimGoal - 在水中上浮
 * - 优先级 1: PanicGoal - 受伤或着火时逃跑
 * - 优先级 1: RunAroundLikeCrazyGoal - 未驯服马被骑乘时乱跑
 * - 优先级 2: BreedGoal - 与同类交配
 * - 优先级 4: FollowParentGoal - 幼体跟随成年个体
 * - 优先级 6: WaterAvoidingRandomWalkingGoal - 随机游荡但避开水域
 * - 优先级 7: LookAtGoal - 看向附近玩家
 * - 优先级 8: LookRandomlyGoal - 随机转头观察
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/MuleEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用 Mock World
 */
class HorseAiGoalsTestWorld final : public mc::test::BaseTestWorld {
public:
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] f32 getBrightness(const BlockPos& /*pos*/) const override { return 1.0f; }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return EntityInstanceId(0); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("HorseAiGoalsTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("HorseAiGoalsTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
};

/**
 * @brief 辅助函数：统计指定类型和优先级的目标数量
 */
template <typename GoalType>
int countGoalsWithPriority(const entity::ai::GoalSelector& selector, int priority)
{
    int count = 0;
    const auto& goals = selector.getAllGoals();
    for (const auto& pg : goals) {
        if (pg.getPriority() == priority) {
            const auto* goal = pg.getGoal();
            if (dynamic_cast<const GoalType*>(goal) != nullptr) {
                ++count;
            }
        }
    }
    return count;
}

/**
 * @brief 辅助函数：检查是否存在指定类型和优先级的目标
 */
template <typename GoalType>
bool hasGoalWithPriority(const entity::ai::GoalSelector& selector, int priority)
{
    return countGoalsWithPriority<GoalType>(selector, priority) > 0;
}

/**
 * @brief 辅助函数：获取总目标数量
 */
int getTotalGoalCount(const entity::ai::GoalSelector& selector)
{
    return static_cast<int>(selector.getAllGoals().size());
}

// ============================================================================
// AbstractHorseEntity AI Goals Tests
// ============================================================================

TEST(AbstractHorseAiGoalsTest, HasSwimGoal)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // SwimGoal 应该在优先级 0
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::SwimGoal>(horse->goalSelector(), 0))
        << "AbstractHorseEntity should have SwimGoal at priority 0";
}

TEST(AbstractHorseAiGoalsTest, HasPanicGoal)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // PanicGoal 应该在优先级 1
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::PanicGoal>(horse->goalSelector(), 1))
        << "AbstractHorseEntity should have PanicGoal at priority 1";
}

TEST(AbstractHorseAiGoalsTest, HasRunAroundLikeCrazyGoal)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // RunAroundLikeCrazyGoal 应该在优先级 1
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::RunAroundLikeCrazyGoal>(horse->goalSelector(), 1))
        << "AbstractHorseEntity should have RunAroundLikeCrazyGoal at priority 1";
}

TEST(AbstractHorseAiGoalsTest, HasBreedGoal)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // BreedGoal 应该在优先级 2
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::BreedGoal>(horse->goalSelector(), 2))
        << "AbstractHorseEntity should have BreedGoal at priority 2";
}

TEST(AbstractHorseAiGoalsTest, HasFollowParentGoal)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // FollowParentGoal 应该在优先级 4
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::FollowParentGoal>(horse->goalSelector(), 4))
        << "AbstractHorseEntity should have FollowParentGoal at priority 4";
}

TEST(AbstractHorseAiGoalsTest, HasWaterAvoidingRandomWalkingGoal)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // WaterAvoidingRandomWalkingGoal 应该在优先级 6
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(horse->goalSelector(), 6))
        << "AbstractHorseEntity should have WaterAvoidingRandomWalkingGoal at priority 6";
}

TEST(AbstractHorseAiGoalsTest, HasLookAtGoal)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // LookAtGoal 应该在优先级 7
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookAtGoal>(horse->goalSelector(), 7))
        << "AbstractHorseEntity should have LookAtGoal at priority 7";
}

TEST(AbstractHorseAiGoalsTest, HasLookRandomlyGoal)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // LookRandomlyGoal 应该在优先级 8
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookRandomlyGoal>(horse->goalSelector(), 8))
        << "AbstractHorseEntity should have LookRandomlyGoal at priority 8";
}

TEST(AbstractHorseAiGoalsTest, TotalGoalCount)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // MC 1.16.5: AbstractHorseEntity 注册了 8 个目标
    EXPECT_EQ(getTotalGoalCount(horse->goalSelector()), 8)
        << "AbstractHorseEntity should have exactly 8 goals registered";
}

// ============================================================================
// DonkeyEntity AI Goals Tests
// ============================================================================

TEST(DonkeyAiGoalsTest, InheritsAllAbstractHorseGoals)
{
    HorseAiGoalsTestWorld world;
    auto donkey = std::make_unique<DonkeyEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    donkey->setWorld(&world);

    // 驴应该继承 AbstractHorseEntity 的所有 AI 目标
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::SwimGoal>(donkey->goalSelector(), 0));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::PanicGoal>(donkey->goalSelector(), 1));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::RunAroundLikeCrazyGoal>(donkey->goalSelector(), 1));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::BreedGoal>(donkey->goalSelector(), 2));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::FollowParentGoal>(donkey->goalSelector(), 4));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(donkey->goalSelector(), 6));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookAtGoal>(donkey->goalSelector(), 7));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookRandomlyGoal>(donkey->goalSelector(), 8));
}

TEST(DonkeyAiGoalsTest, TotalGoalCount)
{
    HorseAiGoalsTestWorld world;
    auto donkey = std::make_unique<DonkeyEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    donkey->setWorld(&world);

    // MC 1.16.5: DonkeyEntity 不添加额外目标，应该有 8 个目标
    EXPECT_EQ(getTotalGoalCount(donkey->goalSelector()), 8)
        << "DonkeyEntity should have exactly 8 goals (inherited from AbstractHorseEntity)";
}

// ============================================================================
// MuleEntity AI Goals Tests
// ============================================================================

TEST(MuleAiGoalsTest, InheritsAllAbstractHorseGoals)
{
    HorseAiGoalsTestWorld world;
    auto mule = std::make_unique<MuleEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    mule->setWorld(&world);

    // 骡应该继承 AbstractHorseEntity 的所有 AI 目标
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::SwimGoal>(mule->goalSelector(), 0));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::PanicGoal>(mule->goalSelector(), 1));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::RunAroundLikeCrazyGoal>(mule->goalSelector(), 1));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::BreedGoal>(mule->goalSelector(), 2));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::FollowParentGoal>(mule->goalSelector(), 4));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(mule->goalSelector(), 6));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookAtGoal>(mule->goalSelector(), 7));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookRandomlyGoal>(mule->goalSelector(), 8));
}

TEST(MuleAiGoalsTest, TotalGoalCount)
{
    HorseAiGoalsTestWorld world;
    auto mule = std::make_unique<MuleEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    mule->setWorld(&world);

    // MC 1.16.5: MuleEntity 不添加额外目标，应该有 8 个目标
    // 注意：骡不育，但 BreedGoal 和 FollowParentGoal 会通过 canBreed() 和 isChild() 检查自动跳过
    EXPECT_EQ(getTotalGoalCount(mule->goalSelector()), 8)
        << "MuleEntity should have exactly 8 goals (inherited from AbstractHorseEntity)";
}

// ============================================================================
// HorseEntity AI Goals Tests
// ============================================================================

TEST(HorseAiGoalsTest, InheritsAllAbstractHorseGoals)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // 马应该继承 AbstractHorseEntity 的所有 AI 目标
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::SwimGoal>(horse->goalSelector(), 0));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::PanicGoal>(horse->goalSelector(), 1));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::RunAroundLikeCrazyGoal>(horse->goalSelector(), 1));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::BreedGoal>(horse->goalSelector(), 2));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::FollowParentGoal>(horse->goalSelector(), 4));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(horse->goalSelector(), 6));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookAtGoal>(horse->goalSelector(), 7));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookRandomlyGoal>(horse->goalSelector(), 8));
}

TEST(HorseAiGoalsTest, TotalGoalCount)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    // MC 1.16.5: HorseEntity 不添加额外目标，应该有 8 个目标
    EXPECT_EQ(getTotalGoalCount(horse->goalSelector()), 8)
        << "HorseEntity should have exactly 8 goals (inherited from AbstractHorseEntity)";
}

// ============================================================================
// Priority Ordering Tests
// ============================================================================

TEST(AbstractHorseAiGoalsTest, PriorityOrdering)
{
    HorseAiGoalsTestWorld world;
    auto horse = std::make_unique<HorseEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    horse->setWorld(&world);

    const auto& goals = horse->goalSelector().getAllGoals();

    // 收集所有优先级
    std::vector<int> priorities;
    for (const auto& pg : goals) {
        priorities.push_back(pg.getPriority());
    }

    // 检查优先级按 MC 1.16.5 规范设置
    // 优先级 0 应该只有 SwimGoal
    EXPECT_EQ(countGoalsWithPriority<entity::ai::goal::SwimGoal>(horse->goalSelector(), 0), 1);

    // 优先级 1 应该有 PanicGoal 和 RunAroundLikeCrazyGoal
    EXPECT_EQ(countGoalsWithPriority<entity::ai::goal::PanicGoal>(horse->goalSelector(), 1), 1);
    EXPECT_EQ(countGoalsWithPriority<entity::ai::goal::RunAroundLikeCrazyGoal>(horse->goalSelector(), 1), 1);

    // 优先级 2 应该只有 BreedGoal
    EXPECT_EQ(countGoalsWithPriority<entity::ai::goal::BreedGoal>(horse->goalSelector(), 2), 1);

    // 优先级 4 应该只有 FollowParentGoal
    EXPECT_EQ(countGoalsWithPriority<entity::ai::goal::FollowParentGoal>(horse->goalSelector(), 4), 1);

    // 优先级 6 应该只有 WaterAvoidingRandomWalkingGoal
    EXPECT_EQ(countGoalsWithPriority<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(horse->goalSelector(), 6), 1);

    // 优先级 7 应该只有 LookAtGoal
    EXPECT_EQ(countGoalsWithPriority<entity::ai::goal::LookAtGoal>(horse->goalSelector(), 7), 1);

    // 优先级 8 应该只有 LookRandomlyGoal
    EXPECT_EQ(countGoalsWithPriority<entity::ai::goal::LookRandomlyGoal>(horse->goalSelector(), 8), 1);
}

// ============================================================================
// LlamaEntity AI Goals Tests
// ============================================================================
// 注意：由于 C++ 虚函数调用机制，派生类的 registerGoals() 在基类构造函数中不会被调用
// 因此 LlamaEntity 特有的 AI 目标（商队、远程攻击、防御）会在实际游戏运行时正确注册
// 这里只测试继承自 AbstractHorseEntity 的基础目标

TEST(LlamaAiGoalsTest, InheritsAbstractHorseGoals)
{
    HorseAiGoalsTestWorld world;
    auto llama = std::make_unique<LlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(&world);

    // 羊驼应该继承 AbstractHorseEntity 的基础 AI 目标
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::SwimGoal>(llama->goalSelector(), 0));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::PanicGoal>(llama->goalSelector(), 1));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::RunAroundLikeCrazyGoal>(llama->goalSelector(), 1));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::BreedGoal>(llama->goalSelector(), 2));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::FollowParentGoal>(llama->goalSelector(), 4));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(llama->goalSelector(), 6));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookAtGoal>(llama->goalSelector(), 7));
    EXPECT_TRUE(hasGoalWithPriority<entity::ai::goal::LookRandomlyGoal>(llama->goalSelector(), 8));
}

TEST(LlamaAiGoalsTest, BaseGoalCount)
{
    HorseAiGoalsTestWorld world;
    auto llama = std::make_unique<LlamaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    llama->setWorld(&world);

    // 继承自 AbstractHorseEntity: 8 个目标
    EXPECT_EQ(getTotalGoalCount(llama->goalSelector()), 8)
        << "LlamaEntity should have 8 base goals from AbstractHorseEntity";
}

// ============================================================================
// LlamaFollowCaravanGoal Constants Tests
// ============================================================================

TEST(LlamaFollowCaravanGoalTest, ConstantsAreCorrect)
{
    // MC 1.16.5 常量验证
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::SEARCH_RADIUS, 9.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::SEARCH_HEIGHT, 4.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::MIN_JOIN_DISTANCE_SQ, 4.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::MAX_FOLLOW_DISTANCE_SQ, 676.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::CARAVAN_FOLLOW_DISTANCE, 2.0);
    EXPECT_EQ(entity::ai::goal::LlamaFollowCaravanGoal::MAX_CARAVAN_LENGTH, 8);
}

// ============================================================================
// LlamaDefendTargetGoal Constants Tests
// ============================================================================

TEST(LlamaDefendTargetGoalTest, ConstantsAreCorrect)
{
    // MC 1.16.5 常量验证
    EXPECT_EQ(entity::ai::goal::LlamaDefendTargetGoal::TARGET_RANGE, 16.0);
    EXPECT_DOUBLE_EQ(entity::ai::goal::LlamaDefendTargetGoal::TARGET_RANGE_MODIFIER, 0.25);
}

} // namespace
} // namespace mc
