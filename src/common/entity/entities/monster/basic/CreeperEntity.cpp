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

#include "CreeperEntity.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/explosion/ExplosionMode.hpp"
#include "../../../../world/gamerule/GameRules.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../ai/goal/goals/special/SpecialGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include <memory>

namespace mc {

CreeperEntity::CreeperEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // MC 1.16.5: 苦力怕不在阳光下燃烧
    setBurnsInDaylight(false);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CreeperEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CreeperEntity>(LegacyEntityType::Unknown, 0);
}

std::optional<ResourceLocation> CreeperEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: entity.creeper.hurt
    return SoundEvents::ENTITY_CREEPER_HURT;
}

std::optional<ResourceLocation> CreeperEntity::getDeathSound() const
{
    // MC 1.16.5: entity.creeper.death
    return SoundEvents::ENTITY_CREEPER_DEATH;
}

i32 CreeperEntity::getCreeperState() const
{
    // MC 1.16.5: -1 = idle, 1 = fusing
    if (m_timeSinceIgnited > 0) {
        return 1;
    }
    return -1;
}

void CreeperEntity::setCreeperState(i32 state)
{
    // MC 1.16.5: 设置状态
    if (state > 0) {
        ignite();
    }
}

void CreeperEntity::ignite()
{
    // MC 1.16.5: 点燃苦力怕
    m_ignited = true;
}

bool CreeperEntity::ableToCauseSkullDrop() const
{
    // MC 1.16.5: 只有高压苦力怕且还没掉过头颅才能导致头颅掉落
    return m_powered && m_droppedSkulls < 1;
}

void CreeperEntity::explode()
{
    // MC 1.16.5 CreeperEntity.explode()
    // 只在服务端爆炸
    if (isDead()) return;

    IWorld* worldPtr = world();
    if (!worldPtr) {
        remove();
        return;
    }

    // 计算爆炸威力（高压翻倍）
    f32 radius = static_cast<f32>(m_explosionRadius);
    if (m_powered) {
        radius *= 2.0f;
    }

    // 苦力怕爆炸模式：检查 mobGriefing 游戏规则
    // 如果 mobGriefing 为 false，则使用 NONE（不破坏方块）
    world::explosion::ExplosionMode mode = world::explosion::ExplosionMode::Destroy;
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        mode = world::explosion::ExplosionMode::None;
    }

    // 创建爆炸
    worldPtr->createExplosion(position(),
        radius,
        mode,
        false, // 不生成火焰
        this   // 爆炸源实体
    );

    // 移除实体
    remove();

    // 生成滞留药水云（如果有药水效果）
    spawnLingeringCloud();
}

void CreeperEntity::spawnLingeringCloud()
{
    // MC 1.16.5 CreeperEntity.spawnLingeringCloud()
    // TODO: 当苦力怕有药水效果时，死亡后生成滞留药水云
    // 需要实现 AreaEffectCloudEntity
}

void CreeperEntity::tick()
{
    // MC 1.16.5 CreeperEntity.tick()
    if (isAlive()) {
        m_lastActiveTime = m_timeSinceIgnited;

        // 如果被点燃，设置状态为膨胀
        if (hasIgnited()) {
            setCreeperState(1);
        }

        i32 state = getCreeperState();
        if (state > 0 && m_timeSinceIgnited == 0) {
            // MC 1.16.5: 开始膨胀时播放音效
            playSound(SoundEvents::ENTITY_CREEPER_PRIMED, 1.0f, 0.5f);
        }

        m_timeSinceIgnited += state;

        if (m_timeSinceIgnited < 0) {
            m_timeSinceIgnited = 0;
        }

        // 达到点燃时间后爆炸
        if (m_timeSinceIgnited >= m_fuseTime) {
            m_timeSinceIgnited = m_fuseTime;
            explode();
            return;
        }
    }

    MonsterEntity::tick();
}

void CreeperEntity::registerGoals()
{
    // 调用父类方法
    MonsterEntity::registerGoals();

    // MC 1.16.5 CreeperEntity.registerGoals()
    // 优先级顺序：
    // 0: SwimGoal (父类已注册)
    // 1: PanicGoal (父类已注册，但苦力怕不使用)
    // 2: CreeperSwellGoal - 膨胀爆炸
    // 3: AvoidEntityGoal<OcelotEntity> - 避开豹猫
    // 3: AvoidEntityGoal<CatEntity> - 避开猫
    // 4: MeleeAttackGoal - 近战攻击（实际不造成伤害，用于接近玩家）
    // 5: WaterAvoidingRandomWalkingGoal - 避水随机行走
    // 6: LookAtGoal<PlayerEntity> - 看向玩家
    // 6: LookRandomlyGoal - 随机看向
    //
    // 目标选择器：
    // 1: NearestAttackableTargetGoal<PlayerEntity> - 攻击玩家
    // 2: HurtByTargetGoal (父类已注册)

    // 优先级 2: 膨胀爆炸
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::CreeperSwellGoal>(this));

    // 优先级 3: 避开猫和豹猫
    // MC 1.16.5: 苦力怕害怕猫和豹猫，会在 6 格内逃跑
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(this,
            6.0f, // avoidDistance - 检测距离
            1.0,  // farSpeed - 远距离逃跑速度
            1.2,  // nearSpeed - 近距离逃跑速度（更快）
            [](const LivingEntity* entity) -> bool {
                if (!entity) return false;
                // 检查是否为猫或豹猫
                // MC 1.16.5: instanceof CatEntity || instanceof OcelotEntity
                auto type = entity->legacyType();
                return type == LegacyEntityType::Cat || type == LegacyEntityType::Ocelot;
            }));

    // 优先级 4: 近战攻击（用于接近玩家）
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, false));

    // 优先级 5: 避水随机行走
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 0.8));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(
        6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            if (!entity) return false;
            // 只看向玩家
            return entity->legacyType() == LegacyEntityType::Player;
        }));

    // 优先级 6: 随机看向
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // 目标选择器：攻击最近的玩家
    // 使用 LivingEntity 类型，通过谓词筛选玩家
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            true, // checkSight - 需要视线
            0,    // chance - 每tick都检查
            [](const LivingEntity* entity) -> bool {
                // MC 1.16.5: 苦力怕只攻击玩家
                if (!entity) return false;
                return entity->legacyType() == LegacyEntityType::Player;
            }));
}

void CreeperEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // MC 1.16.5 CreeperEntity 属性
    // 继承自 MonsterEntity: MAX_HEALTH = 20.0
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
