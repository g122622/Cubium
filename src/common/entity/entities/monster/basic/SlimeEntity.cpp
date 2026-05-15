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
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/EntityType.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../ai/controller/JumpController.hpp"
#include "../../../ai/controller/MovementController.hpp"
#include "../../../ai/goal/goals/special/SlimeGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../passive/golem/IronGolemEntity.hpp"
#include "../../player/Player.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc {

SlimeEntity::SlimeEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // MC 1.16.5: 史莱姆不在阳光下燃烧
    setBurnsInDaylight(false);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SlimeEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SlimeEntity>(LegacyEntityType::Slime, EntityId(0));
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

    // MC 1.16.5: 重置生命值
    if (resetHealth) {
        setHealth(maxHealth());
    }
}

std::optional<ResourceLocation> SlimeEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: 小史莱姆用 hurt_small
    if (isSmallSlime()) {
        return makeSoundEventId("hurt_small");
    }
    // 大史莱姆用 hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> SlimeEntity::getDeathSound() const
{
    // MC 1.16.5: 小史莱姆用 death_small
    if (isSmallSlime()) {
        return makeSoundEventId("death_small");
    }
    // 大史莱姆用 death
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> SlimeEntity::getSquishSound() const
{
    // MC 1.16.5: 小史莱姆用 squish_small
    if (isSmallSlime()) {
        return makeSoundEventId("squish_small");
    }
    // 大史莱姆用 squish
    return makeSoundEventId("squish");
}

void SlimeEntity::alterSquishAmount()
{
    // MC 1.16.5: alterSquishAmount()
    // 挤压量向 0 衰减
    m_squishAmount *= 0.6f;
}

std::optional<ResourceLocation> SlimeEntity::getJumpSound() const
{
    // MC 1.16.5: 跳跃音效
    // 小史莱姆用 squish_small，大史莱姆用 squish
    return getSquishSound();
}

i32 SlimeEntity::getJumpDelay() const
{
    // MC 1.16.5: getJumpDelay() - return this.rand.nextInt(20) + 10;
    // 返回 10-29 tick (0.5-1.45秒)
    math::Random rng = getRandom();
    return rng.nextInt(10, 29);
}

void SlimeEntity::split()
{
    // 已废弃，使用 performSplit()
    performSplit();
}

void SlimeEntity::dealDamage(LivingEntity& target)
{
    // MC 1.16.5: dealDamage()
    // 只有尺寸大于 1 的史莱姆才能造成伤害
    if (m_size <= 1) {
        return;
    }

    // 检查目标是否存活
    if (!target.isAlive()) {
        return;
    }

    // MC 1.16.5: 伤害值等于尺寸
    f32 damage = static_cast<f32>(m_size);

    // 对目标造成伤害
    auto damageSource = DamageSources::mobAttack(this);
    target.hurt(damageSource, damage);
}

bool SlimeEntity::canDamagePlayer() const
{
    // MC 1.16.5: 只有尺寸大于 1 的史莱姆才能伤害玩家
    return m_size > 1;
}

void SlimeEntity::onCollideWithPlayer(LivingEntity& player)
{
    // MC 1.16.5: onCollideWithPlayer()
    if (canDamagePlayer()) {
        dealDamage(player);
    }
}

f32 SlimeEntity::eyeHeight() const
{
    // MC 1.16.5: 0.625F * height
    return EYE_HEIGHT_FACTOR * height();
}

entity::EntitySize SlimeEntity::getDimensions(EntityPose /*pose*/) const
{
    // MC 1.16.5: scale by 0.255F * size
    f32 scaleFactor = SIZE_SCALE * static_cast<f32>(m_size);
    return entity::EntitySize::flexible(0.6f * scaleFactor, 0.6f * scaleFactor);
}

void SlimeEntity::dropExperience()
{
    // MC 1.16.5: 经验值等于尺寸
    MonsterEntity::dropExperience();
}

void SlimeEntity::tick()
{
    // MC 1.16.5 SlimeEntity.tick()

    // 更新挤压动画
    m_squishFactor += (m_squishAmount - m_squishFactor) * 0.5f;
    m_prevSquishFactor = m_squishFactor;

    MonsterEntity::tick();

    // 着地时的挤压效果
    if (onGround() && !m_wasOnGround) {
        // MC 1.16.5: 着地时播放挤压音效和粒子
        auto squishSound = getSquishSound();
        if (squishSound) {
            playSound(*squishSound, getSoundVolume(), 1.0f);
        }

        // 挤压量设为负值
        m_squishAmount = -0.5f;

        // MC 1.16.5: 生成粒子效果
        // 参考: SlimeEntity.tick() - for (int j = 0; j < size * 8; ++j)
        if (world() != nullptr && world()->isClientSide()) {
            using namespace mc::client::renderer::trident::particle;
            math::Random& random = world()->getRandom();

            // 粒子数量 = 尺寸 * 8
            i32 particleCount = m_size * 8;
            for (i32 j = 0; j < particleCount; ++j) {
                // MC 1.16.5: 随机角度和半径
                f32 angle = random.nextFloat() * 2.0f * 3.14159265f; // 0 to 2*PI
                f32 radiusFactor = random.nextFloat() * 0.5f + 0.5f; // 0.5 to 1.0

                // 计算粒子位置偏移
                f32 offsetX = std::sin(static_cast<f64>(angle)) * static_cast<f32>(m_size) * 0.5f * radiusFactor;
                f32 offsetZ = std::cos(static_cast<f64>(angle)) * static_cast<f32>(m_size) * 0.5f * radiusFactor;

                // 在史莱姆脚底生成粒子
                world()->addParticle(ParticleTypeId::ItemSlime,
                    Vector3(x() + static_cast<f64>(offsetX), y(), z() + static_cast<f64>(offsetZ)),
                    Vector3(0.0, 0.0, 0.0));
            }
        }
    } else if (!onGround() && m_wasOnGround) {
        // MC 1.16.5: 离地时的挤压量
        m_squishAmount = 1.0f;
    }

    m_wasOnGround = onGround();
    alterSquishAmount();
}

void SlimeEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // MC 1.16.5 SlimeEntity.registerGoals()
    // 优先级 1: FloatGoal（游泳）
    // 优先级 2: AttackGoal（攻击）
    // 优先级 3: FaceRandomGoal（随机转向）
    // 优先级 5: HopGoal（跳跃）
    //
    // 目标选择器：
    // 优先级 1: NearestAttackableTargetGoal<Player>（攻击玩家，高度差<=4）
    // 优先级 3: NearestAttackableTargetGoal<IronGolem>（攻击铁傀儡）

    // AI 目标选择器
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::SlimeFloatGoal>(this));
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::SlimeAttackGoal>(this));
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::SlimeFaceRandomGoal>(this));
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::SlimeHopGoal>(this));

    // 目标选择器
    // MC 1.16.5 SlimeEntity.registerGoals():
    // 优先级 1: 攻击玩家，距离 <= 10，需要视线，高度差 <= 4
    // 优先级 3: 攻击铁傀儡，需要视线
    m_targetSelector.addGoal(1,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(
            this,
            true,   // checkSight
            10,     // chance
            [this](const LivingEntity* target) -> bool {
                // MC 1.16.5: Y 轴高度差必须 <= 4.0 格
                if (target == nullptr || !target->isAlive()) {
                    return false;
                }
                f64 yDiff = std::abs(target->y() - this->y());
                return yDiff <= 4.0;
            }));
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(
            this,
            true));  // checkSight
}

void SlimeEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // MC 1.16.5: 默认尺寸为1
    m_size = 1;
    updateSizeAttributes();
}

void SlimeEntity::updateSizeAttributes()
{
    // MC 1.16.5: 根据尺寸更新属性
    // HP = size * size
    // Speed = 0.2 + 0.1 * size
    // AttackDamage = size
    f32 health = static_cast<f32>(m_size * m_size);
    f32 speed = 0.2f + 0.1f * static_cast<f32>(m_size);
    f32 damage = static_cast<f32>(m_size);

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, health);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, speed);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, damage);

    // 经验值等于尺寸
    m_experienceValue = m_size;
}

void SlimeEntity::remove()
{
    // MC 1.16.5: 在移除前尝试分裂
    // 只有尺寸大于 1 的史莱姆才会分裂
    if (canSplit()) {
        performSplit();
    }

    // 调用父类移除
    MonsterEntity::remove();
}

void SlimeEntity::performSplit()
{
    // MC 1.16.5: SlimeEntity.remove() 中的分裂逻辑
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

    // 获取实体类型来创建新实例
    // MC 1.16.5: this.getType().create(this.world)
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* slimeType = registry.getType(entity::EntityTypes::SLIME);

    if (!slimeType || !slimeType->canSummon()) {
        spdlog::warn("SlimeEntity: Slime entity type not found or not summonable");
        return;
    }

    // MC 1.16.5: 计算分裂位置偏移
    // f = (float)i / 4.0F  (i = size)
    f32 offsetScale = static_cast<f32>(m_size) / 4.0f;

    // 保存当前实体的属性用于继承
    bool persistenceRequired = isNoDespawnRequired();
    bool invulnerable = isInvulnerable();
    std::string customName = customNameText(); // 获取自定义名称文本

    // 生成小史莱姆
    for (i32 l = 0; l < splitCount; ++l) {
        // MC 1.16.5: 计算每个小史莱姆的偏移位置
        // f1 = ((float)(l % 2) - 0.5F) * f
        // f2 = ((float)(l / 2) - 0.5F) * f
        f32 offsetX = (static_cast<f32>(l % 2) - 0.5f) * offsetScale;
        f32 offsetZ = (static_cast<f32>(l / 2) - 0.5f) * offsetScale;

        // 创建新史莱姆
        std::unique_ptr<Entity> entity = slimeType->create(world());
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

        // MC 1.16.5: 继承父实体的属性
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

        // MC 1.16.5: setLocationAndAngles
        f32 spawnX = static_cast<f32>(x()) + offsetX;
        f32 spawnY = static_cast<f32>(y()) + 0.5f;
        f32 spawnZ = static_cast<f32>(z()) + offsetZ;
        f32 spawnYaw = rng.nextFloat() * 360.0f;

        smallSlime->setPosition(spawnX, spawnY, spawnZ);
        smallSlime->setRotation(spawnYaw, 0.0f);

        // 设置随机速度
        smallSlime->setVelocity(
            (rng.nextFloat() - 0.5f) * 0.5f, rng.nextFloat() * 0.5f, (rng.nextFloat() - 0.5f) * 0.5f);

        // 设置类型 ID
        smallSlime->setTypeId(getTypeId());

        // 生成到世界中
        // 使用 unique_ptr 包装回实体
        std::unique_ptr<Entity> slimePtr(smallSlime);
        EntityId entityId = world()->spawnEntity(std::move(slimePtr));

        if (entityId == 0) {
            spdlog::debug("SlimeEntity: Failed to spawn small slime");
        }
    }
}

} // namespace mc
