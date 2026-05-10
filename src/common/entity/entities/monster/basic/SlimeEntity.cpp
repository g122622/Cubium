#include "SlimeEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/EntityType.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
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
}

std::optional<ResourceLocation> SlimeEntity::getHurtSound(DamageSource& /*source*/) const {
	// MC 1.16.5: 小史莱姆用 hurt_small
	if (isSmallSlime()) {
		return makeSoundEventId("hurt_small");
	}
	// 大史莱姆用 hurt
	return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> SlimeEntity::getDeathSound() const {
	// MC 1.16.5: 小史莱姆用 death_small
	if (isSmallSlime()) {
		return makeSoundEventId("death_small");
	}
	// 大史莱姆用 death
	return makeSoundEventId("death");
}

std::optional<ResourceLocation> SlimeEntity::getSquishSound() const {
	// MC 1.16.5: 小史莱姆用 squish_small
	if (isSmallSlime()) {
		return makeSoundEventId("squish_small");
	}
	// 大史莱姆用 squish
	return makeSoundEventId("squish");
}

void SlimeEntity::alterSquishAmount() {
	// MC 1.16.5: alterSquishAmount()
	// 挤压量向 0 衰减
	m_squishAmount *= 0.6f;
}

void SlimeEntity::dealDamage(LivingEntity& target) {
	// MC 1.16.5: dealDamage()
	// 只有尺寸大于 1 的史莱姆才能造成伤害
	if (m_size <= 1) {
		return;
	}

	// 检查目标是否存活
	if (!target.isAlive()) {
		return;
	}

	// MC 1.16.5: 伤害值等于尺寸
	f32 damage = static_cast<f32>(m_size);

	// 对目标造成伤害
	auto damageSource = DamageSources::mobAttack(this);
	target.hurt(damageSource, damage);
}

bool SlimeEntity::canDamagePlayer() const {
	// MC 1.16.5: 只有尺寸大于 1 的史莱姆才能伤害玩家
	return m_size > 1;
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

		// MC 1.16.5: 生成粒子效果
		// 参考: SlimeEntity.tick() - for (int j = 0; j < size * 8; ++j)
		if (world() != nullptr && world()->isClientSide()) {
			using namespace mc::client::renderer::trident::particle;
			math::Random& random = world()->getRandom();

			// 粒子数量 = 尺寸 * 8
			i32 particleCount = m_size * 8;
			for (i32 j = 0; j < particleCount; ++j) {
				// MC 1.16.5: 随机角度和半径
				f32 angle = random.nextFloat() * 2.0f * 3.14159265f;  // 0 to 2*PI
				f32 radiusFactor = random.nextFloat() * 0.5f + 0.5f;  // 0.5 to 1.0

				// 计算粒子位置偏移
				f32 offsetX = std::sin(static_cast<f64>(angle)) * static_cast<f32>(m_size) * 0.5f * radiusFactor;
				f32 offsetZ = std::cos(static_cast<f64>(angle)) * static_cast<f32>(m_size) * 0.5f * radiusFactor;

				// 在史莱姆脚底生成粒子
				world()->addParticle(
					ParticleTypeId::ItemSlime,
					Vector3(x() + static_cast<f64>(offsetX), y(), z() + static_cast<f64>(offsetZ)),
					Vector3(0.0, 0.0, 0.0)
				);
			}
		}
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

	// 经验值等于尺寸
	m_experienceValue = m_size;
}

void SlimeEntity::remove() {
	// MC 1.16.5: 在移除前尝试分裂
	// 只有尺寸大于 1 的史莱姆才会分裂
	if (canSplit()) {
		performSplit();
	}

	// 调用父类移除
	MonsterEntity::remove();
}

void SlimeEntity::performSplit() {
	// MC 1.16.5: performSplit()
	// 只能在服务端执行
	if (world() == nullptr || world()->isClientSide()) {
		return;
	}

	// 分裂后的小史莱姆数量：2-4 个
	math::Random& rng = world()->getRandom();
	i32 splitCount = rng.nextInt(SPLIT_COUNT_MIN, SPLIT_COUNT_MAX);

	// 新史莱姆的尺寸 = 当前尺寸 / 2
	i32 newSize = m_size / 2;
	if (newSize < 1) {
		return;  // 不能分裂成更小的史莱姆
	}

	// 生成小史莱姆
	for (i32 i = 0; i < splitCount; ++i) {
		// TODO: 创建新的史莱姆实体
		// 需要实体工厂方法
		// auto smallSlime = world->createEntity<SlimeEntity>(...);
		// smallSlime->setSlimeSize(newSize, true);
		// smallSlime->setPosition(x() + (i - splitCount/2) * 0.5, y(), z() + (i - splitCount/2) * 0.5);
		// smallSlime->setVelocity(
		//     (rng.nextFloat() - 0.5f) * 0.5f,
		//     rng.nextFloat() * 0.5f,
		//     (rng.nextFloat() - 0.5f) * 0.5f
		// );
	}
}

} // namespace mc
