#include "WitherEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../ai/goal/goals/LookRandomlyGoal.hpp"
#include <cmath>

namespace mc {
namespace entity {

std::unique_ptr<Entity> WitherEntity::create(IWorld* /*world*/) {
    return std::make_unique<WitherEntity>(LegacyEntityType::Wither, EntityId(0));
}

WitherEntity::WitherEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
    // MC 1.16.5: 设置经验和无敌状态
    setExperienceValue(50);
    // TODO: setNoGravity(true) - 凋灵可以飞行
    // TODO: getNavigator().setCanSwim(true)

    registerAttributes();
    registerGoals();
}

std::optional<ResourceLocation> WitherEntity::getAmbientSound() const {
    // MC 1.16.5: entity.wither.ambient
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> WitherEntity::getHurtSound(DamageSource& /*source*/) const {
    // MC 1.16.5: entity.wither.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> WitherEntity::getDeathSound() const {
    // MC 1.16.5: entity.wither.death
    return makeSoundEventId("death");
}

bool WitherEntity::isInvulnerableTo(DamageSource& source) const {
    // MC 1.16.5 WitherEntity.attackEntityFrom()
    // 凋灵免疫溺水伤害
    if (source.type() == DamageType::Drown) {
        return true;
    }

    // 凋灵免疫凋零伤害
    if (source.type() == DamageType::Wither) {
        return true;
    }

    // 无敌阶段免疫所有伤害（除了虚空伤害）
    if (m_invulTime > 0 && source.type() != DamageType::OutOfWorld) {
        return true;
    }

    // TODO: 免疫其他凋灵的伤害
    // TODO: 充能状态下免疫箭矢伤害

    return MobEntity::isInvulnerableTo(source);
}

bool WitherEntity::canRangedAttack() const {
    // 无敌阶段不能远程攻击
    return m_invulTime <= 0;
}

std::string WitherEntity::getBossName() const {
    // TODO: 返回自定义名称或默认名称
    return "Wither";
}

i32 WitherEntity::getWatchedTargetId(i32 head) const {
    // MC 1.16.5: 从数据管理器获取头部目标
    // TODO: 使用 EntityDataManager
    return 0;
}

void WitherEntity::updateWatchedTargetId(i32 head, i32 targetId) {
    // MC 1.16.5: 更新数据管理器中的头部目标
    // TODO: 使用 EntityDataManager
}

void WitherEntity::launchWitherSkullToEntity(i32 head, LivingEntity* target) {
    // MC 1.16.5: launchWitherSkullToEntity()
    if (!target || !target->isAlive()) {
        return;
    }

    // 计算发射位置
    f32 headX = getHeadX(head);
    f32 headY = getHeadY(head);
    f32 headZ = getHeadZ(head);

    // 计算发射方向
    f32 dx = target->x() - headX;
    f32 dy = target->y() + target->eyeHeight() / 2.0f - headY;
    f32 dz = target->z() - headZ;

    f32 dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist > 0.0f) {
        dx /= dist;
        dy /= dist;
        dz /= dist;
    }

    // 决定是否发射蓝色凋灵之首
    // MC 1.16.5: 主头有 0.1% 概率发射蓝色凋灵之首
    bool isBlue = false;
    if (head == 0) {
        // 主头：充能时或随机发射蓝色
        math::Random rng(ticksExisted());
        if (isCharged() || rng.nextFloat() < 0.001f) {
            isBlue = true;
        }
    }

    // TODO: 创建凋灵之首实体
    // auto skull = std::make_unique<WitherSkullEntity>(...);
    // skull->setPosition(headX, headY, headZ);
    // skull->setShooter(this);
    // skull->shoot(dx, dy, dz, 1.5f, 0.0f);
    // skull->setBlue(isBlue);
    // world->spawnEntity(std::move(skull));

    // 播放发射音效
    playSound(SoundEvents::ENTITY_WITHER_SHOOT, 1.0f, 1.0f);
}

void WitherEntity::ignite() {
    // MC 1.16.5: 开始生成序列
    m_invulTime = INVULNERABILITY_TIME;
    setHealth(maxHealth() / 3.0f);
}

void WitherEntity::tick() {
    // MC 1.16.5 WitherEntity.tick()

    // 更新上一帧头部角度
    for (i32 i = 0; i < 2; ++i) {
        m_prevHeadXRot[i] = m_headXRot[i];
        m_prevHeadYRot[i] = m_headYRot[i];
    }

    // 调用父类tick
    MobEntity::tick();

    // 更新AI任务
    updateAITasks();
}

void WitherEntity::updateAITasks() {
    // MC 1.16.5 WitherEntity.updateAITasks()

    // 无敌阶段处理
    if (m_invulTime > 0) {
        m_invulTime--;

        // 每10tick恢复10点生命值
        if (m_invulTime % 10 == 0) {
            heal(10.0f);
        }

        // 无敌阶段结束时的爆炸
        if (m_invulTime == 0) {
            explodeOnSpawn();
        }
    }

    // 更新头部目标
    updateHeadTargets();

    // 方块破坏逻辑
    if (m_blockBreakCounter > 0) {
        m_blockBreakCounter--;
        if (m_blockBreakCounter == 0) {
            breakNearbyBlocks();
        }
    }

    // 生命值一半以下时持续恢复
    if (isCharged() && ticksExisted() % 20 == 0) {
        heal(1.0f);
    }
}

void WitherEntity::updateHeadTargets() {
    // MC 1.16.5: 更新三个头的目标
    // 主头追踪攻击目标
    // 侧头追踪其他目标

    // TODO: 实现完整的目标追踪逻辑
    // 1. 主头追踪 getAttackTarget()
    // 2. 侧头每20tick更新一次目标，追踪最近的非亡灵生物
}

f32 WitherEntity::getHeadX(i32 head) const {
    // MC 1.16.5: 计算头的X坐标
    if (head == 0) {
        // 主头
        return static_cast<f32>(x());
    } else if (head == 1) {
        // 左头
        return static_cast<f32>(x() + 0.5 * std::cos(yRot() * math::DEG_TO_RAD));
    } else {
        // 右头
        return static_cast<f32>(x() - 0.5 * std::cos(yRot() * math::DEG_TO_RAD));
    }
}

f32 WitherEntity::getHeadY(i32 head) const {
    // MC 1.16.5: 计算头的Y坐标
    return static_cast<f32>(y() + 2.0);
}

f32 WitherEntity::getHeadZ(i32 head) const {
    // MC 1.16.5: 计算头的Z坐标
    if (head == 0) {
        // 主头
        return static_cast<f32>(z());
    } else if (head == 1) {
        // 左头
        return static_cast<f32>(z() + 0.5 * std::sin(yRot() * math::DEG_TO_RAD));
    } else {
        // 右头
        return static_cast<f32>(z() - 0.5 * std::sin(yRot() * math::DEG_TO_RAD));
    }
}

void WitherEntity::breakNearbyBlocks() {
    // MC 1.16.5: 破坏凋灵周围的方块
    // TODO: 检查 mobGriefing 游戏规则
    // TODO: 破坏 1x2x1 范围内的方块（凋灵免疫标签除外）
}

void WitherEntity::explodeOnSpawn() {
    // MC 1.16.5: 生成时创建7.0威力的爆炸
    // TODO: world->createExplosion(this, x(), y(), z(), 7.0f, ...);

    // TODO: 播放生成音效
    // world->playBroadcastSound(this, SoundEvents.ENTITY_WITHER_SPAWN, ...);
}

void WitherEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/) {
    // MC 1.16.5: 主头发射凋灵之首
    launchWitherSkullToEntity(0, target);
}

void WitherEntity::registerGoals() {
    MobEntity::registerGoals();

    // MC 1.16.5 WitherEntity.registerGoals()
    // 优先级 0: DoNothingGoal（无敌阶段什么都不做）
    // 优先级 2: RangedAttackGoal（远程攻击）
    // 优先级 5: WaterAvoidingRandomWalkingGoal（避水随机行走）
    // 优先级 6: LookAtGoal（看向玩家）
    // 优先级 7: LookRandomlyGoal（随机看向）
    //
    // 目标选择器：
    // 优先级 1: HurtByTargetGoal（被攻击反击）
    // 优先级 2: NearestAttackableTargetGoal<MobEntity>（攻击非亡灵生物）

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f,
        [](const LivingEntity* /*entity*/) -> bool {
            // 只看向玩家
            // TODO: 检查是否是玩家
            return true;
        }));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));
}

void WitherEntity::registerAttributes() {
    MobEntity::registerAttributes();

    // MC 1.16.5 WitherEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 300.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 40.0);  // MC 1.16.5: 40 而非 64
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 4.0);          // MC 1.16.5: 4 点护甲

    // TODO: 凋灵可以飞行
    // setNoGravity(true);
}

} // namespace entity
} // namespace mc
