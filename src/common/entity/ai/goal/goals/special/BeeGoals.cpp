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
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/core/CoordConverter.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/blocks/agricultural/StemBlock.hpp"
#include "common/world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "common/world/block/blocks/vegetation/SweetBerryBushBlock.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
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
}

void BeeStingGoal::checkAndPerformAttack(LivingEntity* target, f64 distToEnemySqr)
{
    // 记录攻击前冷却：基类 checkAndPerformAttack 仅在"距离达标 + 冷却结束(m_attackCooldown<=0)"
    // 时调 _attackTarget 造成伤害并重置 m_attackCooldown=adjustedTickDelay(20)。故"冷却由 <=0 变 >0"
    // 是攻击发生的精确判定（不依赖目标 HP 变化——目标可能因 regen/absorption 致 HP 不降，但攻击确实发生），
    // 对齐 vanilla Bee.doHurtTarget 在真正攻击时无条件 setHasStung(true)。
    const i32 cooldownBefore = m_attackCooldown;

    MeleeAttackGoal::checkAndPerformAttack(target, distToEnemySqr);

    // 攻击命中（冷却被重置为正）且未螫刺过 → 设 hasStung=true：
    // ①激活 BeeEntity::tick() 螫刺后逐渐死亡链路（此前为死代码）；
    // ②shouldExecute/shouldContinueExecuting 的 !hasStung() 检查此后为 false，蜜蜂停止蜇人
    // （对齐 vanilla 蜜蜂蜇一次即弃攻）。已 stung 则不重复设置。
    if (target != nullptr && !m_beeEntity->hasStung() && m_attackCooldown > cooldownBefore) {
        m_beeEntity->setHasStung(true);
    }
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

    // 寻找花朵（vanilla BeePollinateGoal.canBeeUse 无随机门控：冷却结束即搜索）
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
    // 对齐 vanilla BeePollinateGoal.start：resetTicksWithoutNectarSinceExitingHive()
    m_bee->resetTicksWithoutNectar();
}

bool BeePollinateGoal::requiresUpdateEveryTick() const
{
    // 对齐 vanilla BeePollinateGoal.requiresUpdateEveryTick() = true：授粉是逐 tick 驱动的
    // 悬停状态机（600 tick 超时、悬停偏移、音效节流均以 tick 计时），不能按半 tick 节流。
    return true;
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

void BeePollinateGoal::stopPollinating()
{
    // 对齐 MC Java 1.21.11 BeePollinateGoal.stopPollinating（Bee.java:1150-1152）：
    //   void stopPollinating() { this.pollinating = false; }
    // 仅置授粉标志为 false，不调 resetTask 的完整结束逻辑（setHasNectar/清路径/花朵冷却）。
    // Bee.hurtServer 受击时调用此方法立即中断授粉。
    // Cubium 同步 m_bee->setPollinating(false) 保持 bee 状态与 goal 一致。
    m_running = false;
    m_bee->setPollinating(false);
}

void BeePollinateGoal::tick()
{
    // 对齐 vanilla BeePollinateGoal.tick（Bee.java:1178-1229）：
    //   有花朵位置时 pollinatingTicks 才递增；超过 600 tick 丢花并置 200 冷却；
    //   离花心超过 1 格则悬停在花上方 0.6 处；抵达后 1/25 概率随机偏移并停止导航，
    //   期间计数成功授粉 tick 并按 5% 概率（间隔 60 tick）播放授粉音效。
    ++m_totalTicks;

    if (!m_bee->hasFlower()) {
        return;
    }

    if (m_totalTicks > MAX_POLLINATION_TIME) {
        // 对齐 vanilla：dropFlower() + 花朵冷却 200 + pollinating=false。
        // 此后本 tick 的 canBeeContinueToUse 返回 false，下一帧由 GoalSelector 调 stop()。
        m_bee->dropFlower();
        m_bee->setFlowerCooldown(200);
        m_running = false;
        m_bee->setPollinating(false);
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
        // 飞向花朵上方悬停点
        m_hoverPos = flowerCenter;
        _setWantedPos();
        return;
    }

    // 在花朵上方悬停授粉
    m_hoverPos = flowerCenter;
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return;
    }

    const f64 hoverDx = beePos.x - static_cast<f64>(m_hoverPos.x);
    const f64 hoverDy = beePos.y - static_cast<f64>(m_hoverPos.y);
    const f64 hoverDz = beePos.z - static_cast<f64>(m_hoverPos.z);
    const bool arrived =
        (hoverDx * hoverDx + hoverDy * hoverDy + hoverDz * hoverDz) <= ARRIVAL_THRESHOLD * ARRIVAL_THRESHOLD;

    if (arrived) {
        // 1/25 概率重新选取悬停点（花心水平随机偏移），并停止导航就地悬停
        math::IRandom& rng = world->getRandom();
        if (rng.nextInt(25) == 0) {
            m_hoverPos.x = flowerCenter.x + _hoverOffset();
            m_hoverPos.z = flowerCenter.z + _hoverOffset();
            if (auto* nav = m_bee->navigator()) {
                nav->clearPath();
            }
            _setWantedPos();
        }
        // 悬停时始终注视花朵
        if (auto* lookCtrl = m_bee->lookController()) {
            lookCtrl->setLookPosition(
                static_cast<f64>(flowerCenter.x), static_cast<f64>(flowerCenter.y), static_cast<f64>(flowerCenter.z));
        }
    }

    ++m_pollinationTicks;

    // 授粉进度（vanilla successfulPollinatingTicks）在抵达悬停点后逐 tick 递增
    // 授粉音效：5% 概率且距上次播放超过 60 tick（对齐 vanilla tick 内判定）
    if (world->getRandom().nextFloat() < 0.05f && m_pollinationTicks > m_lastSoundTick + 60) {
        m_lastSoundTick = m_pollinationTicks;
        world->playSound(SoundEvents::ENTITY_BEE_POLLINATE,
            sound::SoundCategory::Neutral,
            m_bee->position(),
            1.0f,
            1.0f + (world->getRandom().nextFloat() - world->getRandom().nextFloat()) * 0.2f);
    }
}

void BeePollinateGoal::_setWantedPos()
{
    // 对齐 vanilla setWantedPos：以 0.35 速度飞向悬停点。
    if (auto* moveCtrl = m_bee->moveController()) {
        moveCtrl->setMoveTo(static_cast<f64>(m_hoverPos.x),
            static_cast<f64>(m_hoverPos.y),
            static_cast<f64>(m_hoverPos.z),
            HOVER_SPEED_MODIFIER);
    }
}

f32 BeePollinateGoal::_hoverOffset()
{
    // 对齐 vanilla getOffset：(random.nextFloat() * 2 - 1) * 0.33333334
    IWorld* world = m_bee->world();
    if (world == nullptr) {
        return 0.0f;
    }
    return (world->getRandom().nextFloat() * 2.0f - 1.0f) * HOVER_POS_OFFSET;
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
    if (BlockTags::FLOWERS().contains(*state)) {
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

    // 对齐 vanilla BeePollinateGoal.findNearbyFlower：以 blockPosition 为中心做曼哈顿半径 5 的
    // 立方体扫描，命中第一朵可达花朵即返回。
    for (i32 dx = -5; dx <= 5; ++dx) {
        for (i32 dy = -5; dy <= 5; ++dy) {
            for (i32 dz = -5; dz <= 5; ++dz) {
                if (std::abs(dx) + std::abs(dy) + std::abs(dz) > 5) {
                    continue;
                }
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

    // 对齐 vanilla：搜索不到花朵时把寻花冷却设为 20~60 tick，避免每 tick 全量扫描。
    m_bee->setFlowerCooldown(world->getRandom().nextInt(20, 60));
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
// BeeValidateHiveGoal / BeeValidateFlowerGoal - 校验已记录目标
// ============================================================================

namespace {
/// 校验冷却 20~40 tick（对齐 vanilla Mth.nextInt(random, 20, 40)）
i32 _randomValidateCooldown(BeeEntity* bee)
{
    IWorld* world = bee->world();
    math::IRandom& rng = world != nullptr ? world->getRandom() : bee->getRandom();
    return rng.nextInt(20, 40);
}

/// 目标位置所在区块是否已加载（对齐 vanilla Level.isLoaded(pos)）
bool _isPositionLoaded(BeeEntity* bee, const BlockPos& pos)
{
    IWorld* world = bee->world();
    if (world == nullptr || world->chunkManager() == nullptr) {
        return false;
    }
    return world->chunkManager()->hasChunkInMem(
        mc::util::core::CoordConverter::blockToChunk(pos.x), mc::util::core::CoordConverter::blockToChunk(pos.z));
}
} // namespace

BeeValidateHiveGoal::BeeValidateHiveGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
    , m_cooldown(_randomValidateCooldown(bee))
{}

bool BeeValidateHiveGoal::canBeeStart()
{
    IWorld* world = m_bee->world();
    const i64 gameTime = world != nullptr ? static_cast<i64>(world->getGameTime()) : 0;
    return gameTime > m_lastValidateTick + m_cooldown;
}

bool BeeValidateHiveGoal::canBeeContinue()
{
    return false;
}

void BeeValidateHiveGoal::startExecuting()
{
    IWorld* world = m_bee->world();
    if (m_bee->hasHive()) {
        const BlockPos hivePos = m_bee->getHivePos();
        if (_isPositionLoaded(m_bee, hivePos) && !m_bee->isHiveValid()) {
            m_bee->dropHive();
        }
    }
    m_lastValidateTick = world != nullptr ? static_cast<i64>(world->getGameTime()) : 0;
}

BeeValidateFlowerGoal::BeeValidateFlowerGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
    , m_cooldown(_randomValidateCooldown(bee))
{}

bool BeeValidateFlowerGoal::canBeeStart()
{
    IWorld* world = m_bee->world();
    const i64 gameTime = world != nullptr ? static_cast<i64>(world->getGameTime()) : 0;
    return gameTime > m_lastValidateTick + m_cooldown;
}

bool BeeValidateFlowerGoal::canBeeContinue()
{
    return false;
}

void BeeValidateFlowerGoal::startExecuting()
{
    IWorld* world = m_bee->world();
    if (m_bee->hasFlower()) {
        const BlockPos flowerPos = m_bee->getFlowerPos();
        const BlockState* state = world != nullptr ? world->getBlockState(flowerPos) : nullptr;
        if (_isPositionLoaded(m_bee, flowerPos) && (state == nullptr || !BeeEntity::attractsBees(*state))) {
            m_bee->dropFlower();
        }
    }
    m_lastValidateTick = world != nullptr ? static_cast<i64>(world->getGameTime()) : 0;
}

// ============================================================================
// BeeUpdateHiveGoal - 蜜蜂更新蜂巢位置目标
// ============================================================================

BeeUpdateHiveGoal::BeeUpdateHiveGoal(BeeEntity* bee)
    : BeePassiveGoal(bee)
{}

bool BeeUpdateHiveGoal::canBeeStart()
{
    // 对齐 vanilla BeeLocateHiveGoal.canBeeUse：冷却为 0 && 无蜂巢 && 想回巢
    if (m_bee->getHiveLocateCooldown() > 0 || m_bee->hasHive()) {
        return false;
    }
    return m_bee->wantsToEnterHive();
}

bool BeeUpdateHiveGoal::canBeeContinue()
{
    // 一次性执行
    return false;
}

void BeeUpdateHiveGoal::startExecuting()
{
    // 对齐 vanilla BeeLocateHiveGoal.start：先置 200 冷却，再取首个不在黑名单中的蜂巢；
    // 全在黑名单中时清空黑名单并回退到首个蜂巢。
    m_bee->setHiveLocateCooldown(200);

    auto hives = _findNearbyFreeHives();
    if (hives.empty()) {
        return;
    }
    for (const BlockPos& hive : hives) {
        if (!isBlacklisted(hive)) {
            m_bee->setHivePos(hive);
            return;
        }
    }
    clearBlacklist();
    m_bee->setHivePos(hives[0]);
}

bool BeeUpdateHiveGoal::isBlacklisted(const BlockPos& pos) const
{
    return std::find(m_blacklist.begin(), m_blacklist.end(), pos) != m_blacklist.end();
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
    // 对齐 vanilla BeeGoToHiveGoal.canBeeUse：
    //   有蜂巢位置 && 未太远 && 没有"家" && 想回巢 && 未抵达 && 该位置仍是 BEEHIVES 方块。
    if (!m_bee->hasHive()) {
        return false;
    }
    const BlockPos hivePos = m_bee->getHivePos();
    if (_isTooFar(hivePos) || m_bee->hasHome() || !m_bee->wantsToEnterHive() || _hasReachedTarget(hivePos)) {
        return false;
    }
    IWorld* world = m_bee->world();
    const BlockState* state = world != nullptr ? world->getBlockState(hivePos) : nullptr;
    return state != nullptr && !state->isAir() && BlockTags::BEEHIVES().contains(*state);
}

bool BeeFindHiveGoal::canBeeContinue()
{
    return canBeeStart();
}

bool BeeFindHiveGoal::_hasReachedTarget(const BlockPos& pos) const
{
    if (_isCloseEnough(pos)) {
        return true;
    }
    // 最后一段路径的终点即蜂巢且已完成（对齐 vanilla path.getTarget().equals(pos) && canReach && isDone）
    if (auto* nav = m_bee->navigator()) {
        const pathfinding::Path* path = nav->getPath();
        if (path != nullptr && path->getTarget() == pos && path->reachesTarget() && nav->isDone()) {
            return true;
        }
    }
    return false;
}

void BeeFindHiveGoal::_dropAndBlacklistHive()
{
    if (m_bee->hasHive()) {
        const BlockPos hivePos = m_bee->getHivePos();
        if (std::find(m_possibleHives.begin(), m_possibleHives.end(), hivePos) == m_possibleHives.end()) {
            if (m_possibleHives.size() >= BeeUpdateHiveGoal::MAX_BLACKLIST_SIZE) {
                m_possibleHives.erase(m_possibleHives.begin());
            }
            m_possibleHives.push_back(hivePos);
        }
    }
    m_bee->dropHive();
}

void BeeFindHiveGoal::startExecuting()
{
    m_ticks = 0;
    m_stuckCounter = 0;
    m_possibleHives.clear();
}

void BeeFindHiveGoal::resetTask()
{
    // 对齐 vanilla BeeGoToHiveGoal.stop：把未能到达的蜂巢加入黑名单（限长 5），
    // 并重置寻路最大访问节点倍数。dropHive 内部含 200 tick 的离巢冷却。
    if (m_bee->hasHive()) {
        const BlockPos hivePos = m_bee->getHivePos();
        if (std::find(m_possibleHives.begin(), m_possibleHives.end(), hivePos) == m_possibleHives.end()) {
            if (m_possibleHives.size() >= BeeUpdateHiveGoal::MAX_BLACKLIST_SIZE) {
                m_possibleHives.erase(m_possibleHives.begin());
            }
            m_possibleHives.push_back(hivePos);
        }
    }
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
    // 超时检查：adjustedTickDelay(2400)（本 goal 未置 requiresUpdateEveryTick，故折半）
    // 仍未到达则放弃并拉黑蜂巢
    if (m_ticks > adjustedTickDelay(MAX_NAVIGATION_TIME)) {
        _dropAndBlacklistHive();
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
                    // 无法找到路径，放弃蜂巢并拉黑
                    _dropAndBlacklistHive();
                } else {
                    // 卡住检测（对齐 vanilla）：连续两次生成的路径完全相同说明卡住，
                    // 每 2 tick 评估一次下 60 次约合 120 tick，超过即丢弃蜂巢。
                    std::vector<BlockPos> currentPath;
                    if (const pathfinding::Path* path = nav->getPath(); path != nullptr) {
                        const std::vector<pathfinding::PathPoint>& points = path->getPoints();
                        currentPath.reserve(points.size());
                        for (const pathfinding::PathPoint& point : points) {
                            currentPath.push_back(BlockPos(point.x(), point.y(), point.z()));
                        }
                    }
                    if (!m_lastPath.empty() && currentPath == m_lastPath) {
                        ++m_stuckCounter;
                        if (m_stuckCounter > STUCK_THRESHOLD) {
                            m_bee->dropHive();
                            m_stuckCounter = 0;
                        }
                    } else {
                        m_lastPath = std::move(currentPath);
                    }
                }
            }
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
    // 对齐 vanilla BeeGoToKnownFlowerGoal.canBeeUse：有记录的花朵、没有蜂巢（未在归巢途中）、
    // 离巢无花粉超过 600 tick 且距离花朵超过 2 格时，才飞向已知花朵。
    if (!m_bee->hasFlower() || m_bee->hasHive()) {
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

    // 检查是否是花朵（#flowers 已包含 #small_flowers，无需再单独判断）
    if (!BlockTags::FLOWERS().contains(*state)) {
        return false;
    }

    // 检查是否已经足够近（对齐 vanilla !closerThan(flowerPos, 2)：完整三维距离）
    const math::Vector3 beePos = m_bee->position();
    const BlockPos flowerPos = m_bee->getFlowerPos();
    const f64 dx = beePos.x - (static_cast<f64>(flowerPos.x) + 0.5);
    const f64 dy = beePos.y - static_cast<f64>(flowerPos.y);
    const f64 dz = beePos.z - (static_cast<f64>(flowerPos.z) + 0.5);
    return dx * dx + dy * dy + dz * dz > 4.0;
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
    // 对齐 vanilla BeeGoToKnownFlowerGoal.stop：放弃花朵走 dropFlower（含 20~60 tick 冷却）。
    m_bee->dropFlower();
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
    // 超时检查：adjustedTickDelay(2400)（本 goal 未置 requiresUpdateEveryTick，故折半）
    // 仍未到达则放弃花朵。
    if (m_ticks > adjustedTickDelay(MAX_NAVIGATION_TIME)) {
        m_bee->dropFlower();
        return;
    }

    BlockPos flowerPos = m_bee->getFlowerPos();

    if (auto* nav = m_bee->navigator()) {
        if (!nav->isInProgress()) {
            // 检查是否太远（对应 MC: isTooFarAway → !closerThan(48)）
            if (_isTooFar(flowerPos)) {
                m_bee->dropFlower();
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

    // 对齐 vanilla BeeGrowCropGoal.canBeeUse：30% 概率跳过；10% 概率拒绝当次检查。
    IWorld* world = m_bee->world();
    if (world == nullptr || world->getRandom().nextFloat() >= 0.3f) {
        return false;
    }

    // vanilla: random.nextFloat() < 0.3F ? false : (hasNectar() && isHiveValid())
    // 即 30% 概率本 tick 不催熟；其余 70% 中再按 hasNectar && isHiveValid 判定。
    // 已被玩家用花朵吸引时不催熟（对齐 Bee.isTempted 的门控）。
    if (m_bee->isTempted()) {
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

    // 对齐 vanilla BeeGrowCropGoal.tick：random.nextInt(adjustedTickDelay(30)) == 0
    // （本 goal 未置 requiresUpdateEveryTick，故 30 按半 tick 补偿为 15）
    if (world->getRandom().nextInt(adjustedTickDelay(30)) != 0) {
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

    // 对齐 vanilla BeeGrowCropGoal：HAS_NECTAR 的符号是"已采到花蜜、正在传播"，与
    // FALLING_NECTAR 粒子一致；而 BeeGrowCropGoal 只对 BEE_GROWABLES 标签内的作物生效。
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

    // 对齐 vanilla BeeGrowCropGoal：作物 +1 龄、茎 +1、甜浆果丛 +1、洞穴藤蔓按可催熟逻辑生长。
    if (const auto* cropBlock = dynamic_cast<const blocks::CropBlock*>(&block)) {
        if (!cropBlock->isMaxAge(*state)) {
            newState = &cropBlock->withAge(cropBlock->getAge(*state) + 1);
        }
    } else if (dynamic_cast<const blocks::StemBlock*>(&block) != nullptr) {
        const i32 age = state->get(BlockStateProperties::AGE_0_7());
        if (age < 7) {
            newState = &state->with(BlockStateProperties::AGE_0_7(), age + 1);
        }
    } else if (state->is(block_registry::VegetationBlocks::SWEET_BERRY_BUSH)) {
        const i32 age = state->get(BlockStateProperties::AGE_0_3());
        if (age < 3) {
            newState = &state->with(BlockStateProperties::AGE_0_3(), age + 1);
        }
    } else if (state->is(block_registry::CaveBlocks::CAVE_VINES) ||
        state->is(block_registry::CaveBlocks::CAVE_VINES_PLANT)) {
        auto* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&block));
        if (growable != nullptr &&
            growable->canGrow(static_cast<IBlockReader&>(*world), pos, *state, world->isClientSide())) {
            growable->grow(*world, world->getRandom(), pos, *state);
            newState = world->getBlockState(pos);
        }
    }

    // 如果成功生长（newState 有效），更新方块状态并播放粒子
    if (newState != nullptr) {
        world->setBlockState(pos, newState, world::BlockUpdateFlags::UPDATE_CLIENTS);
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

    if (m_bee->isHiveValid()) {
        BlockPos hivePos = m_bee->getHivePos();
        f64 dx = m_bee->x() - static_cast<f64>(hivePos.x);
        f64 dz = m_bee->z() - static_cast<f64>(hivePos.z);
        f64 distSq = dx * dx + dz * dz;

        // 对应 MC 1.21.11 Bee.WanderGoal.getWanderThreshold()：
        //   有蜂巢或有已记录花朵 → 48 - 24 = 24 格；两者皆无 → 48 - 16 = 32 格。
        const bool hasHiveOrFlower = m_bee->hasHive() || m_bee->hasFlower();
        const i32 wanderThreshold = hasHiveOrFlower ? 24 : 32;
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
    math::IRandom& rng = world->getRandom();
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

    // 受击后锁定复仇目标并进入愤怒状态。
    // vanilla Bee 是 NeutralMob：受击时由 LivingEntity.hurt 链路设愤怒目标
    // （setLastHurtByMob + setPersistentAngerTarget + setRemainingAngerTime）。
    // Cubium 无 NeutralMob 中间层，将"受击设愤怒"语义落在 setRevengeTarget：
    //   setAttackTarget(target) + setAngry(true) + revengeTargetId + revengeTimer。
    // HurtByTargetGoal::startExecuting 已通过 TargetGoal::startExecuting 设了 attackTarget，
    // 此处补设愤怒（setAngry→setAngerTime(MAX_ANGER_TIME)→isAngry()=true），否则
    // BeeStingGoal::shouldExecute 的 isAngry() 检查恒为 false，蜜蜂有目标却不蜇人。
    // m_target 由 HurtByTargetGoal::shouldExecute 设为 getLastHurtBy()（攻击者），此处非空。
    if (m_target != nullptr) {
        m_beeEntity->setRevengeTarget(m_target.get());
    }
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
        m_beeEntity->setAttackTarget(m_target.get());
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
