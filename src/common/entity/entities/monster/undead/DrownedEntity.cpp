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

#include "common/entity/ai/controller/DrownedMoveControl.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/DrownedGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/water/AxolotlEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/TridentEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"

namespace mc {

DrownedEntity::DrownedEntity(EntityInstanceId id)
    : ZombieEntity(id)
{
    // 溺尸可以走上1格高的方块
    setStepHeight(1.0f);

    // 使用溺尸专用的两栖移动控制器（水中游泳 + 陆地行走）
    m_moveController = std::make_unique<entity::ai::controller::DrownedMoveControl>(this);

    // 注册属性
    registerAttributes();

    // 三叉戟持有状态在 finalizeSpawn() 中按生成时概率随机决定（与父类僵尸的
    // 破门/南瓜头等生成期初始化同模式）。直接构造（未经过生成初始化）的溺尸
    // 默认不持有三叉戟，避免构造期随机带来的不确定性。
}

std::unique_ptr<Entity> DrownedEntity::create(IWorld* /*world*/)
{
    return std::make_unique<DrownedEntity>(EntityInstanceId(0));
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

bool DrownedEntity::okTarget(const LivingEntity* target) const
{
    if (target == nullptr) {
        return false;
    }

    // 非白天（夜晚或雷暴）时，所有目标都有效
    if (world() != nullptr && !world()->isBrightOutside()) {
        return true;
    }

    // 白天时，只有目标在水中才有效
    return target->isInWater();
}

bool DrownedEntity::wantsToSwim() const
{
    // 正在搜索陆地或当前攻击目标在水中
    if (m_searchingForLand) {
        return true;
    }

    const LivingEntity* target = attackTarget();
    if (target != nullptr && target->isInWater()) {
        return true;
    }

    return false;
}

void DrownedEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    if (target == nullptr || world() == nullptr) {
        return;
    }

    // 创建三叉戟实体
    auto trident = std::make_unique<entity::TridentEntity>(EntityInstanceId(0));
    if (trident == nullptr) {
        return;
    }

    trident->setWorld(world());
    trident->setPosition(x(), static_cast<f32>(getEyeY() - 0.1f), z());
    trident->setShooter(this);

    // 设置三叉戟的基础伤害
    trident->setBaseDamageFromMob(charge);

    // 计算射击方向
    f64 dx = target->x() - x();
    f64 dy = target->getY(0.3333333333333333) - trident->y();
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 不精确度：难度越高，不精确度越低，三叉戟越精准
    // Peaceful=14, Easy=10, Normal=6, Hard=2
    f32 inaccuracy = entity::combat::DifficultyHelper::getRangedAttackInaccuracy(world()->difficulty());

    // 发射三叉戟：速度 1.6，Y轴补偿水平距离的 0.2 倍用于抛物线弹道
    trident->shoot(static_cast<f32>(dx),
        static_cast<f32>(dy + horizontalDist * 0.2),
        static_cast<f32>(dz),
        TRIDENT_VELOCITY,
        inaccuracy);

    // 将三叉戟添加到世界
    world()->spawnEntity(std::move(trident));

    // 播放三叉戟投掷音效
    playSound(SoundEvents::ITEM_TRIDENT_THROW, 1.0f, 1.0f);
}

void DrownedEntity::tick()
{
    ZombieEntity::tick();

    // 服务端推进游泳标志位
    // 对应 MC 1.21.11 Drowned.updateSwimming()，在 Drowned.tick() 中由
    // super.tick()（Zombie.tick → Mob.tick → LivingEntity.tick）间接触发。
    // Cubium 将该调用显式放在 ZombieEntity::tick() 之后，确保环境状态
    // （isInWater、areEyesInWater）已在 baseTick 中更新完毕。
    // updateSwimAmount() 由 LivingEntity::tick() 内部调用，无需在此重复。
    if (world() != nullptr && !world()->isClientSide()) {
        updateSwimming();
    }
}

void DrownedEntity::updateSwimming()
{
    // 对应 MC 1.21.11 Drowned.updateSwimming()
    //   if (!this.level().isClientSide()) {
    //       this.setSwimming(this.isEffectiveAi() && this.isUnderWater() && this.wantsToSwim());
    //   }
    // isEffectiveAi() 在原版排除 NoAi 实体；Cubium 不支持 NoAi，且本方法仅在服务端调用，
    // 因此等价于恒真。isUnderWater() 在原版表示实体身体在水中，Cubium 中等价于
    // areEyesInWater() && isInWater()（注意：DrownedEntity 重写了 canSwim() 恒返回 true，
    // 用于"溺尸总是具备游泳能力"的移动语义，不能作为"当前是否在水下"的判定，因此这里显式
    // 组合 areEyesInWater + isInWater）。isRiding() 排除骑乘状态，与 isVisuallySwimming 的
    // !isPassenger() 约束一致，避免骑乘时仍置位游泳标志导致动画与骑乘姿态冲突。
    if (world() == nullptr || world()->isClientSide()) {
        return;
    }

    const bool underWater = areEyesInWater() && isInWater();
    const bool shouldSwim = !isRiding() && underWater && wantsToSwim();
    setSwimming(shouldSwim);
}

bool DrownedEntity::isVisuallySwimming() const
{
    // 对应 MC 1.21.11 Drowned.isVisuallySwimming()
    //   return this.isSwimming() && !this.isPassenger();
    // 溺尸的视觉游泳完全由 Swimming 标志位驱动（不像 LivingEntity 基类那样还考虑姿态），
    // 且要求未骑乘其他实体。
    return isSwimming() && !isRiding();
}

void DrownedEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    // 先调用父类生成初始化（拾取物品、破门、装备附魔、属性修饰符等）
    ZombieEntity::finalizeSpawn(world, difficulty, spawnReason);

    // 随机决定是否手持三叉戟：约 10% 概率装主手武器，其中 10/16 为三叉戟
    // （综合约 6.25%）。与局部难度无关。转化生成的溺尸不走此路径，不会重新随机。
    math::Random& rng = getRandom();
    if (rng.nextFloat() > 0.9f) {
        m_hasTrident = rng.nextInt(16) < 10;
    } else {
        m_hasTrident = false;
    }
}

void DrownedEntity::registerGoals()
{
    // 调用父类方法（ZombieEntity::registerGoals 注册了基础僵尸 AI）
    ZombieEntity::registerGoals();

    // 溺尸需要替换父类注册的 HurtByTargetGoal
    // 父类注册了不带溺尸排除的 HurtByTargetGoal，需要先移除再添加
    m_targetSelector.removeGoalsOfType<entity::ai::goal::HurtByTargetGoal>();

    // 添加溺尸专用的 HurtByTargetGoal：排除同类溺尸，不警醒僵尸猪灵
    {
        auto hurtByTarget =
            std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
                // 不反击同类溺尸
                return attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::DROWNED;
            });
        hurtByTarget->setAlertOthers([](const LivingEntity* ally) -> bool {
            // 不警醒僵尸猪灵
            return ally != nullptr && ally->entityType() == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN;
        });
        m_targetSelector.addGoal(1, std::move(hurtByTarget));
    }

    // ===== 溺尸专属行为目标 =====

    // 优先级 1: 白天前往水源
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::DrownedGoToWaterGoal>(this, 1.0));

    // 优先级 2: 三叉戟远程攻击（仅当手持三叉戟时）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::DrownedTridentAttackGoal>(this, 1.0, 40, 10.0f));

    // 优先级 2: 近战攻击（带 okTarget 过滤）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::DrownedAttackGoal>(this, 1.0, false));

    // 优先级 5: 夜间前往海滩
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::DrownedGoToBeachGoal>(this, 1.0));

    // 优先级 6: 夜间向上游泳
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::DrownedSwimUpGoal>(this, 1.0, mc::world::SEA_LEVEL));

    // ===== 溺尸专属目标选择 =====

    // 优先级 2: 攻击玩家（带 okTarget 过滤）
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true));

    // 优先级 3: 攻击美西螈
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<AxolotlEntity>>(this, true));
}

void DrownedEntity::registerAttributes()
{
    // 调用父类方法
    ZombieEntity::registerAttributes();

    // 溺尸的属性与僵尸相同
}

} // namespace mc
