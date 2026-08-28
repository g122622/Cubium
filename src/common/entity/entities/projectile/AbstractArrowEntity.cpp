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

#include "AbstractArrowEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/ArrowEffectsComponent.hpp"
#include "common/entity/ecs/components/ProjectileArrowStateComponent.hpp"
#include "common/entity/ecs/components/SpectralArrowComponent.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileHelper.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

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

// ============================================================================
// AbstractArrowEntity
// ============================================================================

// 静态数据参数定义（对应 MC 1.21.11 AbstractArrow.defineSynchedData()）。
// 在静态初始化阶段通过 EntityDataManager::createKey<T>() 分配全局唯一 ID。
// 真实 id 在 registerData() 内由 ClassRegisterGuard 沿继承链续接分配（Entity 8 字段后 → id8/9/10）。
entity::DataParameter<i8> AbstractArrowEntity::DATA_ARROW_FLAGS_PARAM = entity::EntityDataManager::createKey<i8>();
entity::DataParameter<i8> AbstractArrowEntity::DATA_PIERCE_LEVEL_PARAM = entity::EntityDataManager::createKey<i8>();
entity::DataParameter<bool> AbstractArrowEntity::DATA_IN_GROUND_PARAM = entity::EntityDataManager::createKey<bool>();

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = ProjectileEntity::classInfo()）。
// 子类（TridentEntity）的 registerData 首行调 AbstractArrowEntity::registerData()，
// 其 ClassRegisterGuard 沿父链穿过本节点续接 id。
const EntityClassInfo& AbstractArrowEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"AbstractArrowEntity", &ProjectileEntity::classInfo()};
    return s_classInfo;
}

void AbstractArrowEntity::registerData()
{
    // 先调用基类注册基础参数（FLAGS/AIR/CUSTOM_NAME 等，id0..7）
    ProjectileEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册箭矢专属同步参数（id8/9/10，对应 vanilla AbstractArrow）
    // DATA_ARROW_FLAGS：bit0=critical, bit2=shotFromCrossbow（vanilla getFlags/setFlags 编码）
    m_dataManager.registerParam(DATA_ARROW_FLAGS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(DATA_PIERCE_LEVEL_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(DATA_IN_GROUND_PARAM, false);
}

void AbstractArrowEntity::_syncArrowFlags()
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c == nullptr) {
        return;
    }
    // vanilla AbstractArrow.getFlags(): bit0=critical, bit2=shotFromCrossbow
    i8 flags = static_cast<i8>((c->m_critical ? 1 : 0) | (c->m_shotFromCrossbow ? 4 : 0));
    m_dataManager.set(DATA_ARROW_FLAGS_PARAM, flags);
}

AbstractArrowEntity::AbstractArrowEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileEntity(id, registry)
{
    // 批次6 子目标2 Step1：attach ProjectileArrowStateComponent（箭矢 13 字段状态）。
    // ArrowEntity/SpectralArrowEntity/TridentEntity/SpearEntity 子类经此自动获得组件。
    // Step3 已把 m_damage/m_critical/m_pierceLevel/m_inGround 等 13 字段读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::ProjectileArrowStateComponent>(m_entityContext->entity());
    // C++ 虚函数在构造函数中不会派生到子类，故 Entity 基类构造调用的 registerData()
    // 只执行 ProjectileEntity 链。子类必须在此显式调用本类 registerData() 注册箭矢同步字段。
    // 注意：子类（TridentEntity）构造会先调本构造函数，再调自身 registerData()——本调用
    // 注册 id8/9/10，Trident 后续 registerData 续接 id11/12，无重复（PARAM 静态幂等）。
    registerData();
}

// 批次6 子目标2 Step3：以下 getter/setter 经 ProjectileArrowStateComponent 读写。
f32 AbstractArrowEntity::damage() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_damage : 0.0f;
}

void AbstractArrowEntity::setDamage(f32 damage)
{
    auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c != nullptr) {
        c->m_damage = damage;
    }
}

i32 AbstractArrowEntity::knockbackStrength() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_knockbackStrength : 0;
}

void AbstractArrowEntity::setKnockbackStrength(i32 strength)
{
    auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c != nullptr) {
        c->m_knockbackStrength = strength;
    }
}

bool AbstractArrowEntity::isCritical() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_critical : false;
}

void AbstractArrowEntity::setCritical(bool critical)
{
    auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c != nullptr) {
        c->m_critical = critical;
    }
    // 同步 DATA_ARROW_FLAGS 镜像（bit0=critical）
    _syncArrowFlags();
}

u8 AbstractArrowEntity::pierceLevel() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_pierceLevel : 0;
}

void AbstractArrowEntity::setPierceLevel(u8 level)
{
    auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c != nullptr) {
        c->m_pierceLevel = level;
    }
    // 同步 DATA_PIERCE_LEVEL 镜像
    m_dataManager.set(DATA_PIERCE_LEVEL_PARAM, static_cast<i8>(level));
}

bool AbstractArrowEntity::isInGround() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_inGround : false;
}

void AbstractArrowEntity::setInGround(bool inGround)
{
    auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c != nullptr) {
        c->m_inGround = inGround;
    }
    // 同步 DATA_IN_GROUND 镜像
    m_dataManager.set(DATA_IN_GROUND_PARAM, inGround);
}

PickupStatus AbstractArrowEntity::pickupStatus() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_pickupStatus : PickupStatus::Disallowed;
}

void AbstractArrowEntity::setPickupStatus(PickupStatus status)
{
    auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c != nullptr) {
        c->m_pickupStatus = status;
    }
}

bool AbstractArrowEntity::shotFromCrossbow() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_shotFromCrossbow : false;
}

void AbstractArrowEntity::setShotFromCrossbow(bool fromCrossbow)
{
    auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c != nullptr) {
        c->m_shotFromCrossbow = fromCrossbow;
    }
    // 同步 DATA_ARROW_FLAGS 镜像（bit2=shotFromCrossbow）
    _syncArrowFlags();
}

bool AbstractArrowEntity::hasDealtDamage() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_dealtDamage : false;
}

void AbstractArrowEntity::setDealtDamage(bool dealt)
{
    auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c != nullptr) {
        c->m_dealtDamage = dealt;
    }
}

i32 AbstractArrowEntity::timeInGround() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_timeInGround : 0;
}

i32 AbstractArrowEntity::arrowShake() const
{
    const auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    return (c != nullptr) ? c->m_arrowShake : 0;
}

void AbstractArrowEntity::tick()
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    MC_ASSERT_RELEASE(comp != nullptr);

    // 检查是否已离开发射者
    tryUpdateLeftShooter();

    // 如果插在方块中，执行不同的tick逻辑
    if (comp->m_inGround) {
        tickInGround();
        return;
    }

    // 检查抖动
    if (comp->m_arrowShake > 0) {
        --comp->m_arrowShake;
    }

    // 如果在水中，灭火并生成气泡粒子
    if (isInWater()) {
        clearFire();
        // 水中生成气泡粒子尾迹
        if (m_world) {
            for (int j = 0; j < 4; ++j) {
                f32 offset = 0.25f;
                Vector3 pos(x() - m_builtIn.velocity->m_velocity.x * offset,
                    y() - m_builtIn.velocity->m_velocity.y * offset,
                    z() - m_builtIn.velocity->m_velocity.z * offset);
                m_world->addParticle(particle::ParticleTypeId::Bubble, pos, m_builtIn.velocity->m_velocity);
            }
        }
    }

    // ========== 检查是否在方块内 ==========
    BlockPos currentPos = BlockPos(static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.y)),
        static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.z)));
    if (m_world) {
        const BlockState* blockState = m_world->getBlockState(currentPos.x, currentPos.y, currentPos.z);
        // 检查是否在非空气方块的碰撞箱内
        if (blockState != nullptr && !blockState->isAir()) {
            // 获取方块的碰撞形状
            const CollisionShape& collisionShape = blockState->getCollisionShape();

            // 如果碰撞形状不为空，检查箭矢位置是否在碰撞箱内
            if (!collisionShape.isEmpty()) {
                // 获取世界坐标下的碰撞箱列表
                std::vector<AxisAlignedBB> worldBoxes =
                    collisionShape.getWorldBoxes(currentPos.x, currentPos.y, currentPos.z);

                // 检查箭矢位置是否在任意碰撞箱内
                for (const AxisAlignedBB& box : worldBoxes) {
                    if (box.contains(m_builtIn.stateVector->m_pos)) {
                        comp->m_inGround = true;
                        *comp->m_inBlockState = *blockState;
                        break;
                    }
                }
            }
        }
    }

    // 调用父类tick进行射线追踪和移动
    ProjectileEntity::tick();

    // 暴击粒子效果
    if (comp->m_critical && !comp->m_inGround && m_world) {
        math::Random rng = createRandomFromEntity(*this);
        // 每tick有概率生成暴击粒子
        if (rng.nextInt(3) == 0) {
            f32 ox = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;
            f32 oy = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;
            f32 oz = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;

            Vector3 pos(x() + ox, y() + oy, z() + oz);
            // 粒子速度与箭矢速度相反
            Vector3 vel(-velocityX() * 0.01f, -velocityY() * 0.01f, -velocityZ() * 0.01f);

            m_world->addParticle(particle::ParticleTypeId::Crit, pos, vel);
        }
    }
}

void AbstractArrowEntity::tickInGround()
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return;
    }

    // 检查方块是否仍然存在
    if (m_world) {
        const BlockState* currentBlock =
            m_world->getBlockState(static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.x)),
                static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.y)),
                static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.z)));

        // 检查方块变更导致箭矢脱落
        if (currentBlock != nullptr && comp->m_inBlockState->has_value() && *currentBlock != **comp->m_inBlockState &&
            checkInBlockEmpty()) {
            detachFromBlock();
            return;
        }
    }

    ++comp->m_ticksInGround;
    ++comp->m_timeInGround;

    // 超时移除（1200 ticks = 60秒）
    if (comp->m_ticksInGround >= 1200) {
        remove();
    }
}

bool AbstractArrowEntity::checkInBlockEmpty()
{
    // 检查箭矢周围是否有碰撞箱
    // 创建一个很小的检测盒（0.06）
    AxisAlignedBB testBox(m_builtIn.stateVector->m_pos.x - 0.06f,
        m_builtIn.stateVector->m_pos.y - 0.06f,
        m_builtIn.stateVector->m_pos.z - 0.06f,
        m_builtIn.stateVector->m_pos.x + 0.06f,
        m_builtIn.stateVector->m_pos.y + 0.06f,
        m_builtIn.stateVector->m_pos.z + 0.06f);

    if (m_world) {
        return m_world->hasNoCollisions(testBox);
    }
    return true;
}

void AbstractArrowEntity::detachFromBlock()
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return;
    }
    comp->m_inGround = false;

    // 随机弹射
    math::Random rng = createRandomFromEntity(*this);
    f32 randX = rng.nextFloat() * 0.2f;
    f32 randY = rng.nextFloat() * 0.2f;
    f32 randZ = rng.nextFloat() * 0.2f;
    m_builtIn.velocity->m_velocity = Vector3(randX, randY, randZ);

    comp->m_ticksInGround = 0;
    comp->m_timeInGround = 0;
}

bool AbstractArrowEntity::shouldDespawn()
{
    // 由 tickInGround 处理
    return false;
}

void AbstractArrowEntity::clearPiercedEntities()
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp != nullptr && comp->m_piercedEntities != nullptr) {
        comp->m_piercedEntities->clear();
    }
}

RayTraceResult AbstractArrowEntity::rayTraceEntities(const Vector3& start, const Vector3& end)
{
    // 使用父类实现，但使用穿透过滤
    if (m_world == nullptr) {
        return RayTraceResult::miss();
    }

    const AxisAlignedBB searchBox = ProjectileHelper::createMovementSearchBox(*this, end - start, 1.0f);

    return ProjectileHelper::rayTraceEntities(*m_world, *this, start, end, searchBox, [this](const Entity& candidate) {
        return canHitEntityWithPierce(candidate);
    });
}

bool AbstractArrowEntity::canHitEntityWithPierce(const mc::Entity& target) const
{
    const auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return canHitEntity(target);
    }

    // 基础检查
    if (!canHitEntity(target)) {
        return false;
    }

    // 检查是否已穿透过此实体
    if (comp->m_pierceLevel > 0 && comp->m_piercedEntities != nullptr &&
        comp->m_piercedEntities->count(target.id()) > 0) {
        return false;
    }

    return true;
}

void AbstractArrowEntity::onEntityHit(const RayTraceResult& result)
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return;
    }

    if (!result.hitEntity) {
        return;
    }

    mc::Entity* target = result.hitEntity;

    // 计算伤害
    f32 speed = std::sqrt(m_builtIn.velocity->m_velocity.x * m_builtIn.velocity->m_velocity.x +
        m_builtIn.velocity->m_velocity.y * m_builtIn.velocity->m_velocity.y +
        m_builtIn.velocity->m_velocity.z * m_builtIn.velocity->m_velocity.z);
    // 计算伤害。对齐 vanilla AbstractArrow.onHitEntity:420 Mth.ceil(Mth.clamp(f * d0, 0, 2.147E9))：
    // f = 速度向量长度（speed），d0 = baseDamage（已含 Power 加成）。vanilla 用 ceil（向上取整），
    // 此前用 static_cast<i32>（截断）致 speed 非整数倍时少算 1 伤害（如 speed=2.7,base=2.0 →
    // vanilla ceil(5.4)=6，截断 i32(5.4)=5）。改用 std::ceil 对齐。
    i32 damage = static_cast<i32>(std::ceil(std::clamp(static_cast<f64>(speed * comp->m_damage), 0.0, 2147483647.0)));

    // 暴击伤害加成
    if (comp->m_critical) {
        mc::math::Random rng = createRandomFromEntity(*this);
        i32 bonus = rng.nextInt(damage / 2 + 2);
        damage = static_cast<i32>(std::min(static_cast<i64>(damage) + bonus, static_cast<i64>(2147483647)));
    }

    // 穿透检查
    if (comp->m_pierceLevel > 0) {
        if (comp->m_piercedEntities != nullptr &&
            static_cast<i32>(comp->m_piercedEntities->size()) >= comp->m_pierceLevel + 1) {
            // 达到穿透上限，移除箭矢
            remove();
            return;
        }
        comp->m_piercedEntities->insert(target->id());
    }

    // 获取发射者
    mc::Entity* shooter = getShooter();

    // 创建伤害来源：对齐 vanilla arrow 伤害类型 + setProjectile（对齐 DamageSources::arrow 工厂语义）。
    // 此前手动 make_unique<IndirectEntityDamageSource>(Arrow, ...) 漏调 setProjectile，致
    // isProjectile()（旧实现只查 m_isProjectile 标志）返 false，applyPotionDamageCalculations 不设
    // DamageFlags::PROJECTILE 位，弹射物保护附魔 getDamageProtection(Projectile) 返 0、EPF 减伤失效。
    // setProjectile() 设 m_isProjectile=true 保底；isProjectile() 现亦查 IS_PROJECTILE 标签
    // （Arrow 是成员）双保险。shooter 为空时 source/directSource 均为 this（箭矢本身，与旧无 shooter
    // 分支语义一致）。isPlayer 由 shooter 是否为玩家决定。
    bool isPlayer = shooter != nullptr && shooter->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
    mc::Entity* sourceForArrow = (shooter != nullptr) ? shooter : this;
    auto indirectSource =
        std::make_unique<IndirectEntityDamageSource>(DamageType::Arrow, sourceForArrow, this, isPlayer);
    indirectSource->setProjectile();
    std::unique_ptr<DamageSource> damageSource = std::move(indirectSource);

    // 应用伤害并增加箭矢计数
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);

    // 攻击者记录"我打了谁"（对齐 vanilla AbstractArrow.onHitEntity:444 在 hurt 前无条件对
    // shooter(LivingEntity) 调 setLastHurtMob(entity)）。该记录是"攻击意图"语义——即使 hurt 被无敌帧/
    // 免疫吞也记录，与 setLastHurtBy 的 NO_ANGER 门控不同（后者 target 侧记录"谁打了我"，有门控）。
    // 字段由 OwnerHurtTargetGoal 消费（驯服动物帮主人攻击主人正在打的怪）。
    // 此前仅 Player 近战记录，箭矢不记录致玩家用弓射怪时驯服动物（狼）不帮忙。基类补一处即覆盖
    // ArrowEntity/SpectralArrowEntity（两者不重写 onEntityHit）。Trident/Spear override 不调基类 onEntityHit，各自补。
    if (shooter != nullptr) {
        LivingEntity* shooterLiving = dynamic_cast<LivingEntity*>(shooter);
        if (shooterLiving != nullptr && livingTarget != nullptr) {
            shooterLiving->setLastHurtTarget(livingTarget);
        }
    }

    if (livingTarget != nullptr) {
        bool hurt = livingTarget->hurt(*damageSource, static_cast<f32>(damage));
        // 只有非穿透箭在造成伤害后才增加箭矢计数
        if (hurt && comp->m_pierceLevel <= 0) {
            livingTarget->setArrowCountInEntity(livingTarget->getArrowCount() + 1);
        }
        // 对齐 vanilla AbstractArrow.onHitEntity:453-468：hurtOrSimulate 成功后才调
        // doPostHurtEffects。hurt 失败（无敌帧 amount<=lastDamage 返回 false /
        // isInvulnerableTo 拦截）则不施加药水箭药水效果、光灵箭发光等后置效果。
        // 此前 Cubium 子类 onEntityHit 无条件施加，被无敌帧吞掉的箭矢仍施加效果，偏离 vanilla。
        if (hurt) {
            doPostHurtEffects(*livingTarget);
        }
    }

    // 击退效果
    if (comp->m_knockbackStrength > 0) {
        f32 ratio = 0.6f * static_cast<f32>(comp->m_knockbackStrength);
        Vector3 horizontalVel(m_builtIn.velocity->m_velocity.x, 0.0f, m_builtIn.velocity->m_velocity.z);
        if (horizontalVel.lengthSquared() > 0.0f) {
            horizontalVel = horizontalVel.normalized();
            Vector3 knockback(horizontalVel.x * ratio, 0.1f, horizontalVel.z * ratio);
            target->addVelocity(knockback);
        }
    }

    // 火焰伤害
    if (isOnFire()) {
        target->igniteForSeconds(5.0f);
    }

    // 播放命中音效
    math::Random rng = createRandomFromEntity(*this);
    playSound(SoundEvents::ENTITY_ARROW_HIT, 1.0f, 1.2f / (rng.nextFloat() * 0.2f + 0.9f));

    // 如果不是穿透箭，移除
    if (comp->m_pierceLevel <= 0) {
        remove();
    }
}

void AbstractArrowEntity::onBlockHit(const RayTraceResult& result)
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return;
    }
    comp->m_inGround = true;

    // 保存命中的方块状态
    if (m_world && result.type == RayTraceResultType::Block) {
        const BlockState* state = m_world->getBlockState(result.blockPos.x, result.blockPos.y, result.blockPos.z);
        if (state != nullptr) {
            *comp->m_inBlockState = *state;
        }
    }

    // 计算并设置箭矢位置（回退一点使其嵌入方块）
    Vector3 hitVec = result.hitPosition;
    Vector3 hitOffset = hitVec - m_builtIn.stateVector->m_pos;
    m_builtIn.velocity->m_velocity = hitOffset;
    Vector3 normalizedOffset = hitOffset.normalized() * 0.05f;
    m_builtIn.stateVector->m_pos = m_builtIn.stateVector->m_pos - normalizedOffset;

    comp->m_arrowShake = 7;

    // 清除暴击和穿透状态
    comp->m_critical = false;
    comp->m_pierceLevel = 0;
    clearPiercedEntities();

    // 播放命中地面音效
    math::Random rng = createRandomFromEntity(*this);
    playSound(SoundEvents::ENTITY_ARROW_HIT_GROUND, 1.0f, 1.2f / (rng.nextFloat() * 0.2f + 0.9f));
}

void AbstractArrowEntity::doPostHurtEffects(LivingEntity& /*target*/)
{
    // 基类默认空实现（对齐 vanilla AbstractArrow.doPostHurtEffects:567-568）。
    // 子类 ArrowEntity/SpectralArrowEntity 重写以施加药水/发光效果。
    // 由 onEntityHit 在 hurt 成功后调用，hurt 失败不调用。
}

void AbstractArrowEntity::setBaseDamageFromMob(f32 power)
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return;
    }
    // 公式：power * 2.0 + triangle(difficulty * 0.11, 0.57425)
    math::Random rng = createRandomFromEntity(*this);
    f32 difficultyBonus = m_world ? static_cast<f32>(static_cast<u8>(m_world->difficulty())) * 0.11f : 0.0f;
    f32 triangle = difficultyBonus + (rng.nextFloat() - rng.nextFloat()) * 0.57425f;
    comp->m_damage = power * 2.0f + triangle;
}

void AbstractArrowEntity::applyBowEnchantments(LivingEntity& shooter)
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return;
    }
    // 力量附魔增加伤害（PowerEnchantment: 每级 +0.5 伤害 + 基础 0.5）
    i32 powerLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        shooter.getMainHandItem(), &item::enchant::AllEnchantments::POWER);
    if (powerLevel > 0) {
        comp->m_damage += static_cast<f32>(powerLevel) * 0.5f + 0.5f;
    }

    // 冲击附魔增加击退（PunchEnchantment: 每级增加 1 点击退强度）
    i32 punchLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        shooter.getMainHandItem(), &item::enchant::AllEnchantments::PUNCH);
    if (punchLevel > 0) {
        comp->m_knockbackStrength = punchLevel;
    }

    // 火焰附魔：设置箭矢着火 100 ticks（5 秒），命中时点燃目标
    if (item::enchant::EnchantmentHelper::getEnchantmentLevel(
            shooter.getMainHandItem(), &item::enchant::AllEnchantments::FLAME) > 0) {
        igniteForTicks(100);
    }
}

void AbstractArrowEntity::onCollideWithPlayer(Player& player)
{
    const auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return;
    }
    // 只在服务端执行，检查拾取条件
    if (m_world && m_world->isClientSide()) {
        return;
    }

    // 检查是否可以拾取：必须插在方块中或处于穿甲状态，且不在抖动
    if ((!comp->m_inGround && !m_noClip) || comp->m_arrowShake > 0) {
        return;
    }

    // 调用拾取逻辑
    onPlayerPickup(player);
}

bool AbstractArrowEntity::onPlayerPickup(Player& player)
{
    auto* comp = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (comp == nullptr) {
        return false;
    }
    // 必须在服务端执行
    if (m_world && m_world->isClientSide()) {
        return false;
    }

    // 必须插在方块中或者是穿甲箭（noClip 状态）
    if (!comp->m_inGround && !m_noClip) {
        return false;
    }

    // 箭矢不能处于抖动状态
    if (comp->m_arrowShake > 0) {
        return false;
    }

    // 检查拾取权限
    bool canPickup = false;
    if (comp->m_pickupStatus == PickupStatus::Allowed) {
        canPickup = true;
    } else if (comp->m_pickupStatus == PickupStatus::CreativeOnly && player.isCreative()) {
        canPickup = true;
    } else if (m_noClip && getShooter() != nullptr && getShooter()->uuid() == player.uuid()) {
        // 穿甲箭且是自己射出的（忠诚附魔返回的三叉戟）
        canPickup = true;
    }

    if (!canPickup) {
        return false;
    }

    // 只有 Allowed 状态才检查背包空间
    if (comp->m_pickupStatus == PickupStatus::Allowed) {
        // 获取箭矢物品堆
        ItemStack arrowStack = getArrowStack();

        // 尝试添加到玩家背包
        // add() 方法会修改 arrowStack，减少其数量
        player.inventory().add(arrowStack);

        // 如果背包满了，添加失败
        if (arrowStack.getCount() > 0) {
            return false;
        }
    }

    // 播放拾取音效
    if (m_world) {
        math::Random rng = createRandomFromEntity(*this);
        m_world->playSound(SoundEvents::ENTITY_ITEM_PICKUP,
            sound::SoundCategory::Players,
            m_builtIn.stateVector->m_pos,
            0.2f,                                  // 音量
            1.0f + (rng.nextFloat() - 0.5f) * 0.2f // 音调带随机变化
        );
    }

    // 移除箭矢实体
    remove();
    return true;
}

// ============================================================================
// ArrowEntity
// ============================================================================

ArrowEntity::ArrowEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractArrowEntity(id, registry)
{
    setDamage(2.0f);
    // 批次6 子目标2 Step1：attach ArrowEffectsComponent（药水箭颜色/发光/效果列表）。
    // 普通弓箭不挂此组件，仅药水箭（Tipped Arrow）实例化 ArrowEntity 后由 setEffects
    // 填充。Step4 将把 m_color/m_glowing/m_effects 读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::ArrowEffectsComponent>(m_entityContext->entity());
}

std::unique_ptr<Entity> ArrowEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ArrowEntity>(0, registry);
}

std::unique_ptr<ArrowEntity> ArrowEntity::createFromShooter(LivingEntity& shooter, IWorld* world)
{
    // ECS 迁移：实体构造需要 registry 句柄。静态方法无 this，从 shooter 自持的 ECS registry
    // 取句柄（Entity 构造时绑定，ecsRegistry() 返回引用必非空），避免解引用可能为空的 world
    // 指针。生产调用者（ArrowItem/AbstractSkeletonEntity 等）传非空 world，测试可能传 nullptr，
    // 两者均能正确构造箭矢。world 仍传给 setWorld 供箭矢后续 tick/碰撞使用（可为 nullptr）。
    auto& registry = shooter.ecsRegistry();

    auto arrow = std::make_unique<ArrowEntity>(0, registry);
    arrow->setTypeId(EntityTypeKeys::ARROW);
    arrow->setWorld(world);
    arrow->setPosition(shooter.x(), shooter.y() + shooter.eyeHeight() - 0.1f, shooter.z());
    arrow->setShooter(&shooter);

    // 玩家射出的箭默认允许拾取，非玩家射出的箭默认不允许拾取
    Player* player = dynamic_cast<Player*>(&shooter);
    if (player != nullptr) {
        arrow->setPickupStatus(PickupStatus::Allowed);
    }

    return arrow;
}

// 批次6 子目标2 Step4：ArrowEntity 3 字段（m_color/m_glowing/m_effects）迁入
// ecs::ArrowEffectsComponent，以下 getter/setter 经组件读写。
void ArrowEntity::setColor(u32 color)
{
    auto* c = tryGetComponent<ecs::ArrowEffectsComponent>();
    if (c != nullptr) {
        c->m_color = color;
    }
}

u32 ArrowEntity::color() const
{
    const auto* c = tryGetComponent<ecs::ArrowEffectsComponent>();
    return (c != nullptr) ? c->m_color : 0xFFFFFFFF;
}

void ArrowEntity::setGlowing(bool glowing)
{
    auto* c = tryGetComponent<ecs::ArrowEffectsComponent>();
    if (c != nullptr) {
        c->m_glowing = glowing;
    }
}

bool ArrowEntity::isGlowing() const
{
    const auto* c = tryGetComponent<ecs::ArrowEffectsComponent>();
    return (c != nullptr) ? c->m_glowing : false;
}

void ArrowEntity::addEffect(const entity::effect::EffectInstance& effect)
{
    auto* c = tryGetComponent<ecs::ArrowEffectsComponent>();
    if (c != nullptr && c->m_effects != nullptr) {
        c->m_effects->push_back(effect);
    }
}

void ArrowEntity::setEffects(const std::vector<entity::effect::EffectInstance>& effects)
{
    auto* c = tryGetComponent<ecs::ArrowEffectsComponent>();
    if (c != nullptr && c->m_effects != nullptr) {
        *c->m_effects = effects;
    }
}

const std::vector<entity::effect::EffectInstance>& ArrowEntity::effects() const
{
    // nullptr 兜底：返回静态空 vector 引用，避免悬垂。
    static const std::vector<entity::effect::EffectInstance> s_empty;
    const auto* c = tryGetComponent<ecs::ArrowEffectsComponent>();
    return (c != nullptr && c->m_effects != nullptr) ? *c->m_effects : s_empty;
}

bool ArrowEntity::hasEffects() const
{
    const auto* c = tryGetComponent<ecs::ArrowEffectsComponent>();
    return (c != nullptr && c->m_effects != nullptr) ? !c->m_effects->empty() : false;
}

void ArrowEntity::tick()
{
    AbstractArrowEntity::tick();

    // 药水箭的粒子效果处理
    const u32 arrowColor = color();
    if (arrowColor != 0xFFFFFFFF && !isInGround() && m_world && m_world->isClientSide()) {
        // 将 ARGB 颜色转换为 RGB 分量 (0.0-1.0 范围)
        // 使用 EntityEffect 粒子，速度参数作为颜色传递
        f32 r = static_cast<f32>((arrowColor >> 16) & 0xFF) / 255.0f;
        f32 g = static_cast<f32>((arrowColor >> 8) & 0xFF) / 255.0f;
        f32 b = static_cast<f32>(arrowColor & 0xFF) / 255.0f;

        // 飞行中每 tick 生成 2 个粒子
        math::Random rng = createRandomFromEntity(*this);
        for (int i = 0; i < 2; ++i) {
            // 粒子位置在箭矢周围随机偏移
            f32 ox = (rng.nextFloat() - 0.5f) * width();
            f32 oy = rng.nextFloat() * height();
            f32 oz = (rng.nextFloat() - 0.5f) * width();

            Vector3 pos(x() + ox, y() + oy, z() + oz);
            Vector3 colorVel(r, g, b); // 颜色作为速度参数传递

            m_world->addParticle(particle::ParticleTypeId::EntityEffect, pos, colorVel);
        }
    }
}

void ArrowEntity::doPostHurtEffects(LivingEntity& target)
{
    // 先调用父类（基类默认空实现，预留扩展点）
    AbstractArrowEntity::doPostHurtEffects(target);

    // 应用药水效果到被命中且存活的目标（对齐 vanilla Arrow.doPostHurtEffects:113-119）。
    // vanilla 用 potioncontents.forEachEffect(target.addEffect)，Cubium 箭矢存效果列表。
    // 此方法仅在 hurt 成功后由父类 onEntityHit 调用，hurt 失败（无敌帧/免疫）不施加。
    if (!target.isAlive()) {
        return;
    }
    const auto& arrowEffects = effects();
    for (const auto& effect : arrowEffects) {
        target.addEffect(effect);
    }
}

ItemStack ArrowEntity::getArrowStack() const
{
    const u32 arrowColor = color();
    const auto& arrowEffects = effects();

    // 如果有药水效果，返回药水箭；否则返回普通箭矢
    if (hasEffects()) {
        // 创建药水箭物品堆
        ItemStack tippedArrow(*Items::TIPPED_ARROW, 1);

        // 设置药水效果到物品堆的 NBT 标签
        // 注意：ArrowEntity 没有存储 Potion 类型，只有效果列表
        // 所以只设置自定义效果和颜色
        potion::PotionUtils::setCustomEffects(tippedArrow, arrowEffects);

        // 设置自定义颜色（如果有）
        if (arrowColor != 0xFFFFFFFF) {
            potion::PotionUtils::setCustomPotionColor(tippedArrow, arrowColor);
        }

        return tippedArrow;
    } else {
        // 返回普通箭矢
        return ItemStack(*Items::ARROW, 1);
    }
}

// ============================================================================
// SpectralArrowEntity
// ============================================================================

SpectralArrowEntity::SpectralArrowEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractArrowEntity(id, registry)
{
    setDamage(2.0f);
    // 批次6 子目标2 Step1：attach SpectralArrowComponent（光灵箭发光持续时间）。
    // Step4 将把 m_glowDuration 读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::SpectralArrowComponent>(m_entityContext->entity());
}

std::unique_ptr<Entity> SpectralArrowEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SpectralArrowEntity>(0, registry);
}

void SpectralArrowEntity::tick()
{
    AbstractArrowEntity::tick();

    // 光灵箭粒子效果 - 仅客户端执行
    if (!isInGround() && m_world && m_world->isClientSide()) {
        // 使用 INSTANT_EFFECT 粒子
        // 粒子位置：箭矢当前位置，速度为零
        m_world->addParticle(particle::ParticleTypeId::InstantSpell, Vector3(x(), y(), z()), Vector3(0.0f, 0.0f, 0.0f));
    }
}

void SpectralArrowEntity::doPostHurtEffects(LivingEntity& target)
{
    // 先调用父类（基类默认空实现，预留扩展点）
    AbstractArrowEntity::doPostHurtEffects(target);

    // 施加发光效果（对齐 vanilla SpectralArrow.doPostHurtEffects:41-45）。
    // 此方法仅在 hurt 成功后由父类 onEntityHit 调用，hurt 失败（无敌帧/免疫）不施加。
    target.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Glowing, glowDuration(), 0));
}

ItemStack SpectralArrowEntity::getArrowStack() const
{
    // 光灵箭总是返回光灵箭物品
    return ItemStack(*Items::SPECTRAL_ARROW, 1);
}

// 批次6 子目标2 Step4：m_glowDuration 迁入 ecs::SpectralArrowComponent。
i32 SpectralArrowEntity::glowDuration() const
{
    const auto* c = tryGetComponent<ecs::SpectralArrowComponent>();
    return (c != nullptr) ? c->m_glowDuration : 0;
}

void SpectralArrowEntity::setGlowDuration(i32 duration)
{
    auto* c = tryGetComponent<ecs::SpectralArrowComponent>();
    if (c != nullptr) {
        c->m_glowDuration = duration;
    }
}

} // namespace entity
} // namespace mc
