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

#include "VillagerGoals.hpp"

#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/functional/ComposterBlock.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/blocks/agricultural/FarmlandBlock.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <cmath>
#include <limits>

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

// ============================================================================
// 辅助函数
// ============================================================================

namespace {

/**
 * @brief 检查是否是夜间时间
 * @param dayTime 一天内的时间 (0-23999)
 * @return 是否是夜间
 *
 * 夜间时间: 12542-23459 (黄昏到黎明)
 */
[[nodiscard]] bool isNightTime(i64 dayTime)
{
    return dayTime >= 12542 && dayTime <= 23459;
}

/**
 * @brief 检查是否是工作时间
 * @param dayTime 一天内的时间 (0-23999)
 * @return 是否是工作时间
 *
 * 工作时间: 2000-9000 (MC时间)
 */
[[nodiscard]] bool isWorkTime(i64 dayTime)
{
    return dayTime >= 2000 && dayTime <= 9000;
}

/**
 * @brief 计算实体到方块位置的距离平方
 * @param entity 实体
 * @param pos 方块位置
 * @return 距离平方（使用方块中心点）
 */
[[nodiscard]] f32 distanceToBlockCenter(const Entity* entity, const BlockPos& pos)
{
    if (!entity) return std::numeric_limits<f32>::max();
    return entity->distanceSqTo(pos.x + 0.5f, static_cast<f32>(pos.y), pos.z + 0.5f);
}

/**
 * @brief 检查实体是否在指定距离内
 * @param entity 实体
 * @param pos 方块位置
 * @param maxDistance 最大距离
 * @return 是否在范围内
 */
[[nodiscard]] bool isWithinDistance(const Entity* entity, const BlockPos& pos, f32 maxDistance)
{
    f32 distSq = distanceToBlockCenter(entity, pos);
    return distSq < maxDistance * maxDistance;
}

/**
 * @brief 从库存中抛出一半匹配的物品给目标实体
 *
 * 遍历库存，找到第一个匹配 itemFilter 的物品：
 * - 如果数量 > maxStackSize/2，抛出 count/2 个
 * - 如果数量 > 24 但不超过半组，保留24个，抛出剩余
 *
 * @param villager 分享物品的村民
 * @param inventory 源村民库存
 * @param itemFilter 允许抛出的物品及其点数映射
 * @param target 目标实体
 * @return 是否成功抛出了物品
 */
bool throwHalfStackToTarget(VillagerEntity* villager,
    IInventory& inventory,
    const std::unordered_map<const Item*, i32>& itemFilter,
    LivingEntity* target)
{
    if (!villager || !target) return false;
    IWorld* world = villager->world();
    if (!world) return false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) continue;

        const Item* item = stack.getItem();
        if (itemFilter.find(item) == itemFilter.end()) continue;

        // 计算要抛出的数量
        i32 count = stack.getCount();
        i32 maxStackSize = stack.getMaxStackSize();
        i32 throwCount = 0;

        if (count > maxStackSize / 2) {
            // 超过半组，抛出一半
            throwCount = count / 2;
        } else if (count > 24) {
            // 超过24个但不超过半组，保留24个，抛出剩余
            throwCount = count - 24;
        }

        if (throwCount > 0) {
            // 从源库存中移除物品
            inventory.removeItem(i, throwCount);

            // 创建新的物品堆并抛向目标
            ItemStack throwStack(item, throwCount);

            // 在村民眼睛高度略微偏下位置生成物品
            f64 spawnX = villager->x();
            f64 spawnY = villager->y() + villager->eyeHeight() - 0.3;
            f64 spawnZ = villager->z();

            // 计算朝向目标的方向向量
            f64 dx = target->x() - spawnX;
            f64 dy = target->y() + target->eyeHeight() * 0.5 - spawnY;
            f64 dz = target->z() - spawnZ;
            f64 dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            // 抛出速度（与 MC 的 throwItem 一致，约 0.3-0.5）
            static constexpr f32 THROW_SPEED = 0.35f;

            f32 vx = 0.0f, vy = 0.0f, vz = 0.0f;
            if (dist > 0.001) {
                vx = static_cast<f32>(dx / dist) * THROW_SPEED;
                vy = static_cast<f32>(dy / dist) * THROW_SPEED + 0.1f; // 略微向上抛
                vz = static_cast<f32>(dz / dist) * THROW_SPEED;
            } else {
                vy = 0.1f; // 距离太近时直接向上抛
            }

            // 添加随机偏移（与 MC 的 spread(0.3, 0.3, 0.3) 对应）
            math::Random rng = villager->getRandom();
            vx += (rng.nextFloat() - 0.5f) * 0.3f;
            vy += (rng.nextFloat() - 0.5f) * 0.3f;
            vz += (rng.nextFloat() - 0.5f) * 0.3f;

            // 生成物品实体
            auto itemEntity = std::make_unique<ItemEntity>(EntityId(0), throwStack, spawnX, spawnY, spawnZ, vx, vy, vz);

            // 设置拾取延迟（防止村民立即捡回自己扔出的物品）
            static constexpr i32 ITEM_THROW_PICKUP_DELAY = 40; // 2秒
            itemEntity->setPickupDelay(ITEM_THROW_PICKUP_DELAY);
            itemEntity->setOwner(villager->uuid());

            world->spawnEntity(std::move(itemEntity));
            return true;
        }
    }

    return false;
}

} // anonymous namespace

// ============================================================================
// SleepAtNightGoal - 村民夜间睡眠目标
// ============================================================================

SleepAtNightGoal::SleepAtNightGoal(VillagerEntity* villager)
    : m_villager(villager)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool SleepAtNightGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 检查是否是夜间
    if (!_isNightTime()) return false;

    // 已经在睡眠中则不重新开始
    if (m_villager->isSleeping()) return false;

    // 检查是否有绑定的床位（从Brain的HOME记忆获取）
    // HOME记忆使用GlobalPos类型，包含维度信息
    auto homeMemory = m_villager->brain().getMemory<GlobalPos>(ai::brain::memory::MemoryModuleTypes::HOME);
    if (homeMemory.has_value()) {
        // 检查是否在同一维度
        if (m_villager->world() && homeMemory->getDimensionId() == m_villager->world()->dimension()) {
            m_bedPos = homeMemory->getPos();
            return true;
        }
    }

    // 没有绑定床位，尝试查找最近的可用床
    auto bedPos = _findNearestBed();
    if (!bedPos.has_value()) return false;

    m_bedPos = bedPos.value();
    return true;
}

bool SleepAtNightGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 继续执行直到天亮或床位不可用
    if (!_isNightTime()) return false;

    // 如果正在睡眠，继续睡眠直到天亮
    if (m_villager->isSleeping()) return true;

    // 检查床位是否仍然有效
    if (!_isBedStillValid()) return false;

    return m_sleeping || m_trySleepTicks < MAX_TRY_SLEEP_TICKS;
}

void SleepAtNightGoal::startExecuting()
{
    m_sleeping = false;
    m_trySleepTicks = 0;
    _moveToBed();
}

void SleepAtNightGoal::resetTask()
{
    m_sleeping = false;
    m_trySleepTicks = 0;
    m_bedPos = BlockPos::zero();

    if (m_villager) {
        // 如果正在睡眠，停止睡眠
        if (m_villager->isSleeping()) {
            m_villager->stopSleeping();
        }
        m_villager->clearNavigation();
    }
}

void SleepAtNightGoal::tick()
{
    if (!m_villager) return;

    m_trySleepTicks++;

    // 检查是否到达床位
    if (isWithinDistance(m_villager, m_bedPos, 1.5f)) {
        _trySleep();
    } else if (!m_sleeping) {
        // 继续移动到床位
        _moveToBed();
    }
}

bool SleepAtNightGoal::_isNightTime() const
{
    if (!m_villager || !m_villager->world()) return false;
    return isNightTime(m_villager->world()->dayTimeOfDay());
}

std::optional<BlockPos> SleepAtNightGoal::_findNearestBed() const
{
    if (!m_villager || !m_villager->world()) return std::nullopt;

    // 通过VillageManager获取POI存储
    auto* villageManager = m_villager->world()->villageManager();
    if (!villageManager) return std::nullopt;

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(
        static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));

    // 村民搜索床位的范围是48格
    constexpr f32 BED_SEARCH_RANGE = 48.0f;

    // 查找最近的未被占用的床位
    // 床的POI类型包括所有颜色的床，需要搜索所有床类型
    using namespace world::village::poi;

    // 先尝试查找未占用的床
    std::optional<BlockPos> bedPos = std::nullopt;
    f32 closestDist = BED_SEARCH_RANGE * BED_SEARCH_RANGE;

    // 遍历所有床类型
    for (i32 bedType = static_cast<i32>(PointOfInterestType::BedRed);
        bedType <= static_cast<i32>(PointOfInterestType::BedYellow);
        ++bedType) {

        PointOfInterestType poiType = static_cast<PointOfInterestType>(bedType);
        auto result = poiStorage.findNearestFree(villagerPos, poiType, BED_SEARCH_RANGE);

        if (result.has_value()) {
            f32 dist = static_cast<f32>(villagerPos.distanceSq(result.value()));
            if (dist < closestDist) {
                closestDist = dist;
                bedPos = result;
            }
        }
    }

    return bedPos;
}

void SleepAtNightGoal::_moveToBed()
{
    if (!m_villager) return;

    m_villager->tryMoveTo(m_bedPos.x + 0.5, m_bedPos.y, m_bedPos.z + 0.5, 0.5);
}

void SleepAtNightGoal::_trySleep()
{
    if (!m_villager) return;

    // 检查床位是否仍然有效
    if (!_isBedStillValid()) {
        return;
    }

    // 通过VillageManager获取POI存储，占用床位
    auto* villageManager = m_villager->world()->villageManager();
    if (villageManager) {
        auto& poiStorage = villageManager->getPOIStorage();

        // 检查床位是否已被占用
        const auto* poi = poiStorage.getPOI(m_bedPos);
        if (poi && poi->isOccupied()) {
            // 床位已被其他人占用，重新寻找床位
            auto newBedPos = _findNearestBed();
            if (newBedPos.has_value()) {
                m_bedPos = newBedPos.value();
                _moveToBed();
            }
            return;
        }

        // 占用床位
        poiStorage.acquirePOI(m_bedPos, static_cast<u64>(m_villager->id()), m_villager->world()->currentTick());
    }

    // 开始睡眠
    m_villager->startSleeping(m_bedPos);
    m_sleeping = true;

    // 将床位保存到Brain的HOME记忆（使用GlobalPos包含维度信息）
    if (m_villager->world()) {
        GlobalPos homePos(m_villager->world()->dimension(), m_bedPos);
        m_villager->brain().setMemory(ai::brain::memory::MemoryModuleTypes::HOME, homePos);
    }
}

bool SleepAtNightGoal::_isBedStillValid() const
{
    if (!m_villager || !m_villager->world()) return false;

    if (m_bedPos == BlockPos::zero()) return false;

    // 检查方块是否还是床
    const auto* blockState = m_villager->world()->getBlockState(m_bedPos);
    if (!blockState) return false;

    // 使用BedBlock::isBed检查
    return mc::blocks::BedBlock::isBed(*m_villager->world(), m_bedPos);
}

// ============================================================================
// WorkAtJobSiteGoal - 村民工作目标
// ============================================================================

WorkAtJobSiteGoal::WorkAtJobSiteGoal(VillagerEntity* villager)
    : m_villager(villager)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool WorkAtJobSiteGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 傻子村民不工作
    if (m_villager->isNitwit()) return false;

    // 检查是否是工作时间
    if (!_isWorkTime()) return false;

    // 检查是否有工作站点
    return _hasJobSite();
}

bool WorkAtJobSiteGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 继续工作的条件
    if (!_isWorkTime()) return false;
    if (!_hasJobSite()) return false;

    // 限制工作时间
    return m_workTicks < WORK_TICKS_MAX;
}

void WorkAtJobSiteGoal::startExecuting()
{
    m_workTicks = 0;
    m_atJobSite = false;
    _moveToJobSite();
}

void WorkAtJobSiteGoal::resetTask()
{
    m_workTicks = 0;
    m_atJobSite = false;

    if (m_villager) {
        m_villager->clearNavigation();
        // 重置工作状态
        m_villager->rest();
    }
}

void WorkAtJobSiteGoal::tick()
{
    if (!m_villager) return;

    m_workTicks++;

    // 检查是否在工作站点附近
    BlockPos workPos = m_villager->workStation();

    if (isWithinDistance(m_villager, workPos, 2.0f)) {
        m_atJobSite = true;
        _doWork();
    } else {
        m_atJobSite = false;
        _moveToJobSite();
    }

    // 检查补货
    if (_needsRestock()) {
        _restock();
    }
}

bool WorkAtJobSiteGoal::_isWorkTime() const
{
    if (!m_villager || !m_villager->world()) return false;
    return isWorkTime(m_villager->world()->dayTimeOfDay());
}

bool WorkAtJobSiteGoal::_hasJobSite() const
{
    if (!m_villager) return false;
    return m_villager->workStation() != BlockPos::zero();
}

void WorkAtJobSiteGoal::_moveToJobSite()
{
    if (!m_villager) return;

    BlockPos workPos = m_villager->workStation();
    m_villager->tryMoveTo(workPos.x + 0.5, workPos.y, workPos.z + 0.5, 0.4);
}

void WorkAtJobSiteGoal::_doWork()
{
    if (!m_villager) return;

    // 设置工作状态
    m_villager->work();

    // 每隔一段时间增加经验
    if (m_workTicks % 100 == 0) {
        m_villager->addVillagerExperience(1);
    }
}

bool WorkAtJobSiteGoal::_needsRestock() const
{
    if (!m_villager) return false;

    // 检查交易是否需要补货
    // TODO: 检查交易使用次数
    return false;
}

void WorkAtJobSiteGoal::_restock()
{
    if (!m_villager) return;

    m_villager->restockTrades();
}

// ============================================================================
// LookForJobSiteGoal - 村民寻找工作站点目标
// ============================================================================

LookForJobSiteGoal::LookForJobSiteGoal(VillagerEntity* villager)
    : m_villager(villager)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool LookForJobSiteGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 已有工作站点的不需要寻找
    if (m_villager->workStation() != BlockPos::zero()) return false;

    // 傻子村民不找工作
    if (m_villager->isNitwit()) return false;

    // 冷却时间
    if (m_searchCooldown > 0) return false;

    return true;
}

bool LookForJobSiteGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 找到工作站点或超时
    return !m_targetSite.has_value() && m_searchCooldown < SEARCH_COOLDOWN;
}

void LookForJobSiteGoal::startExecuting()
{
    m_targetSite = std::nullopt;
    m_searchCooldown = 0;
    _searchForJobSite();
}

void LookForJobSiteGoal::resetTask()
{
    m_targetSite = std::nullopt;
    m_searchCooldown = SEARCH_COOLDOWN;

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void LookForJobSiteGoal::tick()
{
    if (!m_villager) return;

    m_searchCooldown++;

    if (m_targetSite.has_value()) {
        // 移动到目标工作站点
        BlockPos pos = m_targetSite.value();
        m_villager->tryMoveTo(pos.x + 0.5, pos.y, pos.z + 0.5, 0.4);

        // 检查是否到达
        if (isWithinDistance(m_villager, pos, 2.0f)) {
            // 绑定工作站点
            m_villager->setWorkStation(pos);
            m_targetSite = std::nullopt;
        }
    }
}

void LookForJobSiteGoal::_searchForJobSite()
{
    if (!m_villager || !m_villager->world()) return;

    // TODO: 集成POI系统搜索工作站点
    // 根据村民职业搜索对应的工作站点类型
    // 目前不实现，等待POI系统
}

// ============================================================================
// GatherItemsGoal - 村民收集物品目标
// ============================================================================

GatherItemsGoal::GatherItemsGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_targetItem(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool GatherItemsGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 查找附近的物品
    _findNearbyItems();
    return m_targetItem != 0;
}

bool GatherItemsGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 物品已被拾取或消失
    if (m_targetItem == 0) return false;

    // 检查物品是否仍然有效
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetItem) : nullptr;
    if (!entity) return false;

    ItemEntity* item = dynamic_cast<ItemEntity*>(entity);
    if (!item || !item->isAlive() || !item->canBePickedUp()) {
        return false;
    }

    // 检查物品是否仍在范围内
    f32 distSq = m_villager->distanceSqTo(*item);
    if (distSq > PICKUP_RANGE * PICKUP_RANGE) {
        return false;
    }

    return true;
}

void GatherItemsGoal::startExecuting()
{
    // 已在shouldExecute中找到目标
}

void GatherItemsGoal::resetTask()
{
    m_targetItem = 0;
    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void GatherItemsGoal::tick()
{
    if (!m_villager || m_targetItem == 0) return;

    // 移动到物品
    _moveToItem();

    // 尝试拾取
    _pickupItem();
}

void GatherItemsGoal::_findNearbyItems()
{
    if (!m_villager || !m_villager->world()) {
        m_targetItem = 0;
        return;
    }

    m_targetItem = 0;

    // 使用 EntityUtils 查找最近的 ItemEntity
    ItemEntity* item = EntityUtils::findClosestEntity<ItemEntity>(m_villager->world(),
        m_villager->position(),
        PICKUP_RANGE,
        m_villager, // 排除自己（虽然村民不是 ItemEntity）
        [this](ItemEntity* itemEntity) {
            // 检查物品实体是否有效
            if (!itemEntity || !itemEntity->isAlive()) return false;

            // 检查是否可以被拾取（拾取延迟等）
            if (!itemEntity->canBePickedUp()) return false;

            // 检查村民是否想要这个物品
            const ItemStack& stack = itemEntity->getItemStack();
            if (stack.isEmpty()) return false;

            // 使用 VillagerEntity::canPickUpItem 检查是否是村民可拾取的物品
            return m_villager->canPickUpItem(stack);
        });

    if (item) {
        m_targetItem = item->id();
    }
}

void GatherItemsGoal::_moveToItem()
{
    if (!m_villager || m_targetItem == 0) return;

    // 从世界获取实体
    Entity* entity = m_villager->world()->getEntity(m_targetItem);
    if (!entity) {
        m_targetItem = 0;
        return;
    }

    ItemEntity* item = dynamic_cast<ItemEntity*>(entity);
    if (!item || !item->isAlive()) {
        m_targetItem = 0;
        return;
    }

    // 检查物品是否还能被拾取
    if (!item->canBePickedUp()) {
        m_targetItem = 0;
        return;
    }

    // 移动到物品位置
    // 村民移动速度约 0.5
    m_villager->tryMoveTo(item->x(), item->y(), item->z(), 0.5);
}

void GatherItemsGoal::_pickupItem()
{
    if (!m_villager || m_targetItem == 0) return;

    // 从世界获取实体
    Entity* entity = m_villager->world()->getEntity(m_targetItem);
    if (!entity) {
        m_targetItem = 0;
        return;
    }

    ItemEntity* item = dynamic_cast<ItemEntity*>(entity);
    if (!item || !item->isAlive() || !item->canBePickedUp()) {
        m_targetItem = 0;
        return;
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*item);
    if (distSq > PICKUP_DISTANCE * PICKUP_DISTANCE) {
        return; // 还没到拾取距离
    }

    // 获取物品堆
    ItemStack stack = item->getItemStack();
    if (stack.isEmpty()) {
        m_targetItem = 0;
        return;
    }

    // 再次确认村民可以拾取这个物品
    if (!m_villager->canPickUpItem(stack)) {
        m_targetItem = 0;
        return;
    }

    // 将物品添加到村民库存
    IInventory& inventory = m_villager->inventory();
    ItemStack remaining = inventory.addItem(stack);

    // 如果有剩余物品（库存满了），更新物品实体的数量
    if (!remaining.isEmpty()) {
        item->setItemStack(remaining);
    } else {
        // 完全拾取，移除物品实体
        item->remove();
    }

    m_targetItem = 0; // 清除目标
}

// ============================================================================
// FarmerWorkGoal - 农民工作目标
// ============================================================================
//
// 实现参考：MC 1.21.11 HarvestFarmland + UseBonemeal + WorkAtComposter
// 农民在耕地区域执行：收获成熟作物、种植种子、堆肥多余种子

FarmerWorkGoal::FarmerWorkGoal(VillagerEntity* villager)
    : WorkAtJobSiteGoal(villager)
    , m_farmerWorkTicks(0)
{}

void FarmerWorkGoal::tick()
{
    if (!m_villager) return;

    m_farmerWorkTicks++;

    // 执行基类的工作逻辑
    WorkAtJobSiteGoal::tick();

    // 农民特有行为
    if (m_farmerWorkTicks % FARMER_WORK_INTERVAL == 0) {
        // 尝试收获
        _tryHarvest();

        // 尝试种植
        _tryPlant();

        // 尝试堆肥
        _tryCompost();
    }
}

void FarmerWorkGoal::_tryHarvest()
{
    if (!m_villager || !m_villager->world()) return;

    // 在当前工作区域搜索成熟作物
    IWorld* world = m_villager->world();
    i32 cx = static_cast<i32>(m_villager->x());
    i32 cy = static_cast<i32>(m_villager->y());
    i32 cz = static_cast<i32>(m_villager->z());

    // MC 原版搜索 3x3x3 区域
    for (i32 dx = -FARMER_SEARCH_RANGE; dx <= FARMER_SEARCH_RANGE; ++dx) {
        for (i32 dy = -FARMER_SEARCH_RANGE; dy <= FARMER_SEARCH_RANGE; ++dy) {
            for (i32 dz = -FARMER_SEARCH_RANGE; dz <= FARMER_SEARCH_RANGE; ++dz) {
                BlockPos checkPos(cx + dx, cy + dy, cz + dz);

                if (!_isCropMatureAt(checkPos)) continue;

                // 收获成熟作物：将作物方块设为空气，由方块自身的 drop 逻辑处理掉落物
                // MC 原版使用 destroyBlock，这里设为空气即可（作物破坏后会自然掉落）
                const BlockState* cropState = world->getBlockState(checkPos);
                if (!cropState) continue;

                const Block& block = cropState->getBlock();
                auto* cropBlock = dynamic_cast<const blocks::CropBlock*>(&block);
                if (!cropBlock) continue;

                // 收获掉落物处理：成熟时掉落作物和种子
                u32 cropItemId = cropBlock->getCropItem();
                u32 seedItemId = cropBlock->getSeedItem();

                // 计算掉落数量：成熟时种子掉落 1+0~2（小麦额外 0~3 种子）
                math::Random rng = m_villager->getRandom();
                i32 seedCount = 1 + rng.nextInt(3); // 1~3 颗种子

                // 将作物产品放入村民背包
                if (cropItemId != 0) {
                    const Item* cropItem = Item::getItem(static_cast<ItemId>(cropItemId));
                    if (cropItem) {
                        ItemStack cropStack(cropItem, 1);
                        IInventory& inventory = m_villager->inventory();
                        ItemStack remaining = inventory.addItem(cropStack);
                        // 装不下的掉落在地上
                        if (!remaining.isEmpty()) {
                            ItemDropHelper::spawnItemEntity(world,
                                remaining,
                                checkPos.x + 0.5,
                                static_cast<f64>(checkPos.y),
                                checkPos.z + 0.5,
                                rng);
                        }
                    }
                }

                // 种子掉落
                if (seedItemId != 0) {
                    const Item* seedItem = Item::getItem(static_cast<ItemId>(seedItemId));
                    if (seedItem) {
                        ItemStack seedStack(seedItem, seedCount);
                        IInventory& inventory = m_villager->inventory();
                        ItemStack remaining = inventory.addItem(seedStack);
                        // 装不下的掉落在地上
                        if (!remaining.isEmpty()) {
                            ItemDropHelper::spawnItemEntity(world,
                                remaining,
                                checkPos.x + 0.5,
                                static_cast<f64>(checkPos.y),
                                checkPos.z + 0.5,
                                rng);
                        }
                    }
                }

                // 将作物方块设为空气
                const BlockState* airState = BlockRegistry::instance().airState();
                if (airState) {
                    world->setBlockState(checkPos, airState, 2);
                }

                // 收获一个就返回，避免一次收获太多
                return;
            }
        }
    }
}

void FarmerWorkGoal::_tryPlant()
{
    if (!m_villager || !m_villager->world()) return;

    // 检查是否有可种植的种子
    if (!_hasFarmSeeds()) return;

    IWorld* world = m_villager->world();
    i32 cx = static_cast<i32>(m_villager->x());
    i32 cy = static_cast<i32>(m_villager->y());
    i32 cz = static_cast<i32>(m_villager->z());

    // MC 原版搜索 3x3x3 区域
    for (i32 dx = -FARMER_SEARCH_RANGE; dx <= FARMER_SEARCH_RANGE; ++dx) {
        for (i32 dy = -FARMER_SEARCH_RANGE; dy <= FARMER_SEARCH_RANGE; ++dy) {
            for (i32 dz = -FARMER_SEARCH_RANGE; dz <= FARMER_SEARCH_RANGE; ++dz) {
                BlockPos checkPos(cx + dx, cy + dy, cz + dz);

                if (!_canPlantAt(checkPos)) continue;

                // 找到可种植位置，尝试从背包中获取种子并种植
                IInventory& inventory = m_villager->inventory();
                for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
                    ItemStack stack = inventory.getItem(slot);
                    if (stack.isEmpty()) continue;

                    const Item* item = stack.getItem();
                    if (!item) continue;

                    // 检查该物品是否是方块物品（种子放置后变成作物方块）
                    const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
                    if (!block) continue;

                    // 检查方块是否是作物（继承自 CropBlock）
                    auto* cropBlock = dynamic_cast<const blocks::CropBlock*>(block);
                    if (!cropBlock) continue;

                    // 种植作物：放置默认状态（age=0）
                    const BlockState& plantState = cropBlock->defaultState();
                    world->setBlockState(checkPos, &plantState, 2);

                    // 播放种植音效
                    world->playSound(ResourceLocation("minecraft:block.crop_planted"),
                        sound::SoundCategory::Blocks,
                        Vector3(static_cast<f64>(checkPos.x) + 0.5,
                            static_cast<f64>(checkPos.y),
                            static_cast<f64>(checkPos.z) + 0.5),
                        1.0f,
                        1.0f);

                    // 消耗一个种子
                    inventory.removeItem(slot, 1);

                    // 种植一个就返回
                    return;
                }
            }
        }
    }
}

void FarmerWorkGoal::_tryCompost()
{
    if (!m_villager) return;

    IWorld* world = m_villager->world();
    if (!world) return;

    // 查找附近的堆肥桶（农民的工作站点就是堆肥桶）
    auto* villageManager = world->villageManager();
    if (!villageManager) return;

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(
        static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));

    // 搜索最近的堆肥桶
    using namespace world::village::poi;
    auto composterPos = poiStorage.findNearestFree(villagerPos, PointOfInterestType::Composter, 4.0f);

    if (!composterPos.has_value()) return;

    BlockPos pos = composterPos.value();

    // 检查堆肥桶方块
    const BlockState* state = world->getBlockState(pos);
    if (!state) return;

    // 确认是堆肥桶
    auto* composter = dynamic_cast<const blocks::ComposterBlock*>(&state->getBlock());
    if (!composter) return;

    i32 level = blocks::ComposterBlock::getLevel(*state);

    // 如果堆肥桶已满（等级8），先取出骨粉
    if (level >= 8) {
        // MC 原版：满堆肥桶取出骨粉
        auto newState = blocks::ComposterBlock::empty(*world, pos, *const_cast<BlockState*>(state));
        // 将骨粉加入村民背包
        const Item* boneMeal = Items::BONE_MEAL;
        if (boneMeal) {
            ItemStack boneMealStack(boneMeal, 1);
            IInventory& inventory = m_villager->inventory();
            ItemStack remaining = inventory.addItem(boneMealStack);
            if (!remaining.isEmpty()) {
                // 装不下就丢在地上
                math::Random rng = m_villager->getRandom();
                ItemDropHelper::spawnItemEntity(world,
                    remaining,
                    m_villager->x(),
                    m_villager->y() + m_villager->eyeHeight() - 0.3,
                    m_villager->z(),
                    rng);
            }
        }
        return;
    }

    // 尝试将多余的种子堆肥
    // MC 原版 WorkAtComposter 只堆肥小麦种子和甜菜种子
    IInventory& inventory = m_villager->inventory();
    static const Item* compostableItems[] = {Items::WHEAT_SEEDS, Items::BEETROOT_SEEDS};

    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) continue;

        const Item* item = stack.getItem();
        if (!item) continue;

        // 检查是否是可堆肥物品
        bool isCompostableItem = false;
        for (const Item* compostable : compostableItems) {
            if (item == compostable) {
                isCompostableItem = true;
                break;
            }
        }
        if (!isCompostableItem) continue;

        // MC 原版：保留10个种子，多余的（超过10个的部分，最多20个）用于堆肥
        i32 count = stack.getCount();
        if (count <= 10) continue;

        i32 compostCount = std::min(count - 10, 20);

        // 尝试堆肥
        for (i32 i = 0; i < compostCount; ++i) {
            auto newState = blocks::ComposterBlock::attemptCompost(
                *state, *world, pos, *const_cast<Block*>(&state->getBlock()), item->itemId());

            // 检查堆肥是否成功（等级是否提升）
            i32 newLevel = blocks::ComposterBlock::getLevel(newState);
            if (newLevel > level) {
                level = newLevel;
                // 更新当前状态
                state = world->getBlockState(pos);
                if (!state) break;

                // 等级达到7时停止（即将完成）
                if (level >= 7) {
                    // 从背包移除已堆肥的种子数量
                    i32 consumed = i + 1;
                    inventory.removeItem(slot, consumed);
                    return;
                }
            }
        }

        // 从背包移除已堆肥的种子数量
        inventory.removeItem(slot, compostCount);
        return;
    }
}

std::optional<BlockPos> FarmerWorkGoal::_findFarmland() const
{
    if (!m_villager || !m_villager->world()) return std::nullopt;

    IWorld* world = m_villager->world();
    i32 cx = static_cast<i32>(m_villager->x());
    i32 cy = static_cast<i32>(m_villager->y());
    i32 cz = static_cast<i32>(m_villager->z());

    std::optional<BlockPos> result;
    i64 closestDistSq = std::numeric_limits<i64>::max();

    // MC 原版搜索 3x3x3 区域
    for (i32 dx = -FARMER_SEARCH_RANGE; dx <= FARMER_SEARCH_RANGE; ++dx) {
        for (i32 dy = -FARMER_SEARCH_RANGE; dy <= FARMER_SEARCH_RANGE; ++dy) {
            for (i32 dz = -FARMER_SEARCH_RANGE; dz <= FARMER_SEARCH_RANGE; ++dz) {
                BlockPos checkPos(cx + dx, cy + dy, cz + dz);
                const BlockState* state = world->getBlockState(checkPos);
                if (!state) continue;

                // 检查是否是耕地
                if (!state->is(VanillaBlocks::FARMLAND)) continue;

                i64 distSq = static_cast<i64>(dx * dx + dy * dy + dz * dz);
                if (distSq < closestDistSq) {
                    closestDistSq = distSq;
                    result = checkPos;
                }
            }
        }
    }

    return result;
}

bool FarmerWorkGoal::_isCropMature(BlockPos pos) const
{
    if (!m_villager || !m_villager->world()) return false;

    return _isCropMatureAt(pos);
}

bool FarmerWorkGoal::_canPlant(BlockPos pos) const
{
    if (!m_villager || !m_villager->world()) return false;

    return _canPlantAt(pos);
}

bool FarmerWorkGoal::_hasFarmSeeds() const
{
    if (!m_villager) return false;

    IInventory& inventory = m_villager->inventory();

    // MC 原版 VILLAGER_PLANTABLE_SEEDS 标签包含的物品：
    // 小麦种子、胡萝卜、马铃薯、甜菜种子
    static const Item* plantableSeeds[] = {
        Items::WHEAT_SEEDS,
        Items::CARROT,
        Items::POTATO,
        Items::BEETROOT_SEEDS,
    };

    for (i32 slot = 0; slot < inventory.getContainerSize(); ++slot) {
        ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) continue;

        const Item* item = stack.getItem();
        if (!item) continue;

        for (const Item* seed : plantableSeeds) {
            if (item == seed) return true;
        }
    }

    return false;
}

bool FarmerWorkGoal::_isCropMatureAt(const BlockPos& pos) const
{
    IWorld* world = m_villager->world();
    if (!world) return false;

    const BlockState* state = world->getBlockState(pos);
    if (!state) return false;

    // 检查是否是 CropBlock 且已成熟
    const Block& block = state->getBlock();
    auto* cropBlock = dynamic_cast<const blocks::CropBlock*>(&block);
    if (!cropBlock) return false;

    return cropBlock->isMaxAge(*state);
}

bool FarmerWorkGoal::_canPlantAt(const BlockPos& pos) const
{
    IWorld* world = m_villager->world();
    if (!world) return false;

    // 检查目标位置是否为空气或可替换
    const BlockState* state = world->getBlockState(pos);
    if (!state) return false;
    if (!state->isAir() && !state->canBeReplaced()) return false;

    // 检查下方是否是耕地
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world->getBlockState(belowPos);
    if (!belowState) return false;

    return belowState->is(VanillaBlocks::FARMLAND);
}

bool FarmerWorkGoal::_isValidFarmPos(const BlockPos& pos) const
{
    // 对应 MC HarvestFarmland.validPos()：
    // 1. 位置是 CropBlock 且已成熟（可收获）
    // 2. 位置是空气且下方是耕地（可种植）
    return _isCropMatureAt(pos) || _canPlantAt(pos);
}

std::optional<BlockPos> FarmerWorkGoal::_pickValidFarmland() const
{
    if (!m_villager || !m_villager->world()) return std::nullopt;

    IWorld* world = m_villager->world();
    i32 cx = static_cast<i32>(m_villager->x());
    i32 cy = static_cast<i32>(m_villager->y());
    i32 cz = static_cast<i32>(m_villager->z());

    // MC 原版使用蓄水池抽样算法随机选取有效位置
    i32 count = 0;
    std::optional<BlockPos> result;

    math::Random rng = m_villager->getRandom();

    for (i32 dx = -FARMER_SEARCH_RANGE; dx <= FARMER_SEARCH_RANGE; ++dx) {
        for (i32 dy = -FARMER_SEARCH_RANGE; dy <= FARMER_SEARCH_RANGE; ++dy) {
            for (i32 dz = -FARMER_SEARCH_RANGE; dz <= FARMER_SEARCH_RANGE; ++dz) {
                BlockPos checkPos(cx + dx, cy + dy, cz + dz);
                if (_isValidFarmPos(checkPos)) {
                    // 蓄水池抽样：以 1/(count+1) 的概率替换当前结果
                    if (rng.nextInt(++count) == 0) {
                        result = checkPos;
                    }
                }
            }
        }
    }

    return result;
}

// ============================================================================
// AvoidHostileGoal - 村民逃避敌对目标
// ============================================================================

AvoidHostileGoal::AvoidHostileGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_hostileEntity(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool AvoidHostileGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 查找附近的敌对生物
    _findNearestHostile();
    return m_hostileEntity != 0;
}

bool AvoidHostileGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 敌对生物消失或距离足够远
    if (m_hostileEntity == 0) return false;

    // 检查敌对生物是否仍然存在
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_hostileEntity) : nullptr;
    if (!entity) {
        m_hostileEntity = 0;
        return false;
    }

    LivingEntity* hostile = dynamic_cast<LivingEntity*>(entity);
    if (!hostile || !hostile->isAlive()) {
        m_hostileEntity = 0;
        return false;
    }

    // 检查距离，如果敌对生物已经足够远，停止逃跑
    f32 distSq = m_villager->distanceSqTo(*hostile);
    if (distSq > FLEE_DISTANCE * FLEE_DISTANCE * 4.0f) { // 超过逃跑距离的2倍
        m_hostileEntity = 0;
        return false;
    }

    return true;
}

void AvoidHostileGoal::startExecuting()
{
    _fleeFromHostile();
}

void AvoidHostileGoal::resetTask()
{
    m_hostileEntity = 0;
    m_fleeTarget = BlockPos::zero();

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void AvoidHostileGoal::tick()
{
    if (!m_villager || m_hostileEntity == 0) return;

    // 继续逃跑
    _fleeFromHostile();
}

void AvoidHostileGoal::_findNearestHostile()
{
    if (!m_villager || !m_villager->world()) {
        m_hostileEntity = 0;
        return;
    }

    m_hostileEntity = 0;

    // 使用 EntityUtils 查找最近的敌对生物
    // 村民逃离僵尸、掠夺者、劫掠兽、恼鬼等
    LivingEntity* hostile = EntityUtils::findClosestEntity<LivingEntity>(
        m_villager->world(), m_villager->position(), FLEE_RANGE, m_villager, [](LivingEntity* entity) {
            // 检查是否存活
            if (!entity || !entity->isAlive()) return false;

            // 使用 IMob 接口判断是否是敌对生物
            // IMob 是敌对生物的标记接口
            IMob* mob = dynamic_cast<IMob*>(entity);
            return mob != nullptr;
        });

    if (hostile) {
        m_hostileEntity = hostile->id();
    }
}

void AvoidHostileGoal::_fleeFromHostile()
{
    if (!m_villager || m_hostileEntity == 0) return;

    // 获取敌对生物位置
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_hostileEntity) : nullptr;
    if (!entity) {
        m_hostileEntity = 0;
        return;
    }

    LivingEntity* hostile = dynamic_cast<LivingEntity*>(entity);
    if (!hostile || !hostile->isAlive()) {
        m_hostileEntity = 0;
        return;
    }

    // 计算逃跑方向（远离敌对生物）
    f32 dx = m_villager->x() - hostile->x();
    f32 dz = m_villager->z() - hostile->z();

    // 归一化方向向量
    f32 dist = std::sqrt(dx * dx + dz * dz);
    if (dist < 0.001f) {
        // 距离太近，随机选择方向
        math::Random rng = m_villager->getRandom();
        f32 angle = rng.nextFloat() * math::TWO_PI;
        dx = std::cos(angle);
        dz = std::sin(angle);
    } else {
        dx /= dist;
        dz /= dist;
    }

    // 计算目标位置（逃跑方向）
    f32 targetX = m_villager->x() + dx * FLEE_DISTANCE;
    f32 targetZ = m_villager->z() + dz * FLEE_DISTANCE;
    f32 targetY = m_villager->y();

    m_villager->tryMoveTo(targetX, targetY, targetZ, FLEE_SPEED);
}

// ============================================================================
// GoToBedGoal - 村民前往床位目标
// ============================================================================

GoToBedGoal::GoToBedGoal(VillagerEntity* villager)
    : m_villager(villager)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool GoToBedGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 检查是否是夜间
    if (!m_villager->world()) return false;
    if (!isNightTime(m_villager->world()->dayTimeOfDay())) return false;

    // 已经在睡眠中则不重新开始
    if (m_villager->isSleeping()) return false;

    // 检查是否有绑定的床位（从Brain的HOME记忆获取）
    // HOME记忆使用GlobalPos类型，包含维度信息
    auto homeMemory = m_villager->brain().getMemory<GlobalPos>(ai::brain::memory::MemoryModuleTypes::HOME);
    if (homeMemory.has_value()) {
        // 检查是否在同一维度
        if (m_villager->world() && homeMemory->getDimensionId() == m_villager->world()->dimension()) {
            m_bedPos = homeMemory->getPos();
            return true;
        }
    }

    // 没有绑定床位，尝试查找最近的可用床
    // 村民会搜索48格范围内的床
    auto* villageManager = m_villager->world()->villageManager();
    if (!villageManager) return false;

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(
        static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));
    constexpr f32 BED_SEARCH_RANGE = 48.0f;

    // 遍历所有床类型
    using namespace world::village::poi;
    for (i32 bedType = static_cast<i32>(PointOfInterestType::BedRed);
        bedType <= static_cast<i32>(PointOfInterestType::BedYellow);
        ++bedType) {

        PointOfInterestType poiType = static_cast<PointOfInterestType>(bedType);
        auto result = poiStorage.findNearestFree(villagerPos, poiType, BED_SEARCH_RANGE);

        if (result.has_value()) {
            m_bedPos = result.value();
            return true;
        }
    }

    return false;
}

bool GoToBedGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    return !m_reachedBed;
}

void GoToBedGoal::startExecuting()
{
    m_reachedBed = false;

    if (m_villager && m_bedPos != BlockPos::zero()) {
        m_villager->tryMoveTo(m_bedPos.x + 0.5, m_bedPos.y, m_bedPos.z + 0.5, SPEED_MODIFIER);
    }
}

void GoToBedGoal::resetTask()
{
    m_reachedBed = false;
    m_bedPos = BlockPos::zero();

    if (m_villager) {
        // 如果正在睡眠，停止睡眠
        if (m_villager->isSleeping()) {
            m_villager->stopSleeping();
        }
        m_villager->clearNavigation();
    }
}

void GoToBedGoal::tick()
{
    if (!m_villager) return;

    if (m_bedPos == BlockPos::zero()) return;

    // 检查是否到达床位
    if (isWithinDistance(m_villager, m_bedPos, 1.5f)) {
        m_reachedBed = true;

        // 检查床位是否仍然有效
        const auto* blockState = m_villager->world()->getBlockState(m_bedPos);
        if (blockState && mc::blocks::BedBlock::isBed(*m_villager->world(), m_bedPos)) {
            // 通过VillageManager获取POI存储，占用床位
            auto* villageManager = m_villager->world()->villageManager();
            if (villageManager) {
                auto& poiStorage = villageManager->getPOIStorage();

                // 检查床位是否已被占用
                const auto* poi = poiStorage.getPOI(m_bedPos);
                if (!poi || !poi->isOccupied()) {
                    // 占用床位
                    poiStorage.acquirePOI(
                        m_bedPos, static_cast<u64>(m_villager->id()), m_villager->world()->currentTick());

                    // 开始睡眠
                    m_villager->startSleeping(m_bedPos);

                    // 将床位保存到Brain的HOME记忆（使用GlobalPos包含维度信息）
                    GlobalPos homePos(m_villager->world()->dimension(), m_bedPos);
                    m_villager->brain().setMemory(ai::brain::memory::MemoryModuleTypes::HOME, homePos);
                }
            }
        }
    } else {
        m_villager->tryMoveTo(m_bedPos.x + 0.5, m_bedPos.y, m_bedPos.z + 0.5, SPEED_MODIFIER);
    }
}

// ============================================================================
// VillagerBreedGoal - 村民繁殖目标
// ============================================================================

VillagerBreedGoal::VillagerBreedGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_partnerId(0)
    , m_breedTicks(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool VillagerBreedGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 检查是否愿意繁殖
    if (!_isWillingToBreed()) return false;

    // 检查床位
    if (!_hasEnoughBeds()) return false;

    // 寻找配偶
    _findPartner();
    return m_partnerId != 0;
}

bool VillagerBreedGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 配偶消失或不再愿意繁殖
    if (m_partnerId == 0) return false;

    // 超时
    return m_breedTicks < BREED_TICKS;
}

void VillagerBreedGoal::startExecuting()
{
    m_breedTicks = 0;
}

void VillagerBreedGoal::resetTask()
{
    m_partnerId = 0;
    m_breedTicks = 0;

    if (m_villager) {
        m_villager->clearNavigation();
        // 重置繁殖意愿
        m_villager->resetBreedWillingness();
    }
}

void VillagerBreedGoal::tick()
{
    if (!m_villager || m_partnerId == 0) return;

    m_breedTicks++;

    // 移动到配偶
    _moveToPartner();

    // 检查配偶是否仍然有效
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_partnerId) : nullptr;
    if (!entity) {
        m_partnerId = 0;
        return;
    }

    VillagerEntity* partner = dynamic_cast<VillagerEntity*>(entity);
    if (!partner || !partner->isAlive()) {
        m_partnerId = 0;
        return;
    }

    // 检查距离，足够接近时繁殖
    f32 distSq = m_villager->distanceSqTo(*partner);
    if (distSq <= BREED_DISTANCE * BREED_DISTANCE && m_breedTicks >= BREED_TICKS) {
        _spawnChild();
    }
}

bool VillagerBreedGoal::_hasEnoughBeds() const
{
    if (!m_villager) return false;

    // 检查村庄中是否有足够的床位
    // 通过VillageManager获取POI存储，统计可用床位数
    auto* villageManager = m_villager->world()->villageManager();
    if (!villageManager) {
        // 没有VillageManager时，默认允许繁殖
        return true;
    }

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(
        static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));

    // 搜索48格范围内的所有床
    using namespace world::village::poi;
    constexpr f32 BED_SEARCH_RANGE = 48.0f;
    i32 availableBeds = 0;

    // 遍历所有床类型
    for (i32 bedType = static_cast<i32>(PointOfInterestType::BedRed);
        bedType <= static_cast<i32>(PointOfInterestType::BedYellow);
        ++bedType) {

        PointOfInterestType poiType = static_cast<PointOfInterestType>(bedType);
        auto pois = poiStorage.findAllInRange(villagerPos, BED_SEARCH_RANGE, poiType);

        for (const auto* poi : pois) {
            if (poi && !poi->isOccupied()) {
                availableBeds++;
            }
        }
    }

    // 需要至少有1个可用床位才能繁殖
    return availableBeds > 0;
}

bool VillagerBreedGoal::_isWillingToBreed() const
{
    if (!m_villager) return false;

    return m_villager->isWillingToBreed();
}

void VillagerBreedGoal::_findPartner()
{
    if (!m_villager || !m_villager->world()) {
        m_partnerId = 0;
        return;
    }

    m_partnerId = 0;

    // 搜索附近愿意繁殖的村民
    static constexpr f32 PARTNER_SEARCH_RANGE = 8.0f;

    VillagerEntity* partner = EntityUtils::findClosestEntity<VillagerEntity>(m_villager->world(),
        m_villager->position(),
        PARTNER_SEARCH_RANGE,
        m_villager,
        [this](VillagerEntity* candidate) {
            if (!candidate || !candidate->isAlive()) return false;

            // 检查对方是否也愿意繁殖
            if (!candidate->isWillingToBreed()) return false;

            // 检查是否是成年村民（不是幼年）
            if (candidate->isChild()) return false;

            return true;
        });

    if (partner) {
        m_partnerId = partner->id();
    }
}

void VillagerBreedGoal::_moveToPartner()
{
    if (!m_villager || m_partnerId == 0) return;

    // 获取配偶实体
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_partnerId) : nullptr;
    if (!entity) {
        m_partnerId = 0;
        return;
    }

    VillagerEntity* partner = dynamic_cast<VillagerEntity*>(entity);
    if (!partner || !partner->isAlive()) {
        m_partnerId = 0;
        return;
    }

    // 移动到配偶位置
    // 使用 0.5 的移动速度
    static constexpr f32 BREED_SPEED = 0.5f;
    m_villager->tryMoveTo(partner->x(), partner->y(), partner->z(), BREED_SPEED);
}

void VillagerBreedGoal::_spawnChild()
{
    if (!m_villager) return;

    // 生成幼年村民
    auto child = m_villager->createChild();
    if (child && m_villager->world()) {
        child->setPosition(m_villager->x(), m_villager->y(), m_villager->z());
        m_villager->world()->spawnEntity(std::move(child));
    }

    // 重置
    m_partnerId = 0;
    m_breedTicks = 0;
    m_villager->resetBreedWillingness();
}

// ============================================================================
// CongregateGoal - 村民聚集目标
// ============================================================================

CongregateGoal::CongregateGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_targetVillagerId(0)
    , m_interactCooldown(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool CongregateGoal::shouldExecute()
{
    if (!m_villager || !m_villager->world()) return false;

    // 检查是否有会议点（从 Brain 的 MEETING_POINT 记忆获取）
    auto meetingPoint = m_villager->brain().getMemory<GlobalPos>(ai::brain::memory::MemoryModuleTypes::MEETING_POINT);
    if (!meetingPoint.has_value()) return false;

    // 检查是否在会议点附近
    BlockPos meetingPos = meetingPoint->getPos();
    f32 distSq = m_villager->distanceSqTo(meetingPos.x + 0.5f, static_cast<f32>(meetingPos.y), meetingPos.z + 0.5f);
    if (distSq > 16.0f * 16.0f) return false; // 超过16格

    // 小概率触发（1%）
    math::Random rng = m_villager->getRandom();
    if (rng.nextInt(100) != 0) return false;

    // 查找附近的其他村民
    _findInteractionTarget();
    return m_targetVillagerId != 0;
}

bool CongregateGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 检查目标是否仍然有效
    if (m_targetVillagerId == 0) return false;

    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) {
        m_targetVillagerId = 0;
        return false;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_targetVillagerId = 0;
        return false;
    }

    return m_interactCooldown > 0;
}

void CongregateGoal::startExecuting()
{
    m_interactCooldown = INTERACTION_DURATION;

    // 设置交互目标
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (entity) {
        // 移动到目标
        m_villager->tryMoveTo(entity->x(), entity->y(), entity->z(), 0.3f);
    }
}

void CongregateGoal::resetTask()
{
    m_targetVillagerId = 0;
    m_interactCooldown = 0;

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void CongregateGoal::tick()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    m_interactCooldown--;

    // 获取目标村民
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) {
        m_targetVillagerId = 0;
        return;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_targetVillagerId = 0;
        return;
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*target);

    // 在交互距离内
    if (distSq <= INTERACTION_DISTANCE * INTERACTION_DISTANCE) {
        // 看向目标
        if (auto* lookCtrl = m_villager->lookController()) {
            lookCtrl->setLookPosition(target->x(), target->y() + target->eyeHeight(), target->z());
        }

        // 传播流言
        _spreadGossip();

        // 分享物品（农民分享食物）
        _shareItems();
    } else {
        // 继续移动到目标
        m_villager->tryMoveTo(target->x(), target->y(), target->z(), 0.3f);
    }
}

void CongregateGoal::_findInteractionTarget()
{
    if (!m_villager || !m_villager->world()) {
        m_targetVillagerId = 0;
        return;
    }

    m_targetVillagerId = 0;

    // 查找附近的其他村民
    static constexpr f32 SEARCH_RANGE = 32.0f;

    VillagerEntity* target = EntityUtils::findClosestEntity<VillagerEntity>(
        m_villager->world(), m_villager->position(), SEARCH_RANGE, m_villager, [](VillagerEntity* entity) {
            return entity && entity->isAlive();
        });

    if (target) {
        m_targetVillagerId = target->id();
    }
}

void CongregateGoal::_spreadGossip()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    // 获取目标村民
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) return;

    VillagerEntity* targetVillager = dynamic_cast<VillagerEntity*>(entity);
    if (!targetVillager) return;

    // 调用村民的流言传播方法
    m_villager->spreadGossipTo(targetVillager);
}

void CongregateGoal::_shareItems()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    // 只有农民职业会分享食物
    if (m_villager->profession() != VillagerProfession::Farmer) return;

    // 获取目标村民
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) return;

    VillagerEntity* targetVillager = dynamic_cast<VillagerEntity*>(entity);
    if (!targetVillager || !targetVillager->isAlive()) return;

    // 农民分享食物逻辑：食物过剩时分享食物，小麦超过半组时分享小麦
    IInventory& inventory = m_villager->inventory();

    // 1. 食物分享：农民有食物过剩时分享给目标
    //    农民无条件分享给任意村民；非农民只在目标需要食物时分享
    if (m_villager->hasExcessFood()) {
        throwHalfStackToTarget(m_villager, inventory, VillagerEntity::foodPoints(), targetVillager);
        return;
    }

    // 2. 小麦分享：农民有超过半组小麦时，抛出一半
    i32 wheatCount = inventory.countItem(*Items::WHEAT);
    if (wheatCount > Items::WHEAT->maxStackSize() / 2) {
        std::unordered_map<const Item*, i32> wheatOnly = {{Items::WHEAT, 1}};
        throwHalfStackToTarget(m_villager, inventory, wheatOnly, targetVillager);
    }
}

// ============================================================================
// LookAtEntitiesGoal - 村民看向实体目标
// ============================================================================

LookAtEntitiesGoal::LookAtEntitiesGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_lookTargetId(0)
    , m_lookTime(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

bool LookAtEntitiesGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 概率检查
    math::Random rng = m_villager->getRandom();
    if (rng.nextFloat() >= LOOK_CHANCE) return false;

    // 随机选择目标类型
    _selectTargetType();

    // 查找对应类型的实体
    LivingEntity* target = nullptr;
    switch (m_targetType) {
        case TargetType::Villager:
            target = EntityUtils::findClosestEntity<VillagerEntity>(
                m_villager->world(), m_villager->position(), LOOK_RANGE, m_villager, [](LivingEntity* entity) {
                    return entity && entity->isAlive();
                });
            break;
        case TargetType::Player:
        case TargetType::Cat:
        case TargetType::Creature:
            target = EntityUtils::findClosestEntity<LivingEntity>(
                m_villager->world(), m_villager->position(), LOOK_RANGE, m_villager, [](LivingEntity* entity) {
                    return entity && entity->isAlive();
                });
            break;
    }

    if (target) {
        m_lookTargetId = target->id();
        // 设置看向时间
        math::Random rng2 = m_villager->getRandom();
        m_lookTime = LOOK_MIN_TIME + rng2.nextInt(LOOK_MAX_TIME - LOOK_MIN_TIME);
        return true;
    }

    return false;
}

bool LookAtEntitiesGoal::shouldContinueExecuting()
{
    if (!m_villager || m_lookTargetId == 0) return false;

    // 获取目标实体
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_lookTargetId) : nullptr;
    if (!entity) {
        m_lookTargetId = 0;
        return false;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_lookTargetId = 0;
        return false;
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*target);
    if (distSq > LOOK_RANGE * LOOK_RANGE) return false;

    return m_lookTime > 0;
}

void LookAtEntitiesGoal::startExecuting()
{
    // 开始看向目标
}

void LookAtEntitiesGoal::resetTask()
{
    m_lookTargetId = 0;
    m_lookTime = 0;
}

void LookAtEntitiesGoal::tick()
{
    if (!m_villager || m_lookTargetId == 0) return;

    m_lookTime--;

    // 获取目标实体
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_lookTargetId) : nullptr;
    if (!entity) {
        m_lookTargetId = 0;
        return;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_lookTargetId = 0;
        return;
    }

    // 使用 LookController 看向目标
    if (auto* lookCtrl = m_villager->lookController()) {
        lookCtrl->setLookPosition(target->x(), target->y() + target->eyeHeight(), target->z());
    }
}

void LookAtEntitiesGoal::_selectTargetType()
{
    // 猫: 8, 村民: 2, 玩家: 2, 生物: 1
    math::Random rng = m_villager->getRandom();
    i32 rand = rng.nextInt(13); // 8 + 2 + 2 + 1 = 13

    if (rand < 8) {
        m_targetType = TargetType::Cat;
    } else if (rand < 10) {
        m_targetType = TargetType::Villager;
    } else if (rand < 12) {
        m_targetType = TargetType::Player;
    } else {
        m_targetType = TargetType::Creature;
    }
}

// ============================================================================
// ShareItemsGoal - 分享物品目标
// ============================================================================

ShareItemsGoal::ShareItemsGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_targetVillagerId(0)
    , m_shareCooldown(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool ShareItemsGoal::shouldExecute()
{
    if (!m_villager || !m_villager->world()) return false;

    // 只有农民职业会分享食物
    if (m_villager->profession() != VillagerProfession::Farmer) return false;

    // 冷却时间
    if (m_shareCooldown > 0) return false;

    // 检查是否有多余的食物可以分享（食物点数 >= 24 或小麦超过半组）    if (!_canAbandonItems()) return false;

    // 查找附近需要食物的村民
    // 农民只要有食物过剩就会分享给任意村民（无论对方是否需要），
    // 但也可以优先选择需要食物的村民
    static constexpr f32 SEARCH_RANGE = 8.0f;

    // 先尝试找到需要食物的村民
    VillagerEntity* target = EntityUtils::findClosestEntity<VillagerEntity>(
        m_villager->world(), m_villager->position(), SEARCH_RANGE, m_villager, [this](VillagerEntity* entity) {
            return entity && entity->isAlive() && _targetNeedsFoodForTarget(entity);
        });

    // 如果没有需要食物的村民，农民也会分享给任意村民
    if (!target) {
        target = EntityUtils::findClosestEntity<VillagerEntity>(
            m_villager->world(), m_villager->position(), SEARCH_RANGE, m_villager, [](VillagerEntity* entity) {
                return entity && entity->isAlive();
            });
    }

    if (target) {
        m_targetVillagerId = target->id();
        return true;
    }

    return false;
}

bool ShareItemsGoal::shouldContinueExecuting()
{
    if (!m_villager || m_targetVillagerId == 0) return false;

    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) {
        m_targetVillagerId = 0;
        return false;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_targetVillagerId = 0;
        return false;
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*target);
    return distSq <= SHARE_DISTANCE * SHARE_DISTANCE * 4.0f;
}

void ShareItemsGoal::startExecuting()
{
    m_shareCooldown = SHARE_COOLDOWN;

    // 移动到目标
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (entity) {
        m_villager->tryMoveTo(entity->x(), entity->y(), entity->z(), 0.5f);
    }
}

void ShareItemsGoal::resetTask()
{
    m_targetVillagerId = 0;

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void ShareItemsGoal::tick()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    if (m_shareCooldown > 0) {
        m_shareCooldown--;
    }

    // 获取目标
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_targetVillagerId) : nullptr;
    if (!entity) {
        m_targetVillagerId = 0;
        return;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_targetVillagerId = 0;
        return;
    }

    // 看向目标
    if (auto* lookCtrl = m_villager->lookController()) {
        lookCtrl->setLookPosition(target->x(), target->y() + target->eyeHeight(), target->z());
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*target);
    if (distSq <= SHARE_DISTANCE * SHARE_DISTANCE) {
        // 分享食物
        _shareFoodWithTarget();
    } else {
        // 继续移动
        m_villager->tryMoveTo(target->x(), target->y(), target->z(), 0.5f);
    }
}

bool ShareItemsGoal::_canAbandonItems() const
{
    if (!m_villager) return false;

    // 1. 如果村民有食物过剩（食物点数 >= 24），则可以分享食物
    // 2. 如果是农民且小麦超过半组（>32），则可以分享小麦
    if (m_villager->hasExcessFood()) {
        return true;
    }

    // 农民特殊检查：小麦超过半组时也愿意分享
    if (m_villager->profession() == VillagerProfession::Farmer) {
        IInventory& inventory = m_villager->inventory();
        i32 wheatCount = inventory.countItem(*Items::WHEAT);
        if (wheatCount > Items::WHEAT->maxStackSize() / 2) {
            return true;
        }
    }

    return false;
}

bool ShareItemsGoal::_targetNeedsFoodForTarget(VillagerEntity* target) const
{
    // 检查目标村民是否需要食物（食物点数 < 12）
    if (!target) return false;
    return target->wantsMoreFood();
}

void ShareItemsGoal::_shareFoodWithTarget()
{
    if (!m_villager || m_targetVillagerId == 0) return;

    IWorld* world = m_villager->world();
    if (!world) return;

    // 获取目标
    Entity* entity = world->getEntity(m_targetVillagerId);
    if (!entity) {
        m_targetVillagerId = 0;
        m_shareCooldown = SHARE_COOLDOWN;
        return;
    }

    VillagerEntity* targetVillager = dynamic_cast<VillagerEntity*>(entity);
    if (!targetVillager) {
        m_targetVillagerId = 0;
        m_shareCooldown = SHARE_COOLDOWN;
        return;
    }

    // 物品分享逻辑：按优先级依次检查食物分享 -> 小麦分享

    IInventory& inventory = m_villager->inventory();
    bool shared = false;

    // 1. 食物分享：如果有食物过剩，向目标抛出一半食物
    if (m_villager->hasExcessFood()) {
        shared = throwHalfStackToTarget(m_villager, inventory, VillagerEntity::foodPoints(), targetVillager);
    }

    // 2. 小麦分享：农民有超过半组小麦时，抛出一半
    if (!shared && m_villager->profession() == VillagerProfession::Farmer) {
        i32 wheatCount = inventory.countItem(*Items::WHEAT);
        i32 halfStack = Items::WHEAT->maxStackSize() / 2;
        if (wheatCount > halfStack) {
            std::unordered_map<const Item*, i32> wheatOnly = {{Items::WHEAT, 1}};
            shared = throwHalfStackToTarget(m_villager, inventory, wheatOnly, targetVillager);
        }
    }

    // 重置目标
    m_targetVillagerId = 0;
    m_shareCooldown = SHARE_COOLDOWN;
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
