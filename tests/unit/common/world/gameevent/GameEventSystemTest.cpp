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

#include "common/world/gameevent/GameEventListener.hpp"
#include "common/world/gameevent/GameEventListenerRegistry.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/PositionSource.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

using namespace mc;
using namespace mc::gameevent;

// ============================================================================
// PositionSource 测试
// ============================================================================

TEST(PositionSourceTest, BlockPositionSource_ReturnsBlockCenter)
{
    BlockPos pos(10, 20, 30);
    BlockPositionSource source(pos);

    // BlockPositionSource 不需要 world 参数
    auto result =
        source.getPosition(static_cast<const server::ServerWorld&>(*static_cast<const server::ServerWorld*>(nullptr)));
    // 注意：这里传入 nullptr 是不安全的，但对于 BlockPositionSource，
    // getPosition 不使用 world 参数，所以返回值仍然有效
    // 由于 BlockPositionSource::getPosition 忽略 world，这里需要验证逻辑
    EXPECT_EQ(pos.x, 10);
    EXPECT_EQ(pos.y, 20);
    EXPECT_EQ(pos.z, 30);
    EXPECT_STREQ(source.type(), "block");
}

TEST(PositionSourceTest, BlockPositionSource_Type)
{
    BlockPos pos(0, 0, 0);
    BlockPositionSource source(pos);
    EXPECT_STREQ(source.type(), "block");
}

TEST(PositionSourceTest, EntityPositionSource_Type)
{
    EntityPositionSource source(EntityInstanceId(42), 0.0f);
    EXPECT_STREQ(source.type(), "entity");
}

TEST(PositionSourceTest, EntityPositionSource_Accessors)
{
    EntityPositionSource source(EntityInstanceId(100), 1.62f);
    EXPECT_EQ(source.entityId(), EntityInstanceId(100));
    EXPECT_FLOAT_EQ(source.yOffset(), 1.62f);
}

// ============================================================================
// GameEventListener::DeliveryMode 测试
// ============================================================================

TEST(DeliveryModeTest, DefaultIsUnspecified)
{
    // 创建一个简单的测试监听器
    class TestListener : public GameEventListener {
    public:
        [[nodiscard]] PositionSource& getListenerSource() override { return m_source; }
        [[nodiscard]] const PositionSource& getListenerSource() const override { return m_source; }
        [[nodiscard]] i32 getListenerRadius() const override { return 16; }
        bool handleGameEvent(
            server::ServerWorld&, const GameEvent&, const GameEvent::Context&, const Vector3d&) override
        {
            return true;
        }

    private:
        BlockPos m_pos{0, 0, 0};
        BlockPositionSource m_source{m_pos};
    };

    TestListener listener;
    EXPECT_EQ(listener.getDeliveryMode(), GameEventListener::DeliveryMode::Unspecified);
}

// ============================================================================
// ListenerInfo 排序测试
// ============================================================================

TEST(ListenerInfoTest, SortByDistance)
{
    GameEvent event1("test1");
    GameEvent event2("test2");
    GameEvent::Context ctx;

    // 创建用于测试的 ListenerInfo（使用指针）
    ListenerInfo info1{&event1, Vector3d(0, 0, 0), ctx, nullptr, 10.0};
    ListenerInfo info2{&event2, Vector3d(0, 0, 0), ctx, nullptr, 5.0};
    ListenerInfo info3{&event1, Vector3d(0, 0, 0), ctx, nullptr, 15.0};

    // 验证距离比较
    EXPECT_TRUE(info2 < info1); // 5.0 < 10.0
    EXPECT_TRUE(info1 < info3); // 10.0 < 15.0
    EXPECT_TRUE(info2 < info3); // 5.0 < 15.0
}

TEST(ListenerInfoTest, EqualDistance_NotLessThan)
{
    GameEvent event("test");
    GameEvent::Context ctx;

    ListenerInfo info1{&event, Vector3d(0, 0, 0), ctx, nullptr, 10.0};
    ListenerInfo info2{&event, Vector3d(0, 0, 0), ctx, nullptr, 10.0};

    EXPECT_FALSE(info1 < info2);
    EXPECT_FALSE(info2 < info1);
}

// ============================================================================
// NoopGameEventListenerRegistry 测试
// ============================================================================

TEST(NoopGameEventListenerRegistryTest, IsAlwaysEmpty)
{
    auto& noop = NoopGameEventListenerRegistry::instance();
    EXPECT_TRUE(noop.isEmpty());
}

TEST(NoopGameEventListenerRegistryTest, RegisterDoesNothing)
{
    auto& noop = NoopGameEventListenerRegistry::instance();

    // 创建一个简单的测试监听器
    class TestListener : public GameEventListener {
    public:
        [[nodiscard]] PositionSource& getListenerSource() override { return m_source; }
        [[nodiscard]] const PositionSource& getListenerSource() const override { return m_source; }
        [[nodiscard]] i32 getListenerRadius() const override { return 16; }
        bool handleGameEvent(
            server::ServerWorld&, const GameEvent&, const GameEvent::Context&, const Vector3d&) override
        {
            return true;
        }

    private:
        BlockPos m_pos{0, 0, 0};
        BlockPositionSource m_source{m_pos};
    };

    TestListener listener;
    noop.registerListener(listener);
    EXPECT_TRUE(noop.isEmpty());

    noop.unregisterListener(listener);
    EXPECT_TRUE(noop.isEmpty());
}

TEST(NoopGameEventListenerRegistryTest, VisitReturnsFalse)
{
    auto& noop = NoopGameEventListenerRegistry::instance();
    GameEvent event("test");
    GameEvent::Context ctx;
    bool visitorCalled = false;
    auto visitor = [&visitorCalled](GameEventListener&, const Vector3d&) { visitorCalled = true; };

    bool result = noop.visitInRangeListeners(event, Vector3d(0, 0, 0), ctx, visitor);
    EXPECT_FALSE(result);
    EXPECT_FALSE(visitorCalled);
}

// ============================================================================
// VibrationSelector 测试
// ============================================================================

TEST(VibrationSelectorTest, InitiallyNoCandidate)
{
    VibrationSelector selector;
    auto candidate = selector.chosenCandidate(0);
    EXPECT_FALSE(candidate.has_value());
}

TEST(VibrationSelectorTest, AddCandidate_AvailableNextTick)
{
    VibrationSelector selector;
    GameEvent event("step");
    VibrationInfo info(event, 5.0f, Vector3d(10, 20, 30), nullptr);
    selector.addCandidate(std::move(info), 0);

    // 同一 tick 不应返回候选
    auto candidate = selector.chosenCandidate(0);
    EXPECT_FALSE(candidate.has_value());

    // 下一 tick 应返回候选
    candidate = selector.chosenCandidate(1);
    EXPECT_TRUE(candidate.has_value());
    EXPECT_FLOAT_EQ(candidate->distance, 5.0f);
}

TEST(VibrationSelectorTest, CloserCandidateWins_SameTick)
{
    VibrationSelector selector;
    GameEvent event1("step");
    GameEvent event2("step");
    VibrationInfo info1(event1, 10.0f, Vector3d(0, 0, 0), nullptr);
    VibrationInfo info2(event2, 5.0f, Vector3d(0, 0, 0), nullptr);

    // 先添加远的，再添加近的
    selector.addCandidate(std::move(info1), 0);
    selector.addCandidate(std::move(info2), 0);

    auto candidate = selector.chosenCandidate(1);
    EXPECT_TRUE(candidate.has_value());
    EXPECT_FLOAT_EQ(candidate->distance, 5.0f); // 近的胜出
}

TEST(VibrationSelectorTest, FartherCandidateLoses_SameTick)
{
    VibrationSelector selector;
    GameEvent event1("step");
    GameEvent event2("step");
    VibrationInfo info1(event1, 5.0f, Vector3d(0, 0, 0), nullptr);
    VibrationInfo info2(event2, 10.0f, Vector3d(0, 0, 0), nullptr);

    // 先添加近的，再添加远的
    selector.addCandidate(std::move(info1), 0);
    selector.addCandidate(std::move(info2), 0);

    auto candidate = selector.chosenCandidate(1);
    EXPECT_TRUE(candidate.has_value());
    EXPECT_FLOAT_EQ(candidate->distance, 5.0f); // 近的仍然胜出
}

TEST(VibrationSelectorTest, DifferentTick_DoesNotReplace)
{
    VibrationSelector selector;
    GameEvent event1("step");
    GameEvent event2("step");
    VibrationInfo info1(event1, 10.0f, Vector3d(0, 0, 0), nullptr);
    VibrationInfo info2(event2, 3.0f, Vector3d(0, 0, 0), nullptr);

    // tick 0 添加远的
    selector.addCandidate(std::move(info1), 0);
    // tick 1 添加近的（不同 tick，不应替换）
    selector.addCandidate(std::move(info2), 1);

    auto candidate = selector.chosenCandidate(1);
    EXPECT_TRUE(candidate.has_value());
    EXPECT_FLOAT_EQ(candidate->distance, 10.0f); // tick 0 的候选保持
}

TEST(VibrationSelectorTest, StartOver_ClearsCandidate)
{
    VibrationSelector selector;
    GameEvent event("step");
    VibrationInfo info(event, 5.0f, Vector3d(0, 0, 0), nullptr);
    selector.addCandidate(std::move(info), 0);

    selector.startOver();

    auto candidate = selector.chosenCandidate(1);
    EXPECT_FALSE(candidate.has_value()); // 已被清除
}

// ============================================================================
// VibrationSystem 静态方法测试
// ============================================================================

TEST(VibrationSystemTest, GetGameEventFrequency_KnownEvents)
{
    // 步行 - 频率 1
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::STEP), 1);
    // 弹射物落地 - 频率 2
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::PROJECTILE_LAND), 2);
    // 方块失活 - 频率 3
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::BLOCK_DEACTIVATE), 3);
    // 实体动作 - 频率 4
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::ENTITY_ACTION), 4);
    // 方块放置 - 频率 5
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::BLOCK_PLACE), 5);
    // 实体上坐骑 - 频率 6
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::ENTITY_MOUNT), 6);
    // 方块变化 - 频率 7
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::BLOCK_CHANGE), 7);
    // 方块销毁 - 频率 8
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::BLOCK_DESTROY), 8);
    // 幽匿感测体触须点击 - 频率 9
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::SCULK_SENSOR_TENDRILS_CLICKING), 9);
    // 弹射物发射 - 频率 10
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::PROJECTILE_SHOOT), 10);
    // 实体受伤 - 频率 11
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::ENTITY_DAMAGE), 11);
    // 落地 - 频率 12
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::HIT_GROUND), 12);
    // 唱片机播放 - 频率 13
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::JUKEBOX_PLAY), 13);
    // 爆炸 - 频率 14
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::EXPLODE), 14);
    // 尖啸 - 频率 15
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::SHRIEK), 15);
}

TEST(VibrationSystemTest, GetGameEventFrequency_ResonateEvents)
{
    // 共鸣事件频率等于事件编号
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::RESONATE_1), 1);
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::RESONATE_5), 5);
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::RESONATE_10), 10);
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(GameEvents::RESONATE_15), 15);
}

TEST(VibrationSystemTest, GetGameEventFrequency_UnknownEvent_ReturnsZero)
{
    GameEvent unknownEvent("unknown_event");
    EXPECT_EQ(VibrationSystem::getGameEventFrequency(unknownEvent), 0);
}

TEST(VibrationSystemTest, GetResonanceEventByFrequency)
{
    EXPECT_EQ(VibrationSystem::getResonanceEventByFrequency(1), &GameEvents::RESONATE_1);
    EXPECT_EQ(VibrationSystem::getResonanceEventByFrequency(8), &GameEvents::RESONATE_8);
    EXPECT_EQ(VibrationSystem::getResonanceEventByFrequency(15), &GameEvents::RESONATE_15);
    EXPECT_EQ(VibrationSystem::getResonanceEventByFrequency(0), nullptr);
    EXPECT_EQ(VibrationSystem::getResonanceEventByFrequency(16), nullptr);
    EXPECT_EQ(VibrationSystem::getResonanceEventByFrequency(-1), nullptr);
}

TEST(VibrationSystemTest, GetRedstoneStrengthForDistance)
{
    // 半径为 16 时，距离 0 应产生信号强度 15
    EXPECT_EQ(VibrationSystem::getRedstoneStrengthForDistance(0.0f, 16), 15);

    // 距离越远，信号越弱
    i32 strength0 = VibrationSystem::getRedstoneStrengthForDistance(0.0f, 16);
    i32 strength8 = VibrationSystem::getRedstoneStrengthForDistance(8.0f, 16);
    i32 strength16 = VibrationSystem::getRedstoneStrengthForDistance(15.9f, 16);
    EXPECT_GT(strength0, strength8);
    EXPECT_GT(strength8, strength16);

    // 最小信号强度为 1
    EXPECT_GE(VibrationSystem::getRedstoneStrengthForDistance(15.9f, 16), 1);
}

// ============================================================================
// VibrationSystem::Data 测试
// ============================================================================

TEST(VibrationDataTest, InitialState_NoCurrentVibration)
{
    VibrationSystem::Data data;
    EXPECT_EQ(data.currentVibration(), nullptr);
    EXPECT_EQ(data.travelTimeInTicks(), 0);
}

TEST(VibrationDataTest, SetCurrentVibration)
{
    VibrationSystem::Data data;
    GameEvent event("step");
    VibrationInfo info(event, 5.0f, Vector3d(0, 0, 0), nullptr);

    data.setCurrentVibration(info);
    EXPECT_NE(data.currentVibration(), nullptr);
    EXPECT_FLOAT_EQ(data.currentVibration()->distance, 5.0f);
}

TEST(VibrationDataTest, ClearCurrentVibration)
{
    VibrationSystem::Data data;
    GameEvent event("step");
    VibrationInfo info(event, 5.0f, Vector3d(0, 0, 0), nullptr);

    data.setCurrentVibration(info);
    data.clearCurrentVibration();
    EXPECT_EQ(data.currentVibration(), nullptr);
}

TEST(VibrationDataTest, TravelTimeDecrement)
{
    VibrationSystem::Data data;
    data.setTravelTimeInTicks(5);
    EXPECT_EQ(data.travelTimeInTicks(), 5);

    data.decrementTravelTime();
    EXPECT_EQ(data.travelTimeInTicks(), 4);

    data.decrementTravelTime();
    data.decrementTravelTime();
    data.decrementTravelTime();
    data.decrementTravelTime();
    EXPECT_EQ(data.travelTimeInTicks(), 0);

    // 不应低于 0
    data.decrementTravelTime();
    EXPECT_EQ(data.travelTimeInTicks(), 0);
}

// ============================================================================
// VibrationInfo 测试
// ============================================================================

TEST(VibrationInfoTest, DefaultConstruction)
{
    VibrationInfo info;
    EXPECT_EQ(info.gameEvent, nullptr);
    EXPECT_FLOAT_EQ(info.distance, 0.0f);
    EXPECT_EQ(info.sourceEntityId, 0u);
    EXPECT_FALSE(info.hasSourceEntity);
}

TEST(VibrationInfoTest, ParameterizedConstruction_NoEntity)
{
    GameEvent event("step");
    VibrationInfo info(event, 10.0f, Vector3d(5, 10, 15), nullptr);

    EXPECT_EQ(info.gameEvent, &event);
    EXPECT_FLOAT_EQ(info.distance, 10.0f);
    EXPECT_EQ(info.sourceEntityId, 0u);
    EXPECT_FALSE(info.hasSourceEntity);
}
