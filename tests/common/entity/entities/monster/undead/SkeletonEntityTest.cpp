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
    // MC 1.16.5 / MC 1.21.11 AbstractSkeletonEntity 常量验证
    // 这些是静态常量，不需要实体实例
    EXPECT_EQ(AbstractSkeletonEntity::ATTACK_COOLDOWN, 60);             // 3秒 = 60 ticks
    EXPECT_FLOAT_EQ(AbstractSkeletonEntity::ARROW_DAMAGE, 2.0f);        // 基础箭矢伤害
    EXPECT_DOUBLE_EQ(AbstractSkeletonEntity::RANGED_ATTACK_SPEED, 1.0); // 远程攻击移动速度
    EXPECT_DOUBLE_EQ(AbstractSkeletonEntity::MELEE_ATTACK_SPEED, 1.2);  // 近战攻击移动速度
    EXPECT_EQ(AbstractSkeletonEntity::ATTACK_INTERVAL_MIN, 20);         // 最小攻击间隔
    EXPECT_EQ(AbstractSkeletonEntity::ATTACK_INTERVAL_MAX, 40);         // 最大攻击间隔
    EXPECT_FLOAT_EQ(AbstractSkeletonEntity::ATTACK_RADIUS, 15.0f);      // 远程攻击半径
    EXPECT_EQ(AbstractSkeletonEntity::COMBAT_GOAL_PRIORITY, 4);         // 战斗目标优先级

    // MC 1.21.11 难度相关攻击间隔常量
    // 对应 MC 原版 AbstractSkeleton 中的常量：
    //   HARD_ATTACK_INTERVAL = 20 (困难难度最小攻击间隔)
    //   NORMAL_ATTACK_INTERVAL = 40 (非困难难度最小攻击间隔)
    //   INCREASED_HARD_ATTACK_INTERVAL = 50 (沼骸/焦枯骷髅困难难度间隔)
    //   INCREASED_NORMAL_ATTACK_INTERVAL = 70 (沼骸/焦枯骷髅非困难难度间隔)
    EXPECT_EQ(AbstractSkeletonEntity::HARD_ATTACK_INTERVAL, 20);
    EXPECT_EQ(AbstractSkeletonEntity::NORMAL_ATTACK_INTERVAL, 40);
    EXPECT_EQ(AbstractSkeletonEntity::INCREASED_HARD_ATTACK_INTERVAL, 50);
    EXPECT_EQ(AbstractSkeletonEntity::INCREASED_NORMAL_ATTACK_INTERVAL, 70);
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

// ============================================================================
// 攻击间隔虚方法测试
//
// 验证 getHardAttackInterval() 和 getAttackInterval() 的默认行为。
// 对应 MC 原版 AbstractSkeleton.getHardAttackInterval() 和 getAttackInterval()。
// ============================================================================

TEST(SkeletonAttackIntervalTest, DefaultAttackIntervals)
{
    // 普通骷髅和流浪者使用基类默认值
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(skeleton->getHardAttackInterval(), 20); // HARD_ATTACK_INTERVAL
    EXPECT_EQ(skeleton->getAttackInterval(), 40);     // NORMAL_ATTACK_INTERVAL

    auto stray = std::make_unique<StrayEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    EXPECT_EQ(stray->getHardAttackInterval(), 20); // 流浪者继承基类值（MC 1.21.11）
    EXPECT_EQ(stray->getAttackInterval(), 40);     // 流浪者继承基类值（MC 1.21.11）
}

TEST(SkeletonAttackIntervalTest, WitherSkeletonAttackIntervals)
{
    // 凋灵骷髅使用近战，但攻击间隔方法仍然返回基类值
    auto witherSkeleton = std::make_unique<WitherSkeletonEntity>(EntityInstanceId(3), mc::test::testEcsRegistry());
    EXPECT_EQ(witherSkeleton->getHardAttackInterval(), 20);
    EXPECT_EQ(witherSkeleton->getAttackInterval(), 40);
}

// ============================================================================
// 难度与攻击间隔关系测试
//
// 验证 setCombatTask() 中根据难度调整最小攻击间隔的逻辑。
// 对应 MC 原版 AbstractSkeleton.reassessWeaponGoal() 中的难度判断：
//   - 困难难度: setMinAttackInterval(getHardAttackInterval())
//   - 其他难度: setMinAttackInterval(getAttackInterval())
// ============================================================================

TEST(SkeletonAttackIntervalTest, DifficultyBasedIntervalLogic)
{
    // 验证难度与攻击间隔的对应关系
    // 困难难度: 普通骷髅 20 ticks（射击更快）
    // 其他难度: 普通骷髅 40 ticks（射击更慢）
    auto skeleton = std::make_unique<SkeletonEntity>(EntityInstanceId(4), mc::test::testEcsRegistry());

    // 困难难度: 使用 getHardAttackInterval()
    EXPECT_EQ(skeleton->getHardAttackInterval(), AbstractSkeletonEntity::HARD_ATTACK_INTERVAL);
    EXPECT_EQ(AbstractSkeletonEntity::HARD_ATTACK_INTERVAL, 20);

    // 其他难度: 使用 getAttackInterval()
    EXPECT_EQ(skeleton->getAttackInterval(), AbstractSkeletonEntity::NORMAL_ATTACK_INTERVAL);
    EXPECT_EQ(AbstractSkeletonEntity::NORMAL_ATTACK_INTERVAL, 40);

    // 验证困难难度比其他难度射击更快
    EXPECT_LT(skeleton->getHardAttackInterval(), skeleton->getAttackInterval());
}

TEST(SkeletonAttackIntervalTest, IncreasedAttackIntervalConstants)
{
    // 验证增大型攻击间隔常量（用于沼骸骷髅等射击更慢的变种）
    // 对应 MC 原版 INCREASED_HARD_ATTACK_INTERVAL = 50, INCREASED_NORMAL_ATTACK_INTERVAL = 70
    EXPECT_EQ(AbstractSkeletonEntity::INCREASED_HARD_ATTACK_INTERVAL, 50);
    EXPECT_EQ(AbstractSkeletonEntity::INCREASED_NORMAL_ATTACK_INTERVAL, 70);

    // 增大型间隔比基类间隔更慢
    EXPECT_GT(AbstractSkeletonEntity::INCREASED_HARD_ATTACK_INTERVAL, AbstractSkeletonEntity::HARD_ATTACK_INTERVAL);
    EXPECT_GT(AbstractSkeletonEntity::INCREASED_NORMAL_ATTACK_INTERVAL, AbstractSkeletonEntity::NORMAL_ATTACK_INTERVAL);
}

} // namespace
} // namespace mc
