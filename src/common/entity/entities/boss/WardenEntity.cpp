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

#include "WardenEntity.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../core/Entity.hpp"
#include "../../core/EntityDataManager.hpp"
#include "../../core/MobEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../entities/player/Player.hpp"
#include "../../registry/VanillaEntityTypeKeys.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/boss/WardenAngerLevel.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/explosion/ExplosionImmunityContext.hpp"
#include <algorithm>
#include <memory>
#include <optional>

namespace mc {
namespace entity {

// ============================================================================
// 静态数据参数定义
// ============================================================================

/// 客户端同步怒气值数据参数（对应 MC 1.21.11 Warden.CLIENT_ANGER_LEVEL）
DataParameter<i32> WardenEntity::CLIENT_ANGER_LEVEL = EntityDataManager::createKey<i32>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = MonsterEntity::classInfo()）
// ============================================================================
const EntityClassInfo& WardenEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"WardenEntity", &MonsterEntity::classInfo()};
    return s_classInfo;
}

// ============================================================================
// 工厂方法
// ============================================================================

std::unique_ptr<Entity> WardenEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<WardenEntity>(EntityInstanceId(0), registry);
}

// ============================================================================
// 构造函数
// ============================================================================

WardenEntity::WardenEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    // MC 1.21.11 Warden 构造函数: this.xpReward = 5
    setExperienceValue(5);

    // MC 1.21.11 Warden 不会在阳光下燃烧（其本体不继承 BurnsInDaylight 行为）
    setBurnsInDaylight(false);

    // 注册数据参数（CLIENT_ANGER_LEVEL）
    registerData();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

// ============================================================================
// 数据参数注册
// ============================================================================

void WardenEntity::registerData()
{
    // 调用父类注册数据参数（LivingEntity 注册 HEALTH/FLAGS 等基础参数）
    MonsterEntity::registerData();

    // 标记当前正在注册 WardenEntity 类的字段，使 registerParam 沿 WardenEntity 继承链
    // 分配 id（续接 MonsterEntity 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册客户端同步怒气值，初始值为 0（对应 MC define(CLIENT_ANGER_LEVEL, 0)）
    m_dataManager.registerParam(CLIENT_ANGER_LEVEL, static_cast<i32>(0));
}

// ============================================================================
// 声音
// ============================================================================

std::optional<ResourceLocation> WardenEntity::getAmbientSound() const
{
    // MC 1.21.11 Warden.getAmbientSound():
    //   return !this.hasPose(Pose.ROARING) && !this.isDiggingOrEmerging()
    //       ? this.getAngerLevel().getAmbientSound()
    //       : null;
    //
    // 当前项目尚未引入 Pose::ROARING/DIGGING/EMERGING，姿态判断暂时省略，
    // 直接根据怒气等级返回对应的环境音效。
    //
    // 怒气等级 → 环境音效：
    // - Calmed   → SoundEvents::ENTITY_WARDEN_AMBIENT
    // - Agitated → SoundEvents::ENTITY_WARDEN_AGITATED
    // - Angry    → SoundEvents::ENTITY_WARDEN_ANGRY
    return wardenAngerLevelAmbientSound(getAngerLevel());
}

std::optional<ResourceLocation> WardenEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.21.11 Warden.getHurtSound() 返回 SoundEvents.WARDEN_HURT
    return SoundEvents::ENTITY_WARDEN_HURT;
}

std::optional<ResourceLocation> WardenEntity::getDeathSound() const
{
    // MC 1.21.11 Warden.getDeathSound() 返回 SoundEvents.WARDEN_DEATH
    return SoundEvents::ENTITY_WARDEN_DEATH;
}

// ============================================================================
// 怒气等级（AngerLevel）
// ============================================================================

WardenAngerLevel WardenEntity::getAngerLevel() const noexcept
{
    // 对应 MC 1.21.11 Warden.getAngerLevel() = AngerLevel.byAnger(getActiveAnger())
    // 简化版：直接使用单一聚合怒气值 m_anger，不按目标分别查询
    return wardenAngerLevelByAnger(m_anger);
}

i32 WardenEntity::getClientAngerLevel() const noexcept
{
    // 对应 MC 1.21.11 Warden.getClientAngerLevel() = entityData.get(CLIENT_ANGER_LEVEL)
    return m_dataManager.get<i32>(CLIENT_ANGER_LEVEL);
}

i32 WardenEntity::increaseAnger(i32 amount) noexcept
{
    // 对应 MC 1.21.11 Warden.increaseAngerAt() 简化版：
    //   int i = angerManagement.increaseAnger(entity, amount);
    //   ...
    //   this.syncClientAngerLevel();
    //
    // 项目当前未引入 AngerManagement，直接累加 m_anger 并夹紧到 [0, ANGER_LIMIT]。
    if (amount <= 0) {
        return m_anger;
    }
    m_anger = std::min(m_anger + amount, ANGER_LIMIT);

    // 同步到客户端（对应 MC syncClientAngerLevel）
    m_dataManager.set(CLIENT_ANGER_LEVEL, m_anger);

    return m_anger;
}

void WardenEntity::clearAnger() noexcept
{
    // 对应 MC 1.21.11 Warden.clearAnger(entity)
    m_anger = 0;
    m_dataManager.set(CLIENT_ANGER_LEVEL, m_anger);
}

// ============================================================================
// 伤害与免疫
// ============================================================================

bool WardenEntity::isInvulnerableTo(DamageSource& source) const
{
    // 监守者在 Digging/Emerging 姿态下免疫除"穿透无敌"标签外的所有伤害。
    //
    // 当前实现尚未引入 Pose::DIGGING / Pose::EMERGING 姿态系统，无法判断
    // isDiggingOrEmerging()，因此暂不实现姿态相关免疫。
    //
    // TODO: 引入 Pose 系统后实现 isDiggingOrEmerging() 并补充姿态相关免疫逻辑。

    // 监守者继承自 MonsterEntity，MonsterEntity 默认不免疫火焰/岩浆伤害
    // （监守者也不免疫火焰/岩浆，可被点燃）。但监守者免疫以下伤害：
    // - Drown: 监守者不会溺水
    // - Wither: 监守者免疫凋零效果
    if (source.type() == DamageType::Drown) {
        return true;
    }
    if (source.type() == DamageType::Wither) {
        return true;
    }

    return MonsterEntity::isInvulnerableTo(source);
}

bool WardenEntity::ignoreExplosion(const world::explosion::ExplosionImmunityContext& ctx) const
{
    // 监守者在 Digging/Emerging 姿态下应免疫爆炸（与 isInvulnerableTo 同源）。
    // 当前姿态系统未实现，先回退基类行为。
    // TODO: 引入 Pose::DIGGING / Pose::EMERGING 后，在此姿态下返回 true。
    return MonsterEntity::ignoreExplosion(ctx);
}

bool WardenEntity::onLivingFall(f32 /*distance*/, f32 /*damageMultiplier*/)
{
    // MC 1.21.11 Warden 通过 isInvulnerableTo 对 Fall 伤害返回 true 实现摔落免疫。
    // 此处直接返回 false 阻止摔落伤害逻辑执行。
    return false;
}

// ============================================================================
// AI 目标注册
// ============================================================================

void WardenEntity::registerGoals()
{
    // 调用父类方法（添加 SwimGoal 优先级 0）
    MonsterEntity::registerGoals();

    // ========== 行为目标（goalSelector） ==========

    // 优先级 2: 近战攻击
    // MC 1.21.11 WardenAi.meleeAttack() 创建 MeleeAttack(1.2F, false)
    // 监守者近战范围比普通怪物略大（攻击距离 = 4.5 在 WardenAi 中体现），
    // 此处使用项目 MeleeAttackGoal 默认范围。
    m_goalSelector.addGoal(2, new ai::goal::MeleeAttackGoal(this, 1.2, false));

    // 优先级 7: 避水随机行走
    // MC 1.21.11 WardenAi 中的 RandomSwimming / MoveToVibration 等复杂行为
    // 依赖 Brain 系统和振动系统，当前简化为 WaterAvoidingRandomWalkingGoal。
    m_goalSelector.addGoal(7, new ai::goal::WaterAvoidingRandomWalkingGoal(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8, new ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
        return entity != nullptr && entity->entityType() == VanillaEntityTypeKeys::PLAYER;
    }));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new ai::goal::LookRandomlyGoal(this));

    // ========== 攻击目标（targetSelector） ==========

    // 优先级 1: 被攻击后反击
    // MC 1.21.11 Warden 通过振动系统接收伤害信号并 increaseAngerAt，
    // 简化实现使用 HurtByTargetGoal。
    m_targetSelector.addGoal(1, new ai::goal::HurtByTargetGoal(this));

    // 优先级 2: 攻击玩家
    // MC 1.21.11 Warden.canTargetEntity() 排除创造/旁观模式玩家、
    // 盔甲架、其他监守者、无敌实体、死亡/濒死实体、世界边界外实体。
    // 简化实现使用 NearestAttackableTargetGoal<Player>，由 Player 谓词
    // 自动排除创造/旁观模式。
    m_targetSelector.addGoal(2, new ai::goal::NearestAttackableTargetGoal<Player>(this, true));

    // TODO: 完整的监守者行为系统包括以下未实现的子系统：
    // - VibrationSystem: 振动感知系统（监听 game_event 并 increaseAngerAt）
    // - AngerManagement: 怒气管理（按目标跟踪怒气值，按等级切换行为）
    //   ※ 当前已实现简化版单一聚合怒气（m_anger + WardenAngerLevel），
    //     足以支撑环境音效切换与客户端表现，完整 AngerManagement 后续接入
    // - SonicBoom: 音爆远程攻击（生命值低于一半或目标过远时使用）
    // - Emerging: 从地面钻出动画（生成时 7 秒，期间免疫伤害）
    // - Digging: 钻入地面动画（消失前 4 秒，期间免疫伤害）
    // - Roar: 怒吼动画（首次锁定目标时播放）
    // - Sniff: 嗅闻动画（无目标时定期播放）
    // - Darkness: 周期性给附近玩家施加黑暗效果（每 6 秒）
    // - 心跳音效：根据怒气等级调整心跳频率
    // - 触碰怒气：实体触碰监守者时增加怒气（20 tick 冷却）
    // 这些子系统依赖 Brain 系统、MemoryModuleType、Pose 系统等尚未实现的基建，
    // 后续逐项收敛。
}

// ============================================================================
// AI 任务更新（怒气衰减与同步）
// ============================================================================

void WardenEntity::updateAITasks()
{
    // 调用父类方法（MobEntity::updateAITasks 默认空实现，保留以兼容未来扩展）
    MonsterEntity::updateAITasks();

    // MC 1.21.11 Warden.customServerAiStep():
    //   if (this.tickCount % 20 == 0) {
    //       this.angerManagement.tick(serverLevel, this::canTargetEntity);
    //       this.syncClientAngerLevel();
    //   }
    //
    // 简化版：每 ANGERMANAGEMENT_TICK_DELAY (20) tick 衰减怒气并同步客户端。
    // 衰减率对应 MC AngerManagement.tick 中对每条怒气记录的 -1 衰减。
    const u32 currentTick = ticksExisted();
    if (currentTick > 0 && currentTick % static_cast<u32>(ANGERMANAGEMENT_TICK_DELAY) == 0) {
        if (m_anger > 0) {
            m_anger = std::max(m_anger - ANGER_DECAY_PER_TICK_INTERVAL, 0);
            // 同步到客户端（对应 MC syncClientAngerLevel）
            m_dataManager.set(CLIENT_ANGER_LEVEL, m_anger);
        }
    }
}

// ============================================================================
// 属性注册
// ============================================================================

void WardenEntity::registerAttributes()
{
    // 调用父类方法（注册 ATTACK_DAMAGE 属性）
    MonsterEntity::registerAttributes();

    // MC 1.21.11 Warden.createAttributes():
    //   return Monster.createMonsterAttributes()
    //       .add(Attributes.MAX_HEALTH, 500.0)
    //       .add(Attributes.MOVEMENT_SPEED, 0.3F)
    //       .add(Attributes.KNOCKBACK_RESISTANCE, 1.0)
    //       .add(Attributes.ATTACK_KNOCKBACK, 1.5)
    //       .add(Attributes.ATTACK_DAMAGE, 30.0)
    //       .add(Attributes.FOLLOW_RANGE, 24.0);

    attributes().setBaseValue(attribute::Attributes::MAX_HEALTH, MAX_HEALTH);
    attributes().setBaseValue(attribute::Attributes::MOVEMENT_SPEED, MOVEMENT_SPEED);

    // 注册并设置击退抗性（MobEntity 默认注册了 KNOCKBACK_RESISTANCE，但保险起见调用 setBaseValue）
    attributes().setBaseValue(attribute::Attributes::KNOCKBACK_RESISTANCE, KNOCKBACK_RESISTANCE);

    // 注册攻击击退属性（MobEntity 默认未注册 ATTACK_KNOCKBACK）
    attributes().registerAttribute(*attribute::Attributes::attackKnockback());
    attributes().setBaseValue(attribute::Attributes::ATTACK_KNOCKBACK, ATTACK_KNOCKBACK);

    attributes().setBaseValue(attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
    attributes().setBaseValue(attribute::Attributes::FOLLOW_RANGE, FOLLOW_RANGE);
}

} // namespace entity
} // namespace mc
