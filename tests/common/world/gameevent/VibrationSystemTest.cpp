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

#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

using namespace mc;
using namespace mc::gameevent;

// ============================================================================
// isIgnoredBySneaking 测试 - 验证硬编码事件 ID 与 GameEvents 常量一致
// ============================================================================

TEST(VibrationSystemTest, IsIgnoredBySneaking_HitGround)
{
    // 验证 "hit_ground" 字符串与 GameEvents::HIT_GROUND 的 ID 一致
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::HIT_GROUND));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_ProjectileShoot)
{
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::PROJECTILE_SHOOT));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_Step)
{
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::STEP));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_Swim)
{
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::SWIM));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_ItemInteractStart)
{
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::ITEM_INTERACT_START));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_ItemInteractFinish)
{
    EXPECT_TRUE(VibrationSystem::isIgnoredBySneaking(GameEvents::ITEM_INTERACT_FINISH));
}

// ============================================================================
// isIgnoredBySneaking 测试 - 不应被潜行忽略的事件
// ============================================================================

TEST(VibrationSystemTest, IsIgnoredBySneaking_BlockPlace_NotIgnored)
{
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::BLOCK_PLACE));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_BlockDestroy_NotIgnored)
{
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::BLOCK_DESTROY));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_EntityDamage_NotIgnored)
{
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::ENTITY_DAMAGE));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_Explode_NotIgnored)
{
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::EXPLODE));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_BlockActivate_NotIgnored)
{
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::BLOCK_ACTIVATE));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_Flap_NotIgnored)
{
    // FLAP 不在 IGNORE_VIBRATIONS_SNEAKING 中，即使频率为 1
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::FLAP));
}

TEST(VibrationSystemTest, IsIgnoredBySneaking_SculkSensorTendrilsClicking_NotIgnored)
{
    EXPECT_FALSE(VibrationSystem::isIgnoredBySneaking(GameEvents::SCULK_SENSOR_TENDRILS_CLICKING));
}

// ============================================================================
// getGameEventFrequency 测试 - 验证频率映射正确性
// ============================================================================

TEST(VibrationSystemTest, GetFrequency_StepIs1)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::STEP), 1);
}

TEST(VibrationSystemTest, GetFrequency_SwimIs1)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::SWIM), 1);
}

TEST(VibrationSystemTest, GetFrequency_ProjectileLandIs2)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::PROJECTILE_LAND), 2);
}

TEST(VibrationSystemTest, GetFrequency_BlockActivateIs5)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::BLOCK_ACTIVATE), 5);
}

TEST(VibrationSystemTest, GetFrequency_EntityDamageIs11)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::ENTITY_DAMAGE), 11);
}

TEST(VibrationSystemTest, GetFrequency_ShriekIs15)
{
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::SHRIEK), 15);
}

// ============================================================================
// VibrationSelector 测试
// ============================================================================

TEST(VibrationSystemTest, VibrationSelector_CloserEventWins)
{
    VibrationSelector selector;
    u64 tick = 100;

    GameEvent closerEvent("test_closer", 16);
    GameEvent fartherEvent("test_farther", 16);

    VibrationInfo closerInfo(closerEvent, 5.0f, Vector3d(0, 0, 0), nullptr);
    VibrationInfo fartherInfo(fartherEvent, 10.0f, Vector3d(0, 0, 0), nullptr);

    // 先添加远的事件，再添加近的事件（同 tick 内近的优先）
    selector.addCandidate(fartherInfo, tick);
    selector.addCandidate(closerInfo, tick);

    // 在下一个 tick 才能选择
    auto chosen = selector.chosenCandidate(tick + 1);
    ASSERT_TRUE(chosen.has_value());
    // 距离近的应该被选中
    EXPECT_FLOAT_EQ(chosen->distance, 5.0f);
}

TEST(VibrationSystemTest, VibrationSelector_MustWaitOneTick)
{
    VibrationSelector selector;
    u64 tick = 100;

    GameEvent event("test", 16);
    VibrationInfo info(event, 5.0f, Vector3d(0, 0, 0), nullptr);

    selector.addCandidate(info, tick);

    // 同一 tick 不能选择
    auto chosen = selector.chosenCandidate(tick);
    EXPECT_FALSE(chosen.has_value());

    // 下一 tick 才能选择
    chosen = selector.chosenCandidate(tick + 1);
    EXPECT_TRUE(chosen.has_value());
}

TEST(VibrationSystemTest, VibrationSelector_StartOverClearsCandidate)
{
    VibrationSelector selector;
    u64 tick = 100;

    GameEvent event("test", 16);
    VibrationInfo info(event, 5.0f, Vector3d(0, 0, 0), nullptr);

    selector.addCandidate(info, tick);
    selector.startOver();

    auto chosen = selector.chosenCandidate(tick + 1);
    EXPECT_FALSE(chosen.has_value());
}

// ============================================================================
// getRedstoneStrengthForDistance 测试
// ============================================================================

TEST(VibrationSystemTest, GetRedstoneStrength_CloseRange)
{
    // 距离为 0 时应返回最大信号强度 15
    EXPECT_EQ(VibrationSystem::getRedstoneStrengthForDistance(0.0f, 8), 15);
}

TEST(VibrationSystemTest, GetRedstoneStrength_FarRange)
{
    // 距离接近半径时应返回较低的信号强度
    i32 strength = VibrationSystem::getRedstoneStrengthForDistance(7.5f, 8);
    EXPECT_GE(strength, 1);
    EXPECT_LE(strength, 15);
}

TEST(VibrationSystemTest, GetRedstoneStrength_AlwaysAtLeast1)
{
    // 信号强度至少为 1
    i32 strength = VibrationSystem::getRedstoneStrengthForDistance(7.99f, 8);
    EXPECT_GE(strength, 1);
}

// ============================================================================
// getResonanceEventByFrequency 测试
// ============================================================================

TEST(VibrationSystemTest, GetResonanceEvent_Frequency1)
{
    const GameEvent* event = VibrationSystem::getResonanceEventByFrequency(1);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(*event), 1);
}

TEST(VibrationSystemTest, GetResonanceEvent_Frequency15)
{
    const GameEvent* event = VibrationSystem::getResonanceEventByFrequency(15);
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(*event), 15);
}

TEST(VibrationSystemTest, GetResonanceEvent_InvalidFrequency0)
{
    const GameEvent* event = VibrationSystem::getResonanceEventByFrequency(0);
    EXPECT_EQ(event, nullptr);
}

TEST(VibrationSystemTest, GetResonanceEvent_InvalidFrequency16)
{
    const GameEvent* event = VibrationSystem::getResonanceEventByFrequency(16);
    EXPECT_EQ(event, nullptr);
}

// ============================================================================
// VibrationInfo 构造测试
// ============================================================================

TEST(VibrationSystemTest, VibrationInfo_WithEntity)
{
    GameEvent event("test_event", 16);
    Vector3d pos(10.0, 64.0, 20.0);
    VibrationInfo info(event, 5.0f, pos, nullptr);

    EXPECT_EQ(info.gameEvent, &event);
    EXPECT_FLOAT_EQ(info.distance, 5.0f);
    EXPECT_FALSE(info.hasSourceEntity);
    EXPECT_EQ(info.sourceEntityId, 0u);
}

TEST(VibrationSystemTest, VibrationInfo_DefaultConstructor)
{
    VibrationInfo info;
    EXPECT_EQ(info.gameEvent, nullptr);
    EXPECT_FLOAT_EQ(info.distance, 0.0f);
    EXPECT_FALSE(info.hasSourceEntity);
}
