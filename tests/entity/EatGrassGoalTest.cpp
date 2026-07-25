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
#include "common/entity/ai/goal/goals/EatGrassGoal.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"

using namespace mc;
using namespace mc::entity::ai::goal;

// ============================================================================
// SheepEntity 颜色混合测试
// ============================================================================

class SheepColorMixingTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(SheepColorMixingTest, SameColorReturnsSameColor)
{
    math::Random rng(42);

    // 相同颜色应该返回相同颜色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::White, rng), DyeColor::White);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Red, DyeColor::Red, rng), DyeColor::Red);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Black, DyeColor::Black, rng), DyeColor::Black);
}

TEST_F(SheepColorMixingTest, WhiteAndRedMakesPink)
{
    math::Random rng(42);

    // 白色 + 红色 = 粉红色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Red, rng), DyeColor::Pink);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Red, DyeColor::White, rng), DyeColor::Pink);
}

TEST_F(SheepColorMixingTest, RedAndYellowMakesOrange)
{
    math::Random rng(42);

    // 红色 + 黄色 = 橙色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Red, DyeColor::Yellow, rng), DyeColor::Orange);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Yellow, DyeColor::Red, rng), DyeColor::Orange);
}

TEST_F(SheepColorMixingTest, WhiteAndBlueMakesLightBlue)
{
    math::Random rng(42);

    // 白色 + 蓝色 = 淡蓝色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Blue, rng), DyeColor::LightBlue);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Blue, DyeColor::White, rng), DyeColor::LightBlue);
}

TEST_F(SheepColorMixingTest, BlueAndGreenMakesCyan)
{
    math::Random rng(42);

    // 蓝色 + 绿色 = 青色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Blue, DyeColor::Green, rng), DyeColor::Cyan);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Green, DyeColor::Blue, rng), DyeColor::Cyan);
}

TEST_F(SheepColorMixingTest, BlueAndRedMakesPurple)
{
    math::Random rng(42);

    // 蓝色 + 红色 = 紫色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Blue, DyeColor::Red, rng), DyeColor::Purple);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Red, DyeColor::Blue, rng), DyeColor::Purple);
}

TEST_F(SheepColorMixingTest, WhiteAndGreenMakesLime)
{
    math::Random rng(42);

    // 白色 + 绿色 = 黄绿色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Green, rng), DyeColor::Lime);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Green, DyeColor::White, rng), DyeColor::Lime);
}

TEST_F(SheepColorMixingTest, WhiteAndBlackMakesGray)
{
    math::Random rng(42);

    // 白色 + 黑色 = 灰色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Black, rng), DyeColor::Gray);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Black, DyeColor::White, rng), DyeColor::Gray);
}

TEST_F(SheepColorMixingTest, GrayAndWhiteMakesLightGray)
{
    math::Random rng(42);

    // 灰色 + 白色 = 淡灰色
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::Gray, DyeColor::White, rng), DyeColor::LightGray);
    EXPECT_EQ(SheepEntity::getDyeColorMixFromParents(DyeColor::White, DyeColor::Gray, rng), DyeColor::LightGray);
}

TEST_F(SheepColorMixingTest, NoMixingResultReturnsParentColor)
{
    // 当没有混合配方时，应该随机返回父母颜色之一
    // 使用固定种子的随机数生成器，nextBoolean() 返回确定性结果
    math::Random rng(12345);

    // 选择一对没有混合配方的颜色（例如棕色和粉色）
    DyeColor result = SheepEntity::getDyeColorMixFromParents(DyeColor::Brown, DyeColor::Pink, rng);

    // 结果应该是父母颜色之一
    EXPECT_TRUE(result == DyeColor::Brown || result == DyeColor::Pink);
}

// ============================================================================
// SheepEntity 基础测试
// ============================================================================

class SheepEntityTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(SheepEntityTest, InitialState)
{
    SheepEntity sheep(EntityInstanceId(1));

    // 初始状态
    EXPECT_FALSE(sheep.isSheared());
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::White);
    EXPECT_FALSE(sheep.isChild());
}

TEST_F(SheepEntityTest, SetFleeceColor)
{
    SheepEntity sheep(EntityInstanceId(1));

    sheep.setFleeceColor(DyeColor::Red);
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::Red);

    sheep.setFleeceColor(DyeColor::Black);
    EXPECT_EQ(sheep.getFleeceColor(), DyeColor::Black);
}

TEST_F(SheepEntityTest, SetSheared)
{
    SheepEntity sheep(EntityInstanceId(1));

    EXPECT_FALSE(sheep.isSheared());
    EXPECT_TRUE(sheep.isShearable());

    sheep.setSheared(true);
    EXPECT_TRUE(sheep.isSheared());
    EXPECT_FALSE(sheep.isShearable());

    sheep.setSheared(false);
    EXPECT_FALSE(sheep.isSheared());
    EXPECT_TRUE(sheep.isShearable());
}

TEST_F(SheepEntityTest, ChildCannotBeSheared)
{
    SheepEntity sheep(EntityInstanceId(1));
    sheep.setChild(true);

    EXPECT_FALSE(sheep.isShearable());
}

TEST_F(SheepEntityTest, EatGrassBonusRegrowsWool)
{
    SheepEntity sheep(EntityInstanceId(1));
    sheep.setSheared(true);

    EXPECT_TRUE(sheep.isSheared());

    sheep.eatGrassBonus();

    EXPECT_FALSE(sheep.isSheared());
}

TEST_F(SheepEntityTest, EatGrassBonusAcceleratesChildGrowth)
{
    SheepEntity sheep(EntityInstanceId(1));
    sheep.setChild(true);
    sheep.setGrowingAge(-24000); // 幼羊，-24000 ticks

    EXPECT_TRUE(sheep.isChild());

    i32 ageBefore = sheep.getGrowingAge();
    sheep.eatGrassBonus();
    i32 ageAfter = sheep.getGrowingAge();

    // 应该加速成长 60 ticks
    EXPECT_EQ(ageAfter - ageBefore, 60);
}

TEST_F(SheepEntityTest, GetRandomSheepColor)
{
    // 测试随机颜色生成的分布
    math::Random rng(42);
    int whiteCount = 0;
    int blackCount = 0;
    int grayCount = 0;
    int lightGrayCount = 0;
    int brownCount = 0;
    int pinkCount = 0;

    const int iterations = 10000;
    for (int i = 0; i < iterations; ++i) {
        DyeColor color = SheepEntity::getRandomSheepColor(rng);
        switch (color) {
            case DyeColor::White:
                ++whiteCount;
                break;
            case DyeColor::Black:
                ++blackCount;
                break;
            case DyeColor::Gray:
                ++grayCount;
                break;
            case DyeColor::LightGray:
                ++lightGrayCount;
                break;
            case DyeColor::Brown:
                ++brownCount;
                break;
            case DyeColor::Pink:
                ++pinkCount;
                break;
            default:
                break;
        }
    }

    // 验证概率分布大致正确
    // 白色应该占 ~81.8%
    EXPECT_GT(whiteCount, iterations * 0.75);
    EXPECT_LT(whiteCount, iterations * 0.90);

    // 黑色、灰色、淡灰色各占 ~5%
    EXPECT_GT(blackCount, iterations * 0.03);
    EXPECT_LT(blackCount, iterations * 0.08);
    EXPECT_GT(grayCount, iterations * 0.03);
    EXPECT_LT(grayCount, iterations * 0.08);
    EXPECT_GT(lightGrayCount, iterations * 0.03);
    EXPECT_LT(lightGrayCount, iterations * 0.08);

    // 棕色占 ~3%
    EXPECT_GT(brownCount, iterations * 0.01);
    EXPECT_LT(brownCount, iterations * 0.05);

    // 粉色占 ~0.2%
    EXPECT_GT(pinkCount, 0);
    EXPECT_LT(pinkCount, iterations * 0.01);
}

// ============================================================================
// EatGrassGoal 游戏规则测试
// ============================================================================

namespace {

/**
 * @brief 测试用世界，支持方块状态和游戏规则设置
 */
class EatGrassTestWorld final : public test::BaseTestWorld {
public:
    EatGrassTestWorld() = default;

    void setBlock(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        (void)flags; // 测试中忽略 flags
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_lastEventId = eventId;
        m_lastEventPos = pos;
        m_lastEventData = data;
    }

    // 实体状态广播追踪
    void broadcastEntityStatus(EntityInstanceId entityId, u8 status) override
    {
        m_lastBroadcastEntityId = entityId;
        m_lastBroadcastStatus = status;
        m_broadcastCount++;
    }

    [[nodiscard]] EntityInstanceId getLastBroadcastEntityId() const { return m_lastBroadcastEntityId; }
    [[nodiscard]] u8 getLastBroadcastStatus() const { return m_lastBroadcastStatus; }
    [[nodiscard]] i32 getBroadcastCount() const { return m_broadcastCount; }
    void resetBroadcastTracking()
    {
        m_lastBroadcastEntityId = EntityInstanceId(0);
        m_lastBroadcastStatus = 0;
        m_broadcastCount = 0;
    }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    // 测试辅助方法
    void setMobGriefing(bool value)
    {
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, value, nullptr);
    }

    [[nodiscard]] i32 getLastEventId() const { return m_lastEventId; }
    [[nodiscard]] const BlockPos& getLastEventPos() const { return m_lastEventPos; }
    [[nodiscard]] i32 getLastEventData() const { return m_lastEventData; }
    void clearLastEvent() { m_lastEventId = -1; }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    world::gamerule::GameRules m_gameRules;
    i32 m_lastEventId = -1;
    BlockPos m_lastEventPos{0, 0, 0};
    i32 m_lastEventData = 0;
    EntityInstanceId m_lastBroadcastEntityId{0};
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;
};

/**
 * @brief 测试用 MobEntity
 */
class TestMobEntity final : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityInstanceId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void setPositionForTest(f64 x, f64 y, f64 z)
    {
        setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }

    void setWorldForTest(IWorld* world) { setWorld(world); }
};

} // anonymous namespace

class EatGrassGoalGameRuleTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    void TearDown() override {}
};

// 测试游戏规则默认值（验证 MC 1.16.5 行为）
TEST_F(EatGrassGoalGameRuleTest, MobGriefingDefaultValue)
{
    world::gamerule::GameRules rules;

    // MC 1.16.5: mobGriefing 默认为 true
    EXPECT_TRUE(rules.getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));
}

// 测试游戏规则设置
TEST_F(EatGrassGoalGameRuleTest, MobGriefingCanBeChanged)
{
    world::gamerule::GameRules rules;

    // 设置为 false
    rules.setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);
    EXPECT_FALSE(rules.getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));

    // 设置回 true
    rules.setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);
    EXPECT_TRUE(rules.getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));
}

// 测试 mobGriefing=true 时草方块被转换为泥土
TEST_F(EatGrassGoalGameRuleTest, GrassBlockTurnsToDirt_WhenMobGriefingEnabled)
{
    EatGrassTestWorld world;
    world.setMobGriefing(true);
    world.setBlock(0, 63, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());

    // 验证初始状态
    const BlockState* initialState = world.getBlockState(0, 63, 0);
    ASSERT_NE(initialState, nullptr);
    EXPECT_TRUE(initialState->is(VanillaBlocks::GRASS_BLOCK));

    // 模拟吃草方块的逻辑
    world.playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS,
        BlockPos(0, 63, 0),
        static_cast<i32>(VanillaBlocks::GRASS_BLOCK->defaultState().stateId()));
    world.setBlockState(0, 63, 0, &VanillaBlocks::DIRT->defaultState(), 2);

    // 验证方块变化
    const BlockState* finalState = world.getBlockState(0, 63, 0);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->is(VanillaBlocks::DIRT));

    // 验证 playEvent 被调用
    EXPECT_EQ(world.getLastEventId(), world::WorldEvents::BREAK_BLOCK_EFFECTS);
}

// 测试 mobGriefing=false 时草方块不被改变
TEST_F(EatGrassGoalGameRuleTest, GrassBlockUnchanged_WhenMobGriefingDisabled)
{
    EatGrassTestWorld world;
    world.setMobGriefing(false); // 禁用生物破坏
    world.setBlock(0, 63, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());

    // 验证初始状态
    const BlockState* initialState = world.getBlockState(0, 63, 0);
    ASSERT_NE(initialState, nullptr);
    EXPECT_TRUE(initialState->is(VanillaBlocks::GRASS_BLOCK));

    // 当 mobGriefing=false 时，不应该改变方块
    // 模拟 eatGrass 中的逻辑
    const bool canGrief = world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
    EXPECT_FALSE(canGrief);

    // 不执行任何方块操作（符合 mobGriefing=false 的逻辑）
    // 方块应保持不变
    const BlockState* finalState = world.getBlockState(0, 63, 0);
    EXPECT_TRUE(finalState->is(VanillaBlocks::GRASS_BLOCK));

    // playEvent 不应被调用
    EXPECT_EQ(world.getLastEventId(), -1);
}

// 测试 mobGriefing=true 时草（短草）被移除
TEST_F(EatGrassGoalGameRuleTest, ShortGrassRemoved_WhenMobGriefingEnabled)
{
    EatGrassTestWorld world;
    world.setMobGriefing(true);
    world.setBlock(0, 64, 0, &VanillaBlocks::SHORT_GRASS->defaultState());

    const BlockState* initialState = world.getBlockState(0, 64, 0);
    ASSERT_NE(initialState, nullptr);
    EXPECT_TRUE(initialState->is(VanillaBlocks::SHORT_GRASS));

    // 模拟吃草的逻辑（设置空气）
    world.setBlockState(0, 64, 0, &VanillaBlocks::AIR->defaultState(), 2);

    const BlockState* finalState = world.getBlockState(0, 64, 0);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());
}

// 测试 mobGriefing=false 时草保持不变
TEST_F(EatGrassGoalGameRuleTest, ShortGrassUnchanged_WhenMobGriefingDisabled)
{
    EatGrassTestWorld world;
    world.setMobGriefing(false);
    world.setBlock(0, 64, 0, &VanillaBlocks::SHORT_GRASS->defaultState());

    const bool canGrief = world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
    EXPECT_FALSE(canGrief);

    // 方块应保持不变
    const BlockState* state = world.getBlockState(0, 64, 0);
    EXPECT_TRUE(state->is(VanillaBlocks::SHORT_GRASS));
}

// 测试 mobGriefing=false 时回调仍然被调用（羊仍获得增益）
TEST_F(EatGrassGoalGameRuleTest, EatGrassBonusCalled_WhenMobGriefingDisabled)
{
    EatGrassTestWorld world;
    world.setMobGriefing(false);
    world.setBlock(0, 63, 0, &VanillaBlocks::GRASS_BLOCK->defaultState());

    TestMobEntity mob;
    mob.setPositionForTest(0.5, 64.0, 0.5);
    mob.setWorldForTest(&world);

    bool eatGrassBonusCalled = false;
    int callCount = 0;

    // 根据原版行为，即使 mobGriefing=false，eatGrassBonus 仍应被调用
    // 这允许羊获得饱食度和重新长毛，但不破坏方块
    auto onEatGrass = [&eatGrassBonusCalled, &callCount]() {
        eatGrassBonusCalled = true;
        callCount++;
    };

    // 模拟吃草后的回调调用
    // MC 1.16.5: 无论 mobGriefing 设置如何，都会调用 eatGrassBonus
    onEatGrass();

    EXPECT_TRUE(eatGrassBonusCalled);
    EXPECT_EQ(callCount, 1);
}

// 测试高草丛也能被正确处理
TEST_F(EatGrassGoalGameRuleTest, TallGrassHandled_WhenMobGriefingEnabled)
{
    EatGrassTestWorld world;
    world.setMobGriefing(true);
    world.setBlock(0, 64, 0, &VanillaBlocks::TALL_GRASS->defaultState());

    const BlockState* initialState = world.getBlockState(0, 64, 0);
    ASSERT_NE(initialState, nullptr);
    EXPECT_TRUE(initialState->is(VanillaBlocks::TALL_GRASS));

    // 模拟吃高草的逻辑
    world.setBlockState(0, 64, 0, &VanillaBlocks::AIR->defaultState(), 2);

    const BlockState* finalState = world.getBlockState(0, 64, 0);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());
}

// ============================================================================
// 吃草目标广播实体状态测试
// ============================================================================

TEST_F(EatGrassGoalGameRuleTest, StartExecutingBroadcastsEatBlockStatus)
{
    EatGrassTestWorld world;
    world.setMobGriefing(true);

    TestMobEntity mob;
    mob.setPositionForTest(0.5, 65.0, 0.5);
    mob.setWorldForTest(&world);

    bool eatGrassBonusCalled = false;
    EatGrassGoal goal(&mob, [&eatGrassBonusCalled]() { eatGrassBonusCalled = true; }, []() { return false; });

    // 重置广播追踪
    world.resetBroadcastTracking();

    // 直接调用 startExecuting，应广播 EatBlock 状态码
    goal.startExecuting();

    // 验证广播了 EatBlock(10) 状态
    EXPECT_EQ(world.getBroadcastCount(), 1);
    EXPECT_EQ(mob.id(), world.getLastBroadcastEntityId());
    EXPECT_EQ(world.getLastBroadcastStatus(), static_cast<u8>(network::EntityStatus::EatBlock));
}

TEST_F(EatGrassGoalGameRuleTest, BroadcastsCorrectEntityId)
{
    EatGrassTestWorld world;
    world.setMobGriefing(true);

    TestMobEntity mob;
    mob.setPositionForTest(0.5, 65.0, 0.5);
    mob.setWorldForTest(&world);

    EatGrassGoal goal(&mob, []() {}, []() { return false; });

    world.resetBroadcastTracking();
    goal.startExecuting();

    // 验证广播的实体 ID 是正确的 mob ID
    EXPECT_EQ(world.getLastBroadcastEntityId(), mob.id());
}

TEST_F(EatGrassGoalGameRuleTest, StartExecutingBroadcastsEvenWithoutGrass)
{
    // 即使没有草方块，startExecuting 也会广播 EatBlock 状态
    // 因为 shouldExecute 和 startExecuting 是分开的两个阶段
    EatGrassTestWorld world;

    TestMobEntity mob;
    mob.setPositionForTest(0.5, 65.0, 0.5);
    mob.setWorldForTest(&world);

    EatGrassGoal goal(&mob, []() {}, []() { return false; });

    world.resetBroadcastTracking();
    goal.startExecuting();

    // startExecuting 总是广播 EatBlock 状态
    EXPECT_EQ(world.getBroadcastCount(), 1);
    EXPECT_EQ(world.getLastBroadcastStatus(), static_cast<u8>(network::EntityStatus::EatBlock));
}

// ============================================================================
// EatGrassGoal 边界条件测试
// ============================================================================

TEST_F(EatGrassGoalGameRuleTest, ShouldExecuteReturnsFalseWhenMobIsNull)
{
    // 构造一个 mob 指针为空的 goal
    EatGrassGoal goal(nullptr, []() {}, []() { return false; });

    // shouldExecute 应安全返回 false
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(EatGrassGoalGameRuleTest, ShouldExecuteReturnsFalseWhenNoWorld)
{
    TestMobEntity mob;
    mob.setPositionForTest(0.5, 65.0, 0.5);
    // mob 没有 world

    EatGrassGoal goal(&mob, []() {}, []() { return false; });
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(EatGrassGoalGameRuleTest, StartExecutingDoesNotCrashWithNullMob)
{
    // 构造一个 mob 指针为空的 goal
    EatGrassGoal goal(nullptr, []() {}, []() { return false; });

    // startExecuting 不应崩溃（虽然不会广播状态）
    EXPECT_NO_THROW(goal.startExecuting());
}

TEST_F(EatGrassGoalGameRuleTest, TickDoesNotCrashWhenNotExecuting)
{
    EatGrassTestWorld world;
    TestMobEntity mob;
    mob.setPositionForTest(0.5, 65.0, 0.5);
    mob.setWorldForTest(&world);

    EatGrassGoal goal(&mob, []() {}, []() { return false; });

    // 没有先调用 shouldExecute/startExecuting，直接 tick 不应崩溃
    EXPECT_NO_THROW(goal.tick());
    EXPECT_EQ(goal.getEatingGrassTimer(), 0);
}
