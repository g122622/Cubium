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
#include "../../attribute/Attributes.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../core/Entity.hpp"
#include "../../core/EntityTypeIdNumber.hpp"
#include "../../core/MobEntity.hpp"
#include "../../entities/player/Player.hpp"

namespace mc {
namespace entity {

// ============================================================================
// 工厂方法
// ============================================================================

std::unique_ptr<Entity> WardenEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WardenEntity>(EntityId(0));
}

// ============================================================================
// 构造函数
// ============================================================================

WardenEntity::WardenEntity(EntityId id)
    : MonsterEntity(id)
{
    // MC 1.21.11 Warden 构造函数: this.xpReward = 5
    setExperienceValue(5);

    // MC 1.21.11 Warden 不会在阳光下燃烧（其本体不继承 BurnsInDaylight 行为）
    setBurnsInDaylight(false);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

// ============================================================================
// 声音
// ============================================================================

std::optional<ResourceLocation> WardenEntity::getAmbientSound() const
{
    // MC 1.21.11 Warden.getAmbientSound() 根据 AngerLevel 返回不同声音：
    // - Calmed:  SoundEvents.WARDEN_AMBIENT
    // - Agitated: SoundEvents.WARDEN_AGITATED
    // - Angry:    SoundEvents.WARDEN_LISTENING_ANGRY
    // 当前实现尚未引入 AngerLevel，统一使用 "ambient" 通用 ID。
    //
    // TODO: 引入 AngerLevel 后根据怒气等级返回不同声音。
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> WardenEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.21.11 Warden.getHurtSound() 返回 SoundEvents.WARDEN_HURT
    // 当前项目 SoundEvents.hpp 未预定义该事件，使用通用 "hurt" ID。
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> WardenEntity::getDeathSound() const
{
    // MC 1.21.11 Warden.getDeathSound() 返回 SoundEvents.WARDEN_DEATH
    // 当前项目 SoundEvents.hpp 未预定义该事件，使用通用 "death" ID。
    return makeSoundEventId("death");
}

// ============================================================================
// 伤害与免疫
// ============================================================================

bool WardenEntity::isInvulnerableTo(DamageSource& source) const
{
    // MC 1.21.11 Warden.isInvulnerableTo():
    //   if (this.isDiggingOrEmerging() && !source.is(DamageTypeTags.BYPASSES_INVULNERABILITY)) {
    //       return true;
    //   }
    //   return super.isInvulnerableTo(serverLevel, source);
    //
    // 当前实现尚未引入 Pose::DIGGING / Pose::EMERGING 姿态系统，无法判断
    // isDiggingOrEmerging()，因此暂不实现姿态相关免疫。
    //
    // TODO: 引入 Pose 系统后实现 isDiggingOrEmerging() 并补充姿态相关免疫逻辑。

    // 监守者继承自 MonsterEntity，MonsterEntity 默认不免疫火焰/岩浆伤害
    // （MC 原版监守者也不免疫火焰/岩浆，可被点燃）。但监守者免疫以下伤害：
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
        return entity != nullptr && entity->typeId() == EntityTypeIdNumber::PLAYER;
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
    // - AngerManagement: 怒气管理（跟踪每个实体的怒气值，按等级切换行为）
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

    m_attributes.setBaseValue(attribute::Attributes::MAX_HEALTH, MAX_HEALTH);
    m_attributes.setBaseValue(attribute::Attributes::MOVEMENT_SPEED, MOVEMENT_SPEED);

    // 注册并设置击退抗性（MobEntity 默认注册了 KNOCKBACK_RESISTANCE，但保险起见调用 setBaseValue）
    m_attributes.setBaseValue(attribute::Attributes::KNOCKBACK_RESISTANCE, KNOCKBACK_RESISTANCE);

    // 注册攻击击退属性（MobEntity 默认未注册 ATTACK_KNOCKBACK）
    m_attributes.registerAttribute(*attribute::Attributes::attackKnockback());
    m_attributes.setBaseValue(attribute::Attributes::ATTACK_KNOCKBACK, ATTACK_KNOCKBACK);

    m_attributes.setBaseValue(attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
    m_attributes.setBaseValue(attribute::Attributes::FOLLOW_RANGE, FOLLOW_RANGE);
}

} // namespace entity
} // namespace mc
