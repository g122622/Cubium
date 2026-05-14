#include "WitherEntity.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/gamerule/GameRules.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../damage/DamageSource.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ========== 静态数据参数定义 ==========
// MC 1.16.5: FIRST_HEAD_TARGET, SECOND_HEAD_TARGET, THIRD_HEAD_TARGET
DataParameter<i32> WitherEntity::HEAD_TARGET_1 = EntityDataManager::createKey<i32>();
DataParameter<i32> WitherEntity::HEAD_TARGET_2 = EntityDataManager::createKey<i32>();
DataParameter<i32> WitherEntity::HEAD_TARGET_3 = EntityDataManager::createKey<i32>();

std::unique_ptr<Entity> WitherEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitherEntity>(LegacyEntityType::Wither, EntityId(0));
}

WitherEntity::WitherEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
    // MC 1.16.5: 设置经验和无敌状态
    setExperienceValue(50);
    // TODO: setNoGravity(true) - 凋灵可以飞行
    // TODO: getNavigator().setCanSwim(true)

    registerData();
    registerAttributes();
    registerGoals();
}

std::optional<ResourceLocation> WitherEntity::getAmbientSound() const
{
    // MC 1.16.5: entity.wither.ambient
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> WitherEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: entity.wither.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> WitherEntity::getDeathSound() const
{
    // MC 1.16.5: entity.wither.death
    return makeSoundEventId("death");
}

bool WitherEntity::isInvulnerableTo(DamageSource& source) const
{
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

bool WitherEntity::canRangedAttack() const
{
    // 无敌阶段不能远程攻击
    return m_invulTime <= 0;
}

std::string WitherEntity::getBossName() const
{
    // TODO: 返回自定义名称或默认名称
    return "Wither";
}

i32 WitherEntity::getWatchedTargetId(i32 head) const
{
    // MC 1.16.5: 从数据管理器获取头部目标
    // head: 0=主头, 1=左头, 2=右头
    switch (head) {
        case 0:
            return m_dataManager.get<i32>(HEAD_TARGET_1);
        case 1:
            return m_dataManager.get<i32>(HEAD_TARGET_2);
        case 2:
            return m_dataManager.get<i32>(HEAD_TARGET_3);
        default:
            return 0;
    }
}

void WitherEntity::updateWatchedTargetId(i32 head, i32 targetId)
{
    // MC 1.16.5: 更新数据管理器中的头部目标
    // head: 0=主头, 1=左头, 2=右头
    switch (head) {
        case 0:
            m_dataManager.set(HEAD_TARGET_1, targetId);
            break;
        case 1:
            m_dataManager.set(HEAD_TARGET_2, targetId);
            break;
        case 2:
            m_dataManager.set(HEAD_TARGET_3, targetId);
            break;
        default:
            break;
    }
}

void WitherEntity::registerData()
{
    // MC 1.16.5 WitherEntity.registerData()
    // 调用父类注册数据参数
    MobEntity::registerData();

    // 注册三个头的追踪目标实体ID
    // 初始值为0表示无目标
    m_dataManager.registerParam(HEAD_TARGET_1, 0);
    m_dataManager.registerParam(HEAD_TARGET_2, 0);
    m_dataManager.registerParam(HEAD_TARGET_3, 0);
}

void WitherEntity::launchWitherSkullToEntity(i32 head, LivingEntity* target)
{
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
    static_cast<void>(isBlue);

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

void WitherEntity::ignite()
{
    // MC 1.16.5: 开始生成序列
    m_invulTime = INVULNERABILITY_TIME;
    setHealth(maxHealth() / 3.0f);
}

void WitherEntity::tick()
{
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

void WitherEntity::updateAITasks()
{
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

void WitherEntity::updateHeadTargets()
{
    // MC 1.16.5: 更新三个头的目标
    // 主头追踪 getAttackTarget()
    // 侧头每20tick更新一次目标，追踪最近的非亡灵生物

    // 主头：直接追踪攻击目标
    LivingEntity* attackTarget = this->attackTarget();
    if (attackTarget != nullptr && attackTarget->isAlive()) {
        updateWatchedTargetId(0, static_cast<i32>(attackTarget->id()));
    } else {
        updateWatchedTargetId(0, 0);
    }

    // 侧头：周期性更新目标
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    for (i32 i = 1; i < 3; ++i) {
        // 检查更新时间
        if (static_cast<i32>(ticksExisted()) < m_nextHeadUpdate[i - 1]) {
            continue;
        }

        // 设置下次更新时间：10-20 tick后
        math::Random rng = getRandom();
        m_nextHeadUpdate[i - 1] = static_cast<i32>(ticksExisted()) + 10 + rng.nextInt(10);

        // 获取当前追踪目标
        i32 currentTargetId = getWatchedTargetId(i);
        if (currentTargetId > 0) {
            // 检查当前目标是否仍然有效
            Entity* currentTarget = worldPtr->getEntity(static_cast<EntityId>(currentTargetId));
            if (currentTarget != nullptr && currentTarget->isAlive()) {
                LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(currentTarget);
                if (livingTarget != nullptr) {
                    // 检查距离和视线
                    f32 distSq = distanceSqTo(*livingTarget);
                    if (distSq <= 900.0f && canSee(*livingTarget)) { // 30格距离
                        // 目标有效，发射凋灵之首
                        launchWitherSkullToEntity(i, livingTarget);
                        // 设置攻击冷却：40-60 tick
                        m_nextHeadUpdate[i - 1] = static_cast<i32>(ticksExisted()) + 40 + rng.nextInt(20);
                        continue;
                    }
                }
            }
            // 目标无效，清除
            updateWatchedTargetId(i, 0);
        }

        // 无目标，搜索新目标
        // MC 1.16.5: 搜索范围 20x8x20，目标条件：非亡灵生物、可攻击、可见
        AxisAlignedBB searchBox = boundingBox().expand(20.0f, 8.0f, 20.0f);
        std::vector<Entity*> nearbyEntities = worldPtr->getEntitiesInAABB(searchBox, this);

        LivingEntity* bestTarget = nullptr;
        f32 bestDistSq = std::numeric_limits<f32>::max();

        for (Entity* entity : nearbyEntities) {
            if (entity == nullptr || entity == this) {
                continue;
            }

            LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
            if (living == nullptr) {
                continue;
            }

            // 排除亡灵生物
            if (living->getCreatureAttribute() == CreatureAttribute::Undead) {
                continue;
            }

            // 检查可攻击性
            if (!living->isAlive()) {
                continue;
            }

            // 检查视线
            if (!canSee(*living)) {
                continue;
            }

            // 检查是否是创造模式玩家（创造模式玩家不可被攻击）
            if (living->legacyType() == LegacyEntityType::Player) {
                // TODO: 检查玩家的游戏模式
                // 暂时跳过这个检查
            }

            f32 distSq = distanceSqTo(*living);
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                bestTarget = living;
            }
        }

        if (bestTarget != nullptr) {
            updateWatchedTargetId(i, static_cast<i32>(bestTarget->id()));
        }
    }
}

f32 WitherEntity::getHeadX(i32 head) const
{
    // MC 1.16.5: 计算头的X坐标
    if (head == 0) {
        // 主头
        return static_cast<f32>(x());
    } else if (head == 1) {
        // 左头
        return static_cast<f32>(x() + 0.5 * std::cos(yaw() * math::DEG_TO_RAD));
    } else {
        // 右头
        return static_cast<f32>(x() - 0.5 * std::cos(yaw() * math::DEG_TO_RAD));
    }
}

f32 WitherEntity::getHeadY(i32 head) const
{
    // MC 1.16.5: 计算头的Y坐标
    return static_cast<f32>(y() + 2.0);
}

f32 WitherEntity::getHeadZ(i32 head) const
{
    // MC 1.16.5: 计算头的Z坐标
    if (head == 0) {
        // 主头
        return static_cast<f32>(z());
    } else if (head == 1) {
        // 左头
        return static_cast<f32>(z() + 0.5 * std::sin(yaw() * math::DEG_TO_RAD));
    } else {
        // 右头
        return static_cast<f32>(z() - 0.5 * std::sin(yaw() * math::DEG_TO_RAD));
    }
}

void WitherEntity::breakNearbyBlocks()
{
    // MC 1.16.5: 破坏凋灵周围的方块
    // 检查 mobGriefing 游戏规则
    IWorld* worldPtr = world();
    if (!worldPtr || !worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return;
    }
    // TODO: 破坏 1x2x1 范围内的方块（凋灵免疫标签除外）
}

void WitherEntity::explodeOnSpawn()
{
    // MC 1.16.5: 生成时创建7.0威力的爆炸
    // TODO: world->createExplosion(this, x(), y(), z(), 7.0f, ...);

    // TODO: 播放生成音效
    // world->playBroadcastSound(this, SoundEvents.ENTITY_WITHER_SPAWN, ...);
}

void WitherEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/)
{
    // MC 1.16.5: 主头发射凋灵之首
    launchWitherSkullToEntity(0, target);
}

void WitherEntity::registerGoals()
{
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
    m_goalSelector.addGoal(
        6, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->legacyType() == LegacyEntityType::Player;
        }));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));
}

void WitherEntity::registerAttributes()
{
    MobEntity::registerAttributes();

    // MC 1.16.5 WitherEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 300.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 40.0); // MC 1.16.5: 40 而非 64
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 4.0);         // MC 1.16.5: 4 点护甲

    // TODO: 凋灵可以飞行
    // setNoGravity(true);
}

} // namespace entity
} // namespace mc
