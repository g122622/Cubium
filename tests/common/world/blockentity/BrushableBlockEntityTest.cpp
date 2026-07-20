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

#include "common/world/blockentity/interactive/BrushableBlockEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootPool.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/entries/ItemLootEntry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品
 */
Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }
    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}

/**
 * @brief 创建简单的测试战利品表（包含单个物品）
 *
 * @param manager 战利品表管理器
 * @param tableId 战利品表ID（如 "minecraft:archaeology/test"）
 * @param itemId 物品ID（如 "minecraft:diamond"）
 * @param count 生成数量
 */
void registerSimpleLootTable(
    loot::LootTableManager& manager, const std::string& tableId, const std::string& itemId, i32 count)
{
    auto table = std::make_unique<loot::LootTable>();
    auto pool = std::make_unique<loot::LootPool>();
    pool->setRolls(loot::RandomValueRange(static_cast<f32>(count), static_cast<f32>(count)));
    auto entry = std::make_unique<loot::ItemLootEntry>(itemId, loot::RandomValueRange(1.0f, 1.0f), 1, 0);
    pool->addEntry(std::move(entry));
    table->addPool(std::move(pool));
    manager.registerTable(tableId, std::move(table));
}

/**
 * @brief 创建空的测试战利品表（没有条目，生成 0 个物品）
 */
void registerEmptyLootTable(loot::LootTableManager& manager, const std::string& tableId)
{
    auto table = std::make_unique<loot::LootTable>();
    manager.registerTable(tableId, std::move(table));
}

/**
 * @brief 测试用世界，支持 BrushableBlockEntity 所需的全部接口
 *
 * 提供：
 * - LootTableManager 注入（可切换为 nullptr 模拟客户端）
 * - getBlockEntity / setBlockEntity 管理（内存映射）
 * - getBlockState / setBlockState 方块状态存储
 * - playEvent / spawnEntity 调用追踪
 * - getGameTime 可设置
 * - isClientSide 可设置（用于 scheduleBlockTick 测试）
 */
class BrushableTestWorld final : public test::BaseTestWorld {
public:
    using IWorld::getBlockState;
    using IWorld::setBlockState;

    // ========== 方块状态存储 ==========
    void setBlockStateAt(const BlockPos& pos, const BlockState* state) { m_blockStates[pos] = state; }
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        return it != m_blockStates.end() ? it->second : nullptr;
    }
    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blockStates[BlockPos(x, y, z)] = state;
        return true;
    }

    // ========== 方块实体存储 ==========
    void setBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity)
    {
        m_blockEntities[pos] = std::move(entity);
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
    void removeBlockEntity(const BlockPos& pos) override { m_blockEntities.erase(pos); }

    // ========== 战利品表管理器 ==========
    [[nodiscard]] const loot::LootTableManager* lootTableManager() const override
    {
        if (m_noManager) {
            return nullptr;
        }
        return m_customManager != nullptr ? m_customManager : &m_lootTableManager;
    }
    void setLootTableManager(loot::LootTableManager* manager) { m_customManager = manager; }
    void setNoLootTableManager() { m_noManager = true; }
    [[nodiscard]] loot::LootTableManager& mutableLootTableManager() { return m_lootTableManager; }

    // ========== 时间 ==========
    void setGameTime(u64 tick) { m_gameTime = tick; }
    [[nodiscard]] u64 currentTick() const override { return m_gameTime; }

    // ========== 客户端标志 ==========
    void setClientSide(bool client) { m_clientSide = client; }
    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }

    // ========== playEvent 追踪 ==========
    struct EventCall {
        i32 eventId = 0;
        BlockPos pos;
        i32 data = 0;
    };
    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_eventCalls.push_back({eventId, pos, data});
    }
    [[nodiscard]] const std::vector<EventCall>& eventCalls() const { return m_eventCalls; }
    [[nodiscard]] bool hasEvent(i32 eventId) const
    {
        for (const auto& call : m_eventCalls) {
            if (call.eventId == eventId) {
                return true;
            }
        }
        return false;
    }

    // ========== spawnEntity 追踪 ==========
    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return EntityInstanceId(static_cast<u64>(m_spawnedEntities.size()));
    }
    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    // ========== 边界 ==========
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    loot::LootTableManager m_lootTableManager;
    loot::LootTableManager* m_customManager = nullptr;
    bool m_noManager = false;
    bool m_clientSide = false;
    u64 m_gameTime = 0;
    std::vector<EventCall> m_eventCalls;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

} // namespace

// ============================================================================
// 构造与基本属性测试
// ============================================================================

class BrushableBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        entity_ = std::make_unique<BrushableBlockEntity>(BlockPos(10, 20, 30));
        m_testItem = ensureTestItem("diamond");
    }

    std::unique_ptr<BrushableBlockEntity> entity_;
    Item* m_testItem = nullptr;
};

TEST_F(BrushableBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(entity_->getType(), BlockEntityType::BrushableBlock);
}

TEST_F(BrushableBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(entity_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(BrushableBlockEntityTest, Create_DefaultBrushCountIsZero)
{
    EXPECT_EQ(entity_->getBrushCount(), 0);
}

TEST_F(BrushableBlockEntityTest, Create_DefaultCompletionStateIsZero)
{
    EXPECT_EQ(entity_->getCompletionState(), 0);
}

TEST_F(BrushableBlockEntityTest, Create_DefaultHitDirectionIsEmpty)
{
    EXPECT_FALSE(entity_->getHitDirection().has_value());
}

TEST_F(BrushableBlockEntityTest, Create_DefaultItemIsEmpty)
{
    EXPECT_TRUE(entity_->getItem().isEmpty());
}

TEST_F(BrushableBlockEntityTest, NeedsTick_ReturnsFalse)
{
    // BrushableBlockEntity 的 checkReset 由方块计划刻触发，不需要 BlockEntity::tick
    EXPECT_FALSE(entity_->needsTick());
}

TEST_F(BrushableBlockEntityTest, Constants_HaveCorrectValues)
{
    EXPECT_EQ(BrushableBlockEntity::BRUSH_COOLDOWN_TICKS, 10);
    EXPECT_EQ(BrushableBlockEntity::BRUSH_RESET_TICKS, 40);
    EXPECT_EQ(BrushableBlockEntity::REQUIRED_BRUSHES_TO_BREAK, 10);
    EXPECT_EQ(BrushableBlockEntity::BRUSH_RESET_RETRY_TICKS, 4);
    EXPECT_EQ(BrushableBlockEntity::TICK_DELAY, 2);
}

TEST_F(BrushableBlockEntityTest, NbtKeys_HaveCorrectValues)
{
    EXPECT_STREQ(BrushableBlockEntity::LOOT_TABLE_TAG, "LootTable");
    EXPECT_STREQ(BrushableBlockEntity::LOOT_TABLE_SEED_TAG, "LootTableSeed");
    EXPECT_STREQ(BrushableBlockEntity::HIT_DIRECTION_TAG, "hit_direction");
    EXPECT_STREQ(BrushableBlockEntity::ITEM_TAG, "item");
}

// ============================================================================
// getCompletionState 测试（DUSTED 0-3 阈值）
// ============================================================================

class BrushableCompletionStateTest : public BrushableBlockEntityTest {};

// 通过 setLootTable + brush 间接设置 brushCount 来测试 getCompletionState
// brushCount 0 -> completion 0
// brushCount 1,2 -> completion 1
// brushCount 3,4,5 -> completion 2
// brushCount 6+ -> completion 3

TEST_F(BrushableCompletionStateTest, CompletionState_BrushCount0_Returns0)
{
    EXPECT_EQ(entity_->getCompletionState(), 0);
}

// 以下测试通过模拟多次 brush 调用来验证 completion state
// 由于 brush 需要 world + entity，在专门的 BrushLogicTest 中覆盖

// ============================================================================
// setLootTable 测试
// ============================================================================

TEST_F(BrushableBlockEntityTest, SetLootTable_StoresLootTableAndSeed)
{
    const ResourceLocation tableId("minecraft", "archaeology/desert_pyramid");
    entity_->setLootTable(tableId, 12345LL);

    // setLootTable 后 item 应为空（尚未 unpack）
    EXPECT_TRUE(entity_->getItem().isEmpty());
}

TEST_F(BrushableBlockEntityTest, SetLootTable_OverwritesExistingItem)
{
    // 先设置一个物品（模拟已 unpack 的状态）
    // 由于无法直接设置 m_item，通过 save/load 验证
    nlohmann::json data;
    data["item"] = ItemStack(m_testItem, 5).toJson();
    entity_->load(data);
    EXPECT_FALSE(entity_->getItem().isEmpty());

    // setLootTable 应清空 item
    const ResourceLocation tableId("minecraft", "archaeology/test");
    entity_->setLootTable(tableId, 42LL);
    EXPECT_TRUE(entity_->getItem().isEmpty());
}

TEST_F(BrushableBlockEntityTest, SetLootTable_MarksChanged)
{
    EXPECT_FALSE(entity_->isChanged());
    const ResourceLocation tableId("minecraft", "archaeology/test");
    entity_->setLootTable(tableId, 1LL);
    EXPECT_TRUE(entity_->isChanged());
}

// ============================================================================
// brush() 核心逻辑测试
// ============================================================================

class BrushableBrushLogicTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureTestItem("diamond");
        m_brushItem = ensureTestItem("brush");

        // 注册测试战利品表（产出 1 个钻石）
        registerSimpleLootTable(
            world_.mutableLootTableManager(), "minecraft:archaeology/test_brush", "minecraft:diamond", 1);

        entity_ = std::make_unique<BrushableBlockEntity>(BlockPos(10, 64, 20));
        entity_->setWorld(&world_);
        world_.setBlockEntity(BlockPos(10, 64, 20), std::make_unique<BrushableBlockEntity>(BlockPos(10, 64, 20)));

        // 设置战利品表
        entity_->setLootTable(ResourceLocation("minecraft", "archaeology/test_brush"), 42LL);

        // 创建 LivingEntity（brush 需要 LivingEntity&）
        livingEntity_ = std::make_unique<LivingEntity>(EntityInstanceId(1), &world_);
    }

    BrushableTestWorld world_;
    std::unique_ptr<BrushableBlockEntity> entity_;
    std::unique_ptr<LivingEntity> livingEntity_;
    Item* m_diamond = nullptr;
    Item* m_brushItem = nullptr;
};

TEST_F(BrushableBrushLogicTest, Brush_FirstCall_RecordsHitDirection)
{
    ItemStack brushStack(m_brushItem, 1);
    EXPECT_FALSE(entity_->getHitDirection().has_value());

    entity_->brush(0, world_, *livingEntity_, Direction::North, brushStack);

    EXPECT_TRUE(entity_->getHitDirection().has_value());
    EXPECT_EQ(entity_->getHitDirection().value(), Direction::North);
}

TEST_F(BrushableBrushLogicTest, Brush_FirstCall_DoesNotOverrideHitDirection)
{
    ItemStack brushStack(m_brushItem, 1);

    // 第一次刷扫记录方向
    entity_->brush(0, world_, *livingEntity_, Direction::North, brushStack);
    EXPECT_EQ(entity_->getHitDirection().value(), Direction::North);

    // 第二次从不同方向刷扫，hitDirection 不应改变
    entity_->brush(20, world_, *livingEntity_, Direction::South, brushStack);
    EXPECT_EQ(entity_->getHitDirection().value(), Direction::North);
}

TEST_F(BrushableBrushLogicTest, Brush_SuccessfulCall_IncrementsBrushCount)
{
    ItemStack brushStack(m_brushItem, 1);
    EXPECT_EQ(entity_->getBrushCount(), 0);

    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);

    EXPECT_EQ(entity_->getBrushCount(), 1);
}

TEST_F(BrushableBrushLogicTest, Brush_SuccessfulCall_UnpacksLootTable)
{
    ItemStack brushStack(m_brushItem, 1);
    EXPECT_TRUE(entity_->getItem().isEmpty());

    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);

    // unpackLootTable 应已生成物品（1 个钻石）
    EXPECT_FALSE(entity_->getItem().isEmpty());
    EXPECT_EQ(entity_->getItem().getItem(), m_diamond);
}

TEST_F(BrushableBrushLogicTest, Brush_DuringCooldown_ReturnsFalseAndDoesNotIncrement)
{
    ItemStack brushStack(m_brushItem, 1);

    // 第一次刷扫（gameTime=0），成功
    bool result1 = entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_FALSE(result1);
    EXPECT_EQ(entity_->getBrushCount(), 1);

    // 在冷却期内（gameTime=5 < cooldown=10）刷扫，应返回 false 且不递增
    bool result2 = entity_->brush(5, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_FALSE(result2);
    EXPECT_EQ(entity_->getBrushCount(), 1); // 未递增
}

TEST_F(BrushableBrushLogicTest, Brush_AfterCooldown_ResumesBrushing)
{
    ItemStack brushStack(m_brushItem, 1);

    // 第一次刷扫（gameTime=0）
    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 1);

    // 冷却期内（gameTime=5）刷扫，不递增
    entity_->brush(5, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 1);

    // 冷却结束后（gameTime=10）刷扫，应递增
    entity_->brush(10, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 2);
}

TEST_F(BrushableBrushLogicTest, Brush_CooldownExactlyAtBoundary_AllowsBrush)
{
    ItemStack brushStack(m_brushItem, 1);

    // gameTime=0 刷扫，cooldownEndsAt = 0 + 10 = 10
    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 1);

    // gameTime=10 不小于 coolDownEndsAtTick(10)，应允许刷扫
    entity_->brush(10, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 2);
}

TEST_F(BrushableBrushLogicTest, Brush_AlwaysUpdatesBrushCountResetsAtTick)
{
    ItemStack brushStack(m_brushItem, 1);

    // 第一次刷扫（gameTime=100）
    entity_->brush(100, world_, *livingEntity_, Direction::Up, brushStack);
    // brushCountResetsAtTick 应为 100 + 40 = 140

    // 冷却期内刷扫（gameTime=105），brushCount 不递增但 brushCountResetsAtTick 仍更新
    entity_->brush(105, world_, *livingEntity_, Direction::Up, brushStack);
    // brushCountResetsAtTick 应为 105 + 40 = 145（对齐 MC：每次 brush 都更新）

    // 验证：通过 checkReset 在 gameTime=141 时不重置（因为 resetsAt=145）
    // 但在 gameTime=145 时应重置
    // 由于 checkReset 会递减 brushCount，我们先记录当前值
    const i32 countBefore = entity_->getBrushCount();

    // gameTime=141 < resetsAt(145)，不应触发重置
    world_.setGameTime(141);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), countBefore); // 未递减

    // gameTime=145 >= resetsAt(145)，应触发重置（brushCount - 2）
    world_.setGameTime(145);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), std::max(0, countBefore - 2));
}

TEST_F(BrushableBrushLogicTest, Brush_TenTimes_CompletesAndReturnsTrue)
{
    ItemStack brushStack(m_brushItem, 1);

    // 刷扫 10 次，每次间隔 10 tick（避免冷却）
    bool lastResult = false;
    for (i32 i = 0; i < 10; ++i) {
        lastResult = entity_->brush(static_cast<i64>(i * 10), world_, *livingEntity_, Direction::Up, brushStack);
    }

    // 第 10 次应返回 true（完成）
    EXPECT_TRUE(lastResult);
}

TEST_F(BrushableBrushLogicTest, Brush_Completion_DropsItemAndClearsCache)
{
    ItemStack brushStack(m_brushItem, 1);

    // 刷扫 10 次完成
    for (i32 i = 0; i < 10; ++i) {
        entity_->brush(static_cast<i64>(i * 10), world_, *livingEntity_, Direction::Up, brushStack);
    }

    // 完成后应触发物品掉落（spawnEntity 被调用）
    EXPECT_GT(world_.spawnedEntityCount(), 0u);

    // 完成后 item 应被清空（dropContent 清空 m_item）
    EXPECT_TRUE(entity_->getItem().isEmpty());
}

TEST_F(BrushableBrushLogicTest, Brush_Completion_TriggersBrushBlockCompleteEvent)
{
    ItemStack brushStack(m_brushItem, 1);

    // 刷扫 10 次完成
    for (i32 i = 0; i < 10; ++i) {
        entity_->brush(static_cast<i64>(i * 10), world_, *livingEntity_, Direction::Up, brushStack);
    }

    // 应触发 BRUSH_BLOCK_COMPLETE (3008) 事件
    EXPECT_TRUE(world_.hasEvent(static_cast<i32>(world::WorldEvents::BRUSH_BLOCK_COMPLETE)));
}

TEST_F(BrushableBrushLogicTest, Brush_WithEmptyLootTable_UnpacksToEmptyItem)
{
    // 注册空战利品表
    registerEmptyLootTable(world_.mutableLootTableManager(), "minecraft:archaeology/test_empty");

    entity_ = std::make_unique<BrushableBlockEntity>(BlockPos(10, 64, 20));
    entity_->setWorld(&world_);
    entity_->setLootTable(ResourceLocation("minecraft", "archaeology/test_empty"), 1LL);

    ItemStack brushStack(m_brushItem, 1);
    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);

    // 空战利品表生成 0 个物品，m_item 应为空
    EXPECT_TRUE(entity_->getItem().isEmpty());
    EXPECT_EQ(entity_->getBrushCount(), 1); // brushCount 仍递增
}

TEST_F(BrushableBrushLogicTest, Brush_WithNullLootTableManager_DoesNotCrash)
{
    // 模拟客户端场景：lootTableManager 返回 nullptr
    world_.setNoLootTableManager();

    entity_ = std::make_unique<BrushableBlockEntity>(BlockPos(10, 64, 20));
    entity_->setWorld(&world_);
    entity_->setLootTable(ResourceLocation("minecraft", "archaeology/test_null"), 1LL);

    ItemStack brushStack(m_brushItem, 1);
    // 不应崩溃，item 保持为空
    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);

    EXPECT_TRUE(entity_->getItem().isEmpty());
    // 注意：m_hasLootTable 在 unpackLootTable 中 table==nullptr 时被清除
    // brushCount 仍递增
    EXPECT_EQ(entity_->getBrushCount(), 1);
}

TEST_F(BrushableBrushLogicTest, Brush_WithNonExistentLootTable_ClearsLootTableFlag)
{
    // 不注册该战利品表，模拟表不存在
    entity_ = std::make_unique<BrushableBlockEntity>(BlockPos(10, 64, 20));
    entity_->setWorld(&world_);
    entity_->setLootTable(ResourceLocation("minecraft", "archaeology/nonexistent"), 1LL);

    ItemStack brushStack(m_brushItem, 1);
    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);

    // 战利品表不存在时，m_hasLootTable 被清除，item 为空
    EXPECT_TRUE(entity_->getItem().isEmpty());
    EXPECT_EQ(entity_->getBrushCount(), 1);
}

TEST_F(BrushableBrushLogicTest, Brush_LootTableOnlyUnpackedOnce)
{
    ItemStack brushStack(m_brushItem, 1);

    // 第一次刷扫：unpack loot table
    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_FALSE(entity_->getItem().isEmpty());
    EXPECT_EQ(entity_->getItem().getCount(), 1);

    // 第二次刷扫：不应再次 unpack（m_hasLootTable 已为 false）
    // item 应保持不变
    entity_->brush(10, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_FALSE(entity_->getItem().isEmpty());
    EXPECT_EQ(entity_->getItem().getCount(), 1);
}

// ============================================================================
// brush() DUSTED 完成度变化测试
// ============================================================================

TEST_F(BrushableBrushLogicTest, Brush_CompletionStateChangesAtCorrectThresholds)
{
    ItemStack brushStack(m_brushItem, 1);

    // brushCount 0 -> completion 0
    EXPECT_EQ(entity_->getCompletionState(), 0);

    // brushCount 1 -> completion 1
    entity_->brush(0, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 1);
    EXPECT_EQ(entity_->getCompletionState(), 1);

    // brushCount 2 -> completion 1
    entity_->brush(10, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 2);
    EXPECT_EQ(entity_->getCompletionState(), 1);

    // brushCount 3 -> completion 2
    entity_->brush(20, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 3);
    EXPECT_EQ(entity_->getCompletionState(), 2);

    // brushCount 5 -> completion 2
    entity_->brush(30, world_, *livingEntity_, Direction::Up, brushStack);
    entity_->brush(40, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 5);
    EXPECT_EQ(entity_->getCompletionState(), 2);

    // brushCount 6 -> completion 3
    entity_->brush(50, world_, *livingEntity_, Direction::Up, brushStack);
    EXPECT_EQ(entity_->getBrushCount(), 6);
    EXPECT_EQ(entity_->getCompletionState(), 3);
}

// ============================================================================
// checkReset() 测试
// ============================================================================

class BrushableCheckResetTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_brushItem = ensureTestItem("brush");

        registerSimpleLootTable(
            world_.mutableLootTableManager(), "minecraft:archaeology/test_reset", "minecraft:diamond", 1);

        entity_ = std::make_unique<BrushableBlockEntity>(BlockPos(10, 64, 20));
        entity_->setWorld(&world_);
        entity_->setLootTable(ResourceLocation("minecraft", "archaeology/test_reset"), 42LL);

        livingEntity_ = std::make_unique<LivingEntity>(EntityInstanceId(1), &world_);
    }

    // 辅助：将 brushCount 增加到指定值（每次间隔 10 tick 避免冷却）
    void brushTo(i32 targetCount)
    {
        ItemStack brushStack(m_brushItem, 1);
        for (i32 i = 0; i < targetCount; ++i) {
            entity_->brush(static_cast<i64>(i * 10), world_, *livingEntity_, Direction::Up, brushStack);
        }
        EXPECT_EQ(entity_->getBrushCount(), targetCount);
    }

    BrushableTestWorld world_;
    std::unique_ptr<BrushableBlockEntity> entity_;
    std::unique_ptr<LivingEntity> livingEntity_;
    Item* m_brushItem = nullptr;
};

TEST_F(BrushableCheckResetTest, CheckReset_BeforeResetTick_DoesNotDecrement)
{
    brushTo(4);

    // gameTime=0 刷扫 4 次后，brushCountResetsAtTick = (最后一次brush的gameTime) + 40
    // 最后一次 brush gameTime = 30, resetsAt = 70
    world_.setGameTime(69); // < 70
    entity_->checkReset(world_);

    EXPECT_EQ(entity_->getBrushCount(), 4); // 未递减
}

TEST_F(BrushableCheckResetTest, CheckReset_AtResetTick_DecrementsByTwo)
{
    brushTo(4);

    // resetsAt = 70
    world_.setGameTime(70);
    entity_->checkReset(world_);

    // brushCount = max(0, 4 - 2) = 2
    EXPECT_EQ(entity_->getBrushCount(), 2);
}

TEST_F(BrushableCheckResetTest, CheckReset_DecrementDoesNotGoNegative)
{
    brushTo(1); // brushCount = 1

    // resetsAt = 40
    world_.setGameTime(40);
    entity_->checkReset(world_);

    // brushCount = max(0, 1 - 2) = 0
    EXPECT_EQ(entity_->getBrushCount(), 0);
}

TEST_F(BrushableCheckResetTest, CheckReset_CompleteResetClearsHitDirection)
{
    brushTo(1);
    EXPECT_TRUE(entity_->getHitDirection().has_value());

    // 触发重置到 0
    world_.setGameTime(40);
    entity_->checkReset(world_);

    EXPECT_EQ(entity_->getBrushCount(), 0);
    // brushCount == 0 时应清空 hitDirection
    EXPECT_FALSE(entity_->getHitDirection().has_value());
}

TEST_F(BrushableCheckResetTest, CheckReset_CompleteResetClearsTimers)
{
    brushTo(1);

    world_.setGameTime(40);
    entity_->checkReset(world_);

    EXPECT_EQ(entity_->getBrushCount(), 0);
    // 完全重置后，再次 checkReset 不应递减（brushCount 已为 0）
    // 且不应崩溃
    world_.setGameTime(100);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), 0);
}

TEST_F(BrushableCheckResetTest, CheckReset_PartialDecrement_UpdatesResetTick)
{
    brushTo(6); // brushCount = 6, resetsAt = 50 + 40 = 90...
    // 实际：最后一次 brush gameTime = 50, resetsAt = 90

    // 第一次 checkReset 在 gameTime=90
    world_.setGameTime(90);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), 4); // 6 - 2 = 4

    // resetsAt 应更新为 90 + 4 = 94
    // 在 gameTime=93（< 94）不应再次递减
    world_.setGameTime(93);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), 4); // 未递减

    // 在 gameTime=94（>= 94）应再次递减
    world_.setGameTime(94);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), 2); // 4 - 2 = 2
}

TEST_F(BrushableCheckResetTest, CheckReset_WithZeroBrushCount_DoesNothing)
{
    // brushCount 已为 0，checkReset 不应崩溃或改变状态
    EXPECT_EQ(entity_->getBrushCount(), 0);

    world_.setGameTime(100);
    entity_->checkReset(world_);

    EXPECT_EQ(entity_->getBrushCount(), 0);
    EXPECT_FALSE(entity_->getHitDirection().has_value());
}

TEST_F(BrushableCheckResetTest, CheckReset_DecrementUpdatesCompletionState)
{
    brushTo(6); // completion = 3 (brushCount >= 6)
    EXPECT_EQ(entity_->getCompletionState(), 3);

    // 递减到 4 -> completion = 2 (brushCount < 6)
    world_.setGameTime(90);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), 4);
    EXPECT_EQ(entity_->getCompletionState(), 2);

    // 递减到 2 -> completion = 1 (brushCount < 3)
    world_.setGameTime(94);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), 2);
    EXPECT_EQ(entity_->getCompletionState(), 1);

    // 递减到 0 -> completion = 0
    world_.setGameTime(98);
    entity_->checkReset(world_);
    EXPECT_EQ(entity_->getBrushCount(), 0);
    EXPECT_EQ(entity_->getCompletionState(), 0);
}

// ============================================================================
// 序列化测试 - JSON（区块存档）
// ============================================================================

class BrushableSerializationTest : public BrushableBlockEntityTest {};

TEST_F(BrushableSerializationTest, SaveLoad_LootTable_Preserved)
{
    const ResourceLocation tableId("minecraft", "archaeology/test_serialize");
    entity_->setLootTable(tableId, 99999LL);

    nlohmann::json data;
    entity_->save(data);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(10, 20, 30));
    EXPECT_TRUE(loaded->load(data));

    // 验证 lootTable 通过 save/load 保留
    // （通过再次 save 检查 LootTable 字段存在）
    nlohmann::json data2;
    loaded->save(data2);
    EXPECT_TRUE(data2.contains("LootTable"));
    EXPECT_EQ(data2["LootTable"].get<std::string>(), "minecraft:archaeology/test_serialize");
    EXPECT_EQ(data2["LootTableSeed"].get<i64>(), 99999LL);
}

TEST_F(BrushableSerializationTest, SaveLoad_Item_Preserved)
{
    // 设置 item（通过 load 设置）
    nlohmann::json itemData;
    itemData["item"] = ItemStack(m_testItem, 7).toJson();
    entity_->load(itemData);

    nlohmann::json data;
    entity_->save(data);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->load(data));

    EXPECT_FALSE(loaded->getItem().isEmpty());
    EXPECT_EQ(loaded->getItem().getCount(), 7);
}

TEST_F(BrushableSerializationTest, SaveLoad_HitDirection_Preserved)
{
    // 通过 brush 设置 hitDirection
    // 但 brush 需要 world，这里通过 NBT round-trip 设置
    nlohmann::json data;
    data["hit_direction"] = static_cast<i32>(Direction::East);
    entity_->load(data);

    EXPECT_TRUE(entity_->getHitDirection().has_value());
    EXPECT_EQ(entity_->getHitDirection().value(), Direction::East);

    nlohmann::json saved;
    entity_->save(saved);
    EXPECT_TRUE(saved.contains("hit_direction"));
    EXPECT_EQ(saved["hit_direction"].get<i32>(), static_cast<i32>(Direction::East));
}

TEST_F(BrushableSerializationTest, SaveLoad_BrushCount_Preserved)
{
    nlohmann::json data;
    data["brush_count"] = 5;
    entity_->load(data);

    EXPECT_EQ(entity_->getBrushCount(), 5);

    nlohmann::json saved;
    entity_->save(saved);
    EXPECT_TRUE(saved.contains("brush_count"));
    EXPECT_EQ(saved["brush_count"].get<i32>(), 5);
}

TEST_F(BrushableSerializationTest, SaveLoad_RuntimeTimers_Preserved)
{
    nlohmann::json data;
    data["brush_count_resets_at_tick"] = 1000LL;
    data["cooldown_ends_at_tick"] = 500LL;
    entity_->load(data);

    nlohmann::json saved;
    entity_->save(saved);
    EXPECT_EQ(saved["brush_count_resets_at_tick"].get<i64>(), 1000LL);
    EXPECT_EQ(saved["cooldown_ends_at_tick"].get<i64>(), 500LL);
}

TEST_F(BrushableSerializationTest, SaveLoad_EmptyEntity_NoCrash)
{
    nlohmann::json data;
    entity_->save(data);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getBrushCount(), 0);
    EXPECT_TRUE(loaded->getItem().isEmpty());
    EXPECT_FALSE(loaded->getHitDirection().has_value());
}

TEST_F(BrushableSerializationTest, SaveLoad_InvalidHitDirection_Ignored)
{
    nlohmann::json data;
    data["hit_direction"] = 99; // 无效值
    entity_->load(data);

    EXPECT_FALSE(entity_->getHitDirection().has_value());
}

TEST_F(BrushableSerializationTest, SaveLoad_LootTableTakesPrecedenceOverItem)
{
    // 当同时有 LootTable 和 item 时，load 应优先 LootTable（item 被清空）
    nlohmann::json data;
    data["LootTable"] = "minecraft:archaeology/test";
    data["LootTableSeed"] = 1LL;
    data["item"] = ItemStack(m_testItem, 5).toJson();
    entity_->load(data);

    // LootTable 存在时，item 应为空
    EXPECT_TRUE(entity_->getItem().isEmpty());
}

// ============================================================================
// 序列化测试 - NBT（结构模板 / 客户端同步）
// ============================================================================

TEST_F(BrushableBlockEntityTest, NbtSaveLoad_LootTable_Preserved)
{
    const ResourceLocation tableId("minecraft", "archaeology/test_nbt");
    entity_->setLootTable(tableId, 77777LL);

    nbt::CompoundTag tag;
    entity_->saveToNBT(tag);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(10, 20, 30));
    EXPECT_TRUE(loaded->loadFromNBT(tag));

    // 验证 lootTable 通过 NBT round-trip 保留
    nbt::CompoundTag tag2;
    loaded->saveToNBT(tag2);

    // 检查 LootTable 键存在
    namespace nbt_helper = mc::entity::serialization::nbt_helper;
    auto lootOpt = nbt_helper::tryGetString(tag2, BrushableBlockEntity::LOOT_TABLE_TAG);
    ASSERT_TRUE(lootOpt.has_value());
    EXPECT_EQ(lootOpt.value(), "minecraft:archaeology/test_nbt");

    auto seedOpt = nbt_helper::tryGetLong(tag2, BrushableBlockEntity::LOOT_TABLE_SEED_TAG);
    ASSERT_TRUE(seedOpt.has_value());
    EXPECT_EQ(seedOpt.value(), 77777LL);
}

TEST_F(BrushableBlockEntityTest, NbtSaveLoad_Item_Preserved)
{
    nlohmann::json itemData;
    itemData["item"] = ItemStack(m_testItem, 3).toJson();
    entity_->load(itemData);

    nbt::CompoundTag tag;
    entity_->saveToNBT(tag);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_FALSE(loaded->getItem().isEmpty());
    EXPECT_EQ(loaded->getItem().getCount(), 3);
}

TEST_F(BrushableBlockEntityTest, NbtSaveLoad_HitDirection_Preserved)
{
    nlohmann::json data;
    data["hit_direction"] = static_cast<i32>(Direction::West);
    entity_->load(data);

    nbt::CompoundTag tag;
    entity_->saveToNBT(tag);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_TRUE(loaded->getHitDirection().has_value());
    EXPECT_EQ(loaded->getHitDirection().value(), Direction::West);
}

TEST_F(BrushableBlockEntityTest, NbtSaveLoad_BrushCount_Preserved)
{
    nlohmann::json data;
    data["brush_count"] = 8;
    entity_->load(data);

    nbt::CompoundTag tag;
    entity_->saveToNBT(tag);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_EQ(loaded->getBrushCount(), 8);
}

TEST_F(BrushableBlockEntityTest, NbtSaveLoad_RuntimeTimers_Preserved)
{
    nlohmann::json data;
    data["brush_count_resets_at_tick"] = 2000LL;
    data["cooldown_ends_at_tick"] = 1500LL;
    entity_->load(data);

    nbt::CompoundTag tag;
    entity_->saveToNBT(tag);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->loadFromNBT(tag));

    nbt::CompoundTag tag2;
    loaded->saveToNBT(tag2);

    namespace nbt_helper = mc::entity::serialization::nbt_helper;
    EXPECT_EQ(nbt_helper::tryGetLong(tag2, "brush_count_resets_at_tick").value_or(0), 2000LL);
    EXPECT_EQ(nbt_helper::tryGetLong(tag2, "cooldown_ends_at_tick").value_or(0), 1500LL);
}

TEST_F(BrushableBlockEntityTest, NbtSaveLoad_EmptyEntity_NoCrash)
{
    nbt::CompoundTag tag;
    entity_->saveToNBT(tag);

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_EQ(loaded->getBrushCount(), 0);
    EXPECT_TRUE(loaded->getItem().isEmpty());
}

TEST_F(BrushableBlockEntityTest, NbtSaveLoad_InvalidHitDirection_Ignored)
{
    // 直接构造一个带有无效 hit_direction 的 NBT
    nbt::CompoundTag tag;
    tag.put(BrushableBlockEntity::HIT_DIRECTION_TAG, static_cast<i8>(99)); // 无效

    auto loaded = std::make_unique<BrushableBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_FALSE(loaded->getHitDirection().has_value());
}

// ============================================================================
// 克隆测试
// ============================================================================

TEST_F(BrushableBlockEntityTest, Clone_CreatesDeepCopy)
{
    // 设置状态
    entity_->setLootTable(ResourceLocation("minecraft", "archaeology/test_clone"), 555LL);
    nlohmann::json data;
    data["brush_count"] = 3;
    data["hit_direction"] = static_cast<i32>(Direction::South);
    entity_->load(data);

    std::unique_ptr<BlockEntity> copy = entity_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::BrushableBlock);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));

    auto* brushableCopy = static_cast<BrushableBlockEntity*>(copy.get());
    EXPECT_EQ(brushableCopy->getBrushCount(), 3);
    EXPECT_TRUE(brushableCopy->getHitDirection().has_value());
    EXPECT_EQ(brushableCopy->getHitDirection().value(), Direction::South);
}

TEST_F(BrushableBlockEntityTest, Clone_IsDeepCopy)
{
    // 设置 item
    nlohmann::json data;
    data["item"] = ItemStack(m_testItem, 5).toJson();
    entity_->load(data);

    std::unique_ptr<BlockEntity> copy = entity_->clone();
    auto* brushableCopy = static_cast<BrushableBlockEntity*>(copy.get());

    // 修改原始不影响克隆
    nlohmann::json data2;
    data2["item"] = ItemStack(m_testItem, 10).toJson();
    entity_->load(data2);

    // 克隆应保持原始的 count=5
    EXPECT_EQ(brushableCopy->getItem().getCount(), 5);
}

TEST_F(BrushableBlockEntityTest, Clone_EmptyEntity_NoCrash)
{
    std::unique_ptr<BlockEntity> copy = entity_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::BrushableBlock);

    auto* brushableCopy = static_cast<BrushableBlockEntity*>(copy.get());
    EXPECT_EQ(brushableCopy->getBrushCount(), 0);
    EXPECT_TRUE(brushableCopy->getItem().isEmpty());
}

// ============================================================================
// setChanged 测试
// ============================================================================

TEST_F(BrushableBlockEntityTest, SetChanged_MarksAsChanged)
{
    EXPECT_FALSE(entity_->isChanged());
    entity_->setChanged();
    EXPECT_TRUE(entity_->isChanged());
}
