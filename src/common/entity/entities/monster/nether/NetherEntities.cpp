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

#include "NetherEntities.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/GhastMovementController.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidBlockGoal.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/GhastGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/util/PiglinAi.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/basic/SlimeEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/interfaces/ICrossbowUser.hpp"
#include "common/entity/interfaces/IFlinging.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace mc {

// GhastEntity
std::unique_ptr<Entity> GhastEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<GhastEntity>(EntityInstanceId(0), registry);
}

GhastEntity::GhastEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    setBurnsInDaylight(false);
    // 恶魂使用自定义的飞行移动控制器
    m_moveController = std::make_unique<entity::ai::controller::GhastMovementController>(this);

    // 飞行特性：恶魂不受重力，对齐 vanilla Ghast。
    // vanilla Ghast 覆写 travel() 委托 travelFlying（飞行物理，不应用重力）；
    // Cubium 通过 setNoGravity(true) 让 LivingEntity::travel 跳过重力分支
    // （LivingEntity.cpp:1487 `else if (!hasNoGravity())`），效果等价——
    // 恶魂悬停于空中，配合 GhastMovementController 的 setVelocity 速度补偿
    // 实现随机飞行。此前缺此调用，恶魂受重力持续下落（spawn 后逐 tick 沉降），
    // GhastRandomFlyGoal 的随机速度补偿不足以抵消重力，恶魂无法稳定悬浮开火。
    // 参考 VexEntity/WitherEntity 同用 setNoGravity(true) 实现飞行。
    // TODO: 更彻底的对齐是覆写 travel() 委托 FlyingEntity::travel 的飞行物理
    // （含飞行摩擦 0.91 + 飞行加速 0.02，对齐 vanilla travelFlying），但需提取
    // FlyingEntity::travel 为共享静态方法供非 FlyingEntity 子类复用，待后续重构。
    setNoGravity(true);

    // 补调 registerGoals / registerAttributes：MonsterEntity 构造只调基类版（vtable 指向 MonsterEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。Ghast 的 registerGoals 加专属 GhastRandomFly /
    // Fireball 等目标，registerAttributes 设 MAX_HEALTH/FOLLOW_RANGE/FLYING_SPEED。
    registerGoals();
    registerAttributes();
}

void GhastEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    if (m_isCharging) {
        m_chargeTime++;
        if (m_chargeTime >= 20) { // 充能时间
            shootFireball();
            m_isCharging = false;
            m_chargeTime = 0;
            m_attackCooldown = 40; // 攻击冷却
        }
    }
}

bool GhastEntity::isReflectedFireball(const DamageSource& source)
{
    // 对齐 MC Java 1.21.11 Ghast.isReflectedFireball（Ghast.java:81-83）：
    //   return p_238408_.getDirectEntity() instanceof LargeFireball
    //       && p_238408_.getEntity() instanceof Player;
    // directSource() = 火球本身（FireballEntity 对应 vanilla LargeFireball，不含 SmallFireball/DragonFireball）。
    // getEntity() = 发射者/反弹者（火球被玩家反弹时 setShooter 更新为玩家，见 ProjectileDeflection.cpp）。
    Entity* const directEntity = source.directSource();
    if (directEntity == nullptr) {
        return false;
    }
    // instanceof LargeFireball：仅大型火球（FireballEntity）算反弹判定，小火球/龙息不算。
    if (dynamic_cast<entity::FireballEntity*>(directEntity) == nullptr) {
        return false;
    }
    // instanceof Player：伤害造成者须为玩家（反弹者）。用 dynamic_cast 忠实复刻 instanceof 语义，
    // 覆盖 Player 及其派生类（ServerPlayer）。避免用 entityType() 比较——未 setTypeId 的实体
    // entityType() 懒查询可能返回意外值导致误判。
    Entity* const causingEntity = source.getEntity();
    return causingEntity != nullptr && dynamic_cast<Player*>(causingEntity) != nullptr;
}

bool GhastEntity::isInvulnerableTo(DamageSource& source) const
{
    // 对齐 MC Java 1.21.11 Ghast.isInvulnerableTo（Ghast.java:86-89）：
    //   return this.isInvulnerable() && !p_238289_.is(DamageTypeTags.BYPASSES_INVULNERABILITY)
    //       || !isReflectedFireball(p_238289_) && super.isInvulnerableTo(p_376822_, p_238289_);
    // 第一支：实体处于 invulnerable 标志且伤害不穿透无敌（恶魂默认不设 invulnerable，此支恒 false）。
    // 第二支：非反弹火球 → 委托基类正常判定；反弹火球 → !isReflectedFireball 为 false，整支 false（绕过所有免疫）。
    if (isInvulnerable() && !source.bypassesInvulnerability()) {
        return true;
    }
    if (isReflectedFireball(source)) {
        // 反弹火球绕过所有常规免疫判定（无视无敌帧/火焰免疫等），返回 false 不免疫。
        return false;
    }
    return MonsterEntity::isInvulnerableTo(source);
}

bool GhastEntity::hurt(DamageSource& source, f32 amount)
{
    // 对齐 MC Java 1.21.11 Ghast.hurtServer（Ghast.java:106-113）：
    //   if (isReflectedFireball(p_376819_)) {
    //       super.hurtServer(p_376618_, p_376819_, 1000.0F);  // 反弹火球 1000 伤害秒杀
    //       return true;
    //   } else {
    //       return this.isInvulnerableTo(p_376618_, p_376819_) ? false
    //           : super.hurtServer(p_376618_, p_376819_, p_376363_);
    //   }
    if (isReflectedFireball(source)) {
        // 反弹火球：直接调 LivingEntity::hurt 施加 1000 伤害秒杀，绕过 isInvulnerableTo 门控
        // （LivingEntity::hurt 内部仍查 isInvulnerableTo，但 isInvulnerableTo override 对反弹火球返回
        // false 不免疫，故 1000 伤害穿透生效）。恶魂满血 10，1000 必死。
        LivingEntity::hurt(source, 1000.0f);
        return true;
    }
    // 非反弹火球：走 MonsterEntity::hurt 标准链路（内部先查 isInvulnerableTo 门控，不免疫才扣血）。
    return MonsterEntity::hurt(source, amount);
}

void GhastEntity::shootFireball()
{
    // 在充能 20 ticks 后发射火球
    IWorld* worldPtr = world();
    LivingEntity* target = attackTarget();
    if (!worldPtr || !target || !target->isAlive()) {
        return;
    }

    // 计算发射方向
    // vector3d = this.parentEntity.getLook(1.0F)
    // 发射位置 = 恶魂位置 + lookVector * 4.0

    // 获取恶魂的朝向向量（yaw 和 pitch 计算得出）
    const f32 yawRad = yaw() * math::DEG_TO_RAD;
    const f32 pitchRad = pitch() * math::DEG_TO_RAD;

    // 计算 look 向量
    const f32 lookX = -std::sin(yawRad) * std::cos(pitchRad);
    const f32 lookY = -std::sin(pitchRad);
    const f32 lookZ = std::cos(yawRad) * std::cos(pitchRad);

    // 火球发射位置：恶魂位置 + lookVector * 4.0
    const f32 fireballX = static_cast<f32>(x() + lookX * 4.0);
    const f32 fireballY = static_cast<f32>(getEyeY() + 0.5 + lookY * 4.0);
    const f32 fireballZ = static_cast<f32>(z() + lookZ * 4.0);

    // 计算到目标的方向向量
    const f32 dx = static_cast<f32>(target->x() - fireballX);
    const f32 dy = static_cast<f32>(target->y() + target->eyeHeight() * 0.5 - (y() + eyeHeight() * 0.5 + 0.5));
    const f32 dz = static_cast<f32>(target->z() - fireballZ);

    // 创建火球实体
    // ECS 迁移：实体构造需要 registry 句柄（worldPtr 已判空，此处 registry 必非空）
    auto* registry = worldPtr->entityRegistry();
    if (registry == nullptr) {
        return;
    }
    auto fireball = std::make_unique<entity::FireballEntity>(EntityInstanceId(0), *registry);
    fireball->setTypeId(entity::EntityTypeKeys::FIREBALL); // 工厂绕过补救：直接构造缺 typeId
    fireball->setWorld(worldPtr); // 直接构造的实体须显式 setWorld（对齐 BlazeFireballAttackGoal/DragonFireball）
    fireball->setShooter(this);
    fireball->setPosition(Vector3(fireballX, fireballY, fireballZ));

    // 设置加速度方向
    // FireballEntity 构造函数中设置加速度
    // accelerationX/Y/Z 每tick累加到速度上
    fireball->setAcceleration(dx, dy, dz);

    // 设置爆炸威力
    fireball->setExplosionPower(m_explosionPower);

    // 生成实体
    worldPtr->spawnEntity(std::move(fireball));

    // 播放发射音效
    playSound(SoundEvents::ENTITY_GHAST_SHOOT, 1.0f, 1.0f);
}

void GhastEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 优先级 5: 随机飞行
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::GhastRandomFlyGoal>(this));

    // 优先级 7: 环顾四周
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::GhastLookAroundGoal>(this));

    // 优先级 7: 火球攻击
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::GhastFireballAttackGoal>(this));

    // 目标选择器：攻击最近的玩家
    // 条件：玩家与恶魂的Y坐标差不超过4格
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this,
            true, // checkSight - 需要视线检查
            10,   // chance - 每10tick检查一次
            [](const LivingEntity* /*entity*/) -> bool {
                // Y坐标检查主要影响目标选择，在目标选择器中检查可能不太准确
                return true;
            }));
}

void GhastEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 恶魂有极远的追踪范围
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 100.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    attributes().setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.9);
}

// MagmaCubeEntity
std::unique_ptr<Entity> MagmaCubeEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<MagmaCubeEntity>(EntityInstanceId(0), registry);
}

MagmaCubeEntity::MagmaCubeEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : SlimeEntity(id, registry)
{
    // 岩浆怪不在阳光下燃烧
    setBurnsInDaylight(false);

    // 补调 registerAttributes：派生 override 永不执行（vtable 在基类构造期间指向基类），
    // 须在派生类构造显式调用。MagmaCube 无 registerGoals override（继承 Slime）。
    registerAttributes();
}

void MagmaCubeEntity::setSlimeSize(i32 size, bool resetHealth)
{
    // 调用父类设置尺寸，然后设置护甲属性 = size * 3
    SlimeEntity::setSlimeSize(size, resetHealth);

    attributes().setBaseValue(entity::attribute::Attributes::ARMOR, static_cast<f64>(size * 3));
}

bool MagmaCubeEntity::canDamagePlayer() const
{
    // 小型岩浆怪也能伤害玩家（与史莱姆不同）
    const IWorld* worldPtr = world();
    return worldPtr != nullptr && !worldPtr->isClientSide();
}

f32 MagmaCubeEntity::getAttackDamage() const
{
    // 攻击伤害 = 属性值 + 2.0F
    f32 baseDamage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0));
    return baseDamage + 2.0f;
}

i32 MagmaCubeEntity::getJumpDelay() const
{
    // 跳跃延迟是史莱姆的 4 倍（史莱姆: 10-30 tick, 岩浆怪: 40-120 tick）
    return SlimeEntity::getJumpDelay() * 4;
}

std::optional<ResourceLocation> MagmaCubeEntity::getHurtSound(DamageSource& /*source*/) const
{
    // 小岩浆怪用 hurt_small，大岩浆怪用 hurt
    if (isSmallSlime()) {
        return SoundEvents::ENTITY_MAGMA_CUBE_HURT_SMALL;
    }
    return SoundEvents::ENTITY_MAGMA_CUBE_HURT;
}

std::optional<ResourceLocation> MagmaCubeEntity::getDeathSound() const
{
    // 小岩浆怪用 death_small，大岩浆怪用 death
    if (isSmallSlime()) {
        return SoundEvents::ENTITY_MAGMA_CUBE_DEATH_SMALL;
    }
    return SoundEvents::ENTITY_MAGMA_CUBE_DEATH;
}

std::optional<ResourceLocation> MagmaCubeEntity::getSquishSound() const
{
    // 小岩浆怪用 squish_small，大岩浆怪用 squish
    if (isSmallSlime()) {
        return SoundEvents::ENTITY_MAGMA_CUBE_SQUISH_SMALL;
    }
    return SoundEvents::ENTITY_MAGMA_CUBE_SQUISH;
}

std::optional<ResourceLocation> MagmaCubeEntity::getJumpSound() const
{
    return SoundEvents::ENTITY_MAGMA_CUBE_JUMP;
}

particle::ParticleTypeId MagmaCubeEntity::getSquishParticle() const
{
    // 岩浆怪使用火焰粒子代替史莱姆粒子
    return particle::ParticleTypeId::Flame;
}

void MagmaCubeEntity::registerAttributes()
{
    SlimeEntity::registerAttributes();

    // 移动速度固定为 0.2（不随尺寸变化）
    // 注意：父类 SlimeEntity::registerAttributes() 会设置尺寸相关属性
    // 这里需要确保护甲属性已注册
    attributes().registerAttribute(*entity::attribute::Attributes::armor());

    // 初始尺寸为1，护甲为3
    attributes().setBaseValue(entity::attribute::Attributes::ARMOR, 3.0);
}

void MagmaCubeEntity::alterSquishAmount()
{
    // 挤压动画衰减更慢（史莱姆: 0.6, 岩浆怪: 0.9）
    setSquishAmount(squishAmount() * 0.9f);
}

// AbstractPiglinEntity
AbstractPiglinEntity::AbstractPiglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    setBurnsInDaylight(false);
}

void AbstractPiglinEntity::registerGoals()
{
    // 调用父类方法（MonsterEntity 不再注册 HurtByTargetGoal 和 SwimGoal 以外的目标）
    // 注意: MonsterEntity::registerGoals() 已注册 SwimGoal，无需重复添加
    MonsterEntity::registerGoals();

    // 所有猪灵类实体的共同目标
    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, false));
}

// PiglinEntity
std::unique_ptr<Entity> PiglinEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PiglinEntity>(EntityInstanceId(0), registry);
}

PiglinEntity::PiglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractPiglinEntity(id, registry)
{
    // 补调 registerGoals / registerAttributes：AbstractPiglinEntity 构造不调（vtable 指向基类时
    // 派生 override 永不执行），须在派生类构造显式调用。Piglin 的 registerGoals 加专属弩远程 /
    // 近战 / 避敌目标。
    registerGoals();
    registerAttributes();
}

void PiglinEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    // 猪灵使用弩进行远程攻击
    // charge 参数对弩不重要，弩使用固定速度
    MC_UNUSED(charge);

    if (!target || !m_world) return;

    // 获取主手弩
    ItemStack& crossbow = getMutableMainHandItem();
    const Item* item = crossbow.getItem();

    // 检查是否是弩
    if (item == nullptr || item->getUseAction(crossbow) != UseAction::Crossbow) {
        return;
    }

    // 调用 shootCrossbow 发射弩箭
    shootCrossbow(target, crossbow, 1.0f);
}

void PiglinEntity::onCrossbowLoadComplete(ItemStack& crossbow)
{
    // 装填完成后重置空闲时间，防止立即消失
    setIdleTime(0);

    MC_UNUSED(crossbow);
}

void PiglinEntity::shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge)
{
    if (!target || !m_world || !crossbow.getItem()) return;

    MC_UNUSED(charge);

    // 猪灵弩射击逻辑
    // 计算弹道
    f64 dx = target->x() - x();
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 目标高度偏移：目标高度的 1/3（瞄准躯干下部，对齐 MC AbstractSkeleton.performRangedAttack）
    // 弹道高度补偿：水平距离 * 0.2
    f64 dy = target->getY(0.3333333333333333) - (getEyeY() - 0.15) + horizontalDist * 0.2;

    // 确定速度
    f32 velocity = 3.15f; // 箭矢速度

    // 计算不精确度：14 - difficulty.getId() * 4
    // Peaceful=14, Easy=10, Normal=6, Hard=2
    f32 inaccuracy = entity::combat::DifficultyHelper::getRangedAttackInaccuracy(m_world->difficulty());

    // 创建箭矢实体
    // ECS 迁移：实体构造需要 registry 句柄（m_world 已判空，此处 registry 必非空）
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }
    auto arrow = std::make_unique<entity::ArrowEntity>(EntityInstanceId(0), *registry);
    arrow->setTypeId(entity::EntityTypeKeys::ARROW); // 工厂绕过补救：直接构造缺 typeId
    arrow->setWorld(m_world);
    arrow->setPosition(x(), static_cast<f32>(getEyeY() - 0.15), z());
    arrow->setShooter(this);

    // 设置箭矢属性
    arrow->setShotFromCrossbow(true);
    arrow->setDamage(5.0f); // 猪灵箭矢伤害

    // 计算发射方向
    f32 yaw = this->yaw();
    f32 pitch = this->pitch();

    if (horizontalDist > 0.001) {
        yaw = static_cast<f32>(std::atan2(dz, dx) * 180.0 / math::PI) - 90.0f;
        pitch = static_cast<f32>(std::atan2(dy, horizontalDist) * 180.0 / math::PI);
    }

    // 发射箭矢
    arrow->shootFrom(*this, pitch, yaw, 0.0f, velocity, inaccuracy);

    // 生成箭矢实体
    m_world->spawnEntity(std::move(arrow));

    // 播放发射音效
    playSound(SoundEvents::ITEM_CROSSBOW_SHOOT, 1.0f, getRandom().nextFloat() * 0.4f + 0.8f);

    // 清除弩的装填状态
    const auto* crossbowItem = dynamic_cast<const item::CrossbowItem*>(crossbow.getItem());
    if (crossbowItem) {
        item::CrossbowItem::setCharged(crossbow, false);
        item::CrossbowItem::clearProjectiles(crossbow);
    }
}

void PiglinEntity::registerGoals()
{
    AbstractPiglinEntity::registerGoals();

    // 猪灵使用 Goal 系统实现类似 Brain 系统的行为
    // 成年猪灵：持有弩时使用弩攻击
    // 幼年猪灵：不会攻击

    // 优先级 2: 弩远程攻击（仅成年猪灵）
    if (!m_isBaby) {
        m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RangedCrossbowAttackGoal>(this, 1.0, 15.0f));
    }

    // 优先级 3: 近战攻击（备用，当没有弩或弩无法使用时）
    if (!m_isBaby) {
        m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, false));
    }

    // 优先级 5: 避开僵尸猪灵和僵尸疣兽
    m_goalSelector.addGoal(5,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(
            this, 24.0f, 1.0, 1.2, entity::ai::goal::AvoidEntityGoal::EntityPredicate([](const LivingEntity* entity) {
                if (!entity || !entity->isAlive()) return false;
                // 避开僵尸猪灵和僵尸疣兽
                auto type = entity->entityType();
                return type == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN ||
                    type == entity::VanillaEntityTypeKeys::ZOGLIN;
            })));

    // 优先级 4: 避开排斥方块（猪灵害怕灵魂火、灵魂火把、灵魂灯笼、灵魂营火、诡异菌等）
    // 对应 MC 1.21.11: PiglinAi 中 SetWalkTargetAwayFrom.pos(NEAREST_REPELLENT, 1.0F, 8, false)
    // 原版通过 Brain/Sensor 系统实现，当前使用 Goal 系统等效替代
    // 注意：灵魂营火需要额外检查点燃状态，未点燃的灵魂营火不排斥猪灵
    m_goalSelector.addGoal(4,
        std::make_unique<entity::ai::goal::AvoidBlockGoal>(
            this, BlockTags::PIGLIN_REPELLENTS(), 1.0, 8, 4, [](const BlockState& state) {
                // MC 1.21.11: PiglinSpecificSensor.isValidRepellent
                // 灵魂营火需要额外检查点燃状态，未点燃的灵魂营火不应排斥猪灵
                if (state.is(block_registry::NetherBlocks::SOUL_CAMPFIRE)) {
                    return blocks::CampfireBlock::isLitCampfire(state);
                }
                return true;
            }));

    // 优先级 7: 避开水随机行走
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 9: 随机看向
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    if (!m_isBaby) {
        // 优先级 2: 攻击未穿戴金装备的玩家
        m_targetSelector.addGoal(2,
            std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(
                this, true, 10, [](const LivingEntity* entity) {
                    const Player* player = dynamic_cast<const Player*>(entity);
                    if (!player || !player->isAlive()) return false;
                    // 攻击未穿戴金装备的玩家
                    return !entity::PiglinAi::isWearingGold(*player);
                }));
    }
}

void PiglinEntity::registerAttributes()
{
    AbstractPiglinEntity::registerAttributes();

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 16.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

// ========== PiglinEntity IAngerable 接口实现 ==========

void PiglinEntity::setAttackTarget(LivingEntity* target)
{
    // MobEntity::setAttackTarget 现在是虚函数，此override同时满足
    // MobEntity和IAngerable的setAttackTarget，统一使用MobEntity::m_attackTarget
    MobEntity::setAttackTarget(target);
}

LivingEntity* PiglinEntity::getAttackTarget() const
{
    return const_cast<PiglinEntity*>(this)->MobEntity::attackTarget();
}

void PiglinEntity::setRevengeTarget(LivingEntity* target)
{
    // 对齐同族 IAngerable id 校验模式（TameableEntity:121-131）：存 id 而非裸指针，避免复仇目标
    // remove()/chunk 卸载析构后悬垂 UAF（无 GC 环境，见 [[damage-source-clone-uaf-id-validation]]）。
    if (target != nullptr) {
        m_revengeTargetId = target->id();
        m_revengeTimer = 100; // 5秒复仇计时
    } else {
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

LivingEntity* PiglinEntity::getRevengeTarget() const
{
    // 对齐同族 IAngerable id 校验模式（TameableEntity:133-148）：经 world->getEntity(id)+isAlive
    // 安全校验，复仇目标析构后返回 nullptr，避免解引用悬垂裸指针 UAF。
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    IWorld* worldPtr = const_cast<IWorld*>(this->world());
    if (worldPtr == nullptr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (entity == nullptr || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

i32 PiglinEntity::getRevengeTimer() const
{
    return m_revengeTimer;
}

bool PiglinEntity::isAngry() const
{
    return m_angry || m_angerTime > 0;
}

void PiglinEntity::setAngry(bool angry)
{
    if (angry) {
        m_angerTime = 600; // 30秒愤怒持续时间
    } else {
        m_angerTime = 0;
        setAttackTarget(nullptr);
    }
    m_angry = angry;
}

i32 PiglinEntity::getAngerTime() const
{
    return m_angerTime;
}

void PiglinEntity::setAngerTime(i32 time)
{
    m_angerTime = time;
}

void PiglinEntity::tick()
{
    AbstractPiglinEntity::tick();

    // 更新愤怒计时器
    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_angry = false;
            setAttackTarget(nullptr);
        }
    }

    // 更新复仇计时器
    if (m_revengeTimer > 0) {
        m_revengeTimer--;
        if (m_revengeTimer <= 0) {
            m_revengeTargetId = std::nullopt;
        }
    }
}

// ========== 猪灵寻路权重 ==========

f32 PiglinEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 对应 MC 1.21.11: Piglin.getWalkTargetValue
    //   PiglinAi.isNearRepellent(piglin, pos) 检查附近是否有 PIGLIN_REPELLENTS 方块
    //   如果有排斥物，返回 -1.0F（拒绝前往）
    //
    // MC 原版通过 Brain 系统的 PiglinSpecificSensor 定期扫描附近排斥物并缓存到
    // NEAREST_REPELLENT 记忆模块中，然后 isNearRepellent 检查缓存的位置。
    // 当前项目 PiglinEntity 尚未集成 Brain 系统，
    // 因此采用与 HoglinEntity::getPathWeight 相同的直接扫描方案。
    //
    // TODO: 集成 Brain/Sensor 系统后，迁移为 PiglinSpecificSensor + NEAREST_REPELLENT 记忆模块方案

    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }

    // 排斥物近距离检查
    // MC 1.21.11 BlockTags.PIGLIN_REPELLENTS 包含:
    //   - 灵魂火 (soul_fire)
    //   - 灵魂火把 (soul_torch)
    //   - 灵魂墙火把 (soul_wall_torch)
    //   - 灵魂灯笼 (soul_lantern)
    //   - 灵魂营火 (soul_campfire，需点燃，通过 CampfireBlock::isLitCampfire 额外检查)
    // 注意：MC 1.21.11 中 warped_fungus 不在 PIGLIN_REPELLENTS 中，
    //       仅存在于 HOGLIN_REPELLENTS 中
    // 注意：MC 1.21.11 中 potted_warped_fungus 不在 PIGLIN_REPELLENTS 中，
    //       仅在 HOGLIN_REPELLENTS 中，此处无需添加
    //
    // 注意：此 getPathWeight 与 AvoidBlockGoal（优先级4）协同工作：
    //   - getPathWeight 返回 -1.0 阻止寻路穿过排斥区域
    //   - AvoidBlockGoal 主动使实体远离排斥方块
    //   两者共同实现了 MC 原版 isNearRepellent + SetWalkTargetAwayFrom 的行为
    //
    // 搜索范围: 水平 8 格，垂直 4 格
    // 对应 MC 原版: PiglinSpecificSensor.REPELLENT_DETECTION_RANGE_HORIZONTAL = 8,
    //              PiglinSpecificSensor.REPELLENT_DETECTION_RANGE_VERTICAL = 4
    static constexpr i32 REPELLENT_RANGE_H = 8;
    static constexpr i32 REPELLENT_RANGE_V = 4;

    const BlockTag& piglinRepellents = BlockTags::PIGLIN_REPELLENTS();
    const i32 bx = static_cast<i32>(x);
    const i32 by = static_cast<i32>(y);
    const i32 bz = static_cast<i32>(z);

    for (i32 dx = -REPELLENT_RANGE_H; dx <= REPELLENT_RANGE_H; ++dx) {
        for (i32 dy = -REPELLENT_RANGE_V; dy <= REPELLENT_RANGE_V; ++dy) {
            for (i32 dz = -REPELLENT_RANGE_H; dz <= REPELLENT_RANGE_H; ++dz) {
                const BlockState* state = worldPtr->getBlockState(bx + dx, by + dy, bz + dz);
                if (state != nullptr && piglinRepellents.contains(*state)) {
                    // MC 1.21.11: PiglinSpecificSensor.isValidRepellent
                    // 灵魂营火需要额外检查点燃状态，未点燃的灵魂营火不应排斥猪灵
                    if (state->is(block_registry::NetherBlocks::SOUL_CAMPFIRE)) {
                        if (!blocks::CampfireBlock::isLitCampfire(*state)) {
                            continue; // 未点燃的灵魂营火不排斥猪灵
                        }
                    }
                    return -1.0f;
                }
            }
        }
    }

    return CreatureEntity::getPathWeight(x, y, z);
}

// PiglinBruteEntity
std::unique_ptr<Entity> PiglinBruteEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PiglinBruteEntity>(EntityInstanceId(0), registry);
}

PiglinBruteEntity::PiglinBruteEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractPiglinEntity(id, registry)
{
    // 补调 registerGoals / registerAttributes：AbstractPiglinEntity 构造不调（vtable 指向基类时
    // 派生 override 永不执行），须在派生类构造显式调用。PiglinBrute 的 registerGoals 加专属近战 /
    // 随机行走 / 看向目标。
    registerGoals();
    registerAttributes();
}

void PiglinBruteEntity::registerGoals()
{
    AbstractPiglinEntity::registerGoals();

    // 猪灵蛮兵是纯粹的近战单位，不使用弩

    // 优先级 2: 近战攻击
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, false));

    // 优先级 7: 避开水随机行走
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 9: 随机看向
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    // 猪灵蛮兵不像普通猪灵那样检查金装备，直接攻击玩家

    // 优先级 2: 被攻击后反击并呼叫支援
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true));

    // 优先级 3: 攻击玩家（不检查金装备）
    m_targetSelector.addGoal(3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true, 0));
}

void PiglinBruteEntity::registerAttributes()
{
    AbstractPiglinEntity::registerAttributes();

    // 猪灵蛮兵属性（金斧额外 +4 伤害）
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 50.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 7.0);
}

// ZombifiedPiglinEntity
std::unique_ptr<Entity> ZombifiedPiglinEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ZombifiedPiglinEntity>(EntityInstanceId(0), registry);
}

ZombifiedPiglinEntity::ZombifiedPiglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    setBurnsInDaylight(false);

    // 补调 registerGoals / registerAttributes：MonsterEntity 构造只调基类版（vtable 指向 MonsterEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。ZombifiedPiglin 的 registerGoals 加专属近战 /
    // 随机行走 / HurtByTarget 激怒目标。
    registerGoals();
    registerAttributes();
}

void ZombifiedPiglinEntity::tick()
{
    MonsterEntity::tick();

    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_angry = false;
        }
    }
}

void ZombifiedPiglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 僵尸猪灵是中立生物，被攻击后会激怒附近所有僵尸猪灵

    // 优先级 2: 近战攻击
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, false));

    // 优先级 7: 避开水随机行走
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 9: 随机看向
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器
    // 僵尸猪灵被攻击后会激怒并反击

    // 优先级 1: 被攻击后反击并呼叫支援（激怒附近僵尸猪灵）
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true));
}

void ZombifiedPiglinEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
}

// HoglinEntity
std::unique_ptr<Entity> HoglinEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<HoglinEntity>(EntityInstanceId(0), registry);
}

HoglinEntity::HoglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    setBurnsInDaylight(false);
    registerAttributes();

    // 补调 registerGoals：MonsterEntity 构造只调基类版（vtable 指向 MonsterEntity），派生 override
    // 永不执行，须在派生类构造显式调用。Hoglin 的 registerGoals 加专属近战 / 避排斥方块 / 激怒目标。
    // registerAttributes 已在上方调用。
    registerGoals();
}

void HoglinEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackAnimationTicks > 0) {
        m_attackAnimationTicks--;
    }
}

bool HoglinEntity::attackEntityAsMob(LivingEntity& target)
{
    // 对齐 MC 1.21.11 Hoglin.doHurtTarget（Hoglin.java:119-130）→ HoglinBase.hurtAndThrowTarget。
    // Hoglin 近战攻击与通用 MobEntity::attackEntityAsMob 语义不同，此处 override 自管完整攻击链
    // （对齐 IronGolemEntity::attackEntityAsMob 范式），不调基类避免双重伤害。
    //
    // 由 MeleeAttackGoal::_attackTarget 委托调用（对齐 vanilla MeleeAttackGoal.checkAndPerformAttack 调
    // mob.doHurtTarget）。MeleeAttackGoal 自身已有 attackCooldown（resetAttackCooldown=adjustedTickDelay(20)），
    // 故此处不再维护独立冷却。

    // 1. 设置攻击动画 + 广播到客户端（vanilla doHurtTarget: attackAnimationRemainingTicks=10 + entity event 4）
    m_attackAnimationTicks = 10;
    if (m_world != nullptr) {
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::HoglinAttack));
    }

    // 2. 计算伤害：成年随机化，幼年固定。对齐 HoglinBase.hurtAndThrowTarget:
    //   float f1 = ATTACK_DAMAGE;
    //   if (!isBaby() && (int)f1 > 0) f = f1 / 2.0F + random.nextInt((int)f1);
    //   else f = f1;
    f32 damage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0));
    if (!m_isBaby && static_cast<i32>(damage) > 0) {
        math::Random& rng = getRandom();
        damage = damage / 2.0f + static_cast<f32>(rng.nextInt(static_cast<i32>(damage)));
    }

    // 3. 应用伤害（mobAttack 来源）
    EntityDamageSource damageSource = DamageSources::mobAttack(this);
    bool success = target.hurt(damageSource, damage);

    if (success) {
        // 4. 成年施加抛飞 throwTarget（水平击退 + 垂直抬升 + 随机旋转）。对齐 HoglinBase.throwTarget。
        //    复用 IFlinging::flingTarget（已实现 vanilla 抛飞核心语义：knockbackStrength=ATTACK_KNOCKBACK-
        //    KNOCKBACK_RESISTANCE，水平+垂直 addVelocity）。幼年不抛飞（vanilla isBaby 门控）。
        if (!m_isBaby) {
            entity::IFlinging::flingTarget(*this, target);
        }

        // 5. 触发附魔后续效果（节肢杀手减速等，对齐 vanilla doPostAttackEffects）
        onAttackEntity(target);

        // 6. 设置最后攻击者（对齐基类 attackEntityAsMob 的 setLastHurtBy）
        target.setLastHurtBy(this);
    }

    // 7. 播放攻击音效（无论是否命中，对齐 IronGolem 范式）
    playSound(SoundEvents::ENTITY_HOGLIN_ATTACK, 1.0f, 1.0f);

    return success;
}

void HoglinEntity::playAttackSound(LivingEntity& /*target*/)
{
    // 攻击音效已在 attackEntityAsMob 中播放（无论是否命中），此处 override 为空避免基类重复播放。
    // 对齐 IronGolemEntity::playAttackSound 范式。
}

void HoglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 成年疣猪兽对玩家敌对，幼年疣猪兽被动

    // 优先级 2: 近战攻击（仅成年）
    if (!m_isBaby) {
        m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, true));
    }

    // 优先级 5: 避开排斥方块（疣猪兽害怕诡异菌、诡异菌岩、下界传送门、重生锚等）
    // 对应 MC 1.21.11: HoglinAi 中 SetWalkTargetAwayFrom.pos(NEAREST_REPELLENT, 1.0F, 8, true)
    // 原版通过 Brain/Sensor 系统实现，当前使用 Goal 系统等效替代
    m_goalSelector.addGoal(
        5, std::make_unique<entity::ai::goal::AvoidBlockGoal>(this, BlockTags::HOGLIN_REPELLENTS(), 1.0, 8, 4));

    // 优先级 7: 避开水随机行走
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 9: 随机看向
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器（仅成年）
    if (!m_isBaby) {
        // 优先级 1: 被攻击后反击
        m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, false));

        // 优先级 2: 攻击玩家
        m_targetSelector.addGoal(
            2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true, 0));
    }
}

void HoglinEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 注册攻击属性（MonsterEntity 不自动注册这些）
    attributes().registerAttribute(*entity::attribute::Attributes::attackDamage());
    attributes().registerAttribute(*entity::attribute::Attributes::attackKnockback());

    // 成年疣猪兽属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    attributes().setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.6);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.0);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
}

// ========== 寻路权重 ==========

f32 HoglinEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 对应 MC 1.21.11: Hoglin.getWalkTargetValue
    //   if (HoglinAi.isPosNearNearestRepellent(this, pos)) return -1.0F;
    //   return level.getBlockState(pos.below()).is(Blocks.CRIMSON_NYLIUM) ? 10.0F : 0.0F;
    //
    // MC 原版通过 Brain 系统的 HoglinSpecificSensor 定期扫描附近排斥物并缓存到
    // NEAREST_REPELLENT 记忆模块中，然后 isPosNearNearestRepellent 检查缓存的排斥物
    // 位置是否在 8 格范围内。当前项目 HoglinEntity 尚未集成 Brain 系统，
    // 因此采用直接扫描方案：每次调用时搜索当前位置周围水平 8 格、垂直 4 格范围内的排斥物方块。
    // 这与 MC 原版 HoglinSpecificSensor.findNearestRepellent 的搜索范围一致。

    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }

    // 排斥物近距离检查
    // MC 1.21.11 BlockTags.HOGLIN_REPELLENTS 包含:
    //   - 诡异菌 (warped_fungus)
    //   - 下界传送门 (nether_portal)
    //   - 重生锚 (respawn_anchor)
    // TODO: 花盆系统实现后需在 HOGLIN_REPELLENTS 标签中添加 potted_warped_fungus（盆栽诡异菌），
    //       此处扫描和 AvoidBlockGoal 会自动包含新标签成员
    //
    // 注意：此 getPathWeight 与 AvoidBlockGoal（优先级5）协同工作：
    //   - getPathWeight 返回 -1.0 阻止寻路穿过排斥区域
    //   - AvoidBlockGoal 主动使实体远离排斥方块
    //   两者共同实现了 MC 原版 isPosNearNearestRepellent + SetWalkTargetAwayFrom 的行为
    //
    // 搜索范围: 水平 8 格，垂直 4 格
    // 对应 MC 原版: HoglinSpecificSensor.REPELLENT_DETECTION_RANGE_HORIZONTAL = 8,
    //              HoglinSpecificSensor.REPELLENT_DETECTION_RANGE_VERTICAL = 4
    static constexpr i32 REPELLENT_RANGE_H = 8;
    static constexpr i32 REPELLENT_RANGE_V = 4;

    const BlockTag& hoglinRepellents = BlockTags::HOGLIN_REPELLENTS();
    const i32 bx = static_cast<i32>(x);
    const i32 by = static_cast<i32>(y);
    const i32 bz = static_cast<i32>(z);

    for (i32 dx = -REPELLENT_RANGE_H; dx <= REPELLENT_RANGE_H; ++dx) {
        for (i32 dy = -REPELLENT_RANGE_V; dy <= REPELLENT_RANGE_V; ++dy) {
            for (i32 dz = -REPELLENT_RANGE_H; dz <= REPELLENT_RANGE_H; ++dz) {
                const BlockState* state = worldPtr->getBlockState(bx + dx, by + dy, bz + dz);
                if (state != nullptr && hoglinRepellents.contains(*state)) {
                    return -1.0f;
                }
            }
        }
    }

    // 偏好绯红菌岩：站在绯红菌岩上返回 10.0f
    BlockPos posBelow(static_cast<i32>(x), static_cast<i32>(y) - 1, static_cast<i32>(z));
    const BlockState* groundBlock = worldPtr->getBlockState(posBelow);
    if (groundBlock != nullptr && groundBlock->is(block_registry::NetherBlocks::CRIMSON_NYLIUM)) {
        return 10.0f;
    }

    return 0.0f;
}

// ZoglinEntity
std::unique_ptr<Entity> ZoglinEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ZoglinEntity>(EntityInstanceId(0), registry);
}

ZoglinEntity::ZoglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    // 注意：MonsterEntity 基类构造虽调用了 registerGoals()/registerAttributes()，但 C++ 基类构造期
    // 虚函数不派发到派生类（vtable 此时仍是 MonsterEntity 的），导致 ZoglinEntity 的 override 版本
    // 不会被调用——AI 目标与属性（MAX_HEALTH=40 等）将丢失。故在此显式补调，此时 vtable 已就绪。
    // 与 LivingEntity 生命值同步修复（commit 340f9c235）同源根因。
    registerAttributes();
    registerGoals();
}

void ZoglinEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackAnimationTicks > 0) {
        m_attackAnimationTicks--;
    }
}

bool ZoglinEntity::attackEntityAsMob(LivingEntity& target)
{
    // 对齐 MC 1.21.11 Zoglin.doHurtTarget → HoglinBase.hurtAndThrowTarget（Zoglin 复用 HoglinBase 攻击逻辑）。
    // 与 HoglinEntity::attackEntityAsMob 同源：override 自管完整攻击链（动画+随机化伤害+抛飞+音效），
    // 不调基类避免双重伤害。由 MeleeAttackGoal::_attackTarget 委托调用。

    // 1. 设置攻击动画 + 广播到客户端（vanilla attackAnimationRemainingTicks=10 + entity event 4）
    m_attackAnimationTicks = 10;
    if (m_world != nullptr) {
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::HoglinAttack));
    }

    // 2. 计算伤害：成年随机化，幼年固定。对齐 HoglinBase.hurtAndThrowTarget。
    f32 damage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0));
    if (!m_isBaby && static_cast<i32>(damage) > 0) {
        math::Random& rng = getRandom();
        damage = damage / 2.0f + static_cast<f32>(rng.nextInt(static_cast<i32>(damage)));
    }

    // 3. 应用伤害（mobAttack 来源）
    EntityDamageSource damageSource = DamageSources::mobAttack(this);
    bool success = target.hurt(damageSource, damage);

    if (success) {
        // 4. 成年施加抛飞 throwTarget（对齐 HoglinBase.throwTarget，复用 IFlinging::flingTarget）
        if (!m_isBaby) {
            entity::IFlinging::flingTarget(*this, target);
        }

        // 5. 触发附魔后续效果（节肢杀手减速等）
        onAttackEntity(target);

        // 6. 设置最后攻击者
        target.setLastHurtBy(this);
    }

    // 7. 播放攻击音效（无论是否命中）
    playSound(SoundEvents::ENTITY_HOGLIN_ATTACK, 1.0f, 1.0f);

    return success;
}

void ZoglinEntity::playAttackSound(LivingEntity& /*target*/)
{
    // 攻击音效已在 attackEntityAsMob 中播放，此处 override 为空避免基类重复播放。
}

void ZoglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 僵尸疣兽对几乎所有生物敌对（除了其他僵尸疣兽和幼年生物）

    // 优先级 2: 近战攻击（仅成年）
    if (!m_isBaby) {
        m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, true));
    }

    // 优先级 7: 随机行走
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 8.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 9: 随机看向
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器（仅成年）
    // 僵尸疣兽攻击几乎所有生物
    if (!m_isBaby) {
        // 优先级 1: 被攻击后反击
        m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, false));

        // 优先级 2: 攻击玩家
        m_targetSelector.addGoal(
            2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true, 0));

        // 优先级 3: 攻击其他生物（排除僵尸疣兽和苦力怕）
        // MC 原版 Zoglin.isTargetable 排除 Zoglin 和 Creeper，
        // 创造/旁观模式玩家由 TargetGoal::isSuitableTarget 自动排除
        m_targetSelector.addGoal(3,
            std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(
                this, true, 10, entity::ai::goal::TargetPredicate([](const LivingEntity* entity) {
                    if (!entity || !entity->isAlive()) return false;
                    auto type = entity->entityType();
                    // 排除僵尸疣兽自己
                    if (type == entity::VanillaEntityTypeKeys::ZOGLIN) return false;
                    // 排除苦力怕（MC 原版 Zoglin 不攻击 Creeper）
                    if (type == entity::VanillaEntityTypeKeys::CREEPER) return false;
                    return true;
                })));
    }
}

void ZoglinEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 注册攻击属性（MonsterEntity 不自动注册这些）
    attributes().registerAttribute(*entity::attribute::Attributes::attackDamage());
    attributes().registerAttribute(*entity::attribute::Attributes::attackKnockback());

    // 成年僵尸疣兽属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    attributes().setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.6);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.0);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
}

bool ZoglinEntity::canAttackType(const entity::EntityType& type) const
{
    // MC 原版 Zoglin.isTargetable 排除 Zoglin 和 Creeper
    if (&type == entity::VanillaEntityTypeKeys::ZOGLIN) {
        return false;
    }
    if (&type == entity::VanillaEntityTypeKeys::CREEPER) {
        return false;
    }
    return MonsterEntity::canAttackType(type);
}

} // namespace mc
