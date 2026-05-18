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

#include "MobEntity.hpp"
#include "../../core/Types.hpp"
#include "../../item/core/ActionResult.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../item/enchantment/EnchantmentHelper.hpp"
#include "../../item/enchantment/enchantments/AllEnchantments.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../world/IWorld.hpp"
#include "../ai/EntitySenses.hpp"
#include "../ai/controller/JumpController.hpp"
#include "../ai/controller/LookController.hpp"
#include "../ai/controller/MovementController.hpp"
#include "../ai/pathfinding/PathNavigator.hpp"
#include "../attribute/Attributes.hpp"
#include "../combat/PlayerAttackHelper.hpp"
#include "../damage/DamageSource.hpp"
#include "../entities/player/Player.hpp"
#include "../entities/vehicle/BoatEntity.hpp"
#include "../experience/ExperienceDropHandler.hpp"

namespace mc {

MobEntity::MobEntity(EntityId id)
    : LivingEntity(id)
    , m_lookController(std::make_unique<entity::ai::controller::LookController>(this))
    , m_moveController(std::make_unique<entity::ai::controller::MovementController>(this))
    , m_jumpController(std::make_unique<entity::ai::controller::JumpController>(this))
    , m_senses(std::make_unique<entity::ai::EntitySenses>(this))
    , m_navigator(std::make_unique<entity::ai::pathfinding::PathNavigator>(this))
{
    // 子类可在此初始化寻路器
}

MobEntity::~MobEntity() = default;

void MobEntity::registerAttributes()
{
    // MC 1.16.5 MobEntity.func_233666_p_()
    // 在 LivingEntity 基础上注册和设置属性
    LivingEntity::registerAttributes();

    // 注册并设置跟随范围
    // MC 1.16.5: MobEntity 注册 FOLLOW_RANGE 并设置默认值为 16.0
    m_attributes.registerAttribute(*entity::attribute::Attributes::followRange());
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

entity::ai::controller::LookController* MobEntity::lookController()
{
    return m_lookController.get();
}

const entity::ai::controller::LookController* MobEntity::lookController() const
{
    return m_lookController.get();
}

entity::ai::controller::MovementController* MobEntity::moveController()
{
    return m_moveController.get();
}

const entity::ai::controller::MovementController* MobEntity::moveController() const
{
    return m_moveController.get();
}

entity::ai::controller::JumpController* MobEntity::jumpController()
{
    return m_jumpController.get();
}

const entity::ai::controller::JumpController* MobEntity::jumpController() const
{
    return m_jumpController.get();
}

entity::ai::pathfinding::PathNavigator* MobEntity::navigator()
{
    return m_navigator.get();
}

const entity::ai::pathfinding::PathNavigator* MobEntity::navigator() const
{
    return m_navigator.get();
}

entity::ai::EntitySenses* MobEntity::senses()
{
    return m_senses.get();
}

const entity::ai::EntitySenses* MobEntity::senses() const
{
    return m_senses.get();
}

math::Random MobEntity::getRandom() const
{
    // 基于实体ID和tick生成随机数种子
    return math::Random(static_cast<u64>(m_id) | (static_cast<u64>(m_ticksExisted) << 32));
}

bool MobEntity::isBeingRidden() const
{
    return hasPassengers();
}

void MobEntity::clearNavigation()
{
    if (m_navigator) {
        m_navigator->clearPath();
    }
}

void MobEntity::playAmbientSound()
{
    auto soundEvent = getAmbientSound();
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

void MobEntity::playAttackSound(LivingEntity& target)
{
    (void)target;
}

void MobEntity::lookAt(const Entity& target, f32 deltaYaw, f32 deltaPitch)
{
    lookAt(target.x(), target.y() + target.eyeHeight(), target.z(), deltaYaw, deltaPitch);
}

void MobEntity::lookAt(f64 x, f64 y, f64 z, f32 deltaYaw, f32 deltaPitch)
{
    if (m_lookController) {
        m_lookController->setLookPosition(x, y, z, deltaYaw, deltaPitch);
    }
}

void MobEntity::tick()
{
    // 更新父类（LivingEntity::tick() 已经调用 aiStep()）
    LivingEntity::tick();

    // MC 1.16.5: 空闲时间在 tick 开头递增
    ++m_idleTime;

    // 环境声音检查
    if (isAlive()) {
        math::Random random = getRandom();
        if (random.nextInt(1000) < m_livingSoundTime++) {
            m_livingSoundTime = -getTalkInterval();
            playAmbientSound();
        }
    }

    // MC 1.16.5 updateEntityActionState() 顺序:
    // 1. 感知更新
    if (m_senses) {
        m_senses->tick();
    }

    // 2. 目标选择器 (先于 goalSelector)
    // 3. 行为目标选择器
    if (m_aiEnabled) {
        m_targetSelector.tick();
        m_goalSelector.tick();

        // 4. 导航器更新
        if (m_navigator) {
            m_navigator->tick();
        }

        // 5. AI 任务更新 (子类可重写)
        updateAITasks();

        // MC 1.16.5: 每 5 tick 更新移动目标标志
        if (m_ticksExisted % 5 == 0) {
            updateMovementGoalFlags();
        }
    }

    // 6. 控制器更新 (顺序: move -> look -> jump)
    if (m_moveController) {
        m_moveController->tick();
    }
    if (m_lookController) {
        m_lookController->tick();
    }
    if (m_jumpController) {
        m_jumpController->tick();
    }

    // 注意：aiStep() 已在 LivingEntity::tick() 中调用，这里不需要再次调用
}

void MobEntity::updateMovementGoalFlags()
{
    // MC 1.16.5: 根据骑乘状态更新目标标志
    // 如果被骑乘，禁用 MOVE/JUMP/LOOK 标志
    bool canMove = !isBeingRidden();

    m_goalSelector.setFlag(entity::ai::GoalFlag::Move, canMove);
    m_goalSelector.setFlag(entity::ai::GoalFlag::Jump, canMove);
    m_goalSelector.setFlag(entity::ai::GoalFlag::Look, canMove);
}

std::optional<ResourceLocation> MobEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

void MobEntity::playHurtSound(DamageSource& source)
{
    m_livingSoundTime = -getTalkInterval();
    LivingEntity::playHurtSound(source);
}

void MobEntity::dropExperience()
{
    // 如果有经验值，生成经验球
    if (m_experienceValue > 0 && m_world) {
        math::Random rng = getRandom();
        entity::ExperienceDropHandler::spawnHostileMobExperience(m_world, x(), y(), z(), m_experienceValue, &rng);
    }
}

bool MobEntity::isInDaylight() const
{
    // MC 1.16.5 MobEntity.isInDaylight()
    // 检查条件：
    // 1. 世界为白天 (isDaytime)
    // 2. 不在客户端
    // 3. 亮度 > 0.5
    // 4. 随机检查
    // 5. 天空可见 (canSeeSky)

    if (m_world == nullptr || m_world->isClientSide()) {
        return false;
    }

    // MC 1.16.5: world.isDaytime()
    // dayTime < 12000 为白天
    if (!m_world->isDaytime()) {
        return false;
    }

    // MC 1.16.5: getBrightness() > 0.5F
    f32 brightness = getBrightness();
    if (brightness <= 0.5f) {
        return false;
    }

    // MC 1.16.5: rand.nextFloat() * 30.0F < (brightness - 0.4F) * 2.0F
    // 随机检查，亮度越高越容易触发
    math::Random rng = getRandom();
    f32 randomCheck = rng.nextFloat() * 30.0f;
    f32 brightnessThreshold = (brightness - 0.4f) * 2.0f;
    if (randomCheck >= brightnessThreshold) {
        return false;
    }

    // MC 1.16.5: 获取检测位置
    // 如果骑乘船，检测位置向上偏移一格
    BlockPos pos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::round(y())), static_cast<i32>(std::floor(z())));

    // MC 1.16.5: 如果实体骑乘船，检测位置向上偏移一格
    // 原因：船在水面上，生物坐在船中位置较低，需要向上偏移才能正确检测天空可见性
    if (isRiding()) {
        EntityId vehicleId = getVehicle();
        if (vehicleId != INVALID_ENTITY_ID && m_world != nullptr) {
            const Entity* vehicle = m_world->getEntity(vehicleId);
            if (vehicle != nullptr && dynamic_cast<const entity::BoatEntity*>(vehicle) != nullptr) {
                pos = pos.up();
            }
        }
    }

    // MC 1.16.5: world.canSeeSky(blockpos)
    return m_world->canSeeSky(pos);
}

bool MobEntity::attackEntityAsMob(LivingEntity& target)
{
    // MC 1.16.5 MobEntity.attackEntityAsMob()

    // 1. 获取攻击伤害属性
    f32 attackDamage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0));

    // 2. 获取击退强度属性
    f32 knockbackStrength = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 0.0));

    // 3. 如果目标有武器，应用附魔伤害加成和击退附魔
    // 获取主手武器
    const ItemStack& mainHand = getMainHandItem();

    if (!mainHand.isEmpty()) {
        // 附魔伤害加成（锋利、亡灵杀手、节肢杀手）
        // 使用 PlayerAttackHelper::getEnchantmentDamageBonus 计算附魔伤害
        attackDamage +=
            entity::combat::PlayerAttackHelper::getEnchantmentDamageBonus(mainHand, target.getCreatureAttribute());

        // 击退附魔
        i32 knockbackLevel =
            item::enchant::EnchantmentHelper::getEnchantmentLevel(mainHand, &item::enchant::AllEnchantments::KNOCKBACK);
        if (knockbackLevel > 0) {
            knockbackStrength += static_cast<f32>(knockbackLevel) * 0.5f;
        }
    }

    // 4. 火焰附加（在攻击前应用，MC 1.16.5 逻辑）
    i32 fireAspectLevel = 0;
    if (!mainHand.isEmpty()) {
        fireAspectLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
            mainHand, &item::enchant::AllEnchantments::FIRE_ASPECT);
    }

    // 5. 创建伤害来源并应用伤害
    EntityDamageSource damageSource = DamageSources::mobAttack(this);

    // 如果有火焰附加，在攻击前点燃目标 1 秒（用于燃烧传递）
    // MC 1.16.5: setFire 接收 ticks，1 秒 = 20 ticks
    if (fireAspectLevel > 0) {
        target.setFire(20); // 1 秒 = 20 ticks
    }

    bool attacked = target.hurt(damageSource, attackDamage);

    if (attacked) {
        // 6. 应用击退
        if (knockbackStrength > 0.0f) {
            // 计算击退方向
            f64 ratioX = static_cast<f64>(position().x - target.position().x);
            f64 ratioZ = static_cast<f64>(position().z - target.position().z);

            // 归一化方向向量
            f64 length = std::sqrt(ratioX * ratioX + ratioZ * ratioZ);
            if (length > 0.0) {
                ratioX /= length;
                ratioZ /= length;

                // 击退受击退抗性影响
                knockbackStrength = static_cast<f32>(static_cast<f64>(knockbackStrength) *
                    (1.0 - target.getAttributeValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.0)));

                if (knockbackStrength > 0.0f) {
                    // MC 1.16.5 LivingEntity.applyKnockback()
                    Vector3 velocity = target.velocity();

                    f64 knockbackX = ratioX * static_cast<f64>(knockbackStrength);
                    f64 knockbackZ = ratioZ * static_cast<f64>(knockbackStrength);

                    f64 newVelocityY;
                    if (target.onGround()) {
                        newVelocityY =
                            std::min(0.4, static_cast<f64>(velocity.y) / 2.0 + static_cast<f64>(knockbackStrength));
                    } else {
                        newVelocityY = static_cast<f64>(velocity.y);
                    }

                    target.setVelocity(static_cast<f32>(static_cast<f64>(velocity.x) / 2.0 - knockbackX),
                        static_cast<f32>(newVelocityY),
                        static_cast<f32>(static_cast<f64>(velocity.z) / 2.0 - knockbackZ));
                    target.setOnGround(false);
                }
            }
        }

        // 7. 应用火焰附加（攻击后应用完整燃烧时间）
        if (fireAspectLevel > 0) {
            // MC 1.16.5: 火焰附加持续时间 = level * 4 秒
            target.setFire(fireAspectLevel * 4 * 20); // 20 ticks per second
        }

        // 8. 设置最后攻击者
        target.setLastHurtBy(this);

        // 9. 播放攻击声音
        playAttackSound(target);

        // 10. 设置攻击者的速度（击退反作用）
        // MC 1.16.5: this.setMotion(this.getMotion().mul(0.6D, 1.0D, 0.6D));
        setVelocity(velocity().x * 0.6f, velocity().y, velocity().z * 0.6f);
    }

    return attacked;
}

// ========== 玩家交互 ==========

ActionResultType MobEntity::processInitialInteract(Player& player, Hand hand)
{
    // MC 1.16.5: MobEntity.processInitialInteract()
    if (!isAlive()) {
        return ActionResultType::Pass;
    }

    // TODO: 检查拴绳（如果玩家手持拴绳）
    // TODO: 检查命名牌
    // TODO: 检查刷怪蛋

    // 调用子类的交互逻辑
    ActionResultType result = interactMob(player, hand);
    if (result == ActionResultType::Success || result == ActionResultType::Consume) {
        return result;
    }

    // 调用父类实现
    return LivingEntity::processInitialInteract(player, hand);
}

ActionResultType MobEntity::interactMob(Player& /*player*/, Hand /*hand*/)
{
    // MC 1.16.5: MobEntity.func_230254_b_()
    // 基类默认返回 Pass，子类可重写以处理特定交互
    return ActionResultType::Pass;
}

} // namespace mc
