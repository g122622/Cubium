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

#include "entity/entities/item/ItemEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;

// ============================================================================
// ItemEntity 测试
// ============================================================================

class ItemEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    Item* m_diamond = nullptr;
};

TEST_F(ItemEntityTest, Creation)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 10);
    ItemEntity entity(1, stack, 100.0, 64.0, 200.0, mc::test::testEcsRegistry());

    EXPECT_EQ(entity.id(), 1);
    EXPECT_EQ(entity.getItemStack().getCount(), 10);
    EXPECT_EQ(entity.getItemStack().getItem(), m_diamond);
}

TEST_F(ItemEntityTest, Position)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 100.5, 64.0, 200.5, mc::test::testEcsRegistry());

    EXPECT_DOUBLE_EQ(entity.x(), 100.5);
    EXPECT_DOUBLE_EQ(entity.y(), 64.0);
    EXPECT_DOUBLE_EQ(entity.z(), 200.5);
}

TEST_F(ItemEntityTest, CreationWithVelocity)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0, mc::test::testEcsRegistry());

    EXPECT_DOUBLE_EQ(entity.velocityX(), 1.0);
    EXPECT_DOUBLE_EQ(entity.velocityY(), 2.0);
    EXPECT_DOUBLE_EQ(entity.velocityZ(), 3.0);
}

TEST_F(ItemEntityTest, Dimensions)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());

    EXPECT_FLOAT_EQ(entity.width(), 0.25f);
    EXPECT_FLOAT_EQ(entity.height(), 0.25f);
    EXPECT_FLOAT_EQ(entity.eyeHeight(), 0.125f);
}

TEST_F(ItemEntityTest, PickupDelay)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());

    // 默认拾取延迟
    EXPECT_GT(entity.getPickupDelay(), 0);
    EXPECT_FALSE(entity.canBePickedUp());

    // 设置延迟为0
    entity.setPickupDelay(0);
    EXPECT_EQ(entity.getPickupDelay(), 0);
    EXPECT_TRUE(entity.canBePickedUp());
}

TEST_F(ItemEntityTest, Lifetime)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());

    // 默认存活时间
    EXPECT_EQ(entity.getAge(), 0);
    EXPECT_FALSE(entity.isExpired());

    // 设置存活时间
    entity.setLifetime(100);
    EXPECT_FALSE(entity.isExpired());

    // 模拟tick
    for (int i = 0; i < 100; ++i) {
        entity.tick();
    }
    EXPECT_TRUE(entity.isExpired());
}

TEST_F(ItemEntityTest, Unpickable)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());

    entity.setPickupDelay(0);
    EXPECT_TRUE(entity.canBePickedUp());

    entity.setUnpickable();
    EXPECT_FALSE(entity.canBePickedUp());
}

TEST_F(ItemEntityTest, SetItemStack)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 5);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());

    EXPECT_EQ(entity.getCount(), 5);

    ItemStack newStack(*m_diamond, 32);
    entity.setItemStack(newStack);
    EXPECT_EQ(entity.getCount(), 32);
}

TEST_F(ItemEntityTest, MergeWith)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack1(*m_diamond, 10);
    ItemStack stack2(*m_diamond, 20);

    ItemEntity entity1(1, stack1, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());
    ItemEntity entity2(2, stack2, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());

    EXPECT_TRUE(entity1.canMergeWith(entity2));

    // 合并
    bool merged = entity1.tryMergeWith(entity2);
    EXPECT_TRUE(merged);
    EXPECT_EQ(entity1.getCount(), 30);
    EXPECT_TRUE(entity2.isRemoved());
}

TEST_F(ItemEntityTest, CannotMergeDifferentItems)
{
    ASSERT_NE(m_diamond, nullptr);
    Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    ASSERT_NE(stick, nullptr);

    ItemStack stack1(*m_diamond, 10);
    ItemStack stack2(*stick, 10);

    ItemEntity entity1(1, stack1, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());
    ItemEntity entity2(2, stack2, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());

    EXPECT_FALSE(entity1.canMergeWith(entity2));
}

TEST_F(ItemEntityTest, Owner)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 1);
    ItemEntity entity(1, stack, 0.0, 0.0, 0.0, mc::test::testEcsRegistry());

    entity.setOwner("player-uuid-123", "thrower-uuid-456");
    EXPECT_EQ(entity.ownerUuid(), "player-uuid-123");
    EXPECT_EQ(entity.throwerUuid(), "thrower-uuid-456");
}
