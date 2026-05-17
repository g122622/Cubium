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
#include "../../../../../item/Items.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/GlobalPos.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../../../../world/block/blocks/functional/BedBlock.hpp"
#include "../../../../../world/village/VillageManager.hpp"
#include "../../../../../world/village/poi/PointOfInterestStorage.hpp"
#include "../../../../../world/village/poi/PointOfInterestType.hpp"
#include "../../../../core/EntityUtils.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/item/ItemEntity.hpp"
#include "../../../../entities/monster/MonsterEntity.hpp"
#include "../../../../entities/villager/VillagerEntity.hpp"
#include "../../../../interfaces/IMob.hpp"
#include "../../../brain/memory/MemoryModuleType.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../GoalConstants.hpp"
#include <cmath>

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
 * 参考 MC 1.16.5 时间系统
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
 * 参考 MC 1.16.5 Schedule
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
    if (!isNightTime()) return false;

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
    auto bedPos = findNearestBed();
    if (!bedPos.has_value()) return false;

    m_bedPos = bedPos.value();
    return true;
}

bool SleepAtNightGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 继续执行直到天亮或床位不可用
    if (!isNightTime()) return false;

    // 如果正在睡眠，继续睡眠直到天亮
    if (m_villager->isSleeping()) return true;

    // 检查床位是否仍然有效
    if (!isBedStillValid()) return false;

    return m_sleeping || m_trySleepTicks < MAX_TRY_SLEEP_TICKS;
}

void SleepAtNightGoal::startExecuting()
{
    m_sleeping = false;
    m_trySleepTicks = 0;
    moveToBed();
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
        trySleep();
    } else if (!m_sleeping) {
        // 继续移动到床位
        moveToBed();
    }
}

bool SleepAtNightGoal::isNightTime() const
{
    if (!m_villager || !m_villager->world()) return false;
    return villager::isNightTime(m_villager->world()->dayTime());
}

std::optional<BlockPos> SleepAtNightGoal::findNearestBed() const
{
    if (!m_villager || !m_villager->world()) return std::nullopt;

    // 通过VillageManager获取POI存储
    auto* villageManager = m_villager->world()->villageManager();
    if (!villageManager) return std::nullopt;

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));

    // 参考 MC 1.16.5: 村民搜索床位的范围是48格
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

void SleepAtNightGoal::moveToBed()
{
    if (!m_villager) return;

    m_villager->tryMoveTo(m_bedPos.x + 0.5, m_bedPos.y, m_bedPos.z + 0.5, 0.5);
}

void SleepAtNightGoal::trySleep()
{
    if (!m_villager) return;

    // 检查床位是否仍然有效
    if (!isBedStillValid()) {
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
            auto newBedPos = findNearestBed();
            if (newBedPos.has_value()) {
                m_bedPos = newBedPos.value();
                moveToBed();
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

bool SleepAtNightGoal::isBedStillValid() const
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
    if (!isWorkTime()) return false;

    // 检查是否有工作站点
    return hasJobSite();
}

bool WorkAtJobSiteGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 继续工作的条件
    if (!isWorkTime()) return false;
    if (!hasJobSite()) return false;

    // 限制工作时间
    return m_workTicks < WORK_TICKS_MAX;
}

void WorkAtJobSiteGoal::startExecuting()
{
    m_workTicks = 0;
    m_atJobSite = false;
    moveToJobSite();
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
        doWork();
    } else {
        m_atJobSite = false;
        moveToJobSite();
    }

    // 检查补货
    if (needsRestock()) {
        restock();
    }
}

bool WorkAtJobSiteGoal::isWorkTime() const
{
    if (!m_villager || !m_villager->world()) return false;
    return villager::isWorkTime(m_villager->world()->dayTime());
}

bool WorkAtJobSiteGoal::hasJobSite() const
{
    if (!m_villager) return false;
    return m_villager->workStation() != BlockPos::zero();
}

void WorkAtJobSiteGoal::moveToJobSite()
{
    if (!m_villager) return;

    BlockPos workPos = m_villager->workStation();
    m_villager->tryMoveTo(workPos.x + 0.5, workPos.y, workPos.z + 0.5, 0.4);
}

void WorkAtJobSiteGoal::doWork()
{
    if (!m_villager) return;

    // 设置工作状态
    m_villager->work();

    // 每隔一段时间增加经验
    if (m_workTicks % 100 == 0) {
        m_villager->addVillagerExperience(1);
    }
}

bool WorkAtJobSiteGoal::needsRestock() const
{
    if (!m_villager) return false;

    // 检查交易是否需要补货
    // TODO: 检查交易使用次数
    return false;
}

void WorkAtJobSiteGoal::restock()
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
    searchForJobSite();
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

void LookForJobSiteGoal::searchForJobSite()
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
    findNearbyItems();
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
    moveToItem();

    // 尝试拾取
    pickupItem();
}

void GatherItemsGoal::findNearbyItems()
{
    if (!m_villager || !m_villager->world()) {
        m_targetItem = 0;
        return;
    }

    m_targetItem = 0;

    // 使用 EntityUtils 查找最近的 ItemEntity
    // 参考 MC 1.16.5 VillagerEntity 的拾取逻辑
    ItemEntity* item = EntityUtils::findClosestEntity<ItemEntity>(
        m_villager->world(),
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

void GatherItemsGoal::moveToItem()
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
    // 参考 MC 1.16.5: 村民移动速度约 0.5
    m_villager->tryMoveTo(item->x(), item->y(), item->z(), 0.5);
}

void GatherItemsGoal::pickupItem()
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
    // 参考 MC 1.16.5 VillagerEntity.updateEquipmentIfNeeded()
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
        tryHarvest();

        // 尝试种植
        tryPlant();

        // 尝试堆肥
        tryCompost();
    }
}

void FarmerWorkGoal::tryHarvest()
{
    if (!m_villager || !m_villager->world()) return;

    // TODO: 查找成熟作物并收获
}

void FarmerWorkGoal::tryPlant()
{
    if (!m_villager || !m_villager->world()) return;

    // TODO: 在农田上种植作物
}

void FarmerWorkGoal::tryCompost()
{
    if (!m_villager) return;

    // TODO: 使用堆肥桶
}

std::optional<BlockPos> FarmerWorkGoal::findFarmland() const
{
    if (!m_villager || !m_villager->world()) return std::nullopt;

    // TODO: 搜索附近的农田
    return std::nullopt;
}

bool FarmerWorkGoal::isCropMature(BlockPos pos) const
{
    if (!m_villager || !m_villager->world()) return false;

    // TODO: 检查作物是否成熟
    (void)pos;
    return false;
}

bool FarmerWorkGoal::canPlant(BlockPos pos) const
{
    if (!m_villager || !m_villager->world()) return false;

    // TODO: 检查是否可以种植
    (void)pos;
    return false;
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
    findNearestHostile();
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
    fleeFromHostile();
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
    fleeFromHostile();
}

void AvoidHostileGoal::findNearestHostile()
{
    if (!m_villager || !m_villager->world()) {
        m_hostileEntity = 0;
        return;
    }

    m_hostileEntity = 0;

    // 使用 EntityUtils 查找最近的敌对生物
    // 参考 MC 1.16.5: 村民逃离僵尸、掠夺者、劫掠兽、恼鬼等
    LivingEntity* hostile = EntityUtils::findClosestEntity<LivingEntity>(
        m_villager->world(),
        m_villager->position(),
        FLEE_RANGE,
        m_villager,
        [](LivingEntity* entity) {
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

void AvoidHostileGoal::fleeFromHostile()
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
    // 参考 MC 1.16.5 AvoidEntityGoal
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
    if (!villager::isNightTime(m_villager->world()->dayTime())) return false;

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
    // 参考 MC 1.16.5: 村民会搜索48格范围内的床
    auto* villageManager = m_villager->world()->villageManager();
    if (!villageManager) return false;

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));
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
                    poiStorage.acquirePOI(m_bedPos, static_cast<u64>(m_villager->id()), m_villager->world()->currentTick());

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
    if (!isWillingToBreed()) return false;

    // 检查床位
    if (!hasEnoughBeds()) return false;

    // 寻找配偶
    findPartner();
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
    moveToPartner();

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
        spawnChild();
    }
}

bool VillagerBreedGoal::hasEnoughBeds() const
{
    if (!m_villager) return false;

    // 参考 MC 1.16.5: 检查村庄中是否有足够的床位
    // 通过VillageManager获取POI存储，统计可用床位数
    auto* villageManager = m_villager->world()->villageManager();
    if (!villageManager) {
        // 没有VillageManager时，默认允许繁殖
        return true;
    }

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));

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

bool VillagerBreedGoal::isWillingToBreed() const
{
    if (!m_villager) return false;

    return m_villager->isWillingToBreed();
}

void VillagerBreedGoal::findPartner()
{
    if (!m_villager || !m_villager->world()) {
        m_partnerId = 0;
        return;
    }

    m_partnerId = 0;

    // 参考 MC 1.16.5 BreedGoal.findNearbyMate()
    // 搜索附近愿意繁殖的村民
    static constexpr f32 PARTNER_SEARCH_RANGE = 8.0f;

    VillagerEntity* partner = EntityUtils::findClosestEntity<VillagerEntity>(
        m_villager->world(),
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

void VillagerBreedGoal::moveToPartner()
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
    // 参考 MC 1.16.5 BreedGoal: 使用 0.5 的移动速度
    static constexpr f32 BREED_SPEED = 0.5f;
    m_villager->tryMoveTo(partner->x(), partner->y(), partner->z(), BREED_SPEED);
}

void VillagerBreedGoal::spawnChild()
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

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
