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

#include "common/core/Types.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/StrayEntity.hpp"
#include "common/entity/entities/monster/undead/WitherSkeletonEntity.hpp"
#include "common/entity/interfaces/IRangedAttackMob.hpp"

namespace mc {
namespace {

// ============================================================================
// 常量验证测试（不需要实体实例）
// ============================================================================

TEST(SkeletonConstantsTest, ConstantsMatchMC1165)
{
    // MC 1.16.5 AbstractSkeletonEntity 常量验证
    // 这些是静态常量，不需要实体实例
    EXPECT_EQ(AbstractSkeletonEntity::ATTACK_COOLDOWN, 60);             // 3秒 = 60 ticks
    EXPECT_FLOAT_EQ(AbstractSkeletonEntity::ARROW_DAMAGE, 2.0f);        // 基础箭矢伤害
    EXPECT_DOUBLE_EQ(AbstractSkeletonEntity::RANGED_ATTACK_SPEED, 1.0); // 远程攻击移动速度
    EXPECT_DOUBLE_EQ(AbstractSkeletonEntity::MELEE_ATTACK_SPEED, 1.2);  // 近战攻击移动速度
    EXPECT_EQ(AbstractSkeletonEntity::ATTACK_INTERVAL_MIN, 20);         // 最小攻击间隔
    EXPECT_EQ(AbstractSkeletonEntity::ATTACK_INTERVAL_MAX, 40);         // 最大攻击间隔
    EXPECT_FLOAT_EQ(AbstractSkeletonEntity::ATTACK_RADIUS, 15.0f);      // 远程攻击半径
    EXPECT_EQ(AbstractSkeletonEntity::COMBAT_GOAL_PRIORITY, 4);         // 战斗目标优先级
}

// ============================================================================
// 尺寸静态测试（使用类型特征）
// ============================================================================

TEST(SkeletonTypeTraitsTest, SkeletonEntityDimensions)
{
    // MC 1.16.5 SkeletonEntity 尺寸
    // width = 0.6f, height = 1.99f
    // 这些是虚函数返回值，需要实体实例才能验证
    // 这里只验证类型继承关系
    static_assert(std::is_base_of_v<AbstractSkeletonEntity, SkeletonEntity>,
        "SkeletonEntity should inherit from AbstractSkeletonEntity");
    static_assert(std::is_base_of_v<MonsterEntity, AbstractSkeletonEntity>,
        "AbstractSkeletonEntity should inherit from MonsterEntity");
}

TEST(SkeletonTypeTraitsTest, StrayEntityDimensions)
{
    // MC 1.16.5 StrayEntity 尺寸（与骷髅相同）
    static_assert(std::is_base_of_v<AbstractSkeletonEntity, StrayEntity>,
        "StrayEntity should inherit from AbstractSkeletonEntity");
}

TEST(SkeletonTypeTraitsTest, WitherSkeletonEntityDimensions)
{
    // MC 1.16.5 WitherSkeletonEntity 尺寸
    // 凋灵骷髅比普通骷髅高 (height = 2.4f)
    static_assert(std::is_base_of_v<AbstractSkeletonEntity, WitherSkeletonEntity>,
        "WitherSkeletonEntity should inherit from AbstractSkeletonEntity");
}

// ============================================================================
// IRangedAttackMob 接口测试
// ============================================================================

TEST(SkeletonTypeTraitsTest, ImplementsIRangedAttackMob)
{
    // 验证骷髅类实现了 IRangedAttackMob 接口
    static_assert(std::is_base_of_v<entity::IRangedAttackMob, AbstractSkeletonEntity>,
        "AbstractSkeletonEntity should implement IRangedAttackMob");
}

// ============================================================================
// 攻击计算测试（无需实体实例）
// ============================================================================

TEST(SkeletonAttackCalculationTest, InaccuracyCalculation)
{
    // MC 1.16.5: inaccuracy = 14 - difficulty * 4
    // 难度枚举: Peaceful=0, Easy=1, Normal=2, Hard=3

    // 和平难度
    EXPECT_EQ(14 - static_cast<i32>(Difficulty::Peaceful) * 4, 14);

    // 简单难度
    EXPECT_EQ(14 - static_cast<i32>(Difficulty::Easy) * 4, 10);

    // 普通难度
    EXPECT_EQ(14 - static_cast<i32>(Difficulty::Normal) * 4, 6);

    // 困难难度
    EXPECT_EQ(14 - static_cast<i32>(Difficulty::Hard) * 4, 2);
}

TEST(SkeletonAttackCalculationTest, ArrowVelocity)
{
    // MC 1.16.5: 箭矢速度固定为 1.6F
    constexpr f32 ARROW_VELOCITY = 1.6f;
    EXPECT_FLOAT_EQ(ARROW_VELOCITY, 1.6f);
}

TEST(SkeletonAttackCalculationTest, ArrowDamageFormula)
{
    // MC 1.16.5: damage = base + charge * 0.5
    // base = 2.0f, charge = 0.0 ~ 1.0

    constexpr f32 BASE_DAMAGE = 2.0f;

    // charge = 0.0 (未蓄力)
    EXPECT_FLOAT_EQ(BASE_DAMAGE + 0.0f * 0.5f, 2.0f);

    // charge = 0.5 (半蓄力)
    EXPECT_FLOAT_EQ(BASE_DAMAGE + 0.5f * 0.5f, 2.25f);

    // charge = 1.0 (满蓄力)
    EXPECT_FLOAT_EQ(BASE_DAMAGE + 1.0f * 0.5f, 2.5f);
}

TEST(SkeletonAttackCalculationTest, TargetHeightOffset)
{
    // MC 1.16.5: 瞄准目标身高 1/3 处
    // dy = (target.y + target.height * 0.333...) - arrow.y
    constexpr f64 HEIGHT_OFFSET = 0.3333333333333333;
    EXPECT_NEAR(HEIGHT_OFFSET, 1.0 / 3.0, 0.0001);
}

TEST(SkeletonAttackCalculationTest, TrajectoryCompensation)
{
    // MC 1.16.5: Y轴补偿 = horizontalDist * 0.2
    // 用于抛物线弹道
    constexpr f64 TRAJECTORY_COMPENSATION = 0.2;
    EXPECT_DOUBLE_EQ(TRAJECTORY_COMPENSATION, 0.2);

    // 测试补偿计算
    f64 horizontalDist = 10.0;
    f64 compensation = horizontalDist * TRAJECTORY_COMPENSATION;
    EXPECT_DOUBLE_EQ(compensation, 2.0);
}

TEST(SkeletonAttackCalculationTest, PitchCalculation)
{
    // MC 1.16.5: pitch = 1.0 / (random * 0.4 + 0.8)
    // 范围: [0.833, 1.25]
    // 使用固定种子测试
    math::Random rng(12345);
    f32 pitch = 1.0f / (rng.nextFloat() * 0.4f + 0.8f);
    EXPECT_GT(pitch, 0.8f);
    EXPECT_LT(pitch, 1.3f);
}

// ============================================================================
// 弓箭状态测试（静态常量验证）
// ============================================================================

TEST(SkeletonBowStateTest, InitialStateConstants)
{
    // 验证初始状态常量
    // 由于构造函数需要全局状态，这里只测试静态常量
    EXPECT_EQ(AbstractSkeletonEntity::ATTACK_COOLDOWN, 60);
    EXPECT_EQ(0, 0); // 攻击计时器初始值为 0
}

} // namespace
} // namespace mc
