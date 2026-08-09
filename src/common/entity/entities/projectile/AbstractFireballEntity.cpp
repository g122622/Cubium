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

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/FireballStateComponent.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/projectile/DamagingProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/nether/FireBlock.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionContext.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace entity {

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = DamagingProjectileEntity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点供子类（Fireball/WitherSkull）ClassRegisterGuard
// 沿父链查找最高 id 时穿过（DamagingProjectile/AbstractFireball 均无字段，最高 id 来自
// ProjectileEntity/Entity），子类首字段续接到 id8。
const EntityClassInfo& AbstractFireballEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"AbstractFireballEntity", &DamagingProjectileEntity::classInfo()};
    return s_classInfo;
}

AbstractFireballEntity::AbstractFireballEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : DamagingProjectileEntity(id, registry)
{}

// 静态数据参数定义（对应 MC 1.21.11 Fireball.defineSynchedData() 的 DATA_ITEM_STACK）。
// 真实 id 在 registerData() 内由 ClassRegisterGuard 沿继承链续接分配
// （Entity 8 字段后 → id8，DamagingProjectile/AbstractFireball 无字段不占 id）。
entity::DataParameter<network::ir::play::ItemStackView> FireballEntity::DATA_ITEM_STACK_PARAM =
    entity::EntityDataManager::createKey<network::ir::play::ItemStackView>();

const EntityClassInfo& FireballEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"FireballEntity", &AbstractFireballEntity::classInfo()};
    return s_classInfo;
}

FireballEntity::FireballEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractFireballEntity(id, registry)
{
    setDamage(6.0f);
    // 批次6 子目标2 Step1：attach FireballStateComponent（火球族状态，本类用 m_explosionPower）。
    // Step4 将把 m_explosionPower 读写改走组件；Step5 补 DATA_ITEM_STACK 同步字段对齐 vanilla。
    m_entityContext->enttRegistry().emplace<ecs::FireballStateComponent>(m_entityContext->entity());
    // 注册同步字段 DATA_ITEM_STACK（id8，占位空 ItemStackView）。C++ 虚函数在构造函数不派生，
    // AbstractFireballEntity 构造调用的 registerData 不会派生到 Fireball，故此处显式调用本类
    // registerData 注册火球专属同步字段。
    registerData();
}

void FireballEntity::registerData()
{
    // 先调基类注册基础参数（FLAGS/AIR/CUSTOM_NAME 等，id0..7）。
    // DamagingProjectile/AbstractFireball 为无同步字段的中间基类（无 registerData override），
    // 此限定调用经 ProjectileEntity 虚分发到 Entity::registerData，注册 id0..7。
    // 本类 classInfo 节点（parent=AbstractFireball→DamagingProjectile→Projectile→Entity）
    // 由下方 guard 压栈，allocateIdForCurrentClass 沿父链查最高 id=7，DATA_ITEM_STACK 续接到 id8。
    ProjectileEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册火球专属同步参数（id8，对应 vanilla Fireball.DATA_ITEM_STACK）。
    // TODO: 项目 FireballEntity 当前无 item 字段，占位空 ItemStackView 保持 wire 位置对齐；
    // 补齐物品字段后镜像真实物品。
    m_dataManager.registerParam(DATA_ITEM_STACK_PARAM, network::ir::toItemStackView(ItemStack()));
}

std::unique_ptr<Entity> FireballEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<FireballEntity>(0, registry);
}

void FireballEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    IWorld* worldPtr = world();
    Entity* shooter = getShooter();

    // 创建火球伤害来源
    auto damageSource = DamageSources::fireball(this, shooter, false);

    // 对 LivingEntity 造成 6.0 点伤害
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (livingTarget != nullptr) {
        livingTarget->hurt(damageSource, damage());
    }

    // 触发爆炸（爆炸半径 = explosionPower，默认 1.0）
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode =
            worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        worldPtr->createExplosion(result.hitPosition,
            static_cast<f32>(explosionPower()),
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

    // 方块命中时同样触发爆炸
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode =
            worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        worldPtr->createExplosion(result.hitPosition,
            static_cast<f32>(explosionPower()),
            mode,
            true, // 产生火焰
            shooter);
    }

    remove();
}

// 批次6 子目标2 Step4：Fireball/WitherSkull 状态字段经 ecs::FireballStateComponent 读写。
i32 FireballEntity::explosionPower() const
{
    const auto* c = tryGetComponent<ecs::FireballStateComponent>();
    return (c != nullptr) ? c->m_explosionPower : 1;
}

void FireballEntity::setExplosionPower(i32 power)
{
    auto* c = tryGetComponent<ecs::FireballStateComponent>();
    if (c != nullptr) {
        c->m_explosionPower = power;
    }
}

SmallFireballEntity::SmallFireballEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractFireballEntity(id, registry)
{
    setDamage(5.0f);
}

std::unique_ptr<Entity> SmallFireballEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SmallFireballEntity>(0, registry);
}

void SmallFireballEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    IWorld* worldPtr = world();
    Entity* shooter = getShooter();

    // 创建火球伤害来源
    auto damageSource = DamageSources::fireball(this, shooter, false);

    // 只有目标不免疫火焰时才造成伤害和点燃
    if (!result.hitEntity->isImmuneToFire()) {
        // 保存当前燃烧时间
        i32 fireTicks = result.hitEntity->getRemainingFireTicks();

        // 点燃目标 5 秒
        result.hitEntity->igniteForSeconds(5.0f);

        // 对 LivingEntity 造成 5.0 点伤害
        LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
        if (livingTarget != nullptr) {
            bool hurt = livingTarget->hurt(damageSource, damage());
            if (!hurt) {
                // 如果伤害失败，恢复原燃烧时间
                result.hitEntity->forceFireTicks(fireTicks);
            }
        } else {
            // 非 LivingEntity 也尝试造成伤害（如船、矿车等）
            result.hitEntity->hurt(damageSource, damage());
        }
    }

    remove();
}

void SmallFireballEntity::onBlockHit(const RayTraceResult& result)
{
    IWorld* worldPtr = world();

    // 检查是否可以放置火焰（需要 mobGriefing 规则允许）
    bool canPlaceFire = true;
    if (worldPtr != nullptr) {
        canPlaceFire = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
    }

    if (canPlaceFire && worldPtr != nullptr) {
        // 在命中方块相邻的空气位置放置火焰
        const BlockPos& hitPos = result.blockPos;
        BlockPos placePos = hitPos.up(); // 尝试在碰撞方块上方放置火焰

        const BlockState* placeState = worldPtr->getBlockState(placePos);

        // 检查目标位置是否为空气
        if (placeState != nullptr && placeState->isAir()) {
            // 根据环境选择正确的火焰类型（灵魂火或普通火）
            const BlockState& fireState = blocks::FireBlock::getFireState(*worldPtr, placePos);
            worldPtr->setBlockState(placePos, &fireState, 3);
        }
    }

    remove();
}

DragonFireballEntity::DragonFireballEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractFireballEntity(id, registry)
{
    setDamage(12.0f);
}

std::unique_ptr<Entity> DragonFireballEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<DragonFireballEntity>(0, registry);
}

void DragonFireballEntity::onEntityHit(const RayTraceResult& result)
{
    // 龙息火球不直接造成伤害，而是生成龙息区域效果云
    _createDragonBreathCloud();
    remove();
    (void)result; // 避免未使用警告
}

void DragonFireballEntity::onBlockHit(const RayTraceResult& /*result*/)
{
    // 方块命中也生成龙息区域效果云
    _createDragonBreathCloud();
    remove();
}

void DragonFireballEntity::_createDragonBreathCloud()
{
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 创建龙息区域效果云
    // 参数：半径 3.0，持续时间 600 ticks (30秒)，半径变化率扩展到 7.0
    // ECS 迁移：实体构造需要 registry 句柄（worldPtr 已判空，此处 registry 必非空）
    auto* registry = worldPtr->entityRegistry();
    if (registry == nullptr) {
        return;
    }
    auto cloud = std::make_unique<AreaEffectCloudEntity>(*registry);

    // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
    cloud->setTypeId(EntityTypeKeys::AREA_EFFECT_CLOUD);

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

    // 龙息云参数
    cloud->setRadius(3.0f);
    cloud->setDuration(600);                         // 30秒
    cloud->setRadiusPerTick((7.0f - 3.0f) / 600.0f); // 逐渐扩展到7.0
    cloud->setWaitTime(10);                          // 0.5秒等待时间
    cloud->setReapplicationDelay(20);                // 1秒重应用延迟

    // 添加瞬间伤害 II 效果
    entity::effect::EffectInstance instantDamage(entity::effect::EffectType::InstantDamage,
        1,     // 持续时间（瞬间效果只需要1 tick）
        1,     // amplifier = 1 表示等级 II
        false, // 不是环境效果
        true,  // 显示粒子
        true   // 显示图标
    );
    cloud->addEffect(instantDamage);

    // 设置龙息粒子类型
    cloud->setParticleType(static_cast<u32>(particle::ParticleTypeId::DragonBreath));

    // 生成区域效果云
    worldPtr->spawnEntity(std::move(cloud));

    // 播放龙息效果音和粒子（事件ID 2006）
    // data: 1 = 播放粒子和声音, -1 = 仅播放粒子（实体静音时）
    worldPtr->playEvent(world::WorldEvents::DRAGON_FIREBALL_HIT,
        BlockPos(math::floorTo<i32>(x()), math::floorTo<i32>(y()), math::floorTo<i32>(z())),
        isSilent() ? -1 : 1);
}

particle::ParticleTypeId DragonFireballEntity::getParticleType() const
{
    // 返回 DRAGON_BREATH 粒子
    return particle::ParticleTypeId::DragonBreath;
}

// 静态数据参数定义（对应 MC 1.21.11 WitherSkull.defineSynchedData() 的 DATA_DANGEROUS）。
// 真实 id 在 registerData() 内由 ClassRegisterGuard 沿继承链续接分配
// （Entity 8 字段后 → id8，DamagingProjectile/AbstractFireball 无字段不占 id）。
entity::DataParameter<bool> WitherSkullEntity::DATA_DANGEROUS_PARAM = entity::EntityDataManager::createKey<bool>();

WitherSkullEntity::WitherSkullEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractFireballEntity(id, registry)
{
    setDamage(8.0f);
    // 批次6 子目标2 Step1：attach FireballStateComponent（火球族状态，本类用 m_blue）。
    // Step4 将把 m_blue 读写改走组件；Step5 补 DATA_DANGEROUS 同步字段对齐 vanilla。
    m_entityContext->enttRegistry().emplace<ecs::FireballStateComponent>(m_entityContext->entity());
    // 注册同步字段 DATA_DANGEROUS（id8）。C++ 虚函数在构造函数不派生，AbstractFireballEntity
    // 构造调用的 registerData 不会派生到 WitherSkull，故此处显式调用本类 registerData。
    registerData();
}

const EntityClassInfo& WitherSkullEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"WitherSkullEntity", &AbstractFireballEntity::classInfo()};
    return s_classInfo;
}

void WitherSkullEntity::registerData()
{
    // 先调基类注册基础参数（id0..7）。DamagingProjectile/AbstractFireball 无 registerData override，
    // 此限定调用经 ProjectileEntity 虚分发到 Entity::registerData。
    ProjectileEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册凋灵之首专属同步参数（id8，对应 vanilla WitherSkull.DATA_DANGEROUS）。
    m_dataManager.registerParam(DATA_DANGEROUS_PARAM, false);
}

std::unique_ptr<Entity> WitherSkullEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<WitherSkullEntity>(0, registry);
}

void WitherSkullEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    IWorld* worldPtr = world();
    Entity* shooter = getShooter();
    LivingEntity* livingShooter = shooter != nullptr ? dynamic_cast<LivingEntity*>(shooter) : nullptr;

    // 凋灵之首造成伤害
    // 如果 shooter 是 LivingEntity，使用投射物伤害；否则使用魔法伤害
    IndirectEntityDamageSource damageSource(livingShooter != nullptr ? DamageType::MobProjectile : DamageType::Magic,
        shooter != nullptr ? shooter : this,
        this,
        false);
    if (livingShooter != nullptr) {
        damageSource.setProjectile();
    } else {
        damageSource.setMagicDamage();
    }

    // 对 LivingEntity 造成伤害
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    bool causedDamage = false;
    f32 damageAmount = damage(); // 使用基类方法获取伤害值

    if (livingTarget != nullptr) {
        // 有发射者时造成 8.0 伤害，无发射者时造成 5.0 魔法伤害
        if (livingShooter == nullptr) {
            damageAmount = 5.0f;
        }
        causedDamage = livingTarget->hurt(damageSource, damageAmount);

        // 如果造成伤害，施加凋零效果
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
                        witherDuration = 200; // 10 秒
                        break;
                    case Difficulty::Hard:
                        witherDuration = 800; // 40 秒
                        break;
                    case Difficulty::Easy:
                    case Difficulty::Peaceful:
                    default:
                        witherDuration = 0;
                        break;
                }
            }

            if (witherDuration > 0) {
                entity::effect::EffectInstance witherEffect(entity::effect::EffectType::Wither,
                    witherDuration,
                    1 // II级 = amplifier 1
                );
                livingTarget->addEffect(std::move(witherEffect));
            }

            // 如果目标死亡，治疗发射者 5.0 HP
            if (livingShooter != nullptr && livingTarget->isDead()) {
                livingShooter->heal(5.0f);
            }
        }
    } else {
        // 非 LivingEntity 目标（如船、矿车等）
        result.hitEntity->hurt(damageSource, damageAmount);
    }

    // 凋灵之首爆炸半径 1.0
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode =
            worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        // 蓝色凋灵之首（dangerous skull）使用特殊爆炸上下文，
        // 可以穿透高抗性方块（黑曜石等），但不能破坏 WITHER_IMMUNE 方块（基岩等）
        auto context = std::make_unique<world::explosion::WitherSkullExplosionContext>(shooter, isBlue());

        worldPtr->createExplosionWithContext(result.hitPosition,
            game::explosion::WITHER_SKULL_RADIUS,
            mode,
            false, // 不生成火焰
            shooter,
            std::move(context));
    }

    remove();
}

void WitherSkullEntity::onBlockHit(const RayTraceResult& result)
{
    IWorld* worldPtr = world();
    Entity* shooter = getShooter();

    // 凋灵之首在方块上爆炸
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode =
            worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        // 蓝色凋灵之首（dangerous skull）使用特殊爆炸上下文，
        // 可以穿透高抗性方块（黑曜石等），但不能破坏 WITHER_IMMUNE 方块（基岩等）
        auto context = std::make_unique<world::explosion::WitherSkullExplosionContext>(shooter, isBlue());

        worldPtr->createExplosionWithContext(result.hitPosition,
            game::explosion::WITHER_SKULL_RADIUS,
            mode,
            false, // 不生成火焰
            shooter,
            std::move(context));
    }

    remove();
}

f32 WitherSkullEntity::getMotionFactor() const
{
    // 蓝色凋灵之首运动因子为 0.73，普通为 0.95
    return isBlue() ? 0.73f : 0.95f;
}

bool WitherSkullEntity::isFiery() const
{
    // 凋灵之首不燃烧
    return false;
}

bool WitherSkullEntity::isBlue() const
{
    const auto* c = tryGetComponent<ecs::FireballStateComponent>();
    return (c != nullptr) ? c->m_blue : false;
}

void WitherSkullEntity::setBlue(bool blue)
{
    auto* c = tryGetComponent<ecs::FireballStateComponent>();
    if (c != nullptr) {
        c->m_blue = blue;
    }
    // 批次6 子目标2 Step5：镜像同步 DATA_DANGEROUS（对齐 vanilla WitherSkull）。
    m_dataManager.set(DATA_DANGEROUS_PARAM, blue);
}

} // namespace entity
} // namespace mc
