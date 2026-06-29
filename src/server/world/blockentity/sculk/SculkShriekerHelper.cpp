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

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/sculk/SculkBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc::server {

// ============================================================================
// tryShriek - 尝试激活幽匿尖啸体
// ============================================================================

void SculkShriekerHelper::tryShriek(ServerWorld& world, const BlockPos& pos, const Entity* sourceEntity)
{
    // 对齐 MC Java: SculkShriekerBlockEntity.tryShriek()

    // 1. 尝试将触发实体解析为玩家
    Player* player = tryGetPlayer(sourceEntity);
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

    // 4. 重置警告等级为 0（对齐 MC Java: this.warningLevel = 0）
    //    MC Java 的 tryShriek 每次调用都重置 warningLevel，
    //    然后通过 tryWarn 重新递增
    BlockEntity* be = world.getBlockEntity(pos);
    if (be == nullptr || be->getType() != BlockEntityType::SculkShrieker) {
        return;
    }
    auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);
    shrieker->setWarningLevel(0);

    // 5. 如果可以响应（CAN_SUMMON + 非和平 + 游戏规则），尝试递增警告等级
    bool warningIncreased = false;
    if (canRespond(world, pos)) {
        warningIncreased = tryWarn(world, pos);
    }

    // 6. 执行尖啸效果（无论是否成功递增警告等级都会执行）
    //    对齐 MC Java: if (!canRespond || tryWarn) { shriek(); }
    if (!canRespond(world, pos) || warningIncreased) {
        blocks::SculkShriekerBlock::shriek(world, pos, *state, sourceEntity);
    }
}

// ============================================================================
// tryRespond - 尖啸结束后的响应逻辑
// ============================================================================

void SculkShriekerHelper::tryRespond(ServerWorld& world, const BlockPos& pos)
{
    // 对齐 MC Java: SculkShriekerBlockEntity.tryRespond()

    BlockEntity* be = world.getBlockEntity(pos);
    if (be == nullptr || be->getType() != BlockEntityType::SculkShrieker) {
        return;
    }
    auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);

    if (!canRespond(world, pos)) {
        return;
    }

    i32 warningLevel = shrieker->getWarningLevel();
    if (warningLevel <= 0) {
        return;
    }

    // 尝试召唤监守者（仅在警告等级达到4级时）
    bool wardenSummoned = trySummonWarden(world, pos);

    // 如果未召唤监守者，播放警告等级对应的声音
    if (!wardenSummoned) {
        playWardenReplySound(world, pos, warningLevel);
    }

    // 对附近玩家应用黑暗效果
    applyDarknessAround(world, pos);
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
// canRespond - 检查是否可以响应（召唤监守者的前置条件）
// ============================================================================

bool SculkShriekerHelper::canRespond(ServerWorld& world, const BlockPos& pos)
{
    // 对齐 MC Java: SculkShriekerBlockEntity.canRespond()
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
// trySummonWarden - 尝试召唤监守者
// ============================================================================

bool SculkShriekerHelper::trySummonWarden(ServerWorld& world, const BlockPos& pos)
{
    // 对齐 MC Java: SculkShriekerBlockEntity.trySummonWarden()
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
    if (hasNearbyWarden(world, pos)) {
        return false;
    }

    // 查找监守者实体类型
    const entity::EntityType* wardenType = entity::EntityRegistry::instance().getType("minecraft:warden");
    if (wardenType == nullptr || !wardenType->canSummon()) {
        // TODO: 监守者实体类型尚未注册，当前无法召唤监守者
        // 当 WardenEntity 实现后，此分支将不再触发
        return false;
    }

    // 对齐 MC Java: SpawnUtil.trySpawnMob()
    // 在尖啸体附近尝试找到有效生成位置
    math::Random& rng = world.getRandom();
    for (i32 attempt = 0; attempt < SUMMON_ATTEMPTS; ++attempt) {
        // 随机偏移位置：水平 +/-5
        i32 dx = rng.nextInt(SUMMON_HORIZONTAL_RANGE * 2 + 1) - SUMMON_HORIZONTAL_RANGE;
        i32 dz = rng.nextInt(SUMMON_HORIZONTAL_RANGE * 2 + 1) - SUMMON_HORIZONTAL_RANGE;

        // 对齐 MC Java: SpawnUtil.Strategy.ON_TOP_OF_COLLIDER
        // 从 pos.y + SUMMON_VERTICAL_RANGE 向下搜索有效生成位置
        for (i32 y = pos.y + SUMMON_VERTICAL_RANGE; y >= pos.y - SUMMON_VERTICAL_RANGE; --y) {
            BlockPos checkPos(pos.x + dx, y, pos.z + dz);
            const BlockState* belowState = world.getBlockState(checkPos.down());
            const BlockState* atState = world.getBlockState(checkPos);
            const BlockState* aboveState = world.getBlockState(checkPos.up());

            if (belowState == nullptr || atState == nullptr || aboveState == nullptr) {
                continue;
            }

            // 检查下方方块有完整的上表面碰撞（对齐 MC Java: SpawnUtil.Strategy.ON_TOP_OF_COLLIDER）
            if (!belowState->isFaceFull(Direction::Up)) {
                continue;
            }

            // 检查生成位置和上方位置没有方块阻挡
            if (!atState->isAir() || !aboveState->isAir()) {
                continue;
            }

            // 有效生成位置，创建监守者实体
            auto wardenEntity = wardenType->create(&world);
            if (wardenEntity == nullptr) {
                return false;
            }

            // 设置位置和旋转
            Vector3 spawnWorldPos(
                static_cast<f32>(checkPos.x) + 0.5f, static_cast<f32>(checkPos.y), static_cast<f32>(checkPos.z) + 0.5f);
            wardenEntity->setPosition(spawnWorldPos);
            wardenEntity->setRotation(rng.nextFloat() * 360.0f, 0.0f);

            // 对齐 MC Java: mob.checkSpawnRules() && mob.checkSpawnObstruction()
            // 跳过生成规则检查，因为监守者是特殊召唤而非自然生成

            // 生成到世界中
            EntityId id = world.spawnEntity(std::move(wardenEntity));
            if (id != 0) {
                // 召唤成功，重置警告等级
                shrieker->setWarningLevel(0);
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
// playWardenReplySound - 播放监守者回应声音
// ============================================================================

void SculkShriekerHelper::playWardenReplySound(ServerWorld& world, const BlockPos& pos, i32 warningLevel)
{
    // 对齐 MC Java: SculkShriekerBlockEntity.playWardenReplySound()
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

    // 对齐 MC Java: level.playSound(null, x, y, z, soundEvent, SoundSource.HOSTILE, 5.0F, 1.0F)
    world.playSound(soundEvent, sound::SoundCategory::Hostile, Vector3(soundX, soundY, soundZ), 5.0f, 1.0f);
}

// ============================================================================
// hasNearbyWarden - 检查附近是否有监守者
// ============================================================================

bool SculkShriekerHelper::hasNearbyWarden(ServerWorld& world, const BlockPos& pos)
{
    // 对齐 MC Java: WardenSpawnTracker.hasNearbyWarden()
    // 在 48x48x48 范围内搜索是否存在监守者实体
    Vector3 center = pos.center();
    AxisAlignedBB searchBox(center.x - WARDEN_SEARCH_RADIUS,
        center.y - WARDEN_SEARCH_RADIUS,
        center.z - WARDEN_SEARCH_RADIUS,
        center.x + WARDEN_SEARCH_RADIUS,
        center.y + WARDEN_SEARCH_RADIUS,
        center.z + WARDEN_SEARCH_RADIUS);

    // 检查是否有 warden 类型的实体在范围内
    // 对齐 MC Java: level.getEntitiesOfClass(Warden.class, aabb)
    // TODO: 当 WardenEntity 实现后，使用 getEntitiesByType 查询监守者类型
    // 当前使用 getEntitiesInAABB + 名称匹配作为简化实现
    std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox);
    for (Entity* entity : entities) {
        if (entity != nullptr && !entity->isRemoved()) {
            // 检查实体类型名称是否为监守者
            if (entity->getTypeId() == "minecraft:warden") {
                return true;
            }
        }
    }

    return false;
}

// ============================================================================
// applyDarknessAround - 对附近玩家应用黑暗效果
// ============================================================================

void SculkShriekerHelper::applyDarknessAround(ServerWorld& world, const BlockPos& pos)
{
    // 对齐 MC Java: Warden.applyDarknessAround()
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
        // 对齐 MC Java: MobEffectInstance(MobEffects.DARKNESS, 260, 0, false, false)
        // 以及 MobEffectUtil.addEffectToPlayersAround 的 200 tick 冷却
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
// tryWarn - 尝试递增附近玩家的警告等级
// ============================================================================

bool SculkShriekerHelper::tryWarn(ServerWorld& world, const BlockPos& pos)
{
    // 对齐 MC Java: WardenSpawnTracker.tryWarn()

    // 检查附近是否已有监守者
    if (hasNearbyWarden(world, pos)) {
        return false;
    }

    // 查找附近 16 格内的玩家
    Vector3 center = pos.center();
    std::vector<Entity*> allPlayers = world.getPlayers();

    // 找出范围内的玩家
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

    // 对齐 MC Java: 如果任何附近玩家的 WardenSpawnTracker 在冷却中，则不递增
    // 当前项目的 WardenWarningEffect 由方块实体管理，
    // 冷却逻辑通过方块实体的警告等级递增间隔实现。
    // 为简化实现，直接递增方块实体的警告等级。

    // 递增警告等级
    BlockEntity* be = world.getBlockEntity(pos);
    if (be == nullptr || be->getType() != BlockEntityType::SculkShrieker) {
        return false;
    }
    auto* shrieker = static_cast<blockentity::SculkShriekerBlockEntity*>(be);

    i32 newLevel = shrieker->incrementWarningLevel();
    shrieker->setChanged();

    // 对齐 MC Java: 同步附近玩家的 WardenSpawnTracker
    // 将警告效果应用到附近玩家（通过 WardenWarningEffect）
    for (Player* player : nearbyPlayers) {
        player->increaseWardenWarning(pos);
    }

    return newLevel > 0;
}

// ============================================================================
// tryGetPlayer - 将触发实体解析为玩家
// ============================================================================

Player* SculkShriekerHelper::tryGetPlayer(const Entity* entity)
{
    // 对齐 MC Java: SculkShriekerBlockEntity.tryGetPlayer()
    if (entity == nullptr) {
        return nullptr;
    }

    // 1. 直接是玩家
    const Player* player = dynamic_cast<const Player*>(entity);
    if (player != nullptr) {
        return const_cast<Player*>(player);
    }

    // 2. 载具上的玩家
    // TODO: 当载具系统完善后，检查 entity.getControllingPassenger()
    // 当前跳过此检查

    // 3. 投射物的主人（如投掷的雪球、末影珍珠等）
    // TODO: 当投射物所有者追踪完善后，检查 projectile.getOwner()
    // 当前跳过此检查

    // 4. 掉落物品的主人
    // TODO: 当物品实体所有者追踪完善后，检查 itemEntity.getOwner()
    // 当前跳过此检查

    return nullptr;
}

} // namespace mc::server
