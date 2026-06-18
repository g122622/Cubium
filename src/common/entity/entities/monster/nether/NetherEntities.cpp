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
#include "common/entity/ai/controller/GhastMovementController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/GhastGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/interfaces/ICrossbowUser.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>

namespace mc {

// GhastEntity
std::unique_ptr<Entity> GhastEntity::create(IWorld* world)
{
    return std::make_unique<GhastEntity>(EntityId(0));
}

GhastEntity::GhastEntity(EntityId id)
    : MonsterEntity(id)
{
    setBurnsInDaylight(false);
    // 恶魂使用自定义的飞行移动控制器
    m_moveController = std::make_unique<entity::ai::controller::GhastMovementController>(this);
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
    const f32 fireballY = static_cast<f32>(y() + eyeHeight() + 0.5 + lookY * 4.0);
    const f32 fireballZ = static_cast<f32>(z() + lookZ * 4.0);

    // 计算到目标的方向向量
    const f32 dx = static_cast<f32>(target->x() - fireballX);
    const f32 dy = static_cast<f32>(target->y() + target->eyeHeight() * 0.5 - (y() + eyeHeight() * 0.5 + 0.5));
    const f32 dz = static_cast<f32>(target->z() - fireballZ);

    // 创建火球实体
    auto fireball = std::make_unique<entity::FireballEntity>(EntityId(0));
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
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 100.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.9);
}

// MagmaCubeEntity
std::unique_ptr<Entity> MagmaCubeEntity::create(IWorld* world)
{
    return std::make_unique<MagmaCubeEntity>(EntityId(0));
}

MagmaCubeEntity::MagmaCubeEntity(EntityId id)
    : SlimeEntity(id)
{
    // 岩浆怪不在阳光下燃烧
    setBurnsInDaylight(false);
}

void MagmaCubeEntity::setSlimeSize(i32 size, bool resetHealth)
{
    // 调用父类设置尺寸，然后设置护甲属性 = size * 3
    SlimeEntity::setSlimeSize(size, resetHealth);

    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, static_cast<f64>(size * 3));
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

client::renderer::trident::particle::ParticleTypeId MagmaCubeEntity::getSquishParticle() const
{
    // 岩浆怪使用火焰粒子代替史莱姆粒子
    return client::renderer::trident::particle::ParticleTypeId::Flame;
}

void MagmaCubeEntity::registerAttributes()
{
    SlimeEntity::registerAttributes();

    // 移动速度固定为 0.2（不随尺寸变化）
    // 注意：父类 SlimeEntity::registerAttributes() 会设置尺寸相关属性
    // 这里需要确保护甲属性已注册
    m_attributes.registerAttribute(*entity::attribute::Attributes::armor());

    // 初始尺寸为1，护甲为3
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 3.0);
}

void MagmaCubeEntity::alterSquishAmount()
{
    // 挤压动画衰减更慢（史莱姆: 0.6, 岩浆怪: 0.9）
    setSquishAmount(squishAmount() * 0.9f);
}

// AbstractPiglinEntity
AbstractPiglinEntity::AbstractPiglinEntity(EntityId id)
    : MonsterEntity(id)
{
    setBurnsInDaylight(false);
}

void AbstractPiglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 所有猪灵类实体的共同目标

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, false));
}

// PiglinEntity
std::unique_ptr<Entity> PiglinEntity::create(IWorld* world)
{
    return std::make_unique<PiglinEntity>(EntityId(0));
}

PiglinEntity::PiglinEntity(EntityId id)
    : AbstractPiglinEntity(id)
{}

void PiglinEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    // 猪灵使用弩进行远程攻击
    // charge 参数对弩不重要，弩使用固定速度
    MC_UNUSED(charge);

    if (!target || !m_world) return;

    // 获取主手弩
    ItemStack& crossbow = const_cast<ItemStack&>(getMainHandItem());
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

    // 目标高度偏移：目标眼睛高度的 1/3
    // 弹道高度补偿：水平距离 * 0.2
    f64 dy = (target->y() + target->height() * 0.3333333333333333) - (y() + eyeHeight() - 0.15) + horizontalDist * 0.2;

    // 确定速度
    f32 velocity = 3.15f; // 箭矢速度

    // 计算不精确度：14 - difficulty.getId() * 4
    // Peaceful=14, Easy=10, Normal=6, Hard=2
    f32 inaccuracy = entity::combat::DifficultyHelper::getRangedAttackInaccuracy(m_world->difficulty());

    // 创建箭矢实体
    auto arrow = std::make_unique<entity::ArrowEntity>(EntityId(0));
    arrow->setWorld(m_world);
    arrow->setPosition(x(), y() + eyeHeight() - 0.15, z());
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
                auto type = entity->typeId();
                return type == entity::EntityTypeIdNumber::ZOMBIFIED_PIGLIN ||
                    type == entity::EntityTypeIdNumber::ZOGLIN;
            })));

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
                    return !player->isWearingGoldArmor();
                }));
    }
}

void PiglinEntity::registerAttributes()
{
    AbstractPiglinEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 16.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

// PiglinBruteEntity
std::unique_ptr<Entity> PiglinBruteEntity::create(IWorld* world)
{
    return std::make_unique<PiglinBruteEntity>(EntityId(0));
}

PiglinBruteEntity::PiglinBruteEntity(EntityId id)
    : AbstractPiglinEntity(id)
{}

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
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 50.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 7.0);
}

// ZombifiedPiglinEntity
std::unique_ptr<Entity> ZombifiedPiglinEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ZombifiedPiglinEntity>(EntityId(0));
}

ZombifiedPiglinEntity::ZombifiedPiglinEntity(EntityId id)
    : MonsterEntity(id)
{
    setBurnsInDaylight(false);
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

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
}

// HoglinEntity
std::unique_ptr<Entity> HoglinEntity::create(IWorld* world)
{
    return std::make_unique<HoglinEntity>(EntityId(0));
}

HoglinEntity::HoglinEntity(EntityId id)
    : MonsterEntity(id)
{
    setBurnsInDaylight(false);
    registerAttributes();
}

void HoglinEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    if (m_attackAnimationTicks > 0) {
        m_attackAnimationTicks--;
    }
}

bool HoglinEntity::attackLivingTarget(LivingEntity& target)
{
    if (m_attackCooldown > 0) {
        return false;
    }

    m_attackCooldown = 20;
    m_attackAnimationTicks = 10;
    return entity::IFlinging::attackWithFling(*this, target, m_isBaby);
}

void HoglinEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 成年疣猪兽对玩家敌对，幼年疣猪兽被动

    // 优先级 2: 近战攻击（仅成年）
    if (!m_isBaby) {
        m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, true));
    }

    // 优先级 5: 避开传送门（疣猪兽害怕传送门）
    // TODO: 实现 AvoidPortalGoal 当传送门系统完善后

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
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackKnockback());

    // 成年疣猪兽属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
}

// ZoglinEntity
std::unique_ptr<Entity> ZoglinEntity::create(IWorld* world)
{
    return std::make_unique<ZoglinEntity>(EntityId(0));
}

ZoglinEntity::ZoglinEntity(EntityId id)
    : MonsterEntity(id)
{
    registerAttributes();
}

void ZoglinEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackAnimationTicks > 0) {
        m_attackAnimationTicks--;
    }
}

bool ZoglinEntity::attackLivingTarget(LivingEntity& target)
{
    m_attackAnimationTicks = 10;
    return entity::IFlinging::attackWithFling(*this, target, m_isBaby);
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

        // 优先级 3: 攻击其他生物（排除僵尸疣兽和幼年生物）
        m_targetSelector.addGoal(3,
            std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(
                this, true, 10, entity::ai::goal::TargetPredicate([](const LivingEntity* entity) {
                    if (!entity || !entity->isAlive()) return false;
                    auto type = entity->typeId();
                    // 排除僵尸疣兽自己
                    if (type == entity::EntityTypeIdNumber::ZOGLIN) return false;
                    // TODO: 排除幼年生物和创造/旁观模式玩家
                    return true;
                })));
    }
}

void ZoglinEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 注册攻击属性（MonsterEntity 不自动注册这些）
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackKnockback());

    // 成年僵尸疣兽属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
}

} // namespace mc
