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
 * The above copyright notice shall be included in all
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

#include "WindChargeEntity.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/WindChargeStateComponent.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/explosion/ExplosionImmunityContext.hpp"
#include "common/world/gamerule/GameRules.hpp"

// 粒子类型
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ThrowableEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/ray/Ray.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace entity {

// ============================================================================
// 风爆常量
// ============================================================================

namespace {

/// 玩家投掷风弹的爆炸半径（对应 MC WindCharge.EXPLOSION_RADIUS = 1.2F）
constexpr f32 PLAYER_EXPLOSION_RADIUS = 1.2f;

/// 旋风人投掷风弹的爆炸半径（对应 MC BreezeWindCharge.EXPLOSION_RADIUS = 3.0F）
constexpr f32 BREEZE_EXPLOSION_RADIUS = 3.0f;

/// 玩家投掷风弹的击退乘数（对应 MC WindCharge.KNOCKBACK_MULTIPLIER = 1.22F）
constexpr f32 PLAYER_KNOCKBACK_MULTIPLIER = 1.22f;

/// 旋风人投掷风弹的击退乘数（对应 MC 使用默认值 1.0）
constexpr f32 BREEZE_KNOCKBACK_MULTIPLIER = 1.0f;

/// 实体影响范围乘数（与 MC ServerExplosion 一致：radius * 2.0）
constexpr f32 ENTITY_RANGE_MULTIPLIER = 2.0f;

/// 风爆音量
constexpr f32 WIND_BURST_VOLUME = 1.0f;

/// 风爆音调
constexpr f32 WIND_BURST_PITCH = 1.0f;

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

WindChargeEntity::WindChargeEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ThrowableEntity(id, registry)
{
    // 批次6 子目标2 Step1：attach WindChargeStateComponent（风弹爆裂状态 3 字段）。
    // Step4 将把 m_hasBurst/m_burstCenter/m_hasBurstCenter 读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::WindChargeStateComponent>(m_entityContext->entity());
}

std::unique_ptr<Entity> WindChargeEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<WindChargeEntity>(EntityInstanceId(0), registry);
}

// ============================================================================
// 命中处理
// ============================================================================

void WindChargeEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    auto* living = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (living != nullptr) {
        // 风弹造成1点风爆伤害
        // 与 MC 1.21.11 一致：风爆伤害不绕过护甲（不在 DamageTypeTags::BYPASSES_ARMOR 中）
        // 对应 MC: DamageSources.windCharge(this, target)
        Entity* shooter = getShooter();
        bool isPlayer = shooter != nullptr && shooter->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        auto damageSource = DamageSources::windBurst(this, shooter, isPlayer);
        living->hurt(damageSource, PLAYER_DAMAGE);
    }

    // 命中实体后触发风爆
    applyWindBurst();
    remove();
}

void WindChargeEntity::onBlockHit(const RayTraceResult& result)
{
    // 命中方块时，将风爆中心沿命中面法线偏移
    // 对应 MC: Vec3.atLowerCornerOf(result.direction.getNormal()).multiply(0.25, 0.25, 0.25)
    if (result.type == RayTraceResultType::Block) {
        auto* wind = tryGetComponent<ecs::WindChargeStateComponent>();
        if (wind != nullptr) {
            const BlockPos& hitPos = result.blockPos;
            // 将命中方块的坐标乘以 0.25 作为偏移量
            wind->m_burstCenter = Vector3(static_cast<f32>(hitPos.x) * 0.25f,
                static_cast<f32>(hitPos.y) * 0.25f,
                static_cast<f32>(hitPos.z) * 0.25f);
            wind->m_hasBurstCenter = true;
        }
    }

    // 命中方块后触发风爆
    applyWindBurst();
    remove();
}

void WindChargeEntity::onImpact(const RayTraceResult& /*result*/)
{
    // onEntityHit / onBlockHit 已处理
}

// ============================================================================
// 风爆效果
// ============================================================================

f32 WindChargeEntity::getExplosionRadius() const
{
    // 玩家投掷风弹：半径 1.2
    // 旋风人投掷风弹：半径 3.0
    Entity* shooter = getShooter();
    if (shooter != nullptr && shooter->entityType() == entity::VanillaEntityTypeKeys::BREEZE) {
        return BREEZE_EXPLOSION_RADIUS;
    }
    return PLAYER_EXPLOSION_RADIUS;
}

f32 WindChargeEntity::getKnockbackMultiplier() const
{
    // 玩家投掷风弹：击退乘数 1.22
    // 旋风人投掷风弹：默认 1.0
    Entity* shooter = getShooter();
    if (shooter != nullptr && shooter->entityType() == entity::VanillaEntityTypeKeys::BREEZE) {
        return BREEZE_KNOCKBACK_MULTIPLIER;
    }
    return PLAYER_KNOCKBACK_MULTIPLIER;
}

Vector3 WindChargeEntity::getBurstCenter() const
{
    const auto* wind = tryGetComponent<ecs::WindChargeStateComponent>();
    if (wind != nullptr && wind->m_hasBurstCenter) {
        return wind->m_burstCenter;
    }
    return position();
}

void WindChargeEntity::applyWindBurst()
{
    auto* wind = tryGetComponent<ecs::WindChargeStateComponent>();
    if (wind != nullptr && wind->m_hasBurst) {
        return;
    }
    if (wind != nullptr) {
        wind->m_hasBurst = true;
    }

    if (m_world == nullptr) {
        return;
    }

    const Vector3 burstPos = getBurstCenter();
    const f32 radius = getExplosionRadius();
    const f32 knockbackMultiplier = getKnockbackMultiplier();
    const f32 range = radius * ENTITY_RANGE_MULTIPLIER;

    // 收集每个受影响玩家的击退向量，用于广播 Explosion IR
    // 对应 MC Java ServerExplosion.hitPlayers 映射
    std::unordered_map<u64, Vector3> playerKnockback;

    // ========== 1. 查询范围内的实体 ==========
    // 对应 MC ServerExplosion.hurtEntities 中的实体范围查询
    AxisAlignedBB searchBox(burstPos.x - range - 1.0f,
        burstPos.y - range - 1.0f,
        burstPos.z - range - 1.0f,
        burstPos.x + range + 1.0f,
        burstPos.y + range + 1.0f,
        burstPos.z + range + 1.0f);

    std::vector<Entity*> entities = m_world->getEntitiesInAABB(searchBox, this);

    // 风弹路径等价 vanilla TRIGGER：不破坏方块，shouldAffectBlocklikeEntities 恒 false，
    // 故掉落物/盔甲架等"方块类实体"在此路径下恒忽略爆炸（不受击退）。
    // 直接源为风弹自身，间接源追溯其发射者（若为 LivingEntity），供载具判定。
    Entity* shooter = getShooter();
    const world::explosion::ExplosionImmunityContext immunityCtx{
        .shouldAffectBlocklikeEntities = false,
        .indirectSource = shooter != nullptr ? dynamic_cast<LivingEntity*>(shooter) : nullptr,
        .directSource = this,
        .mobGriefing = m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING),
    };

    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 跳过忽略爆炸的实体
        if (entity->ignoreExplosion(immunityCtx)) {
            continue;
        }

        // ========== 1. 计算距离和方向 ==========
        // 对应 MC: entity.distanceToSqr(center) / (radius * 2)
        Vector3 entityPos = entity->position();
        f32 dx = entityPos.x - burstPos.x;
        f32 dy = entityPos.y - burstPos.y;
        f32 dz = entityPos.z - burstPos.z;
        f32 distanceSq = dx * dx + dy * dy + dz * dz;

        // 距离比例
        f32 distanceRatio = std::sqrt(distanceSq) / range;
        if (distanceRatio > 1.0f) {
            continue; // 超出影响范围
        }

        // 归一化方向向量
        f32 length = std::sqrt(distanceSq);
        if (length < 0.001f) {
            // 实体就在爆炸中心，使用随机方向
            dx = (m_world->getRandom().nextFloat() * 2.0f - 1.0f);
            dy = (m_world->getRandom().nextFloat() * 2.0f - 1.0f);
            dz = (m_world->getRandom().nextFloat() * 2.0f - 1.0f);
            length = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (length < 0.001f) {
                length = 1.0f;
            }
        }
        dx /= length;
        dy /= length;
        dz /= length;

        // ========== 2. 计算视线遮挡密度 ==========
        // 对应 MC: Explosion.getSeenPercent() / getBlockDensity()
        f32 density = _calculateSeenPercent(entity->boundingBox(), burstPos);

        // ========== 3. 计算推力 ==========
        // 对应 MC ServerExplosion 的推力计算：
        // impact = (1.0 - distanceRatio) * density
        // knockbackResistance = 爆炸保护附魔提供的击退抗性
        // finalImpact = impact * knockbackMultiplier * (1.0 - knockbackResistance)
        f32 impact = (1.0f - distanceRatio) * density;

        // 计算爆炸击退抗性（来自爆炸保护附魔）
        f32 knockbackResistance = 0.0f;
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living != nullptr) {
            i32 blastProtection = item::enchant::EnchantmentHelper::getTotalArmorProtection(
                living->getArmorSlots(), DamageFlags::EXPLOSION);
            if (blastProtection > 0) {
                // 爆炸保护每级减少 15% 击退
                knockbackResistance = static_cast<f32>(blastProtection) * 0.15f;
            }
        }

        // 最终推力 = impact * knockbackMultiplier * (1 - knockbackResistance)
        f32 finalImpact = impact * knockbackMultiplier * (1.0f - knockbackResistance);

        if (finalImpact <= 0.0f) {
            continue;
        }

        // ========== 4. 玩家分支：仅通过 Explosion IR 应用击退 ==========
        // 玩家速度由客户端权威管理（对应 MC Java ServerPlayer 的 client-authoritative motion）。
        // 服务端不调用 addVelocity，避免 EntityVelocityPacket 与 Explosion IR 双重应用。
        // 同时清除 LivingEntity::hurt 设置的 hurtMarked，防止 EntityTracker 发送
        // EntityVelocityPacket 覆盖客户端速度。
        Player* player = dynamic_cast<Player*>(entity);
        if (player != nullptr) {
            // 观察者模式不受击退
            if (player->isSpectator()) {
                continue;
            }
            // 创造模式飞行中不受击退
            const PlayerAbilities& abilities = player->abilities();
            if (player->isCreative() && abilities.flying) {
                continue;
            }

            // 风弹对玩家造成 1 点风爆伤害（与 onEntityHit 中的伤害一致）
            // 对应 MC: DamageSources.windCharge(this, target)
            Entity* shooter = getShooter();
            bool isPlayerShooter = shooter != nullptr && shooter->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
            auto damageSource = DamageSources::windBurst(this, shooter, isPlayerShooter);
            player->hurt(damageSource, PLAYER_DAMAGE);

            // 清除 hurtMarked，防止 EntityTracker 发送 EntityVelocityPacket
            // （玩家速度由客户端通过 Explosion IR 应用）
            player->clearHurtMarked();

            // 记录玩家击退向量，将通过 Explosion IR 发送给客户端
            // 客户端收到后调用 addDeltaMovement/addVelocity 累加到现有速度
            playerKnockback[static_cast<u64>(player->id())] =
                Vector3(dx * finalImpact, dy * finalImpact, dz * finalImpact);

            // 通知实体被爆炸击中（冲量坠落伤害免疫等）
            entity->onExplosionHit(getShooter());
            continue;
        }

        // ========== 5. 非玩家实体分支：服务端权威速度 ==========
        // 风弹自身不受推力（addVelocity 已被重写为空方法）
        // 对应 MC: entity.push(vec32) 即 deltaMovement += vec32
        entity->addVelocity(dx * finalImpact, dy * finalImpact, dz * finalImpact);
        // 标记受伤（风弹推力改变了实体速度，需要同步到客户端）
        // 对应 MC Java ApplyEntityImpulse 中 hurtMarked = true
        entity->markHurt();

        // 通知实体被爆炸击中（冲量坠落伤害免疫等）
        // 对应 MC Java ServerPlayer.onExplosionHit 中对 WindCharge 的检查
        entity->onExplosionHit(getShooter());
    }

    // ========== 8. 广播爆炸事件给附近玩家 ==========
    // 对应 MC Java ServerLevel.explode() 中遍历 64 格内玩家发送 ClientboundExplodePacket
    // ServerWorld 通过回调委托给 MinecraftServer::broadcastExplosionInRange 进行范围筛选和发包
    m_world->broadcastExplosion(burstPos, radius, {}, playerKnockback);

    // ========== 9. 播放风爆音效 ==========
    _playWindBurstSound(burstPos);

    // ========== 10. 生成风爆粒子效果 ==========
    _spawnWindBurstParticles(burstPos);
}

f32 WindChargeEntity::_calculateSeenPercent(const AxisAlignedBB& entityBox, const Vector3& center) const
{
    // 参考 MC Explosion.getBlockDensity 和项目的 Explosion::_getBlockDensity
    // 在实体碰撞箱内采样点，检测有多少可以看到爆炸中心

    // 计算采样步长
    f32 dx = (entityBox.maxX - entityBox.minX) * 2.0f + 1.0f;
    f32 dy = (entityBox.maxY - entityBox.minY) * 2.0f + 1.0f;
    f32 dz = (entityBox.maxZ - entityBox.minZ) * 2.0f + 1.0f;

    f32 stepX = 1.0f / dx;
    f32 stepY = 1.0f / dy;
    f32 stepZ = 1.0f / dz;

    // 计算偏移量以居中采样点
    f32 offsetX = (1.0f - std::floor(1.0f / stepX) * stepX) * 0.5f;
    f32 offsetZ = (1.0f - std::floor(1.0f / stepZ) * stepZ) * 0.5f;

    // 检查步长是否有效
    if (stepX <= 0.0f || stepY <= 0.0f || stepZ <= 0.0f) {
        return 0.0f;
    }

    i32 visible = 0;
    i32 total = 0;

    // 采样点
    for (f32 fx = 0.0f; fx <= 1.0f; fx += stepX) {
        for (f32 fy = 0.0f; fy <= 1.0f; fy += stepY) {
            for (f32 fz = 0.0f; fz <= 1.0f; fz += stepZ) {
                // 采样点位置（世界坐标）
                Vector3 samplePoint(entityBox.minX + fx * (entityBox.maxX - entityBox.minX) + offsetX,
                    entityBox.minY + fy * (entityBox.maxY - entityBox.minY),
                    entityBox.minZ + fz * (entityBox.maxZ - entityBox.minZ) + offsetZ);

                // 使用射线检测是否有方块阻挡
                Vector3 dir(center.x - samplePoint.x, center.y - samplePoint.y, center.z - samplePoint.z);
                f32 distance = dir.length();
                Ray ray(samplePoint, dir);
                RaycastContext context(ray, distance);

                BlockRaycastResult result = raycastBlocks(context, *m_world);

                // 如果射线未击中任何方块，说明该采样点可以看到爆炸中心
                if (result.isMiss()) {
                    ++visible;
                }
                ++total;
            }
        }
    }

    return total > 0 ? static_cast<f32>(visible) / static_cast<f32>(total) : 0.0f;
}

void WindChargeEntity::_playWindBurstSound(const Vector3& pos) const
{
    // 玩家风弹使用 ENTITY_WIND_CHARGE_WIND_BURST
    // 旋风人风弹使用 ENTITY_BREEZE_WIND_CHARGE_BURST
    Entity* shooter = getShooter();
    bool isBreeze = shooter != nullptr && shooter->entityType() == entity::VanillaEntityTypeKeys::BREEZE;

    const ResourceLocation& soundEvent =
        isBreeze ? SoundEvents::ENTITY_BREEZE_WIND_CHARGE_BURST : SoundEvents::ENTITY_WIND_CHARGE_WIND_BURST;

    m_world->playSound(soundEvent, sound::SoundCategory::Blocks, pos, WIND_BURST_VOLUME, WIND_BURST_PITCH);
}

void WindChargeEntity::_spawnWindBurstParticles(const Vector3& pos) const
{
    using ParticleTypeId = particle::ParticleTypeId;

    // 风爆粒子：小型发射器 + 大型发射器
    // 对应 MC: ParticleTypes.GUST_EMITTER_SMALL 和 ParticleTypes.GUST_EMITTER_LARGE
    m_world->addParticle(ParticleTypeId::GustEmitterSmall, pos, Vector3(0.0f, 0.0f, 0.0f));
    m_world->addParticle(ParticleTypeId::GustEmitterLarge, pos, Vector3(0.0f, 0.0f, 0.0f));
}

} // namespace entity
} // namespace mc
