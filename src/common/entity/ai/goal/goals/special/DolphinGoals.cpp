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

#include "DolphinGoals.hpp"
#include "../../../../../entity/core/LivingEntity.hpp"
#include "../../../../../entity/core/MobEntity.hpp"
#include "../../../../../entity/effect/EffectInstance.hpp"
#include "../../../../../entity/effect/EffectType.hpp"
#include "../../../../../entity/entities/item/ItemEntity.hpp"
#include "../../../../../entity/entities/passive/water/DolphinEntity.hpp"
#include "../../../../../entity/entities/player/Player.hpp"
#include "../../../../../entity/registry/VanillaEntityTypeKeys.hpp"
#include "../../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../../item/core/ItemStack.hpp"
#include "../../../../../resource/ResourceLocation.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../world/block/Block.hpp"
#include "../../../../../world/fluid/Fluid.hpp"
#include "../../../../../world/fluid/FluidTags.hpp"
#include "../../../util/RandomPositionGenerator.hpp"
#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <optional>
#include <vector>

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// DolphinJumpGoal
// ============================================================================

DolphinJumpGoal::DolphinJumpGoal(DolphinEntity* dolphin, i32 chance)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Jump, GoalFlag::Move})
    , m_dolphin(dolphin)
    , m_chance(chance)
{
    MC_ASSERT_RELEASE(dolphin != nullptr);
}

bool DolphinJumpGoal::shouldExecute()
{
    // 随机概率检查
    math::Random& rng = m_dolphin->getRandom();
    if (rng.nextInt(m_chance) != 0) {
        return false;
    }

    // 获取海豚朝向（从 yaw 计算）
    f32 yaw = m_dolphin->yaw();
    // 将 yaw 转换为方向向量
    f32 yawRad = yaw * math::DEG_TO_RAD;
    i32 dx = static_cast<i32>(std::round(std::sin(yawRad)));
    i32 dz = static_cast<i32>(-std::round(std::cos(yawRad)));

    Vector3 pos = m_dolphin->position();
    BlockPos blockPos(
        static_cast<i32>(std::floor(pos.x)), static_cast<i32>(std::floor(pos.y)), static_cast<i32>(std::floor(pos.z)));

    // 检查所有跳跃距离
    for (i32 scale : JUMP_DISTANCES) {
        if (!_canJumpTo(blockPos, dx, dz, scale) || !_isAirAbove(blockPos, dx, dz, scale)) {
            return false;
        }
    }

    return true;
}

bool DolphinJumpGoal::shouldContinueExecuting()
{
    // 检查垂直速度和俯仰角
    f32 vy = m_dolphin->velocityY();
    bool hasVerticalMotion = (static_cast<f64>(vy) * vy >= 0.03);

    // 检查俯仰角是否接近零
    f32 pitch = m_dolphin->pitch();
    bool pitchNearZero = (std::abs(pitch) < 10.0f);

    // 检查是否在水中
    bool inWater = m_dolphin->isInWater();

    // 检查是否在地面上
    bool onGround = m_dolphin->onGround();

    // 继续执行的条件：有垂直运动 或 俯仰角不为零 或 不在水中 或 不在地面上
    return (hasVerticalMotion || !pitchNearZero || !inWater) && !onGround;
}

void DolphinJumpGoal::startExecuting()
{
    // 根据朝向设置跳跃速度
    f32 yaw = m_dolphin->yaw();
    f32 yawRad = yaw * math::DEG_TO_RAD;
    i32 dx = static_cast<i32>(std::round(std::sin(yawRad)));
    i32 dz = static_cast<i32>(-std::round(std::cos(yawRad)));

    // 设置跳跃速度 (水平 0.6, 垂直 0.7)
    Vector3 vel = m_dolphin->velocity();
    f32 vx = vel.x + static_cast<f32>(dx) * 0.6f;
    f32 vy = 0.7f;
    f32 vz = vel.z + static_cast<f32>(dz) * 0.6f;

    m_dolphin->setVelocity(vx, vy, vz);

    // 清除路径
    m_dolphin->clearNavigationPath();

    m_inWater = m_dolphin->isInWater();
}

void DolphinJumpGoal::resetTask()
{
    m_dolphin->setRotationPitch(0.0f);
}

void DolphinJumpGoal::tick()
{
    bool wasInWater = m_inWater;

    if (!wasInWater) {
        // 检查是否进入水中
        IWorld* world = m_dolphin->world();
        if (world != nullptr) {
            Vector3 pos = m_dolphin->position();
            BlockPos blockPos(static_cast<i32>(std::floor(pos.x)),
                static_cast<i32>(std::floor(pos.y)),
                static_cast<i32>(std::floor(pos.z)));
            const fluid::FluidState* fluidState = world->getFluidState(blockPos);
            m_inWater = fluidState != nullptr && !fluidState->isEmpty() &&
                fluidState->getFluid().isIn(fluid::FluidTags::WATER());
        }
    }

    // 刚入水时播放跳跃音效
    if (m_inWater && !wasInWater) {
        m_dolphin->playSound(SoundEvents::ENTITY_DOLPHIN_JUMP, 1.0f, 1.0f);
    }

    // 根据运动方向计算俯仰角
    Vector3 velocity = m_dolphin->velocity();
    f64 vy = velocity.y;
    f64 horizontalSpeed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);

    if (vy * vy < 0.03 && m_dolphin->pitch() != 0.0f) {
        // 缓慢恢复俯仰角
        m_dolphin->setRotationPitch(math::lerp(m_dolphin->pitch(), 0.0f, 0.2f));
    } else {
        // 根据运动方向计算俯仰角
        f64 pitch = std::copysign(std::acos(horizontalSpeed / velocity.length()) * (180.0 / math::PI), -vy);
        m_dolphin->setRotationPitch(static_cast<f32>(pitch));
    }
}

bool DolphinJumpGoal::_canJumpTo(const BlockPos& pos, i32 dx, i32 dz, i32 scale) const
{
    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return false;
    }

    BlockPos targetPos(pos.x + dx * scale, pos.y, pos.z + dz * scale);

    // 检查目标位置是否是水
    const fluid::FluidState* fluidState = world->getFluidState(targetPos);
    if (fluidState == nullptr || fluidState->isEmpty() || !fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    // 检查目标位置是否有阻挡移动的方块
    const BlockState* state = world->getBlockState(targetPos);
    if (state != nullptr && state->getMaterial().blocksMovement()) {
        return false;
    }

    return true;
}

bool DolphinJumpGoal::_isAirAbove(const BlockPos& pos, i32 dx, i32 dz, i32 scale) const
{
    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return false;
    }

    // 检查目标位置上方一格和两格是否是空气
    BlockPos above1(pos.x + dx * scale, pos.y + 1, pos.z + dz * scale);
    BlockPos above2(pos.x + dx * scale, pos.y + 2, pos.z + dz * scale);

    const BlockState* state1 = world->getBlockState(above1);
    const BlockState* state2 = world->getBlockState(above2);

    bool isAir1 = (state1 == nullptr || state1->isAir());
    bool isAir2 = (state2 == nullptr || state2->isAir());

    return isAir1 && isAir2;
}

// ============================================================================
// SwimToTreasureGoal
// ============================================================================

SwimToTreasureGoal::SwimToTreasureGoal(DolphinEntity* dolphin)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_dolphin(dolphin)
{
    MC_ASSERT_RELEASE(dolphin != nullptr);
}

bool SwimToTreasureGoal::shouldExecute()
{
    // 检查是否得到了鱼且空气充足
    if (!m_dolphin->hasGotFish()) {
        return false;
    }

    if (m_dolphin->air() < MIN_AIR) {
        return false;
    }

    return true;
}

bool SwimToTreasureGoal::shouldContinueExecuting()
{
    if (m_failed) {
        return false;
    }

    if (m_dolphin->air() < MIN_AIR) {
        return false;
    }

    // 检查是否接近宝藏位置
    BlockPos treasurePos = m_dolphin->getTreasurePos();
    f64 dx = static_cast<f64>(treasurePos.x) - m_dolphin->x();
    f64 dz = static_cast<f64>(treasurePos.z) - m_dolphin->z();
    f64 distSq = dx * dx + dz * dz;

    return distSq > static_cast<f64>(ARRIVE_DISTANCE * ARRIVE_DISTANCE);
}

void SwimToTreasureGoal::startExecuting()
{
    m_failed = false;
    m_dolphin->clearNavigationPath();

    // 对应 MC 1.21.11 DolphinSwimToTreasureGoal.start():
    //   BlockPos blockpos = this.dolphin.blockPosition();
    //   BlockPos blockpos1 = serverlevel.findNearestMapStructure(StructureTags.DOLPHIN_LOCATED, blockpos, 50, false);
    //   if (blockpos1 != null) {
    //       this.dolphin.treasurePos = blockpos1;
    //       serverlevel.broadcastEntityState(this.dolphin, (byte)38);
    //   } else {
    //       this.stuck = true;
    //   }
    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        m_failed = true;
        return;
    }

    // 获取海豚的方块位置
    Vector3 pos = m_dolphin->position();
    BlockPos dolphinPos(
        static_cast<i32>(std::floor(pos.x)), static_cast<i32>(std::floor(pos.y)), static_cast<i32>(std::floor(pos.z)));

    // 通过 minecraft:dolphin_located 结构标签查找最近的沉船或海底废墟
    auto treasurePos = world->findNearestMapStructure(
        dolphinPos, ResourceLocation("minecraft", "dolphin_located"), SEARCH_RADIUS_BLOCKS, false);

    if (treasurePos.has_value()) {
        m_dolphin->setTreasurePos(treasurePos.value());
        // 广播实体状态 38（Dolphin），触发客户端播放 HAPPY_VILLAGER 粒子效果
        world->broadcastEntityStatus(m_dolphin->id(), static_cast<u8>(mc::network::EntityStatus::Dolphin));
    } else {
        m_failed = true;
    }
}

void SwimToTreasureGoal::resetTask()
{
    BlockPos treasurePos = m_dolphin->getTreasurePos();

    // 检查是否到达宝藏
    f64 dx = static_cast<f64>(treasurePos.x) - m_dolphin->x();
    f64 dz = static_cast<f64>(treasurePos.z) - m_dolphin->z();
    f64 distSq = dx * dx + dz * dz;

    if (distSq <= static_cast<f64>(ARRIVE_DISTANCE * ARRIVE_DISTANCE) || m_failed) {
        m_dolphin->setGotFish(false);
    }
}

void SwimToTreasureGoal::tick()
{
    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        m_failed = true;
        return;
    }

    BlockPos treasurePos = m_dolphin->getTreasurePos();

    // 如果接近目标或路径结束，重新计算路径
    if (m_dolphin->closeToTarget() || !m_dolphin->hasPath()) {
        // 计算朝向宝藏的中心位置
        Vector3 treasureCenter(static_cast<f64>(treasurePos.x) + 0.5,
            static_cast<f64>(treasurePos.y) + 0.5,
            static_cast<f64>(treasurePos.z) + 0.5);

        // 生成路径点
        Vector3 targetPos;
        bool hasPath = entity::ai::util::RandomPositionGenerator::findRandomTargetTowardsScaled(
            m_dolphin, 16, 1, treasureCenter, math::PI / 8.0, targetPos);

        // 如果第一个方法失败，尝试 findRandomTargetBlockTowards
        if (!hasPath) {
            hasPath = entity::ai::util::RandomPositionGenerator::findRandomTargetBlockTowards(
                m_dolphin, 8, 4, treasureCenter, targetPos);
        }

        // 如果仍然失败，检查目标位置是否是水
        if (hasPath) {
            BlockPos targetBlockPos(static_cast<i32>(std::floor(targetPos.x)),
                static_cast<i32>(std::floor(targetPos.y)),
                static_cast<i32>(std::floor(targetPos.z)));

            const fluid::FluidState* fluidState = world->getFluidState(targetBlockPos);
            if (fluidState == nullptr || fluidState->isEmpty() ||
                !fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                // 目标不是水，尝试其他位置
                hasPath = entity::ai::util::RandomPositionGenerator::findRandomTargetBlock(
                    m_dolphin, 8, 5, std::nullopt, targetPos);
            }
        }

        if (!hasPath) {
            m_failed = true;
            return;
        }

        // 看向目标
        m_dolphin->lookAt(static_cast<f64>(targetPos.x), static_cast<f64>(targetPos.y), static_cast<f64>(targetPos.z));

        // 导航到目标位置
        hasPath = m_dolphin->tryMoveTo(targetPos.x, targetPos.y, targetPos.z, 1.3);
        if (!hasPath) {
            m_failed = true;
            return;
        }
    }

    // 随机播放粒子效果
    math::Random& rng = m_dolphin->getRandom();
    if (rng.nextInt(80) == 0) {
        world->broadcastEntityStatus(m_dolphin->id(), static_cast<u8>(mc::network::EntityStatus::Dolphin));
    }
}

// ============================================================================
// SwimWithPlayerGoal
// ============================================================================

SwimWithPlayerGoal::SwimWithPlayerGoal(DolphinEntity* dolphin, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_dolphin(dolphin)
    , m_speed(speed)
{
    MC_ASSERT_RELEASE(dolphin != nullptr);
}

bool SwimWithPlayerGoal::shouldExecute()
{
    m_targetPlayer = _findSwimmingPlayer();
    return m_targetPlayer != nullptr;
}

bool SwimWithPlayerGoal::shouldContinueExecuting()
{
    if (m_targetPlayer == nullptr) {
        return false;
    }

    if (!m_targetPlayer->isSwimming()) {
        return false;
    }

    f32 distSq = m_dolphin->distanceSqTo(*m_targetPlayer);
    return static_cast<f64>(distSq) < MAX_DISTANCE_SQ;
}

void SwimWithPlayerGoal::startExecuting()
{
    if (m_targetPlayer != nullptr) {
        // 给玩家添加海豚的恩惠效果
        m_targetPlayer->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::DolphinsGrace,
            EFFECT_DURATION,
            0,     // amplifier = 0 (I级)
            false, // ambient
            true,  // visible
            true   // showIcon
            ));
    }
}

void SwimWithPlayerGoal::resetTask()
{
    m_targetPlayer = nullptr;
    m_dolphin->clearNavigationPath();
}

void SwimWithPlayerGoal::tick()
{
    if (m_targetPlayer == nullptr) {
        return;
    }

    // 看向玩家
    m_dolphin->lookAt(static_cast<f64>(m_targetPlayer->x()),
        static_cast<f64>(m_targetPlayer->y() + m_targetPlayer->eyeHeight()),
        static_cast<f64>(m_targetPlayer->z()));

    f32 distSq = m_dolphin->distanceSqTo(*m_targetPlayer);

    if (static_cast<f64>(distSq) < CLOSE_DISTANCE_SQ) {
        // 太近了，停止移动
        m_dolphin->clearNavigationPath();
    } else {
        // 跟随玩家
        m_dolphin->tryMoveToEntity(*m_targetPlayer, m_speed);
    }

    // 定期刷新海豚的恩惠效果
    IWorld* world = m_dolphin->world();
    if (world != nullptr && m_targetPlayer->isSwimming()) {
        math::Random& rng = m_dolphin->getRandom();
        if (rng.nextInt(EFFECT_INTERVAL) == 0) {
            m_targetPlayer->addEffect(entity::effect::EffectInstance(
                entity::effect::EffectType::DolphinsGrace, EFFECT_DURATION, 0, false, true, true));
        }
    }
}

Player* SwimWithPlayerGoal::_findSwimmingPlayer() const
{
    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return nullptr;
    }

    // 获取附近所有实体
    std::vector<Entity*> entities = world->getEntitiesInRange(m_dolphin->position(), SEARCH_RADIUS, m_dolphin);

    Player* closestPlayer = nullptr;
    f64 closestDist = SEARCH_RADIUS * SEARCH_RADIUS;

    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }

        // 检查是否是玩家
        Player* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 检查是否在游泳
        if (!player->isSwimming()) {
            continue;
        }

        // 检查是否是攻击目标
        LivingEntity* attackTarget = m_dolphin->attackTarget();
        if (attackTarget == player) {
            continue;
        }

        f32 distSq = m_dolphin->distanceSqTo(*player);
        if (static_cast<f64>(distSq) < closestDist) {
            closestDist = static_cast<f64>(distSq);
            closestPlayer = player;
        }
    }

    return closestPlayer;
}

// ============================================================================
// PlayWithItemsGoal
// ============================================================================

PlayWithItemsGoal::PlayWithItemsGoal(DolphinEntity* dolphin)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_dolphin(dolphin)
{
    MC_ASSERT_RELEASE(dolphin != nullptr);
}

bool PlayWithItemsGoal::shouldExecute()
{
    u32 ticksExisted = m_dolphin->ticksExisted();
    if (static_cast<i32>(ticksExisted) < m_cooldown) {
        return false;
    }

    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return false;
    }

    // 检查附近是否有物品或海豚是否持有物品
    // 首先检查主手是否持有物品
    const ItemStack& mainHandItem = m_dolphin->getMainHandItem();
    if (!mainHandItem.isEmpty()) {
        return true;
    }

    // 检查附近是否有物品
    ItemEntity* item = _findNearbyItem();
    return item != nullptr;
}

void PlayWithItemsGoal::startExecuting()
{
    // 检查是否已持有物品
    const ItemStack& mainHandItem = m_dolphin->getMainHandItem();
    if (!mainHandItem.isEmpty()) {
        // 已持有物品，扔出
        ItemStack copy = mainHandItem;
        m_dolphin->setMainHandItem(ItemStack());
        _throwItem(copy);
        m_cooldown = static_cast<i32>(m_dolphin->ticksExisted()) + MIN_COOLDOWN;
        return;
    }

    // 没有持有物品，寻找附近物品
    ItemEntity* item = _findNearbyItem();
    if (item != nullptr) {
        m_dolphin->tryMoveToEntity(*item, 1.2);
        m_dolphin->playSound(SoundEvents::ENTITY_DOLPHIN_PLAY, 1.0f, 1.0f);
    }

    m_cooldown = 0;
}

void PlayWithItemsGoal::resetTask()
{
    // 任务结束时扔出手中物品
    ItemStack mainHandItem = m_dolphin->getMainHandItem();
    if (!mainHandItem.isEmpty()) {
        m_dolphin->setMainHandItem(ItemStack());
        _throwItem(mainHandItem);
        // 设置随机冷却 (0-99 ticks)
        math::Random& rng = m_dolphin->getRandom();
        m_cooldown = static_cast<i32>(m_dolphin->ticksExisted()) + rng.nextInt(100);
    }
}

void PlayWithItemsGoal::tick()
{
    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return;
    }

    // 完整的物品拾取和扔出逻辑
    // 1. 检查是否持有物品
    const ItemStack& mainHandItem = m_dolphin->getMainHandItem();

    if (!mainHandItem.isEmpty()) {
        // 持有物品时，随机决定是否扔出
        math::Random& rng = m_dolphin->getRandom();
        if (rng.nextInt(40) == 0) {
            // 扔出物品
            ItemStack copy = mainHandItem;
            m_dolphin->setMainHandItem(ItemStack());
            _throwItem(copy);
            m_cooldown = static_cast<i32>(m_dolphin->ticksExisted()) + MIN_COOLDOWN;
        }
        return;
    }

    // 2. 没有持有物品，查找附近物品并移动过去
    ItemEntity* item = _findNearbyItem();
    if (item != nullptr) {
        // 检查是否足够靠近可以拾取
        f64 distSq = m_dolphin->distanceSqTo(*item);
        constexpr f64 PICKUP_DISTANCE_SQ = 2.25; // 1.5 * 1.5

        if (distSq < PICKUP_DISTANCE_SQ) {
            // 拾取物品
            ItemStack stack = item->getItemStack();
            m_dolphin->setMainHandItem(stack);

            // 标记物品实体为移除
            item->remove();
        } else {
            // 向物品移动
            m_dolphin->tryMoveToEntity(*item, 1.2);
        }
    }
}

void PlayWithItemsGoal::_throwItem(ItemStack& stack)
{
    if (stack.isEmpty()) {
        return;
    }

    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return;
    }

    // 在海豚眼睛位置略下方生成物品
    f64 y = m_dolphin->y() + m_dolphin->eyeHeight() - 0.3;

    // 计算扔出速度
    math::Random& rng = m_dolphin->getRandom();
    f32 angle = rng.nextFloat() * math::TWO_PI;
    f32 inaccuracy = 0.02f * rng.nextFloat();

    // 基于海豚朝向计算速度
    f32 yaw = m_dolphin->yaw();
    f32 pitch = m_dolphin->pitch();
    f32 cosYaw = std::cos(yaw * math::DEG_TO_RAD);
    f32 sinYaw = std::sin(yaw * math::DEG_TO_RAD);
    f32 cosPitch = std::cos(pitch * math::DEG_TO_RAD);
    f32 sinPitch = std::sin(pitch * math::DEG_TO_RAD);

    f32 vx = THROW_VELOCITY * (-sinYaw * cosPitch) + std::cos(angle) * inaccuracy;
    f32 vy = THROW_VELOCITY * sinPitch * 1.5f;
    f32 vz = THROW_VELOCITY * (cosYaw * cosPitch) + std::sin(angle) * inaccuracy;

    // 生成物品实体
    ItemEntity* itemEntity = ItemDropHelper::spawnItemEntity(
        world, stack, m_dolphin->x(), y, m_dolphin->z(), vx, vy, vz, PICKUP_DELAY, m_dolphin->uuid());

    MC_UNUSED(itemEntity);
}

ItemEntity* PlayWithItemsGoal::_findNearbyItem() const
{
    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return nullptr;
    }

    // 获取附近所有实体
    std::vector<Entity*> entities = world->getEntitiesInRange(m_dolphin->position(), SEARCH_RADIUS, m_dolphin);

    ItemEntity* closestItem = nullptr;
    f64 closestDist = SEARCH_RADIUS * SEARCH_RADIUS;

    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }

        // 检查是否是物品实体
        if (entity->entityType() != entity::VanillaEntityTypeKeys::ITEM) {
            continue;
        }

        ItemEntity* item = static_cast<ItemEntity*>(entity);

        // 物品必须在水中且可以被拾取
        if (!item->isInWater()) {
            continue;
        }

        if (!item->canBePickedUp()) {
            continue;
        }

        f32 distSq = m_dolphin->distanceSqTo(*item);
        if (static_cast<f64>(distSq) < closestDist) {
            closestDist = static_cast<f64>(distSq);
            closestItem = item;
        }
    }

    return closestItem;
}

// ============================================================================
// FollowBoatGoal
// ============================================================================

FollowBoatGoal::FollowBoatGoal(DolphinEntity* dolphin)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_dolphin(dolphin)
{
    MC_ASSERT_RELEASE(dolphin != nullptr);
}

bool FollowBoatGoal::shouldExecute()
{
    // 检查是否有正在驾驶的玩家
    // 条件1: 已经有跟踪的玩家且玩家正在操作船移动
    // 条件2: 5格范围内有玩家驾驶的船
    if (m_player != nullptr && _isPlayerOperatingBoat(*m_player)) {
        return true;
    }

    // 查找附近正在驾驶船的玩家
    m_player = _findPlayerDrivingBoat();
    return m_player != nullptr;
}

bool FollowBoatGoal::shouldContinueExecuting()
{
    // 玩家必须在船上且正在操作移动
    if (m_player == nullptr) {
        return false;
    }

    // 检查玩家是否仍在骑乘（船上）
    if (!m_player->isRiding()) {
        return false;
    }

    // 检查玩家是否正在操作移动
    return _isPlayerOperatingBoat(*m_player);
}

void FollowBoatGoal::startExecuting()
{
    // 在5格范围内寻找有玩家驾驶的船
    m_player = _findPlayerDrivingBoat();
    m_navigationTimer = 0;
    m_state = BoatFollowState::GoToBoat;
}

void FollowBoatGoal::resetTask()
{
    m_player = nullptr;
    m_dolphin->clearNavigationPath();
}

void FollowBoatGoal::tick()
{
    if (m_player == nullptr) {
        return;
    }

    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return;
    }

    // 检查玩家是否正在操作移动
    bool isOperating = _isPlayerOperatingBoat(*m_player);

    // 根据状态设置移动速度
    // GoInBoatDirection 状态下，玩家不操作时不移动
    f32 speed = (m_state == BoatFollowState::GoInBoatDirection) ? (isOperating ? GO_IN_DIRECTION_SPEED : 0.0f)
                                                                : GO_TO_BOAT_SPEED;

    // 每10tick更新一次导航目标
    if (--m_navigationTimer <= 0) {
        m_navigationTimer = NAVIGATION_UPDATE_INTERVAL;

        if (m_state == BoatFollowState::GoToBoat) {
            // 阶段1：游向船
            // 计算目标位置 = 玩家位置 - 玩家朝向方向 + 下移一格
            f32 playerYaw = m_player->yaw();
            f32 yawRad = playerYaw * math::DEG_TO_RAD;

            // 玩家朝向的反方向
            i32 dx = static_cast<i32>(-std::round(std::sin(yawRad)));
            i32 dz = static_cast<i32>(std::round(std::cos(yawRad)));

            BlockPos playerPos(static_cast<i32>(std::floor(m_player->x())),
                static_cast<i32>(std::floor(m_player->y())),
                static_cast<i32>(std::floor(m_player->z())));

            // 目标位置：船尾后方，下移一格
            BlockPos targetPos(playerPos.x + dx, playerPos.y - 1, playerPos.z + dz);

            // 尝试移动到该位置
            m_dolphin->tryMoveTo(static_cast<f64>(targetPos.x) + 0.5,
                static_cast<f64>(targetPos.y),
                static_cast<f64>(targetPos.z) + 0.5,
                NAVIGATE_SPEED);

            // 当距离小于4格时，切换到跟随状态
            f32 dist = m_dolphin->distanceTo(*m_player);
            if (dist < SWITCH_TO_FOLLOW_DISTANCE) {
                m_navigationTimer = 0; // 立即更新
                m_state = BoatFollowState::GoInBoatDirection;
            }
        } else if (m_state == BoatFollowState::GoInBoatDirection) {
            // 阶段2：跟随船的行进方向
            // 获取玩家朝向，计算前方10格的位置
            f32 playerYaw = m_player->yaw();
            f32 yawRad = playerYaw * math::DEG_TO_RAD;

            // 玩家朝向方向
            i32 dx = static_cast<i32>(std::round(std::sin(yawRad)));
            i32 dz = static_cast<i32>(-std::round(std::cos(yawRad)));

            BlockPos playerPos(static_cast<i32>(std::floor(m_player->x())),
                static_cast<i32>(std::floor(m_player->y())),
                static_cast<i32>(std::floor(m_player->z())));

            // 目标位置：船前方10格，下移一格
            BlockPos targetPos(playerPos.x + dx * 10, playerPos.y - 1, playerPos.z + dz * 10);

            // 尝试移动到该位置
            m_dolphin->tryMoveTo(static_cast<f64>(targetPos.x) + 0.5,
                static_cast<f64>(targetPos.y),
                static_cast<f64>(targetPos.z) + 0.5,
                NAVIGATE_SPEED);

            // 当距离超过12格时，切回游向船状态
            f32 dist = m_dolphin->distanceTo(*m_player);
            if (dist > SWITCH_TO_APPROACH_DISTANCE) {
                m_navigationTimer = 0; // 立即更新
                m_state = BoatFollowState::GoToBoat;
            }
        }
    }

    // 看向玩家
    m_dolphin->lookAt(static_cast<f64>(m_player->x()),
        static_cast<f64>(m_player->y() + m_player->eyeHeight()),
        static_cast<f64>(m_player->z()));
}

Player* FollowBoatGoal::_findPlayerDrivingBoat()
{
    IWorld* world = m_dolphin->world();
    if (world == nullptr) {
        return nullptr;
    }

    // 获取5格范围内的所有实体
    std::vector<Entity*> entities = world->getEntitiesInRange(m_dolphin->position(), SEARCH_RADIUS, m_dolphin);

    for (Entity* entity : entities) {
        if (entity == nullptr) {
            continue;
        }

        // 检查是否是船
        if (entity->entityType() != entity::VanillaEntityTypeKeys::BOAT) {
            continue;
        }

        // 获取船的控制乘客
        EntityInstanceId controllerId = entity->getControllingPassenger();
        if (controllerId == 0) {
            continue;
        }

        Entity* controller = world->getEntity(controllerId);
        if (controller == nullptr) {
            continue;
        }

        // 检查控制者是否是玩家
        Player* player = dynamic_cast<Player*>(controller);
        if (player == nullptr) {
            continue;
        }

        // 检查玩家是否正在操作船移动
        if (_isPlayerOperatingBoat(*player)) {
            return player;
        }
    }

    return nullptr;
}

bool FollowBoatGoal::_isPlayerOperatingBoat(const Player& player)
{
    // 检查玩家的 moveStrafing 或 moveForward 是否非零
    f32 strafe = std::abs(player.moveStrafing());
    f32 forward = std::abs(player.moveForward());
    return strafe > 0.0f || forward > 0.0f;
}

} // namespace entity::ai::goal
} // namespace mc
