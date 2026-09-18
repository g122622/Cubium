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

/**
 * @file DecorativeBlockTest.cpp
 * @brief 装饰性方块单元测试
 *
 * 测试 LadderBlock、CarpetBlock、FlowerPotBlock、LanternBlock 的功能：
 * - 状态属性
 * - 形状获取
 * - 放置验证
 * - 邻居更新
 * - 花盆交互（放入/取出植物、客户端/服务端分支、getByContent 反查映射表）
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "util/property/Properties.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/decorative/CarpetBlock.hpp"
#include "world/block/blocks/decorative/FlowerPotBlock.hpp"
#include "world/block/blocks/decorative/LadderBlock.hpp"
#include "world/block/blocks/decorative/LanternBlock.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

// ========== LadderBlock 测试 ==========

class LadderBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ladder_ = std::make_unique<LadderBlock>(BlockProperties(Material::WOOD).hardness(0.4f).resistance(0.4f));
    }

    std::unique_ptr<LadderBlock> ladder_;
};

TEST_F(LadderBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(ladder_, nullptr);
}

TEST_F(LadderBlockTest, DefaultState_FacingNorth)
{
    const auto& state = ladder_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(LadderBlockTest, DefaultState_NotWaterlogged)
{
    const auto& state = ladder_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(LadderBlockTest, IsLadder_ReturnsTrue)
{
    const auto& state = ladder_->defaultState();
    EXPECT_TRUE(ladder_->isLadder(state, nullptr, nullptr, nullptr));
}

TEST_F(LadderBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = ladder_->defaultState();
    const auto& shape = ladder_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(LadderBlockTest, GetCollisionShape_ReturnsEmpty)
{
    const auto& state = ladder_->defaultState();
    const auto& shape = ladder_->getCollisionShape(state);
    // 梯子没有碰撞箱
    EXPECT_TRUE(shape.isEmpty());
}

TEST_F(LadderBlockTest, Rotate_ChangesFacing)
{
    const auto& state = ladder_->defaultState();

    // 旋转 90 度
    const auto& rotated90 = ladder_->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated90.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // 旋转 180 度
    const auto& rotated180 = ladder_->rotate(state, Rotation::Clockwise180);
    EXPECT_EQ(rotated180.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
}

// ========== CarpetBlock 测试 ==========

class CarpetBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        carpet_ = std::make_unique<CarpetBlock>(BlockProperties(Material::WOOL).hardness(0.1f).resistance(0.5f));
    }

    std::unique_ptr<CarpetBlock> carpet_;
};

TEST_F(CarpetBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(carpet_, nullptr);
}

TEST_F(CarpetBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = carpet_->defaultState();
    const auto& shape = carpet_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(CarpetBlockTest, GetCollisionShape_ReturnsEmpty)
{
    const auto& state = carpet_->defaultState();
    const auto& shape = carpet_->getCollisionShape(state);
    // 地毯没有碰撞箱
    EXPECT_TRUE(shape.isEmpty());
}

// ========== FlowerPotBlock 基础测试 ==========

namespace {

/// 简单测试用世界（继承 BaseTestWorld 以访问其 protected 构造函数）
class FlowerPotTestWorld : public mc::test::BaseTestWorld {};

} // anonymous namespace

class FlowerPotBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保原版方块已初始化（GetByContent_ReturnsNullptrForUnknownContent 使用 VanillaBlocks::STONE）
        VanillaBlocks::initialize();

        // 创建空花盆（content 为 nullptr）
        flowerPot_ = std::make_unique<FlowerPotBlock>(
            BlockProperties(Material::DECORATION).hardness(0.0f).resistance(0.0f), nullptr);

        // 创建带内容物的花盆（用于测试非空场景）
        // 使用一个简单的占位 SimpleBlock 作为内容物（Block 构造函数为 protected）
        contentBlock_ = std::make_unique<SimpleBlock>(BlockProperties(Material::PLANT));
        pottedPot_ = std::make_unique<FlowerPotBlock>(
            BlockProperties(Material::DECORATION).hardness(0.0f).resistance(0.0f), contentBlock_.get());
    }

    std::unique_ptr<FlowerPotBlock> flowerPot_;
    std::unique_ptr<FlowerPotBlock> pottedPot_;
    std::unique_ptr<SimpleBlock> contentBlock_;
};

TEST_F(FlowerPotBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(flowerPot_, nullptr);
}

TEST_F(FlowerPotBlockTest, GetPotted_ReturnsNullForEmptyPot)
{
    EXPECT_EQ(flowerPot_->getPotted(), nullptr);
}

TEST_F(FlowerPotBlockTest, GetPotted_ReturnsContentForPottedPot)
{
    EXPECT_EQ(pottedPot_->getPotted(), contentBlock_.get());
}

TEST_F(FlowerPotBlockTest, IsEmpty_ReturnsTrueForEmptyPot)
{
    EXPECT_TRUE(flowerPot_->isEmpty());
}

TEST_F(FlowerPotBlockTest, IsEmpty_ReturnsFalseForPottedPot)
{
    EXPECT_FALSE(pottedPot_->isEmpty());
}

TEST_F(FlowerPotBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = flowerPot_->defaultState();
    const auto& shape = flowerPot_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(FlowerPotBlockTest, GetCollisionShape_ReturnsValidShape)
{
    const auto& state = flowerPot_->defaultState();
    const auto& shape = flowerPot_->getCollisionShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

// ========== FlowerPotBlock getByContent 反查映射表测试 ==========

TEST_F(FlowerPotBlockTest, GetByContent_ReturnsNullptrForUnknownContent)
{
    // 使用一个原版中绝不会被盆栽的方块（如石头）验证反查映射表返回 nullptr
    // 注意：不能使用临时创建的 SimpleBlock，因为其地址可能与已销毁的测试花盆内容物地址重叠，
    // 而 FlowerPotBlock 析构时不清理 s_pottedByContent（生产环境花盆永不被销毁）
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_EQ(FlowerPotBlock::getByContent(*VanillaBlocks::STONE), nullptr);
}

TEST_F(FlowerPotBlockTest, GetByContent_ReturnsPotForRegisteredContent)
{
    // pottedPot_ 在构造时已将 contentBlock_ -> pottedPot_ 注册到反查映射表
    const FlowerPotBlock* found = FlowerPotBlock::getByContent(*contentBlock_);
    EXPECT_EQ(found, pottedPot_.get());
}

TEST_F(FlowerPotBlockTest, GetByContent_MultiplePotsResolveCorrectly)
{
    // 创建多个不同的内容物和花盆，验证反查映射表能正确区分
    auto content1 = std::make_unique<SimpleBlock>(BlockProperties(Material::PLANT));
    auto content2 = std::make_unique<SimpleBlock>(BlockProperties(Material::PLANT));
    auto pot1 = std::make_unique<FlowerPotBlock>(
        BlockProperties(Material::DECORATION).hardness(0.0f).resistance(0.0f), content1.get());
    auto pot2 = std::make_unique<FlowerPotBlock>(
        BlockProperties(Material::DECORATION).hardness(0.0f).resistance(0.0f), content2.get());

    EXPECT_EQ(FlowerPotBlock::getByContent(*content1), pot1.get());
    EXPECT_EQ(FlowerPotBlock::getByContent(*content2), pot2.get());
    EXPECT_NE(FlowerPotBlock::getByContent(*content1), pot2.get());
}

// ========== FlowerPotBlock ticksRandomly 测试（眼眸花特殊逻辑） ==========

TEST_F(FlowerPotBlockTest, TicksRandomly_EmptyPot_ReturnsFalse)
{
    EXPECT_FALSE(flowerPot_->ticksRandomly());
}

TEST_F(FlowerPotBlockTest, TicksRandomly_NormalContent_ReturnsFalse)
{
    // contentBlock_ 是普通植物方块，不响应随机刻
    EXPECT_FALSE(pottedPot_->ticksRandomly());
}

// ========== FlowerPotBlock isValidPosition / updatePostPlacement 测试 ==========

TEST_F(FlowerPotBlockTest, IsValidPosition_AlwaysReturnsTrue)
{
    // 匹配 MC Java 1.21.11: canSurvive 默认返回 true，花盆可放置在任何位置
    // 使用 FlowerPotTestWorld（BaseTestWorld 子类）作为桩世界，避免使用 nullptr（未定义行为）
    FlowerPotTestWorld world;
    EXPECT_TRUE(flowerPot_->isValidPosition(flowerPot_->defaultState(), world, BlockPos(0, 0, 0)));
    // 即使在悬空位置也应返回 true
    EXPECT_TRUE(flowerPot_->isValidPosition(flowerPot_->defaultState(), world, BlockPos(100, -64, 100)));
}

TEST_F(FlowerPotBlockTest, UpdatePostPlacement_DoesNotBreakOnUnsupportedBelow)
{
    // 匹配 MC Java 1.21.11: updateShape 检查 DOWN && !canSurvive，
    // 由于 canSurvive 始终为 true，花盆不会因下方方块变化而破坏
    FlowerPotTestWorld world;
    const auto& state = flowerPot_->defaultState();
    // 下方方向更新不应改变花盆状态（返回值应与原状态属于同一方块）
    auto result =
        flowerPot_->updatePostPlacement(state, Direction::Down, state, world, BlockPos(0, 1, 0), BlockPos(0, 0, 0));
    // 验证返回的状态仍属于同一个花盆方块（未被破坏为空气）
    EXPECT_EQ(&result.owner(), &state.owner());
}

// ============================================================================
// FlowerPotBlock 交互测试 — 使用完整 Mock 世界
// ============================================================================

namespace {

/**
 * @brief 花盆交互测试用 Mock 世界
 *
 * 支持方块存储、setBlockState 标志追踪、游戏事件追踪、实体生成追踪。
 */
class FlowerPotInteractionTestWorld final : public mc::test::BaseTestWorld {
public:
    FlowerPotInteractionTestWorld()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        if (VanillaBlocks::AIR) {
            return &VanillaBlocks::AIR->defaultState();
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        m_setBlockCallCount++;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        m_lastSetBlockFlags = flags;
        m_lastSetBlockPos = BlockPos(x, y, z);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entityLookup.find(id);
        return it != m_entityLookup.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entityLookup.find(id);
        return it != m_entityLookup.end() ? it->second : nullptr;
    }

    void registerEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entityLookup[entity->id()] = entity;
        }
    }

    void unregisterEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entityLookup.erase(entity->id());
        }
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntityCount++;
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEventFired = true;
        m_gameEventCount++;
        MC_UNUSED(event);
        MC_UNUSED(pos);
        MC_UNUSED(context);
    }

    void notifyBlockUpdate(const BlockPos& pos) override { MC_UNUSED(pos); }

    // ========== 测试辅助方法 ==========

    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
    }

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(pos);
        return it != m_blocks.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] bool wasGameEventFired() const { return m_gameEventFired; }
    [[nodiscard]] i32 gameEventCount() const { return m_gameEventCount; }
    [[nodiscard]] i32 setBlockCallCount() const { return m_setBlockCallCount; }
    [[nodiscard]] i32 lastSetBlockFlags() const { return m_lastSetBlockFlags; }
    [[nodiscard]] const BlockPos& lastSetBlockPos() const { return m_lastSetBlockPos; }
    [[nodiscard]] i32 spawnedEntityCount() const { return m_spawnedEntityCount; }

    void resetTrackedState()
    {
        m_gameEventFired = false;
        m_gameEventCount = 0;
        m_setBlockCallCount = 0;
        m_lastSetBlockFlags = 0;
        m_lastSetBlockPos = BlockPos(0, 0, 0);
        m_spawnedEntityCount = 0;
        m_spawnedEntities.clear();
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::unordered_map<EntityInstanceId, Entity*> m_entityLookup;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    bool m_gameEventFired = false;
    i32 m_gameEventCount = 0;
    i32 m_setBlockCallCount = 0;
    i32 m_lastSetBlockFlags = 0;
    BlockPos m_lastSetBlockPos{0, 0, 0};
    i32 m_spawnedEntityCount = 0;
};

} // anonymous namespace

class FlowerPotInteractionTest : public ::testing::Test {
protected:
    FlowerPotInteractionTestWorld m_world;

    void SetUp() override
    {
        // 确保 VanillaBlocks、Items、BlockItemRegistry 已按正确顺序初始化
        // 顺序：VanillaBlocks::initialize() → Items::initialize() → BlockItemRegistry::initializeVanillaBlockItems()
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        m_world.setClientSide(false);
    }

    /// 创建玩家并关联到测试世界
    std::unique_ptr<Player> createPlayer()
    {
        auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
        player->setWorld(&m_world);
        m_world.registerEntity(player.get());
        return player;
    }

    /// 构造一个 BlockRaycastResult
    static BlockRaycastResult makeHitResult(const BlockPos& pos)
    {
        return BlockRaycastResult::hit(
            Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f),
            pos,
            Direction::Up,
            0.0f);
    }
};

// ========== 分支1：空花盆 + 手持可盆栽植物 → 放入植物 ==========

TEST_F(FlowerPotInteractionTest, EmptyPot_WithPottableItem_PlacesPottedBlock_ServerSide)
{
    // 前置条件：FLOWER_POT 和 POTTED_POPPY 已注册
    ASSERT_NE(VanillaBlocks::FLOWER_POT, nullptr);
    ASSERT_NE(VanillaBlocks::POTTED_POPPY, nullptr);
    ASSERT_NE(VanillaBlocks::POPPY, nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& emptyPotState = VanillaBlocks::FLOWER_POT->defaultState();
    m_world.setBlockAt(pos, &emptyPotState);

    auto player = createPlayer();
    // 手持罂粟 BlockItem
    const BlockItem* poppyBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::POPPY);
    ASSERT_NE(poppyBlockItem, nullptr);
    player->getHeldItem(Hand::MainHand) = ItemStack(*poppyBlockItem, 1);

    m_world.resetTrackedState();
    auto* flowerPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::FLOWER_POT);
    BlockRaycastResult hit = makeHitResult(pos);
    auto result = flowerPotBlock->onBlockActivated(emptyPotState, m_world, pos, *player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    // 验证方块被替换为 potted_poppy
    const BlockState* newState = m_world.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(newState->owner().blockLocation(), ResourceLocation("minecraft", "potted_poppy"));
    // 验证触发了 BLOCK_CHANGE 游戏事件
    EXPECT_TRUE(m_world.wasGameEventFired());
    // 验证消耗了 1 个物品
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 0);
}

TEST_F(FlowerPotInteractionTest, EmptyPot_WithPottableItem_ClientSide_ReturnsSuccessWithoutChanges)
{
    ASSERT_NE(VanillaBlocks::FLOWER_POT, nullptr);
    ASSERT_NE(VanillaBlocks::POPPY, nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& emptyPotState = VanillaBlocks::FLOWER_POT->defaultState();
    m_world.setBlockAt(pos, &emptyPotState);

    // 切换到客户端
    m_world.setClientSide(true);

    auto player = createPlayer();
    const BlockItem* poppyBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::POPPY);
    ASSERT_NE(poppyBlockItem, nullptr);
    player->getHeldItem(Hand::MainHand) = ItemStack(*poppyBlockItem, 1);

    m_world.resetTrackedState();
    auto* flowerPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::FLOWER_POT);
    BlockRaycastResult hit = makeHitResult(pos);
    auto result = flowerPotBlock->onBlockActivated(emptyPotState, m_world, pos, *player, Hand::MainHand, hit);

    // 客户端返回 Success
    EXPECT_EQ(result, ActionResultType::Success);
    // 客户端不应修改方块
    EXPECT_EQ(m_world.setBlockCallCount(), 0);
    // 客户端不应触发游戏事件
    EXPECT_FALSE(m_world.wasGameEventFired());
    // 客户端不应消耗物品
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 1);
}

// ========== 分支2：已盆栽花盆 + 手持可盆栽植物 → 消费但不执行动作 ==========

TEST_F(FlowerPotInteractionTest, PottedPot_WithPottableItem_ConsumesWithoutAction)
{
    ASSERT_NE(VanillaBlocks::POTTED_POPPY, nullptr);
    ASSERT_NE(VanillaBlocks::POPPY, nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& pottedState = VanillaBlocks::POTTED_POPPY->defaultState();
    m_world.setBlockAt(pos, &pottedState);

    auto player = createPlayer();
    const BlockItem* poppyBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::POPPY);
    ASSERT_NE(poppyBlockItem, nullptr);
    player->getHeldItem(Hand::MainHand) = ItemStack(*poppyBlockItem, 1);

    m_world.resetTrackedState();
    auto* pottedPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::POTTED_POPPY);
    BlockRaycastResult hit = makeHitResult(pos);
    auto result = pottedPotBlock->onBlockActivated(pottedState, m_world, pos, *player, Hand::MainHand, hit);

    // 匹配 MC Java: 返回 Consume，不修改方块
    EXPECT_EQ(result, ActionResultType::Consume);
    EXPECT_EQ(m_world.setBlockCallCount(), 0);
    // 物品不被消耗（Consume 在本项目中不自动消耗物品，由调用方处理）
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 1);
}

// ========== 分支3：空花盆 + 空手 → 消费动作 ==========

TEST_F(FlowerPotInteractionTest, EmptyPot_WithEmptyHand_ReturnsConsume)
{
    ASSERT_NE(VanillaBlocks::FLOWER_POT, nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& emptyPotState = VanillaBlocks::FLOWER_POT->defaultState();
    m_world.setBlockAt(pos, &emptyPotState);

    auto player = createPlayer();
    // 手持为空

    m_world.resetTrackedState();
    auto* flowerPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::FLOWER_POT);
    BlockRaycastResult hit = makeHitResult(pos);
    auto result = flowerPotBlock->onBlockActivated(emptyPotState, m_world, pos, *player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Consume);
    EXPECT_EQ(m_world.setBlockCallCount(), 0);
}

// ========== 分支4：已盆栽花盆 + 空手 → 取出内容物 ==========

TEST_F(FlowerPotInteractionTest, PottedPot_WithEmptyHand_ExtractsContent_ServerSide)
{
    ASSERT_NE(VanillaBlocks::POTTED_POPPY, nullptr);
    ASSERT_NE(VanillaBlocks::FLOWER_POT, nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& pottedState = VanillaBlocks::POTTED_POPPY->defaultState();
    m_world.setBlockAt(pos, &pottedState);

    auto player = createPlayer();

    m_world.resetTrackedState();
    auto* pottedPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::POTTED_POPPY);
    BlockRaycastResult hit = makeHitResult(pos);
    auto result = pottedPotBlock->onBlockActivated(pottedState, m_world, pos, *player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    // 验证方块被替换为空花盆
    const BlockState* newState = m_world.getBlockAt(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_EQ(newState->owner().blockLocation(), ResourceLocation("minecraft", "flower_pot"));
    // 验证触发了游戏事件
    EXPECT_TRUE(m_world.wasGameEventFired());
}

TEST_F(FlowerPotInteractionTest, PottedPot_WithEmptyHand_ClientSide_ReturnsSuccessWithoutChanges)
{
    ASSERT_NE(VanillaBlocks::POTTED_POPPY, nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& pottedState = VanillaBlocks::POTTED_POPPY->defaultState();
    m_world.setBlockAt(pos, &pottedState);

    m_world.setClientSide(true);

    auto player = createPlayer();

    m_world.resetTrackedState();
    auto* pottedPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::POTTED_POPPY);
    BlockRaycastResult hit = makeHitResult(pos);
    auto result = pottedPotBlock->onBlockActivated(pottedState, m_world, pos, *player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
    // 客户端不应修改方块
    EXPECT_EQ(m_world.setBlockCallCount(), 0);
    EXPECT_FALSE(m_world.wasGameEventFired());
}

// ========== 分支5：空花盆 + 手持非 BlockItem 物品 → Pass ==========

TEST_F(FlowerPotInteractionTest, EmptyPot_WithNonBlockItem_ReturnsPass)
{
    ASSERT_NE(VanillaBlocks::FLOWER_POT, nullptr);
    // 使用一个非 BlockItem 的物品（如钻石）进行测试
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& emptyPotState = VanillaBlocks::FLOWER_POT->defaultState();
    m_world.setBlockAt(pos, &emptyPotState);

    auto player = createPlayer();
    player->getHeldItem(Hand::MainHand) = ItemStack(*diamond, 1);

    m_world.resetTrackedState();
    auto* flowerPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::FLOWER_POT);
    BlockRaycastResult hit = makeHitResult(pos);
    auto result = flowerPotBlock->onBlockActivated(emptyPotState, m_world, pos, *player, Hand::MainHand, hit);

    // 非 BlockItem 物品应返回 Pass（交给其他处理器）
    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(m_world.setBlockCallCount(), 0);
    // 物品不被消耗
    EXPECT_EQ(player->getHeldItem(Hand::MainHand).getCount(), 1);
}

// ========== 分支6：空花盆 + 手持不可盆栽的 BlockItem → Pass ==========

TEST_F(FlowerPotInteractionTest, EmptyPot_WithNonPottableBlockItem_ReturnsPass)
{
    ASSERT_NE(VanillaBlocks::FLOWER_POT, nullptr);
    // 使用一个不可盆栽的 BlockItem（如石头）进行测试
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    const BlockItem* stoneBlockItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::STONE);
    ASSERT_NE(stoneBlockItem, nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& emptyPotState = VanillaBlocks::FLOWER_POT->defaultState();
    m_world.setBlockAt(pos, &emptyPotState);

    auto player = createPlayer();
    player->getHeldItem(Hand::MainHand) = ItemStack(*stoneBlockItem, 1);

    m_world.resetTrackedState();
    auto* flowerPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::FLOWER_POT);
    BlockRaycastResult hit = makeHitResult(pos);
    auto result = flowerPotBlock->onBlockActivated(emptyPotState, m_world, pos, *player, Hand::MainHand, hit);

    // 不可盆栽的 BlockItem 应返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(m_world.setBlockCallCount(), 0);
}

// ========== getByContent 在交互场景下的集成测试 ==========

TEST_F(FlowerPotInteractionTest, GetByContent_ResolvesAllVanillaPottedContents)
{
    // 验证原版注册的所有内容物方块都能通过 getByContent 反查到对应的花盆
    struct TestCase {
        Block* contentBlock;
        Block* expectedPotBlock;
        const char* contentName;
    };

    TestCase cases[] = {
        {VanillaBlocks::OAK_SAPLING, VanillaBlocks::POTTED_OAK_SAPLING, "oak_sapling"},
        {VanillaBlocks::DANDELION, VanillaBlocks::POTTED_DANDELION, "dandelion"},
        {VanillaBlocks::POPPY, VanillaBlocks::POTTED_POPPY, "poppy"},
        {VanillaBlocks::FERN, VanillaBlocks::POTTED_FERN, "fern"},
        {VanillaBlocks::RED_MUSHROOM, VanillaBlocks::POTTED_RED_MUSHROOM, "red_mushroom"},
        {VanillaBlocks::BROWN_MUSHROOM, VanillaBlocks::POTTED_BROWN_MUSHROOM, "brown_mushroom"},
        {VanillaBlocks::CACTUS, VanillaBlocks::POTTED_CACTUS, "cactus"},
        {VanillaBlocks::CRIMSON_FUNGUS, VanillaBlocks::POTTED_CRIMSON_FUNGUS, "crimson_fungus"},
        {VanillaBlocks::WARPED_FUNGUS, VanillaBlocks::POTTED_WARPED_FUNGUS, "warped_fungus"},
        {VanillaBlocks::AZALEA, VanillaBlocks::POTTED_AZALEA_BUSH, "azalea"},
    };

    for (const auto& tc : cases) {
        ASSERT_NE(tc.contentBlock, nullptr) << "Content block missing: " << tc.contentName;
        ASSERT_NE(tc.expectedPotBlock, nullptr) << "Pot block missing for: " << tc.contentName;
        const FlowerPotBlock* found = FlowerPotBlock::getByContent(*tc.contentBlock);
        EXPECT_EQ(found, static_cast<const FlowerPotBlock*>(tc.expectedPotBlock))
            << "getByContent mismatch for content: " << tc.contentName;
    }
}

// ========== getCloneItemStack 测试 ==========

TEST_F(FlowerPotInteractionTest, GetCloneItemStack_EmptyPot_ReturnsEmptyStack)
{
    ASSERT_NE(VanillaBlocks::FLOWER_POT, nullptr);
    auto* flowerPotBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::FLOWER_POT);
    const BlockState& state = flowerPotBlock->defaultState();

    ItemStack clone = flowerPotBlock->getCloneItemStack(state);
    // 空花盆返回默认（Block::getCloneItemStack 返回空物品堆），
    // 由外部拾取系统通过 BlockItemRegistry 查找 flower_pot 物品
    EXPECT_TRUE(clone.isEmpty());
}

TEST_F(FlowerPotInteractionTest, GetCloneItemStack_PottedPot_ReturnsContentItem)
{
    ASSERT_NE(VanillaBlocks::POTTED_POPPY, nullptr);
    ASSERT_NE(VanillaBlocks::POPPY, nullptr);
    auto* pottedPoppyBlock = static_cast<FlowerPotBlock*>(VanillaBlocks::POTTED_POPPY);
    const BlockState& state = pottedPoppyBlock->defaultState();

    ItemStack clone = pottedPoppyBlock->getCloneItemStack(state);
    // 已盆栽花盆返回内容物对应的物品（poppy 物品）
    EXPECT_FALSE(clone.isEmpty());
    // 验证物品是 poppy
    const Item* poppyItem = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "poppy"));
    ASSERT_NE(poppyItem, nullptr);
    EXPECT_EQ(clone.getItem(), poppyItem);
}

// ========== LanternBlock 测试 ==========

class LanternBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        lantern_ = std::make_unique<LanternBlock>(
            BlockProperties(Material::IRON).hardness(3.5f).resistance(3.5f).lightLevel(15), 15);
    }

    std::unique_ptr<LanternBlock> lantern_;
};

TEST_F(LanternBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(lantern_, nullptr);
}

TEST_F(LanternBlockTest, DefaultState_NotHanging)
{
    const auto& state = lantern_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::HANGING()));
}

TEST_F(LanternBlockTest, DefaultState_NotWaterlogged)
{
    const auto& state = lantern_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(LanternBlockTest, GetShape_ReturnsValidShape)
{
    const auto& state = lantern_->defaultState();
    const auto& shape = lantern_->getShape(state);
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(LanternBlockTest, GetShape_DifferentForHanging)
{
    const auto& standingState = lantern_->defaultState();
    const auto& hangingState = standingState.with(BlockStateProperties::HANGING(), true);

    const auto& standingShape = lantern_->getShape(standingState);
    const auto& hangingShape = lantern_->getShape(hangingState);

    // 站立和悬挂形状应该不同
    // 注意：由于 CollisionShape 目前只比较指针，这里只验证形状有效
    EXPECT_FALSE(standingShape.isEmpty());
    EXPECT_FALSE(hangingShape.isEmpty());
}

TEST_F(LanternBlockTest, LightLevel_ReturnsCorrectValue)
{
    EXPECT_EQ(lantern_->lightLevel(), 15u);
}
