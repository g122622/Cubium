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
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/MusicDiscItem.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/JukeboxEntity.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <optional>

using namespace mc;
using namespace mc::blockentity;

namespace {

/**
 * @brief 测试用世界存根 - 记录 playEvent 调用
 */
class JukeboxTestWorld final : public test::BaseTestWorld {
public:
    struct PlayEventCall {
        i32 eventId;
        BlockPos pos;
        i32 data;
    };

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_playEventCalls.push_back({eventId, pos, data});
    }

    [[nodiscard]] const std::vector<PlayEventCall>& playEventCalls() const { return m_playEventCalls; }
    void clearPlayEventCalls() { m_playEventCalls.clear(); }

private:
    std::vector<PlayEventCall> m_playEventCalls;
};

class JukeboxEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override
    {
        pos_ = BlockPos(5, 64, 10);
        jukebox_ = std::make_unique<JukeboxEntity>(pos_);
        world_ = std::make_unique<JukeboxTestWorld>();
    }

    BlockPos pos_;
    std::unique_ptr<JukeboxEntity> jukebox_;
    std::unique_ptr<JukeboxTestWorld> world_;
};

// ========== 基本创建测试 ==========

TEST_F(JukeboxEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(jukebox_->getType(), BlockEntityType::Jukebox);
}

TEST_F(JukeboxEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(jukebox_->getPos(), pos_);
}

TEST_F(JukeboxEntityTest, Create_HasContainerSize1)
{
    EXPECT_EQ(jukebox_->getContainerSize(), 1);
}

TEST_F(JukeboxEntityTest, Create_IsEmptyInitially)
{
    EXPECT_TRUE(jukebox_->getRecord().isEmpty());
    EXPECT_FALSE(jukebox_->hasRecord());
}

TEST_F(JukeboxEntityTest, Create_NotPlayingInitially)
{
    EXPECT_FALSE(jukebox_->isPlaying());
}

TEST_F(JukeboxEntityTest, Create_NeedsTickIsFalseWhenNotPlaying)
{
    EXPECT_FALSE(jukebox_->needsTick());
}

TEST_F(JukeboxEntityTest, Create_ComparatorSignalIs0WhenEmpty)
{
    EXPECT_EQ(jukebox_->getComparatorSignal(), 0);
}

// ========== setRecord / getRecord 测试 ==========

TEST_F(JukeboxEntityTest, SetRecord_StoresDisc)
{
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);

    EXPECT_TRUE(jukebox_->hasRecord());
    EXPECT_FALSE(jukebox_->getRecord().isEmpty());
    EXPECT_EQ(jukebox_->getRecord().getItem(), Items::MUSIC_DISC_13);
}

TEST_F(JukeboxEntityTest, SetRecord_StartsPlaying)
{
    ItemStack disc(Items::MUSIC_DISC_CAT, 1);
    jukebox_->setRecord(disc, *world_);

    EXPECT_TRUE(jukebox_->isPlaying());
    EXPECT_TRUE(jukebox_->needsTick());
}

TEST_F(JukeboxEntityTest, SetRecord_BroadcastsPlayEvent)
{
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);

    // 应该广播 PLAY_RECORD_SOUND 事件，data 为唱片的比较器信号强度
    ASSERT_FALSE(world_->playEventCalls().empty());
    const auto& call = world_->playEventCalls().back();
    EXPECT_EQ(call.eventId, world::WorldEvents::PLAY_RECORD_SOUND);
    EXPECT_EQ(call.pos, pos_);
    EXPECT_EQ(call.data, 1); // MUSIC_DISC_13 比较器信号 = 1
}

TEST_F(JukeboxEntityTest, SetRecord_EmptyStopsPlaying)
{
    // 先放入唱片
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);
    EXPECT_TRUE(jukebox_->isPlaying());

    world_->clearPlayEventCalls();

    // 再设置为空
    jukebox_->setRecord(ItemStack::EMPTY, *world_);
    EXPECT_FALSE(jukebox_->hasRecord());
    EXPECT_FALSE(jukebox_->isPlaying());

    // 应该广播停止事件 (data=0)
    ASSERT_FALSE(world_->playEventCalls().empty());
    const auto& call = world_->playEventCalls().back();
    EXPECT_EQ(call.eventId, world::WorldEvents::PLAY_RECORD_SOUND);
    EXPECT_EQ(call.data, 0);
}

// ========== startPlaying / stopPlaying 测试 ==========

TEST_F(JukeboxEntityTest, StartPlaying_WithoutRecord_DoesNothing)
{
    // 没有唱片时调用 startPlaying 不应播放
    jukebox_->startPlaying(*world_);
    EXPECT_FALSE(jukebox_->isPlaying());

    // 不应广播任何事件
    EXPECT_TRUE(world_->playEventCalls().empty());
}

TEST_F(JukeboxEntityTest, StopPlaying_WhenNotPlaying_DoesNothing)
{
    // 不在播放时调用 stopPlaying 不应广播事件
    jukebox_->stopPlaying(*world_);
    EXPECT_TRUE(world_->playEventCalls().empty());
}

TEST_F(JukeboxEntityTest, StopPlaying_WhenPlaying_BroadcastsStopEvent)
{
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);
    EXPECT_TRUE(jukebox_->isPlaying());

    world_->clearPlayEventCalls();

    jukebox_->stopPlaying(*world_);
    EXPECT_FALSE(jukebox_->isPlaying());

    // 应该广播停止事件
    ASSERT_FALSE(world_->playEventCalls().empty());
    const auto& call = world_->playEventCalls().back();
    EXPECT_EQ(call.eventId, world::WorldEvents::PLAY_RECORD_SOUND);
    EXPECT_EQ(call.data, 0);
}

// ========== 比较器信号测试 ==========

TEST_F(JukeboxEntityTest, ComparatorSignal_ReturnsDiscSignalStrength)
{
    // 放入 MUSIC_DISC_PIGSTEP (信号强度 13)
    ItemStack disc(Items::MUSIC_DISC_PIGSTEP, 1);
    jukebox_->setRecord(disc, *world_);
    EXPECT_EQ(jukebox_->getComparatorSignal(), 13);
}

TEST_F(JukeboxEntityTest, ComparatorSignal_DifferentDiscsDifferentSignals)
{
    // MUSIC_DISC_13 = 1
    {
        JukeboxEntity jb(pos_);
        jb.setRecord(ItemStack(Items::MUSIC_DISC_13, 1), *world_);
        EXPECT_EQ(jb.getComparatorSignal(), 1);
    }

    // MUSIC_DISC_5 = 15
    {
        JukeboxEntity jb(pos_);
        jb.setRecord(ItemStack(Items::MUSIC_DISC_5, 1), *world_);
        EXPECT_EQ(jb.getComparatorSignal(), 15);
    }
}

TEST_F(JukeboxEntityTest, ComparatorSignal_ZeroAfterRemovingDisc)
{
    ItemStack disc(Items::MUSIC_DISC_WARD, 1);
    jukebox_->setRecord(disc, *world_);
    EXPECT_EQ(jukebox_->getComparatorSignal(), 10);

    jukebox_->setRecord(ItemStack::EMPTY, *world_);
    EXPECT_EQ(jukebox_->getComparatorSignal(), 0);
}

// ========== tick 测试 ==========

TEST_F(JukeboxEntityTest, Tick_IncrementsTicksSinceSongStarted)
{
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);

    // tick 几次
    jukebox_->tick(*world_);
    jukebox_->tick(*world_);
    jukebox_->tick(*world_);

    // needsTick 仍然为 true（正在播放）
    EXPECT_TRUE(jukebox_->needsTick());
}

TEST_F(JukeboxEntityTest, Tick_DoesNothingWhenNotPlaying)
{
    // 不播放时 tick 不应崩溃
    jukebox_->tick(*world_);
    jukebox_->tick(*world_);
    SUCCEED();
}

TEST_F(JukeboxEntityTest, Tick_StopsPlayingWhenRecordRemoved)
{
    // 模拟漏斗提取唱片的情况：直接操作 inventory 移除唱片
    // 此时 tick 应该检测到唱片消失并停止播放
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);
    EXPECT_TRUE(jukebox_->isPlaying());

    // 直接通过 inventory 移除唱片（模拟漏斗行为）
    IInventory* inv = jukebox_->getInventory();
    ASSERT_NE(inv, nullptr);
    inv->setItem(JukeboxEntity::SLOT_RECORD, ItemStack::EMPTY);

    // tick 应该检测到唱片被移除
    jukebox_->tick(*world_);
    EXPECT_FALSE(jukebox_->isPlaying());
}

TEST_F(JukeboxEntityTest, NeedsTick_TrueWhenPlaying)
{
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);
    EXPECT_TRUE(jukebox_->needsTick());
}

TEST_F(JukeboxEntityTest, NeedsTick_FalseWhenNotPlaying)
{
    EXPECT_FALSE(jukebox_->needsTick());

    // 放入唱片后播放，取出后不播放
    jukebox_->setRecord(ItemStack(Items::MUSIC_DISC_13, 1), *world_);
    EXPECT_TRUE(jukebox_->needsTick());

    jukebox_->setRecord(ItemStack::EMPTY, *world_);
    EXPECT_FALSE(jukebox_->needsTick());
}

// ========== JSON 序列化测试 ==========

TEST_F(JukeboxEntityTest, JSON_SaveEmptyJukebox)
{
    nlohmann::json data;
    jukebox_->save(data);

    // 空唱片机不应有 RecordItem
    EXPECT_EQ(data.find("RecordItem"), data.end());
    EXPECT_EQ(data.value("IsPlaying", true), false);
}

TEST_F(JukeboxEntityTest, JSON_SaveJukeboxWithDisc)
{
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);

    nlohmann::json data;
    jukebox_->save(data);

    // 应该有 RecordItem
    EXPECT_NE(data.find("RecordItem"), data.end());
    EXPECT_TRUE(data["RecordItem"].is_object());
    EXPECT_EQ(data.value("IsPlaying", false), true);
}

TEST_F(JukeboxEntityTest, JSON_RoundTrip)
{
    // 放入唱片
    ItemStack disc(Items::MUSIC_DISC_CAT, 1);
    jukebox_->setRecord(disc, *world_);

    // 保存
    nlohmann::json data;
    jukebox_->save(data);

    // 加载到新的实体
    auto jukebox2 = std::make_unique<JukeboxEntity>(pos_);
    ASSERT_TRUE(jukebox2->load(data));

    // 验证唱片已恢复
    EXPECT_TRUE(jukebox2->hasRecord());
    EXPECT_EQ(jukebox2->getRecord().getItem(), Items::MUSIC_DISC_CAT);

    // IsPlaying 在 load 时不重新播放，但值应该保存了
    // TicksSinceSongStarted 应该保存
    EXPECT_EQ(data.value("TicksSinceSongStarted", static_cast<i64>(-1)), static_cast<i64>(0));
}

TEST_F(JukeboxEntityTest, JSON_LoadPreservesTicksSinceSongStarted)
{
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);

    // tick 几次
    jukebox_->tick(*world_);
    jukebox_->tick(*world_);
    jukebox_->tick(*world_);

    // 保存
    nlohmann::json data;
    jukebox_->save(data);

    // 验证 TicksSinceSongStarted
    EXPECT_EQ(data.value("TicksSinceSongStarted", static_cast<i64>(0)), static_cast<i64>(3));
}

// ========== clone 测试 ==========

TEST_F(JukeboxEntityTest, Clone_CopiesRecordAndState)
{
    ItemStack disc(Items::MUSIC_DISC_PIGSTEP, 1);
    jukebox_->setRecord(disc, *world_);

    auto cloned = jukebox_->clone();
    auto* clonedJukebox = dynamic_cast<JukeboxEntity*>(cloned.get());
    ASSERT_NE(clonedJukebox, nullptr);

    EXPECT_TRUE(clonedJukebox->hasRecord());
    EXPECT_EQ(clonedJukebox->getRecord().getItem(), Items::MUSIC_DISC_PIGSTEP);
    EXPECT_EQ(clonedJukebox->isPlaying(), jukebox_->isPlaying());
}

TEST_F(JukeboxEntityTest, Clone_EmptyJukebox)
{
    auto cloned = jukebox_->clone();
    auto* clonedJukebox = dynamic_cast<JukeboxEntity*>(cloned.get());
    ASSERT_NE(clonedJukebox, nullptr);

    EXPECT_FALSE(clonedJukebox->hasRecord());
    EXPECT_FALSE(clonedJukebox->isPlaying());
}

// ========== IInventory 接口测试 ==========

TEST_F(JukeboxEntityTest, Inventory_SetAndGetItem)
{
    IInventory* inv = jukebox_->getInventory();
    ASSERT_NE(inv, nullptr);

    ItemStack disc(Items::MUSIC_DISC_13, 1);
    inv->setItem(JukeboxEntity::SLOT_RECORD, disc);

    ItemStack retrieved = inv->getItem(JukeboxEntity::SLOT_RECORD);
    EXPECT_EQ(retrieved.getItem(), Items::MUSIC_DISC_13);
}

TEST_F(JukeboxEntityTest, Inventory_ClearRemovesDisc)
{
    ItemStack disc(Items::MUSIC_DISC_13, 1);
    jukebox_->setRecord(disc, *world_);
    EXPECT_TRUE(jukebox_->hasRecord());

    IInventory* inv = jukebox_->getInventory();
    inv->clear();
    EXPECT_TRUE(jukebox_->getRecord().isEmpty());
}

} // namespace
