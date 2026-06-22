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

#include "DrownedEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {

DrownedEntity::DrownedEntity(EntityId id)
    : ZombieEntity(id)
{
    // 溺尸可以走上1格高的方块
    setStepHeight(1.0f);

    // 注册属性
    registerAttributes();

    // 随机决定是否手持三叉戟
    math::Random& rng = getRandom();
    m_hasTrident = rng.nextInt(1, 100) <= 15; // 15% 概率
}

std::unique_ptr<Entity> DrownedEntity::create(IWorld* /*world*/)
{
    return std::make_unique<DrownedEntity>(EntityId(0));
}

bool DrownedEntity::isInWater() const
{
    // 调用父类的 isInWater() 方法
    // Entity::isInWater() 已经在 updateEnvironmentState() 中正确更新
    return ZombieEntity::isInWater();
}

bool DrownedEntity::shouldBurnInDaylight() const
{
    // 在水中不燃烧
    return !isInWater();
}

void DrownedEntity::registerGoals()
{
    // 调用父类方法（ZombieEntity::registerGoals 注册了基础僵尸 AI）
    ZombieEntity::registerGoals();

    // 溺尸需要替换父类注册的 HurtByTargetGoal
    // MC 原版: Drowned 使用 HurtByTargetGoal(this, Drowned.class).setAlertOthers(ZombifiedPiglin.class)
    // 即不反击同类溺尸，且不警醒僵尸猪灵
    // 父类 ZombieEntity 注册了不带 Drowned 排除的 HurtByTargetGoal，需要先移除再添加
    m_targetSelector.removeGoalsOfType<entity::ai::goal::HurtByTargetGoal>();

    // 添加溺尸专用的 HurtByTargetGoal：排除同类溺尸，不警醒僵尸猪灵
    {
        auto hurtByTarget =
            std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
                // MC 原版: Drowned.class — 不反击同类溺尸
                return attacker != nullptr && attacker->typeId() == entity::EntityTypeIdNumber::DROWNED;
            });
        hurtByTarget->setAlertOthers([](const LivingEntity* ally) -> bool {
            // MC 原版: ZombifiedPiglin.class — 不警醒僵尸猪灵
            return ally != nullptr && ally->typeId() == entity::EntityTypeIdNumber::ZOMBIFIED_PIGLIN;
        });
        m_targetSelector.addGoal(1, std::move(hurtByTarget));
    }

    // TODO: 实现溺尸特有的行为目标（DrownedGoToWaterGoal、DrownedTridentAttackGoal、
    // DrownedAttackGoal、DrownedGoToBeachGoal、DrownedSwimUpGoal）
    // 这些目标需要先实现对应的 AI 目标类

    // 溺尸还需要攻击美西螈（Axolotl）— 需要实现 AxolotlEntity 后添加
    // MC 原版: targetSelector.addGoal(3, NearestAttackableTargetGoal(Axolotl.class, true))
}

void DrownedEntity::tick()
{
    ZombieEntity::tick();

    // 在水中时的特殊行为
    // TODO: 实现溺尸水中游泳AI目标，游泳状态由AI目标控制
    (void)isInWater(); // 暂时避免未使用警告，AI目标实现后会使用
}

void DrownedEntity::registerAttributes()
{
    // 调用父类方法
    ZombieEntity::registerAttributes();

    // 溺尸的属性与僵尸相同
}

} // namespace mc
