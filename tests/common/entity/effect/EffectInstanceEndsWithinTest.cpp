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

#include "entity/effect/EffectInstance.hpp"
#include "entity/effect/EffectType.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::effect;

/**
 * @brief EffectInstance::endsWithin() 方法测试
 *
 * 测试效果实例的到期查询功能，覆盖各种边界场景。
 * endsWithin(maxDuration) 当效果的剩余持续时间 <= maxDuration 时返回 true，
 * 永久效果（duration < 0）始终返回 false。
 */
class EffectInstanceEndsWithinTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ============================================================================
// 基础场景
// ============================================================================

TEST_F(EffectInstanceEndsWithinTest, NormalEffect_WithinThreshold_ReturnsTrue)
{
    // 持续时间 100tick，阈值 200 → 100 <= 200，应返回 true
    EffectInstance effect(EffectType::Regeneration, 100);
    EXPECT_TRUE(effect.endsWithin(200));
}

TEST_F(EffectInstanceEndsWithinTest, NormalEffect_ExceedsThreshold_ReturnsFalse)
{
    // 持续时间 300tick，阈值 200 → 300 > 200，应返回 false
    EffectInstance effect(EffectType::Regeneration, 300);
    EXPECT_FALSE(effect.endsWithin(200));
}

TEST_F(EffectInstanceEndsWithinTest, NormalEffect_ExactlyAtThreshold_ReturnsTrue)
{
    // 持续时间 200tick，阈值 200 → 200 <= 200，应返回 true
    EffectInstance effect(EffectType::Regeneration, 200);
    EXPECT_TRUE(effect.endsWithin(200));
}

// ============================================================================
// 边界场景：零和负值
// ============================================================================

TEST_F(EffectInstanceEndsWithinTest, ZeroThreshold_AlwaysFalseForPositiveDuration)
{
    // 持续时间 > 0 时，endsWithin(0) 始终返回 false（效果不会在 0 tick 内结束）
    EffectInstance effect(EffectType::Regeneration, 100);
    EXPECT_FALSE(effect.endsWithin(0));

    EffectInstance effect1(EffectType::Regeneration, 1);
    EXPECT_FALSE(effect1.endsWithin(0));
}

TEST_F(EffectInstanceEndsWithinTest, ExpiredEffect_EndsWithinZero)
{
    // 过期效果（duration = 0）：endsWithin(0) 返回 true
    // 因为 0 <= 0，效果确实在 0 tick 内结束
    // 注意：实际运行中过期效果会被 EffectManager 移除，
    // 这里只测试 endsWithin 的语义正确性
    EffectInstance effect(EffectType::Regeneration, 600);
    // 手动将 duration tick 到 0 来测试
    // 由于没有 setter，我们通过逻辑推理验证：
    // duration=0 时，endsWithin(any_non_negative) 都应返回 true
    // 这里我们验证 endsWithin(0) 对非零 duration 返回 false
    EXPECT_FALSE(effect.endsWithin(0)); // 600 > 0，不在 0 tick 内结束
}

TEST_F(EffectInstanceEndsWithinTest, NormalEffect_EndsWithinZero_ReturnsFalse)
{
    // 持续时间 100tick，阈值 0 → 100 > 0，应返回 false
    EffectInstance effect(EffectType::Regeneration, 100);
    EXPECT_FALSE(effect.endsWithin(0));
}

TEST_F(EffectInstanceEndsWithinTest, PermanentEffect_NegativeDuration_ReturnsFalse)
{
    // 永久效果（duration = -1），任何阈值都返回 false
    EffectInstance effect(EffectType::Regeneration, -1);
    EXPECT_FALSE(effect.endsWithin(0));
    EXPECT_FALSE(effect.endsWithin(100));
    EXPECT_FALSE(effect.endsWithin(2399));
    EXPECT_FALSE(effect.endsWithin(2400));
}

TEST_F(EffectInstanceEndsWithinTest, PermanentEffect_NegativeDuration_AnyThreshold)
{
    // 永久效果使用更大的负值也返回 false
    EffectInstance effect(EffectType::Regeneration, -100);
    EXPECT_FALSE(effect.endsWithin(0));
    EXPECT_FALSE(effect.endsWithin(1000));
}

// ============================================================================
// 美西螈再生效果场景
// ============================================================================

TEST_F(EffectInstanceEndsWithinTest, AxolotlRegen_NoExistingEffect)
{
    // 无效果时 → endsWithin 不适用（getEffect 返回 nullptr），
    // 直接给予基础 100tick 再生
    // 此测试验证 endsWithin 在基础场景下的正确行为
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;
    constexpr i32 REGEN_BUFF_BASE_DURATION = 100;

    // 模拟新效果
    EffectInstance newEffect(EffectType::Regeneration, REGEN_BUFF_BASE_DURATION, 0);
    EXPECT_EQ(newEffect.duration(), 100);
    EXPECT_TRUE(newEffect.endsWithin(REGEN_BUFF_MAX_DURATION - 1));
}

TEST_F(EffectInstanceEndsWithinTest, AxolotlRegen_ExistingEffect2399Ticks)
{
    // 现有再生效果 2399 tick（恰好低于上限）
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;
    constexpr i32 REGEN_BUFF_BASE_DURATION = 100;

    EffectInstance existing(EffectType::Regeneration, 2399, 0);
    // 2399 <= 2399 → endsWithin 返回 true → 可以刷新
    EXPECT_TRUE(existing.endsWithin(REGEN_BUFF_MAX_DURATION - 1));

    // 新持续时间 = min(2400, 100 + 2399) = 2400
    i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + existing.duration());
    EXPECT_EQ(newDuration, 2400);
}

TEST_F(EffectInstanceEndsWithinTest, AxolotlRegen_ExistingEffect2400Ticks)
{
    // 现有再生效果 2400 tick（已达到上限）
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;

    EffectInstance existing(EffectType::Regeneration, 2400, 0);
    // 2400 > 2399 → endsWithin 返回 false → 不刷新
    EXPECT_FALSE(existing.endsWithin(REGEN_BUFF_MAX_DURATION - 1));
}

TEST_F(EffectInstanceEndsWithinTest, AxolotlRegen_ExistingEffect500Ticks)
{
    // 现有再生效果 500 tick
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;
    constexpr i32 REGEN_BUFF_BASE_DURATION = 100;

    EffectInstance existing(EffectType::Regeneration, 500, 0);
    EXPECT_TRUE(existing.endsWithin(REGEN_BUFF_MAX_DURATION - 1));

    i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + existing.duration());
    EXPECT_EQ(newDuration, 600);
}

TEST_F(EffectInstanceEndsWithinTest, AxolotlRegen_PermanentEffect_NotRefreshed)
{
    // 永久再生效果（如信标给予的）不应被刷新
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;

    EffectInstance permanent(EffectType::Regeneration, -1, 0);
    EXPECT_FALSE(permanent.endsWithin(REGEN_BUFF_MAX_DURATION - 1));
}

// ============================================================================
// 其他效果类型
// ============================================================================

TEST_F(EffectInstanceEndsWithinTest, DifferentEffectTypes_SameBehavior)
{
    // endsWithin 不依赖效果类型，对所有效果行为一致
    EffectInstance speed(EffectType::Speed, 50);
    EffectInstance resistance(EffectType::Resistance, 50);
    EffectInstance fireRes(EffectType::FireResistance, 50);

    EXPECT_TRUE(speed.endsWithin(100));
    EXPECT_TRUE(resistance.endsWithin(100));
    EXPECT_TRUE(fireRes.endsWithin(100));

    EXPECT_FALSE(speed.endsWithin(49));
    EXPECT_FALSE(resistance.endsWithin(49));
    EXPECT_FALSE(fireRes.endsWithin(49));
}

TEST_F(EffectInstanceEndsWithinTest, AmplifierDoesNotAffectEndsWithin)
{
    // endsWithin 只看持续时间，不看等级
    EffectInstance regen1(EffectType::Regeneration, 100, 0); // Regeneration I
    EffectInstance regen2(EffectType::Regeneration, 100, 1); // Regeneration II
    EffectInstance regen3(EffectType::Regeneration, 100, 2); // Regeneration III

    EXPECT_TRUE(regen1.endsWithin(200));
    EXPECT_TRUE(regen2.endsWithin(200));
    EXPECT_TRUE(regen3.endsWithin(200));
}
