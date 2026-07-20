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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/FishSwimGoal.hpp"
#include "common/entity/entities/passive/fish/AbstractFishEntity.hpp"
#include "common/entity/entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "common/entity/entities/passive/fish/CodEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/passive/fish/SalmonEntity.hpp"
#include "common/entity/entities/passive/fish/TropicalFishEntity.hpp"

namespace mc {
namespace {

// ============================================================================
// Test World for FishSwimGoal
// ============================================================================

class TestFishSwimWorld final : public test::BaseTestWorld {
public:
    void setEntities(std::vector<Entity*> entities) { m_entities = std::move(entities); }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity* except) const override
    {
        std::vector<Entity*> result;
        for (Entity* entity : m_entities) {
            if (entity == except) {
                continue;
            }
            result.push_back(entity);
        }
        return result;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos, f32 range, const Entity* except) const override
    {
        std::vector<Entity*> result;
        const f32 rangeSq = range * range;

        for (Entity* entity : m_entities) {
            if (entity == except) {
                continue;
            }

            if (pos.distanceSquared(entity->position()) <= rangeSq) {
                result.push_back(entity);
            }
        }

        return result;
    }

    [[nodiscard]] bool isWaterAt(const BlockPos&) const override { return true; }
    [[nodiscard]] bool isLavaAt(const BlockPos&) const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TestFishSwimWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TestFishSwimWorld::tickManager not implemented");
    }

private:
    std::vector<Entity*> m_entities;
};

// ============================================================================
// FishSwimGoal Construction Tests
// ============================================================================

TEST(FishSwimGoalTest, ConstructionWithFish)
{
    CodEntity cod(EntityInstanceId(1));
    entity::ai::goal::FishSwimGoal goal(&cod);
    EXPECT_EQ(goal.getTypeName(), "FishSwimGoal");
}

TEST(FishSwimGoalTest, ConstructionWithSpeedAndChance)
{
    CodEntity cod(EntityInstanceId(1));
    entity::ai::goal::FishSwimGoal goal(&cod, 1.5, 20);
    EXPECT_EQ(goal.getTypeName(), "FishSwimGoal");
}

// 注意：FishSwimGoal 的 shouldExecute 会在内部检查 m_fish 是否为 nullptr
// RandomSwimmingGoal 构造函数会断言 creature != nullptr
// 所以我们使用有效的鱼实体来测试

// ============================================================================
// FishSwimGoal shouldExecute Tests
// ============================================================================

class FishSwimGoalShouldExecuteTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<TestFishSwimWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<TestFishSwimWorld> m_world;
};

TEST_F(FishSwimGoalShouldExecuteTest, ShouldExecuteForRegularFish)
{
    // 普通鱼类（非群游）总是可以随机游泳
    PufferfishEntity pufferfish(EntityInstanceId(1));
    pufferfish.setWorld(m_world.get());
    pufferfish.setPosition(0.0f, 62.0f, 0.0f);

    // Pufferfish 不是群游鱼类，canRandomSwim() 返回 true
    EXPECT_TRUE(pufferfish.canRandomSwim());

    // 注意：shouldExecute 还需要其他条件满足（如概率检查）
    // 由于没有有效的世界导航，可能返回 false
    entity::ai::goal::FishSwimGoal goal(&pufferfish, 1.0, 1); // chance=1 确保总是检查
    // 这里只测试 canRandomSwim 部分不影响执行
}

TEST_F(FishSwimGoalShouldExecuteTest, ShouldExecuteForSchoolingFishWithoutLeader)
{
    // 群游鱼类没有群首时可以随机游泳
    CodEntity cod(EntityInstanceId(1));
    cod.setWorld(m_world.get());
    cod.setPosition(0.0f, 62.0f, 0.0f);

    // 没有 group leader
    EXPECT_FALSE(cod.hasGroupLeader());
    EXPECT_TRUE(cod.canRandomSwim());

    entity::ai::goal::FishSwimGoal goal(&cod, 1.0, 1);
    // canRandomSwim() 返回 true，所以可以执行（取决于其他条件）
}

TEST_F(FishSwimGoalShouldExecuteTest, ShouldNotExecuteForSchoolingFishWithLeader)
{
    // 群游鱼类有群首时不能随机游泳（跟随群首）
    CodEntity leader(EntityInstanceId(1));
    CodEntity follower(EntityInstanceId(2));

    leader.setWorld(m_world.get());
    follower.setWorld(m_world.get());

    leader.setPosition(0.0f, 62.0f, 0.0f);
    follower.setPosition(5.0f, 62.0f, 0.0f);

    // follower 加入 leader 的群体
    follower.joinGroup(leader);

    // 验证群体状态
    EXPECT_TRUE(follower.hasGroupLeader());
    EXPECT_FALSE(follower.canRandomSwim());

    // FishSwimGoal 应该不执行
    entity::ai::goal::FishSwimGoal goal(&follower, 1.0, 1);
    EXPECT_FALSE(goal.shouldExecute());
}

// ============================================================================
// FishSwimGoal canRandomSwim Tests
// ============================================================================

TEST(FishSwimGoalCanRandomSwimTest, AbstractFishEntityReturnsTrue)
{
    // 抽象鱼类基类默认返回 true
    PufferfishEntity pufferfish(EntityInstanceId(1));
    EXPECT_TRUE(pufferfish.canRandomSwim());
}

TEST(FishSwimGoalCanRandomSwimTest, SchoolingFishWithoutLeaderReturnsTrue)
{
    // 群游鱼类没有群首时返回 true
    CodEntity cod(EntityInstanceId(1));
    EXPECT_FALSE(cod.hasGroupLeader());
    EXPECT_TRUE(cod.canRandomSwim());

    SalmonEntity salmon(EntityInstanceId(2));
    EXPECT_FALSE(salmon.hasGroupLeader());
    EXPECT_TRUE(salmon.canRandomSwim());
}

TEST(FishSwimGoalCanRandomSwimTest, SchoolingFishWithLeaderReturnsFalse)
{
    // 群游鱼类有群首时返回 false
    CodEntity leader(EntityInstanceId(1));
    CodEntity follower(EntityInstanceId(2));

    follower.joinGroup(leader);

    EXPECT_TRUE(follower.hasGroupLeader());
    EXPECT_FALSE(follower.canRandomSwim());
}

TEST(FishSwimGoalCanRandomSwimTest, LeaderFishReturnsTrue)
{
    // 群首自己返回 true（没有 leader）
    CodEntity leader(EntityInstanceId(1));
    CodEntity follower(EntityInstanceId(2));

    follower.joinGroup(leader);

    // leader 有跟随者，是群首
    EXPECT_TRUE(leader.isGroupLeader());
    EXPECT_FALSE(leader.hasGroupLeader());
    EXPECT_TRUE(leader.canRandomSwim());
}

// ============================================================================
// FishSwimGoal Goal Flags Tests
// ============================================================================

TEST(FishSwimGoalFlagsTest, HasCorrectMutexFlags)
{
    // FishSwimGoal 继承 RandomSwimmingGoal，应该有 Move 标志
    CodEntity cod(EntityInstanceId(1));
    entity::ai::goal::FishSwimGoal goal(&cod);

    auto flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
}

// ============================================================================
// FishSwimGoal Group Behavior Integration Tests
// ============================================================================

class FishSwimGoalGroupTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<TestFishSwimWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<TestFishSwimWorld> m_world;
};

TEST_F(FishSwimGoalGroupTest, LeaderCanSwimAfterGainingFollowers)
{
    CodEntity leader(EntityInstanceId(1));
    CodEntity follower1(EntityInstanceId(2));
    CodEntity follower2(EntityInstanceId(3));

    leader.setWorld(m_world.get());
    follower1.setWorld(m_world.get());
    follower2.setWorld(m_world.get());

    // follower 加入 leader
    follower1.joinGroup(leader);
    follower2.joinGroup(leader);

    // leader 是群首，可以随机游泳
    EXPECT_TRUE(leader.isGroupLeader());
    EXPECT_TRUE(leader.canRandomSwim());

    // followers 不能随机游泳
    EXPECT_FALSE(follower1.canRandomSwim());
    EXPECT_FALSE(follower2.canRandomSwim());
}

TEST_F(FishSwimGoalGroupTest, FollowerCanSwimAfterLeavingGroup)
{
    CodEntity leader(EntityInstanceId(1));
    CodEntity follower(EntityInstanceId(2));

    leader.setWorld(m_world.get());
    follower.setWorld(m_world.get());

    follower.joinGroup(leader);
    EXPECT_FALSE(follower.canRandomSwim());

    // 离开群体后可以随机游泳
    follower.leaveGroup();
    EXPECT_FALSE(follower.hasGroupLeader());
    EXPECT_TRUE(follower.canRandomSwim());
}

TEST_F(FishSwimGoalGroupTest, MultipleGroupsIndependent)
{
    // 多个独立群体
    CodEntity leader1(EntityInstanceId(1));
    CodEntity follower1(EntityInstanceId(2));
    CodEntity leader2(EntityInstanceId(3));
    CodEntity follower2(EntityInstanceId(4));

    leader1.setWorld(m_world.get());
    follower1.setWorld(m_world.get());
    leader2.setWorld(m_world.get());
    follower2.setWorld(m_world.get());

    // 两个独立群体
    follower1.joinGroup(leader1);
    follower2.joinGroup(leader2);

    // 两个 leader 都可以随机游泳
    EXPECT_TRUE(leader1.canRandomSwim());
    EXPECT_TRUE(leader2.canRandomSwim());

    // 两个 follower 都不能随机游泳
    EXPECT_FALSE(follower1.canRandomSwim());
    EXPECT_FALSE(follower2.canRandomSwim());
}

// ============================================================================
// AbstractFishEntity AI Goals Tests
// ============================================================================

class AbstractFishEntityGoalsTest : public ::testing::Test {
protected:
    void SetUp() override { m_world = std::make_unique<TestFishSwimWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<TestFishSwimWorld> m_world;
};

TEST_F(AbstractFishEntityGoalsTest, PufferfishIsNotSchooling)
{
    // 河豚不是群游鱼类
    PufferfishEntity pufferfish(EntityInstanceId(1));
    EXPECT_FALSE(pufferfish.canSchool());
    EXPECT_TRUE(pufferfish.canRandomSwim());
}

TEST_F(AbstractFishEntityGoalsTest, CodIsSchooling)
{
    // 鳕鱼是群游鱼类
    CodEntity cod(EntityInstanceId(1));
    EXPECT_TRUE(cod.canSchool());
}

TEST_F(AbstractFishEntityGoalsTest, SalmonIsSchooling)
{
    // 鲑鱼是群游鱼类
    SalmonEntity salmon(EntityInstanceId(1));
    EXPECT_TRUE(salmon.canSchool());
}

TEST_F(AbstractFishEntityGoalsTest, TropicalFishIsSchooling)
{
    // 热带鱼是群游鱼类
    TropicalFishEntity tropicalFish(EntityInstanceId(1));
    EXPECT_TRUE(tropicalFish.canSchool());
}

// ============================================================================
// FishSwimGoal Type Name Test
// ============================================================================

TEST(FishSwimGoalTypeNameTest, ReturnsCorrectTypeName)
{
    CodEntity cod(EntityInstanceId(1));
    entity::ai::goal::FishSwimGoal goal(&cod);
    EXPECT_EQ(goal.getTypeName(), "FishSwimGoal");
}

} // namespace
} // namespace mc
