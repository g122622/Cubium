#include "SlimeEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include <cmath>

namespace mc {

SlimeEntity::SlimeEntity(LegacyEntityType type, EntityId id)
	: MonsterEntity(type, id)
{
	// MC 1.16.5: 史莱姆不在阳光下燃烧
	setBurnsInDaylight(false);

	// 注册 AI 目标
	registerGoals();

	// 注册属性
	registerAttributes();
}

std::unique_ptr<Entity> SlimeEntity::create(IWorld* /*world*/) {
	return std::make_unique<SlimeEntity>(LegacyEntityType::Slime, EntityId(0));
}

void SlimeEntity::setSlimeSize(i32 size, bool resetHealth) {
	i32 clampedSize = std::clamp(size, 1, 4);
	if (m_size == clampedSize) {
		return;
	}

	m_size = clampedSize;
	updateSizeAttributes();
	refreshDimensions();

	// MC 1.16.5: 重置生命值
	if (resetHealth) {
		setHealth(maxHealth());
	}

	// MC 1.16.5: 经验值等于尺寸
	setExperienceValue(m_size);
}

std::optional<ResourceLocation> SlimeEntity::getHurtSound(DamageSource& /*source*/) const {
	// MC 1.16.5: 小史莱姆用 hurt_small
	if (isSmallSlime()) {
		return makeSoundEventId("hurt_small");
	}
	return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> SlimeEntity::getDeathSound() const {
	// MC 1.16.5: 小史莱姆用 death_small
	if (isSmallSlime()) {
		return makeSoundEventId("death_small");
	}
	return makeSoundEventId("death");
}

std::optional<ResourceLocation> SlimeEntity::getSquishSound() const {
	// MC 1.16.5: 小史莱姆用 squish_small
	if (isSmallSlime()) {
		return makeSoundEventId("squish_small");
	}
	return makeSoundEventId("squish");
}

std::optional<ResourceLocation> SlimeEntity::getJumpSound() const {
	// MC 1.16.5: 小史莱姆用 jump_small
	if (isSmallSlime()) {
		return makeSoundEventId("jump_small");
	}
	return makeSoundEventId("jump");
}

bool SlimeEntity::canDamagePlayer() const {
	// MC 1.16.5: !isSmallSlime() && isServerWorld()
	return !isSmallSlime();
}

i32 SlimeEntity::getJumpDelay() const {
	// MC 1.16.5: rand.nextInt(20) + 10
	math::Random rng(ticksExisted());
	return rng.nextInt(10, 30);
}

void SlimeEntity::split() {
	// MC 1.16.5: 在 remove() 中分裂
	if (!canSplit()) {
		return;
	}

	// TODO: 生成 2-4 个小史莱姆
	// i32 smallSize = m_size / 2;
	// for (int i = 0; i < 2 + rand() % 3; i++) {
	//     auto smallSlime = std::make_unique<SlimeEntity>(LegacyEntityType::Slime, EntityId(0));
	//     smallSlime->setSlimeSize(smallSize, true);
	//     // 在附近生成
	//     world->spawnEntity(std::move(smallSlime));
	// }
}

void SlimeEntity::dealDamage(LivingEntity& target) {
	// MC 1.16.5: dealDamage()
	if (!isAlive()) {
		return;
	}

	i32 size = getSlimeSize();
	f32 distanceSq = distanceSqTo(target);

	// MC 1.16.5: 距离检查 < 0.6 * size * 0.6 * size
	f32 maxDistance = 0.6f * static_cast<f32>(size);
	if (distanceSq < maxDistance * maxDistance && canSee(target)) {
		// TODO: 造成伤害
		// f32 damage = static_cast<f32>(getAttributeValue(Attributes::ATTACK_DAMAGE));
		// target.hurt(DamageSource::mobAttack(this), damage);
		playSound(SoundEvents::ENTITY_SLIME_ATTACK, 1.0f, 1.0f);

		// TODO: 应用附魔效果
		// applyEnchantments(this, target);
	}
}

void SlimeEntity::onCollideWithPlayer(LivingEntity& player) {
	// MC 1.16.5: onCollideWithPlayer()
	if (canDamagePlayer()) {
		dealDamage(player);
	}
}

f32 SlimeEntity::eyeHeight() const {
	// MC 1.16.5: 0.625F * height
	return EYE_HEIGHT_FACTOR * height();
}

entity::EntitySize SlimeEntity::getDimensions(EntityPose /*pose*/) const {
	// MC 1.16.5: scale by 0.255F * size
	f32 scaleFactor = SIZE_SCALE * static_cast<f32>(m_size);
	return entity::EntitySize::flexible(0.6f * scaleFactor, 0.6f * scaleFactor);
}

void SlimeEntity::dropExperience() {
	// MC 1.16.5: 经验值等于尺寸
	MonsterEntity::dropExperience();
}

void SlimeEntity::tick() {
	// MC 1.16.5 SlimeEntity.tick()

	// 更新挤压动画
	m_squishFactor += (m_squishAmount - m_squishFactor) * 0.5f;
	m_prevSquishFactor = m_squishFactor;

	MonsterEntity::tick();

	// 着地时的挤压效果
	if (onGround() && !m_wasOnGround) {
		// MC 1.16.5: 着地时播放挤压音效和粒子
		auto squishSound = getSquishSound();
		if (squishSound) {
			playSound(*squishSound, getSoundVolume(), 1.0f);
		}

		// 挤压量设为负值
		m_squishAmount = -0.5f;

		// TODO: 生成粒子效果
		// for (int j = 0; j < size * 8; ++j) {
		//     world->addParticle(ParticleTypes::ITEM_SLIME, ...);
		// }
	} else if (!onGround() && m_wasOnGround) {
		// MC 1.16.5: 离地时的挤压量
		m_squishAmount = 1.0f;
	}

	m_wasOnGround = onGround();
	alterSquishAmount();
}

void SlimeEntity::registerGoals() {
	MonsterEntity::registerGoals();

	// MC 1.16.5 SlimeEntity.registerGoals()
	// 优先级 1: FloatGoal（游泳）
	// 优先级 2: AttackGoal（攻击）
	// 优先级 3: FaceRandomGoal（随机转向）
	// 优先级 5: HopGoal（跳跃）
	//
	// 目标选择器：
	// 优先级 1: NearestAttackableTargetGoal<Player>（攻击玩家，高度差<=4）
	// 优先级 3: NearestAttackableTargetGoal<IronGolem>（攻击铁傀儡）

	// TODO: 实现史莱姆特有的AI目标
	// m_goalSelector.addGoal(1, new SlimeFloatGoal(this));
	// m_goalSelector.addGoal(2, new SlimeAttackGoal(this));
	// m_goalSelector.addGoal(3, new SlimeFaceRandomGoal(this));
	// m_goalSelector.addGoal(5, new SlimeHopGoal(this));
	// m_targetSelector.addGoal(1, new NearestAttackableTargetGoal<Player>(this));
	// m_targetSelector.addGoal(3, new NearestAttackableTargetGoal<IronGolem>(this));
}

void SlimeEntity::registerAttributes() {
	MonsterEntity::registerAttributes();

	// MC 1.16.5: 默认尺寸为1
	m_size = 1;
	updateSizeAttributes();
}

void SlimeEntity::updateSizeAttributes() {
	// MC 1.16.5: 根据尺寸更新属性
	// HP = size * size
	// Speed = 0.2 + 0.1 * size
	// AttackDamage = size
	f32 health = static_cast<f32>(m_size * m_size);
	f32 speed = 0.2f + 0.1f * static_cast<f32>(m_size);
	f32 damage = static_cast<f32>(m_size);

	m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, health);
	m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, speed);
	m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, damage);
}

void SlimeEntity::alterSquishAmount() {
	// MC 1.16.5: 挤压量衰减
	m_squishAmount *= 0.6f;
}

} // namespace mc
