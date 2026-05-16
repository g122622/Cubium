/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the above copyright notice
* and this permission notice shall be included in all copies or substantial portions
* of the Software.
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

#include "AbstractFireballEntity.hpp"

#include "../../../core/Constants.hpp"
#include "../../../core/Types.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/gamerule/GameRules.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../effect/EffectInstance.hpp"
#include "../../effect/EffectType.hpp"
#include "../effect/EffectEntities.hpp"
#include <cmath>

namespace mc {
namespace entity {

AbstractFireballEntity::AbstractFireballEntity(LegacyEntityType type, EntityId id)
    : DamagingProjectileEntity(type, id)
{}

FireballEntity::FireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    setDamage(6.0f);
}

std::unique_ptr<Entity> FireballEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FireballEntity>(LegacyEntityType::Unknown, 0);
}

void FireballEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    IWorld* worldPtr = world();
    Entity* shooter = getShooter();

    // MC 1.16.5: 创建火球伤害来源
    // DamageSource.func_233547_a_(this, shooter) -> IndirectEntityDamageSource("fireball", shooter, this).setFireDamage().setProjectile()
    auto damageSource = DamageSources::fireball(this, shooter, false);

    // MC 1.16.5: 对 LivingEntity 造成 6.0 点伤害
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (livingTarget != nullptr) {
        livingTarget->hurt(damageSource, damage());
    }

    // MC 1.16.5: 触发爆炸（爆炸半径 = explosionPower，默认 1.0）
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        worldPtr->createExplosion(
            result.hitPosition,
            static_cast<f32>(m_explosionPower),
            mode,
            true, // 产生火焰
            shooter);
    }

    remove();
}

void FireballEntity::onBlockHit(const RayTraceResult& result)
{
    IWorld* worldPtr = world();
    Entity* shooter = getShooter();

    // MC 1.16.5: 方块命中时同样触发爆炸
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        worldPtr->createExplosion(
            result.hitPosition,
            static_cast<f32>(m_explosionPower),
            mode,
            true, // 产生火焰
            shooter);
    }

    remove();
}

SmallFireballEntity::SmallFireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    setDamage(5.0f);
}

std::unique_ptr<Entity> SmallFireballEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SmallFireballEntity>(LegacyEntityType::Unknown, 0);
}

void SmallFireballEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    IWorld* worldPtr = world();
    Entity* shooter = getShooter();

    // MC 1.16.5: 创建火球伤害来源
    // DamageSource.func_233547_a_(this, shooter)
    auto damageSource = DamageSources::fireball(this, shooter, false);

    // MC 1.16.5: 只有目标不免疫火焰时才造成伤害和点燃
    if (!result.hitEntity->isImmuneToFire()) {
        // 保存当前燃烧时间
        i32 fireTicks = result.hitEntity->getFireTimer();

        // 点燃目标 5 秒
        result.hitEntity->setFire(5);

        // MC 1.16.5: 对 LivingEntity 造成 5.0 点伤害
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
        if (livingTarget != nullptr) {
            bool hurt = livingTarget->hurt(damageSource, damage());
            if (!hurt) {
                // 如果伤害失败，恢复原燃烧时间
                result.hitEntity->forceFireTicks(fireTicks);
            }
        } else {
            // 非 LivingEntity 也尝试造成伤害（如船、矿车等）
            // Entity::hurt 通过 virtual 调用
            result.hitEntity->hurt(damageSource, damage());
        }
    }

    remove();
}

void SmallFireballEntity::onBlockHit(const RayTraceResult& result)
{
    IWorld* worldPtr = world();

    // MC 1.16.5: 检查是否可以放置火焰
    // 条件：发射者为空 或 发射者不是 MobEntity 或 mobGriefing 为 true
    bool canPlaceFire = true;
    if (worldPtr != nullptr) {
        canPlaceFire = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
    }

    if (canPlaceFire && worldPtr != nullptr) {
        // MC 1.16.5: SmallFireballEntity.func_230299_a_()
        // 在命中方块相邻的空气位置放置火焰
        // 简化实现：尝试在命中位置上方放置火焰

        const BlockPos& hitPos = result.blockPos;
        BlockPos placePos = hitPos.up(); // 尝试在碰撞方块上方放置火焰

        const BlockState* placeState = worldPtr->getBlockState(placePos);

        // 检查目标位置是否为空气
        if (placeState != nullptr && placeState->isAir()) {
            // 放置火焰方块
            // 火焰方块会在 tick 时自动检查是否可以维持
            if (VanillaBlocks::FIRE != nullptr) {
                const BlockState& fireState = VanillaBlocks::FIRE->defaultState();
                worldPtr->setBlockState(placePos, &fireState, 3);
            }
        }
    }

    remove();
}

DragonFireballEntity::DragonFireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    setDamage(12.0f);
}

std::unique_ptr<Entity> DragonFireballEntity::create(IWorld* /*world*/)
{
    return std::make_unique<DragonFireballEntity>(LegacyEntityType::Unknown, 0);
}

void DragonFireballEntity::onEntityHit(const RayTraceResult& result)
{
    // MC 1.16.5: 龙息火球不直接造成伤害，而是生成龙息区域效果云
    // 参考 DragonFireballEntity.onImpact()
    createDragonBreathCloud();
    remove();
    (void)result; // 避免未使用警告
}

void DragonFireballEntity::onBlockHit(const RayTraceResult& /*result*/)
{
    // MC 1.16.5: 方块命中也生成龙息区域效果云
    createDragonBreathCloud();
    remove();
}

void DragonFireballEntity::createDragonBreathCloud()
{
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // MC 1.16.5: 创建龙息区域效果云
    // AreaEffectCloudEntity 参数：
    // - 半径: 3.0F
    // - 持续时间: 600 ticks (30秒)
    // - 半径变化: (7.0F - 3.0F) / 600 ≈ 0.0067F/tick
    // - 效果: 瞬间伤害 II (InstantDamage, amplifier=1)
    // - 粒子: DRAGON_BREATH

    auto cloud = std::make_unique<AreaEffectCloudEntity>();
    cloud->setWorld(worldPtr);
    cloud->setPosition(x(), y(), z());

    // 设置拥有者（发射龙息的实体）
    Entity* shooter = getShooter();
    if (shooter != nullptr) {
        LivingEntity* livingShooter = dynamic_cast<LivingEntity*>(shooter);
        if (livingShooter != nullptr) {
            cloud->setOwner(livingShooter);
        }
    }

    // MC 1.16.5 龙息云参数
    cloud->setRadius(3.0f);
    cloud->setDuration(600);      // 30秒
    cloud->setRadiusPerTick((7.0f - 3.0f) / 600.0f);  // 逐渐扩展到7.0
    cloud->setWaitTime(10);       // 0.5秒等待时间
    cloud->setReapplicationDelay(20);  // 1秒重应用延迟

    // 添加瞬间伤害 II 效果
    // EffectInstance(type, duration, amplifier, ambient, visible, showIcon)
    // 瞬间伤害效果持续时间可以是1 tick，因为它是瞬间生效的
    entity::effect::EffectInstance instantDamage(
        entity::effect::EffectType::InstantDamage,
        1,     // 持续时间（瞬间效果只需要1 tick）
        1,     // amplifier = 1 表示等级 II
        false, // 不是环境效果
        true,  // 显示粒子
        true   // 显示图标
    );
    cloud->addEffect(instantDamage);

    // TODO: 设置龙息粒子类型
    // cloud->setParticleType(ParticleTypes::DRAGON_BREATH);

    // 生成区域效果云
    worldPtr->spawnEntity(std::move(cloud));

    // MC 1.16.5: 播放龙息效果音（事件ID 2006）
    // worldPtr->playEvent(2006, position(), isSilent() ? -1 : 1);
}

WitherSkullEntity::WitherSkullEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    setDamage(8.0f);
}

std::unique_ptr<Entity> WitherSkullEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitherSkullEntity>(LegacyEntityType::Unknown, 0);
}

void WitherSkullEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    IWorld* worldPtr = world();
    Entity* shooter = getShooter();
    LivingEntity* livingShooter = shooter != nullptr ? dynamic_cast<LivingEntity*>(shooter) : nullptr;

    // MC 1.16.5: 凋灵之首造成伤害
    // DamageSource.func_233549_a_(this, shooter) -> IndirectEntityDamageSource("witherSkull", shooter, this).setProjectile()
    // 如果 shooter 是 LivingEntity，使用投射物伤害；否则使用魔法伤害
    IndirectEntityDamageSource damageSource(
        livingShooter != nullptr ? DamageType::MobProjectile : DamageType::Magic,
        shooter != nullptr ? shooter : this,
        this,
        false);
    if (livingShooter != nullptr) {
        damageSource.setProjectile();
    } else {
        damageSource.setMagicDamage();
    }

    // MC 1.16.5: 对 LivingEntity 造成伤害
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    bool causedDamage = false;
    f32 damageAmount = damage(); // 使用基类方法获取伤害值

    if (livingTarget != nullptr) {
        // 有发射者时造成 8.0 伤害，无发射者时造成 5.0 魔法伤害
        if (livingShooter == nullptr) {
            damageAmount = 5.0f;
        }
        causedDamage = livingTarget->hurt(damageSource, damageAmount);

        // MC 1.16.5: 如果造成伤害，施加凋零效果
        if (causedDamage) {
            // 根据难度调整凋零效果持续时间
            // 简单难度：无凋零效果
            // 普通难度：凋零 II 10 秒（200 ticks）
            // 困难难度：凋零 II 40 秒（800 ticks）
            i32 witherDuration = 0;
            if (worldPtr != nullptr) {
                Difficulty diff = worldPtr->difficulty();
                switch (diff) {
                    case Difficulty::Normal:
                        witherDuration = 200;  // 10 秒
                        break;
                    case Difficulty::Hard:
                        witherDuration = 800;  // 40 秒
                        break;
                    case Difficulty::Easy:
                    case Difficulty::Peaceful:
                    default:
                        witherDuration = 0;
                        break;
                }
            }

            if (witherDuration > 0) {
                entity::effect::EffectInstance witherEffect(
                    entity::effect::EffectType::Wither,
                    witherDuration,
                    1  // II级 = amplifier 1
                );
                livingTarget->addEffect(std::move(witherEffect));
            }

            // MC 1.16.5: 如果目标死亡，治疗发射者 5.0 HP
            if (livingShooter != nullptr && livingTarget->isDead()) {
                livingShooter->heal(5.0f);
            }
        }
    } else {
        // 非 LivingEntity 目标（如船、矿车等）
        result.hitEntity->hurt(damageSource, damageAmount);
    }

    // MC 1.16.5: 凋灵之首爆炸半径 1.0
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        // 蓝色凋灵之首破坏更多方块类型（爆炸抗性更高的方块）
        // TODO: 蓝色凋灵之首有特殊的方块破坏规则，需要在爆炸系统中实现
        (void)m_blue;

        worldPtr->createExplosion(
            result.hitPosition,
            game::explosion::WITHER_SKULL_RADIUS, // 1.0f
            mode,
            false, // 不生成火焰
            shooter);
    }

    remove();
}

void WitherSkullEntity::onBlockHit(const RayTraceResult& result)
{
    IWorld* worldPtr = world();
    Entity* shooter = getShooter();

    // MC 1.16.5: 凋灵之首在方块上爆炸
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        // 蓝色凋灵之首破坏更多方块类型
        (void)m_blue;

        worldPtr->createExplosion(
            result.hitPosition,
            game::explosion::WITHER_SKULL_RADIUS, // 1.0f
            mode,
            false, // 不生成火焰
            shooter);
    }

    remove();
}

f32 WitherSkullEntity::getMotionFactor() const
{
    // MC 1.16.5: 蓝色凋灵之首运动因子为 0.73，普通为 0.95
    return m_blue ? 0.73f : 0.95f;
}

bool WitherSkullEntity::isFiery() const
{
    // MC 1.16.5: 凋灵之首不燃烧
    return false;
}

} // namespace entity
} // namespace mc
