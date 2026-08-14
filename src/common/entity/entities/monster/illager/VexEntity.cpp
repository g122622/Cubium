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

#include "VexEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/controller/VexMovementController.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/special/VexGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include <memory>

namespace mc {

// ============================================================================
// 继承链标识（parent = MonsterEntity::classInfo()）。独立链（不经 Raider），
// 透传层无自身同步字段，classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& VexEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"VexEntity", &MonsterEntity::classInfo()};
    return s_classInfo;
}

VexEntity::VexEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    // 恼鬼不在阳光下燃烧：wiki tech_恼鬼.txt#行为 章节未提阳光燃烧，原版 Vex 不燃。
    // 对齐原版，在构造时关闭日光燃烧。MonsterEntity::handleDaylightBurning() 读成员 m_burnsInDaylight
    // （而非虚函数 shouldBurnInDaylight()，后者全仓零调用是遗留死代码 API），故用 setBurnsInDaylight(false) 生效。
    // hpp 中的 shouldBurnInDaylight() override 语义正确（返回 false）但因死代码不生效，保留以备未来清理。
    setBurnsInDaylight(false);

    // 恼鬼使用专用的飞行移动控制器
    m_moveController = std::make_unique<entity::ai::controller::VexMovementController>(this);

    // 注册属性与 AI 目标（基类构造函数中调用 registerAttributes/registerGoals 不会派发到子类，
    // vtable 在基类构造期间指向基类，派生 override 永不执行，须在派生类构造显式调用）。
    // Vex 的 registerGoals 加专属 VexChargeAttack / VexMoveRandom / 复制主人目标等。
    registerAttributes();
    registerGoals();
}

std::unique_ptr<Entity> VexEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<VexEntity>(EntityInstanceId(0), registry);
}

void VexEntity::tick()
{
    // 恼鬼在 tick 期间可以穿墙
    setNoClip(true);
    MonsterEntity::tick();
    setNoClip(false);

    // 恼鬼始终不受重力影响
    setNoGravity(true);

    // 更新生命时间
    if (m_limitedLife && m_lifeTime > 0) {
        m_lifeTime--;

        // 生命结束时造成饥饿伤害
        if (m_lifeTime <= 0) {
            m_lifeTime = 20; // 重置为 20 tick 防止连续伤害
            auto damageSource = DamageSources::starve();
            hurt(damageSource, 1.0f);
        }
    }
}

void VexEntity::registerGoals()
{
    // 调用父类方法注册基础 AI（SwimGoal, HurtByTargetGoal）
    MonsterEntity::registerGoals();

    // ========== 行为目标 ==========
    // 优先级 0: 游泳（已在 MonsterEntity::registerGoals() 中注册）

    // 优先级 4: 冲锋攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::VexChargeAttackGoal>(this));

    // 优先级 8: 随机飞行移动
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::VexMoveRandomGoal>(this));

    // 优先级 9: 看向玩家（距离3格，概率1.0）
    m_goalSelector.addGoal(
        9, std::make_unique<entity::ai::goal::LookAtGoal>(this, 3.0f, 1.0f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 10: 看向生物（距离8格）
    m_goalSelector.addGoal(
        10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && dynamic_cast<const MobEntity*>(entity) != nullptr;
        }));

    // ========== 目标选择器 ==========
    // 优先级 1: 被攻击后反击 — 替换父类的默认 HurtByTargetGoal
    // MC 原版: HurtByTargetGoal(this, Raider.class).setAlertOthers()
    // 恼鬼不会反击灾厄村民，但会警醒同类
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));

    // 优先级 2: 复制主人目标
    // 当主人（唤魔者）攻击某个目标时，恼鬼也会攻击该目标
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::VexCopyOwnerTargetGoal>(this));

    // 优先级 3: 攻击最近的玩家（需要视线检查）
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this,
            true, // checkSight - 需要视线检查
            0     // chance - 每tick都检查
            ));
}

void VexEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 恼鬼属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 14.0f);
    // MOVEMENT_SPEED: 使用默认值（恼鬼飞行速度由移动控制器控制）
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0f);
    // FOLLOW_RANGE: 使用默认值
}

} // namespace mc
