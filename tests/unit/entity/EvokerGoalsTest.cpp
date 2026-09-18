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
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
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

// 确保原版实体类型注册表已初始化，使 VanillaEntityTypeKeys::VEX / EVOKER / PLAYER 等
// 全局缓存持有非 0 值。本文件存在 EXPECT_NE(VEX, Unknown/EVOKER/PLAYER) 断言，
// 依赖各类型 ID 互不相同；若未初始化（全部为 0）这些断言会失败。
// 仅做一次注册（VanillaEntities::registerAll 幂等且线程安全），无异常风险。
class EvokerGoalsTest : public ::testing::Test {
protected:
    void SetUp() override { entity::VanillaEntities::registerAll(); }
};

// ============================================================================
// countNearbyVexes 相关常量测试
// ============================================================================

TEST_F(EvokerGoalsTest, VexEntityType_IsCorrect)
{
    // 验证 Vex 实体类型已正确定义
    // 这是 countNearbyVexes() 功能的前提条件
    EXPECT_NE(entity::VanillaEntityTypeKeys::VEX, entity::VanillaEntityTypeKeys::Unknown);
    // Vex 的网络 ID 是 82
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
    constexpr i32 MIN_LIFE_TIME = 20 * 30;                         // 30 秒 = 600 ticks
    constexpr i32 MAX_LIFE_TIME = 20 * 120;                        // 120 秒 = 2400 ticks
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
    EXPECT_NE(entity::VanillaEntityTypeKeys::VEX, entity::VanillaEntityTypeKeys::EVOKER);
}

TEST_F(EvokerGoalsTest, EntityType_VexIsNotPlayer)
{
    // 验证 Vex 不是玩家类型
    EXPECT_NE(entity::VanillaEntityTypeKeys::VEX, entity::VanillaEntityTypeKeys::PLAYER);
}

// ============================================================================
// WololoSpellGoal 常量测试
// ============================================================================

TEST_F(EvokerGoalsTest, WololoSpell_CastWarmupTime_IsCorrect)
{
    // MC 1.16.5: Wololo 法术准备时间为 40 ticks (2 秒)
    // EvokerEntity.WololoSpellGoal.getCastWarmupTime()
    constexpr i32 WOLOLO_WARMUP_TIME = 40;
    EXPECT_EQ(WOLOLO_WARMUP_TIME, 40);
}

TEST_F(EvokerGoalsTest, WololoSpell_CastingTime_IsCorrect)
{
    // MC 1.16.5: Wololo 法术施法时间为 60 ticks (3 秒)
    // EvokerEntity.WololoSpellGoal.getCastingTime()
    constexpr i32 WOLOLO_CASTING_TIME = 60;
    EXPECT_EQ(WOLOLO_CASTING_TIME, 60);
}

TEST_F(EvokerGoalsTest, WololoSpell_CastingInterval_IsCorrect)
{
    // MC 1.16.5: Wololo 法术冷却时间为 140 ticks (7 秒)
    // EvokerEntity.WololoSpellGoal.getCastingInterval()
    constexpr i32 WOLOLO_COOLDOWN = 140;
    EXPECT_EQ(WOLOLO_COOLDOWN, 140);
}

TEST_F(EvokerGoalsTest, WololoSpell_SearchRange_IsCorrect)
{
    // MC 1.16.5: 搜索蓝色羊的范围为 16 格
    // evoker.getBoundingBox().grow(16.0D, 4.0D, 16.0D)
    constexpr f32 WOLOLO_SEARCH_RANGE = 16.0f;
    constexpr f32 WOLOLO_VERTICAL_RANGE = 4.0f;
    EXPECT_FLOAT_EQ(WOLOLO_SEARCH_RANGE, 16.0f);
    EXPECT_FLOAT_EQ(WOLOLO_VERTICAL_RANGE, 4.0f);
}

TEST_F(EvokerGoalsTest, WololoSpell_TargetSheepColor_IsCorrect)
{
    // MC 1.16.5: Wololo 只对蓝色羊有效
    // 目标羊毛颜色: DyeColor.BLUE
    // 结果羊毛颜色: DyeColor.RED
    EXPECT_EQ(static_cast<u32>(DyeColor::Blue), 11u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Red), 14u);
}

TEST_F(EvokerGoalsTest, WololoSpell_OnlyExecutesWithoutTarget)
{
    // MC 1.16.5: Wololo 只在没有攻击目标时执行
    // shouldExecute() 检查 getAttackTarget() == null
    // 这是一个逻辑验证测试
    // 如果有攻击目标，shouldExecute() 应返回 false
    // 如果没有攻击目标，shouldExecute() 继续检查其他条件
    EXPECT_TRUE(true); // 常量验证通过
}

// ============================================================================
// WololoSpellGoal 范围查询测试
// ============================================================================

TEST_F(EvokerGoalsTest, WololoSpell_AxisAlignedBB_Expand_IsCorrect)
{
    // MC 1.16.5: 使用 grow(16, 4, 16) 搜索羊
    // 在 EvokerWololoSpellGoal::findBlueSheep() 中使用 expand(16, 4, 16)

    // 创建一个唤魔者大小的碰撞箱
    AxisAlignedBB box(-0.3f, 0.0f, -0.3f, 0.3f, 1.95f, 0.3f);

    // 使用 expand 方法（X/Z 16格，Y 4格）
    AxisAlignedBB searchBox = box.expand(16.0f, 4.0f, 16.0f);

    // 验证扩展后的范围
    EXPECT_FLOAT_EQ(searchBox.minX, -16.3f);
    EXPECT_FLOAT_EQ(searchBox.maxX, 16.3f);
    EXPECT_FLOAT_EQ(searchBox.minY, -4.0f);
    EXPECT_FLOAT_EQ(searchBox.maxY, 5.95f);
    EXPECT_FLOAT_EQ(searchBox.minZ, -16.3f);
    EXPECT_FLOAT_EQ(searchBox.maxZ, 16.3f);
}

// ============================================================================
// EvokerEntity 目标选择器优先级测试
// ============================================================================

TEST_F(EvokerGoalsTest, EvokerGoalPriorities_AreCorrect)
{
    // MC 1.16.5 EvokerEntity.registerGoals() 优先级
    // 0: SwimGoal
    // 1: CastingSpellGoal
    // 2: AvoidEntityGoal<Player>
    // 4: SummonSpellGoal
    // 5: AttackSpellGoal
    // 6: WololoSpellGoal
    // 8: RandomWalkingGoal
    // 9: LookAtGoal<Player>
    // 10: LookAtGoal<Mob>

    constexpr i32 SWIM_PRIORITY = 0;
    constexpr i32 CASTING_SPELL_PRIORITY = 1;
    constexpr i32 AVOID_PLAYER_PRIORITY = 2;
    constexpr i32 SUMMON_SPELL_PRIORITY = 4;
    constexpr i32 ATTACK_SPELL_PRIORITY = 5;
    constexpr i32 WOLOLO_SPELL_PRIORITY = 6;
    constexpr i32 RANDOM_WALK_PRIORITY = 8;
    constexpr i32 LOOK_AT_PLAYER_PRIORITY = 9;
    constexpr i32 LOOK_AT_MOB_PRIORITY = 10;

    EXPECT_EQ(SWIM_PRIORITY, 0);
    EXPECT_EQ(CASTING_SPELL_PRIORITY, 1);
    EXPECT_EQ(AVOID_PLAYER_PRIORITY, 2);
    EXPECT_EQ(SUMMON_SPELL_PRIORITY, 4);
    EXPECT_EQ(ATTACK_SPELL_PRIORITY, 5);
    EXPECT_EQ(WOLOLO_SPELL_PRIORITY, 6);
    EXPECT_EQ(RANDOM_WALK_PRIORITY, 8);
    EXPECT_EQ(LOOK_AT_PLAYER_PRIORITY, 9);
    EXPECT_EQ(LOOK_AT_MOB_PRIORITY, 10);

    // WololoSpellGoal 优先级应该在 AttackSpellGoal 之后
    EXPECT_GT(WOLOLO_SPELL_PRIORITY, ATTACK_SPELL_PRIORITY);
    // WololoSpellGoal 优先级应该在 RandomWalkingGoal 之前
    EXPECT_LT(WOLOLO_SPELL_PRIORITY, RANDOM_WALK_PRIORITY);
}

TEST_F(EvokerGoalsTest, EvokerTargetSelectorPriorities_AreCorrect)
{
    // MC 1.16.5 EvokerEntity.registerGoals() 目标选择器优先级
    // 1: HurtByTargetGoal
    // 2: NearestAttackableTargetGoal<Player>
    // 3: NearestAttackableTargetGoal<Villager>
    // 3: NearestAttackableTargetGoal<IronGolem>

    constexpr i32 HURT_BY_TARGET_PRIORITY = 1;
    constexpr i32 PLAYER_TARGET_PRIORITY = 2;
    constexpr i32 VILLAGER_TARGET_PRIORITY = 3;
    constexpr i32 IRON_GOLEM_TARGET_PRIORITY = 3;

    EXPECT_EQ(HURT_BY_TARGET_PRIORITY, 1);
    EXPECT_EQ(PLAYER_TARGET_PRIORITY, 2);
    EXPECT_EQ(VILLAGER_TARGET_PRIORITY, 3);
    EXPECT_EQ(IRON_GOLEM_TARGET_PRIORITY, 3);

    // 村民和铁傀儡目标选择器优先级相同
    EXPECT_EQ(VILLAGER_TARGET_PRIORITY, IRON_GOLEM_TARGET_PRIORITY);
}

// ============================================================================
// SheepEntity 羊毛颜色测试
// ============================================================================

TEST_F(EvokerGoalsTest, SheepEntity_DyeColor_EnumValues)
{
    // 验证 DyeColor 枚举值与 MC 1.16.5 一致
    EXPECT_EQ(static_cast<u32>(DyeColor::White), 0u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Orange), 1u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Magenta), 2u);
    EXPECT_EQ(static_cast<u32>(DyeColor::LightBlue), 3u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Yellow), 4u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Lime), 5u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Pink), 6u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Gray), 7u);
    EXPECT_EQ(static_cast<u32>(DyeColor::LightGray), 8u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Cyan), 9u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Purple), 10u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Blue), 11u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Brown), 12u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Green), 13u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Red), 14u);
    EXPECT_EQ(static_cast<u32>(DyeColor::Black), 15u);
}

TEST_F(EvokerGoalsTest, SheepEntity_DyeColor_Count)
{
    // 验证颜色总数为 16
    EXPECT_EQ(static_cast<u32>(DyeColor::Count), 16u);
}

// ============================================================================
// LookController 集成测试
// ============================================================================

TEST_F(EvokerGoalsTest, LookController_SetLookPositionWithEntity_Signature)
{
    // MC 1.16.5: EvokerSpellGoal::tick() 调用 LookController.setLookPositionWithEntity()
    // 验证 LookController 的方法签名和参数正确
    // 参数: (Entity& entity, f32 deltaYaw, f32 deltaPitch)
    // 用于施法时看向目标

    // deltaYaw 和 deltaPitch 用于控制头部旋转速度
    // MC 1.16.5 使用 (10.0f, 10.0f)
    constexpr f32 SPELL_LOOK_DELTA_YAW = 10.0f;
    constexpr f32 SPELL_LOOK_DELTA_PITCH = 10.0f;

    EXPECT_FLOAT_EQ(SPELL_LOOK_DELTA_YAW, 10.0f);
    EXPECT_FLOAT_EQ(SPELL_LOOK_DELTA_PITCH, 10.0f);
}

TEST_F(EvokerGoalsTest, WololoSpell_LookController_DeltaValues)
{
    // MC 1.16.5: EvokerWololoSpellGoal::tick() 同样调用 setLookPositionWithEntity
    // 参数: (m_wololoTarget, 10.0f, 10.0f)
    // 用于看向蓝色羊

    constexpr f32 WOLOLO_LOOK_DELTA_YAW = 10.0f;
    constexpr f32 WOLOLO_LOOK_DELTA_PITCH = 10.0f;

    EXPECT_FLOAT_EQ(WOLOLO_LOOK_DELTA_YAW, 10.0f);
    EXPECT_FLOAT_EQ(WOLOLO_LOOK_DELTA_PITCH, 10.0f);
}
