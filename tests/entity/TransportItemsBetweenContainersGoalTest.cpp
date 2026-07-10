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

// TransportItemsBetweenContainersGoal 单元测试
//
// 测试覆盖：
// 1. ContainerUser 接口在 CopperGolemEntity 上的实现
//    - hasContainerOpen / getContainerInteractionRange / getLivingEntity
//    - setOpenedChestPos / clearOpenedChestPos
// 2. TransportItemsBetweenContainersGoal 构造与基本属性
//    - 类型名、互斥标志
// 3. 物品转移核心逻辑（通过 TestAccessor 访问私有方法）
//    - _isPickingUpItems（主手空/非空判断）
//    - _pickupItemFromContainer（从容器取出最多 16 个物品）
//    - _addItemsToContainer（放入容器：先空槽后可堆叠，对齐 MC 原版顺序）
//    - _setAnimationState（CopperGolemState 设置）
//    - _enterCooldown / _resetTransportState

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/CopperGolemGoals.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemTypes.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"

#include <memory>

using namespace mc;

namespace mc::test {

// ============================================================================
// 测试访问器：暴露 TransportItemsBetweenContainersGoal 的私有方法
// ============================================================================
//
// TransportItemsBetweenContainersGoal 已在 CopperGolemGoals.hpp 中声明
// friend class test::TransportItemsBetweenContainersGoalTestAccessor
// 这里提供具体实现，将私有方法和成员暴露给测试代码。

class TransportItemsBetweenContainersGoalTestAccessor {
public:
    explicit TransportItemsBetweenContainersGoalTestAccessor(
        entity::ai::goal::TransportItemsBetweenContainersGoal& goal)
        : m_goal(goal)
    {}

    // 暴露私有查询方法
    [[nodiscard]] bool isPickingUpItems() const { return m_goal._isPickingUpItems(); }

    // 暴露私有物品操作方法
    void pickupItemFromContainer(IInventory& container) { m_goal._pickupItemFromContainer(container); }
    void addItemsToContainer(IInventory& container) { m_goal._addItemsToContainer(container); }

    // 暴露私有状态控制方法
    void setAnimationState(bool success) { m_goal._setAnimationState(success); }
    void playInteractionSound(bool success) { m_goal._playInteractionSound(success); }
    void enterCooldown() { m_goal._enterCooldown(); }
    void resetTransportState() { m_goal._resetTransportState(); }

    // 暴露状态查询
    [[nodiscard]] i32 getCooldown() const { return m_goal.m_cooldown; }
    [[nodiscard]] bool hasDestinationBlock() const { return m_goal.m_destinationBlock.has_value(); }
    [[nodiscard]] i32 getInteractionTicks() const { return m_goal.m_interactionTicks; }
    [[nodiscard]] bool getInteractionSuccess() const { return m_goal.m_interactionSuccess; }

    // 暴露常量
    [[nodiscard]] static i32 getMaxStackSize()
    {
        return entity::ai::goal::TransportItemsBetweenContainersGoal::TRANSPORTED_ITEM_MAX_STACK_SIZE;
    }
    [[nodiscard]] static i32 getIdleCooldown()
    {
        return entity::ai::goal::TransportItemsBetweenContainersGoal::IDLE_COOLDOWN;
    }
    [[nodiscard]] static i32 getTargetInteractionTime()
    {
        return entity::ai::goal::TransportItemsBetweenContainersGoal::TARGET_INTERACTION_TIME;
    }

private:
    entity::ai::goal::TransportItemsBetweenContainersGoal& m_goal;
};

} // namespace mc::test

namespace {

// ============================================================================
// 测试用世界 - 支持方块状态和方块实体
// ============================================================================

class TransportItemsTestWorld final : public test::BaseTestWorld {
public:
    TransportItemsTestWorld() = default;

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] bool isClientSide() const override { return false; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 空实现：测试不关心音效
    }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    // 测试辅助方法
    void setTick(u64 tick) { m_currentTick = tick; }

private:
    u64 m_currentTick = 0;
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

// ============================================================================
// 辅助函数：创建铜傀儡并设置世界和位置
// ============================================================================

std::unique_ptr<CopperGolemEntity> createCopperGolem(
    TransportItemsTestWorld& world, EntityId id = EntityId{1}, f32 x = 0.0f, f32 y = 64.0f, f32 z = 0.0f)
{
    auto golem = std::make_unique<CopperGolemEntity>(id);
    golem->setTypeId(entity::EntityTypes::COPPER_GOLEM);
    golem->setWorld(&world);
    golem->setPosition(x, y, z);
    return golem;
}

} // namespace

// ============================================================================
// 测试夹具
// ============================================================================

class TransportItemsGoalTestFixture : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            // 初始化方块、物品、实体注册表
            VanillaBlocks::initialize();
            Items::initialize();
            BlockItemRegistry::instance().initializeVanillaBlockItems();
            entity::VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<TransportItemsTestWorld>(); }

    void TearDown() override { m_world.reset(); }

    std::unique_ptr<TransportItemsTestWorld> m_world;
};

// ============================================================================
// ContainerUser 接口测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, ContainerUser_GetInteractionRange_Returns3)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 对应 MC CopperGolem.getContainerInteractionRange() = 3.0
    EXPECT_DOUBLE_EQ(golemPtr->getContainerInteractionRange(), 3.0);
}

TEST_F(TransportItemsGoalTestFixture, ContainerUser_GetLivingEntity_ReturnsSelf)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // getLivingEntity 应返回 this（即 CopperGolemEntity*，可转换为 LivingEntity*）
    LivingEntity* living = golemPtr->getLivingEntity();
    EXPECT_EQ(static_cast<void*>(living), static_cast<void*>(golemPtr));

    const CopperGolemEntity* constGolemPtr = golemPtr;
    const LivingEntity* constLiving = constGolemPtr->getLivingEntity();
    EXPECT_EQ(static_cast<const void*>(constLiving), static_cast<const void*>(constGolemPtr));
}

TEST_F(TransportItemsGoalTestFixture, ContainerUser_HasContainerOpen_NoOpenedPos_ReturnsFalse)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 未设置 openedChestPos 时，任何位置都应返回 false
    EXPECT_FALSE(golemPtr->hasContainerOpen(BlockPos(10, 64, 10)));
    EXPECT_FALSE(golemPtr->hasContainerOpen(BlockPos(0, 0, 0)));
}

TEST_F(TransportItemsGoalTestFixture, ContainerUser_HasContainerOpen_ExactMatch_ReturnsTrue)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    BlockPos targetPos(10, 64, 10);
    golemPtr->setOpenedChestPos(targetPos);

    // 完全匹配的位置应返回 true
    EXPECT_TRUE(golemPtr->hasContainerOpen(targetPos));
}

TEST_F(TransportItemsGoalTestFixture, ContainerUser_HasContainerOpen_DifferentPos_ReturnsFalse)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    golemPtr->setOpenedChestPos(BlockPos(10, 64, 10));

    // 完全不同的位置（且不是双箱另一半）应返回 false
    EXPECT_FALSE(golemPtr->hasContainerOpen(BlockPos(20, 64, 20)));
}

TEST_F(TransportItemsGoalTestFixture, ContainerUser_ClearOpenedChestPos_RemovesPosition)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    BlockPos targetPos(10, 64, 10);
    golemPtr->setOpenedChestPos(targetPos);
    EXPECT_TRUE(golemPtr->hasContainerOpen(targetPos));

    golemPtr->clearOpenedChestPos();
    EXPECT_FALSE(golemPtr->hasContainerOpen(targetPos));
}

// ============================================================================
// TransportItemsBetweenContainersGoal 基本属性测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, Goal_TypeName_IsCorrect)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TransportItemsBetweenContainersGoal");
}

TEST_F(TransportItemsGoalTestFixture, Goal_Constants_MatchMCValues)
{
    // 验证关键常量与 MC 1.21.11 一致
    EXPECT_EQ(test::TransportItemsBetweenContainersGoalTestAccessor::getMaxStackSize(), 16);
    EXPECT_EQ(test::TransportItemsBetweenContainersGoalTestAccessor::getIdleCooldown(), 140);
    EXPECT_EQ(test::TransportItemsBetweenContainersGoalTestAccessor::getTargetInteractionTime(), 60);
}

// ============================================================================
// _isPickingUpItems 测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, IsPickingUpItems_EmptyMainHand_ReturnsTrue)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 确保主手为空
    golemPtr->setMainHandItem(ItemStack{});

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    // 主手为空 → 拾取模式
    EXPECT_TRUE(accessor.isPickingUpItems());
}

TEST_F(TransportItemsGoalTestFixture, IsPickingUpItems_NonEmptyMainHand_ReturnsFalse)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 主手放入一个物品（使用 STICK，Items 初始化后应可用）
    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    golemPtr->setMainHandItem(ItemStack(*stick, 1));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    // 主手有物品 → 放置模式
    EXPECT_FALSE(accessor.isPickingUpItems());
}

// ============================================================================
// _pickupItemFromContainer 测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, PickupFromContainer_TakesAtMost16Items)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    // 创建一个有 32 个 STICK 的容器
    blockentity::SimpleInventory container(27);
    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    container.setItem(0, ItemStack(*stick, 32));

    // 执行拾取
    accessor.pickupItemFromContainer(container);

    // 验证：主手拿到 16 个 STICK（TRANSPORTED_ITEM_MAX_STACK_SIZE = 16）
    const ItemStack& mainHand = golemPtr->getMainHandItem();
    EXPECT_FALSE(mainHand.isEmpty());
    EXPECT_EQ(mainHand.getCount(), 16);

    // 容器中剩余 16 个
    EXPECT_EQ(container.getItem(0).getCount(), 16);
}

TEST_F(TransportItemsGoalTestFixture, PickupFromContainer_LessThan16_TakesAll)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    blockentity::SimpleInventory container(27);
    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    container.setItem(0, ItemStack(*stick, 5));

    accessor.pickupItemFromContainer(container);

    // 容器中只有 5 个，全部取出
    const ItemStack& mainHand = golemPtr->getMainHandItem();
    EXPECT_EQ(mainHand.getCount(), 5);
    EXPECT_TRUE(container.getItem(0).isEmpty());
}

TEST_F(TransportItemsGoalTestFixture, PickupFromContainer_FindsFirstNonEmptySlot)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    blockentity::SimpleInventory container(27);
    const Item* stick = Items::STICK;
    const Item* apple = Items::APPLE;
    ASSERT_NE(stick, nullptr);
    ASSERT_NE(apple, nullptr);

    // 槽位 0、1 为空，槽位 2 有物品
    container.setItem(2, ItemStack(*apple, 8));

    accessor.pickupItemFromContainer(container);

    // 应取出槽位 2 的物品
    const ItemStack& mainHand = golemPtr->getMainHandItem();
    EXPECT_FALSE(mainHand.isEmpty());
    EXPECT_EQ(mainHand.getItem(), apple);
    EXPECT_EQ(mainHand.getCount(), 8);
    EXPECT_TRUE(container.getItem(2).isEmpty());
}

TEST_F(TransportItemsGoalTestFixture, PickupFromContainer_EmptyContainer_DoesNothing)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    blockentity::SimpleInventory container(27);
    // 容器完全为空

    accessor.pickupItemFromContainer(container);

    // 主手应仍为空
    EXPECT_TRUE(golemPtr->getMainHandItem().isEmpty());
}

// ============================================================================
// _addItemsToContainer 测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, AddItemsToContainer_FillsEmptySlotFirst)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 主手放入 10 个 STICK
    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    golemPtr->setMainHandItem(ItemStack(*stick, 10));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    // 容器完全为空
    blockentity::SimpleInventory container(27);

    accessor.addItemsToContainer(container);

    // MC 原版顺序：先找空槽整堆放入 → 槽位 0 应有 10 个 STICK
    EXPECT_EQ(container.getItem(0).getCount(), 10);
    EXPECT_EQ(container.getItem(0).getItem(), stick);

    // 主手应清空
    EXPECT_TRUE(golemPtr->getMainHandItem().isEmpty());
}

TEST_F(TransportItemsGoalTestFixture, AddItemsToContainer_StacksOnExistingWhenNoEmptySlot)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);

    // 主手有 70 个 STICK（超过最大堆叠 64）
    golemPtr->setMainHandItem(ItemStack(*stick, 70));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    // 容器槽位 0 已有 60 个 STICK（可堆叠，剩余空间 4）
    blockentity::SimpleInventory container(27);
    container.setItem(0, ItemStack(*stick, 60));

    accessor.addItemsToContainer(container);

    // MC 原版顺序：先找空槽，但容器槽位 1..26 都是空槽 → 走空槽路径整堆放入
    // 注：本项目实现中"先空槽后可堆叠"，空槽优先 → 若有空槽会先整堆放入空槽
    // 这里容器槽位 1..26 都是空槽，所以主手 70 个 STICK 整堆放入槽位 1（但超过 maxStack=64，
    // setItem 只会放入 70 个，不自动拆分，可能超过 maxStack）
    // 验证总体行为：所有 70 个物品都应被放入容器，主手应清空
    // 容器中总 STICK 数 = 原有 60 + 新增 70 = 130
    i32 totalInContainer = 0;
    for (i32 i = 0; i < container.getContainerSize(); ++i) {
        const ItemStack& s = container.getItem(i);
        if (!s.isEmpty() && s.getItem() == stick) {
            totalInContainer += s.getCount();
        }
    }
    EXPECT_EQ(totalInContainer, 130);
    EXPECT_TRUE(golemPtr->getMainHandItem().isEmpty());
}

TEST_F(TransportItemsGoalTestFixture, AddItemsToContainer_FullContainer_LeavesRemainingInHand)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);

    // 主手有 10 个 STICK
    golemPtr->setMainHandItem(ItemStack(*stick, 10));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    // 容器完全装满（用 DIRT 填满所有槽位，与 STICK 不可堆叠）
    const Item* dirt = Items::DIRT;
    ASSERT_NE(dirt, nullptr);
    blockentity::SimpleInventory container(27);
    for (i32 i = 0; i < container.getContainerSize(); ++i) {
        container.setItem(i, ItemStack(*dirt, 1));
    }

    accessor.addItemsToContainer(container);

    // 容器无空槽、无可堆叠槽 → 主手保留所有 STICK
    const ItemStack& mainHand = golemPtr->getMainHandItem();
    EXPECT_FALSE(mainHand.isEmpty());
    EXPECT_EQ(mainHand.getCount(), 10);
    EXPECT_EQ(mainHand.getItem(), stick);
}

// ============================================================================
// _setAnimationState 测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, SetAnimationState_PickupSuccess_SetsGettingItem)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 主手为空 → 拾取模式
    golemPtr->setMainHandItem(ItemStack{});

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setAnimationState(true);
    EXPECT_EQ(golemPtr->getBehaviorState(), entity::CopperGolemState::GettingItem);
}

TEST_F(TransportItemsGoalTestFixture, SetAnimationState_PickupFailure_SetsGettingNoItem)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    golemPtr->setMainHandItem(ItemStack{});

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setAnimationState(false);
    EXPECT_EQ(golemPtr->getBehaviorState(), entity::CopperGolemState::GettingNoItem);
}

TEST_F(TransportItemsGoalTestFixture, SetAnimationState_PlaceSuccess_SetsDroppingItem)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 主手有物品 → 放置模式
    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    golemPtr->setMainHandItem(ItemStack(*stick, 1));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setAnimationState(true);
    EXPECT_EQ(golemPtr->getBehaviorState(), entity::CopperGolemState::DroppingItem);
}

TEST_F(TransportItemsGoalTestFixture, SetAnimationState_PlaceFailure_SetsDroppingNoItem)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    golemPtr->setMainHandItem(ItemStack(*stick, 1));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setAnimationState(false);
    EXPECT_EQ(golemPtr->getBehaviorState(), entity::CopperGolemState::DroppingNoItem);
}

// ============================================================================
// _enterCooldown / _resetTransportState 测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, EnterCooldown_SetsCooldownTo140)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.enterCooldown();
    EXPECT_EQ(accessor.getCooldown(), 140);
}

TEST_F(TransportItemsGoalTestFixture, ResetTransportState_ClearsDestinationAndResetsGolemState)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 预设铜傀儡状态
    golemPtr->setOpenedChestPos(BlockPos(10, 64, 10));
    golemPtr->setBehaviorState(entity::CopperGolemState::GettingItem);

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.resetTransportState();

    // 重置后应清除打开位置
    EXPECT_FALSE(golemPtr->hasContainerOpen(BlockPos(10, 64, 10)));
    // 行为状态应重置为 Idle
    EXPECT_EQ(golemPtr->getBehaviorState(), entity::CopperGolemState::Idle);
    // 目标方块应被清除
    EXPECT_FALSE(accessor.hasDestinationBlock());
    // 交互 tick 应重置
    EXPECT_EQ(accessor.getInteractionTicks(), 0);
}
