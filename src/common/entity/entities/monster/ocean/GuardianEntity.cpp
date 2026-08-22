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

#include "GuardianEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/fluid/Fluid.hpp"
#include "../../../../world/fluid/FluidTags.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../ai/goal/goals/special/GuardianAttackGoal.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

GuardianEntity::GuardianEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> GuardianEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<GuardianEntity>(EntityInstanceId(0), registry);
}

bool GuardianEntity::isInWater() const
{
    // 调用父类的 isInWater() 方法
    // Entity::isInWater() 已经在 updateEnvironmentState() 中正确更新
    return MonsterEntity::isInWater();
}

void GuardianEntity::tick()
{
    MonsterEntity::tick();

    // 注意: 激光攻击由 GuardianAttackGoal AI 目标处理
    // 激光充能、目标追踪、伤害计算等逻辑都在 GuardianAttackGoal 中实现

    // 更新尖刺动画
    m_spikeTimer++;
    if (m_spikeTimer >= 40) {
        m_spikeTimer = 0;
        m_spikesRetracted = !m_spikesRetracted;
    }
}

void GuardianEntity::registerGoals()
{
    // 调用父类方法
    MonsterEntity::registerGoals();

    // 目标优先级说明:
    // - 优先级数字越小，优先级越高
    // - 目标选择器(targetSelector)用于选择攻击目标
    // - 行为目标选择器(goalSelector)用于控制行为

    // ========== 行为目标选择器 (goalSelector) ==========
    // 优先级 4: 激光攻击目标
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::GuardianAttackGoal>(this));

    // 优先级 5: 向限制区域移动
    // 守卫者有移动限制区域（海底神殿附近）
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::MoveTowardsRestrictionGoal>(this, 1.0));

    // 优先级 7: 随机漫步
    // 80 tick 的移动间隔
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 1.0, 80));

    // 优先级 8: 看向玩家 (8格内)
    m_goalSelector.addGoal(
        8, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            if (!entity) return false;
            return entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 8: 看向同类守卫者 (12格内，低频率)
    m_goalSelector.addGoal(
        8, std::make_unique<entity::ai::goal::LookAtGoal>(this, 12.0f, 0.01f, [](const LivingEntity* entity) -> bool {
            if (!entity) return false;
            auto type = entity->entityType();
            return type == entity::VanillaEntityTypeKeys::GUARDIAN ||
                type == entity::VanillaEntityTypeKeys::ELDER_GUARDIAN;
        }));

    // 优先级 9: 随机看向
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // ========== 目标选择器 (targetSelector) ==========
    // 优先级 1: 搜索最近的可攻击目标
    // 参数: 10 = 每10tick检查一次, true = 需要视线, false = 不需要近战距离
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            true, // checkSight - 需要视线检查
            10,   // chance - 每10tick检查一次
            // 目标筛选谓词: 对齐 Java 1.21.11 Guardian.GuardianAttackSelector.test（Guardian.java:436-438）
            // vanilla: (target instanceof Player || instanceof Squid || instanceof Axolotl) && distSq > 9.0
            // 注：Java 的 instanceof Squid 涵盖 GlowSquid（GlowSquid extends Squid）。
            //     Cubium entityType 是扁平枚举指针无继承层级，须显式列举 GLOW_SQUID。
            [this](const LivingEntity* candidate) -> bool {
                if (!candidate || !candidate->isAlive()) {
                    return false;
                }

                // 类型筛选: 攻击玩家、鱿鱼、发光鱿鱼、美西螈（对齐 wiki tech_守卫者.txt#攻击
                //   "约16格激光射程内的玩家、鱿鱼、发光鱿鱼和美西螈"）。
                //   vanilla instanceof Squid 涵盖 GlowSquid；Cubium 须显式列举两者。
                auto type = candidate->entityType();
                bool isPlayer = (type == entity::VanillaEntityTypeKeys::PLAYER);
                bool isSquid =
                    (type == entity::VanillaEntityTypeKeys::SQUID || type == entity::VanillaEntityTypeKeys::GLOW_SQUID);
                bool isAxolotl = (type == entity::VanillaEntityTypeKeys::AXOLOTL);
                if (!isPlayer && !isSquid && !isAxolotl) {
                    return false;
                }

                // 玩家特殊检查: 创造模式和观察者模式不能被攻击
                if (isPlayer) {
                    const Player* player = dynamic_cast<const Player*>(candidate);
                    if (player != nullptr && (player->isCreative() || player->isSpectator())) {
                        return false;
                    }
                }

                // 距离筛选: 必须距离 > 3 格
                f64 distSq = this->distanceSqTo(*candidate);
                if (distSq <= 9.0) { // 3.0 * 3.0 = 9.0
                    return false;
                }

                return true;
            }));
}

void GuardianEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 守卫者的属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, LASER_DAMAGE);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

std::optional<ResourceLocation> GuardianEntity::getAmbientSound() const
{
    // 在水中和陆地使用不同音效
    if (isInWater()) {
        return SoundEvents::ENTITY_GUARDIAN_AMBIENT;
    }
    return SoundEvents::ENTITY_GUARDIAN_AMBIENT_LAND;
}

std::optional<ResourceLocation> GuardianEntity::getHurtSound(DamageSource& /*source*/) const
{
    // 在水中和陆地使用不同音效
    if (isInWater()) {
        return SoundEvents::ENTITY_GUARDIAN_HURT;
    }
    return SoundEvents::ENTITY_GUARDIAN_HURT_LAND;
}

std::optional<ResourceLocation> GuardianEntity::getDeathSound() const
{
    // 在水中和陆地使用不同音效
    if (isInWater()) {
        return SoundEvents::ENTITY_GUARDIAN_DEATH;
    }
    return SoundEvents::ENTITY_GUARDIAN_DEATH_LAND;
}

void GuardianEntity::playLaserSound()
{
    playSound(SoundEvents::ENTITY_GUARDIAN_ATTACK, 1.0f, 1.0f);
}

f32 GuardianEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 守卫者偏好水中位置：在水中返回 10.0f + lightCost，否则返回父类值
    // 对应 MC Guardian.getWalkTargetValue:
    //   return fluid.is(WATER) ? 10.0F + level.getPathfindingCostFromLightLevels(pos)
    //                          : super.getWalkTargetValue(pos, level);
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }

    BlockPos pos(static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(z));
    const fluid::FluidState* fluid = worldPtr->getFluidState(pos);
    if (fluid != nullptr && !fluid->isEmpty() && fluid->getFluid().isIn(fluid::FluidTags::WATER())) {
        f32 brightness = worldPtr->getBrightness(pos);
        return 10.0f + (brightness - 0.5f);
    }

    return MonsterEntity::getPathWeight(x, y, z);
}

} // namespace mc
