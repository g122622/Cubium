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
#include "../../../ai/controller/VexMovementController.hpp"
#include "../../../ai/goal/GoalFlag.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/special/VexGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../player/Player.hpp"
#include <memory>

namespace mc {

VexEntity::VexEntity(EntityId id)
    : MonsterEntity(id)
{
    // MC 1.16.5: 恼鬼使用专用的飞行移动控制器
    // 参考 VexEntity 构造函数: this.moveController = new VexEntity.MoveHelperController(this);
    m_moveController = std::make_unique<entity::ai::controller::VexMovementController>(this);

    // 注册属性（基类构造函数中调用 registerAttributes() 不会派发到子类）
    registerAttributes();
}

std::unique_ptr<Entity> VexEntity::create(IWorld* /*world*/)
{
    return std::make_unique<VexEntity>(EntityId(0));
}

void VexEntity::tick()
{
    // MC 1.16.5: 恼鬼在 tick 期间可以穿墙
    // 参考 VexEntity.tick() 行 62-71
    setNoClip(true);
    MonsterEntity::tick();
    setNoClip(false);

    // 恼鬼始终不受重力影响
    setNoGravity(true);

    // 更新生命时间
    if (m_limitedLife && m_lifeTime > 0) {
        m_lifeTime--;

        // 生命结束时造成饥饿伤害
        // MC 1.16.5: limitedLifeTicks <= 0 时攻击自己造成 1.0 饥饿伤害
        if (m_lifeTime <= 0) {
            m_lifeTime = 20; // 重置为 20 tick 防止连续伤害
            auto damageSource = DamageSources::starve();
            hurt(damageSource, 1.0f);
        }
    }
}

void VexEntity::registerGoals()
{
    // MC 1.16.5 VexEntity.registerGoals()
    // 调用父类方法注册基础 AI（SwimGoal, HurtByTargetGoal）
    MonsterEntity::registerGoals();

    // ========== 行为目标 ==========
    // 优先级 0: 游泳（已在 MonsterEntity::registerGoals() 中注册）

    // MC 1.16.5: 优先级 4: 冲锋攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::VexChargeAttackGoal>(this));

    // MC 1.16.5: 优先级 8: 随机飞行移动
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::VexMoveRandomGoal>(this));

    // MC 1.16.5: 优先级 9: 看向玩家（距离3格，概率1.0）
    m_goalSelector.addGoal(
        9, std::make_unique<entity::ai::goal::LookAtGoal>(this, 3.0f, 1.0f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PLAYER;
        }));

    // MC 1.16.5: 优先级 10: 看向生物（距离8格）
    m_goalSelector.addGoal(
        10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // MC 1.16.5: MobEntity.class
            return entity != nullptr && dynamic_cast<const MobEntity*>(entity) != nullptr;
        }));

    // ========== 目标选择器 ==========
    // 优先级 1: 被攻击后反击（已在 MonsterEntity::registerGoals() 中注册）

    // MC 1.16.5: 优先级 2: 复制主人目标
    // 当主人（唤魔者）攻击某个目标时，恼鬼也会攻击该目标
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::VexCopyOwnerTargetGoal>(this));

    // MC 1.16.5: 优先级 3: 攻击最近的玩家（需要视线检查）
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this,
            true, // checkSight - 需要视线检查
            0     // chance - 每tick都检查
            ));
}

void VexEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // MC 1.16.5 VexEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 14.0f);
    // MOVEMENT_SPEED: 使用默认值（恼鬼飞行速度由移动控制器控制）
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0f); // MC 1.16.5: 铁剑伤害为 4.0
    // FOLLOW_RANGE: 使用默认值
}

} // namespace mc
