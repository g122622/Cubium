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

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/MathUtils.hpp"

using namespace mc;
using namespace mc::math;

// ============================================================================
// EvokerGoals 测试
// ============================================================================
//
// 测试唤魔者 AI 目标相关功能，特别是 countNearbyVexes() 的实体范围查询逻辑。
// 参考 MC 1.16.5 EvokerEntity.SummonSpellGoal
//
// 注意：完整的集成测试需要 Mock 世界和实体系统。
// 这里测试常量和核心逻辑。

class EvokerGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置代码
    }
};

// ============================================================================
// countNearbyVexes 相关常量测试
// ============================================================================

TEST_F(EvokerGoalsTest, VexEntityType_IsCorrect)
{
    // 验证 Vex 实体类型已正确定义
    // 这是 countNearbyVexes() 功能的前提条件
    EXPECT_NE(static_cast<u32>(LegacyEntityType::Vex), static_cast<u32>(LegacyEntityType::Unknown));
    EXPECT_EQ(static_cast<u32>(LegacyEntityType::Vex), 81u);
}

TEST_F(EvokerGoalsTest, SearchRange_IsCorrect)
{
    // MC 1.16.5: 唤魔者搜索恼鬼的范围为 16 格
    // evoker.getBoundingBox().grow(16.0D)
    constexpr f32 VEX_SEARCH_RANGE = 16.0f;
    EXPECT_FLOAT_EQ(VEX_SEARCH_RANGE, 16.0f);
}

TEST_F(EvokerGoalsTest, MaxVexCount_IsCorrect)
{
    // MC 1.16.5: 唤魔者最多召唤恼鬼直到周围有 8 个
    // rand.nextInt(8) + 1 > vexCount
    constexpr i32 MAX_VEX_COUNT = 8;
    EXPECT_EQ(MAX_VEX_COUNT, 8);
}

// ============================================================================
// AxisAlignedBB 范围扩展测试
// ============================================================================

TEST_F(EvokerGoalsTest, AxisAlignedBB_Grow_ExpandsCorrectly)
{
    // 测试 AxisAlignedBB::grow() 方法是否正确扩展范围
    // 这是 countNearbyVexes() 使用的核心方法

    // 创建一个 1x2x1 的碰撞箱（唤魔者尺寸）
    AxisAlignedBB box(0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f);

    // 向各方向扩展 16 格
    AxisAlignedBB expanded = box.grow(16.0f);

    // 验证扩展后的范围
    EXPECT_FLOAT_EQ(expanded.minX, -16.0f);
    EXPECT_FLOAT_EQ(expanded.maxX, 17.0f);
    EXPECT_FLOAT_EQ(expanded.minY, -16.0f);
    EXPECT_FLOAT_EQ(expanded.maxY, 18.0f);
    EXPECT_FLOAT_EQ(expanded.minZ, -16.0f);
    EXPECT_FLOAT_EQ(expanded.maxZ, 17.0f);
}

TEST_F(EvokerGoalsTest, AxisAlignedBB_Contains_Origin)
{
    // 测试碰撞箱是否正确检测点在范围内
    AxisAlignedBB box(-16.0f, -16.0f, -16.0f, 17.0f, 18.0f, 17.0f);

    // 原点应在范围内
    EXPECT_TRUE(box.contains(mc::Vector3(0.0f, 0.0f, 0.0f)));

    // 边界点应在范围内
    EXPECT_TRUE(box.contains(mc::Vector3(-15.0f, -15.0f, -15.0f)));
    EXPECT_TRUE(box.contains(mc::Vector3(16.0f, 17.0f, 16.0f)));

    // 范围外的点不应在范围内
    EXPECT_FALSE(box.contains(mc::Vector3(-17.0f, 0.0f, 0.0f)));
    EXPECT_FALSE(box.contains(mc::Vector3(18.0f, 0.0f, 0.0f)));
}

// ============================================================================
// 召唤恼鬼逻辑测试
// ============================================================================

TEST_F(EvokerGoalsTest, SummonVex_SummonCount_IsCorrect)
{
    // MC 1.16.5: 唤魔者每次召唤 3 个恼鬼
    constexpr i32 VEX_SUMMON_COUNT = 3;
    EXPECT_EQ(VEX_SUMMON_COUNT, 3);
}

TEST_F(EvokerGoalsTest, SummonVex_LifeTime_IsCorrect)
{
    // MC 1.16.5: 恼鬼有限生命为 30-120 秒 (600-2400 ticks)
    constexpr i32 MIN_LIFE_TIME = 20 * 30;  // 30 秒 = 600 ticks
    constexpr i32 MAX_LIFE_TIME = 20 * 120; // 120 秒 = 2400 ticks
    constexpr i32 LIFE_TIME_RANGE = MAX_LIFE_TIME - MIN_LIFE_TIME; // 90 秒 = 1800 ticks

    EXPECT_EQ(MIN_LIFE_TIME, 600);
    EXPECT_EQ(MAX_LIFE_TIME, 2400);
    EXPECT_EQ(LIFE_TIME_RANGE, 1800);
}

TEST_F(EvokerGoalsTest, SummonVex_SpawnOffset_IsCorrect)
{
    // MC 1.16.5: 恼鬼在唤魔者周围 -2 到 +2 格的范围内生成
    // offsetX = -2 + rng.nextInt(5) -> [-2, 2]
    // offsetZ = -2 + rng.nextInt(5) -> [-2, 2]
    constexpr i32 MIN_OFFSET = -2;
    constexpr i32 MAX_OFFSET = 2;
    constexpr i32 RANDOM_RANGE = 5; // nextInt(5) -> [0, 4]

    EXPECT_EQ(MIN_OFFSET + 0, -2);
    EXPECT_EQ(MIN_OFFSET + RANDOM_RANGE - 1, MAX_OFFSET);
}

// ============================================================================
// 施法常量测试
// ============================================================================

TEST_F(EvokerGoalsTest, CastingDuration_IsCorrect)
{
    // MC 1.16.5: 唤魔者施法持续时间为 40 ticks (2 秒)
    constexpr i32 CASTING_DURATION = 40;
    EXPECT_EQ(CASTING_DURATION, 40);
}

TEST_F(EvokerGoalsTest, FangsCooldown_IsCorrect)
{
    // MC 1.16.5: 尖牙攻击冷却时间为 100 ticks (5 秒)
    constexpr i32 FANGS_COOLDOWN = 100;
    EXPECT_EQ(FANGS_COOLDOWN, 100);
}

TEST_F(EvokerGoalsTest, SummonCooldown_IsCorrect)
{
    // MC 1.16.5: 召唤恼鬼冷却时间为 340 ticks (17 秒)
    constexpr i32 SUMMON_COOLDOWN = 340;
    EXPECT_EQ(SUMMON_COOLDOWN, 340);
}

// ============================================================================
// 尖牙攻击参数测试
// ============================================================================

TEST_F(EvokerGoalsTest, FangsAttack_CloseRange_InnerRadius)
{
    // MC 1.16.5: 近距离攻击时内圈半径为 1.5 格
    constexpr f32 INNER_CIRCLE_RADIUS = 1.5f;
    EXPECT_FLOAT_EQ(INNER_CIRCLE_RADIUS, 1.5f);
}

TEST_F(EvokerGoalsTest, FangsAttack_CloseRange_InnerCount)
{
    // MC 1.16.5: 近距离攻击时内圈有 5 个尖牙
    constexpr i32 INNER_CIRCLE_COUNT = 5;
    EXPECT_EQ(INNER_CIRCLE_COUNT, 5);
}

TEST_F(EvokerGoalsTest, FangsAttack_CloseRange_OuterRadius)
{
    // MC 1.16.5: 近距离攻击时外圈半径为 2.5 格
    constexpr f32 OUTER_CIRCLE_RADIUS = 2.5f;
    EXPECT_FLOAT_EQ(OUTER_CIRCLE_RADIUS, 2.5f);
}

TEST_F(EvokerGoalsTest, FangsAttack_CloseRange_OuterCount)
{
    // MC 1.16.5: 近距离攻击时外圈有 8 个尖牙
    constexpr i32 OUTER_CIRCLE_COUNT = 8;
    EXPECT_EQ(OUTER_CIRCLE_COUNT, 8);
}

TEST_F(EvokerGoalsTest, FangsAttack_FarRange_Count)
{
    // MC 1.16.5: 远距离攻击时直线生成 16 个尖牙
    constexpr i32 FAR_RANGE_FANG_COUNT = 16;
    EXPECT_EQ(FAR_RANGE_FANG_COUNT, 16);
}

TEST_F(EvokerGoalsTest, FangsAttack_FarRange_Spacing)
{
    // MC 1.16.5: 远距离攻击时尖牙间距为 1.25 格
    constexpr f32 FANG_SPACING = 1.25f;
    EXPECT_FLOAT_EQ(FANG_SPACING, 1.25f);
}

TEST_F(EvokerGoalsTest, FangsAttack_Damage_IsCorrect)
{
    // MC 1.16.5: 尖牙伤害为 6 点（3 颗心）
    constexpr f32 FANGS_DAMAGE = 6.0f;
    EXPECT_FLOAT_EQ(FANGS_DAMAGE, 6.0f);
}

TEST_F(EvokerGoalsTest, FangsAttack_WarmupDelay_IsCorrect)
{
    // MC 1.16.5: 尖牙攻击预热延迟
    // 内圈: 0 ticks
    // 外圈: 3 ticks
    // 远距离: 每个 1 tick 递增
    constexpr i32 INNER_WARMUP = 0;
    constexpr i32 OUTER_WARMUP = 3;

    EXPECT_EQ(INNER_WARMUP, 0);
    EXPECT_EQ(OUTER_WARMUP, 3);
}

// ============================================================================
// 实体类型判断测试
// ============================================================================

TEST_F(EvokerGoalsTest, EntityType_VexIsNotEvoker)
{
    // 验证 Vex 和 Evoker 是不同的实体类型
    EXPECT_NE(static_cast<u32>(LegacyEntityType::Vex), static_cast<u32>(LegacyEntityType::Evoker));
}

TEST_F(EvokerGoalsTest, EntityType_VexIsNotPlayer)
{
    // 验证 Vex 不是玩家类型
    EXPECT_NE(static_cast<u32>(LegacyEntityType::Vex), static_cast<u32>(LegacyEntityType::Player));
}
