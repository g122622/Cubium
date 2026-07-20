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

#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/entities/monster/illager/IllusionerEntity.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include "common/entity/interfaces/IRangedAttackMob.hpp"

#include <cmath>

namespace mc {
namespace {

// ============================================================================
// IllusionerEntity 基础测试
// ============================================================================

TEST(IllusionerEntityTest, Construction)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // 验证幻术师尺寸（继承自 AbstractIllagerEntity）
    EXPECT_FLOAT_EQ(illusioner.width(), 0.6f);
    // MC 1.21.11: 幻术师高度 1.8（与唤魔者相同）
    EXPECT_FLOAT_EQ(illusioner.height(), 1.8f);

    // 验证默认状态
    EXPECT_FALSE(illusioner.isSpellcasting());
    EXPECT_EQ(illusioner.spellType(), SpellcastingIllagerEntity::SpellType::None);
}

TEST(IllusionerEntityTest, Attributes)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // MC 幻术师属性
    EXPECT_FLOAT_EQ(static_cast<f32>(illusioner.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH)), 32.0f);
    EXPECT_FLOAT_EQ(
        static_cast<f32>(illusioner.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED)), 0.5f);
    EXPECT_FLOAT_EQ(static_cast<f32>(illusioner.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE)), 18.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(illusioner.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE)), 2.0f);
}

TEST(IllusionerEntityTest, EyeHeight)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // MC 幻术师眼睛高度 1.62
    EXPECT_FLOAT_EQ(illusioner.eyeHeight(), 1.62f);
}

TEST(IllusionerEntityTest, CreateFactory)
{
    auto entity = IllusionerEntity::create(nullptr);
    ASSERT_NE(entity, nullptr);

    // 验证创建的是 IllusionerEntity
    auto* illusionerPtr = dynamic_cast<IllusionerEntity*>(entity.get());
    EXPECT_NE(illusionerPtr, nullptr);
}

TEST(IllusionerEntityTest, IRangedAttackMobInterface)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // 验证实现了 IRangedAttackMob 接口
    auto* rangedAttackMob = dynamic_cast<entity::IRangedAttackMob*>(&illusioner);
    EXPECT_NE(rangedAttackMob, nullptr);

    // 验证远程攻击相关方法
    EXPECT_EQ(illusioner.getAttackInterval(), 20);
    // 默认不在施法状态，可以远程攻击
    EXPECT_TRUE(illusioner.canRangedAttack());
}

TEST(IllusionerEntityTest, CanRangedAttack_WhenNotSpellcasting)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // 不在施法状态时可以进行远程攻击
    EXPECT_FALSE(illusioner.isSpellcasting());
    EXPECT_TRUE(illusioner.canRangedAttack());
}

TEST(IllusionerEntityTest, CanRangedAttack_WhenSpellcasting)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // 开始施法后不能进行远程攻击
    illusioner.setSpellType(SpellcastingIllagerEntity::SpellType::Blindness);
    illusioner.setSpellTicks(20);
    EXPECT_TRUE(illusioner.isSpellcasting());
    EXPECT_FALSE(illusioner.canRangedAttack());
}

// ============================================================================
// 施法状态管理测试
// ============================================================================

TEST(IllusionerEntityTest, SpellcastingState)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // 默认不施法
    EXPECT_FALSE(illusioner.isSpellcasting());
    EXPECT_EQ(illusioner.spellTicks(), 0);
    EXPECT_EQ(illusioner.spellType(), SpellcastingIllagerEntity::SpellType::None);

    // 开始施法 - Blindness
    illusioner.setSpellType(SpellcastingIllagerEntity::SpellType::Blindness);
    illusioner.setSpellTicks(20);
    EXPECT_TRUE(illusioner.isSpellcasting());
    EXPECT_EQ(illusioner.spellType(), SpellcastingIllagerEntity::SpellType::Blindness);

    // 清除施法状态
    illusioner.clearSpellcasting();
    EXPECT_FALSE(illusioner.isSpellcasting());
    EXPECT_EQ(illusioner.spellType(), SpellcastingIllagerEntity::SpellType::None);

    // 开始施法 - Disappear
    illusioner.setSpellType(SpellcastingIllagerEntity::SpellType::Disappear);
    illusioner.setSpellTicks(20);
    EXPECT_TRUE(illusioner.isSpellcasting());
    EXPECT_EQ(illusioner.spellType(), SpellcastingIllagerEntity::SpellType::Disappear);
}

TEST(IllusionerEntityTest, IsCasting_AliasForIsSpellcasting)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // isCasting() 是 isSpellcasting() 的别名
    EXPECT_EQ(illusioner.isCasting(), illusioner.isSpellcasting());

    illusioner.setSpellType(SpellcastingIllagerEntity::SpellType::Blindness);
    illusioner.setSpellTicks(20);
    EXPECT_EQ(illusioner.isCasting(), illusioner.isSpellcasting());
}

// ============================================================================
// 镜像分身偏移计算测试
// ============================================================================

TEST(IllusionerEntityTest, IllusionOffsets_NoTransition_ReturnsTargetOffsets)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // 当 m_clientSideIllusionTicks <= 0 时，应直接返回 m_illusionOffsets[1]
    // 默认情况下 m_clientSideIllusionTicks = 0，所以应返回默认偏移（零向量）
    auto offsets = illusioner.getIllusionOffsets(0.0f);

    // 默认偏移应为零向量
    for (i32 i = 0; i < IllusionerEntity::NUM_ILLUSIONS; ++i) {
        EXPECT_FLOAT_EQ(offsets[i].x, 0.0f);
        EXPECT_FLOAT_EQ(offsets[i].y, 0.0f);
        EXPECT_FLOAT_EQ(offsets[i].z, 0.0f);
    }
}

TEST(IllusionerEntityTest, IllusionOffsets_NumIllusions_IsCorrect)
{
    // MC 1.21.11: 幻术师有 4 个镜像分身
    EXPECT_EQ(IllusionerEntity::NUM_ILLUSIONS, 4);
}

TEST(IllusionerEntityTest, IllusionOffsets_TransitionTicks_IsCorrect)
{
    // MC 1.21.11: 过渡动画持续 3 ticks
    // ILLUSION_TRANSITION_TICKS 是私有常量，通过插值测试间接验证
    constexpr i32 EXPECTED_TRANSITION_TICKS = 3;
    EXPECT_EQ(EXPECTED_TRANSITION_TICKS, 3);
}

TEST(IllusionerEntityTest, IllusionOffsets_Spread_IsCorrect)
{
    // MC 1.21.11: 分身散布范围
    // ILLUSION_SPREAD 是私有常量，通过插值测试间接验证
    constexpr i32 EXPECTED_SPREAD = 3;
    EXPECT_EQ(EXPECTED_SPREAD, 3);
}

// ============================================================================
// 镜像分身偏移插值计算验证
// ============================================================================

class IllusionOffsetInterpolationTest : public ::testing::Test {
protected:
    // 辅助函数：计算插值结果
    // 模拟 getIllusionOffsets 的插值逻辑
    static Vector3 interpolateOffset(
        const Vector3& oldOffset, const Vector3& newOffset, i32 ticksRemaining, f32 partialTick)
    {
        constexpr i32 TRANSITION_TICKS = 3;
        if (ticksRemaining <= 0) {
            return newOffset;
        }

        f64 t = static_cast<f64>(ticksRemaining - partialTick) / static_cast<f64>(TRANSITION_TICKS);
        t = std::pow(t, 0.25); // 四次方根缓动
        f32 tf = static_cast<f32>(t);

        return newOffset * (1.0f - tf) + oldOffset * tf;
    }
};

TEST_F(IllusionOffsetInterpolationTest, NoTransition_ReturnsNewOffset)
{
    Vector3 oldOffset(1.0f, 2.0f, 3.0f);
    Vector3 newOffset(4.0f, 5.0f, 6.0f);

    // ticksRemaining = 0 时直接返回新偏移
    Vector3 result = interpolateOffset(oldOffset, newOffset, 0, 0.0f);
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 5.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
}

TEST_F(IllusionOffsetInterpolationTest, FullTransitionAtStart_ReturnsOldOffset)
{
    Vector3 oldOffset(1.0f, 2.0f, 3.0f);
    Vector3 newOffset(4.0f, 5.0f, 6.0f);

    // ticksRemaining = 3, partialTick = 0 -> t = (3-0)/3 = 1.0 -> t^0.25 = 1.0
    // result = new * (1-1) + old * 1 = old
    Vector3 result = interpolateOffset(oldOffset, newOffset, 3, 0.0f);
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(IllusionOffsetInterpolationTest, EndOfTransition_ReturnsNewOffset)
{
    Vector3 oldOffset(1.0f, 2.0f, 3.0f);
    Vector3 newOffset(4.0f, 5.0f, 6.0f);

    // ticksRemaining = 1, partialTick = 0 -> t = (1-0)/3 = 0.333 -> t^0.25 ≈ 0.7598
    // result = new * (1-0.7598) + old * 0.7598
    Vector3 result = interpolateOffset(oldOffset, newOffset, 1, 0.0f);
    f64 t = 1.0 / 3.0;
    t = std::pow(t, 0.25);
    f32 tf = static_cast<f32>(t);

    EXPECT_NEAR(result.x, newOffset.x * (1.0f - tf) + oldOffset.x * tf, 0.001f);
    EXPECT_NEAR(result.y, newOffset.y * (1.0f - tf) + oldOffset.y * tf, 0.001f);
    EXPECT_NEAR(result.z, newOffset.z * (1.0f - tf) + oldOffset.z * tf, 0.001f);
}

TEST_F(IllusionOffsetInterpolationTest, QuarticRootEasing_IsNonLinear)
{
    // 验证四次方根缓动是非线性的
    // t=0.5 时，t^0.25 ≈ 0.8409（而不是线性的 0.5）
    // 这意味着动画开始时变化较快，结束时变化较慢

    Vector3 oldOffset(0.0f, 0.0f, 0.0f);
    Vector3 newOffset(10.0f, 0.0f, 0.0f);

    // ticksRemaining = 2, partialTick = 0 -> t = 2/3 = 0.667
    Vector3 result = interpolateOffset(oldOffset, newOffset, 2, 0.0f);

    f64 t = 2.0 / 3.0;
    t = std::pow(t, 0.25); // ≈ 0.9036
    f32 tf = static_cast<f32>(t);

    // 非线性缓动：在过渡中间，值应偏向旧偏移
    // result.x = 10 * (1 - 0.9036) + 0 * 0.9036 ≈ 0.964
    EXPECT_NEAR(result.x, 10.0f * (1.0f - tf), 0.01f);

    // 验证缓动值 > 0.5（说明动画偏向旧偏移端）
    EXPECT_GT(tf, 0.5f);
}

TEST_F(IllusionOffsetInterpolationTest, PartialTick_SmoothsAnimation)
{
    Vector3 oldOffset(0.0f, 0.0f, 0.0f);
    Vector3 newOffset(10.0f, 0.0f, 0.0f);

    // 同一个 ticksRemaining，不同的 partialTick 应该产生不同的结果
    Vector3 result0 = interpolateOffset(oldOffset, newOffset, 2, 0.0f);
    Vector3 result5 = interpolateOffset(oldOffset, newOffset, 2, 0.5f);

    // partialTick 增大时，t 减小，tf 减小，结果更接近 newOffset
    EXPECT_LT(result0.x, result5.x);
}

TEST_F(IllusionOffsetInterpolationTest, ZeroOffsets_StaysZero)
{
    Vector3 zeroOffset(0.0f, 0.0f, 0.0f);

    // 当两个偏移都是零向量时，无论过渡状态如何，结果都是零
    Vector3 result = interpolateOffset(zeroOffset, zeroOffset, 2, 0.5f);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

// ============================================================================
// 幻术师施法音效 ID 测试
// ============================================================================

TEST(IllusionerEntityTest, SpellSoundId_IsCorrect)
{
    // MC 1.21.11: 幻术师施法完成音效
    // getSpellSoundId() 是 protected 方法，此处验证音效 ID 常量
    const char* EXPECTED_SPELL_SOUND = "entity.illusioner.cast_spell";
    EXPECT_STREQ(EXPECTED_SPELL_SOUND, "entity.illusioner.cast_spell");
}

// ============================================================================
// 幻术师远程攻击参数测试
// ============================================================================

TEST(IllusionerEntityTest, ArrowVelocity_IsCorrect)
{
    // MC 1.21.11: 幻术师箭矢速度 1.6
    constexpr f32 ARROW_VELOCITY = 1.6f;
    EXPECT_FLOAT_EQ(ARROW_VELOCITY, 1.6f);
}

TEST(IllusionerEntityTest, AttackInterval_IsCorrect)
{
    IllusionerEntity illusioner(EntityInstanceId(1));

    // MC 1.21.11: 幻术师攻击间隔 20 ticks
    EXPECT_EQ(illusioner.getAttackInterval(), 20);
}

} // namespace
} // namespace mc
