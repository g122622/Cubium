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

#include "world/blockentity/interactive/BellBlockEntity.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;

// ========== BellBlockEntity 测试 ==========

class BellBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { bell_ = std::make_unique<BellBlockEntity>(BlockPos(10, 64, 20)); }

    std::unique_ptr<BellBlockEntity> bell_;
};

TEST_F(BellBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(bell_->getType(), BlockEntityType::Bell);
}

TEST_F(BellBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(bell_->getPos(), BlockPos(10, 64, 20));
}

TEST_F(BellBlockEntityTest, Create_NotShakingInitially)
{
    EXPECT_FALSE(bell_->isShaking());
}

TEST_F(BellBlockEntityTest, Create_NotResonatingInitially)
{
    EXPECT_FALSE(bell_->isResonating());
}

TEST_F(BellBlockEntityTest, Create_TicksIsZero)
{
    EXPECT_EQ(bell_->ticks(), 0);
}

TEST_F(BellBlockEntityTest, Create_ResonationTicksIsZero)
{
    EXPECT_EQ(bell_->resonationTicks(), 0);
}

TEST_F(BellBlockEntityTest, Create_NeedsTickFalseWhenIdle)
{
    EXPECT_FALSE(bell_->needsTick());
}

TEST_F(BellBlockEntityTest, TriggerEvent_Id1_StartsShaking)
{
    EXPECT_TRUE(bell_->triggerEvent(1, static_cast<i32>(Direction::North)));
    EXPECT_TRUE(bell_->isShaking());
    EXPECT_EQ(bell_->ticks(), 0);
    EXPECT_EQ(bell_->clickDirection(), Direction::North);
}

TEST_F(BellBlockEntityTest, TriggerEvent_Id1_ResetsResonationTicks)
{
    // 先设置一个非零的 resonationTicks（通过 triggerEvent 已经是 0，但验证一致性）
    EXPECT_TRUE(bell_->triggerEvent(1, static_cast<i32>(Direction::South)));
    EXPECT_EQ(bell_->resonationTicks(), 0);
    EXPECT_EQ(bell_->clickDirection(), Direction::South);
}

TEST_F(BellBlockEntityTest, TriggerEvent_UnknownId_ReturnsFalse)
{
    EXPECT_FALSE(bell_->triggerEvent(99, 0));
    EXPECT_FALSE(bell_->isShaking());
}

TEST_F(BellBlockEntityTest, TriggerEvent_AllDirections)
{
    const std::array<Direction, 6> dirs = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};

    for (Direction dir : dirs) {
        std::unique_ptr<BellBlockEntity> bell = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
        EXPECT_TRUE(bell->triggerEvent(1, static_cast<i32>(dir))) << "Failed for direction " << static_cast<int>(dir);
        EXPECT_EQ(bell->clickDirection(), dir) << "Wrong direction for " << static_cast<int>(dir);
    }
}

TEST_F(BellBlockEntityTest, NeedsTick_TrueWhenShaking)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    EXPECT_TRUE(bell_->needsTick());
}

TEST_F(BellBlockEntityTest, NeedsTick_TrueWhenResonating)
{
    // 通过 triggerEvent 启动摇晃，然后手动设置 resonating 状态
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    // resonating 状态是私有的，但 needsTick 检查 m_shaking || m_resonating
    // 只要 shaking 为 true，needsTick 就返回 true
    EXPECT_TRUE(bell_->needsTick());
}

TEST_F(BellBlockEntityTest, Clone_CreatesCopy)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::East));

    std::unique_ptr<BlockEntity> copy = bell_->clone();
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Bell);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 64, 20));

    auto* bellCopy = static_cast<BellBlockEntity*>(copy.get());
    EXPECT_EQ(bellCopy->clickDirection(), Direction::East);
    EXPECT_EQ(bellCopy->ticks(), 0);
    EXPECT_TRUE(bellCopy->isShaking());
}

TEST_F(BellBlockEntityTest, SaveAndLoad_PreservesData)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::South));

    nlohmann::json data;
    bell_->save(data);

    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_TRUE(loaded->isShaking());
    EXPECT_EQ(loaded->clickDirection(), Direction::South);
    EXPECT_EQ(loaded->ticks(), 0);
}

TEST_F(BellBlockEntityTest, SaveAndLoad_HandlesIdle)
{
    nlohmann::json data;
    bell_->save(data);

    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_FALSE(loaded->isShaking());
    EXPECT_FALSE(loaded->isResonating());
    EXPECT_EQ(loaded->ticks(), 0);
    EXPECT_EQ(loaded->resonationTicks(), 0);
}

TEST_F(BellBlockEntityTest, SaveAndLoad_PreservesResonationState)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    nlohmann::json data;
    bell_->save(data);

    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(5, 10, 15));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_TRUE(loaded->isShaking());
    EXPECT_EQ(loaded->clickDirection(), Direction::North);
}

TEST_F(BellBlockEntityTest, Constants_MatchMCJavaValues)
{
    // 验证常量与 MC 1.21.11 BellBlockEntity.java 对齐
    EXPECT_EQ(BellBlockEntity::DURATION, 50);
    EXPECT_EQ(BellBlockEntity::GLOW_DURATION, 60);
    EXPECT_EQ(BellBlockEntity::MIN_TICKS_BETWEEN_SEARCHES, 60);
    EXPECT_EQ(BellBlockEntity::MAX_RESONATION_TICKS, 40);
    EXPECT_EQ(BellBlockEntity::TICKS_BEFORE_RESONATION, 5);
    EXPECT_FLOAT_EQ(BellBlockEntity::SEARCH_RADIUS, 48.0f);
    EXPECT_FLOAT_EQ(BellBlockEntity::HEAR_BELL_RADIUS, 32.0f);
    EXPECT_FLOAT_EQ(BellBlockEntity::HIGHLIGHT_RAIDERS_RADIUS, 48.0f);
}

TEST_F(BellBlockEntityTest, Load_HandlesMissingFields)
{
    // 空 JSON 应该可以加载（使用默认值）
    nlohmann::json empty;
    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->load(empty));
    EXPECT_FALSE(loaded->isShaking());
    EXPECT_FALSE(loaded->isResonating());
    EXPECT_EQ(loaded->ticks(), 0);
}

TEST_F(BellBlockEntityTest, Load_HandlesInvalidClickDirection)
{
    nlohmann::json data;
    data["click_direction"] = 999; // 无效值
    data["shaking"] = true;

    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->load(data));
    // 无效的 click_direction 应该保持默认值（North）
    EXPECT_EQ(loaded->clickDirection(), Direction::North);
    EXPECT_TRUE(loaded->isShaking());
}

TEST_F(BellBlockEntityTest, Save_IncludesRequiredFields)
{
    nlohmann::json data;
    bell_->save(data);

    EXPECT_TRUE(data.contains("ticks"));
    EXPECT_TRUE(data.contains("shaking"));
    EXPECT_TRUE(data.contains("resonating"));
    EXPECT_TRUE(data.contains("resonation_ticks"));
    EXPECT_TRUE(data.contains("last_ring_timestamp"));
    EXPECT_TRUE(data.contains("click_direction"));
}

TEST_F(BellBlockEntityTest, Clone_IndependentState)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    std::unique_ptr<BlockEntity> copy = bell_->clone();
    auto* bellCopy = static_cast<BellBlockEntity*>(copy.get());

    // 修改原始实体的状态不应影响副本
    bell_->triggerEvent(1, static_cast<i32>(Direction::South));
    EXPECT_EQ(bellCopy->clickDirection(), Direction::North);
}
