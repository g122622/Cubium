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

#include "common/entity/player/CooldownTracker.hpp"
#include "common/item/core/Item.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::player;

/**
 * @brief CooldownTracker 单元测试
 *
 * 测试物品冷却追踪器的核心功能：
 * - 设置冷却
 * - 检查冷却状态
 * - 冷却过期
 * - 冷却进度计算
 */

// 测试用物品类
class TestCooldownItem final : public Item {
public:
    explicit TestCooldownItem()
        : Item(ItemProperties().maxStackSize(64))
    {}
};

class CooldownTrackerTest : public ::testing::Test {
protected:
    void SetUp() override { tracker = std::make_unique<CooldownTracker>(); }

    void TearDown() override
    {
        tracker.reset();
        testItems.clear();
    }

    std::unique_ptr<CooldownTracker> tracker;
    std::vector<std::unique_ptr<TestCooldownItem>> testItems;

    // 创建一个简单的测试物品
    const Item* createTestItem()
    {
        auto item = std::make_unique<TestCooldownItem>();
        const Item* ptr = item.get();
        testItems.push_back(std::move(item));
        return ptr;
    }
};

// ============================================================================
// 基本功能测试
// ============================================================================

TEST_F(CooldownTrackerTest, SetCooldown_SetsCorrectly)
{
    auto item = createTestItem();

    EXPECT_FALSE(tracker->hasCooldown(item));

    tracker->setCooldown(item, 20);
    EXPECT_TRUE(tracker->hasCooldown(item));
    EXPECT_EQ(tracker->getCooldownTicks(item), 20);
}

TEST_F(CooldownTrackerTest, SetCooldown_ZeroTicks_DoesNotSet)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 0);
    EXPECT_FALSE(tracker->hasCooldown(item));
}

TEST_F(CooldownTrackerTest, SetCooldown_NegativeTicks_DoesNotSet)
{
    auto item = createTestItem();

    tracker->setCooldown(item, -5);
    EXPECT_FALSE(tracker->hasCooldown(item));
}

TEST_F(CooldownTrackerTest, SetCooldown_NullItem_DoesNotSet)
{
    tracker->setCooldown(nullptr, 20);
    EXPECT_FALSE(tracker->hasCooldown(nullptr));
}

TEST_F(CooldownTrackerTest, RemoveCooldown_RemovesCorrectly)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 20);
    EXPECT_TRUE(tracker->hasCooldown(item));

    tracker->removeCooldown(item);
    EXPECT_FALSE(tracker->hasCooldown(item));
    EXPECT_EQ(tracker->getCooldownTicks(item), 0);
}

// ============================================================================
// Tick 更新测试
// ============================================================================

TEST_F(CooldownTrackerTest, Tick_DecrementsCooldown)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 20);
    EXPECT_EQ(tracker->getCooldownTicks(item), 20);

    tracker->tick();
    EXPECT_EQ(tracker->getCooldownTicks(item), 19);

    tracker->tick();
    EXPECT_EQ(tracker->getCooldownTicks(item), 18);
}

TEST_F(CooldownTrackerTest, Tick_CooldownExpires)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 3);
    EXPECT_TRUE(tracker->hasCooldown(item));

    tracker->tick(); // 2 ticks remaining
    EXPECT_TRUE(tracker->hasCooldown(item));

    tracker->tick(); // 1 tick remaining
    EXPECT_TRUE(tracker->hasCooldown(item));

    tracker->tick(); // expired
    EXPECT_FALSE(tracker->hasCooldown(item));
    EXPECT_EQ(tracker->getCooldownTicks(item), 0);
}

TEST_F(CooldownTrackerTest, Tick_MultipleItems_Independent)
{
    auto item1 = createTestItem();
    auto item2 = createTestItem();

    tracker->setCooldown(item1, 5);
    tracker->setCooldown(item2, 10);

    EXPECT_TRUE(tracker->hasCooldown(item1));
    EXPECT_TRUE(tracker->hasCooldown(item2));

    // Tick 5 times
    for (int i = 0; i < 5; ++i) {
        tracker->tick();
    }

    EXPECT_FALSE(tracker->hasCooldown(item1));
    EXPECT_TRUE(tracker->hasCooldown(item2));
    EXPECT_EQ(tracker->getCooldownTicks(item2), 5);

    // Tick 5 more times
    for (int i = 0; i < 5; ++i) {
        tracker->tick();
    }

    EXPECT_FALSE(tracker->hasCooldown(item1));
    EXPECT_FALSE(tracker->hasCooldown(item2));
}

TEST_F(CooldownTrackerTest, Tick_EmptyTracker_NoCrash)
{
    // 空 tracker 不应该崩溃
    for (int i = 0; i < 100; ++i) {
        tracker->tick();
    }
    EXPECT_TRUE(tracker->isEmpty());
}

// ============================================================================
// 冷却进度测试
// ============================================================================

TEST_F(CooldownTrackerTest, GetCooldownProgress_NoCooldown_ReturnsZero)
{
    auto item = createTestItem();
    EXPECT_FLOAT_EQ(tracker->getCooldownProgress(item), 0.0f);
    EXPECT_FLOAT_EQ(tracker->getCooldownProgress(item, 0.5f), 0.0f);
}

TEST_F(CooldownTrackerTest, GetCooldownProgress_JustSet_ReturnsNearOne)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 20);
    f32 progress = tracker->getCooldownProgress(item);

    // 刚设置时，进度应该接近 1.0（刚开始冷却）
    EXPECT_GT(progress, 0.9f);
    EXPECT_LE(progress, 1.0f);
}

TEST_F(CooldownTrackerTest, GetCooldownProgress_Halfway_ReturnsHalf)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 20);

    // Tick 10 times (一半时间)
    for (int i = 0; i < 10; ++i) {
        tracker->tick();
    }

    f32 progress = tracker->getCooldownProgress(item);
    // 进度应该在 0.45-0.55 范围内（接近 0.5）
    EXPECT_GT(progress, 0.45f);
    EXPECT_LT(progress, 0.55f);
}

TEST_F(CooldownTrackerTest, GetCooldownProgress_AfterExpire_ReturnsZero)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 5);

    // Tick 直到过期
    for (int i = 0; i < 10; ++i) {
        tracker->tick();
    }

    EXPECT_FLOAT_EQ(tracker->getCooldownProgress(item), 0.0f);
}

TEST_F(CooldownTrackerTest, GetCooldownProgress_PartialTicks)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 20);
    tracker->tick();

    // 不带 partialTicks
    f32 progress1 = tracker->getCooldownProgress(item);
    // 带 partialTicks
    f32 progress2 = tracker->getCooldownProgress(item, 0.5f);

    // 带插值的进度应该略小（因为部分帧时间使冷却看起来更接近完成）
    EXPECT_LE(progress2, progress1);
}

// ============================================================================
// 覆盖设置测试
// ============================================================================

TEST_F(CooldownTrackerTest, SetCooldown_Overwrite_UpdatesCooldown)
{
    auto item = createTestItem();

    tracker->setCooldown(item, 20);
    EXPECT_EQ(tracker->getCooldownTicks(item), 20);

    // 覆盖为更长的冷却
    tracker->setCooldown(item, 40);
    EXPECT_EQ(tracker->getCooldownTicks(item), 40);

    // 覆盖为更短的冷却
    tracker->setCooldown(item, 10);
    EXPECT_EQ(tracker->getCooldownTicks(item), 10);
}

// ============================================================================
// 辅助方法测试
// ============================================================================

TEST_F(CooldownTrackerTest, IsEmpty_ReturnsCorrectStatus)
{
    auto item = createTestItem();

    EXPECT_TRUE(tracker->isEmpty());

    tracker->setCooldown(item, 20);
    EXPECT_FALSE(tracker->isEmpty());

    // Tick 直到过期
    for (int i = 0; i < 25; ++i) {
        tracker->tick();
    }

    EXPECT_TRUE(tracker->isEmpty());
}

TEST_F(CooldownTrackerTest, CooldownCount_ReturnsCorrectCount)
{
    auto item1 = createTestItem();
    auto item2 = createTestItem();
    auto item3 = createTestItem();

    EXPECT_EQ(tracker->cooldownCount(), 0u);

    tracker->setCooldown(item1, 20);
    EXPECT_EQ(tracker->cooldownCount(), 1u);

    tracker->setCooldown(item2, 20);
    EXPECT_EQ(tracker->cooldownCount(), 2u);

    tracker->setCooldown(item3, 20);
    EXPECT_EQ(tracker->cooldownCount(), 3u);

    // 覆盖不算新的
    tracker->setCooldown(item1, 30);
    EXPECT_EQ(tracker->cooldownCount(), 3u);

    // 移除
    tracker->removeCooldown(item2);
    EXPECT_EQ(tracker->cooldownCount(), 2u);
}

TEST_F(CooldownTrackerTest, CurrentTick_Increments)
{
    EXPECT_EQ(tracker->currentTick(), 0);

    tracker->tick();
    EXPECT_EQ(tracker->currentTick(), 1);

    tracker->tick();
    EXPECT_EQ(tracker->currentTick(), 2);

    for (int i = 0; i < 100; ++i) {
        tracker->tick();
    }
    EXPECT_EQ(tracker->currentTick(), 102);
}

// ============================================================================
// 典型使用场景测试
// ============================================================================

TEST_F(CooldownTrackerTest, Scenario_ChorusFruitCooldown)
{
    // 模拟紫颂果冷却：20 ticks
    auto chorusFruit = createTestItem();

    // 使用紫颂果
    tracker->setCooldown(chorusFruit, 20);

    // 第一 tick，冷却进度应该接近 1
    f32 progress = tracker->getCooldownProgress(chorusFruit);
    EXPECT_GT(progress, 0.9f);

    // 模拟 20 ticks
    for (int i = 0; i < 20; ++i) {
        tracker->tick();
    }

    // 冷却结束
    EXPECT_FALSE(tracker->hasCooldown(chorusFruit));
    EXPECT_FLOAT_EQ(tracker->getCooldownProgress(chorusFruit), 0.0f);
}

TEST_F(CooldownTrackerTest, Scenario_ShieldCooldown)
{
    // 模拟盾牌冷却：100 ticks (5秒)
    auto shield = createTestItem();

    // 盾牌被斧击中
    tracker->setCooldown(shield, 100);

    // 检查是否在冷却中
    EXPECT_TRUE(tracker->hasCooldown(shield));

    // 模拟 50 ticks (2.5秒)
    for (int i = 0; i < 50; ++i) {
        tracker->tick();
    }

    // 进度应该接近 0.5
    f32 progress = tracker->getCooldownProgress(shield);
    EXPECT_GT(progress, 0.45f);
    EXPECT_LT(progress, 0.55f);

    // 模拟剩余 50 ticks
    for (int i = 0; i < 50; ++i) {
        tracker->tick();
    }

    // 冷却结束
    EXPECT_FALSE(tracker->hasCooldown(shield));
}
