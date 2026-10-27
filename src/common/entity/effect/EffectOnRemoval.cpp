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

#include "EffectOnRemoval.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/RemovalReason.hpp"
#include "common/entity/entities/monster/basic/SlimeEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionContext.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace mc {
namespace entity {
namespace effect {
namespace onremoval {

namespace {

/// 随机整数闭区间 [min, max]（对齐 vanilla Mth.randomBetweenInclusive）
i32 _randomBetweenInclusive(math::IRandom& random, i32 min, i32 max)
{
    return min + random.nextInt(max - min + 1);
}

/// 织网：在实体周围随机生成蜘蛛网（对齐 vanilla WeavingMobEffect.spawnCobwebsRandomlyAround）
void _spawnCobwebsRandomlyAround(IWorld& world, LivingEntity& entity, i32 maxCobwebs)
{
    const BlockPos origin = entity.onPos();
    std::set<BlockPos> chosen;

    // vanilla BlockPos.randomInCube(random, 15, pos, 1)：立方体内随机抽 15 个候选（可重复）。
    for (i32 i = 0; i < 15; ++i) {
        const BlockPos candidate(origin.x + world.getRandom().nextInt(-1, 2),
            origin.y + world.getRandom().nextInt(-1, 2),
            origin.z + world.getRandom().nextInt(-1, 2));
        if (chosen.contains(candidate)) {
            continue;
        }
        const BlockPos below = candidate.down();
        const BlockState* state = world.getBlockState(candidate);
        const BlockState* belowState = world.getBlockState(below);
        if (state == nullptr || belowState == nullptr) {
            continue;
        }
        if (!state->canBeReplaced()) {
            continue;
        }
        // vanilla isFaceSturdy(level, below, UP)：下方方块顶面为完整支撑面
        if (!belowState->isFaceSturdy(world, below, Direction::Up, SupportType::Full)) {
            continue;
        }
        chosen.insert(candidate);
        if (static_cast<i32>(chosen.size()) >= maxCobwebs) {
            break;
        }
    }

    for (const BlockPos& pos : chosen) {
        world.setBlockState(pos, &VanillaBlocks::COBWEB->defaultState(), 3);
        world.playEvent(3018, pos, 0);
    }
}

/// 寄生：在实体位置生成蠹虫（对齐 vanilla InfestedMobEffect.spawnSilverfish）
void _spawnSilverfish(IWorld& world, LivingEntity& entity)
{
    const entity::EntityType* type = EntityRegistry::instance().getType(VanillaEntityTypeKeys::SILVERFISH->name());
    ecs::EntityRegistry* ecsRegistry = world.entityRegistry();
    if (type == nullptr || ecsRegistry == nullptr) {
        return;
    }
    auto silverfish = type->create(&world, *ecsRegistry);
    if (silverfish == nullptr) {
        return;
    }

    // vanilla：实体视线方向压缩（y 放大 1.5）后绕 Y 轴随机偏转 [-90°, 90°]，再归一化到 0.3 倍
    // 视线向量与 LivingEntity::getLookAngle 同算法（该方法在 protected 段，此处按 yaw/pitch 复算）
    const f32 yawRad = math::toRadians(entity.yaw());
    const f32 pitchRad = math::toRadians(entity.pitch());
    const Vector3 look(
        -std::sin(yawRad) * std::cos(pitchRad), -std::sin(pitchRad), std::cos(yawRad) * std::cos(pitchRad));
    Vector3 direction(look.x * 0.3f, look.y * 0.3f * 1.5f, look.z * 0.3f);
    const f32 angle = world.getRandom().nextFloat(-static_cast<f32>(math::PI / 2.0), static_cast<f32>(math::PI / 2.0));
    const f32 cosA = std::cos(angle);
    const f32 sinA = std::sin(angle);
    const Vector3 rotated(
        direction.x * cosA - direction.z * sinA, direction.y, direction.x * sinA + direction.z * cosA);

    silverfish->setPosition(entity.x(), entity.y() + entity.height() / 2.0f, entity.z());
    silverfish->setRotation(world.getRandom().nextFloat() * 360.0f, 0.0f);
    silverfish->setVelocity(rotated);
    world.spawnEntity(std::move(silverfish));
}

/// 渗浆：生成小史莱姆（对齐 vanilla OozingMobEffect.onMobRemoved + spawnSlimeOffspring）
void _spawnOozingSlimes(IWorld& world, LivingEntity& entity, i32 spawnedCount)
{
    const i32 maxCramming = world.getGameRules().getInt(world::gamerule::GameRuleKeys::MAX_ENTITY_CRAMMING);
    // vanilla NearbySlimes.closeTo：统计 2 格膨胀盒内的其他史莱姆数量
    i32 nearby = 0;
    for (Entity* other : world.getEntitiesInRange(entity.position(), 2.0f, &entity)) {
        if (other == nullptr || other == &entity) {
            continue;
        }
        if (other->getTypeId() == VanillaEntityTypeKeys::SLIME->name()) {
            ++nearby;
        }
    }

    // vanilla numberOfSlimesToSpawn：min(max(0, cramming - nearby), spawnedCount)
    i32 count = 0;
    if (maxCramming >= 1) {
        count = std::clamp(maxCramming - nearby, 0, spawnedCount);
    } else {
        count = spawnedCount;
    }

    for (i32 i = 0; i < count; ++i) {
        const entity::EntityType* type = EntityRegistry::instance().getType(VanillaEntityTypeKeys::SLIME->name());
        ecs::EntityRegistry* ecsRegistry = world.entityRegistry();
        if (type == nullptr || ecsRegistry == nullptr) {
            return;
        }
        auto slime = type->create(&world, *ecsRegistry);
        if (slime == nullptr) {
            continue;
        }
        auto* slimeEntity = dynamic_cast<SlimeEntity*>(slime.get());
        if (slimeEntity == nullptr) {
            continue;
        }
        slimeEntity->setSlimeSize(2, true);
        slimeEntity->setPosition(entity.x() + 0.5f, entity.y() + 0.5f, entity.z() + 0.5f);
        slimeEntity->setRotation(world.getRandom().nextFloat() * 360.0f, 0.0f);
        world.spawnEntity(std::move(slime));
    }
}

/// 风充能：以实体为中心触发风弹式爆炸（对齐 vanilla WindChargedMobEffect.onMobRemoved）
void _explodeWindBurst(IWorld& world, LivingEntity& entity)
{
    const f32 radius = 3.0f + world.getRandom().nextFloat() * 2.0f;
    const Vector3 center(entity.x(), entity.y() + entity.height() / 2.0f, entity.z());
    // 风弹爆炸：不破坏方块、不伤实体（仅击退），屏障/基岩抗性无穷大。
    world::explosion::Explosion explosion(world,
        center,
        radius,
        world::explosion::ExplosionMode::None,
        false,
        &entity,
        nullptr,
        nullptr,
        std::make_unique<world::explosion::WindChargeExplosionContext>(&entity));
    explosion.explode();
}

} // namespace

void applyOnMobRemoved(IWorld& world, LivingEntity& entity, EffectType type, i32 amplifier, RemovalReason reason)
{
    MC_UNUSED(amplifier);
    if (reason != RemovalReason::Killed) {
        return;
    }

    switch (type) {
        case EffectType::Weaving: {
            // 对齐 WeavingMobEffect：maxCobwebs 随机 2~3；玩家不受 mobGriefing 限制
            const bool canGrief = world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
            if (dynamic_cast<Player*>(&entity) != nullptr || canGrief) {
                _spawnCobwebsRandomlyAround(world, entity, _randomBetweenInclusive(world.getRandom(), 2, 3));
            }
            break;
        }
        case EffectType::Oozing: {
            // 对齐 OozingMobEffect：spawnedCount 固定 2
            _spawnOozingSlimes(world, entity, 2);
            break;
        }
        case EffectType::WindCharged: {
            _explodeWindBurst(world, entity);
            break;
        }
        default:
            // INFESTED 的蠹虫生成发生在 onMobHurt（非移除时），其余效果无移除行为。
            break;
    }
}

void spawnInfestedSilverfish(IWorld& world, LivingEntity& entity)
{
    // 对齐 vanilla InfestedMobEffect.onMobHurt：每次触发生成 1~2 只蠹虫。
    const i32 count = _randomBetweenInclusive(world.getRandom(), 1, 2);
    for (i32 i = 0; i < count; ++i) {
        _spawnSilverfish(world, entity);
    }
}

} // namespace onremoval
} // namespace effect
} // namespace entity
} // namespace mc
