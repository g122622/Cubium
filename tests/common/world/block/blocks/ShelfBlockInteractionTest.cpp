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
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/ShelfBlockEntity.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "world/block/blocks/ShelfBlock.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace {

/**
 * @brief 书架交互测试用世界桩
 *
 * 继承自 BaseTestWorld，提供方块状态存储、方块实体存储、音效捕获能力。
 * 用于测试 ShelfBlock::onBlockActivated 的物品交换逻辑和 heldItemTransformedTo 语义。
 */
class ShelfTestWorld final : public test::BaseTestWorld {
public:
    ShelfTestWorld() = default;

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        return true;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    void playSound(const ResourceLocation& sound,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({sound, category, pos, volume, pitch});
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity != nullptr) {
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        (void)entity;
        return ++m_lastEntityId;
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ShelfTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ShelfTestWorld::tickManager not implemented");
    }

    struct SoundRecord {
        ResourceLocation sound;
        sound::SoundCategory category;
        Vector3 pos;
        f32 volume;
        f32 pitch;
    };

    [[nodiscard]] const std::vector<SoundRecord>& sounds() const { return m_sounds; }
    [[nodiscard]] bool wasSoundPlayed() const { return !m_sounds.empty(); }
    [[nodiscard]] const ResourceLocation& lastSoundId() const { return m_sounds.back().sound; }
    void clearSounds() { m_sounds.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<SoundRecord> m_sounds;
    EntityInstanceId m_lastEntityId = 0;
};

// ============================================================================
// 测试固件
// ============================================================================

class ShelfBlockInteractionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    void SetUp() override
    {
        m_pos = BlockPos(0, 64, 0);
        // 放置未充能的橡木书架，朝向北
        m_world.setBlockState(m_pos.x, m_pos.y, m_pos.z, &VanillaBlocks::OAK_SHELF->defaultState());
        // 创建并放置书架方块实体
        m_shelfEntity = std::make_unique<blockentity::ShelfBlockEntity>(m_pos);
        m_shelfEntityPtr = m_shelfEntity.get();
        m_world.setBlockEntity(m_pos, m_shelfEntity.release());
    }

    /// 将书架切换为充能模式（POWERED=true，侧链保持 Unconnected）
    void setPowered()
    {
        const BlockState* statePtr = m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
        ASSERT_NE(statePtr, nullptr);
        BlockState poweredState = statePtr->with(BlockStateProperties::POWERED(), true);
        m_world.setBlockState(m_pos.x, m_pos.y, m_pos.z, &poweredState);
    }

    /// 构造朝向方块的射线命中结果（正面命中，槽位0）
    [[nodiscard]] BlockRaycastResult makeHitSlot0() const
    {
        // 朝北的书架正面在 z=负方向，命中点设为方块正面中央偏右（玩家视角左侧）
        // 对 North 朝向：x = 1.0 - relX，槽位0要求 x < 16/3/16 ≈ 0.333
        // 因此 relX > 0.667，取 relX = 0.9 → x = 0.1 → column = 0
        return BlockRaycastResult::hit(Vector3(0.9f, 64.5f, 0.0f), m_pos, Direction::North, 0.0f);
    }

    ShelfTestWorld m_world;
    BlockPos m_pos;
    blockentity::ShelfBlockEntity* m_shelfEntityPtr = nullptr;

private:
    std::unique_ptr<blockentity::ShelfBlockEntity> m_shelfEntity;
};

// ============================================================================
// 未充能模式：单物品交换 + heldItemTransformedTo 测试
// ============================================================================

TEST_F(ShelfBlockInteractionTest, OnBlockActivated_Unpowered_EmptyHandAndEmptyShelf_ReturnsPass)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    // 玩家手持空
    player.inventory().getSelectedStackRef() = ItemStack();

    const BlockState& state = *m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    BlockRaycastResult hit = makeHitSlot0();

    auto result = VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::MainHand, hit);

    // 空手 + 空书架 → Pass（MC 1.21.11：wasSwapOrTake=false, heldItem.isEmpty() → PASS）
    EXPECT_EQ(result, ActionResultType::Pass);
    // 不携带 heldItemTransformedTo
    EXPECT_FALSE(result.heldItemTransformedTo().has_value());
}

TEST_F(ShelfBlockInteractionTest, OnBlockActivated_Unpowered_PlaceItem_ReturnsSuccessWithTransformedItem)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    // 玩家手持 5 个石头
    const Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    ItemStack heldStack(*stone, 5);
    player.inventory().getSelectedStackRef() = heldStack;

    const BlockState* statePtr = m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    ASSERT_NE(statePtr, nullptr);
    const BlockState& state = *statePtr;

    // 诊断断言：确认方块状态和方块实体正确
    ASSERT_TRUE(state.is(VanillaBlocks::OAK_SHELF));
    BlockEntity* be = m_world.getBlockEntity(m_pos);
    ASSERT_NE(be, nullptr);
    ASSERT_EQ(be->getType(), BlockEntityType::Shelf);

    BlockRaycastResult hit = makeHitSlot0();

    auto result = VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::MainHand, hit);

    // 放入物品 → Success（swapSingleItem 交换整组：书架得到 5 个石头，玩家手持变空）
    EXPECT_EQ(result, ActionResultType::Success);
    // 应携带 heldItemTransformedTo（MC 1.21.11：return SUCCESS.heldItemTransformedTo(p_433583_);）
    ASSERT_TRUE(result.heldItemTransformedTo().has_value());
    // 转换后的手持物品应为空（书架原为空，整组交换后玩家手持为空）
    EXPECT_TRUE(result.heldItemTransformedTo()->isEmpty());

    // 书架槽位 0 应有 5 个石头
    EXPECT_EQ(m_shelfEntityPtr->getInventory()->getItem(0).getCount(), 5);
    EXPECT_EQ(m_shelfEntityPtr->getInventory()->getItem(0).getItem(), stone);

    // 玩家手持物品应被清空（整组放入书架）
    EXPECT_TRUE(player.inventory().getSelectedStackRef().isEmpty());
}

TEST_F(ShelfBlockInteractionTest, OnBlockActivated_Unpowered_SwapItem_ReturnsSuccessWithTransformedItem)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    // 玩家手持 3 个石头
    const Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    player.inventory().getSelectedStackRef() = ItemStack(*stone, 3);

    // 书架槽位 0 放入一个苹果
    const Item* apple = ItemRegistry::instance().getItem(ResourceLocation("minecraft:apple"));
    ASSERT_NE(apple, nullptr);
    m_shelfEntityPtr->swapItemNoUpdate(0, ItemStack(*apple, 1));

    const BlockState& state = *m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    BlockRaycastResult hit = makeHitSlot0();

    auto result = VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::MainHand, hit);

    // 交换物品 → Success
    EXPECT_EQ(result, ActionResultType::Success);
    // 应携带 heldItemTransformedTo，转换后的手持物品是苹果（从书架取出）
    ASSERT_TRUE(result.heldItemTransformedTo().has_value());
    EXPECT_EQ(result.heldItemTransformedTo()->getItem(), apple);
    EXPECT_EQ(result.heldItemTransformedTo()->getCount(), 1);

    // 书架槽位 0 应有 3 个石头（从玩家手中交换过来）
    EXPECT_EQ(m_shelfEntityPtr->getInventory()->getItem(0).getItem(), stone);
    EXPECT_EQ(m_shelfEntityPtr->getInventory()->getItem(0).getCount(), 3);

    // 玩家手持物品应变为苹果
    EXPECT_EQ(player.inventory().getSelectedStackRef().getItem(), apple);
    EXPECT_EQ(player.inventory().getSelectedStackRef().getCount(), 1);
}

TEST_F(ShelfBlockInteractionTest, OnBlockActivated_Unpowered_TakeItem_ReturnsSuccessWithTransformedItem)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    // 玩家空手
    player.inventory().getSelectedStackRef() = ItemStack();

    // 书架槽位 0 放入 2 个苹果
    const Item* apple = ItemRegistry::instance().getItem(ResourceLocation("minecraft:apple"));
    ASSERT_NE(apple, nullptr);
    m_shelfEntityPtr->swapItemNoUpdate(0, ItemStack(*apple, 2));

    const BlockState& state = *m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    BlockRaycastResult hit = makeHitSlot0();

    auto result = VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::MainHand, hit);

    // 取出物品 → Success
    EXPECT_EQ(result, ActionResultType::Success);
    // 应携带 heldItemTransformedTo，转换后的手持物品是苹果（从书架取出）
    ASSERT_TRUE(result.heldItemTransformedTo().has_value());
    EXPECT_EQ(result.heldItemTransformedTo()->getItem(), apple);
    EXPECT_EQ(result.heldItemTransformedTo()->getCount(), 2);

    // 书架槽位 0 应为空
    EXPECT_TRUE(m_shelfEntityPtr->getInventory()->getItem(0).isEmpty());

    // 玩家手持物品应变为 2 个苹果
    EXPECT_EQ(player.inventory().getSelectedStackRef().getItem(), apple);
    EXPECT_EQ(player.inventory().getSelectedStackRef().getCount(), 2);
}

TEST_F(ShelfBlockInteractionTest, OnBlockActivated_OffHand_ReturnsPass)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    const Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    player.inventory().getSelectedStackRef() = ItemStack(*stone, 5);

    const BlockState& state = *m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    BlockRaycastResult hit = makeHitSlot0();

    auto result = VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::OffHand, hit);

    // 副手 → Pass
    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_FALSE(result.heldItemTransformedTo().has_value());
}

TEST_F(ShelfBlockInteractionTest, OnBlockActivated_Unpowered_PlaysPlaceItemSound)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    const Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    player.inventory().getSelectedStackRef() = ItemStack(*stone, 5);

    const BlockState& state = *m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    BlockRaycastResult hit = makeHitSlot0();

    VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::MainHand, hit);

    // 放入物品应播放 SHELF_PLACE_ITEM 音效
    EXPECT_TRUE(m_world.wasSoundPlayed());
}

// ============================================================================
// 充能模式：热栏整体交换（swapHotbar）+ heldItemTransformedTo 测试
//
// 单个充能书架（chainSize=1, containerSize=3）的热栏映射：
//   hotbarSlot = 9 - 1*3 + 0*3 + j = 6 + j
//   即：书架槽位0 ↔ 热栏6，书架槽位1 ↔ 热栏7，书架槽位2 ↔ 热栏8
// ============================================================================

TEST_F(ShelfBlockInteractionTest,
    OnBlockActivated_Powered_SwapHotbar_SelectedItemChanged_ReturnsSuccessWithTransformedItem)
{
    setPowered();
    Player player(EntityInstanceId(1), "TestPlayer");
    // 选中热栏槽位 6（会被交换）
    player.inventory().setSelectedSlot(6);

    const Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    const Item* apple = ItemRegistry::instance().getItem(ResourceLocation("minecraft:apple"));
    ASSERT_NE(apple, nullptr);

    // 热栏槽位 6 放入 3 个石头
    player.inventory().setItem(6, ItemStack(*stone, 3));
    // 书架槽位 0 放入 1 个苹果
    m_shelfEntityPtr->swapItemNoUpdate(0, ItemStack(*apple, 1));

    const BlockState& state = *m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    ASSERT_TRUE(state.get(BlockStateProperties::POWERED()));
    BlockRaycastResult hit = makeHitSlot0();

    auto result = VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::MainHand, hit);

    // 热栏交换成功 → Success
    EXPECT_EQ(result, ActionResultType::Success);
    // 交换前后选中物品不同（石头 → 苹果），应携带 heldItemTransformedTo
    ASSERT_TRUE(result.heldItemTransformedTo().has_value());
    EXPECT_EQ(result.heldItemTransformedTo()->getItem(), apple);
    EXPECT_EQ(result.heldItemTransformedTo()->getCount(), 1);

    // 书架槽位 0 应有 3 个石头（从热栏交换过来）
    EXPECT_EQ(m_shelfEntityPtr->getInventory()->getItem(0).getItem(), stone);
    EXPECT_EQ(m_shelfEntityPtr->getInventory()->getItem(0).getCount(), 3);

    // 热栏槽位 6 应有 1 个苹果（从书架交换过来）
    EXPECT_EQ(player.inventory().getItem(6).getItem(), apple);
    EXPECT_EQ(player.inventory().getItem(6).getCount(), 1);

    // 应播放 MULTI_SWAP 音效
    EXPECT_TRUE(m_world.wasSoundPlayed());
}

TEST_F(ShelfBlockInteractionTest,
    OnBlockActivated_Powered_SwapHotbar_SelectedItemUnchanged_ReturnsSuccessWithoutTransformedItem)
{
    setPowered();
    Player player(EntityInstanceId(1), "TestPlayer");
    // 选中热栏槽位 6（会被交换）
    player.inventory().setSelectedSlot(6);

    const Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);

    // 热栏槽位 6 放入 3 个石头
    player.inventory().setItem(6, ItemStack(*stone, 3));
    // 书架槽位 0 也放入 3 个石头（相同物品、相同数量）
    m_shelfEntityPtr->swapItemNoUpdate(0, ItemStack(*stone, 3));

    const BlockState& state = *m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    ASSERT_TRUE(state.get(BlockStateProperties::POWERED()));
    BlockRaycastResult hit = makeHitSlot0();

    auto result = VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::MainHand, hit);

    // 热栏交换成功 → Success
    EXPECT_EQ(result, ActionResultType::Success);
    // 交换前后选中物品值相同（石头 ↔ 石头，值比较相等），不携带 heldItemTransformedTo
    EXPECT_FALSE(result.heldItemTransformedTo().has_value());

    // 书架槽位 0 仍有 3 个石头（交换了等价物品）
    EXPECT_EQ(m_shelfEntityPtr->getInventory()->getItem(0).getItem(), stone);
    EXPECT_EQ(m_shelfEntityPtr->getInventory()->getItem(0).getCount(), 3);

    // 热栏槽位 6 仍有 3 个石头
    EXPECT_EQ(player.inventory().getItem(6).getItem(), stone);
    EXPECT_EQ(player.inventory().getItem(6).getCount(), 3);

    // 应播放 MULTI_SWAP 音效（发生了交换，虽然值相同）
    EXPECT_TRUE(m_world.wasSoundPlayed());
}

TEST_F(ShelfBlockInteractionTest, OnBlockActivated_Powered_NoSwap_ReturnsConsume)
{
    setPowered();
    Player player(EntityInstanceId(1), "TestPlayer");
    // 选中热栏槽位 6
    player.inventory().setSelectedSlot(6);

    // 书架槽位全部为空，热栏槽位 6/7/8 也全部为空 → 不会发生任何交换
    const BlockState& state = *m_world.getBlockState(m_pos.x, m_pos.y, m_pos.z);
    ASSERT_TRUE(state.get(BlockStateProperties::POWERED()));
    BlockRaycastResult hit = makeHitSlot0();

    m_world.clearSounds();
    auto result = VanillaBlocks::OAK_SHELF->onBlockActivated(state, m_world, m_pos, player, Hand::MainHand, hit);

    // 未发生交换 → Consume
    EXPECT_EQ(result, ActionResultType::Consume);
    // 不携带 heldItemTransformedTo
    EXPECT_FALSE(result.heldItemTransformedTo().has_value());
    // 不应播放 MULTI_SWAP 音效
    EXPECT_FALSE(m_world.wasSoundPlayed());
}

} // namespace
} // namespace mc
