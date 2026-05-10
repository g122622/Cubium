#include "BlazeEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <cmath>

namespace mc {

BlazeEntity::BlazeEntity(LegacyEntityType type, EntityId id)
	: MonsterEntity(type, id)
{
	// MC 1.16.5: 烈焰人不在阳光下燃烧
	setBurnsInDaylight(false);

	// 注册 AI 目标
	registerGoals();

	// 注册属性
	registerAttributes();

	// 经验值
	setExperienceValue(10);
}

std::unique_ptr<Entity> BlazeEntity::create(IWorld* /*world*/) {
	return std::make_unique<BlazeEntity>(LegacyEntityType::Blaze, EntityId(0));
}

std::optional<ResourceLocation> BlazeEntity::getAmbientSound() const {
	// MC 1.16.5: entity.blaze.ambient
	return SoundEvents::ENTITY_BLAZE_AMBIENT;
}

std::optional<ResourceLocation> BlazeEntity::getHurtSound(DamageSource& /*source*/) const {
	// MC 1.16.5: entity.blaze.hurt
	return SoundEvents::ENTITY_BLAZE_HURT;
}

std::optional<ResourceLocation> BlazeEntity::getDeathSound() const {
	// MC 1.16.5: entity.blaze.death
	return SoundEvents::ENTITY_BLAZE_DEATH;
}

void BlazeEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/) {
	// MC 1.16.5: 发射小火球
	if (!target || !target->isAlive()) {
		return;
	}

	// TODO: 创建 SmallFireballEntity 并发射
	// SmallFireballEntity* fireball = new SmallFireballEntity(world, this, dx, dy, dz);
	// fireball->setPosition(x, y + 0.5, z);
	// world->spawnEntity(fireball);

	m_fireballCount--;
	if (m_fireballCount <= 0) {
		m_charging = false;
		m_attackStep = 0;
		m_attackTime = ATTACK_COOLDOWN;
	}
}

void BlazeEntity::tick() {
	// MC 1.16.5 BlazeEntity.tick()

	// 空中悬浮减速
	if (!onGround() && velocityY() < 0.0f) {
		// MC 1.16.5: motion.y *= 0.6
		setVelocity(velocityX(), velocityY() * 0.6f, velocityZ());
	}

	// MC 1.16.5: 客户端粒子效果和音效
	if (world() != nullptr && world()->isClientSide()) {
		math::Random& random = world()->getRandom();

		// 随机播放燃烧音效（24分之1概率）
		if (random.nextInt(24) == 0 && !isSilent()) {
			world()->playSound(
				SoundEvents::ENTITY_BLAZE_BURN,
				sound::SoundCategory::Hostile,
				m_position,
				1.0f + random.nextFloat() * 0.3f,  // 音量
				random.nextFloat() * 0.7f + 0.3f    // 音调
			);
		}

		// 生成烟雾粒子
		using namespace client::renderer::trident::particle;
		for (i32 i = 0; i < 2; ++i) {
			f32 px = static_cast<f32>(x()) + (random.nextFloat() - 0.5f) * width();
			f32 py = static_cast<f32>(y()) + random.nextFloat() * height();
			f32 pz = static_cast<f32>(z()) + (random.nextFloat() - 0.5f) * width();
			world()->addParticle(
				ParticleTypeId::LargeSmoke,
				Vector3(px, py, pz),
				Vector3(0.0, 0.0, 0.0)
			);
		}
	}

	// 更新攻击冷却
	if (m_attackTime > 0) {
		m_attackTime--;
	}

	MonsterEntity::tick();
}

void BlazeEntity::registerGoals() {
	MonsterEntity::registerGoals();

	// MC 1.16.5 BlazeEntity.registerGoals()
	// 优先级 4: FireballAttackGoal（火球攻击）
	// 优先级 5: MoveTowardsRestrictionGoal（向限制点移动）
	// 优先级 7: WaterAvoidingRandomWalkingGoal（避水随机行走）
	// 优先级 8: LookAtGoal（看向玩家）
	// 优先级 8: LookRandomlyGoal（随机看向）
	//
	// 目标选择器：
	// 优先级 1: HurtByTargetGoal（被攻击反击，呼唤同伴）
	// 优先级 2: NearestAttackableTargetGoal<Player>（攻击玩家）

	// TODO: 实现烈焰人特有的AI目标
	// m_goalSelector.addGoal(4, new BlazeFireballAttackGoal(this));
	// m_targetSelector.addGoal(1, new HurtByTargetGoal(this).setCallsForHelp());
	// m_targetSelector.addGoal(2, new NearestAttackableTargetGoal<Player>(this));
}

void BlazeEntity::registerAttributes() {
	MonsterEntity::registerAttributes();

	// MC 1.16.5 BlazeEntity 属性
	m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
	m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
	m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
	m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 48.0);
}

void BlazeEntity::updateAITasks() {
	// MC 1.16.5: 更新 AI 任务
	// 如果正在充能且攻击时间 > 0，递减攻击时间
	// 如果攻击时间 <= 0 且充能状态，发射火球
}

} // namespace mc
