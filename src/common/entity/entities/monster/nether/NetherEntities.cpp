#include "NetherEntities.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../../world/IWorld.hpp"

namespace mc {

// GhastEntity
std::unique_ptr<Entity> GhastEntity::create(IWorld* world) {
    return std::make_unique<GhastEntity>(LegacyEntityType::Ghast, EntityId(0));
}

GhastEntity::GhastEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    setBurnsInDaylight(false);
}

void GhastEntity::tick() {
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

void GhastEntity::shootFireball() {
    // TODO: 实现发射火球
}

void GhastEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加恶魂特有AI
}

void GhastEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.9);
}

// MagmaCubeEntity
std::unique_ptr<Entity> MagmaCubeEntity::create(IWorld* world) {
    return std::make_unique<MagmaCubeEntity>(LegacyEntityType::MagmaCube, EntityId(0));
}

MagmaCubeEntity::MagmaCubeEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
}

void MagmaCubeEntity::setSize(i32 size) {
    m_size = size;
    // TODO: 更新碰撞箱和属性
}

void MagmaCubeEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加岩浆怪特有AI
}

void MagmaCubeEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 16.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
}

// AbstractPiglinEntity
AbstractPiglinEntity::AbstractPiglinEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    setBurnsInDaylight(false);
}

void AbstractPiglinEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加猪灵基础AI
}

// PiglinEntity
std::unique_ptr<Entity> PiglinEntity::create(IWorld* world) {
    return std::make_unique<PiglinEntity>(LegacyEntityType::Piglin, EntityId(0));
}

PiglinEntity::PiglinEntity(LegacyEntityType type, EntityId id)
    : AbstractPiglinEntity(type, id)
{
}

void PiglinEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge) {
    // TODO: 实现弩攻击逻辑
    (void)target;
    (void)charge;
}

void PiglinEntity::onCrossbowLoadComplete(ItemStack& crossbow) {
    // TODO: 实现弩装填完成逻辑
    (void)crossbow;
}

void PiglinEntity::shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge) {
    // TODO: 实现弩射击逻辑
    (void)target;
    (void)crossbow;
    (void)charge;
}

void PiglinEntity::registerGoals() {
    AbstractPiglinEntity::registerGoals();
    // TODO: 添加猪灵特有AI
}

void PiglinEntity::registerAttributes() {
    AbstractPiglinEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 16.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
}

// PiglinBruteEntity
std::unique_ptr<Entity> PiglinBruteEntity::create(IWorld* world) {
    return std::make_unique<PiglinBruteEntity>(LegacyEntityType::PiglinBrute, EntityId(0));
}

PiglinBruteEntity::PiglinBruteEntity(LegacyEntityType type, EntityId id)
    : AbstractPiglinEntity(type, id)
{
}

void PiglinBruteEntity::registerGoals() {
    AbstractPiglinEntity::registerGoals();
    // TODO: 添加猪灵蛮兵特有AI
}

void PiglinBruteEntity::registerAttributes() {
    AbstractPiglinEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 50.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 13.0); // 金斧伤害
}

// ZombifiedPiglinEntity
ZombifiedPiglinEntity::ZombifiedPiglinEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    setBurnsInDaylight(false);
}

void ZombifiedPiglinEntity::tick() {
    MonsterEntity::tick();

    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_angry = false;
        }
    }
}

void ZombifiedPiglinEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加僵尸猪灵特有AI
}

void ZombifiedPiglinEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
}

// HoglinEntity
std::unique_ptr<Entity> HoglinEntity::create(IWorld* world) {
    return std::make_unique<HoglinEntity>(LegacyEntityType::Hoglin, EntityId(0));
}

HoglinEntity::HoglinEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    setBurnsInDaylight(false);
    registerAttributes();
}

void HoglinEntity::tick() {
    MonsterEntity::tick();

    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    if (m_attackAnimationTicks > 0) {
        m_attackAnimationTicks--;
    }
}

bool HoglinEntity::attackLivingTarget(LivingEntity& target) {
    if (m_attackCooldown > 0) {
        return false;
    }

    m_attackCooldown = 20;
    m_attackAnimationTicks = 10;
    return entity::IFlinging::attackWithFling(*this, target, m_isBaby);
}

void HoglinEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加疣猪兽特有AI
}

void HoglinEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    // 注册攻击属性（MonsterEntity 不自动注册这些）
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackKnockback());

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0); // 成年伤害
}

// ZoglinEntity
std::unique_ptr<Entity> ZoglinEntity::create(IWorld* world) {
    return std::make_unique<ZoglinEntity>(LegacyEntityType::Zoglin, EntityId(0));
}

ZoglinEntity::ZoglinEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    registerAttributes();
}

void ZoglinEntity::tick() {
    MonsterEntity::tick();

    if (m_attackAnimationTicks > 0) {
        m_attackAnimationTicks--;
    }
}

bool ZoglinEntity::attackLivingTarget(LivingEntity& target) {
    m_attackAnimationTicks = 10;
    return entity::IFlinging::attackWithFling(*this, target, m_isBaby);
}

void ZoglinEntity::registerGoals() {
    MonsterEntity::registerGoals();
    // TODO: 添加僵尸疣兽特有AI
}

void ZoglinEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    // 注册攻击属性（MonsterEntity 不自动注册这些）
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackKnockback());

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0);
}

} // namespace mc
