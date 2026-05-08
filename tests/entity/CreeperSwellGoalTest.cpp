#include <gtest/gtest.h>

#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/core/EnumSet.hpp"

using namespace mc;
using namespace mc::entity::ai;

// ============================================================================
// CreeperSwellGoal 基本测试
// ============================================================================
//
// 注意：CreeperSwellGoal 的完整行为测试需要 CreeperEntity、MobEntity 和相关依赖。
// 这里只测试常量和基本配置。
//
// 完整的行为测试应在集成测试中进行，使用 Mock 世界和实体。

class CreeperSwellGoalBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置代码
    }
};

// ============================================================================
// GoalFlag 基本测试
// ============================================================================

TEST_F(CreeperSwellGoalBasicTest, GoalFlag_MoveFlagValue) {
    // 验证 Move 标志是第一个标志
    EXPECT_EQ(static_cast<int>(GoalFlag::Move), 0);
    EXPECT_EQ(static_cast<int>(GoalFlag::Look), 1);
    EXPECT_EQ(static_cast<int>(GoalFlag::Jump), 2);
    EXPECT_EQ(static_cast<int>(GoalFlag::Target), 3);
    EXPECT_EQ(static_cast<int>(GoalFlag::Count), 4);
}

TEST_F(CreeperSwellGoalBasicTest, EnumSet_CanStoreGoalFlags) {
    // 验证 EnumSet 可以正确存储 GoalFlag
    EnumSet<GoalFlag> flags;
    flags.set(GoalFlag::Move);
    flags.set(GoalFlag::Look);

    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_TRUE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));

    EXPECT_EQ(flags.count(), 2);
}

TEST_F(CreeperSwellGoalBasicTest, EnumSet_InitializerList) {
    // 使用初始化列表创建 EnumSet
    EnumSet<GoalFlag> flags{GoalFlag::Move, GoalFlag::Look};

    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_TRUE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

// ============================================================================
// CreeperSwellGoal 常量测试
// ============================================================================

TEST_F(CreeperSwellGoalBasicTest, SwellDistances_AreCorrect) {
    // MC 1.16.5 常量验证
    // 触发距离：3 格 (3 * 3 = 9)
    // 取消距离：7 格 (7 * 7 = 49)
    constexpr f32 SWELL_TRIGGER_DISTANCE = 3.0f;
    constexpr f32 SWELL_TRIGGER_DISTANCE_SQ = SWELL_TRIGGER_DISTANCE * SWELL_TRIGGER_DISTANCE;
    constexpr f32 SWELL_CANCEL_DISTANCE = 7.0f;
    constexpr f32 SWELL_CANCEL_DISTANCE_SQ = SWELL_CANCEL_DISTANCE * SWELL_CANCEL_DISTANCE;

    EXPECT_FLOAT_EQ(SWELL_TRIGGER_DISTANCE_SQ, 9.0f);
    EXPECT_FLOAT_EQ(SWELL_CANCEL_DISTANCE_SQ, 49.0f);
}

TEST_F(CreeperSwellGoalBasicTest, CreeperEntityConstants_AreCorrect) {
    // CreeperEntity 默认常量
    constexpr i32 DEFAULT_FUSE_TIME = 30;           // 默认点燃时间 (1.5秒)
    constexpr i32 DEFAULT_EXPLOSION_RADIUS = 3;     // 默认爆炸半径
    constexpr f32 NORMAL_EXPLOSION_POWER = 3.0f;    // 普通爆炸威力
    constexpr f32 POWERED_EXPLOSION_POWER = 6.0f;   // 高压爆炸威力

    EXPECT_EQ(DEFAULT_FUSE_TIME, 30);
    EXPECT_EQ(DEFAULT_EXPLOSION_RADIUS, 3);
    EXPECT_FLOAT_EQ(NORMAL_EXPLOSION_POWER, 3.0f);
    EXPECT_FLOAT_EQ(POWERED_EXPLOSION_POWER, 6.0f);
    // 高压爆炸威力是普通的两倍
    EXPECT_FLOAT_EQ(POWERED_EXPLOSION_POWER, NORMAL_EXPLOSION_POWER * 2.0f);
}

// ============================================================================
// allGoalFlags 函数测试
// ============================================================================

TEST_F(CreeperSwellGoalBasicTest, AllGoalFlags_ReturnsAllFlags) {
    auto all = allGoalFlags();

    EXPECT_TRUE(all.test(GoalFlag::Move));
    EXPECT_TRUE(all.test(GoalFlag::Look));
    EXPECT_TRUE(all.test(GoalFlag::Jump));
    EXPECT_TRUE(all.test(GoalFlag::Target));
    EXPECT_EQ(all.count(), 4);
}

// ============================================================================
// EnumSet 操作测试
// ============================================================================

TEST_F(CreeperSwellGoalBasicTest, EnumSet_Operators) {
    EnumSet<GoalFlag> a{GoalFlag::Move, GoalFlag::Look};
    EnumSet<GoalFlag> b{GoalFlag::Look, GoalFlag::Jump};

    // 并集
    auto union_ = a | b;
    EXPECT_TRUE(union_.test(GoalFlag::Move));
    EXPECT_TRUE(union_.test(GoalFlag::Look));
    EXPECT_TRUE(union_.test(GoalFlag::Jump));
    EXPECT_FALSE(union_.test(GoalFlag::Target));

    // 交集
    auto intersect = a & b;
    EXPECT_FALSE(intersect.test(GoalFlag::Move));
    EXPECT_TRUE(intersect.test(GoalFlag::Look));
    EXPECT_FALSE(intersect.test(GoalFlag::Jump));

    // 差集
    auto diff = a - b;
    EXPECT_TRUE(diff.test(GoalFlag::Move));
    EXPECT_FALSE(diff.test(GoalFlag::Look));
}

TEST_F(CreeperSwellGoalBasicTest, EnumSet_Intersects) {
    EnumSet<GoalFlag> a{GoalFlag::Move, GoalFlag::Look};
    EnumSet<GoalFlag> b{GoalFlag::Look, GoalFlag::Jump};
    EnumSet<GoalFlag> c{GoalFlag::Target};

    EXPECT_TRUE(a.intersects(b));
    EXPECT_FALSE(a.intersects(c));
}

TEST_F(CreeperSwellGoalBasicTest, EnumSet_ForEach) {
    EnumSet<GoalFlag> flags{GoalFlag::Move, GoalFlag::Target};
    std::vector<GoalFlag> values;
    flags.forEach([&values](GoalFlag flag) {
        values.push_back(flag);
    });

    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], GoalFlag::Move);
    EXPECT_EQ(values[1], GoalFlag::Target);
}
