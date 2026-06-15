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

#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/MusicDiscItem.hpp"

using namespace mc;
using namespace mc::item::items;

namespace {

class MusicDiscItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }
};

// ========== 基本属性测试 ==========

TEST_F(MusicDiscItemTest, AllDiscsAreRegistered)
{
    // 验证所有21张唱片都已注册
    ASSERT_NE(Items::MUSIC_DISC_13, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_CAT, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_BLOCKS, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_CHIRP, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_FAR, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_MALL, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_MELLOHI, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_STAL, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_STRAD, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_WARD, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_11, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_WAIT, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_PIGSTEP, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_OTHERSIDE, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_5, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_RELIC, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_TEARS, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_CREATOR, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_CREATOR_MUSIC_BOX, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_PRECIPICE, nullptr);
    ASSERT_NE(Items::MUSIC_DISC_LAVA_CHICKEN, nullptr);
}

TEST_F(MusicDiscItemTest, IsMusicDiscReturnsTrue)
{
    // 所有 MusicDiscItem 实例 isMusicDisc() 应返回 true
    EXPECT_TRUE(Items::MUSIC_DISC_13->isMusicDisc());
    EXPECT_TRUE(Items::MUSIC_DISC_CAT->isMusicDisc());
    EXPECT_TRUE(Items::MUSIC_DISC_PIGSTEP->isMusicDisc());
    EXPECT_TRUE(Items::MUSIC_DISC_5->isMusicDisc());
}

TEST_F(MusicDiscItemTest, NonDiscItemIsNotMusicDisc)
{
    // 非唱片物品 isMusicDisc() 应返回 false
    EXPECT_FALSE(Items::DIAMOND->isMusicDisc());
    EXPECT_FALSE(Items::STICK->isMusicDisc());
}

// ========== 比较器信号强度测试 ==========

TEST_F(MusicDiscItemTest, ComparatorOutput_ClassicDiscs)
{
    // 13张经典唱片信号强度 1-12
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_13)->getComparatorOutput(), 1);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_CAT)->getComparatorOutput(), 2);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_BLOCKS)->getComparatorOutput(), 3);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_CHIRP)->getComparatorOutput(), 4);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_FAR)->getComparatorOutput(), 5);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_MALL)->getComparatorOutput(), 6);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_MELLOHI)->getComparatorOutput(), 7);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_STAL)->getComparatorOutput(), 8);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_STRAD)->getComparatorOutput(), 9);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_WARD)->getComparatorOutput(), 10);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_11)->getComparatorOutput(), 11);
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_WAIT)->getComparatorOutput(), 12);
}

TEST_F(MusicDiscItemTest, ComparatorOutput_NewDiscs)
{
    // Pigstep = 13
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_PIGSTEP)->getComparatorOutput(), 13);
    // otherside = 14
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_OTHERSIDE)->getComparatorOutput(), 14);
    // 5 = 15
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_5)->getComparatorOutput(), 15);
    // Relic = 14
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_RELIC)->getComparatorOutput(), 14);
    // Tears = 10
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_TEARS)->getComparatorOutput(), 10);
    // Creator = 12
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_CREATOR)->getComparatorOutput(), 12);
    // Creator (Music Box) = 11
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_CREATOR_MUSIC_BOX)->getComparatorOutput(), 11);
    // Precipice = 13
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_PRECIPICE)->getComparatorOutput(), 13);
    // Lava Chicken = 9
    EXPECT_EQ(static_cast<MusicDiscItem*>(Items::MUSIC_DISC_LAVA_CHICKEN)->getComparatorOutput(), 9);
}

TEST_F(MusicDiscItemTest, ComparatorOutputRange)
{
    // 所有唱片的比较器信号强度都应在 [1, 15] 范围内
    Item* allDiscs[] = {
        Items::MUSIC_DISC_13,
        Items::MUSIC_DISC_CAT,
        Items::MUSIC_DISC_BLOCKS,
        Items::MUSIC_DISC_CHIRP,
        Items::MUSIC_DISC_FAR,
        Items::MUSIC_DISC_MALL,
        Items::MUSIC_DISC_MELLOHI,
        Items::MUSIC_DISC_STAL,
        Items::MUSIC_DISC_STRAD,
        Items::MUSIC_DISC_WARD,
        Items::MUSIC_DISC_11,
        Items::MUSIC_DISC_WAIT,
        Items::MUSIC_DISC_PIGSTEP,
        Items::MUSIC_DISC_OTHERSIDE,
        Items::MUSIC_DISC_5,
        Items::MUSIC_DISC_RELIC,
        Items::MUSIC_DISC_TEARS,
        Items::MUSIC_DISC_CREATOR,
        Items::MUSIC_DISC_CREATOR_MUSIC_BOX,
        Items::MUSIC_DISC_PRECIPICE,
        Items::MUSIC_DISC_LAVA_CHICKEN,
    };

    for (auto* disc : allDiscs) {
        ASSERT_NE(disc, nullptr);
        auto* musicDisc = static_cast<MusicDiscItem*>(disc);
        i32 signal = musicDisc->getComparatorOutput();
        EXPECT_GE(signal, 1) << "Disc " << disc->itemLocation().toString() << " signal < 1";
        EXPECT_LE(signal, 15) << "Disc " << disc->itemLocation().toString() << " signal > 15";
    }
}

// ========== 声音事件ID测试 ==========

TEST_F(MusicDiscItemTest, SoundEventIdIsCorrect)
{
    // 验证唱片的声音事件ID不为空
    auto* disc13 = static_cast<MusicDiscItem*>(Items::MUSIC_DISC_13);
    EXPECT_FALSE(disc13->getSoundEventId().toString().empty());

    auto* discCat = static_cast<MusicDiscItem*>(Items::MUSIC_DISC_CAT);
    EXPECT_FALSE(discCat->getSoundEventId().toString().empty());

    // 不同唱片的声音事件ID应该不同
    EXPECT_NE(disc13->getSoundEventId().toString(), discCat->getSoundEventId().toString());
}

// ========== dynamic_cast 测试 ==========

TEST_F(MusicDiscItemTest, DynamicCastFromItemSucceeds)
{
    // 从 Item* dynamic_cast 到 MusicDiscItem* 应该成功
    Item* item = Items::MUSIC_DISC_13;
    auto* discItem = dynamic_cast<MusicDiscItem*>(item);
    ASSERT_NE(discItem, nullptr);
    EXPECT_EQ(discItem->getComparatorOutput(), 1);
}

TEST_F(MusicDiscItemTest, DynamicCastFromNonDiscFails)
{
    // 从非唱片 Item* dynamic_cast 到 MusicDiscItem* 应该失败
    Item* item = Items::DIAMOND;
    auto* discItem = dynamic_cast<MusicDiscItem*>(item);
    EXPECT_EQ(discItem, nullptr);
}

// ========== ItemStack 交互测试 ==========

TEST_F(MusicDiscItemTest, ItemStackHoldsDisc)
{
    // ItemStack 可以持有唱片
    ItemStack stack(Items::MUSIC_DISC_13, 1);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), Items::MUSIC_DISC_13);
    EXPECT_TRUE(stack.getItem()->isMusicDisc());
}

TEST_F(MusicDiscItemTest, DiscMaxStackSizeIs1)
{
    // 唱片最大堆叠数应为1
    EXPECT_EQ(Items::MUSIC_DISC_13->maxStackSize(), 1);
    EXPECT_EQ(Items::MUSIC_DISC_PIGSTEP->maxStackSize(), 1);
}

// ========== ItemRegistry 查找测试 ==========

TEST_F(MusicDiscItemTest, DiscFoundByResourceLocation)
{
    // 通过 ResourceLocation 可以找到唱片物品
    auto& registry = ItemRegistry::instance();
    Item* disc13 = registry.getItem(ResourceLocation("minecraft", "music_disc_13"));
    ASSERT_NE(disc13, nullptr);
    EXPECT_EQ(disc13, Items::MUSIC_DISC_13);
    EXPECT_TRUE(disc13->isMusicDisc());
}

} // namespace
