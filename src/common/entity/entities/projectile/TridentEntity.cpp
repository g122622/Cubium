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

#include "TridentEntity.hpp"
#include "../../../item/enchantment/EnchantmentHelper.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../core/EntityRegistry.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../entities/effect/EffectEntities.hpp"
#include "../../entities/player/Player.hpp"
#include "../../registry/VanillaEntityTypeKeys.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "ProjectileHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/ProjectileArrowStateComponent.hpp"
#include "common/entity/ecs/components/TridentStateComponent.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace entity {

namespace {

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity)
{
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

// 静态数据参数定义（对应 MC 1.21.11 ThrownTrident.defineSynchedData()）。
// 真实 id 在 registerData() 内由 ClassRegisterGuard 沿继承链续接分配
// （AbstractArrow id8/9/10 后 → id11/12）。
entity::DataParameter<i8> TridentEntity::DATA_LOYALTY_PARAM = entity::EntityDataManager::createKey<i8>();
entity::DataParameter<bool> TridentEntity::DATA_FOIL_PARAM = entity::EntityDataManager::createKey<bool>();

const EntityClassInfo& TridentEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"TridentEntity", &AbstractArrowEntity::classInfo()};
    return s_classInfo;
}

void TridentEntity::registerData()
{
    // 先调用父类注册 AbstractArrow 同步字段（id8/9/10）
    AbstractArrowEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册三叉戟专属同步参数（id11/12，对应 vanilla ThrownTrident）
    m_dataManager.registerParam(DATA_LOYALTY_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(DATA_FOIL_PARAM, false);
}

TridentEntity::TridentEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractArrowEntity(id, registry)
{
    setDamage(8.0f); // 三叉戟伤害更高
    setPickupStatus(PickupStatus::Allowed);
    // 批次6 子目标2 Step1：attach TridentStateComponent（三叉戟物品/命中/返回/忠诚 6 字段）。
    // dealtDamage 复用父类 ProjectileArrowStateComponent::m_dealtDamage 不另存。
    // Step4 已把 m_tridentStack/m_hitBlock/m_returning/m_hitBlockPos/m_loyaltyLevel/
    // m_returningTicks 读写改走组件；Step5 补 DATA_LOYALTY/DATA_FOIL 同步字段。
    m_entityContext->enttRegistry().emplace<ecs::TridentStateComponent>(m_entityContext->entity());
    // C++ 虚函数在构造函数中不会派生到子类，故 AbstractArrowEntity 构造调用的 registerData()
    // 不会派生到 TridentEntity::registerData()。此处显式调用注册三叉戟专属同步字段（id11/12）。
    registerData();
}

std::unique_ptr<Entity> TridentEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<TridentEntity>(0, registry);
}

void TridentEntity::tick()
{
    auto* trident = tryGetComponent<ecs::TridentStateComponent>();

    // 检查是否应该开始返回
    if (timeInGround() > 4) {
        setDealtDamage(true);
    }

    // 检查忠诚附魔状态
    Entity* shooter = getShooter();
    if ((hasDealtDamage() || isInGround()) && shooter != nullptr) {
        const i32 loyaltyLevel = trident ? static_cast<i32>(trident->m_loyaltyLevel) : 0;

        if (loyaltyLevel > 0 && !_shouldReturnToThrower()) {
            // 忠诚附魔但无法返回（射手已死亡或是旁观者模式），掉落物品
            if (!m_world->isClientSide() && pickupStatus() == PickupStatus::Allowed) {
                math::Random rng = createRandomFromEntity(*this);
                ItemDropHelper::spawnItemAtEntity(this, getArrowStack(), 0.1f, rng);
            }
            remove();
        } else if (loyaltyLevel > 0) {
            // 开始返回
            setNoClip(true);
            _tickReturning();
            return;
        }
    }

    // 调用父类tick
    AbstractArrowEntity::tick();
}

bool TridentEntity::_shouldReturnToThrower()
{
    Entity* shooter = getShooter();
    if (shooter == nullptr || !shooter->isAlive()) {
        return false;
    }
    // 旁观者模式玩家不能接收返回的三叉戟
    Player* player = dynamic_cast<Player*>(shooter);
    if (player != nullptr && player->isSpectator()) {
        return false;
    }
    return true;
}

void TridentEntity::_tickReturning()
{
    Entity* shooter = getShooter();
    if (!shooter || !shooter->isAlive()) {
        // 射手已死亡或不存在，掉落物品
        if (!m_world->isClientSide() && pickupStatus() == PickupStatus::Allowed) {
            math::Random rng = createRandomFromEntity(*this);
            ItemDropHelper::spawnItemAtEntity(this, getArrowStack(), 0.1f, rng);
        }
        remove();
        return;
    }

    // 计算到射手眼部位置的方向
    Vector3 direction(shooter->x() - m_builtIn.stateVector->m_pos.x,
        shooter->y() + shooter->eyeHeight() - m_builtIn.stateVector->m_pos.y,
        shooter->z() - m_builtIn.stateVector->m_pos.z);
    f32 distance = direction.length();

    // 非玩家射手：当三叉戟足够近时掉落物品
    Player* player = dynamic_cast<Player*>(shooter);
    if (player == nullptr) {
        f32 threshold = shooter->width() + 1.0f;
        if (distance < threshold) {
            if (!m_world->isClientSide() && pickupStatus() == PickupStatus::Allowed) {
                math::Random rng = createRandomFromEntity(*this);
                ItemDropHelper::spawnItemAtEntity(this, getArrowStack(), 0.1f, rng);
            }
            remove();
            return;
        }
    }

    // 更新旋转朝向运动方向
    ProjectileHelper::rotateTowardsMovement(*this, 0.2f);

    auto* trident = tryGetComponent<ecs::TridentStateComponent>();
    const i32 loyalty = trident ? static_cast<i32>(trident->m_loyaltyLevel) : 0;

    // Y轴微小偏移
    m_builtIn.stateVector->m_pos.y += direction.y * 0.015f * static_cast<f32>(loyalty);

    // 返回速度
    f32 speed = 0.05f * static_cast<f32>(loyalty);

    // 设置速度：当前速度缩放 0.95 后加上朝向射手的方向
    Vector3 currentVel = m_builtIn.velocity->m_velocity;
    Vector3 normalizedDir = direction.normalized();
    m_builtIn.velocity->m_velocity = Vector3(currentVel.x * 0.95f + normalizedDir.x * speed,
        currentVel.y * 0.95f + normalizedDir.y * speed,
        currentVel.z * 0.95f + normalizedDir.z * speed);

    // 更新位置
    m_builtIn.stateVector->m_posPrev = m_builtIn.stateVector->m_pos;
    m_builtIn.stateVector->m_pos = m_builtIn.stateVector->m_pos + m_builtIn.velocity->m_velocity;

    // 玩家射手：检查是否到达射手，添加到背包（仅服务端）
    if (player != nullptr && distance < 2.0f) {
        if (!m_world->isClientSide()) {
            onPlayerPickup(*player);
        }
        return;
    }

    // 播放返回音效（首次）
    const i32 retTicks = trident ? trident->m_returningTicks : 0;
    if (retTicks == 0) {
        playSound(SoundEvents::ITEM_TRIDENT_RETURN, 10.0f, 1.0f);
    }
    if (trident != nullptr) {
        ++trident->m_returningTicks;
    }

    // 检查是否在水中，生成气泡粒子
    if (isInWater() && m_world) {
        for (int i = 0; i < 4; ++i) {
            f32 offset = 0.25f;
            Vector3 pos(x() - m_builtIn.velocity->m_velocity.x * offset,
                y() - m_builtIn.velocity->m_velocity.y * offset,
                z() - m_builtIn.velocity->m_velocity.z * offset);
            m_world->addParticle(particle::ParticleTypeId::Bubble, pos, m_builtIn.velocity->m_velocity);
        }
    }

    Entity::tick();
}

void TridentEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    Entity* target = result.hitEntity;

    auto* trident = tryGetComponent<ecs::TridentStateComponent>();
    const bool hasStack =
        (trident != nullptr && trident->m_tridentStack != nullptr && !trident->m_tridentStack->isEmpty());

    // 计算基础伤害
    f32 damage = 8.0f;

    // 应用穿刺附魔伤害（对水生生物造成额外伤害，每级 2.5 点）
    // 对齐 MC Java 1.21.11：穿刺目标判定用 EntityTypeTags.SENSITIVE_TO_IMPALING 标签
    // （Enchantments.java:994），在 ImpalingEnchantment::getDamageBonus 内查标签，
    // 故此处直接传 LivingEntity* 目标（不再转 CreatureAttribute 枚举）。
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);
    if (livingTarget != nullptr && hasStack) {
        // 使用附魔助手的 getTotalDamageBonus 方法计算额外伤害（穿刺查 SENSITIVE_TO_IMPALING 标签）
        damage += mc::item::enchant::EnchantmentHelper::getTotalDamageBonus(*trident->m_tridentStack, livingTarget);
    }

    // 获取射击者
    Entity* shooter = getShooter();

    // 创建伤害来源
    std::unique_ptr<DamageSource> damageSource;
    if (shooter != nullptr) {
        bool isPlayer = shooter->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Trident, shooter, this, isPlayer);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Trident, this, this, false);
    }

    // 标记已造成伤害
    setDealtDamage(true);

    // 攻击者记录"我打了谁"（对齐 vanilla AbstractArrow.onHitEntity:444 在 hurt 前无条件对 shooter
    // (LivingEntity) 调 setLastHurtMob(entity)；vanilla ThrownTrident 不重写 onHitEntity 故继承该调用）。
    // 字段由 OwnerHurtTargetGoal 消费（驯服动物帮主人攻击主人正在打的怪）。本 override 自管不调基类
    // onEntityHit，须显式补。
    if (shooter != nullptr) {
        LivingEntity* shooterLiving = dynamic_cast<LivingEntity*>(shooter);
        if (shooterLiving != nullptr && livingTarget != nullptr) {
            shooterLiving->setLastHurtTarget(livingTarget);
        }
    }

    // 应用伤害
    if (livingTarget != nullptr) {
        livingTarget->hurt(*damageSource, damage);
    }

    // 击退效果
    if (knockbackStrength() > 0) {
        f32 ratio = 0.6f * static_cast<f32>(knockbackStrength());
        Vector3 horizontalVel(m_builtIn.velocity->m_velocity.x, 0.0f, m_builtIn.velocity->m_velocity.z);
        if (horizontalVel.lengthSquared() > 0.0f) {
            horizontalVel = horizontalVel.normalized();
            Vector3 knockback(horizontalVel.x * ratio, 0.1f, horizontalVel.z * ratio);
            target->addVelocity(knockback);
        }
    }

    // 引雷附魔
    // 对齐 vanilla ThrownTrident.onHitEntity：hasChanneling(stack) && level.isThundering() &&
    // level.canSeeSky(targetPos) → 召唤 LightningBoltEntity。
    // 注：Level.isThundering() = getThunderLevel(1.0) > 0.9（强度阈值，非裸 thundering 标志），
    // 与 Cubium WeatherState::isThundering() 一致。/weather thunder 只设 thundering=true 不设强度，
    // 强度靠 WeatherManager::tick 每 tick 渐变 +0.01，故设雷暴后约 91 tick 才 isThundering()=true。
    // 此渐变延迟是 vanilla 真实行为（非缺陷），依赖引雷的测试须等待强度达标。
    if (m_world != nullptr && !m_world->isClientSide() && livingTarget != nullptr) {
        const bool hasChan = hasStack && mc::item::enchant::EnchantmentHelper::hasChanneling(*trident->m_tridentStack);
        if (hasChan) {
            const BlockPos targetPos(
                static_cast<i32>(target->x()), static_cast<i32>(target->y()), static_cast<i32>(target->z()));
            if (m_world->isThundering() && m_world->canSeeSky(targetPos)) {
                // 创建闪电实体
                // ECS 迁移：实体构造需要 registry 句柄（m_world 已判空，此处 registry 必非空）
                auto* registry = &ecsRegistry();
                if (registry == nullptr) {
                    return;
                }
                auto lightning = std::make_unique<entity::LightningBoltEntity>(*registry);
                lightning->setTypeId(EntityTypeKeys::LIGHTNING_BOLT);
                lightning->setPosition(target->x(), target->y(), target->z());

                // 设置触发者
                Player* playerShooter = dynamic_cast<Player*>(shooter);
                if (playerShooter != nullptr) {
                    lightning->setCaster(playerShooter->playerId());
                }

                // 生成闪电
                m_world->spawnEntity(std::move(lightning));

                // 播放引雷音效
                playSound(SoundEvents::ITEM_TRIDENT_THUNDER, 5.0f, 1.0f);
            }
        }
    }

    // 速度反转为轻微反弹
    m_builtIn.velocity->m_velocity = Vector3(m_builtIn.velocity->m_velocity.x * -0.01f,
        m_builtIn.velocity->m_velocity.y * -0.1f,
        m_builtIn.velocity->m_velocity.z * -0.01f);

    // 三叉戟不移除，而是等待返回
    // 如果没有忠诚附魔，会进入 m_inGround 状态
}

void TridentEntity::onBlockHit(const RayTraceResult& result)
{
    setInGround(true);
    auto* trident = tryGetComponent<ecs::TridentStateComponent>();
    if (trident != nullptr) {
        trident->m_hitBlock = true;
        trident->m_hitBlockPos = result.blockPos;
    }

    auto* arrowState = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    // 保存方块状态
    if (arrowState != nullptr && m_world && result.type == RayTraceResultType::Block) {
        const BlockState* state = m_world->getBlockState(result.blockPos.x, result.blockPos.y, result.blockPos.z);
        if (state != nullptr) {
            *arrowState->m_inBlockState = *state;
        }
    }

    // 清除暴击和穿透状态
    setCritical(false);
    setPierceLevel(0);
    clearPiercedEntities();

    // 三叉戟有特殊的命中地面音效
    playSound(SoundEvents::ITEM_TRIDENT_HIT_GROUND, 1.0f, 1.0f);
}

f32 TridentEntity::getWaterDrag() const
{
    // 三叉戟在水中阻力很小
    return 0.99f;
}

void TridentEntity::setBaseDamageFromMob(f32 power)
{
    // 三叉戟不使用弓类附魔（力量/冲击/火焰），因此不重写 applyBowEnchantments。
    // 三叉戟的专属附魔已在其他地方正确处理：
    // - 忠诚附魔：setItemStack() 中获取忠诚等级
    // - 穿刺附魔：onEntityHit() 中对水生生物造成额外伤害
    // - 引雷附魔：onEntityHit() 中召唤闪电
    // - 激流附魔：TridentItem::onPlayerStoppedUsing() 中处理冲刺
    //
    // 基础伤害计算与箭矢相同公式：power * 2.0 + triangle(difficulty * 0.11, 0.57425)
    math::Random rng = createRandomFromEntity(*this);
    f32 difficultyBonus = m_world ? static_cast<f32>(static_cast<u8>(m_world->difficulty())) * 0.11f : 0.0f;
    f32 triangle = difficultyBonus + (rng.nextFloat() - rng.nextFloat()) * 0.57425f;
    setDamage(power * 2.0f + triangle);
}

void TridentEntity::setItemStack(const ItemStack& stack)
{
    auto* trident = tryGetComponent<ecs::TridentStateComponent>();
    if (trident == nullptr || trident->m_tridentStack == nullptr) {
        return;
    }
    *trident->m_tridentStack = stack;
    // 从物品堆获取忠诚附魔等级
    trident->m_loyaltyLevel =
        static_cast<u8>(mc::item::enchant::EnchantmentHelper::getEnchantmentLevel(stack, "minecraft:loyalty"));
    // 批次6 子目标2 Step5：镜像同步 DATA_LOYALTY / DATA_FOIL（对齐 vanilla ThrownTrident）。
    // foil = 物品是否有任意附魔光泽（vanilla ItemStack.hasFoil 语义）。
    m_dataManager.set(DATA_LOYALTY_PARAM, static_cast<i8>(trident->m_loyaltyLevel));
    m_dataManager.set(DATA_FOIL_PARAM, mc::item::enchant::EnchantmentHelper::hasEnchantments(stack));
}

bool TridentEntity::onPlayerPickup(Player& player)
{
    // 必须在服务端执行
    if (m_world->isClientSide()) {
        return false;
    }

    // 只有当三叉戟在地上或返回时才能被拾取
    if (!isInGround() && !noClip()) {
        return false;
    }

    // 箭矢不能处于抖动状态
    if (arrowShake() > 0) {
        return false;
    }

    // 检查拾取权限
    bool canPickup = (pickupStatus() == PickupStatus::Allowed) ||
        (pickupStatus() == PickupStatus::CreativeOnly && player.isCreative()) ||
        (noClip() && getShooter() != nullptr && getShooter()->uuid() == player.uuid());

    if (!canPickup) {
        return false;
    }

    auto* trident = tryGetComponent<ecs::TridentStateComponent>();
    const bool hasStack =
        (trident != nullptr && trident->m_tridentStack != nullptr && !trident->m_tridentStack->isEmpty());

    // 只有 Allowed 状态才检查背包空间
    // 三叉戟的 pickupStatus 通常是 Allowed，所以总是检查背包
    if (hasStack) {
        i32 added = player.inventory().add(*trident->m_tridentStack);
        // 三叉戟只能有一个，检查是否成功添加
        if (trident->m_tridentStack->getCount() > 0) {
            return false; // 背包满了
        }
    }

    // 播放拾取音效
    if (m_world) {
        playSound(SoundEvents::ENTITY_ITEM_PICKUP, 0.2f, 1.0f);
    }

    remove();
    return true;
}

void TridentEntity::tickInGroundTrident()
{
    auto* trident = tryGetComponent<ecs::TridentStateComponent>();
    const i32 loyalty = trident ? static_cast<i32>(trident->m_loyaltyLevel) : 0;
    // 如果不允许拾取或没有忠诚附魔，则使用普通超时逻辑
    if (pickupStatus() != PickupStatus::Allowed || loyalty <= 0) {
        AbstractArrowEntity::tickInGround();
    }
    // 否则不超时，等待返回
}

// 批次6 子目标2 Step4：以下 getter/setter 经 ecs::TridentStateComponent 读写。
ItemStack TridentEntity::getItemStack() const
{
    const auto* c = tryGetComponent<ecs::TridentStateComponent>();
    return (c != nullptr && c->m_tridentStack != nullptr) ? *c->m_tridentStack : ItemStack();
}

ItemStack TridentEntity::getArrowStack() const
{
    const auto* c = tryGetComponent<ecs::TridentStateComponent>();
    return (c != nullptr && c->m_tridentStack != nullptr) ? c->m_tridentStack->copy() : ItemStack();
}

bool TridentEntity::isReturning() const
{
    const auto* c = tryGetComponent<ecs::TridentStateComponent>();
    return (c != nullptr) ? c->m_returning : false;
}

void TridentEntity::setReturning(bool returning)
{
    auto* c = tryGetComponent<ecs::TridentStateComponent>();
    if (c != nullptr) {
        c->m_returning = returning;
    }
}

bool TridentEntity::hasHitBlock() const
{
    const auto* c = tryGetComponent<ecs::TridentStateComponent>();
    return (c != nullptr) ? c->m_hitBlock : false;
}

BlockPos TridentEntity::hitBlockPos() const
{
    const auto* c = tryGetComponent<ecs::TridentStateComponent>();
    return (c != nullptr) ? c->m_hitBlockPos : BlockPos{};
}

u8 TridentEntity::loyaltyLevel() const
{
    const auto* c = tryGetComponent<ecs::TridentStateComponent>();
    return (c != nullptr) ? c->m_loyaltyLevel : 0;
}

void TridentEntity::setLoyaltyLevel(u8 level)
{
    auto* c = tryGetComponent<ecs::TridentStateComponent>();
    if (c != nullptr) {
        c->m_loyaltyLevel = level;
    }
    // 批次6 子目标2 Step5：镜像同步 DATA_LOYALTY（对齐 vanilla ThrownTrident）。
    m_dataManager.set(DATA_LOYALTY_PARAM, static_cast<i8>(level));
}

i32 TridentEntity::returningTicks() const
{
    const auto* c = tryGetComponent<ecs::TridentStateComponent>();
    return (c != nullptr) ? c->m_returningTicks : 0;
}

void TridentEntity::setReturningTicks(i32 ticks)
{
    auto* c = tryGetComponent<ecs::TridentStateComponent>();
    if (c != nullptr) {
        c->m_returningTicks = ticks;
    }
}

} // namespace entity
} // namespace mc
