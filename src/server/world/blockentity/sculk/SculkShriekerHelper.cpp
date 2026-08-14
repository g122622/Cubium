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

#include "SculkShriekerHelper.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/sculk/SculkBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "server/world/ServerWorld.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc::server {

// ============================================================================
// tryShriek - 尝试激活幽匿尖啸体
// ============================================================================

void SculkShriekerHelper::tryShriek(ServerWorld& world, const BlockPos& pos, const Entity* sourceEntity)
{
    // 1. 尝试将触发实体解析为玩家
    Player* player = tryGetPlayer(world, sourceEntity);
    if (player == nullptr) {
        // 没有玩家触发的振动不激活尖啸体
        return;
    }

    // 2. 检查尖啸体当前是否正在 SHRIEKING
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->get(BlockStateProperties::SHRIEKING())) {
        // 已经在 SHRIEKING 状态，不重复激活
        return;
    }

    // 3. 检查玩家是否为旁观者模式
    if (player->isSpectator()) {
        return;
    }

    // 4. 重置警告等级为 0
    //    tryShriek 每次调用都重置 warningLevel，然后通过 _tryWarn 重新递增
    BlockEntity* be = world.getBlockEntity(pos);
    if (be == nullptr || be->getType() != BlockEntityType::SculkShrieker) {
        return;
    }
    auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);
    shrieker->setWarningLevel(0);

    // 5. 如果可以响应（CAN_SUMMON + 非和平 + 游戏规则），尝试递增警告等级
    bool warningIncreased = false;
    if (_canRespond(world, pos)) {
        warningIncreased = _tryWarn(world, pos);
    }

    // 6. 执行尖啸效果（无论是否成功递增警告等级都会执行）
    if (!_canRespond(world, pos) || warningIncreased) {
        blocks::SculkShriekerBlock::shriek(world, pos, *state, sourceEntity);
    }
}

// ============================================================================
// tryRespond - 尖啸结束后的响应逻辑
// ============================================================================

void SculkShriekerHelper::tryRespond(ServerWorld& world, const BlockPos& pos)
{
    BlockEntity* be = world.getBlockEntity(pos);
    if (be == nullptr || be->getType() != BlockEntityType::SculkShrieker) {
        return;
    }
    auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);

    if (!_canRespond(world, pos)) {
        return;
    }

    i32 warningLevel = shrieker->getWarningLevel();
    if (warningLevel <= 0) {
        return;
    }

    // 尝试召唤监守者（仅在警告等级达到4级时）
    bool wardenSummoned = _trySummonWarden(world, pos);

    // 如果未召唤监守者，播放警告等级对应的声音
    if (!wardenSummoned) {
        _playWardenReplySound(world, pos, warningLevel);
    }

    // 对附近玩家应用黑暗效果
    _applyDarknessAround(world, pos);
}

// ============================================================================
// checkShriekingFinished - 检查尖啸结束标志
// ============================================================================

void SculkShriekerHelper::checkShriekingFinished(ServerWorld& world, const BlockPos& pos)
{
    BlockEntity* be = world.getBlockEntity(pos);
    if (be == nullptr || be->getType() != BlockEntityType::SculkShrieker) {
        return;
    }
    auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);

    if (shrieker->isShriekingFinished()) {
        // 清除标志
        shrieker->setShriekingFinished(false);
        shrieker->setChanged();

        // 执行响应逻辑
        tryRespond(world, pos);
    }
}

// ============================================================================
// _canRespond - 检查是否可以响应（召唤监守者的前置条件）
// ============================================================================

bool SculkShriekerHelper::_canRespond(ServerWorld& world, const BlockPos& pos)
{
    // 条件1: CAN_SUMMON 方块状态属性为 true（自然生成的尖啸体）
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || !state->get(BlockStateProperties::CAN_SUMMON())) {
        return false;
    }

    // 条件2: 非和平难度
    if (world.difficulty() == Difficulty::Peaceful) {
        return false;
    }

    // 条件3: 游戏规则允许监守者生成
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_WARDEN_SPAWNING)) {
        return false;
    }

    return true;
}

// ============================================================================
// _trySummonWarden - 尝试召唤监守者
// ============================================================================

bool SculkShriekerHelper::_trySummonWarden(ServerWorld& world, const BlockPos& pos)
{
    // 仅在警告等级 >= 4 时尝试召唤

    BlockEntity* be = world.getBlockEntity(pos);
    if (be == nullptr || be->getType() != BlockEntityType::SculkShrieker) {
        return false;
    }
    auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);

    if (!shrieker->canSummonWarden()) {
        return false;
    }

    // 检查附近是否已有监守者
    if (_hasNearbyWarden(world, pos)) {
        return false;
    }

    // 查找监守者实体类型
    const entity::EntityType* wardenType = entity::EntityRegistry::instance().getType("minecraft:warden");
    if (wardenType == nullptr || !wardenType->canSummon()) {
        // 监守者实体类型应已注册（WardenEntity）。
        // 此处保留防御性回退，仅在实体注册表被异常清空或注册失败时触发。
        return false;
    }

    // 在尖啸体附近尝试找到有效生成位置
    math::Random& rng = world.getRandom();
    for (i32 attempt = 0; attempt < SUMMON_ATTEMPTS; ++attempt) {
        // 随机偏移位置：水平 +/-5
        i32 dx = rng.nextInt(SUMMON_HORIZONTAL_RANGE * 2 + 1) - SUMMON_HORIZONTAL_RANGE;
        i32 dz = rng.nextInt(SUMMON_HORIZONTAL_RANGE * 2 + 1) - SUMMON_HORIZONTAL_RANGE;

        // 从 pos.y + SUMMON_VERTICAL_RANGE 向下搜索有效生成位置
        for (i32 y = pos.y + SUMMON_VERTICAL_RANGE; y >= pos.y - SUMMON_VERTICAL_RANGE; --y) {
            BlockPos checkPos(pos.x + dx, y, pos.z + dz);
            const BlockState* belowState = world.getBlockState(checkPos.down());
            const BlockState* atState = world.getBlockState(checkPos);
            const BlockState* aboveState = world.getBlockState(checkPos.up());

            if (belowState == nullptr || atState == nullptr || aboveState == nullptr) {
                continue;
            }

            // 检查下方方块有完整的上表面碰撞
            if (!belowState->isFaceFull(Direction::Up)) {
                continue;
            }

            // 检查生成位置和上方位置没有方块阻挡
            if (!atState->isAir() || !aboveState->isAir()) {
                continue;
            }

            // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
            auto* registry = world.entityRegistry();
            if (registry == nullptr) {
                return false;
            }

            // 有效生成位置，创建监守者实体
            auto wardenEntity = wardenType->create(&world, *registry);
            if (wardenEntity == nullptr) {
                return false;
            }

            // 设置位置和旋转
            Vector3 spawnWorldPos(
                static_cast<f32>(checkPos.x) + 0.5f, static_cast<f32>(checkPos.y), static_cast<f32>(checkPos.z) + 0.5f);
            wardenEntity->setPosition(spawnWorldPos);
            wardenEntity->setRotation(rng.nextFloat() * 360.0f, 0.0f);

            // 跳过生成规则检查，因为监守者是特殊召唤而非自然生成

            // 生成到世界中
            EntityInstanceId id = world.spawnEntity(std::move(wardenEntity));
            if (id != 0) {
                // 召唤成功
                // 注意：MC 原版中召唤监守者后不重置方块实体的 warningLevel，
                // warningLevel 通过 WardenSpawnTracker 的自然衰减机制逐渐降低
                shrieker->setChanged();
                return true;
            }

            return false;
        }
    }

    // 所有尝试都失败
    return false;
}

// ============================================================================
// _playWardenReplySound - 播放监守者回应声音
// ============================================================================

void SculkShriekerHelper::_playWardenReplySound(ServerWorld& world, const BlockPos& pos, i32 warningLevel)
{
    if (warningLevel < 1 || warningLevel > 4) {
        return;
    }

    // 根据警告等级选择声音
    ResourceLocation soundEvent(WARDEN_SOUND_BY_LEVEL[warningLevel]);

    // 在尖啸体附近随机偏移位置播放声音（偏移 +/-10 格）
    math::Random& rng = world.getRandom();
    f32 soundX = static_cast<f32>(pos.x + rng.nextInt(21) - 10) + 0.5f;
    f32 soundY = static_cast<f32>(pos.y + rng.nextInt(21) - 10);
    f32 soundZ = static_cast<f32>(pos.z + rng.nextInt(21) - 10) + 0.5f;

    world.playSound(soundEvent, sound::SoundCategory::Hostile, Vector3(soundX, soundY, soundZ), 5.0f, 1.0f);
}

// ============================================================================
// _hasNearbyWarden - 检查附近是否有监守者
// ============================================================================

bool SculkShriekerHelper::_hasNearbyWarden(ServerWorld& world, const BlockPos& pos)
{
    // 在 48x48x48 范围内搜索是否存在监守者实体
    Vector3 center = pos.center();
    AxisAlignedBB searchBox(center.x - WARDEN_SEARCH_RADIUS,
        center.y - WARDEN_SEARCH_RADIUS,
        center.z - WARDEN_SEARCH_RADIUS,
        center.x + WARDEN_SEARCH_RADIUS,
        center.y + WARDEN_SEARCH_RADIUS,
        center.z + WARDEN_SEARCH_RADIUS);

    // 查找监守者实体类型ID，用于高效比较
    const entity::EntityType* wardenType = entity::EntityRegistry::instance().getType("minecraft:warden");
    std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox);
    for (Entity* entity : entities) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }
        // 优先使用 EntityType 指针比较（高效），回退到字符串类型ID比较
        if (wardenType != nullptr) {
            if (entity->entityType() == wardenType) {
                return true;
            }
        } else {
            // 监守者实体类型尚未注册，使用字符串匹配作为回退
            if (entity->getTypeId() == "minecraft:warden") {
                return true;
            }
        }
    }

    return false;
}

// ============================================================================
// _applyDarknessAround - 对附近玩家应用黑暗效果
// ============================================================================

void SculkShriekerHelper::_applyDarknessAround(ServerWorld& world, const BlockPos& pos)
{
    // 对半径 DARKNESS_RADIUS (40格) 内的玩家应用黑暗效果
    Vector3 center = pos.center();

    std::vector<Entity*> players = world.getPlayers();
    for (Entity* entity : players) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        Player* player = dynamic_cast<Player*>(entity);
        if (player == nullptr) {
            continue;
        }

        // 旁观者模式不受影响
        if (player->isSpectator()) {
            continue;
        }

        // 检查距离
        f32 distSq = player->position().distanceSquared(center);
        f32 radiusSq = DARKNESS_RADIUS * DARKNESS_RADIUS;
        if (distSq > radiusSq) {
            continue;
        }

        // 应用黑暗效果：260 tick 持续，amplifier 0，非环境效果，不显示粒子
        entity::effect::EffectInstance darknessEffect(entity::effect::EffectType::Darkness,
            DARKNESS_DURATION, // duration: 260 ticks = 13 seconds
            0,                 // amplifier: 0
            false,             // ambient: false
            false              // visible: false
        );
        player->addEffect(std::move(darknessEffect));
    }
}

// ============================================================================
// _tryWarn - 尝试递增附近玩家的警告等级
// ============================================================================

bool SculkShriekerHelper::_tryWarn(ServerWorld& world, const BlockPos& pos)
{
    // 检查附近是否已有监守者
    if (_hasNearbyWarden(world, pos)) {
        return false;
    }

    // 查找附近 16 格内的玩家
    Vector3 center = pos.center();
    std::vector<Entity*> allPlayers = world.getPlayers();

    // 找出范围内的玩家（排除旁观者）
    std::vector<Player*> nearbyPlayers;
    for (Entity* entity : allPlayers) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }
        Player* player = dynamic_cast<Player*>(entity);
        if (player == nullptr || player->isSpectator()) {
            continue;
        }
        f32 distSq = player->position().distanceSquared(center);
        f32 radiusSq = PLAYER_SEARCH_RADIUS * PLAYER_SEARCH_RADIUS;
        if (distSq <= radiusSq) {
            nearbyPlayers.push_back(player);
        }
    }

    if (nearbyPlayers.empty()) {
        return false;
    }

    // 任何一个附近玩家在冷却中，则不递增
    for (Player* player : nearbyPlayers) {
        if (player->wardenWarningEffect().onCooldown()) {
            return false;
        }
    }

    // 找到最高警告等级的玩家追踪器进行递增
    Player* highestPlayer = nullptr;
    i32 highestLevel = -1;
    for (Player* player : nearbyPlayers) {
        i32 level = player->wardenWarningEffect().getWarningLevel();
        if (level > highestLevel) {
            highestLevel = level;
            highestPlayer = player;
        }
    }

    if (highestPlayer == nullptr) {
        return false;
    }

    // 递增最高等级追踪器的警告等级
    // 设置触发源位置，然后递增
    highestPlayer->wardenWarningEffect().setSourcePos(pos);
    highestPlayer->wardenWarningEffect().increaseWarning();

    // 将递增后的数据同步到附近所有玩家的追踪器
    for (Player* player : nearbyPlayers) {
        if (player != highestPlayer) {
            player->wardenWarningEffect().copyData(highestPlayer->wardenWarningEffect());
        }
    }

    // 将警告等级同步到方块实体
    BlockEntity* be = world.getBlockEntity(pos);
    if (be == nullptr || be->getType() != BlockEntityType::SculkShrieker) {
        return false;
    }
    auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);
    shrieker->setWarningLevel(highestPlayer->wardenWarningEffect().getWarningLevel());
    shrieker->setChanged();

    return highestPlayer->wardenWarningEffect().getWarningLevel() > 0;
}

// ============================================================================
// tryGetPlayer - 将触发实体解析为玩家
// ============================================================================

Player* SculkShriekerHelper::tryGetPlayer(IWorld& world, const Entity* entity)
{
    // 按优先级链式判断：直接玩家 -> 载具控制者 -> 投射物所有者 -> 物品所有者
    if (entity == nullptr) {
        return nullptr;
    }

    // 1. 直接是玩家
    const Player* player = dynamic_cast<const Player*>(entity);
    if (player != nullptr) {
        return const_cast<Player*>(player);
    }

    // 2. 载具上的玩家：检查实体的控制乘客是否为玩家
    EntityInstanceId controllingPassengerId = entity->getControllingPassenger();
    if (controllingPassengerId != INVALID_ENTITY_ID) {
        Entity* passenger = world.getEntity(controllingPassengerId);
        if (passenger != nullptr) {
            Player* passengerPlayer = dynamic_cast<Player*>(passenger);
            if (passengerPlayer != nullptr) {
                return passengerPlayer;
            }
        }
    }

    // 3. 投射物的主人（如投掷的雪球、末影珍珠等）
    const entity::ProjectileEntity* projectile = dynamic_cast<const entity::ProjectileEntity*>(entity);
    if (projectile != nullptr) {
        Entity* shooter = projectile->getShooter();
        if (shooter != nullptr) {
            Player* shooterPlayer = dynamic_cast<Player*>(shooter);
            if (shooterPlayer != nullptr) {
                return shooterPlayer;
            }
        }
    }

    // 4. 掉落物品的主人
    // ItemEntity 没有直接的 getOwner() 方法返回 Entity*，但存储了 ownerUuid
    // 需要通过 UUID 在世界中查找对应实体
    const ItemEntity* itemEntity = dynamic_cast<const ItemEntity*>(entity);
    if (itemEntity != nullptr) {
        const std::string& ownerUuid = itemEntity->ownerUuid();
        if (!ownerUuid.empty()) {
            Entity* owner = world.getEntityByUuid(ownerUuid);
            if (owner != nullptr) {
                Player* ownerPlayer = dynamic_cast<Player*>(owner);
                if (ownerPlayer != nullptr) {
                    return ownerPlayer;
                }
            }
        }
    }

    return nullptr;
}

} // namespace mc::server
