/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "PhantomEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/controller/PhantomLookController.hpp"
#include "common/entity/ai/controller/PhantomMovementController.hpp"
#include "common/entity/ai/goal/goals/special/PhantomGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/FlyingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>

namespace mc {

std::unique_ptr<Entity> PhantomEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PhantomEntity>(EntityInstanceId(0), registry);
}

PhantomEntity::PhantomEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : FlyingEntity(id, registry)
{
    // 注册 AI 目标与属性。
    // C++ 虚函数在基类构造函数中不会派发到派生类（FlyingEntity/MobEntity 构造期间调 registerGoals
    // 命中的是空基类实现），必须在 PhantomEntity 自己的构造函数体里显式调用，参考 CatEntity 模式。
    // 此前漏调导致 phantom 的 m_goalSelector/m_targetSelector 为空，所有 goal 不生效——phantom 静止
    // 悬浮，不会环绕飞行也不会因猫切换俯冲/盘旋（GameTest phantoms_should_fly_from_cats 假通过/失败）。
    registerGoals();
    registerAttributes();

    // 幻翼在阳光下燃烧
    setExperienceValue(5);

    // 安装专用的移动控制器和视线控制器
    // PhantomMovementController 直接操控速度向量实现飞行
    // PhantomLookController 为空操作，幻翼朝向完全由移动控制器控制
    m_moveController = std::make_unique<entity::ai::controller::PhantomMovementController>(this);
    m_lookController = std::make_unique<entity::ai::controller::PhantomLookController>(this);

    // 计算翅膀拍打偏移量，使不同幻翼的拍打节奏错开
    m_uniqueFlapOffset = static_cast<i64>(id) % TICKS_PER_FLAP;
}

void PhantomEntity::setPhantomSize(i32 size)
{
    m_phantomSize = std::clamp(size, 0, MAX_PHANTOM_SIZE);
    // 更新攻击力
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE,
        BASE_ATTACK_DAMAGE + static_cast<f32>(m_phantomSize) * SIZE_ATTACK_BONUS);
    refreshDimensions();
}

entity::EntitySize PhantomEntity::getDimensions(EntityPose pose) const
{
    // 尺寸随幻翼大小变化
    f32 scaleFactor = 1.0f + 0.15f * static_cast<f32>(m_phantomSize);
    return entity::EntitySize::flexible(0.9f * scaleFactor, 0.5f * scaleFactor);
}

std::optional<ResourceLocation> PhantomEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> PhantomEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> PhantomEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

void PhantomEntity::tick()
{
    FlyingEntity::tick();

    // 幻翼在阳光下燃烧（亡灵生物属性）
    burnUndead();

    // 客户端侧效果
    if (world() != nullptr && world()->isClientSide()) {
        _clientTickBodyRotation();
        _clientTickEffects();
    }
}

void PhantomEntity::travel(f32 x, f32 y, f32 z)
{
    // 与 MC 原版一致：委托 FlyingEntity::travel() 并使用 0.2 飞行惯性因子
    // 飞行惯性因子 0.2 控制速度混合比例，值越小转向越灵活
    FlyingEntity::travel(x, y, z);
}

void PhantomEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    FlyingEntity::finalizeSpawn(world, difficulty, spawnReason);

    // 设置环绕位置为生成位置上方5格
    BlockPos spawnPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())) + 5, static_cast<i32>(std::floor(z())));
    m_orbitPosition = spawnPos;

    // 默认幻翼大小为0
    setPhantomSize(0);
}

bool PhantomEntity::canAttackType(const entity::EntityType& /*type*/) const
{
    // 覆盖 Mob 基类排除恶魂的限制，幻翼本身是飞行生物，可以攻击空中目标
    return true;
}

void PhantomEntity::registerGoals()
{
    FlyingEntity::registerGoals();

    // 攻击目标选择器：
    // 优先级 1: PickAttackGoal - 选择攻击阶段
    // 优先级 2: SweepAttackGoal - 俯冲攻击
    // 优先级 3: OrbitPointGoal - 环绕飞行
    // 目标选择器：
    // 优先级 1: AttackPlayerTargetGoal - 攻击玩家

    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::PhantomPickAttackGoal>(this));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::PhantomSweepAttackGoal>(this));
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::PhantomOrbitPointGoal>(this));
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::PhantomAttackPlayerTargetGoal>(this));
}

void PhantomEntity::registerAttributes()
{
    FlyingEntity::registerAttributes();

    // 幻翼属性：生命值20，移动速度0（飞行生物不使用地面速度），攻击力6，追踪距离64
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, BASE_ATTACK_DAMAGE);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
}

void PhantomEntity::_clientTickBodyRotation()
{
    // 将身体旋转和头部旋转同步为偏航角，使幻翼整体朝向与飞行方向一致
    // 与 MC 原版 PhantomBodyRotationControl.clientTick() 一致
    setRotationYawHead(yaw());
    setRenderYawOffset(yaw());
}

void PhantomEntity::_clientTickEffects()
{
    // 检测翅膀拍打的过零点（余弦从正变零/负），播放拍打音效
    f32 flapCosCurrent = std::cos(
        (static_cast<f32>(m_uniqueFlapOffset + m_ticksExisted)) * FLAP_DEGREES_PER_TICK * math::DEG_TO_RAD + math::PI);
    f32 flapCosNext = std::cos(
        (static_cast<f32>(m_uniqueFlapOffset + m_ticksExisted + 1)) * FLAP_DEGREES_PER_TICK * math::DEG_TO_RAD +
        math::PI);

    if (flapCosCurrent > 0.0f && flapCosNext <= 0.0f) {
        // 翅膀拍打过零点：播放拍打音效
        math::Random& rng = getRandom();
        if (!isSilent()) {
            world()->playSound(SoundEvents::ENTITY_PHANTOM_FLAP,
                getSoundCategory(),
                m_builtIn.stateVector->m_pos,
                0.95f + rng.nextFloat() * 0.05f,
                0.95f + rng.nextFloat() * 0.05f);
        }
    }

    // 翼尖菌丝粒子
    // 计算翼尖位置：根据偏航角和拍打余弦值
    f32 wingSpan = width() * 1.48f;
    f32 cosYaw = std::cos(yaw() * math::DEG_TO_RAD);
    f32 sinYaw = std::sin(yaw() * math::DEG_TO_RAD);
    f32 wingTipX1 = static_cast<f32>(x()) + cosYaw * wingSpan;
    f32 wingTipZ1 = static_cast<f32>(z()) + sinYaw * wingSpan;
    f32 wingTipX2 = static_cast<f32>(x()) - cosYaw * wingSpan;
    f32 wingTipZ2 = static_cast<f32>(z()) - sinYaw * wingSpan;
    f32 wingY = static_cast<f32>(y()) + (0.3f + flapCosCurrent * 0.45f) * height() * 2.5f;

    // MC 原版使用 MYCELIUM 粒子类型（SuspendedTownParticle）
    using namespace particle;
    world()->addParticle(ParticleTypeId::Mycelium, Vector3(wingTipX1, wingY, wingTipZ1), Vector3(0.0, 0.0, 0.0));
    world()->addParticle(ParticleTypeId::Mycelium, Vector3(wingTipX2, wingY, wingTipZ2), Vector3(0.0, 0.0, 0.0));
}

} // namespace mc
