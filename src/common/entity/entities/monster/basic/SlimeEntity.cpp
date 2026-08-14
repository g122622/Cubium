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

#include "SlimeEntity.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/goals/special/SlimeGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../combat/DifficultyInstance.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/EntityType.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../passive/golem/IronGolemEntity.hpp"
#include "../../player/Player.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc {

SlimeEntity::SlimeEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    // 史莱姆不在阳光下燃烧
    setBurnsInDaylight(false);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SlimeEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SlimeEntity>(EntityInstanceId(0), registry);
}

void SlimeEntity::setSlimeSize(i32 size, bool resetHealth)
{
    i32 clampedSize = std::clamp(size, 1, 4);
    if (m_size == clampedSize) {
        return;
    }

    m_size = clampedSize;
    updateSizeAttributes();
    refreshDimensions();

    // 重置生命值
    if (resetHealth) {
        setHealth(maxHealth());
    }
}

std::optional<ResourceLocation> SlimeEntity::getHurtSound(DamageSource& /*source*/) const
{
    // 对齐 MC 1.21.11 Slime.getHurtSound：直接用 SoundEvents 字面量，不依赖 typeId。
    // 此前用 makeSoundEventId 派生 ID，在 typeId 未设置（如单元测试直接构造）时返回 nullopt。
    if (isSmallSlime()) {
        return SoundEvents::ENTITY_SLIME_HURT_SMALL;
    }
    return SoundEvents::ENTITY_SLIME_HURT;
}

std::optional<ResourceLocation> SlimeEntity::getDeathSound() const
{
    // 对齐 MC 1.21.11 Slime.getDeathSound
    if (isSmallSlime()) {
        return SoundEvents::ENTITY_SLIME_DEATH_SMALL;
    }
    return SoundEvents::ENTITY_SLIME_DEATH;
}

std::optional<ResourceLocation> SlimeEntity::getSquishSound() const
{
    // 对齐 MC 1.21.11 Slime.getSquishSound
    if (isSmallSlime()) {
        return SoundEvents::ENTITY_SLIME_SQUISH_SMALL;
    }
    return SoundEvents::ENTITY_SLIME_SQUISH;
}

void SlimeEntity::alterSquishAmount()
{
    // 挤压量向 0 衰减
    m_squishAmount *= 0.6f;
}

std::optional<ResourceLocation> SlimeEntity::getJumpSound() const
{
    // 跳跃音效使用挤压音效
    return getSquishSound();
}

particle::ParticleTypeId SlimeEntity::getSquishParticle() const
{
    // 史莱姆使用粘液粒子
    return particle::ParticleTypeId::ItemSlime;
}

i32 SlimeEntity::getJumpDelay() const
{
    // 返回 10-29 tick（0.5-1.45秒）
    math::Random& rng = getRandom();
    return rng.nextInt(10, 29);
}

void SlimeEntity::split()
{
    // 已废弃，使用 performSplit()
    performSplit();
}

f32 SlimeEntity::getAttackDamage() const
{
    // 对齐 Java Slime.getAttackDamage：返回 ATTACK_DAMAGE 属性值（updateSizeAttributes 设为 size）。
    // 岩浆怪 override 在此基础上 +2（见 MagmaCubeEntity::getAttackDamage）。
    return static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0));
}

void SlimeEntity::dealDamage(LivingEntity& target)
{
    // 对齐 Java Slime.dealDamage：canDamagePlayer() 为真时按 getAttackDamage() 伤害目标。
    // canDamagePlayer() 已含尺寸门控：史莱姆 size>1 才真，岩浆怪 override 始终真（不限尺寸）。
    // 故此处不再重复 m_size<=1 判断——此前该判断会让小型岩浆怪（canDamagePlayer=true）误不伤害，
    // 与 wiki "小型岩浆怪伤害=3" 矛盾。canDamagePlayer() 假时 onCollideWithPlayer 不会调本函数。
    if (!canDamagePlayer()) {
        return;
    }

    // 检查目标是否存活
    if (!target.isAlive()) {
        return;
    }

    // 伤害值取 getAttackDamage()（属性驱动，子类可 override）
    f32 damage = getAttackDamage();

    // 对目标造成伤害
    auto damageSource = DamageSources::mobAttack(this);
    target.hurt(damageSource, damage);
}

bool SlimeEntity::canDamagePlayer() const
{
    // 只有尺寸大于 1 的史莱姆才能伤害玩家（对齐 Java Slime.isDealsDamage: size>1）。
    // 岩浆怪 override 此方法不限尺寸（小型岩浆怪也能伤害，见 MagmaCubeEntity::canDamagePlayer）。
    return m_size > 1;
}

void SlimeEntity::onCollideWithPlayer(Player& player)
{
    // 玩家碰撞时尝试造成伤害。canDamagePlayer() 为尺寸/类型门控。
    // 注：参数类型为 Player& 以 override 基类 Entity::onCollideWithPlayer(Player&)，
    // 此前误用 LivingEntity& 不构成 override 致本函数从未被调用（死代码）。
    // Player 继承 LivingEntity，可直接传给 dealDamage(LivingEntity&)。
    dealDamage(player);
}

f32 SlimeEntity::eyeHeight() const
{
    return EYE_HEIGHT_FACTOR * height();
}

entity::EntitySize SlimeEntity::getDimensions(EntityPose /*pose*/) const
{
    f32 scaleFactor = SIZE_SCALE * static_cast<f32>(m_size);
    return entity::EntitySize::flexible(0.6f * scaleFactor, 0.6f * scaleFactor);
}

void SlimeEntity::dropExperience()
{
    // 经验值等于尺寸
    MonsterEntity::dropExperience();
}

void SlimeEntity::tick()
{
    // 更新挤压动画
    m_squishFactor += (m_squishAmount - m_squishFactor) * 0.5f;
    m_prevSquishFactor = m_squishFactor;

    MonsterEntity::tick();

    // 着地时的挤压效果
    if (onGround() && !m_wasOnGround) {
        // 着地时播放挤压音效和粒子
        auto squishSound = getSquishSound();
        if (squishSound) {
            playSound(*squishSound, getSoundVolume(), 1.0f);
        }

        // 挤压量设为负值
        m_squishAmount = -0.5f;

        // 生成粒子效果
        if (world() != nullptr && world()->isClientSide()) {
            math::Random& random = world()->getRandom();
            auto particleType = getSquishParticle();

            // 粒子数量 = 尺寸 * 8
            i32 particleCount = m_size * 8;
            for (i32 j = 0; j < particleCount; ++j) {
                // 随机角度和半径
                f32 angle = random.nextFloat() * math::TWO_PI;
                f32 radiusFactor = random.nextFloat() * 0.5f + 0.5f; // 0.5 to 1.0

                // 计算粒子位置偏移
                f32 offsetX = std::sin(static_cast<f64>(angle)) * static_cast<f32>(m_size) * 0.5f * radiusFactor;
                f32 offsetZ = std::cos(static_cast<f64>(angle)) * static_cast<f32>(m_size) * 0.5f * radiusFactor;

                // 在史莱姆脚底生成粒子
                world()->addParticle(particleType,
                    Vector3(x() + static_cast<f64>(offsetX), y(), z() + static_cast<f64>(offsetZ)),
                    Vector3(0.0, 0.0, 0.0));
            }
        }
    } else if (!onGround() && m_wasOnGround) {
        // 离地时的挤压量
        m_squishAmount = 1.0f;
    }

    m_wasOnGround = onGround();
    alterSquishAmount();
}

void SlimeEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // AI 目标选择器
    // 优先级 1: FloatGoal（游泳）
    // 优先级 2: AttackGoal（攻击）
    // 优先级 3: FaceRandomGoal（随机转向）
    // 优先级 5: HopGoal（跳跃）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::SlimeFloatGoal>(this));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::SlimeAttackGoal>(this));
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::SlimeFaceRandomGoal>(this));
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::SlimeHopGoal>(this));

    // 目标选择器
    // 优先级 1: 攻击玩家，距离 <= 10，需要视线，高度差 <= 4
    // 优先级 3: 攻击铁傀儡，需要视线
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this,
            true, // checkSight
            10,   // chance
            [this](const LivingEntity* target) -> bool {
                // Y 轴高度差必须 <= 4.0 格
                if (target == nullptr || !target->isAlive()) {
                    return false;
                }
                f64 yDiff = std::abs(target->y() - this->y());
                return yDiff <= 4.0;
            }));
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this,
            true)); // checkSight
}

void SlimeEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 默认尺寸为1
    m_size = 1;
    updateSizeAttributes();
}

void SlimeEntity::updateSizeAttributes()
{
    // 根据尺寸更新属性
    // HP = size * size
    // Speed = 0.2 + 0.1 * size
    // AttackDamage = size
    f32 health = static_cast<f32>(m_size * m_size);
    f32 speed = 0.2f + 0.1f * static_cast<f32>(m_size);
    f32 damage = static_cast<f32>(m_size);

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, health);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, speed);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, damage);

    // 经验值等于尺寸
    m_experienceValue = m_size;
}

void SlimeEntity::remove()
{
    // 在移除前尝试分裂
    // 只有尺寸大于 1 的史莱姆才会分裂
    if (canSplit()) {
        performSplit();
    }

    // 调用父类移除
    MonsterEntity::remove();
}

void SlimeEntity::performSplit()
{
    // 只能在服务端执行
    if (world() == nullptr || world()->isClientSide()) {
        return;
    }

    // 分裂后的小史莱姆数量：2-4 个
    math::Random& rng = world()->getRandom();
    i32 splitCount = rng.nextInt(SPLIT_COUNT_MIN, SPLIT_COUNT_MAX);

    // 新史莱姆的尺寸 = 当前尺寸 / 2
    i32 newSize = m_size / 2;
    if (newSize < 1) {
        return; // 不能分裂成更小的史莱姆
    }

    // 获取实体类型来创建新实例。用当前实体自身的类型（entityType()）而非硬编码 SLIME——
    // 这样 MagmaCubeEntity（继承 SlimeEntity）分裂时取到 magma_cube 类型，工厂绑定
    // MagmaCubeEntity::create，生成真正的 MagmaCubeEntity 对象（对齐 Java Slime.remove 的
    // convertTo(本类 EntityType, true) 语义）。此前硬编码 SLIME 会导致岩浆怪分裂出" typeId 为
    // magma_cube 但 C++ 对象类型为 SlimeEntity "的混乱态（见第 423 行 setTypeId 后的字段错配）。
    const entity::EntityType* slimeType = entityType();
    if (!slimeType || !slimeType->canSummon()) {
        spdlog::warn("SlimeEntity: entity type not found or not summonable: {}", getTypeId());
        return;
    }

    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* ecsRegistry = world()->entityRegistry();
    if (ecsRegistry == nullptr) {
        spdlog::warn("SlimeEntity: World has no entity registry");
        return;
    }

    // 计算分裂位置偏移
    f32 offsetScale = static_cast<f32>(m_size) / 4.0f;

    // 保存当前实体的属性用于继承
    bool persistenceRequired = isNoDespawnRequired();
    bool invulnerable = isInvulnerable();
    std::string customName = customNameText(); // 获取自定义名称文本

    // 生成小史莱姆
    for (i32 l = 0; l < splitCount; ++l) {
        // 计算每个小史莱姆的偏移位置
        f32 offsetX = (static_cast<f32>(l % 2) - 0.5f) * offsetScale;
        f32 offsetZ = (static_cast<f32>(l / 2) - 0.5f) * offsetScale;

        // 创建新史莱姆
        std::unique_ptr<Entity> entity = slimeType->create(world(), *ecsRegistry);
        if (!entity) {
            spdlog::warn("SlimeEntity: Failed to create slime entity");
            continue;
        }

        SlimeEntity* smallSlime = dynamic_cast<SlimeEntity*>(entity.get());
        if (!smallSlime) {
            spdlog::warn("SlimeEntity: Created entity is not a SlimeEntity");
            continue;
        }

        // 释放所有权，我们直接使用原始指针
        entity.release();

        // 继承父实体的属性
        if (persistenceRequired) {
            smallSlime->enablePersistence();
        }

        // 继承自定义名称（如果有）
        if (!customName.empty()) {
            smallSlime->setCustomName(customName);
        }

        // 继承无敌状态
        smallSlime->setInvulnerable(invulnerable);

        // 设置史莱姆尺寸（会自动设置生命值）
        smallSlime->setSlimeSize(newSize, true);

        // 设置位置和旋转
        f32 spawnX = static_cast<f32>(x()) + offsetX;
        f32 spawnY = static_cast<f32>(y()) + 0.5f;
        f32 spawnZ = static_cast<f32>(z()) + offsetZ;
        f32 spawnYaw = rng.nextFloat() * 360.0f;

        smallSlime->setPosition(spawnX, spawnY, spawnZ);
        smallSlime->setRotation(spawnYaw, 0.0f);

        // 对分裂生成的小史莱姆调用 finalizeSpawn（使用位置感知的区域难度）
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*world(),
            BlockPos(
                static_cast<i32>(std::floor(spawnX)), static_cast<i32>(spawnY), static_cast<i32>(std::floor(spawnZ))));
        smallSlime->finalizeSpawn(*world(), difficultyInstance, world::spawn::SpawnReason::Reinforcement);

        // 设置随机速度
        smallSlime->setVelocity(
            (rng.nextFloat() - 0.5f) * 0.5f, rng.nextFloat() * 0.5f, (rng.nextFloat() - 0.5f) * 0.5f);

        // 设置类型 ID
        smallSlime->setTypeId(getTypeId());

        // 生成到世界中
        std::unique_ptr<Entity> slimePtr(smallSlime);
        world()->spawnEntity(std::move(slimePtr));
    }
}

} // namespace mc
