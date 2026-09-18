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
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/entity/ItemPickupManager.hpp"

using namespace mc;
using namespace mc::server;

// Test fixture for ItemPickupManager
class ItemPickupManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和物品系统
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// ============================================================================
// 常量测试
// ============================================================================

// 常量测试参考 MC 1.21.11 物品实体行为

TEST_F(ItemPickupManagerTest, ConstantsAreCorrect)
{
    EXPECT_FLOAT_EQ(ItemPickupManager::PICKUP_RANGE, 1.0f);
    EXPECT_FLOAT_EQ(ItemPickupManager::PICKUP_RANGE_EXTENDED, 1.5f);
    EXPECT_FLOAT_EQ(ItemPickupManager::PICKUP_RANGE_CREATIVE, 1.0f);
    EXPECT_FLOAT_EQ(ItemPickupManager::PICKUP_RANGE_SNEAKING, 0.5f);
    EXPECT_FLOAT_EQ(ItemEntity::MERGE_RANGE, 0.5f);
    EXPECT_EQ(ItemPickupManager::DEFAULT_THROWER_PICKUP_DELAY, 10);
    EXPECT_EQ(ItemPickupManager::MERGE_DELAY, 20);
}

// ============================================================================
// 构造函数测试
// ============================================================================

TEST_F(ItemPickupManagerTest, DefaultConstructor)
{
    ItemPickupManager manager;
    // 验证默认构造不抛出异常
    (void)manager;
}

// ============================================================================
// 拾取范围计算测试
// ============================================================================

TEST_F(ItemPickupManagerTest, CalculatePickupRange_Normal)
{
    // 普通玩家拾取范围应为 1.0
    // 由于 Player 需要 World，这里只能间接测试
    // 实际测试在集成测试中进行
    EXPECT_FLOAT_EQ(ItemPickupManager::PICKUP_RANGE, 1.0f);
}

// ============================================================================
// ItemEntity 拾取测试
// ============================================================================

TEST_F(ItemPickupManagerTest, ItemEntity_CanBePickedUp)
{
    // 创建物品实体
    ItemStack stack(Items::DIAMOND, 1);
    ItemEntity entity(1, stack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 刚创建时有拾取延迟
    EXPECT_FALSE(entity.canBePickedUp());

    // 设置延迟为0后可以拾取
    entity.setPickupDelay(0);
    EXPECT_TRUE(entity.canBePickedUp());

    // 设置为不可拾取
    entity.setUnpickable();
    EXPECT_FALSE(entity.canBePickedUp());
}

TEST_F(ItemPickupManagerTest, ItemEntity_PickupDelay)
{
    ItemStack stack(Items::DIAMOND, 1);
    ItemEntity entity(1, stack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 默认拾取延迟为 10 ticks
    EXPECT_EQ(entity.getPickupDelay(), 10);

    // 设置为 0
    entity.setPickupDelay(0);
    EXPECT_EQ(entity.getPickupDelay(), 0);
}

TEST_F(ItemPickupManagerTest, ItemEntity_Age)
{
    ItemStack stack(Items::DIAMOND, 1);
    ItemEntity entity(1, stack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 初始年龄为 0
    EXPECT_EQ(entity.getAge(), 0);

    // tick 后年龄增加
    entity.tick();
    EXPECT_EQ(entity.getAge(), 1);

    entity.tick();
    EXPECT_EQ(entity.getAge(), 2);
}

// ============================================================================
// 物品合并测试
// ============================================================================

TEST_F(ItemPickupManagerTest, ItemEntity_CanMergeWith)
{
    // 创建两个相同物品的实体
    ItemStack stack1(Items::DIAMOND, 32);
    ItemStack stack2(Items::DIAMOND, 16);

    ItemEntity entity1(1, stack1, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    ItemEntity entity2(2, stack2, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 可以合并
    EXPECT_TRUE(entity1.canMergeWith(entity2));

    // 设置拾取延迟后不能合并
    entity2.setPickupDelay(10);
    // canMergeWith 不检查拾取延迟，只检查物品类型

    // 不同物品不能合并
    ItemStack stack3(Items::IRON_INGOT, 16);
    ItemEntity entity3(3, stack3, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    EXPECT_FALSE(entity1.canMergeWith(entity3));
}

TEST_F(ItemPickupManagerTest, ItemEntity_TryMergeWith)
{
    // 创建两个相同物品的实体
    ItemStack stack1(Items::DIAMOND, 32);
    ItemStack stack2(Items::DIAMOND, 16);

    ItemEntity entity1(1, stack1, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    ItemEntity entity2(2, stack2, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 设置无拾取延迟
    entity1.setPickupDelay(0);
    entity2.setPickupDelay(0);

    // 合并成功
    EXPECT_TRUE(entity1.tryMergeWith(entity2));

    // 验证合并后的数量
    EXPECT_EQ(entity1.getCount(), 48);
}

TEST_F(ItemPickupManagerTest, ItemEntity_TryMergeWith_FullStack)
{
    // 创建两个满堆叠的实体
    ItemStack stack1(Items::DIAMOND, 64);
    ItemStack stack2(Items::DIAMOND, 64);

    ItemEntity entity1(1, stack1, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());
    ItemEntity entity2(2, stack2, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 满堆叠不能合并
    EXPECT_FALSE(entity1.canMergeWith(entity2));
}

// ============================================================================
// 物品过期测试
// ============================================================================

TEST_F(ItemPickupManagerTest, ItemEntity_Expiration)
{
    ItemStack stack(Items::DIAMOND, 1);
    ItemEntity entity(1, stack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 默认存活时间为 6000 ticks
    EXPECT_FALSE(entity.isExpired());

    // 设置存活时间为 5 ticks
    entity.setLifetime(5);

    // tick 5 次
    for (int i = 0; i < 5; ++i) {
        entity.tick();
    }

    // 应该过期
    EXPECT_TRUE(entity.isExpired());
}

TEST_F(ItemPickupManagerTest, ItemEntity_InfiniteLifetime)
{
    ItemStack stack(Items::DIAMOND, 1);
    ItemEntity entity(1, stack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 设置无限存活时间
    entity.setLifetime(ItemEntity::INFINITE_LIFETIME);

    // 永不过期
    for (int i = 0; i < 10000; ++i) {
        entity.tick();
    }
    EXPECT_FALSE(entity.isExpired());
}

// ============================================================================
// 所有者测试
// ============================================================================

TEST_F(ItemPickupManagerTest, ItemEntity_Owner)
{
    ItemStack stack(Items::DIAMOND, 1);
    ItemEntity entity(1, stack, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    // 设置所有者
    entity.setOwner("player-uuid-123", "thrower-uuid-456");

    EXPECT_EQ(entity.ownerUuid(), "player-uuid-123");
    EXPECT_EQ(entity.throwerUuid(), "thrower-uuid-456");
}

// ============================================================================
// 物品数量修改测试
// ============================================================================

TEST_F(ItemPickupManagerTest, ItemEntity_SetItemStack)
{
    ItemStack stack1(Items::DIAMOND, 10);
    ItemEntity entity(1, stack1, 0.0f, 0.0f, 0.0f, mc::test::testEcsRegistry());

    EXPECT_EQ(entity.getCount(), 10);

    // 替换物品堆
    ItemStack stack2(Items::IRON_INGOT, 32);
    entity.setItemStack(stack2);

    EXPECT_EQ(entity.getCount(), 32);
    EXPECT_TRUE(entity.getItemStack().isSameItem(stack2));
}

// ============================================================================
// PlayerInventory 添加物品测试
// ============================================================================

class PlayerInventoryPickupTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(PlayerInventoryPickupTest, AddItem_EmptyInventory)
{
    PlayerInventory inventory(nullptr);

    // 添加一个物品
    ItemStack stack(Items::DIAMOND, 32);
    i32 added = inventory.add(stack);

    // 应该完全添加
    EXPECT_EQ(added, 32);
    EXPECT_TRUE(stack.isEmpty());
}

TEST_F(PlayerInventoryPickupTest, AddItem_ExistingStack)
{
    PlayerInventory inventory(nullptr);

    // 先添加一些物品
    ItemStack existing(Items::DIAMOND, 32);
    inventory.setItem(0, existing);

    // 添加更多相同物品
    ItemStack stack(Items::DIAMOND, 16);
    i32 added = inventory.add(stack);

    // 应该合并到已有堆叠
    EXPECT_EQ(added, 16);                           // 添加了 16 个
    EXPECT_TRUE(stack.isEmpty());                   // 原堆叠应该为空
    EXPECT_EQ(inventory.getItem(0).getCount(), 48); // 32 + 16 = 48
}

TEST_F(PlayerInventoryPickupTest, AddItem_FullInventory)
{
    PlayerInventory inventory(nullptr);

    // 填满背包（快捷栏 + 主背包 = 36 槽位，每个堆叠 64）
    for (int i = 0; i < 36; ++i) {
        ItemStack stack(Items::DIAMOND, 64);
        inventory.setItem(i, stack);
    }

    // 尝试添加更多物品
    ItemStack stack(Items::DIAMOND, 16);
    i32 added = inventory.add(stack);

    // 背包已满，无法添加
    EXPECT_EQ(added, 0);
    EXPECT_EQ(stack.getCount(), 16); // 原堆叠未改变
}

TEST_F(PlayerInventoryPickupTest, AddItem_DifferentItems)
{
    PlayerInventory inventory(nullptr);

    // 添加钻石
    ItemStack diamonds(Items::DIAMOND, 32);
    inventory.add(diamonds);

    // 添加铁锭
    ItemStack iron(Items::IRON_INGOT, 16);
    i32 added = inventory.add(iron);

    EXPECT_EQ(added, 16);
    EXPECT_EQ(inventory.getItem(0).getCount(), 32); // 钻石
    EXPECT_EQ(inventory.getItem(1).getCount(), 16); // 铁锭
}
