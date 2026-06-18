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

#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"

using namespace mc;
using namespace mc::gameevent;

// ========== GameEvent 类测试 ==========

TEST(GameEventTest, Construction_DefaultRadius)
{
    GameEvent event("test_event");
    EXPECT_STREQ(event.id(), "test_event");
    EXPECT_EQ(event.notificationRadius(), GameEvent::DEFAULT_NOTIFICATION_RADIUS);
    EXPECT_EQ(event.notificationRadius(), 16);
}

TEST(GameEventTest, Construction_CustomRadius)
{
    GameEvent event("custom_event", 10);
    EXPECT_STREQ(event.id(), "custom_event");
    EXPECT_EQ(event.notificationRadius(), 10);
}

TEST(GameEventTest, Construction_LargeRadius)
{
    GameEvent event("loud_event", 32);
    EXPECT_EQ(event.notificationRadius(), 32);
}

// ========== GameEvent::Context 测试 ==========

TEST(GameEventContextTest, DefaultContext_HasNullEntityAndState)
{
    GameEvent::Context ctx;
    EXPECT_EQ(ctx.sourceEntity(), nullptr);
    EXPECT_EQ(ctx.affectedState(), nullptr);
}

TEST(GameEventContextTest, OfEntity_StoresEntity)
{
    const Entity* entity = reinterpret_cast<const Entity*>(0x1234);
    auto ctx = GameEvent::Context::of(entity);
    EXPECT_EQ(ctx.sourceEntity(), entity);
    EXPECT_EQ(ctx.affectedState(), nullptr);
}

TEST(GameEventContextTest, OfBlockState_StoresBlockState)
{
    const BlockState* state = reinterpret_cast<const BlockState*>(0x5678);
    auto ctx = GameEvent::Context::of(state);
    EXPECT_EQ(ctx.sourceEntity(), nullptr);
    EXPECT_EQ(ctx.affectedState(), state);
}

TEST(GameEventContextTest, OfEntityAndBlockState_StoresBoth)
{
    const Entity* entity = reinterpret_cast<const Entity*>(0x1234);
    const BlockState* state = reinterpret_cast<const BlockState*>(0x5678);
    auto ctx = GameEvent::Context::of(entity, state);
    EXPECT_EQ(ctx.sourceEntity(), entity);
    EXPECT_EQ(ctx.affectedState(), state);
}

TEST(GameEventContextTest, OfNullEntityAndNullState)
{
    auto ctx = GameEvent::Context::of(nullptr, nullptr);
    EXPECT_EQ(ctx.sourceEntity(), nullptr);
    EXPECT_EQ(ctx.affectedState(), nullptr);
}

// ========== GameEvents 常量测试 ==========

TEST(GameEventsTest, BlockEvents_HaveDefaultRadius)
{
    EXPECT_EQ(GameEvents::BLOCK_ACTIVATE.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::BLOCK_CHANGE.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::BLOCK_CLOSE.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::BLOCK_OPEN.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::BLOCK_PLACE.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::BLOCK_DESTROY.notificationRadius(), 16);
}

TEST(GameEventsTest, JukeboxEvents_HaveRadius10)
{
    // JUKEBOX_PLAY 和 JUKEBOX_STOP_PLAY 的通知半径为 10 格
    EXPECT_EQ(GameEvents::JUKEBOX_PLAY.notificationRadius(), 10);
    EXPECT_EQ(GameEvents::JUKEBOX_STOP_PLAY.notificationRadius(), 10);
}

TEST(GameEventsTest, ShriekEvent_HasRadius32)
{
    // SHRIEK 的通知半径为 32 格
    EXPECT_EQ(GameEvents::SHRIEK.notificationRadius(), 32);
}

TEST(GameEventsTest, ContainerEvents_HaveDefaultRadius)
{
    EXPECT_EQ(GameEvents::CONTAINER_OPEN.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::CONTAINER_CLOSE.notificationRadius(), 16);
}

TEST(GameEventsTest, EntityEvents_HaveDefaultRadius)
{
    EXPECT_EQ(GameEvents::ENTITY_DAMAGE.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::ENTITY_DIE.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::ENTITY_MOUNT.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::ENTITY_DISMOUNT.notificationRadius(), 16);
}

TEST(GameEventsTest, FluidEvents_HaveDefaultRadius)
{
    EXPECT_EQ(GameEvents::FLUID_PICKUP.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::FLUID_PLACE.notificationRadius(), 16);
}

TEST(GameEventsTest, ResonateEvents_HaveDefaultRadius)
{
    EXPECT_EQ(GameEvents::RESONATE_1.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::RESONATE_8.notificationRadius(), 16);
    EXPECT_EQ(GameEvents::RESONATE_15.notificationRadius(), 16);
}

TEST(GameEventsTest, EventIds_AreNonEmpty)
{
    // 验证所有事件 ID 不为空且格式正确
    EXPECT_STREQ(GameEvents::BLOCK_ACTIVATE.id(), "block_activate");
    EXPECT_STREQ(GameEvents::BLOCK_CHANGE.id(), "block_change");
    EXPECT_STREQ(GameEvents::JUKEBOX_PLAY.id(), "jukebox_play");
    EXPECT_STREQ(GameEvents::JUKEBOX_STOP_PLAY.id(), "jukebox_stop_play");
    EXPECT_STREQ(GameEvents::SHRIEK.id(), "shriek");
    EXPECT_STREQ(GameEvents::NOTE_BLOCK_PLAY.id(), "note_block_play");
    EXPECT_STREQ(GameEvents::CONTAINER_OPEN.id(), "container_open");
    EXPECT_STREQ(GameEvents::ENTITY_DAMAGE.id(), "entity_damage");
    EXPECT_STREQ(GameEvents::FLUID_PICKUP.id(), "fluid_pickup");
    EXPECT_STREQ(GameEvents::RESONATE_1.id(), "resonate_1");
}

TEST(GameEventsTest, DistinctEventsHaveDistinctIds)
{
    // 验证不同事件有不同的 ID
    EXPECT_STRNE(GameEvents::BLOCK_ACTIVATE.id(), GameEvents::BLOCK_CHANGE.id());
    EXPECT_STRNE(GameEvents::JUKEBOX_PLAY.id(), GameEvents::JUKEBOX_STOP_PLAY.id());
    EXPECT_STRNE(GameEvents::CONTAINER_OPEN.id(), GameEvents::CONTAINER_CLOSE.id());
}
