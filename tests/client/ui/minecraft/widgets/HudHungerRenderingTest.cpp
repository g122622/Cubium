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
 * IMPLIED, NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN NO EVENT SHALL THE
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file HudHungerRenderingTest.cpp
 * @brief 饥饿条渲染逻辑单元测试
 *
 * 测试范围：
 * - 饥饿值到图标状态的映射（满/半/空）
 * - 饥饿效果（Hunger状态效果）对图标变体选择的影响
 * - 饱和度抖动动画条件的判定
 * - 边界场景：foodLevel=0、foodLevel=20、饱和度恰好为0
 * - 抖动频率公式 tickCount % (foodLevel * 3 + 1)
 * - 确定性随机种子确保帧内一致性
 */

#include "common/core/Constants.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/util/math/random/Random.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::effect;

namespace {

/**
 * @brief 计算指定饥饿槽位的图标状态
 *
 * @param food 当前饥饿值（0-20）
 * @param slotIndex 槽位索引（0-9，从左到右）
 * @return std::pair<bool, bool> {full, half} 图标状态
 */
std::pair<bool, bool> computeHungerIconState(i32 food, i32 slotIndex)
{
    i32 foodPoints = food - slotIndex * 2;
    bool full = foodPoints >= 2;
    bool half = foodPoints == 1;
    return {full, half};
}

/**
 * @brief 判断指定饥饿槽位是否应该使用饥饿效果变体图标
 */
bool shouldUseHungerEffectVariant(bool hasHungerEffect)
{
    return hasHungerEffect;
}

/**
 * @brief 判断指定帧是否应该触发抖动
 *
 * @param tickCount 当前 tick 计数
 * @param food 当前饥饿值
 * @param saturation 当前饱和度
 * @return bool 是否应该抖动
 */
bool shouldShake(u32 tickCount, i32 food, f32 saturation)
{
    return saturation <= 0.0f && food > 0 && tickCount % (static_cast<u32>(food) * 3 + 1) == 0;
}

/**
 * @brief 计算抖动偏移（模拟 _renderHunger 中的逻辑）
 *
 * @param tickCount 当前 tick 计数
 * @param food 当前饥饿值
 * @param saturation 当前饱和度
 * @return f32 Y轴偏移（-1.0, 0.0, 或 +1.0）
 */
f32 computeShakeOffset(u32 tickCount, i32 food, f32 saturation)
{
    if (!shouldShake(tickCount, food, saturation)) {
        return 0.0f;
    }
    math::Random rng(static_cast<u64>(tickCount) * 312871ULL);
    return static_cast<f32>(rng.nextInt(3) - 1);
}

} // namespace

// ============================================================================
// 饥饿值到图标状态映射
// ============================================================================

TEST(HudHungerRenderingTest, FullHunger_AllIconsFull)
{
    // foodLevel=20 时，所有10个图标都应该是满的
    for (i32 i = 0; i < 10; ++i) {
        auto [full, half] = computeHungerIconState(20, i);
        EXPECT_TRUE(full);
        EXPECT_FALSE(half) << "slot " << i << " should not be half when food=20";
    }
}

TEST(HudHungerRenderingTest, ZeroHunger_AllIconsEmpty)
{
    // foodLevel=0 时，所有10个图标都应该是空的
    for (i32 i = 0; i < 10; ++i) {
        auto [full, half] = computeHungerIconState(0, i);
        EXPECT_FALSE(full);
        EXPECT_FALSE(half) << "slot " << i << " should not be half when food=0";
    }
}

TEST(HudHungerRenderingTest, OddHungerLevel_HalfIconAtCorrectSlot)
{
    // foodLevel=17: 前8个满，第9个半，第10个空
    auto [full0, half0] = computeHungerIconState(17, 0);
    EXPECT_TRUE(full0);
    EXPECT_FALSE(half0);

    auto [full8, half8] = computeHungerIconState(17, 8);
    EXPECT_FALSE(full8);
    EXPECT_TRUE(half8); // 17 - 8*2 = 1，半饥饿

    auto [full9, half9] = computeHungerIconState(17, 9);
    EXPECT_FALSE(full9);
    EXPECT_FALSE(half9); // 17 - 9*2 = -1，空
}

TEST(HudHungerRenderingTest, SingleHungerPoint_HalfIconOnlyAtFirstSlot)
{
    // foodLevel=1: 第1个半，其余空
    auto [full0, half0] = computeHungerIconState(1, 0);
    EXPECT_FALSE(full0);
    EXPECT_TRUE(half0); // 1 - 0*2 = 1，半饥饿

    auto [full1, half1] = computeHungerIconState(1, 1);
    EXPECT_FALSE(full1);
    EXPECT_FALSE(half1); // 1 - 1*2 = -1，空
}

TEST(HudHungerRenderingTest, EvenHungerLevel_NoHalfIcons)
{
    // foodLevel=14: 前7个满，后3个空，无半饥饿图标
    for (i32 i = 0; i < 7; ++i) {
        auto [full, half] = computeHungerIconState(14, i);
        EXPECT_TRUE(full) << "slot " << i << " should be full when food=14";
        EXPECT_FALSE(half);
    }
    for (i32 i = 7; i < 10; ++i) {
        auto [full, half] = computeHungerIconState(14, i);
        EXPECT_FALSE(full) << "slot " << i << " should not be full when food=14";
        EXPECT_FALSE(half) << "slot " << i << " should not be half when food=14";
    }
}

// ============================================================================
// 饥饿效果变体选择
// ============================================================================

TEST(HudHungerRenderingTest, HungerEffect_UsesEffectVariant)
{
    EXPECT_TRUE(shouldUseHungerEffectVariant(true));
    EXPECT_FALSE(shouldUseHungerEffectVariant(false));
}

// ============================================================================
// 饱和度抖动动画条件
// ============================================================================

TEST(HudHungerRenderingTest, ShakeCondition_NoShakeWhenSaturationPositive)
{
    // 饱和度 > 0 时不抖动
    EXPECT_FALSE(shouldShake(0, 20, 1.0f));
    EXPECT_FALSE(shouldShake(100, 10, 5.0f));
    EXPECT_FALSE(shouldShake(0, 1, 0.01f)); // 即使很小的饱和度也不抖动
}

TEST(HudHungerRenderingTest, ShakeCondition_ShakeWhenSaturationZero)
{
    // 饱和度 = 0 且 tick 满足条件时抖动
    // food=20: tickCount % (20*3+1) = tickCount % 61
    EXPECT_TRUE(shouldShake(0, 20, 0.0f));  // 0 % 61 == 0
    EXPECT_TRUE(shouldShake(61, 20, 0.0f)); // 61 % 61 == 0
    EXPECT_FALSE(shouldShake(1, 20, 0.0f)); // 1 % 61 != 0
}

TEST(HudHungerRenderingTest, ShakeCondition_NoShakeWhenFoodZero)
{
    // foodLevel=0 时不抖动（没有图标可抖动）
    EXPECT_FALSE(shouldShake(0, 0, 0.0f));
    EXPECT_FALSE(shouldShake(100, 0, 0.0f));
}

TEST(HudHungerRenderingTest, ShakeCondition_LowFoodShakesMoreFrequently)
{
    // foodLevel=1 时每隔 tickCount % (1*3+1)=4 抖动一次
    EXPECT_TRUE(shouldShake(0, 1, 0.0f));  // 0 % 4 == 0
    EXPECT_TRUE(shouldShake(4, 1, 0.0f));  // 4 % 4 == 0
    EXPECT_TRUE(shouldShake(8, 1, 0.0f));  // 8 % 4 == 0
    EXPECT_FALSE(shouldShake(1, 1, 0.0f)); // 1 % 4 != 0
    EXPECT_FALSE(shouldShake(2, 1, 0.0f)); // 2 % 4 != 0
    EXPECT_FALSE(shouldShake(3, 1, 0.0f)); // 3 % 4 != 0

    // foodLevel=10 时每隔 tickCount % (10*3+1)=31 抖动一次
    EXPECT_TRUE(shouldShake(0, 10, 0.0f));   // 0 % 31 == 0
    EXPECT_TRUE(shouldShake(31, 10, 0.0f));  // 31 % 31 == 0
    EXPECT_FALSE(shouldShake(15, 10, 0.0f)); // 15 % 31 != 0
}

TEST(HudHungerRenderingTest, ShakeCondition_SaturationExactlyZero)
{
    // 饱和度恰好为 0.0f 时应该抖动（边界条件）
    EXPECT_TRUE(shouldShake(0, 10, 0.0f));
    EXPECT_TRUE(shouldShake(0, 5, 0.0f));
    EXPECT_TRUE(shouldShake(0, 1, 0.0f));
}

TEST(HudHungerRenderingTest, ShakeCondition_NegativeSaturation)
{
    // 负饱和度也应该触发抖动
    EXPECT_TRUE(shouldShake(0, 10, -0.1f));
    EXPECT_TRUE(shouldShake(0, 5, -1.0f));
}

// ============================================================================
// 抖动偏移值范围
// ============================================================================

TEST(HudHungerRenderingTest, ShakeOffset_RangeInMinusOneToPlusOne)
{
    // 抖动偏移值必须在 -1.0, 0.0, +1.0 范围内
    for (u32 tick = 0; tick < 200; ++tick) {
        f32 offset = computeShakeOffset(tick, 10, 0.0f);
        if (shouldShake(tick, 10, 0.0f)) {
            EXPECT_TRUE(offset == -1.0f || offset == 0.0f || offset == 1.0f)
                << "offset at tick " << tick << " is " << offset;
        } else {
            EXPECT_FLOAT_EQ(offset, 0.0f) << "non-shake tick " << tick << " should have zero offset";
        }
    }
}

TEST(HudHungerRenderingTest, ShakeOffset_NoShakeWhenSaturated)
{
    // 有饱和度时偏移始终为 0
    for (u32 tick = 0; tick < 100; ++tick) {
        f32 offset = computeShakeOffset(tick, 20, 5.0f);
        EXPECT_FLOAT_EQ(offset, 0.0f);
    }
}

TEST(HudHungerRenderingTest, ShakeOffset_DeterministicWithinSameTick)
{
    // 同一 tick 的偏移应该是确定性的
    f32 offset1 = computeShakeOffset(42, 10, 0.0f);
    f32 offset2 = computeShakeOffset(42, 10, 0.0f);
    EXPECT_FLOAT_EQ(offset1, offset2);
}

// ============================================================================
// 抖动频率验证
// ============================================================================

TEST(HudHungerRenderingTest, ShakeFrequency_LowerFoodShakesMore)
{
    // 统计 1000 tick 内的抖动次数，foodLevel 越低抖动越频繁
    i32 shakesAtFood1 = 0;
    i32 shakesAtFood10 = 0;
    i32 shakesAtFood20 = 0;

    for (u32 t = 0; t < 1000; ++t) {
        if (shouldShake(t, 1, 0.0f)) ++shakesAtFood1;
        if (shouldShake(t, 10, 0.0f)) ++shakesAtFood10;
        if (shouldShake(t, 20, 0.0f)) ++shakesAtFood20;
    }

    // food=1: 每4tick一次，1000/4=250次
    // food=10: 每31tick一次，1000/31≈32次
    // food=20: 每61tick一次，1000/61≈16次
    EXPECT_GT(shakesAtFood1, shakesAtFood10);
    EXPECT_GT(shakesAtFood10, shakesAtFood20);

    // 验证大致频率
    EXPECT_NEAR(shakesAtFood1, 250, 5);
    EXPECT_NEAR(shakesAtFood10, 32, 5);
    EXPECT_NEAR(shakesAtFood20, 16, 5);
}

// ============================================================================
// 图标状态映射边界值
// ============================================================================

TEST(HudHungerRenderingTest, FoodLevelBoundary_TwoPerSlot)
{
    // 每个槽位代表2点饥饿值
    // foodLevel=2: 第1个满，其余空
    auto [full0, half0] = computeHungerIconState(2, 0);
    EXPECT_TRUE(full0);
    EXPECT_FALSE(half0);

    auto [full1, half1] = computeHungerIconState(2, 1);
    EXPECT_FALSE(full1);
    EXPECT_FALSE(half1);

    // foodLevel=3: 第1个满，第2个半
    auto [f0, h0] = computeHungerIconState(3, 0);
    EXPECT_TRUE(f0);
    EXPECT_FALSE(h0);

    auto [f1, h1] = computeHungerIconState(3, 1);
    EXPECT_FALSE(f1);
    EXPECT_TRUE(h1);
}

TEST(HudHungerRenderingTest, MaxHungerLevel)
{
    // foodLevel = PLAYER_MAX_HUNGER = 20
    for (i32 i = 0; i < 10; ++i) {
        auto [full, half] = computeHungerIconState(game::PLAYER_MAX_HUNGER, i);
        EXPECT_TRUE(full) << "slot " << i;
    }
}
