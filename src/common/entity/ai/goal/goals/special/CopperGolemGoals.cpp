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

#include "CopperGolemGoals.hpp"

#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemTypes.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"

#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// 构造函数
// ============================================================================

TransportItemsBetweenContainersGoal::TransportItemsBetweenContainersGoal(CopperGolemEntity* golem, f64 speedMultiplier)
    : m_golem(golem)
    , m_speedMultiplier(speedMultiplier)
{
    MC_ASSERT_RELEASE(golem != nullptr);
    // 对应 MC TransportItemsBetweenContainers 的 mutex：移动 + 看向
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

// ============================================================================
// shouldExecute / shouldContinueExecuting
// ============================================================================

bool TransportItemsBetweenContainersGoal::shouldExecute()
{
    // 对应 MC TransportItemsBetweenContainers 的触发条件：
    //   1. 冷却 TRANSPORT_ITEMS_COOLDOWN_TICKS 已结束
    //   2. 寻找有效运输目标
    if (m_golem == nullptr) {
        return false;
    }

    // 冷却期内不执行
    if (m_cooldown > 0) {
        --m_cooldown;
        return false;
    }

    // 搜索目标
    return _searchForTarget();
}

bool TransportItemsBetweenContainersGoal::shouldContinueExecuting()
{
    if (m_golem == nullptr) {
        return false;
    }

    switch (m_state) {
        case TransportState::Travelling: {
            // 寻路中：目标仍有效且未到达
            if (!m_destinationBlock.has_value()) {
                return false;
            }
            // 目标方块已被破坏或改变类型 → 停止
            IWorld* w = m_golem->world();
            if (w == nullptr) {
                return false;
            }
            const BlockState* state = w->getBlockState(m_destinationBlock.value());
            if (state == nullptr || !_isValidTargetBlock(*state)) {
                return false;
            }
            // 寻路失败（navigator 无路径且未到达）→ 停止
            auto* mob = dynamic_cast<MobEntity*>(m_golem);
            if (mob != nullptr && mob->navigator() != nullptr) {
                if (mob->navigator()->noPath() && !_hasReachedTarget()) {
                    // 记录不可达位置
                    m_unreachablePositions.insert(m_destinationBlock.value().asLong());
                    if (static_cast<i32>(m_unreachablePositions.size()) > MAX_UNREACHABLE_POSITIONS) {
                        m_unreachablePositions.clear();
                    }
                    return false;
                }
            }
            return true;
        }

        case TransportState::Queuing:
            // 排队状态：目标仍被占用就继续等待
            return m_destinationBlock.has_value();

        case TransportState::Interacting:
            // 交互中：直到完成 60 tick 序列
            return m_interactionTicks < TARGET_INTERACTION_TIME;
    }

    return false;
}

// ============================================================================
// startExecuting / resetTask
// ============================================================================

void TransportItemsBetweenContainersGoal::startExecuting()
{
    // 开始寻路前往目标
    _startTravelling();
}

void TransportItemsBetweenContainersGoal::resetTask()
{
    // 对应 MC onTravelling：旅行中清除打开位置 + 重置状态为 IDLE
    _resetTransportState();
}

// ============================================================================
// tick
// ============================================================================

void TransportItemsBetweenContainersGoal::tick()
{
    if (m_golem == nullptr || !m_destinationBlock.has_value()) {
        return;
    }

    switch (m_state) {
        case TransportState::Travelling:
            // 检查是否到达目标
            if (_hasReachedTarget()) {
                _startInteracting();
            }
            // 寻路过程中每 tick 检查 navigator 是否还在工作，若卡住则重试
            // （PathNavigator 自身有 retry 机制，这里不重复）
            break;

        case TransportState::Queuing:
            // 排队等待：检查目标是否仍被占用
            // 本项目简化实现：不真正排队，直接转为 Travelling 重试
            _startTravelling();
            break;

        case TransportState::Interacting:
            _tickInteracting();
            break;
    }
}

// ============================================================================
// 私有方法实现
// ============================================================================

bool TransportItemsBetweenContainersGoal::_isPickingUpItems() const
{
    // 对应 MC TransportItemsBetweenContainers.isPickingUpItems(mob):
    //   return mob.getMainHandItem().isEmpty();
    return m_golem->getMainHandItem().isEmpty();
}

bool TransportItemsBetweenContainersGoal::_isValidTargetBlock(const BlockState& state) const
{
    // 对应 MC CopperGolemAi 中的两个谓词：
    //   TRANSPORT_ITEM_SOURCE_BLOCK = state -> state.is(BlockTags.COPPER_CHESTS);
    //   TRANSPORT_ITEM_DESTINATION_BLOCK = state -> state.is(Blocks.CHEST) || state.is(Blocks.TRAPPED_CHEST);
    if (_isPickingUpItems()) {
        // 拾取模式：源方块必须是铜箱子
        return BlockTags::COPPER_CHESTS().contains(state);
    }

    // 放置模式：目标方块必须是普通箱子或陷阱箱
    const Block& block = state.getBlock();
    // VanillaBlocks::CHEST 和 VanillaBlocks::TRAPPED_CHEST 是静态 Block* 指针
    return (&block == VanillaBlocks::CHEST) || (&block == VanillaBlocks::TRAPPED_CHEST);
}

bool TransportItemsBetweenContainersGoal::_searchForTarget()
{
    IWorld* world = m_golem->world();
    if (world == nullptr) {
        return false;
    }

    // 搜索中心：铜傀儡所在方块位置
    i32 centerX = static_cast<i32>(std::floor(m_golem->x()));
    i32 centerY = static_cast<i32>(std::floor(m_golem->y()));
    i32 centerZ = static_cast<i32>(std::floor(m_golem->z()));

    // 对应 MC getTransportTarget 的螺旋搜索
    // 范围：HORIZONTAL_SEARCH_RADIUS × VERTICAL_SEARCH_RADIUS
    for (i32 dy = -VERTICAL_SEARCH_RADIUS; dy <= VERTICAL_SEARCH_RADIUS; ++dy) {
        // Y 轴交替搜索（0, 1, -1, 2, -2, ...）
        i32 y = centerY + dy;

        // 水平螺旋搜索（按距离递增）
        for (i32 radius = 0; radius <= HORIZONTAL_SEARCH_RADIUS; ++radius) {
            // 遍历该半径上的所有方块
            for (i32 dx = -radius; dx <= radius; ++dx) {
                for (i32 dz = -radius; dz <= radius; ++dz) {
                    // 只处理当前半径的边缘（避免重复访问内层）
                    if (std::abs(dx) != radius && std::abs(dz) != radius) {
                        continue;
                    }

                    BlockPos checkPos(centerX + dx, y, centerZ + dz);
                    i64 packed = checkPos.asLong();

                    // 跳过已访问位置
                    if (m_visitedPositions.count(packed) > 0) {
                        continue;
                    }

                    // 跳过不可达位置
                    if (m_unreachablePositions.count(packed) > 0) {
                        continue;
                    }

                    const BlockState* state = world->getBlockState(checkPos);
                    if (state == nullptr) {
                        continue;
                    }

                    if (!_isValidTargetBlock(*state)) {
                        continue;
                    }

                    // 检查方块实体是否为 ChestEntity
                    BlockEntity* be = world->getBlockEntity(checkPos);
                    if (be == nullptr) {
                        continue;
                    }
                    auto* chest = dynamic_cast<blockentity::ChestEntity*>(be);
                    if (chest == nullptr) {
                        continue;
                    }

                    // 拾取模式：容器必须非空（否则取不到东西）
                    IInventory* inv = chest->getInventory();
                    if (inv == nullptr) {
                        continue;
                    }
                    if (_isPickingUpItems() && inv->isEmpty()) {
                        continue;
                    }

                    // 放置模式：容器必须能接受物品（主手物品可放入）
                    if (!_isPickingUpItems()) {
                        const ItemStack& mainHand = m_golem->getMainHandItem();
                        if (!inv->canAddItem(mainHand)) {
                            continue;
                        }
                    }

                    // 找到有效目标
                    m_destinationBlock = checkPos;
                    return true;
                }
            }
        }
    }

    // 未找到目标 → 进入冷却
    _enterCooldown();
    return false;
}

blockentity::ChestEntity* TransportItemsBetweenContainersGoal::_getTargetChestEntity() const
{
    if (!m_destinationBlock.has_value() || m_golem == nullptr) {
        return nullptr;
    }
    IWorld* world = m_golem->world();
    if (world == nullptr) {
        return nullptr;
    }
    BlockEntity* be = world->getBlockEntity(m_destinationBlock.value());
    if (be == nullptr) {
        return nullptr;
    }
    return dynamic_cast<blockentity::ChestEntity*>(be);
}

bool TransportItemsBetweenContainersGoal::_hasReachedTarget() const
{
    if (!m_destinationBlock.has_value() || m_golem == nullptr) {
        return false;
    }

    const BlockPos& target = m_destinationBlock.value();
    // 计算到目标方块中心的水平距离平方
    f64 dx = m_golem->x() - (static_cast<f64>(target.x) + 0.5);
    f64 dz = m_golem->z() - (static_cast<f64>(target.z) + 0.5);
    f64 horizontalDistSq = dx * dx + dz * dz;

    f64 dy = m_golem->y() - static_cast<f64>(target.y);

    // 对应 MC CLOSE_ENOUGH_TO_START_INTERACTING_WITH_TARGET_PATH_END_DISTANCE = 1.0
    return horizontalDistSq <= CLOSE_ENOUGH_TO_INTERACT_SQ && std::abs(dy) <= CLOSE_ENOUGH_VERTICAL;
}

void TransportItemsBetweenContainersGoal::_startTravelling()
{
    if (!m_destinationBlock.has_value()) {
        return;
    }

    const BlockPos& target = m_destinationBlock.value();
    // 寻路到目标方块中心
    m_golem->tryMoveTo(static_cast<f64>(target.x) + 0.5,
        static_cast<f64>(target.y),
        static_cast<f64>(target.z) + 0.5,
        m_speedMultiplier);

    m_state = TransportState::Travelling;
}

void TransportItemsBetweenContainersGoal::_startInteracting()
{
    m_state = TransportState::Interacting;
    m_interactionTicks = 0;
    m_interactionSuccess = false;

    // 取消寻路（交互期间不再移动）
    auto* mob = dynamic_cast<MobEntity*>(m_golem);
    if (mob != nullptr && mob->navigator() != nullptr) {
        mob->navigator()->clearPath();
    }
}

void TransportItemsBetweenContainersGoal::_tickInteracting()
{
    ++m_interactionTicks;

    // 对应 MC onReachedTargetInteraction 的三个关键 tick 点
    if (m_interactionTicks == TICK_TO_START_INTERACTION) {
        // tick 1：打开容器 + 记录位置
        blockentity::ChestEntity* chest = _getTargetChestEntity();
        if (chest != nullptr) {
            chest->startOpen(*m_golem);
            m_golem->setOpenedChestPos(m_destinationBlock.value());

            // 若是双箱，同时打开另一半（对应 MC CompoundContainer.startOpen 转发）
            IWorld* w = m_golem->world();
            if (w != nullptr) {
                blockentity::ChestEntity* connected = chest->getConnectedChest(*w);
                if (connected != nullptr) {
                    connected->startOpen(*m_golem);
                }
            }
        }
        // 预判交互结果以设置动画状态（tick 1 就设置动画状态）
        // 对应 MC onReachedTargetInteraction tick==1: coppergolem.setState(p_479346_)
        // 实际物品转移结果在 tick 60 才知道，这里先按预判设置
        IInventory* inv = (chest != nullptr) ? chest->getInventory() : nullptr;
        if (_isPickingUpItems()) {
            // 拾取模式：预判容器是否有物品
            bool hasItems = (inv != nullptr) && !inv->isEmpty();
            m_interactionSuccess = hasItems;
        } else {
            // 放置模式：预判容器能否接受物品
            const ItemStack& mainHand = m_golem->getMainHandItem();
            bool canAdd = (inv != nullptr) && inv->canAddItem(mainHand);
            m_interactionSuccess = canAdd;
        }
        _setAnimationState(m_interactionSuccess);
    } else if (m_interactionTicks == TICK_TO_PLAY_SOUND) {
        // tick 9：播放音效
        _playInteractionSound(m_interactionSuccess);
    } else if (m_interactionTicks == TICK_TO_END_INTERACTION) {
        // tick 60：执行物品转移
        blockentity::ChestEntity* chest = _getTargetChestEntity();
        if (chest != nullptr) {
            IInventory* inv = chest->getInventory();
            if (inv != nullptr) {
                if (_isPickingUpItems()) {
                    _pickupItemFromContainer(*inv);
                } else {
                    _addItemsToContainer(*inv);
                }
                inv->setChanged();
            }
        }

        // 关闭容器 + 清除打开位置
        if (chest != nullptr) {
            chest->stopOpen(*m_golem);
            IWorld* w = m_golem->world();
            if (w != nullptr) {
                blockentity::ChestEntity* connected = chest->getConnectedChest(*w);
                if (connected != nullptr) {
                    connected->stopOpen(*m_golem);
                }
            }
        }
        m_golem->clearOpenedChestPos();

        // 记录已访问位置（避免立即重复访问同一箱子）
        if (m_destinationBlock.has_value()) {
            m_visitedPositions.insert(m_destinationBlock.value().asLong());
            if (static_cast<i32>(m_visitedPositions.size()) > MAX_VISITED_POSITIONS) {
                m_visitedPositions.clear();
            }
        }

        // 进入冷却
        _enterCooldown();
    }
}

void TransportItemsBetweenContainersGoal::_pickupItemFromContainer(IInventory& container)
{
    // 对应 MC TransportItemsBetweenContainers.pickupItemFromContainer:
    //   int i = 0;
    //   for (ItemStack itemstack : p_434826_) {
    //       if (!itemstack.isEmpty()) {
    //           int j = Math.min(itemstack.getCount(), 16);
    //           return p_434826_.removeItem(i, j);
    //       }
    //       i++;
    //   }
    //   return ItemStack.EMPTY;
    for (i32 i = 0; i < container.getContainerSize(); ++i) {
        ItemStack stack = container.getItem(i);
        if (!stack.isEmpty()) {
            i32 toTake = std::min(stack.getCount(), TRANSPORTED_ITEM_MAX_STACK_SIZE);
            ItemStack picked = container.removeItem(i, toTake);
            // 对应 MC: p_435070_.setItemSlot(EquipmentSlot.MAINHAND, pickupItemFromContainer(...))
            m_golem->setMainHandItem(picked);
            // 对应 MC: p_435070_.setGuaranteedDrop(EquipmentSlot.MAINHAND)
            // 标记主手物品必然掉落（死亡时不丢失运输物品）
            auto* mob = dynamic_cast<MobEntity*>(m_golem);
            if (mob != nullptr) {
                mob->setGuaranteedDrop(EquipmentSlot::MainHand);
            }
            m_interactionSuccess = !picked.isEmpty();
            return;
        }
    }
    // 容器为空 → 交互失败
    m_interactionSuccess = false;
}

void TransportItemsBetweenContainersGoal::_addItemsToContainer(IInventory& container)
{
    // 对应 MC 1.21.11 TransportItemsBetweenContainers.addItemsToContainer:
    //   遍历容器一次：遇到空槽直接整堆放入；遇到可堆叠槽增量堆叠。
    //   单次遍历中同时处理空槽和可堆叠槽，直到主手物品清空或遍历完。
    ItemStack itemstack = m_golem->getMainHandItem(); // 取副本
    if (itemstack.isEmpty()) {
        m_interactionSuccess = false;
        return;
    }

    const i32 originalCount = itemstack.getCount();
    i32 containerSize = container.getContainerSize();

    // 单次遍历：与 MC 一致，空槽和可堆叠槽在同一循环中处理
    for (i32 i = 0; i < containerSize && !itemstack.isEmpty(); ++i) {
        ItemStack existing = container.getItem(i);
        if (existing.isEmpty()) {
            // 空槽：整堆放入（对应 MC: setItem(i, itemstack); return EMPTY）
            if (!container.canPlaceItem(i, itemstack)) {
                continue;
            }
            container.setItem(i, itemstack);
            itemstack = ItemStack(); // 清空
            break;
        }

        // 非空槽：检查是否可堆叠（物品相同 + 有剩余空间）
        if (!existing.canMergeWith(itemstack)) {
            continue;
        }

        i32 maxCount = std::min(container.getMaxStackSize(), existing.getMaxStackSize());
        i32 space = maxCount - existing.getCount();
        if (space <= 0) {
            continue;
        }
        i32 toAdd = std::min(space, itemstack.getCount());
        existing.grow(toAdd);
        container.setItem(i, existing);
        itemstack.shrink(toAdd);
    }

    // 设置主手为剩余物品（可能为空）
    m_golem->setMainHandItem(itemstack);
    // 交互成功 = 至少放入了一些（剩余数量 < 原始数量）
    m_interactionSuccess = itemstack.getCount() < originalCount;
}

void TransportItemsBetweenContainersGoal::_setAnimationState(bool success)
{
    // 对应 MC onReachedTargetInteraction 的 CopperGolemState 设置：
    //   PICKUP_ITEM    → GETTING_ITEM    (success)
    //   PICKUP_NO_ITEM → GETTING_NO_ITEM (failure)
    //   PLACE_ITEM     → DROPPING_ITEM   (success)
    //   PLACE_NO_ITEM  → DROPPING_NO_ITEM(failure)
    if (_isPickingUpItems()) {
        if (success) {
            m_golem->setBehaviorState(entity::CopperGolemState::GettingItem);
        } else {
            m_golem->setBehaviorState(entity::CopperGolemState::GettingNoItem);
        }
    } else {
        if (success) {
            m_golem->setBehaviorState(entity::CopperGolemState::DroppingItem);
        } else {
            m_golem->setBehaviorState(entity::CopperGolemState::DroppingNoItem);
        }
    }
}

void TransportItemsBetweenContainersGoal::_playInteractionSound(bool success)
{
    // 对应 MC onReachedTargetInteraction tick==9: coppergolem.playSound(p_480078_)
    // 音效映射（MC CopperGolemAi.getTargetReachedInteractions）：
    //   PICKUP_ITEM    → COPPER_GOLEM_ITEM_GET
    //   PICKUP_NO_ITEM → COPPER_GOLEM_ITEM_NO_GET
    //   PLACE_ITEM     → COPPER_GOLEM_ITEM_DROP
    //   PLACE_NO_ITEM  → COPPER_GOLEM_ITEM_NO_DROP
    const ResourceLocation* sound = nullptr;
    if (_isPickingUpItems()) {
        if (success) {
            sound = &SoundEvents::ENTITY_COPPER_GOLEM_ITEM_GET;
        } else {
            sound = &SoundEvents::ENTITY_COPPER_GOLEM_ITEM_NO_GET;
        }
    } else {
        if (success) {
            sound = &SoundEvents::ENTITY_COPPER_GOLEM_ITEM_DROP;
        } else {
            sound = &SoundEvents::ENTITY_COPPER_GOLEM_ITEM_NO_DROP;
        }
    }

    if (sound != nullptr) {
        m_golem->playSound(*sound, 1.0f, 1.0f);
    }
}

void TransportItemsBetweenContainersGoal::_enterCooldown()
{
    m_cooldown = IDLE_COOLDOWN;
}

void TransportItemsBetweenContainersGoal::_resetTransportState()
{
    // 对应 MC onTravelling: coppergolem.clearOpenedChestPos(); coppergolem.setState(IDLE);
    m_destinationBlock.reset();
    m_state = TransportState::Travelling;
    m_interactionTicks = 0;
    m_interactionSuccess = false;

    // 取消寻路
    auto* mob = dynamic_cast<MobEntity*>(m_golem);
    if (mob != nullptr && mob->navigator() != nullptr) {
        mob->navigator()->clearPath();
    }

    // 清除打开位置 + 重置行为状态
    m_golem->clearOpenedChestPos();
    m_golem->setBehaviorState(entity::CopperGolemState::Idle);
}

} // namespace mc::entity::ai::goal
