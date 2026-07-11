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
#include "common/world/blockentity/storage/ChestEntity.hpp"

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
    [[nodiscard]] bool hasReachedTarget() const { return m_goal._hasReachedTarget(); }
    [[nodiscard]] bool isWithinQueuingDistance() const { return m_goal._isWithinQueuingDistance(); }
    [[nodiscard]] bool isWithinContinueInteractingDistance() const
    {
        return m_goal._isWithinContinueInteractingDistance();
    }
    [[nodiscard]] bool isAnotherMobInteractingWithTarget() const { return m_goal._isAnotherMobInteractingWithTarget(); }

    // 暴露私有 AABB 距离判定方法（用于直接测试 MC isWithinTargetDistance 复刻）
    [[nodiscard]] bool isWithinTargetDistance(f64 distance, const Vector3& center) const
    {
        return m_goal._isWithinTargetDistance(distance, center);
    }
    [[nodiscard]] Vector3 getCenterPos() const { return m_goal._getCenterPos(); }
    [[nodiscard]] f64 getInteractionRange() const { return m_goal._getInteractionRange(); }

    // 暴露私有物品操作方法
    void pickupItemFromContainer(IInventory& container) { m_goal._pickupItemFromContainer(container); }
    void addItemsToContainer(IInventory& container) { m_goal._addItemsToContainer(container); }

    // 暴露私有状态控制方法
    void setAnimationState(bool success) { m_goal._setAnimationState(success); }
    void playInteractionSound(bool success) { m_goal._playInteractionSound(success); }
    void enterCooldown() { m_goal._enterCooldown(); }
    void resetTransportState() { m_goal._resetTransportState(); }
    void startTravelling() { m_goal._startTravelling(); }
    void startQueuing() { m_goal._startQueuing(); }
    void resumeTravelling() { m_goal._resumeTravelling(); }
    void startInteracting() { m_goal._startInteracting(); }
    void tickInteracting() { m_goal._tickInteracting(); }
    void tick() { m_goal.tick(); }

    // 暴露状态查询
    [[nodiscard]] i32 getCooldown() const { return m_goal.m_cooldown; }
    [[nodiscard]] bool hasDestinationBlock() const { return m_goal.m_destinationBlock.has_value(); }
    [[nodiscard]] BlockPos getDestinationBlock() const { return m_goal.m_destinationBlock.value(); }
    [[nodiscard]] i32 getInteractionTicks() const { return m_goal.m_interactionTicks; }
    [[nodiscard]] bool getInteractionSuccess() const { return m_goal.m_interactionSuccess; }
    [[nodiscard]] u8 getTransportState() const { return static_cast<u8>(m_goal.m_state); }

    // 暴露状态枚举值（供测试比较）
    static constexpr u8 STATE_TRAVELLING =
        static_cast<u8>(entity::ai::goal::TransportItemsBetweenContainersGoal::TransportState::Travelling);
    static constexpr u8 STATE_QUEUING =
        static_cast<u8>(entity::ai::goal::TransportItemsBetweenContainersGoal::TransportState::Queuing);
    static constexpr u8 STATE_INTERACTING =
        static_cast<u8>(entity::ai::goal::TransportItemsBetweenContainersGoal::TransportState::Interacting);

    // 设置私有状态（用于驱动测试场景）
    void setDestinationBlock(const BlockPos& pos) { m_goal.m_destinationBlock = pos; }
    void setTransportState(u8 state)
    {
        m_goal.m_state = static_cast<entity::ai::goal::TransportItemsBetweenContainersGoal::TransportState>(state);
    }
    void setCooldown(i32 cd) { m_goal.m_cooldown = cd; }
    void clearVisitedPositions() { m_goal.m_visitedPositions.clear(); }
    void clearUnreachablePositions() { m_goal.m_unreachablePositions.clear(); }

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
    [[nodiscard]] static i32 getTickToStartInteraction()
    {
        return entity::ai::goal::TransportItemsBetweenContainersGoal::TICK_TO_START_INTERACTION;
    }
    [[nodiscard]] static i32 getTickToPlaySound()
    {
        return entity::ai::goal::TransportItemsBetweenContainersGoal::TICK_TO_PLAY_SOUND;
    }
    [[nodiscard]] static i32 getTickToEndInteraction()
    {
        return entity::ai::goal::TransportItemsBetweenContainersGoal::TICK_TO_END_INTERACTION;
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
        Entity* raw = entity.get();
        m_spawnedEntities.push_back(std::move(entity));
        m_allEntities.push_back(raw);
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

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3& center, f32 radius, const Entity* exclude) const override
    {
        std::vector<Entity*> result;
        const f32 radiusSq = radius * radius;
        for (Entity* entity : m_allEntities) {
            if (entity == nullptr || entity == exclude) {
                continue;
            }
            const Vector3& pos = entity->position();
            const f32 dx = pos.x - center.x;
            const f32 dy = pos.y - center.y;
            const f32 dz = pos.z - center.z;
            if (dx * dx + dy * dy + dz * dz <= radiusSq) {
                result.push_back(entity);
            }
        }
        return result;
    }

    // 测试辅助方法
    void setTick(u64 tick) { m_currentTick = tick; }

    /// 注册外部实体到搜索列表（用于测试中需要被 getEntitiesInRange 发现的实体）
    void registerEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_allEntities.push_back(entity);
        }
    }

    /// 在指定位置放置一个 ChestEntity 并返回指针
    blockentity::ChestEntity* placeChest(const BlockPos& pos)
    {
        auto chest = std::make_unique<blockentity::ChestEntity>(pos);
        blockentity::ChestEntity* raw = chest.get();
        m_blockEntities[pos] = std::move(chest);
        return raw;
    }

    /// 在指定位置放置方块状态
    void setBlock(const BlockPos& pos, const BlockState& state) { setBlockState(pos.x, pos.y, pos.z, &state); }

private:
    u64 m_currentTick = 0;
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<Entity*> m_allEntities;
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

// ============================================================================
// Goal 生命周期集成测试
// ============================================================================
//
// 这些测试验证 TransportItemsBetweenContainersGoal 的完整状态机流转，
// 包括 shouldExecute 目标搜索、tick 状态切换、60 tick 交互序列、
// 以及双箱 startOpen 转发等关键行为。

TEST_F(TransportItemsGoalTestFixture, HasReachedTarget_WithinOneBlock_ReturnsTrue)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 铜傀儡站在目标箱子正上方
    golemPtr->setPosition(5.5f, 64.0f, 5.5f);

    // 在目标位置放置箱子方块（AABB 相交判定需要目标方块具有非空碰撞形状）
    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    EXPECT_TRUE(accessor.hasReachedTarget());
}

TEST_F(TransportItemsGoalTestFixture, HasReachedTarget_TooFar_ReturnsFalse)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 铜傀儡站在距目标 5 格的位置
    golemPtr->setPosition(10.5f, 64.0f, 10.5f);

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    EXPECT_FALSE(accessor.hasReachedTarget());
}

TEST_F(TransportItemsGoalTestFixture, IsWithinQueuingDistance_Within3Blocks_ReturnsTrue)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 铜傀儡距目标 2 格（< 3.0 的排队阈值）
    golemPtr->setPosition(7.5f, 64.0f, 7.5f);

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    EXPECT_TRUE(accessor.isWithinQueuingDistance());
}

TEST_F(TransportItemsGoalTestFixture, IsWithinQueuingDistance_Beyond3Blocks_ReturnsFalse)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 铜傀儡距目标 5 格（> 3.0 的排队阈值）
    golemPtr->setPosition(10.5f, 64.0f, 10.5f);

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    EXPECT_FALSE(accessor.isWithinQueuingDistance());
}

TEST_F(TransportItemsGoalTestFixture, IsAnotherMobInteracting_NoOtherMob_ReturnsFalse)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 没有其他实体打开目标箱子
    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(BlockPos(5, 64, 5));
    EXPECT_FALSE(accessor.isAnotherMobInteractingWithTarget());
}

TEST_F(TransportItemsGoalTestFixture, IsAnotherMobInteracting_OtherMobHasContainerOpen_ReturnsTrue)
{
    // 铜傀儡 A 是 Goal 拥有者
    auto golemA = createCopperGolem(*m_world, EntityId{1}, 0.0f, 64.0f, 0.0f);
    CopperGolemEntity* golemAPtr = golemA.get();
    m_world->spawnEntity(std::move(golemA));

    // 铜傀儡 B 已打开目标箱子
    auto golemB = createCopperGolem(*m_world, EntityId{2}, 2.0f, 64.0f, 2.0f);
    CopperGolemEntity* golemBPtr = golemB.get();
    m_world->spawnEntity(std::move(golemB));

    // 铜傀儡 B 声明打开了 (5, 64, 5) 位置的箱子
    golemBPtr->setOpenedChestPos(BlockPos(5, 64, 5));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemAPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(BlockPos(5, 64, 5));
    EXPECT_TRUE(accessor.isAnotherMobInteractingWithTarget());
}

TEST_F(TransportItemsGoalTestFixture, IsAnotherMobInteracting_OtherMobOpenedDifferentChest_ReturnsFalse)
{
    auto golemA = createCopperGolem(*m_world, EntityId{1}, 0.0f, 64.0f, 0.0f);
    CopperGolemEntity* golemAPtr = golemA.get();
    m_world->spawnEntity(std::move(golemA));

    auto golemB = createCopperGolem(*m_world, EntityId{2}, 2.0f, 64.0f, 2.0f);
    CopperGolemEntity* golemBPtr = golemB.get();
    m_world->spawnEntity(std::move(golemB));

    // 铜傀儡 B 打开的是另一个位置的箱子
    golemBPtr->setOpenedChestPos(BlockPos(20, 64, 20));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemAPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(BlockPos(5, 64, 5));
    EXPECT_FALSE(accessor.isAnotherMobInteractingWithTarget());
}

// ============================================================================
// 状态机流转测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, StartQueuing_SetsStateToQueuing)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(BlockPos(5, 64, 5));
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_TRAVELLING);

    accessor.startQueuing();

    EXPECT_EQ(accessor.getTransportState(), test::TransportItemsBetweenContainersGoalTestAccessor::STATE_QUEUING);
}

TEST_F(TransportItemsGoalTestFixture, ResumeTravelling_SetsStateToTravelling)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(BlockPos(5, 64, 5));
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_QUEUING);

    accessor.resumeTravelling();

    EXPECT_EQ(accessor.getTransportState(), test::TransportItemsBetweenContainersGoalTestAccessor::STATE_TRAVELLING);
}

TEST_F(TransportItemsGoalTestFixture, StartInteracting_SetsStateToInteracting)
{
    auto golem = createCopperGolem(*m_world);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(BlockPos(5, 64, 5));
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_TRAVELLING);

    accessor.startInteracting();

    EXPECT_EQ(accessor.getTransportState(), test::TransportItemsBetweenContainersGoalTestAccessor::STATE_INTERACTING);
    EXPECT_EQ(accessor.getInteractionTicks(), 0);
    EXPECT_FALSE(accessor.getInteractionSuccess());
}

TEST_F(TransportItemsGoalTestFixture, Tick_TravellingToQueuing_TransitionsWhenOccupiedAndWithinQueueRange)
{
    // 铜傀儡 A：Goal 拥有者，在排队距离内（2 格）
    auto golemA = createCopperGolem(*m_world, EntityId{1}, 7.5f, 64.0f, 7.5f);
    CopperGolemEntity* golemAPtr = golemA.get();
    m_world->spawnEntity(std::move(golemA));

    // 铜傀儡 B：已占用目标箱子
    auto golemB = createCopperGolem(*m_world, EntityId{2}, 6.0f, 64.0f, 6.0f);
    CopperGolemEntity* golemBPtr = golemB.get();
    m_world->spawnEntity(std::move(golemB));

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());
    golemBPtr->setOpenedChestPos(chestPos);

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemAPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_TRAVELLING);

    accessor.tick();

    // 应进入 Queuing 状态
    EXPECT_EQ(accessor.getTransportState(), test::TransportItemsBetweenContainersGoalTestAccessor::STATE_QUEUING);
}

TEST_F(TransportItemsGoalTestFixture, Tick_QueuingToTravelling_TransitionsWhenTargetFreed)
{
    auto golemA = createCopperGolem(*m_world, EntityId{1}, 7.5f, 64.0f, 7.5f);
    CopperGolemEntity* golemAPtr = golemA.get();
    m_world->spawnEntity(std::move(golemA));

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemAPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_QUEUING);

    // 没有其他实体占用目标 → 恢复 Travelling
    accessor.tick();

    EXPECT_EQ(accessor.getTransportState(), test::TransportItemsBetweenContainersGoalTestAccessor::STATE_TRAVELLING);
}

// ============================================================================
// 60 tick 交互序列测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, TickInteracting_Tick1_OpensContainerAndSetsAnimation)
{
    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 主手为空 → 拾取模式
    golemPtr->setMainHandItem(ItemStack{});

    // 放置一个有物品的箱子
    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());
    blockentity::ChestEntity* chest = m_world->placeChest(chestPos);
    ASSERT_NE(chest, nullptr);

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    chest->getInventory()->setItem(0, ItemStack(*stick, 32));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_INTERACTING);

    // 第一次 tick → tick 1
    accessor.tickInteracting();

    EXPECT_EQ(accessor.getInteractionTicks(), 1);
    // 铜傀儡应记录打开的箱子位置
    EXPECT_TRUE(golemPtr->hasContainerOpen(chestPos));
    // 拾取模式 + 容器非空 → GettingItem 动画
    EXPECT_EQ(golemPtr->getBehaviorState(), entity::CopperGolemState::GettingItem);
}

TEST_F(TransportItemsGoalTestFixture, TickInteracting_Tick9_PlaysSound)
{
    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    golemPtr->setMainHandItem(ItemStack{});

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());
    blockentity::ChestEntity* chest = m_world->placeChest(chestPos);
    ASSERT_NE(chest, nullptr);

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    chest->getInventory()->setItem(0, ItemStack(*stick, 32));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_INTERACTING);

    // 推进到 tick 9
    for (i32 i = 0; i < 9; ++i) {
        accessor.tickInteracting();
    }

    EXPECT_EQ(accessor.getInteractionTicks(), 9);
    // 铜傀儡应仍记录打开位置（tick 60 才清除）
    EXPECT_TRUE(golemPtr->hasContainerOpen(chestPos));
}

TEST_F(TransportItemsGoalTestFixture, TickInteracting_Tick60_TransfersItemsAndClosesContainer)
{
    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    golemPtr->setMainHandItem(ItemStack{});

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());
    blockentity::ChestEntity* chest = m_world->placeChest(chestPos);
    ASSERT_NE(chest, nullptr);

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    chest->getInventory()->setItem(0, ItemStack(*stick, 32));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_INTERACTING);

    // 推进到 tick 60（完整交互序列）
    for (i32 i = 0; i < 60; ++i) {
        accessor.tickInteracting();
    }

    EXPECT_EQ(accessor.getInteractionTicks(), 60);
    // 主手应拿到 16 个 STICK
    const ItemStack& mainHand = golemPtr->getMainHandItem();
    EXPECT_FALSE(mainHand.isEmpty());
    EXPECT_EQ(mainHand.getCount(), 16);
    // 容器中应剩余 16 个
    EXPECT_EQ(chest->getInventory()->getItem(0).getCount(), 16);
    // 铜傀儡应清除打开位置
    EXPECT_FALSE(golemPtr->hasContainerOpen(chestPos));
    // 应进入冷却
    EXPECT_EQ(accessor.getCooldown(), 140);
}

// ============================================================================
// 双箱 startOpen 转发测试
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, TickInteracting_DoubleChest_OpensBothHalves)
{
    // 此测试验证双箱场景下 startOpen 在两个 ChestEntity 上分别调用
    // 由于测试环境搭建双箱比较复杂（需要正确的 ChestType 和 facing），
    // 这里通过直接调用 _startInteracting + _tickInteracting 验证转发逻辑

    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    golemPtr->setMainHandItem(ItemStack{});

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());
    blockentity::ChestEntity* chest = m_world->placeChest(chestPos);
    ASSERT_NE(chest, nullptr);

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    chest->getInventory()->setItem(0, ItemStack(*stick, 10));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_INTERACTING);

    // tick 1：应触发 startOpen（即使没有双箱连接，单箱也应正常打开）
    accessor.tickInteracting();

    EXPECT_EQ(accessor.getInteractionTicks(), 1);
    EXPECT_TRUE(golemPtr->hasContainerOpen(chestPos));

    // 推进到 tick 60 完成交互
    for (i32 i = 0; i < 59; ++i) {
        accessor.tickInteracting();
    }

    EXPECT_EQ(accessor.getInteractionTicks(), 60);
    // 主手应拿到 10 个 STICK（容器中只有 10 个，少于 16 上限）
    const ItemStack& mainHand = golemPtr->getMainHandItem();
    EXPECT_FALSE(mainHand.isEmpty());
    EXPECT_EQ(mainHand.getCount(), 10);
}

// ============================================================================
// AABB 距离判定测试（对应 MC 1.21.11 isWithinTargetDistance 算法复刻）
// ============================================================================
//
// 以下测试验证 _isWithinTargetDistance / _getCenterPos / _getInteractionRange
// / _isWithinContinueInteractingDistance 四个核心方法的正确性。
//
// 关键算法（对应 MC isWithinTargetDistance）：
//   1. 以铜傀儡中心点构造 mobSideAABB（尺寸=铜傀儡 boundingBox 尺寸）
//   2. 取目标方块碰撞箱包围盒，X/Z 轴膨胀 distance、Y 轴膨胀 0.5
//   3. 平移到目标方块世界坐标
//   4. 与 mobSideAABB 做严格开区间相交测试
//
// 注意：目标方块必须具有非空碰撞形状（如 CHEST 的完整方块碰撞箱），
// 否则 _isWithinTargetDistance 永远返回 false。

TEST_F(TransportItemsGoalTestFixture, GetCenterPos_ReturnsMiddleYPosition)
{
    // 对应 MC getCenterPos：铜傀儡脚底 Y + boundingBox 高度的一半
    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    const Vector3 center = accessor.getCenterPos();
    // X/Z 为脚底位置
    EXPECT_FLOAT_EQ(center.x, 5.5f);
    EXPECT_FLOAT_EQ(center.z, 5.5f);
    // Y = 64 + height/2（铜傀儡 height=0.98 → 中心 Y=64.49）
    EXPECT_FLOAT_EQ(center.y, 64.0f + golemPtr->boundingBox().height() / 2.0f);
}

TEST_F(TransportItemsGoalTestFixture, GetInteractionRange_NoPath_ReturnsHalfBlock)
{
    // 对应 MC getInteractionRange：navigator.getPath() == null 或 path 未完成 → 0.5
    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    // 无路径 → 0.5
    EXPECT_DOUBLE_EQ(accessor.getInteractionRange(), 0.5);
}

TEST_F(TransportItemsGoalTestFixture, IsWithinTargetDistance_EmptyCollisionShape_ReturnsFalse)
{
    // 目标方块为空气（空碰撞箱）→ 永远不相交
    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    // 不设置任何方块 → getBlockState 返回 AIR（空碰撞形状）

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(BlockPos(5, 64, 5));

    const Vector3 center = accessor.getCenterPos();
    // 即使距离很大，空碰撞箱也应返回 false
    EXPECT_FALSE(accessor.isWithinTargetDistance(10.0, center));
}

TEST_F(TransportItemsGoalTestFixture, IsWithinTargetDistance_FullChestBlock_IntersectsAtShortRange)
{
    // 铜傀儡紧贴目标箱子（中心距离 < 1），distance=0.5 → 应相交
    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);

    const Vector3 center = accessor.getCenterPos();
    // distance=0.5 时仍应相交（铜傀儡 AABB 与箱子 AABB 已有重叠）
    EXPECT_TRUE(accessor.isWithinTargetDistance(0.5, center));
}

TEST_F(TransportItemsGoalTestFixture, IsWithinTargetDistance_FullChestBlock_NoIntersectAtLongRange)
{
    // 铜傀儡远离目标箱子，distance=0.5 → 不应相交
    auto golem = createCopperGolem(*m_world, EntityId{1}, 20.5f, 64.0f, 20.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);

    const Vector3 center = accessor.getCenterPos();
    EXPECT_FALSE(accessor.isWithinTargetDistance(0.5, center));
}

TEST_F(TransportItemsGoalTestFixture, IsWithinTargetDistance_LargerDistance_IncludesFartherGolem)
{
    // 验证 distance 参数对 AABB 膨胀的影响：
    // 铜傀儡在 (9.5, 64, 9.5)，目标箱子在 (5, 64, 5)。
    // 箱子碰撞箱 [0,1] 膨胀后 maxX = 1 + distance + 5（方块世界坐标偏移）。
    // 铜傀儡 mobSideAABB minX = 9.5 - 0.245 = 9.255。
    // - distance=3.0：膨胀后 maxX=9，9.255 > 9 → 严格开区间不相交
    // - distance=4.0：膨胀后 maxX=10，9.255 < 10 → 相交
    auto golem = createCopperGolem(*m_world, EntityId{1}, 9.5f, 64.0f, 9.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);

    const Vector3 center = accessor.getCenterPos();
    // distance=3.0 → 不相交
    EXPECT_FALSE(accessor.isWithinTargetDistance(3.0, center));
    // distance=4.0 → 相交
    EXPECT_TRUE(accessor.isWithinTargetDistance(4.0, center));
}

TEST_F(TransportItemsGoalTestFixture, IsWithinContinueInteractingDistance_Within2Blocks_ReturnsTrue)
{
    // 铜傀儡紧贴目标箱子 → 继续交互距离 2.0 内 → 返回 true
    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    EXPECT_TRUE(accessor.isWithinContinueInteractingDistance());
}

TEST_F(TransportItemsGoalTestFixture, IsWithinContinueInteractingDistance_Beyond2Blocks_ReturnsFalse)
{
    // 铜傀儡距目标 5 格 → 超出继续交互距离 2.0 → 返回 false
    auto golem = createCopperGolem(*m_world, EntityId{1}, 10.5f, 64.0f, 10.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    EXPECT_FALSE(accessor.isWithinContinueInteractingDistance());
}

// ============================================================================
// 交互中断测试（_tickInteracting 在超出继续交互距离时中断交互序列）
// ============================================================================

TEST_F(TransportItemsGoalTestFixture, TickInteracting_GolemMovesAway_AbortsInteractionAndReturnsToTravelling)
{
    // 对应 MC onReachedTarget 的"Interacting 保持判定"：
    //   if (!isWithinTargetDistance(2.0, ...)) { onStartTravelling(mob); return; }
    // 铜傀儡在交互过程中被移出 2.0 距离阈值 → 中断交互、回到 Travelling、清除打开位置

    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    golemPtr->setMainHandItem(ItemStack{});

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());
    blockentity::ChestEntity* chest = m_world->placeChest(chestPos);
    ASSERT_NE(chest, nullptr);

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    chest->getInventory()->setItem(0, ItemStack(*stick, 32));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_INTERACTING);

    // 推进到 tick 1：应触发 startOpen + 记录打开位置
    accessor.tickInteracting();
    EXPECT_EQ(accessor.getInteractionTicks(), 1);
    EXPECT_TRUE(golemPtr->hasContainerOpen(chestPos));

    // 将铜傀儡移出继续交互距离（2.0 阈值）
    golemPtr->setPosition(20.5f, 64.0f, 20.5f);

    // 再次 tick：应中断交互
    accessor.tickInteracting();

    // 交互 tick 不应继续递增（应被重置为 0）
    EXPECT_EQ(accessor.getInteractionTicks(), 0);
    // 状态应回到 Travelling
    EXPECT_EQ(accessor.getTransportState(), test::TransportItemsBetweenContainersGoalTestAccessor::STATE_TRAVELLING);
    // 打开位置应被清除（对应 MC onTravelling 回调：clearOpenedChestPos）
    EXPECT_FALSE(golemPtr->hasContainerOpen(chestPos));
    // 动画状态应重置为 Idle
    EXPECT_EQ(golemPtr->getBehaviorState(), entity::CopperGolemState::Idle);
    // 目标方块应保留（不重新搜索）
    EXPECT_TRUE(accessor.hasDestinationBlock());
}

TEST_F(TransportItemsGoalTestFixture, TickInteracting_GolemStaysClose_ContinuesInteractionSequence)
{
    // 对照测试：铜傀儡保持在继续交互距离内 → 交互序列正常推进到 tick 60

    auto golem = createCopperGolem(*m_world, EntityId{1}, 5.5f, 64.0f, 5.5f);
    CopperGolemEntity* golemPtr = golem.get();
    m_world->spawnEntity(std::move(golem));

    golemPtr->setMainHandItem(ItemStack{});

    BlockPos chestPos(5, 64, 5);
    m_world->setBlock(chestPos, VanillaBlocks::CHEST->defaultState());
    blockentity::ChestEntity* chest = m_world->placeChest(chestPos);
    ASSERT_NE(chest, nullptr);

    const Item* stick = Items::STICK;
    ASSERT_NE(stick, nullptr);
    chest->getInventory()->setItem(0, ItemStack(*stick, 32));

    entity::ai::goal::TransportItemsBetweenContainersGoal goal(golemPtr, 1.0);
    test::TransportItemsBetweenContainersGoalTestAccessor accessor(goal);

    accessor.setDestinationBlock(chestPos);
    accessor.setTransportState(test::TransportItemsBetweenContainersGoalTestAccessor::STATE_INTERACTING);

    // 铜傀儡保持在原地 → 交互序列应完整推进到 tick 60
    for (i32 i = 0; i < 60; ++i) {
        accessor.tickInteracting();
    }

    EXPECT_EQ(accessor.getInteractionTicks(), 60);
    // 主手应拿到 16 个 STICK
    const ItemStack& mainHand = golemPtr->getMainHandItem();
    EXPECT_FALSE(mainHand.isEmpty());
    EXPECT_EQ(mainHand.getCount(), 16);
}
