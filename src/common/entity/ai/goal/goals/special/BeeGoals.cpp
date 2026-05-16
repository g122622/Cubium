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

#include "BeeGoals.hpp"
#include "../../../../entities/passive/special/BeeEntity.hpp"
#include "../../../controller/MovementController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../GoalConstants.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/BlockState.hpp"
#include "../../../../../world/block/Block.hpp"
#include "../../../../../world/block/BlockTags.hpp"
#include "../../../../../world/block/blocks/agricultural/CropBlock.hpp"
#include "../../../../../entity/core/EntityUtils.hpp"
#include "../../../../../entity/damage/DamageSource.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// BeePassiveGoal - 蜜蜂被动目标基类
// ============================================================================

BeePassiveGoal::BeePassiveGoal(BeeEntity* bee)
    : Goal()
    , m_bee(bee)
{
}

bool BeePassiveGoal::shouldExecute()
{
    // 愤怒时不执行被动行为
    return !m_bee->isAngry() && canBeeStart();
}

bool BeePassiveGoal::shouldContinueExecuting()
{
    // 愤怒时停止被动行为
    return !m_bee->isAngry() && canBeeContinue();
}

// ============================================================================
// BeeStingGoal - 蜜蜂蛰刺攻击目标
// ============================================================================

BeeStingGoal::BeeStingGoal(BeeEntity* bee)
    : MeleeAttackGoal(bee, 1.4, true)
    , m_beeEntity(bee)
{
}

bool BeeStingGoal::shouldExecute()
{
    // 只有愤怒且未蛰刺过时才执行
    return m_beeEntity->isAngry() && !m_beeEntity->hasStung() && MeleeAttackGoal::shouldExecute();
}

bool BeeStingGoal::shouldContinueExecuting()
{
    return m_beeEntity->isAngry() && !m_beeEntity->hasStung() && MeleeAttackGoal::shouldContinueExecuting();
}

void BeeStingGoal::startExecuting()
{
    MeleeAttackGoal::startExecuting();
}

void BeeStingGoal::tick()
{
    MeleeAttackGoal::tick();

    // 如果攻击成功，设置蛰刺状态
    // MC 1.16.5: 在 attackEntityAsMob 中处理
    // 这里在 MeleeAttackGoal 的攻击回调中处理
}

// ============================================================================
// BeeEnterHiveGoal - 蜜蜂进入蜂巢目标
// ============================================================================

BeeEnterHiveGoal::BeeEnterHiveGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{
}

bool BeeEnterHiveGoal::canBeeStart()
{
    // 检查是否有蜂巢
    if (!m_bee->hasHive()) {
        return false;
    }

    // 检查是否能进入蜂巢
    // MC 1.16.5: canEnterHive() 方法
    // 条件：stayOutOfHiveCountdown <= 0 && !pollinateGoal.isRunning() && !hasStung() && getAttackTarget() == null
    //      && (failedPollinatingTooLong() || world.isRaining() || world.isNightTime() || hasNectar())
    // 简化实现：检查距离和基本条件
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    // 检查是否在蜂巢附近（2格内）
    BlockPos hivePos = m_bee->getHivePos();
    math::Vector3 beePos = m_bee->position();
    f64 distSq = (beePos.x - hivePos.x - 0.5) * (beePos.x - hivePos.x - 0.5)
               + (beePos.y - hivePos.y - 0.5) * (beePos.y - hivePos.y - 0.5)
               + (beePos.z - hivePos.z - 0.5) * (beePos.z - hivePos.z - 0.5);

    if (distSq > 4.0) { // 2格距离平方
        return false;
    }

    // 检查蜂巢是否有效
    const BlockState* state = world->getBlockState(hivePos);
    if (state == nullptr) {
        return false;
    }

    // 检查是否是蜂巢方块
    if (!BlockTags::BEEHIVES().contains(*state)) {
        return false;
    }

    // TODO: 检查蜂巢是否有空间
    // 需要访问 BeehiveBlockEntity 来检查蜜蜂数量

    return true;
}

bool BeeEnterHiveGoal::canBeeContinue()
{
    // 一次性执行
    return false;
}

void BeeEnterHiveGoal::startExecuting()
{
    // TODO: 实际进入蜂巢逻辑
    // 需要与 BeehiveBlockEntity 交互
    // beehivetileentity.tryEnterHive(this, hasNectar);

    // 暂时简化处理：重置状态
    m_bee->setHasNectar(false);
    m_bee->setReturningToHive(false);
}

// ============================================================================
// BeePollinateGoal - 蜜蜂授粉目标
// ============================================================================

BeePollinateGoal::BeePollinateGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{
    // 设置互斥标志：移动
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool BeePollinateGoal::canBeeStart()
{
    // 检查冷却
    // TODO: 实现 remainingCooldownBeforeLocatingNewFlower
    // if (m_bee->getFlowerCooldown() > 0) return false;

    // 已有花粉时不授粉
    if (m_bee->hasNectar()) {
        return false;
    }

    // 下雨时不授粉
    IWorld* world = m_bee->world();
    if (world != nullptr && world->isRaining()) {
        return false;
    }

    // 30% 概率执行
    if (world == nullptr || world->getRandom().nextFloat() >= 0.3f) {
        return false;
    }

    // 寻找花朵
    return findFlower();
}

bool BeePollinateGoal::canBeeContinue()
{
    if (!m_running) {
        return false;
    }

    if (!m_bee->hasFlower()) {
        return false;
    }

    // 下雨时停止
    IWorld* world = m_bee->world();
    if (world != nullptr && world->isRaining()) {
        return false;
    }

    // 完成授粉后有 20% 概率继续
    if (completedPollination()) {
        return world != nullptr && world->getRandom().nextFloat() < 0.2f;
    }

    // 检查花朵是否仍然有效
    if (m_totalTicks % 20 == 0 && !isFlower(m_bee->getFlowerPos())) {
        m_bee->setFlowerPos(BlockPos::zero());
        return false;
    }

    return true;
}

void BeePollinateGoal::startExecuting()
{
    m_pollinationTicks = 0;
    m_totalTicks = 0;
    m_lastSoundTick = 0;
    m_running = true;
    // m_bee->resetTicksWithoutNectar();
}

void BeePollinateGoal::resetTask()
{
    // 授粉成功
    if (completedPollination()) {
        m_bee->setHasNectar(true);
    }

    m_running = false;

    // 清除路径
    if (auto* nav = m_bee->navigator()) {
        nav->clearPath();
    }

    // 设置冷却
    // m_bee->setFlowerCooldown(200); // 10秒
}

void BeePollinateGoal::tick()
{
    ++m_totalTicks;

    // 超时检查
    if (m_totalTicks > MAX_POLLINATION_TIME) {
        m_bee->setFlowerPos(BlockPos::zero());
        return;
    }

    // 检查花朵位置
    if (!m_bee->hasFlower()) {
        return;
    }

    BlockPos flowerPos = m_bee->getFlowerPos();
    math::Vector3f flowerCenter(static_cast<f32>(flowerPos.x + 0.5), static_cast<f32>(flowerPos.y + 0.6), static_cast<f32>(flowerPos.z + 0.5));
    math::Vector3 beePos = m_bee->position();

    f64 distSq = (beePos.x - flowerCenter.x) * (beePos.x - flowerCenter.x)
               + (beePos.y - flowerCenter.y) * (beePos.y - flowerCenter.y)
               + (beePos.z - flowerCenter.z) * (beePos.z - flowerCenter.z);

    if (distSq > 1.0) {
        // 飞向花朵
        m_nextTarget = flowerCenter;
        moveToNextTarget();
    } else {
        // 在花朵附近徘徊授粉
        ++m_pollinationTicks;

        // 播放授粉声音
        IWorld* world = m_bee->world();
        if (world != nullptr) {
            if (world->getRandom().nextFloat() < 0.05f && m_pollinationTicks > m_lastSoundTick + 60) {
                m_lastSoundTick = m_pollinationTicks;
                // TODO: 播放蜜蜂授粉声音
                // world->playSound(m_bee->position(), SoundEvents::ENTITY_BEE_POLLINATE, 1.0f, 1.0f);
            }
        }
    }
}

bool BeePollinateGoal::isFlower(const BlockPos& pos) const
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    const BlockState* state = world->getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // 检查是否是小花朵
    if (BlockTags::SMALL_FLOWERS().contains(*state)) {
        return true;
    }

    // 检查是否是高花朵
    if (BlockTags::TALL_FLOWERS().contains(*state)) {
        // 向日葵只检测上半部分
        // TODO: 需要检查 DoublePlantBlock.HALF 属性
        return true;
    }

    return false;
}

bool BeePollinateGoal::findFlower()
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    math::Vector3 beePos = m_bee->position();
    BlockPos centerPos(static_cast<i32>(beePos.x), static_cast<i32>(beePos.y), static_cast<i32>(beePos.z));

    // 搜索5格范围内的花朵
    for (i32 dx = -5; dx <= 5; ++dx) {
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dz = -5; dz <= 5; ++dz) {
                BlockPos checkPos(centerPos.x + dx, centerPos.y + dy, centerPos.z + dz);

                if (isFlower(checkPos)) {
                    m_bee->setFlowerPos(checkPos);

                    // 开始导航
                    if (auto* nav = m_bee->navigator()) {
                        (void)nav->moveTo(checkPos.x + 0.5, checkPos.y + 0.5, checkPos.z + 0.5, 1.0);
                    }

                    return true;
                }
            }
        }
    }

    return false;
}

void BeePollinateGoal::moveToNextTarget()
{
    // 飞向目标位置
    if (auto* moveCtrl = m_bee->moveController()) {
        moveCtrl->setMoveTo(m_nextTarget.x, m_nextTarget.y, m_nextTarget.z, 1.0);
    }
}

// ============================================================================
// BeeUpdateHiveGoal - 蜜蜂更新蜂巢位置目标
// ============================================================================

BeeUpdateHiveGoal::BeeUpdateHiveGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{
}

bool BeeUpdateHiveGoal::canBeeStart()
{
    // TODO: 检查冷却
    // if (m_bee->getHiveCooldown() > 0) return false;

    // 没有蜂巢位置时执行
    // TODO: 实现 canEnterHive 检查
    return !m_bee->hasHive();
}

bool BeeUpdateHiveGoal::canBeeContinue()
{
    // 一次性执行
    return false;
}

void BeeUpdateHiveGoal::startExecuting()
{
    // 搜索附近可用的蜂巢
    auto hives = findNearbyFreeHives();

    if (!hives.empty()) {
        // 设置最近的蜂巢
        m_bee->setHivePos(hives[0]);
    }

    // 设置冷却
    // m_bee->setHiveCooldown(200); // 10秒
}

std::vector<BlockPos> BeeUpdateHiveGoal::findNearbyFreeHives() const
{
    std::vector<BlockPos> hives;

    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return hives;
    }

    math::Vector3 beePos = m_bee->position();
    BlockPos centerPos(static_cast<i32>(beePos.x), static_cast<i32>(beePos.y), static_cast<i32>(beePos.z));

    // 搜索20格范围内的蜂巢
    for (i32 dx = -20; dx <= 20; ++dx) {
        for (i32 dy = -8; dy <= 8; ++dy) {
            for (i32 dz = -20; dz <= 20; ++dz) {
                BlockPos checkPos(centerPos.x + dx, centerPos.y + dy, centerPos.z + dz);

                const BlockState* state = world->getBlockState(checkPos);
                if (state != nullptr && BlockTags::BEEHIVES().contains(*state)) {
                    // TODO: 检查蜂巢是否有空间
                    if (doesHiveHaveSpace(checkPos)) {
                        hives.push_back(checkPos);
                    }
                }
            }
        }
    }

    // 按距离排序
    std::sort(hives.begin(), hives.end(), [&beePos](const BlockPos& a, const BlockPos& b) {
        f64 distA = (a.x - beePos.x) * (a.x - beePos.x)
                  + (a.y - beePos.y) * (a.y - beePos.y)
                  + (a.z - beePos.z) * (a.z - beePos.z);
        f64 distB = (b.x - beePos.x) * (b.x - beePos.x)
                  + (b.y - beePos.y) * (b.y - beePos.y)
                  + (b.z - beePos.z) * (b.z - beePos.z);
        return distA < distB;
    });

    return hives;
}

bool BeeUpdateHiveGoal::doesHiveHaveSpace(const BlockPos& /*pos*/) const
{
    // TODO: 检查 BeehiveBlockEntity 是否有空间
    // 目前简化返回 true
    return true;
}

// ============================================================================
// BeeFindHiveGoal - 蜜蜂寻找蜂巢目标
// ============================================================================

BeeFindHiveGoal::BeeFindHiveGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool BeeFindHiveGoal::canBeeStart()
{
    // 有蜂巢位置且需要返回
    if (!m_bee->hasHive()) {
        return false;
    }

    // TODO: 实现 canEnterHive 检查
    // if (!m_bee->canEnterHive()) return false;

    // 检查是否已经在蜂巢附近
    if (isCloseEnough(m_bee->getHivePos())) {
        return false;
    }

    // 检查蜂巢是否有效
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    const BlockState* state = world->getBlockState(m_bee->getHivePos());
    if (state == nullptr || !BlockTags::BEEHIVES().contains(*state)) {
        return false;
    }

    return true;
}

bool BeeFindHiveGoal::canBeeContinue()
{
    return m_bee->hasHive() && !isCloseEnough(m_bee->getHivePos());
}

void BeeFindHiveGoal::startExecuting()
{
    m_ticks = 0;
    m_stuckCounter = 0;
    m_possibleHives.clear();
}

void BeeFindHiveGoal::resetTask()
{
    m_bee->setHivePos(BlockPos::zero());
    m_bee->setHasHive(false);
    // m_bee->setHiveCooldown(200);
}

void BeeFindHiveGoal::tick()
{
    if (!m_bee->hasHive()) {
        return;
    }

    ++m_ticks;

    // 超时检查
    if (m_ticks > MAX_NAVIGATION_TIME) {
        // 放弃当前蜂巢
        m_possibleHives.push_back(m_bee->getHivePos());
        m_bee->setHivePos(BlockPos::zero());
        m_bee->setHasHive(false);
        return;
    }

    // 导航到蜂巢
    BlockPos hivePos = m_bee->getHivePos();
    math::Vector3 beePos = m_bee->position();

    if (auto* nav = m_bee->navigator()) {
        if (nav->noPath()) {
            // 没有路径
            f64 distSq = (beePos.x - hivePos.x) * (beePos.x - hivePos.x)
                       + (beePos.z - hivePos.z) * (beePos.z - hivePos.z);

            if (distSq > 256.0) { // 16格
                // 太远了，放弃
                if (isTooFar(hivePos)) {
                    resetTask();
                } else {
                    // 尝试导航
                    (void)nav->moveTo(hivePos.x + 0.5, hivePos.y + 0.5, hivePos.z + 0.5, 1.0);
                }
            } else {
                // 尝试更精确的导航
                (void)nav->moveTo(hivePos.x + 0.5, hivePos.y + 0.5, hivePos.z + 0.5, 1.0);

                // 检查是否卡住
                ++m_stuckCounter;
                if (m_stuckCounter > STUCK_THRESHOLD) {
                    resetTask();
                }
            }
        } else {
            // 有路径，重置卡住计数
            m_stuckCounter = 0;
        }
    }
}

bool BeeFindHiveGoal::isCloseEnough(const BlockPos& pos) const
{
    math::Vector3 beePos = m_bee->position();
    f64 distSq = (beePos.x - pos.x - 0.5) * (beePos.x - pos.x - 0.5)
               + (beePos.y - pos.y - 0.5) * (beePos.y - pos.y - 0.5)
               + (beePos.z - pos.z - 0.5) * (beePos.z - pos.z - 0.5);
    return distSq <= 4.0; // 2格距离平方
}

bool BeeFindHiveGoal::isTooFar(const BlockPos& pos) const
{
    math::Vector3 beePos = m_bee->position();
    f64 distSq = (beePos.x - pos.x) * (beePos.x - pos.x)
               + (beePos.z - pos.z) * (beePos.z - pos.z);
    return distSq > 1024.0; // 32格距离平方
}

bool BeeFindHiveGoal::isPossibleHive(const BlockPos& pos) const
{
    return std::find(m_possibleHives.begin(), m_possibleHives.end(), pos) != m_possibleHives.end();
}

// ============================================================================
// BeeFindFlowerGoal - 蜜蜂寻找花朵目标
// ============================================================================

BeeFindFlowerGoal::BeeFindFlowerGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool BeeFindFlowerGoal::canBeeStart()
{
    // 有花朵位置时执行
    if (!m_bee->hasFlower()) {
        return false;
    }

    // TODO: 实现 shouldMoveToFlower 检查
    // return m_bee->getTicksWithoutNectarSinceExitingHive() > 2400;
    // 简化：总是执行

    // 检查花朵是否有效
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    const BlockState* state = world->getBlockState(m_bee->getFlowerPos());
    if (state == nullptr) {
        return false;
    }

    // 检查是否是花朵
    if (!BlockTags::SMALL_FLOWERS().contains(*state) && !BlockTags::TALL_FLOWERS().contains(*state)) {
        return false;
    }

    // 检查是否已经足够近
    math::Vector3 beePos = m_bee->position();
    BlockPos flowerPos = m_bee->getFlowerPos();
    f64 distSq = (beePos.x - flowerPos.x - 0.5) * (beePos.x - flowerPos.x - 0.5)
               + (beePos.z - flowerPos.z - 0.5) * (beePos.z - flowerPos.z - 0.5);

    return distSq > 4.0; // 超过2格距离
}

bool BeeFindFlowerGoal::canBeeContinue()
{
    return m_bee->hasFlower();
}

void BeeFindFlowerGoal::startExecuting()
{
    m_ticks = 0;
}

void BeeFindFlowerGoal::resetTask()
{
    m_bee->setFlowerPos(BlockPos::zero());
}

void BeeFindFlowerGoal::tick()
{
    if (!m_bee->hasFlower()) {
        return;
    }

    ++m_ticks;

    // 超时检查
    if (m_ticks > MAX_NAVIGATION_TIME) {
        m_bee->setFlowerPos(BlockPos::zero());
        return;
    }

    // 导航到花朵
    BlockPos flowerPos = m_bee->getFlowerPos();

    if (auto* nav = m_bee->navigator()) {
        if (nav->noPath()) {
            // 检查是否太远
            math::Vector3 beePos = m_bee->position();
            f64 distSq = (beePos.x - flowerPos.x) * (beePos.x - flowerPos.x)
                       + (beePos.z - flowerPos.z) * (beePos.z - flowerPos.z);

            if (distSq > 1024.0) { // 32格
                m_bee->setFlowerPos(BlockPos::zero());
            } else {
                (void)nav->moveTo(flowerPos.x + 0.5, flowerPos.y + 0.5, flowerPos.z + 0.5, 1.0);
            }
        }
    }
}

// ============================================================================
// BeeFindPollinationTargetGoal - 蜜蜂寻找授粉目标
// ============================================================================

BeeFindPollinationTargetGoal::BeeFindPollinationTargetGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{
}

bool BeeFindPollinationTargetGoal::canBeeStart()
{
    // 有花粉时执行
    if (!m_bee->hasNectar()) {
        return false;
    }

    // TODO: 检查作物数限制
    // if (m_bee->getCropsGrownSincePollination() >= MAX_CROPS_GROWN) return false;

    // 检查是否有有效蜂巢
    // if (!m_bee->isHiveValid()) return false;

    // 30% 概率
    IWorld* world = m_bee->world();
    if (world == nullptr || world->getRandom().nextFloat() >= 0.3f) {
        return false;
    }

    return true;
}

bool BeeFindPollinationTargetGoal::canBeeContinue()
{
    return canBeeStart();
}

void BeeFindPollinationTargetGoal::tick()
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return;
    }

    // 30 tick 检查一次
    if (world->getRandom().nextInt(30) != 0) {
        return;
    }

    // 检查脚下作物
    math::Vector3 beePos = m_bee->position();
    BlockPos beeBlockPos(static_cast<i32>(beePos.x), static_cast<i32>(beePos.y), static_cast<i32>(beePos.z));

    for (i32 dy = 1; dy <= 2; ++dy) {
        BlockPos checkPos(beeBlockPos.x, beeBlockPos.y - dy, beeBlockPos.z);

        if (isPollinationTarget(checkPos)) {
            growCrop(checkPos);
            // m_bee->addCropCounter();

            // 检查是否达到上限
            // if (m_bee->getCropsGrownSincePollination() >= MAX_CROPS_GROWN) {
            //     break;
            // }
        }
    }
}

bool BeeFindPollinationTargetGoal::isPollinationTarget(const BlockPos& pos) const
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    const BlockState* state = world->getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // 检查是否是可授粉作物
    return BlockTags::BEE_GROWABLES().contains(*state);
}

void BeeFindPollinationTargetGoal::growCrop(const BlockPos& pos)
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return;
    }

    const BlockState* state = world->getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    // TODO: 实现作物生长逻辑
    // 需要根据作物类型增加生长阶段
    // 1. 农作物（小麦、胡萝卜、马铃薯、甜菜根）：增加 age 属性
    // 2. 瓜果茎（西瓜茎、南瓜茎）：增加 age 属性
    // 3. 甜浆果丛：增加 age 属性

    // 播放生长粒子效果
    // world->playEvent(2005, pos, 0);
}

// ============================================================================
// BeeWanderGoal - 蜜蜂随机飞行目标
// ============================================================================

BeeWanderGoal::BeeWanderGoal(BeeEntity* bee)
    : Goal()
    , m_bee(bee)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool BeeWanderGoal::shouldExecute()
{
    // 没有路径时执行，10% 概率
    if (auto* nav = m_bee->navigator()) {
        if (!nav->noPath()) {
            return false;
        }
    }

    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    return world->getRandom().nextInt(WANDER_CHANCE) == 0;
}

void BeeWanderGoal::startExecuting()
{
    math::Vector3f target = getRandomLocation();
    if (isValidLocation(target)) {
        if (auto* nav = m_bee->navigator()) {
            (void)nav->moveTo(target.x, target.y, target.z, 1.0);
        }
    }
}

void BeeWanderGoal::tick()
{
    // 持续导航
    if (auto* nav = m_bee->navigator()) {
        if (nav->noPath()) {
            math::Vector3f target = getRandomLocation();
            if (isValidLocation(target)) {
                (void)nav->moveTo(target.x, target.y, target.z, 1.0);
            }
        }
    }
}

math::Vector3f BeeWanderGoal::getRandomLocation()
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return m_bee->position();
    }

    math::Vector3 beePos = m_bee->position();
    math::Random& rng = world->getRandom();

    // 如果有蜂巢且离得太远，飞回蜂巢方向
    math::Vector3f direction(0.0f, 0.0f, 0.0f);

    if (m_bee->hasHive()) {
        BlockPos hivePos = m_bee->getHivePos();
        f64 distSq = (beePos.x - hivePos.x) * (beePos.x - hivePos.x)
                   + (beePos.z - hivePos.z) * (beePos.z - hivePos.z);

        if (distSq > HIVE_RETURN_DISTANCE * HIVE_RETURN_DISTANCE) {
            // 飞回蜂巢方向
            direction = math::Vector3f(hivePos.x + 0.5f - beePos.x, 0.0f, hivePos.z + 0.5f - beePos.z);
            direction = direction.normalized();
        }
    }

    if (direction.lengthSquared() < 0.01f) {
        // 随机方向 - 使用yaw计算前方向
        f32 yaw = m_bee->yaw() * math::DEG_TO_RAD;
        direction = math::Vector3f(-std::sin(yaw), 0.0f, std::cos(yaw));
    }

    // 寻找空中目标或地面目标
    // TODO: 实现 RandomPositionGenerator.findAirTarget
    // 简化实现：随机位置
    f64 targetX = beePos.x + (rng.nextDouble() * WANDER_RANGE * 2 - WANDER_RANGE) + direction.x * WANDER_RANGE;
    f64 targetY = beePos.y + (rng.nextDouble() * WANDER_HEIGHT * 2 - WANDER_HEIGHT);
    f64 targetZ = beePos.z + (rng.nextDouble() * WANDER_RANGE * 2 - WANDER_RANGE) + direction.z * WANDER_RANGE;

    return math::Vector3f(static_cast<f32>(targetX), static_cast<f32>(targetY), static_cast<f32>(targetZ));
}

bool BeeWanderGoal::isValidLocation(const math::Vector3f& /*pos*/) const
{
    // 检查位置是否在有效范围内
    // 简化实现
    return true;
}

// ============================================================================
// BeeAngerGoal - 蜜蜂愤怒目标
// ============================================================================

BeeAngerGoal::BeeAngerGoal(BeeEntity* bee)
    : HurtByTargetGoal(bee, true)  // alertAllies = true
    , m_beeEntity(bee)
{
}

bool BeeAngerGoal::shouldContinueExecuting()
{
    return m_beeEntity->isAngry() && HurtByTargetGoal::shouldContinueExecuting();
}

void BeeAngerGoal::startExecuting()
{
    HurtByTargetGoal::startExecuting();

    // 蜜蜂被攻击时会召唤附近的其他蜜蜂
    // MC 1.16.5: 在 setRevengeTarget 中已经实现
}

// ============================================================================
// BeeAttackPlayerGoal - 蜜蜂攻击玩家目标
// ============================================================================

BeeAttackPlayerGoal::BeeAttackPlayerGoal(BeeEntity* bee, i32 chance)
    : TargetGoal(bee, true)
    , m_beeEntity(bee)
    , m_chance(chance)
{
}

bool BeeAttackPlayerGoal::shouldExecute()
{
    if (!canSting()) {
        return false;
    }

    // 概率检查
    IWorld* world = m_beeEntity->world();
    if (world != nullptr && m_chance > 0) {
        if (world->getRandom().nextInt(m_chance) != 0) {
            return false;
        }
    }

    // 搜索附近玩家
    // TODO: 使用 EntityUtils::findClosestEntity<PlayerEntity>
    // 简化实现
    return false;
}

bool BeeAttackPlayerGoal::shouldContinueExecuting()
{
    if (!canSting()) {
        m_target = nullptr;
        return false;
    }

    if (m_target != nullptr) {
        return TargetGoal::shouldContinueExecuting();
    }

    return false;
}

void BeeAttackPlayerGoal::startExecuting()
{
    TargetGoal::startExecuting();

    if (m_target != nullptr) {
        m_beeEntity->setAttackTarget(dynamic_cast<LivingEntity*>(m_target));
    }
}

void BeeAttackPlayerGoal::resetTask()
{
    m_target = nullptr;
    m_targetPlayer = nullptr;
    TargetGoal::resetTask();
}

bool BeeAttackPlayerGoal::canSting() const
{
    return m_beeEntity->isAngry() && !m_beeEntity->hasStung();
}

// ============================================================================
// BeeResetAngerGoal - 蜜蜂重置愤怒目标
// ============================================================================

BeeResetAngerGoal::BeeResetAngerGoal(BeeEntity* bee)
    : Goal()
    , m_bee(bee)
{
}

bool BeeResetAngerGoal::shouldExecute()
{
    // 愤怒时间结束时重置
    return m_bee->getAngerTime() == 0 && m_bee->isAngry();
}

void BeeResetAngerGoal::startExecuting()
{
    m_bee->setAngry(false);
    m_bee->setAttackTarget(nullptr);
}

} // namespace entity::ai::goal
} // namespace mc
