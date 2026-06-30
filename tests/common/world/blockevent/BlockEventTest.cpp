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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/network/packet/BlockEventPacket.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include "common/world/blockentity/interactive/EndGatewayEntity.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/blockentity/storage/EnderChestEntity.hpp"
#include "common/world/blockentity/storage/ShulkerBoxEntity.hpp"
#include "common/world/blockevent/BlockEventData.hpp"
#include <unordered_set>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

// ============================================================================
// BlockEventData 测试
// ============================================================================

namespace {
/// 获取测试用方块指针（从注册表中获取已注册的方块）
const Block* getTestBlock()
{
    // 从注册表获取已注册的方块
    const Block* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    if (block != nullptr) {
        return block;
    }
    // 如果 stone 不可用，尝试其他方块
    return BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
}
} // namespace

TEST(BlockEventDataTest, Equality_SameFields_AreEqual)
{
    const Block* block = getTestBlock();
    ASSERT_NE(block, nullptr);
    BlockEventData a{BlockPos(1, 2, 3), block, 1, 0};
    BlockEventData b{BlockPos(1, 2, 3), block, 1, 0};
    EXPECT_TRUE(a == b);
}

TEST(BlockEventDataTest, Equality_DifferentPos_NotEqual)
{
    const Block* block = getTestBlock();
    ASSERT_NE(block, nullptr);
    BlockEventData a{BlockPos(1, 2, 3), block, 1, 0};
    BlockEventData b{BlockPos(4, 5, 6), block, 1, 0};
    EXPECT_FALSE(a == b);
}

TEST(BlockEventDataTest, Equality_DifferentBlock_NotEqual)
{
    const Block* block1 = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    const Block* block2 = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    if (block1 == nullptr || block2 == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }
    BlockEventData a{BlockPos(1, 2, 3), block1, 1, 0};
    BlockEventData b{BlockPos(1, 2, 3), block2, 1, 0};
    EXPECT_FALSE(a == b);
}

TEST(BlockEventDataTest, Equality_DifferentParamA_NotEqual)
{
    const Block* block = getTestBlock();
    ASSERT_NE(block, nullptr);
    BlockEventData a{BlockPos(1, 2, 3), block, 1, 0};
    BlockEventData b{BlockPos(1, 2, 3), block, 2, 0};
    EXPECT_FALSE(a == b);
}

TEST(BlockEventDataTest, Equality_DifferentParamB_NotEqual)
{
    const Block* block = getTestBlock();
    ASSERT_NE(block, nullptr);
    BlockEventData a{BlockPos(1, 2, 3), block, 1, 0};
    BlockEventData b{BlockPos(1, 2, 3), block, 1, 5};
    EXPECT_FALSE(a == b);
}

TEST(BlockEventDataTest, Hash_SameFields_SameHash)
{
    const Block* block = getTestBlock();
    ASSERT_NE(block, nullptr);
    BlockEventData a{BlockPos(10, 20, 30), block, 1, 2};
    BlockEventData b{BlockPos(10, 20, 30), block, 1, 2};
    EXPECT_EQ(std::hash<BlockEventData>()(a), std::hash<BlockEventData>()(b));
}

TEST(BlockEventDataTest, Hash_CanBeUsedInUnorderedSet)
{
    const Block* block1 = getTestBlock();
    const Block* block2 = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    if (block1 == nullptr || block2 == nullptr) {
        GTEST_SKIP() << "Required blocks not registered";
    }
    BlockEventData a{BlockPos(1, 2, 3), block1, 1, 0};
    BlockEventData b{BlockPos(1, 2, 3), block1, 1, 0}; // 与 a 相同
    BlockEventData c{BlockPos(4, 5, 6), block2, 2, 1};

    std::unordered_set<BlockEventData> set;
    set.insert(a);
    set.insert(b); // 重复，不应增加大小
    set.insert(c);
    EXPECT_EQ(set.size(), 2u);
}

// ============================================================================
// BlockEventPacket 测试
// ============================================================================

TEST(BlockEventPacketTest, Create_SetsFields)
{
    auto packet = network::BlockEventPacket::create(BlockPos(10, 20, 30), 1, 5, 42);
    EXPECT_EQ(packet.position().x, 10);
    EXPECT_EQ(packet.position().y, 20);
    EXPECT_EQ(packet.position().z, 30);
    EXPECT_EQ(packet.paramA(), 1);
    EXPECT_EQ(packet.paramB(), 5);
    EXPECT_EQ(packet.blockStateId(), 42u);
}

TEST(BlockEventPacketTest, Create_ZeroParams)
{
    auto packet = network::BlockEventPacket::create(BlockPos(0, 0, 0), 0, 0, 0);
    EXPECT_EQ(packet.position().x, 0);
    EXPECT_EQ(packet.position().y, 0);
    EXPECT_EQ(packet.position().z, 0);
    EXPECT_EQ(packet.paramA(), 0);
    EXPECT_EQ(packet.paramB(), 0);
    EXPECT_EQ(packet.blockStateId(), 0u);
}

// ============================================================================
// BlockEntity::triggerEvent 测试
// ============================================================================

// --- ChestEntity triggerEvent ---

TEST(ChestEntityTriggerEventTest, Event1_NonZeroType_SetsLidOpen)
{
    ChestEntity chest(BlockPos(0, 0, 0));
    EXPECT_FLOAT_EQ(chest.getLidAngle(), 0.0f);

    // id=1, type=3 表示有3个打开者
    bool result = chest.triggerEvent(1, 3);
    EXPECT_TRUE(result);
    EXPECT_FLOAT_EQ(chest.getLidAngle(), 1.0f);
}

TEST(ChestEntityTriggerEventTest, Event1_ZeroType_SetsLidClosed)
{
    ChestEntity chest(BlockPos(0, 0, 0));
    chest.triggerEvent(1, 3); // 先打开
    EXPECT_FLOAT_EQ(chest.getLidAngle(), 1.0f);

    bool result = chest.triggerEvent(1, 0); // 关闭
    EXPECT_TRUE(result);
    EXPECT_FLOAT_EQ(chest.getLidAngle(), 0.0f);
}

TEST(ChestEntityTriggerEventTest, UnknownEvent_ReturnsFalse)
{
    ChestEntity chest(BlockPos(0, 0, 0));
    bool result = chest.triggerEvent(99, 0);
    EXPECT_FALSE(result);
}

// --- EnderChestEntity triggerEvent ---

TEST(EnderChestEntityTriggerEventTest, Event1_NonZeroType_SetsOpenCount)
{
    EnderChestEntity enderChest(BlockPos(0, 0, 0));
    EXPECT_EQ(enderChest.getOpenCount(), 0);

    bool result = enderChest.triggerEvent(1, 2);
    EXPECT_TRUE(result);
    EXPECT_EQ(enderChest.getOpenCount(), 2);
}

TEST(EnderChestEntityTriggerEventTest, Event1_ZeroType_SetsOpenCountToZero)
{
    EnderChestEntity enderChest(BlockPos(0, 0, 0));
    enderChest.triggerEvent(1, 1);
    EXPECT_EQ(enderChest.getOpenCount(), 1);

    bool result = enderChest.triggerEvent(1, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(enderChest.getOpenCount(), 0);
}

TEST(EnderChestEntityTriggerEventTest, UnknownEvent_ReturnsFalse)
{
    EnderChestEntity enderChest(BlockPos(0, 0, 0));
    bool result = enderChest.triggerEvent(5, 0);
    EXPECT_FALSE(result);
}

// --- ShulkerBoxEntity triggerEvent ---

TEST(ShulkerBoxEntityTriggerEventTest, Event1_TypeOne_SetsOpening)
{
    ShulkerBoxEntity shulker(BlockPos(0, 0, 0));

    bool result = shulker.triggerEvent(1, 1);
    EXPECT_TRUE(result);
    // type == 1 设置动画状态为 Opening
    EXPECT_EQ(shulker.getAnimationStatus(), ShulkerBoxEntity::AnimationStatus::Opening);
}

TEST(ShulkerBoxEntityTriggerEventTest, Event1_TypeZero_SetsClosing)
{
    ShulkerBoxEntity shulker(BlockPos(0, 0, 0));
    shulker.triggerEvent(1, 1); // 先打开

    bool result = shulker.triggerEvent(1, 0); // 关闭
    EXPECT_TRUE(result);
    EXPECT_EQ(shulker.getOpenCount(), 0);
}

TEST(ShulkerBoxEntityTriggerEventTest, UnknownEvent_ReturnsFalse)
{
    ShulkerBoxEntity shulker(BlockPos(0, 0, 0));
    bool result = shulker.triggerEvent(2, 0);
    EXPECT_FALSE(result);
}

// --- DecoratedPotBlockEntity triggerEvent ---

TEST(DecoratedPotBlockEntityTriggerEventTest, Event1_TypeZero_SetsPositiveWobble)
{
    DecoratedPotBlockEntity pot(BlockPos(0, 0, 0));
    bool result = pot.triggerEvent(1, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(pot.lastWobbleStyle(), DecoratedPotBlockEntity::WobbleStyle::Positive);
}

TEST(DecoratedPotBlockEntityTriggerEventTest, Event1_TypeOne_SetsNegativeWobble)
{
    DecoratedPotBlockEntity pot(BlockPos(0, 0, 0));
    bool result = pot.triggerEvent(1, 1);
    EXPECT_TRUE(result);
    EXPECT_EQ(pot.lastWobbleStyle(), DecoratedPotBlockEntity::WobbleStyle::Negative);
}

TEST(DecoratedPotBlockEntityTriggerEventTest, UnknownEvent_ReturnsFalse)
{
    DecoratedPotBlockEntity pot(BlockPos(0, 0, 0));
    bool result = pot.triggerEvent(99, 0);
    EXPECT_FALSE(result);
}

// --- EndGatewayEntity triggerEvent ---

TEST(EndGatewayEntityTriggerEventTest, Event1_SetsCooldown)
{
    EndGatewayEntity gateway(BlockPos(0, 0, 0));
    EXPECT_FALSE(gateway.isCoolingDown());
    EXPECT_EQ(gateway.getTeleportCooldown(), 0);

    bool result = gateway.triggerEvent(1, 0);
    EXPECT_TRUE(result);
    EXPECT_TRUE(gateway.isCoolingDown());
    EXPECT_EQ(gateway.getTeleportCooldown(), EndGatewayEntity::TRIGGER_COOLDOWN);
}

TEST(EndGatewayEntityTriggerEventTest, UnknownEvent_ReturnsFalse)
{
    EndGatewayEntity gateway(BlockPos(0, 0, 0));
    bool result = gateway.triggerEvent(99, 0);
    EXPECT_FALSE(result);
}

// --- MobSpawnerBlockEntity triggerEvent ---

TEST(MobSpawnerBlockEntityTriggerEventTest, Event1_ReturnsTrue)
{
    MobSpawnerBlockEntity spawner(BlockPos(0, 0, 0));
    // triggerEvent(id=1) 返回 true（即使 m_world 为 nullptr）
    bool result = spawner.triggerEvent(1, 0);
    EXPECT_TRUE(result);
}

TEST(MobSpawnerBlockEntityTriggerEventTest, UnknownEvent_ReturnsFalse)
{
    MobSpawnerBlockEntity spawner(BlockPos(0, 0, 0));
    bool result = spawner.triggerEvent(99, 0);
    EXPECT_FALSE(result);
}
