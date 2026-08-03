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
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/blocks/agricultural/StemBlock.hpp"
#include "common/world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "common/world/block/blocks/vegetation/SweetBerryBushBlock.hpp"
#include "common/world/block/registry/VegetationBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BeehiveBlockEntity.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// BeePassiveGoal - 蜜蜂被动目标基类
// ============================================================================

BeePassiveGoal::BeePassiveGoal(BeeEntity* bee)
    : Goal()
    , m_bee(bee)
{}

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
{}

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
    // 在 MeleeAttackGoal 的攻击回调中处理
}

// ============================================================================
// BeeEnterHiveGoal - 蜜蜂进入蜂巢目标
// ============================================================================

BeeEnterHiveGoal::BeeEnterHiveGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{}

bool BeeEnterHiveGoal::canBeeStart()
{
    // 检查是否有蜂巢
    if (!m_bee->hasHive()) {
        return false;
    }

    // 检查蜜蜂是否想进入蜂巢
    if (!m_bee->wantsToEnterHive()) {
        return false;
    }

    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    // 检查是否在蜂巢附近（2格内）
    BlockPos hivePos = m_bee->getHivePos();
    math::Vector3 beePos = m_bee->position();
    f64 distSq = (beePos.x - hivePos.x - 0.5) * (beePos.x - hivePos.x - 0.5) +
        (beePos.y - hivePos.y - 0.5) * (beePos.y - hivePos.y - 0.5) +
        (beePos.z - hivePos.z - 0.5) * (beePos.z - hivePos.z - 0.5);

    if (distSq > 4.0) { // 2格距离平方
        return false;
    }

    // 检查蜂巢是否有效且未满
    auto* beehive = m_bee->getBeehiveBlockEntity();
    if (beehive == nullptr) {
        // 蜂巢方块实体不存在或距离过远，忘记这个蜂巢
        m_bee->dropHive();
        return false;
    }

    if (beehive->isFull()) {
        // 蜂巢已满，忘记这个蜂巢
        m_bee->dropHive();
        return false;
    }

    return true;
}

bool BeeEnterHiveGoal::canBeeContinue()
{
    // 一次性执行
    return false;
}

void BeeEnterHiveGoal::startExecuting()
{
    // 蜜蜂进入蜂巢
    auto* beehive = m_bee->getBeehiveBlockEntity();
    if (beehive != nullptr) {
        beehive->addOccupant(*m_bee);
        // addOccupant 会调用 bee.discard()，蜜蜂实体被移除
        return;
    }

    // 蜂巢方块实体不存在，重置状态
    m_bee->resetCropCounter();
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
    // 检查花朵冷却
    if (m_bee->getFlowerCooldown() > 0) {
        return false;
    }

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
    return _findFlower();
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
    if (_completedPollination()) {
        return world != nullptr && world->getRandom().nextFloat() < 0.2f;
    }

    // 检查花朵是否仍然有效
    if (m_totalTicks % 20 == 0 && !_isFlower(m_bee->getFlowerPos())) {
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
    m_bee->setPollinating(true);
}

void BeePollinateGoal::resetTask()
{
    // 授粉成功
    if (_completedPollination()) {
        m_bee->setHasNectar(true);
    }

    m_running = false;
    m_bee->setPollinating(false);

    // 清除路径
    if (auto* nav = m_bee->navigator()) {
        nav->clearPath();
    }

    // 设置花朵冷却（200 tick）
    m_bee->setFlowerCooldown(200);
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
    math::Vector3f flowerCenter(
        static_cast<f32>(flowerPos.x + 0.5), static_cast<f32>(flowerPos.y + 0.6), static_cast<f32>(flowerPos.z + 0.5));
    math::Vector3 beePos = m_bee->position();

    f64 distSq = (beePos.x - flowerCenter.x) * (beePos.x - flowerCenter.x) +
        (beePos.y - flowerCenter.y) * (beePos.y - flowerCenter.y) +
        (beePos.z - flowerCenter.z) * (beePos.z - flowerCenter.z);

    if (distSq > 1.0) {
        // 飞向花朵
        m_nextTarget = flowerCenter;
        _moveToNextTarget();
    } else {
        // 在花朵附近徘徊授粉
        ++m_pollinationTicks;

        // 播放授粉声音
        IWorld* world = m_bee->world();
        if (world != nullptr) {
            if (world->getRandom().nextFloat() < 0.05f && m_pollinationTicks > m_lastSoundTick + 60) {
                m_lastSoundTick = m_pollinationTicks;
                world->playSound(SoundEvents::ENTITY_BEE_POLLINATE,
                    sound::SoundCategory::Neutral,
                    m_bee->position(),
                    1.0f,
                    1.0f + (world->getRandom().nextFloat() - world->getRandom().nextFloat()) * 0.2f);
            }
        }
    }
}

bool BeePollinateGoal::_isFlower(const BlockPos& pos) const
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
        // 向日葵只有上半部分吸引蜜蜂，下半部分不吸引
        if (state->is(block_registry::VegetationBlocks::SUNFLOWER)) {
            auto half = state->get(BlockStateProperties::DOUBLE_BLOCK_HALF());
            return half == blocks::DoublePlantBlock::DoubleBlockHalf::Upper;
        }
        return true;
    }

    return false;
}

bool BeePollinateGoal::_findFlower()
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

                if (_isFlower(checkPos)) {
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

void BeePollinateGoal::_moveToNextTarget()
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
{}

bool BeeUpdateHiveGoal::canBeeStart()
{
    // 检查冷却
    if (m_bee->getHiveLocateCooldown() > 0) {
        return false;
    }

    // 没有蜂巢位置时执行
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
    auto hives = _findNearbyFreeHives();

    if (!hives.empty()) {
        // 设置最近的蜂巢
        m_bee->setHivePos(hives[0]);
    }

    // 设置冷却（10秒 = 200 tick）
    m_bee->setHiveLocateCooldown(200);
}

std::vector<BlockPos> BeeUpdateHiveGoal::_findNearbyFreeHives() const
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
                    if (_doesHiveHaveSpace(checkPos)) {
                        hives.push_back(checkPos);
                    }
                }
            }
        }
    }

    // 按距离排序
    std::sort(hives.begin(), hives.end(), [&beePos](const BlockPos& a, const BlockPos& b) {
        f64 distA = (a.x - beePos.x) * (a.x - beePos.x) + (a.y - beePos.y) * (a.y - beePos.y) +
            (a.z - beePos.z) * (a.z - beePos.z);
        f64 distB = (b.x - beePos.x) * (b.x - beePos.x) + (b.y - beePos.y) * (b.y - beePos.y) +
            (b.z - beePos.z) * (b.z - beePos.z);
        return distA < distB;
    });

    return hives;
}

bool BeeUpdateHiveGoal::_doesHiveHaveSpace(const BlockPos& pos) const
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    auto* blockEntity = world->getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Beehive) {
        return false;
    }

    auto* beehive = static_cast<blockentity::BeehiveBlockEntity*>(blockEntity);
    return !beehive->isFull();
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

    // 检查蜜蜂是否想进入蜂巢
    if (!m_bee->wantsToEnterHive()) {
        return false;
    }

    // 检查是否已经在蜂巢附近
    if (_isCloseEnough(m_bee->getHivePos())) {
        return false;
    }

    // 检查蜂巢是否有效
    if (!m_bee->isHiveValid()) {
        m_bee->dropHive();
        return false;
    }

    return true;
}

bool BeeFindHiveGoal::canBeeContinue()
{
    return m_bee->hasHive() && !_isCloseEnough(m_bee->getHivePos());
}

void BeeFindHiveGoal::startExecuting()
{
    m_ticks = 0;
    m_stuckCounter = 0;
    m_possibleHives.clear();
}

void BeeFindHiveGoal::resetTask()
{
    m_bee->dropHive();
    // MC 1.21.11: BeeGoToHiveGoal.stop() 中重置 maxVisitedNodesMultiplier 为默认值 1.0F
    if (auto* nav = m_bee->navigator()) {
        nav->resetMaxVisitedNodesMultiplier();
    }
}

void BeeFindHiveGoal::tick()
{
    if (!m_bee->hasHive()) {
        return;
    }

    ++m_ticks;

    // 对应 MC 1.21.11 BeeGoToHiveGoal.tick()
    // 超时检查：2400 tick（120秒）仍未到达则放弃并拉黑蜂巢
    if (m_ticks > MAX_NAVIGATION_TIME) {
        // 放弃当前蜂巢并加入黑名单
        m_possibleHives.push_back(m_bee->getHivePos());
        m_bee->setHivePos(BlockPos::zero());
        m_bee->setHasHive(false);
        return;
    }

    BlockPos hivePos = m_bee->getHivePos();

    if (auto* nav = m_bee->navigator()) {
        if (!nav->isInProgress()) {
            // 没有路径时的逻辑
            // 对应 MC: !closerThan(hivePos, 16) → 远距离使用 pathfindRandomlyTowards
            math::Vector3 beePos = m_bee->position();
            f64 dx = beePos.x - (static_cast<f64>(hivePos.x) + 0.5);
            f64 dz = beePos.z - (static_cast<f64>(hivePos.z) + 0.5);
            f64 distSqXZ = dx * dx + dz * dz;

            if (distSqXZ > 256.0) { // 超过16格
                if (_isTooFar(hivePos)) {
                    // 太远了（超过48格），放弃蜂巢
                    resetTask();
                } else {
                    // 使用 pathfindRandomlyTowards 漂移飞行
                    (void)m_bee->pathfindRandomlyTowards(hivePos);
                }
            } else {
                // 近距离（16格内）：使用精确导航
                bool foundPath = m_bee->pathfindDirectlyTowards(hivePos);
                if (!foundPath) {
                    // 无法找到路径，放弃蜂巢
                    m_possibleHives.push_back(m_bee->getHivePos());
                    m_bee->setHivePos(BlockPos::zero());
                    m_bee->setHasHive(false);
                } else {
                    // 卡住检测：如果多次导航到同一路径仍未到达，可能卡住了
                    ++m_stuckCounter;
                    if (m_stuckCounter > STUCK_THRESHOLD) {
                        m_bee->dropHive();
                        m_stuckCounter = 0;
                    }
                }
            }
        } else {
            // 有路径，重置卡住计数
            m_stuckCounter = 0;
        }
    }
}

bool BeeFindHiveGoal::_isCloseEnough(const BlockPos& pos) const
{
    math::Vector3 beePos = m_bee->position();
    f64 distSq = (beePos.x - pos.x - 0.5) * (beePos.x - pos.x - 0.5) +
        (beePos.y - pos.y - 0.5) * (beePos.y - pos.y - 0.5) + (beePos.z - pos.z - 0.5) * (beePos.z - pos.z - 0.5);
    return distSq <= 4.0; // 2格距离平方
}

bool BeeFindHiveGoal::_isTooFar(const BlockPos& pos) const
{
    // 对应 MC 1.21.11 Bee.isTooFarAway() → !closerThan(pos, 48)
    // 使用方块中心距离的平方与 48² = 2304 比较
    math::Vector3 beePos = m_bee->position();
    f64 dx = beePos.x - (static_cast<f64>(pos.x) + 0.5);
    f64 dy = beePos.y - static_cast<f64>(pos.y);
    f64 dz = beePos.z - (static_cast<f64>(pos.z) + 0.5);
    return (dx * dx + dy * dy + dz * dz) > 2304.0; // 48格距离平方
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

    // 蜜蜂离巢后无花粉超过 600 tick（30秒）时，才想飞往已知花朵位置
    if (m_bee->getTicksWithoutNectar() <= TICKS_WITHOUT_NECTAR_THRESHOLD) {
        return false;
    }

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
    f64 distSq = (beePos.x - flowerPos.x - 0.5) * (beePos.x - flowerPos.x - 0.5) +
        (beePos.z - flowerPos.z - 0.5) * (beePos.z - flowerPos.z - 0.5);

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
    // MC 1.21.11: BeeGoToKnownFlowerGoal.stop() 中重置 maxVisitedNodesMultiplier 为默认值 1.0F
    if (auto* nav = m_bee->navigator()) {
        nav->resetMaxVisitedNodesMultiplier();
    }
}

void BeeFindFlowerGoal::tick()
{
    if (!m_bee->hasFlower()) {
        return;
    }

    ++m_ticks;

    // 对应 MC 1.21.11 BeeGoToKnownFlowerGoal.tick()
    // 超时检查：2400 tick（120秒）仍未到达则放弃花朵
    if (m_ticks > MAX_NAVIGATION_TIME) {
        m_bee->setFlowerPos(BlockPos::zero());
        return;
    }

    BlockPos flowerPos = m_bee->getFlowerPos();

    if (auto* nav = m_bee->navigator()) {
        if (!nav->isInProgress()) {
            // 检查是否太远（对应 MC: isTooFarAway → !closerThan(48)）
            if (_isTooFar(flowerPos)) {
                m_bee->setFlowerPos(BlockPos::zero());
            } else {
                // 使用 pathfindRandomlyTowards 漂移飞行
                (void)m_bee->pathfindRandomlyTowards(flowerPos);
            }
        }
    }
}

bool BeeFindFlowerGoal::_isTooFar(const BlockPos& pos) const
{
    // 对应 MC 1.21.11 Bee.isTooFarAway() → !closerThan(pos, 48)
    // 使用方块中心距离的平方与 48² = 2304 比较
    math::Vector3 beePos = m_bee->position();
    f64 dx = beePos.x - (static_cast<f64>(pos.x) + 0.5);
    f64 dy = beePos.y - static_cast<f64>(pos.y);
    f64 dz = beePos.z - (static_cast<f64>(pos.z) + 0.5);
    return (dx * dx + dy * dy + dz * dz) > 2304.0; // 48格距离平方
}

// ============================================================================
// BeeFindPollinationTargetGoal - 蜜蜂寻找授粉目标
// ============================================================================

BeeFindPollinationTargetGoal::BeeFindPollinationTargetGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{}

bool BeeFindPollinationTargetGoal::canBeeStart()
{
    // 有花粉时执行
    if (!m_bee->hasNectar()) {
        return false;
    }

    // 检查作物数限制：每次采粉后最多促进 MAX_CROPS_GROWN 棵作物生长
    if (m_bee->getCropsGrownSincePollination() >= MAX_CROPS_GROWN) {
        return false;
    }

    // 检查是否有有效蜂巢
    if (!m_bee->isHiveValid()) {
        return false;
    }

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

    // 检查脚下作物（蜜蜂下方1格和2格）
    math::Vector3 beePos = m_bee->position();
    BlockPos beeBlockPos(static_cast<i32>(beePos.x), static_cast<i32>(beePos.y), static_cast<i32>(beePos.z));

    for (i32 dy = 1; dy <= 2; ++dy) {
        BlockPos checkPos(beeBlockPos.x, beeBlockPos.y - dy, beeBlockPos.z);

        if (_isPollinationTarget(checkPos)) {
            if (_growCrop(checkPos)) {
                m_bee->addCropCounter();

                // 检查是否达到上限
                if (m_bee->getCropsGrownSincePollination() >= MAX_CROPS_GROWN) {
                    break;
                }
            }
        }
    }
}

bool BeeFindPollinationTargetGoal::_isPollinationTarget(const BlockPos& pos) const
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

bool BeeFindPollinationTargetGoal::_growCrop(const BlockPos& pos)
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    const BlockState* state = world->getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    const Block& block = state->owner();
    const BlockState* newState = nullptr;

    // 蜜蜂授粉只增加1个生长阶段（而非骨粉的2-5个阶段）

    // 1. CropBlock（小麦、胡萝卜、马铃薯、甜菜根等农作物）：
    //    如果未到最大年龄，age + 1
    auto* cropBlock = dynamic_cast<const blocks::CropBlock*>(&block);
    if (cropBlock != nullptr) {
        if (!cropBlock->isMaxAge(*state)) {
            i32 newAge = cropBlock->getAge(*state) + 1;
            newState = &cropBlock->withAge(newAge);
        }
    }

    // 2. StemBlock（西瓜茎、南瓜茎）：如果 age < 7，age + 1
    if (newState == nullptr) {
        auto* stemBlock = dynamic_cast<const blocks::StemBlock*>(&block);
        if (stemBlock != nullptr) {
            i32 age = state->get(BlockStateProperties::AGE_0_7());
            if (age < 7) {
                newState = &state->with(BlockStateProperties::AGE_0_7(), age + 1);
            }
        }
    }

    // 3. SweetBerryBushBlock：如果 age < maxAge，age + 1
    // 注意：SweetBerryBushBlock 虽然实现了 IGrowable，但 MC 原版 BeeGrowCropGoal
    // 对其直接 age+1 而非调用 performBonemeal/grow，此处与原版行为保持一致
    if (newState == nullptr) {
        auto* sweetBerryBlock = dynamic_cast<const blocks::SweetBerryBushBlock*>(&block);
        if (sweetBerryBlock != nullptr) {
            i32 age = state->get(BlockStateProperties::AGE_0_3());
            if (age < sweetBerryBlock->getMaxAge()) {
                newState = &state->with(BlockStateProperties::AGE_0_3(), age + 1);
            }
        }
    }

    // 4. CaveVines / CaveVinesPlant（洞穴藤蔓）：
    //    使用 IGrowable 接口，调用 canGrow + grow
    if (newState == nullptr) {
        auto* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&block));
        if (growable != nullptr) {
            if (growable->canGrow(static_cast<IBlockReader&>(*world), pos, *state, world->isClientSide())) {
                growable->grow(*world, world->getRandom(), pos, *state);
                // 对于洞穴藤蔓，grow() 会直接更新方块状态，
                // 需要读取更新后的状态判断是否成功
                const BlockState* updatedState = world->getBlockState(pos);
                if (updatedState == nullptr || updatedState == state) {
                    // 状态未变化，说明生长未成功
                    return false;
                }
                // 蜜蜂授粉促进生长：使用 PLANT_GROWTH_PARTICLES(2011) 而非 BONEMEAL_PARTICLES(2005)，
                // 区别是不播放骨粉使用音效
                world->playEvent(world::WorldEvents::PLANT_GROWTH_PARTICLES, pos, 15);
                return true;
            }
        }
    }

    // 如果成功生长（newState 有效），更新方块状态并播放粒子
    if (newState != nullptr) {
        world->setBlockState(pos, newState, 2);
        // 蜜蜂授粉促进生长：使用 PLANT_GROWTH_PARTICLES(2011) 而非 BONEMEAL_PARTICLES(2005)，
        // 区别是不播放骨粉使用音效
        world->playEvent(world::WorldEvents::PLANT_GROWTH_PARTICLES, pos, 15);
        return true;
    }

    return false;
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
    math::Vector3f target = _getRandomLocation();
    if (_isValidLocation(target)) {
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
            math::Vector3f target = _getRandomLocation();
            if (_isValidLocation(target)) {
                (void)nav->moveTo(target.x, target.y, target.z, 1.0);
            }
        }
    }
}

math::Vector3f BeeWanderGoal::_getRandomLocation()
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return math::Vector3f(static_cast<f32>(m_bee->x()), static_cast<f32>(m_bee->y()), static_cast<f32>(m_bee->z()));
    }

    // 对应 MC 1.21.11 Bee.WanderGoal.findPos()
    // 方向选择：如果离蜂巢太远则飞回蜂巢方向，否则使用朝向
    f64 dirX = 0.0;
    f64 dirZ = 0.0;

    if (m_bee->hasHive()) {
        BlockPos hivePos = m_bee->getHivePos();
        f64 dx = m_bee->x() - static_cast<f64>(hivePos.x);
        f64 dz = m_bee->z() - static_cast<f64>(hivePos.z);
        f64 distSq = dx * dx + dz * dz;

        // 对应 MC 1.21.11 Bee.WanderGoal.getWanderThreshold()
        // 有蜂巢/花朵时阈值=24格，无时阈值=32格
        i32 wanderThreshold = m_bee->hasFlower() ? 24 : 32;
        f64 thresholdSq = static_cast<f64>(wanderThreshold * wanderThreshold);

        if (distSq > thresholdSq) {
            // 飞回蜂巢方向（归一化）
            f64 hiveCenterX = static_cast<f64>(hivePos.x) + 0.5;
            f64 hiveCenterZ = static_cast<f64>(hivePos.z) + 0.5;
            f64 toHiveX = hiveCenterX - m_bee->x();
            f64 toHiveZ = hiveCenterZ - m_bee->z();
            f64 len = std::sqrt(toHiveX * toHiveX + toHiveZ * toHiveZ);
            if (len > 0.001) {
                dirX = toHiveX / len;
                dirZ = toHiveZ / len;
            }
        }
    }

    if (dirX * dirX + dirZ * dirZ < 0.01) {
        // 使用蜜蜂朝向作为飞行方向
        f32 yaw = m_bee->yaw() * math::DEG_TO_RAD;
        dirX = -std::sin(yaw);
        dirZ = std::cos(yaw);
    }

    // 主策略：使用 HoverRandomPos 算法
    // 在固体方块上方 1~3 格范围内选择悬停位置，确保有足够空气空间
    // 对应 MC 1.21.11 HoverRandomPos.getPos(bee, 8, 7, dirX, dirZ, PI/2, 3, 1)
    {
        Vector3 targetPos;
        if (ai::util::RandomPositionGenerator::findHoverPosition(
                m_bee, XZ_RANGE, Y_RANGE, dirX, dirZ, math::HALF_PI, 3, 1, targetPos)) {
            return math::Vector3f(targetPos.x, targetPos.y, targetPos.z);
        }
    }

    // 备选策略：使用 AirAndWaterRandomPos 算法
    // 在空中选择位置（仅移出固体方块，不要求在固体上方特定高度）
    // 对应 MC 1.21.11 AirAndWaterRandomPos.getPos(bee, 8, 4, -2, dirX, dirZ, PI/2)
    {
        Vector3 targetPos;
        if (ai::util::RandomPositionGenerator::findAirAndWaterPosition(
                m_bee, XZ_RANGE, Y_RANGE_FALLBACK, Y_OFFSET_FALLBACK, dirX, dirZ, math::HALF_PI, targetPos)) {
            return math::Vector3f(targetPos.x, targetPos.y, targetPos.z);
        }
    }

    // 最终回退：简单随机偏移
    math::Random& rng = world->getRandom();
    f64 targetX = m_bee->x() + (rng.nextDouble() * WANDER_RANGE * 2 - WANDER_RANGE) + dirX * WANDER_RANGE;
    f64 targetY = m_bee->y() + (rng.nextDouble() * WANDER_HEIGHT * 2 - WANDER_HEIGHT);
    f64 targetZ = m_bee->z() + (rng.nextDouble() * WANDER_RANGE * 2 - WANDER_RANGE) + dirZ * WANDER_RANGE;

    return math::Vector3f(static_cast<f32>(targetX), static_cast<f32>(targetY), static_cast<f32>(targetZ));
}

bool BeeWanderGoal::_isValidLocation(const math::Vector3f& pos) const
{
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return false;
    }

    // 蜜蜂是飞行生物，目标位置不应在方块内部
    BlockPos blockPos(static_cast<i32>(pos.x), static_cast<i32>(pos.y), static_cast<i32>(pos.z));
    const BlockState* state = world->getBlockState(blockPos);
    if (state != nullptr && state->isSolid()) {
        return false;
    }

    return true;
}

// ============================================================================
// BeeAngerGoal - 蜜蜂愤怒目标
// ============================================================================

BeeAngerGoal::BeeAngerGoal(BeeEntity* bee)
    : HurtByTargetGoal(bee, true) // alertAllies = true
    , m_beeEntity(bee)
{}

bool BeeAngerGoal::shouldContinueExecuting()
{
    return m_beeEntity->isAngry() && HurtByTargetGoal::shouldContinueExecuting();
}

void BeeAngerGoal::startExecuting()
{
    HurtByTargetGoal::startExecuting();

    // 蜜蜂被攻击时会召唤附近的其他蜜蜂
}

// ============================================================================
// BeeAttackPlayerGoal - 蜜蜂攻击玩家目标
// ============================================================================

BeeAttackPlayerGoal::BeeAttackPlayerGoal(BeeEntity* bee, i32 chance)
    : TargetGoal(bee, true)
    , m_beeEntity(bee)
    , m_chance(chance)
{}

bool BeeAttackPlayerGoal::shouldExecute()
{
    if (!_canSting()) {
        return false;
    }

    // 概率检查
    IWorld* world = m_beeEntity->world();
    if (world != nullptr && m_chance > 0) {
        if (world->getRandom().nextInt(m_chance) != 0) {
            return false;
        }
    }

    // 搜索附近玩家，仅攻击蜜蜂愤怒的目标玩家
    LivingEntity* currentTarget = m_beeEntity->getAttackTarget();
    Player* nearestPlayer = EntityUtils::findClosestEntity<Player>(
        world, m_beeEntity->position(), TARGET_RANGE, m_beeEntity, [this, currentTarget](Player* player) {
            if (!player->isAlive()) {
                return false;
            }
            // 蜜蜂只攻击它当前愤怒的玩家目标
            if (currentTarget != nullptr && currentTarget == player) {
                return true;
            }
            return false;
        });

    if (nearestPlayer != nullptr) {
        m_targetPlayer = nearestPlayer;
        m_target = nearestPlayer;
        return true;
    }

    return false;
}

bool BeeAttackPlayerGoal::shouldContinueExecuting()
{
    if (!_canSting()) {
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

bool BeeAttackPlayerGoal::_canSting() const
{
    return m_beeEntity->isAngry() && !m_beeEntity->hasStung();
}

// ============================================================================
// BeeResetAngerGoal - 蜜蜂重置愤怒目标
// ============================================================================

BeeResetAngerGoal::BeeResetAngerGoal(BeeEntity* bee)
    : Goal()
    , m_bee(bee)
{}

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
