#include "VillagerGoals.hpp"
#include "../../../../entities/villager/VillagerEntity.hpp"
#include "../../../../core/EntityUtils.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../GoalConstants.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../util/math/MathUtils.hpp"
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
[[nodiscard]] bool isNightTime(i64 dayTime) {
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
[[nodiscard]] bool isWorkTime(i64 dayTime) {
    return dayTime >= 2000 && dayTime <= 9000;
}

/**
 * @brief 计算实体到方块位置的距离平方
 * @param entity 实体
 * @param pos 方块位置
 * @return 距离平方（使用方块中心点）
 */
[[nodiscard]] f32 distanceToBlockCenter(const Entity* entity, const BlockPos& pos) {
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
[[nodiscard]] bool isWithinDistance(const Entity* entity, const BlockPos& pos, f32 maxDistance) {
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

bool SleepAtNightGoal::shouldExecute() {
    if (!m_villager) return false;

    // 检查是否是夜间
    if (!isNightTime()) return false;

    // 检查是否有绑定的床位
    // TODO: 集成POI系统检查床位
    auto bedPos = findNearestBed();
    if (!bedPos.has_value()) return false;

    m_bedPos = bedPos.value();
    return true;
}

bool SleepAtNightGoal::shouldContinueExecuting() {
    if (!m_villager) return false;

    // 继续执行直到天亮或床位不可用
    if (!isNightTime()) return false;

    // TODO: 检查床位是否仍然有效
    return m_sleeping || m_trySleepTicks < MAX_TRY_SLEEP_TICKS;
}

void SleepAtNightGoal::startExecuting() {
    m_sleeping = false;
    m_trySleepTicks = 0;
    moveToBed();
}

void SleepAtNightGoal::resetTask() {
    m_sleeping = false;
    m_trySleepTicks = 0;
    m_bedPos = BlockPos::zero();

    if (m_villager) {
        // TODO: 清除睡眠状态
        m_villager->clearNavigation();
    }
}

void SleepAtNightGoal::tick() {
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

bool SleepAtNightGoal::isNightTime() const {
    if (!m_villager || !m_villager->world()) return false;
    return villager::isNightTime(m_villager->world()->dayTime());
}

std::optional<BlockPos> SleepAtNightGoal::findNearestBed() const {
    if (!m_villager || !m_villager->world()) return std::nullopt;

    // TODO: 集成POI系统查询床位
    // 目前返回空，等待POI系统实现
    return std::nullopt;
}

void SleepAtNightGoal::moveToBed() {
    if (!m_villager) return;

    m_villager->tryMoveTo(m_bedPos.x + 0.5, m_bedPos.y, m_bedPos.z + 0.5, 0.5);
}

void SleepAtNightGoal::trySleep() {
    if (!m_villager) return;

    // TODO: 设置村民睡眠状态
    // m_villager->setSleeping(true);
    m_sleeping = true;
}

// ============================================================================
// WorkAtJobSiteGoal - 村民工作目标
// ============================================================================

WorkAtJobSiteGoal::WorkAtJobSiteGoal(VillagerEntity* villager)
    : m_villager(villager)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool WorkAtJobSiteGoal::shouldExecute() {
    if (!m_villager) return false;

    // 傻子村民不工作
    if (m_villager->isNitwit()) return false;

    // 检查是否是工作时间
    if (!isWorkTime()) return false;

    // 检查是否有工作站点
    return hasJobSite();
}

bool WorkAtJobSiteGoal::shouldContinueExecuting() {
    if (!m_villager) return false;

    // 继续工作的条件
    if (!isWorkTime()) return false;
    if (!hasJobSite()) return false;

    // 限制工作时间
    return m_workTicks < WORK_TICKS_MAX;
}

void WorkAtJobSiteGoal::startExecuting() {
    m_workTicks = 0;
    m_atJobSite = false;
    moveToJobSite();
}

void WorkAtJobSiteGoal::resetTask() {
    m_workTicks = 0;
    m_atJobSite = false;

    if (m_villager) {
        m_villager->clearNavigation();
        // 重置工作状态
        m_villager->rest();
    }
}

void WorkAtJobSiteGoal::tick() {
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

bool WorkAtJobSiteGoal::isWorkTime() const {
    if (!m_villager || !m_villager->world()) return false;
    return villager::isWorkTime(m_villager->world()->dayTime());
}

bool WorkAtJobSiteGoal::hasJobSite() const {
    if (!m_villager) return false;
    return m_villager->workStation() != BlockPos::zero();
}

void WorkAtJobSiteGoal::moveToJobSite() {
    if (!m_villager) return;

    BlockPos workPos = m_villager->workStation();
    m_villager->tryMoveTo(workPos.x + 0.5, workPos.y, workPos.z + 0.5, 0.4);
}

void WorkAtJobSiteGoal::doWork() {
    if (!m_villager) return;

    // 设置工作状态
    m_villager->work();

    // 每隔一段时间增加经验
    if (m_workTicks % 100 == 0) {
        m_villager->addVillagerExperience(1);
    }
}

bool WorkAtJobSiteGoal::needsRestock() const {
    if (!m_villager) return false;

    // 检查交易是否需要补货
    // TODO: 检查交易使用次数
    return false;
}

void WorkAtJobSiteGoal::restock() {
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

bool LookForJobSiteGoal::shouldExecute() {
    if (!m_villager) return false;

    // 已有工作站点的不需要寻找
    if (m_villager->workStation() != BlockPos::zero()) return false;

    // 傻子村民不找工作
    if (m_villager->isNitwit()) return false;

    // 冷却时间
    if (m_searchCooldown > 0) return false;

    return true;
}

bool LookForJobSiteGoal::shouldContinueExecuting() {
    if (!m_villager) return false;

    // 找到工作站点或超时
    return !m_targetSite.has_value() && m_searchCooldown < SEARCH_COOLDOWN;
}

void LookForJobSiteGoal::startExecuting() {
    m_targetSite = std::nullopt;
    m_searchCooldown = 0;
    searchForJobSite();
}

void LookForJobSiteGoal::resetTask() {
    m_targetSite = std::nullopt;
    m_searchCooldown = SEARCH_COOLDOWN;

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void LookForJobSiteGoal::tick() {
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

void LookForJobSiteGoal::searchForJobSite() {
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

bool GatherItemsGoal::shouldExecute() {
    if (!m_villager) return false;

    // 查找附近的物品
    findNearbyItems();
    return m_targetItem != 0;
}

bool GatherItemsGoal::shouldContinueExecuting() {
    if (!m_villager) return false;

    // 物品已被拾取或消失
    if (m_targetItem == 0) return false;

    // 检查物品是否还存在
    // TODO: 检查物品实体是否仍然有效
    return true;
}

void GatherItemsGoal::startExecuting() {
    // 已在shouldExecute中找到目标
}

void GatherItemsGoal::resetTask() {
    m_targetItem = 0;
    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void GatherItemsGoal::tick() {
    if (!m_villager || m_targetItem == 0) return;

    // 移动到物品
    moveToItem();

    // 尝试拾取
    pickupItem();
}

void GatherItemsGoal::findNearbyItems() {
    if (!m_villager || !m_villager->world()) return;

    // TODO: 搜索附近的物品实体
    // 目前不实现
    m_targetItem = 0;
}

void GatherItemsGoal::moveToItem() {
    if (!m_villager || m_targetItem == 0) return;

    // TODO: 获取物品位置并移动
}

void GatherItemsGoal::pickupItem() {
    if (!m_villager || m_targetItem == 0) return;

    // TODO: 拾取物品
    m_targetItem = 0;
}

// ============================================================================
// FarmerWorkGoal - 农民工作目标
// ============================================================================

FarmerWorkGoal::FarmerWorkGoal(VillagerEntity* villager)
    : WorkAtJobSiteGoal(villager)
    , m_farmerWorkTicks(0)
{
}

void FarmerWorkGoal::tick() {
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

void FarmerWorkGoal::tryHarvest() {
    if (!m_villager || !m_villager->world()) return;

    // TODO: 查找成熟作物并收获
}

void FarmerWorkGoal::tryPlant() {
    if (!m_villager || !m_villager->world()) return;

    // TODO: 在农田上种植作物
}

void FarmerWorkGoal::tryCompost() {
    if (!m_villager) return;

    // TODO: 使用堆肥桶
}

std::optional<BlockPos> FarmerWorkGoal::findFarmland() const {
    if (!m_villager || !m_villager->world()) return std::nullopt;

    // TODO: 搜索附近的农田
    return std::nullopt;
}

bool FarmerWorkGoal::isCropMature(BlockPos pos) const {
    if (!m_villager || !m_villager->world()) return false;

    // TODO: 检查作物是否成熟
    (void)pos;
    return false;
}

bool FarmerWorkGoal::canPlant(BlockPos pos) const {
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

bool AvoidHostileGoal::shouldExecute() {
    if (!m_villager) return false;

    // 查找附近的敌对生物
    findNearestHostile();
    return m_hostileEntity != 0;
}

bool AvoidHostileGoal::shouldContinueExecuting() {
    if (!m_villager) return false;

    // 敌对生物消失或距离足够远
    if (m_hostileEntity == 0) return false;

    // TODO: 检查敌对生物是否仍然存在和追踪
    return true;
}

void AvoidHostileGoal::startExecuting() {
    fleeFromHostile();
}

void AvoidHostileGoal::resetTask() {
    m_hostileEntity = 0;
    m_fleeTarget = BlockPos::zero();

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void AvoidHostileGoal::tick() {
    if (!m_villager || m_hostileEntity == 0) return;

    // 继续逃跑
    fleeFromHostile();
}

void AvoidHostileGoal::findNearestHostile() {
    if (!m_villager || !m_villager->world()) return;

    // TODO: 搜索附近的敌对生物（僵尸、掠夺者等）
    // 目前不实现
    m_hostileEntity = 0;
}

void AvoidHostileGoal::fleeFromHostile() {
    if (!m_villager || m_hostileEntity == 0) return;

    // 计算逃跑方向
    // TODO: 获取敌对生物位置，计算反方向
    math::Random rng = m_villager->getRandom();

    // 简化：随机选择逃跑方向
    f32 angle = rng.nextFloat() * math::TWO_PI;
    f32 dist = FLEE_DISTANCE;

    f32 targetX = m_villager->x() + std::cos(angle) * dist;
    f32 targetZ = m_villager->z() + std::sin(angle) * dist;
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

bool GoToBedGoal::shouldExecute() {
    if (!m_villager) return false;

    // 检查是否是夜间
    if (!m_villager->world()) return false;
    if (!villager::isNightTime(m_villager->world()->dayTime())) return false;

    // TODO: 检查是否有绑定的床位
    return false;
}

bool GoToBedGoal::shouldContinueExecuting() {
    if (!m_villager) return false;

    return !m_reachedBed;
}

void GoToBedGoal::startExecuting() {
    m_reachedBed = false;

    if (m_villager && m_bedPos != BlockPos::zero()) {
        m_villager->tryMoveTo(m_bedPos.x + 0.5, m_bedPos.y, m_bedPos.z + 0.5, SPEED_MODIFIER);
    }
}

void GoToBedGoal::resetTask() {
    m_reachedBed = false;
    m_bedPos = BlockPos::zero();

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void GoToBedGoal::tick() {
    if (!m_villager) return;

    if (m_bedPos == BlockPos::zero()) return;

    // 检查是否到达床位
    if (isWithinDistance(m_villager, m_bedPos, 1.5f)) {
        m_reachedBed = true;
        // TODO: 开始睡眠
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

bool VillagerBreedGoal::shouldExecute() {
    if (!m_villager) return false;

    // 检查是否愿意繁殖
    if (!isWillingToBreed()) return false;

    // 检查床位
    if (!hasEnoughBeds()) return false;

    // 寻找配偶
    findPartner();
    return m_partnerId != 0;
}

bool VillagerBreedGoal::shouldContinueExecuting() {
    if (!m_villager) return false;

    // 配偶消失或不再愿意繁殖
    if (m_partnerId == 0) return false;

    // 超时
    return m_breedTicks < BREED_TICKS;
}

void VillagerBreedGoal::startExecuting() {
    m_breedTicks = 0;
}

void VillagerBreedGoal::resetTask() {
    m_partnerId = 0;
    m_breedTicks = 0;

    if (m_villager) {
        m_villager->clearNavigation();
        // 重置繁殖意愿
        m_villager->resetBreedWillingness();
    }
}

void VillagerBreedGoal::tick() {
    if (!m_villager || m_partnerId == 0) return;

    m_breedTicks++;

    // 移动到配偶
    moveToPartner();

    // 检查是否足够接近以繁殖
    // TODO: 获取配偶位置检查距离
    if (m_breedTicks >= BREED_TICKS) {
        spawnChild();
    }
}

bool VillagerBreedGoal::hasEnoughBeds() const {
    if (!m_villager) return false;

    // TODO: 检查村庄中是否有足够的床位
    // 目前简化为总是返回true
    return true;
}

bool VillagerBreedGoal::isWillingToBreed() const {
    if (!m_villager) return false;

    return m_villager->isWillingToBreed();
}

void VillagerBreedGoal::findPartner() {
    if (!m_villager || !m_villager->world()) return;

    // TODO: 搜索附近愿意繁殖的村民
    m_partnerId = 0;
}

void VillagerBreedGoal::moveToPartner() {
    if (!m_villager || m_partnerId == 0) return;

    // TODO: 获取配偶位置并移动
}

void VillagerBreedGoal::spawnChild() {
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
