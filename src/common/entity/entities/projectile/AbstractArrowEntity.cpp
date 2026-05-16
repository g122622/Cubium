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

#include "AbstractArrowEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../inventory/PlayerInventory.hpp"
#include "ProjectileHelper.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace entity {

namespace {

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity)
{
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

// ============================================================================
// AbstractArrowEntity
// ============================================================================

AbstractArrowEntity::AbstractArrowEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
    m_noGravity = false;
}

void AbstractArrowEntity::tick()
{
    // 检查是否已离开发射者
    if (!m_leftShooter) {
        m_leftShooter = checkLeftShooter();
    }

    // 如果插在方块中，执行不同的tick逻辑
    if (m_inGround) {
        tickInGround();
        return;
    }

    // 检查抖动
    if (m_arrowShake > 0) {
        --m_arrowShake;
    }

    // 如果在水中，灭火并生成气泡粒子
    if (isInWater()) {
        setFire(0);
        // MC 1.16.5 AbstractArrowEntity.tick() 第239-244行
        // 水中生成气泡粒子尾迹
        if (m_world) {
            for (int j = 0; j < 4; ++j) {
                f32 offset = 0.25f;
                Vector3 pos(x() - m_velocity.x * offset, y() - m_velocity.y * offset, z() - m_velocity.z * offset);
                m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Bubble, pos, m_velocity);
            }
        }
    }

    // ========== MC 1.16.5: 检查是否在方块内 ==========
    // 参考 AbstractArrowEntity.tick() 第146-160行
    BlockPos currentPos = BlockPos(static_cast<BlockCoord>(std::floor(m_position.x)),
        static_cast<BlockCoord>(std::floor(m_position.y)),
        static_cast<BlockCoord>(std::floor(m_position.z)));
    if (m_world) {
        const BlockState* blockState = m_world->getBlockState(currentPos.x, currentPos.y, currentPos.z);
        // 检查是否在非空气方块的碰撞箱内
        // TODO: 需要实现 VoxelShape 检查
        // 当前简化处理：如果方块不透明且不在水中，认为在方块内
        if (blockState != nullptr && !blockState->isAir() && blockState->isSolid()) {
            m_inGround = true;
            m_inBlockState = *blockState;
        }
    }

    // 调用父类tick进行射线追踪和移动
    ProjectileEntity::tick();

    // MC 1.16.5: 暴击粒子效果
    if (m_critical && !m_inGround && m_world) {
        math::Random rng = createRandomFromEntity(*this);
        // 每tick有概率生成暴击粒子
        if (rng.nextInt(3) == 0) {
            f32 ox = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;
            f32 oy = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;
            f32 oz = (rng.nextFloat() * 2.0f - 1.0f) * 0.3f;

            Vector3 pos(x() + ox, y() + oy, z() + oz);
            // 粒子速度与箭矢速度相反
            Vector3 vel(-velocityX() * 0.01f, -velocityY() * 0.01f, -velocityZ() * 0.01f);

            m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Crit, pos, vel);
        }
    }
}

void AbstractArrowEntity::tickInGround()
{
    // 检查方块是否仍然存在
    if (m_world) {
        const BlockState* currentBlock = m_world->getBlockState(static_cast<BlockCoord>(std::floor(m_position.x)),
            static_cast<BlockCoord>(std::floor(m_position.y)),
            static_cast<BlockCoord>(std::floor(m_position.z)));

        // 参考 MC 1.16.5: 检查方块变更导致箭矢脱落
        if (currentBlock != nullptr && m_inBlockState.has_value() && *currentBlock != *m_inBlockState &&
            checkInBlockEmpty()) {
            detachFromBlock();
            return;
        }
    }

    ++m_ticksInGround;
    ++m_timeInGround;

    // 超时移除（MC 1.16.5: 1200 ticks = 60秒）
    if (m_ticksInGround >= 1200) {
        remove();
    }
}

bool AbstractArrowEntity::checkInBlockEmpty()
{
    // 参考 MC 1.16.5 AbstractArrowEntity.func_234593_u_()
    // 检查箭矢周围是否有碰撞箱
    // 创建一个很小的检测盒（0.06）
    AxisAlignedBB testBox(m_position.x - 0.06f,
        m_position.y - 0.06f,
        m_position.z - 0.06f,
        m_position.x + 0.06f,
        m_position.y + 0.06f,
        m_position.z + 0.06f);

    if (m_world) {
        return m_world->hasNoCollisions(testBox);
    }
    return true;
}

void AbstractArrowEntity::detachFromBlock()
{
    // 参考 MC 1.16.5 AbstractArrowEntity.func_234594_z_()
    m_inGround = false;

    // 随机弹射
    math::Random rng = createRandomFromEntity(*this);
    f32 randX = rng.nextFloat() * 0.2f;
    f32 randY = rng.nextFloat() * 0.2f;
    f32 randZ = rng.nextFloat() * 0.2f;
    m_velocity = Vector3(randX, randY, randZ);

    m_ticksInGround = 0;
    m_timeInGround = 0;
}

bool AbstractArrowEntity::shouldDespawn()
{
    // 由 tickInGround 处理
    return false;
}

void AbstractArrowEntity::clearPiercedEntities()
{
    m_piercedEntities.clear();
}

RayTraceResult AbstractArrowEntity::rayTraceEntities(const Vector3& start, const Vector3& end)
{
    // 使用父类实现，但使用穿透过滤
    if (m_world == nullptr) {
        return RayTraceResult::miss();
    }

    const AxisAlignedBB searchBox = ProjectileHelper::createMovementSearchBox(*this, end - start, 1.0f);

    return ProjectileHelper::rayTraceEntities(*m_world, *this, start, end, searchBox, [this](const Entity& candidate) {
        return canHitEntityWithPierce(candidate);
    });
}

bool AbstractArrowEntity::canHitEntityWithPierce(const mc::Entity& target) const
{
    // 基础检查
    if (!canHitEntity(target)) {
        return false;
    }

    // 检查是否已穿透过此实体
    if (m_pierceLevel > 0 && m_piercedEntities.count(target.id()) > 0) {
        return false;
    }

    return true;
}

void AbstractArrowEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    mc::Entity* target = result.hitEntity;

    // 计算伤害 - 参考 MC 1.16.5 AbstractArrowEntity.onEntityHit() 第303-304行
    f32 speed = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.y * m_velocity.y + m_velocity.z * m_velocity.z);
    i32 damage = static_cast<i32>(std::clamp(static_cast<f64>(speed * m_damage), 0.0, 2147483647.0));

    // 暴击伤害加成 - MC 1.16.5: i += rand.nextInt(i / 2 + 2)
    if (m_critical) {
        mc::math::Random rng = createRandomFromEntity(*this);
        i32 bonus = rng.nextInt(damage / 2 + 2);
        damage = static_cast<i32>(std::min(static_cast<i64>(damage) + bonus, 2147483647LL));
    }

    // 穿透检查 - 参考 MC 1.16.5 第305-320行
    if (m_pierceLevel > 0) {
        if (static_cast<i32>(m_piercedEntities.size()) >= m_pierceLevel + 1) {
            // 达到穿透上限，移除箭矢
            remove();
            return;
        }
        m_piercedEntities.insert(target->id());
    }

    // 获取发射者
    mc::Entity* shooter = getShooter();

    // 创建伤害来源
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource =
            std::make_unique<IndirectEntityDamageSource>(DamageType::Arrow, shooter, this, shooter != nullptr);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Arrow, this, this, false);
    }

    // 应用伤害并增加箭矢计数
    // 参考 MC 1.16.5 AbstractArrowEntity.onEntityHit() 第351-352行
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);
    if (livingTarget != nullptr) {
        bool hurt = livingTarget->hurt(*damageSource, static_cast<f32>(damage));
        // 只有非穿透箭在造成伤害后才增加箭矢计数
        if (hurt && m_pierceLevel <= 0) {
            livingTarget->setArrowCountInEntity(livingTarget->getArrowCount() + 1);
        }
    }

    // 击退效果 - 参考 MC 1.16.5 第355-360行
    if (m_knockbackStrength > 0) {
        f32 ratio = 0.6f * static_cast<f32>(m_knockbackStrength);
        Vector3 horizontalVel(m_velocity.x, 0.0f, m_velocity.z);
        if (horizontalVel.lengthSquared() > 0.0f) {
            horizontalVel = horizontalVel.normalized();
            Vector3 knockback(horizontalVel.x * ratio, 0.1f, horizontalVel.z * ratio);
            target->addVelocity(knockback);
        }
    }

    // 火焰伤害
    if (isOnFire()) {
        target->setFire(5);
    }

    // 播放命中音效
    math::Random rng = createRandomFromEntity(*this);
    playSound(SoundEvents::ENTITY_ARROW_HIT, 1.0f, 1.2f / (rng.nextFloat() * 0.2f + 0.9f));

    // 如果不是穿透箭，移除
    if (m_pierceLevel <= 0) {
        remove();
    }
}

void AbstractArrowEntity::onBlockHit(const RayTraceResult& result)
{
    // 参考 MC 1.16.5 AbstractArrowEntity.func_230299_a_() 第406-421行
    m_inGround = true;

    // 保存命中的方块状态
    if (m_world && result.type == RayTraceResultType::Block) {
        const BlockState* state = m_world->getBlockState(result.blockPos.x, result.blockPos.y, result.blockPos.z);
        if (state != nullptr) {
            m_inBlockState = *state;
        }
    }

    // 计算并设置箭矢位置（回退一点使其嵌入方块）
    Vector3 hitVec = result.hitPosition;
    Vector3 hitOffset = hitVec - m_position;
    m_velocity = hitOffset;
    Vector3 normalizedOffset = hitOffset.normalized() * 0.05f;
    m_position = m_position - normalizedOffset;

    m_arrowShake = 7;

    // 清除暴击和穿透状态
    m_critical = false;
    m_pierceLevel = 0;
    clearPiercedEntities();

    // 播放命中地面音效
    math::Random rng = createRandomFromEntity(*this);
    playSound(SoundEvents::ENTITY_ARROW_HIT_GROUND, 1.0f, 1.2f / (rng.nextFloat() * 0.2f + 0.9f));
}

void AbstractArrowEntity::setEnchantmentEffectsFrom(LivingEntity& shooter, f32 baseVelocity)
{
    // 参考 MC 1.16.5 AbstractArrowEntity.setEnchantmentEffectsFromEntity() 第596-612行

    // 设置基础伤害
    math::Random rng = createRandomFromEntity(*this);
    f32 difficultyBonus = m_world ? static_cast<f32>(static_cast<u8>(m_world->difficulty())) * 0.11f : 0.0f;
    m_damage = static_cast<f32>(baseVelocity * 2.0 + rng.nextGaussian() * 0.25 + difficultyBonus);

    // 力量附魔增加伤害
    // i32 power = EnchantmentHelper::getEnchantmentLevel(shooter.getMainHandItem(), "minecraft:power");
    // if (power > 0) {
    //     m_damage += power * 0.5 + 0.5;
    // }

    // 冲击附魔增加击退
    // i32 punch = EnchantmentHelper::getEnchantmentLevel(shooter.getMainHandItem(), "minecraft:punch");
    // if (punch > 0) {
    //     m_knockbackStrength = punch;
    // }

    // 火焰附魔
    // if (EnchantmentHelper::hasEnchantment(shooter.getMainHandItem(), "minecraft:flame")) {
    //     setFire(100);
    // }

    (void)shooter; // 暂时未使用
}

void AbstractArrowEntity::onCollideWithPlayer(Player& player)
{
    // 参考 MC 1.16.5 AbstractArrowEntity.onCollideWithPlayer() 第508-521行
    // 只在服务端执行，检查拾取条件
    if (m_world && m_world->isClientSide()) {
        return;
    }

    // 检查是否可以拾取：必须插在方块中或处于穿甲状态，且不在抖动
    if ((!m_inGround && !m_noClip) || m_arrowShake > 0) {
        return;
    }

    // 调用拾取逻辑
    onPlayerPickup(player);
}

bool AbstractArrowEntity::onPlayerPickup(Player& player)
{
    // 参考 MC 1.16.5 AbstractArrowEntity.onCollideWithPlayer() 第508-521行

    // 必须在服务端执行
    if (m_world && m_world->isClientSide()) {
        return false;
    }

    // 必须插在方块中或者是穿甲箭（noClip 状态）
    if (!m_inGround && !m_noClip) {
        return false;
    }

    // 箭矢不能处于抖动状态
    if (m_arrowShake > 0) {
        return false;
    }

    // 检查拾取权限
    bool canPickup = false;
    if (m_pickupStatus == PickupStatus::Allowed) {
        canPickup = true;
    } else if (m_pickupStatus == PickupStatus::CreativeOnly && player.isCreative()) {
        canPickup = true;
    } else if (m_noClip && getShooter() != nullptr && getShooter()->uuid() == player.uuid()) {
        // 穿甲箭且是自己射出的（忠诚附魔返回的三叉戟）
        canPickup = true;
    }

    if (!canPickup) {
        return false;
    }

    // 只有 Allowed 状态才检查背包空间
    if (m_pickupStatus == PickupStatus::Allowed) {
        // 获取箭矢物品堆
        ItemStack arrowStack = getArrowStack();

        // 尝试添加到玩家背包
        // add() 方法会修改 arrowStack，减少其数量
        player.inventory().add(arrowStack);

        // 如果背包满了，添加失败
        if (arrowStack.getCount() > 0) {
            return false;
        }
    }

    // 播放拾取音效
    // 参考 MC 1.16.5 AbstractArrowEntity.onCollideWithPlayer() 第517行
    // entityIn.onItemPickup(this, 1); 会播放音效
    if (m_world) {
        math::Random rng = createRandomFromEntity(*this);
        m_world->playSound(SoundEvents::ENTITY_ITEM_PICKUP,
            sound::SoundCategory::Players,
            m_position,
            0.2f,                                  // 音量
            1.0f + (rng.nextFloat() - 0.5f) * 0.2f // 音调带随机变化
        );
    }

    // 移除箭矢实体
    remove();
    return true;
}

// ============================================================================
// ArrowEntity
// ============================================================================

ArrowEntity::ArrowEntity(LegacyEntityType type, EntityId id)
    : AbstractArrowEntity(type, id)
{
    m_damage = 2.0f;
}

std::unique_ptr<Entity> ArrowEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 0);
}

std::unique_ptr<ArrowEntity> ArrowEntity::createFromShooter(LivingEntity& shooter, IWorld* world)
{
    auto arrow = std::make_unique<ArrowEntity>(LegacyEntityType::Unknown, 0);
    arrow->setWorld(world);
    arrow->setPosition(shooter.x(), shooter.y() + shooter.eyeHeight() - 0.1f, shooter.z());
    arrow->setShooter(&shooter);

    // 玩家射出的箭可以被拾取
    // if (shooter.isPlayer()) {
    //     arrow->setPickupStatus(PickupStatus::Allowed);
    // }

    return arrow;
}

void ArrowEntity::tick()
{
    AbstractArrowEntity::tick();

    // 药水箭的效果处理
    if (m_color != 0xFFFFFFFF && !m_inGround) {
        // TODO: 生成彩色粒子
    }
}

ItemStack ArrowEntity::getArrowStack() const
{
    // 参考 MC 1.16.5 ArrowEntity.getArrowStack() 第195-208行
    // 如果有药水效果，返回药水箭；否则返回普通箭矢
    if (hasEffects()) {
        // 创建药水箭物品堆
        ItemStack tippedArrow(*Items::TIPPED_ARROW, 1);
        // TODO: 设置药水效果到物品堆的 NBT 标签
        // PotionUtils.addPotionToItemStack(itemstack, this.potion);
        // PotionUtils.appendEffects(itemstack, this.customPotionEffects);
        return tippedArrow;
    } else {
        // 返回普通箭矢
        return ItemStack(*Items::ARROW, 1);
    }
}

// ============================================================================
// SpectralArrowEntity
// ============================================================================

SpectralArrowEntity::SpectralArrowEntity(LegacyEntityType type, EntityId id)
    : AbstractArrowEntity(type, id)
{
    m_damage = 2.0f;
}

std::unique_ptr<Entity> SpectralArrowEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SpectralArrowEntity>(LegacyEntityType::Unknown, 0);
}

void SpectralArrowEntity::tick()
{
    AbstractArrowEntity::tick();

    // MC 1.16.5 SpectralArrowEntity.tick() 第31-36行
    // 光灵箭粒子效果 - 仅客户端执行
    if (!m_inGround && m_world && m_world->isClientSide()) {
        // 使用 INSTANT_EFFECT 粒子（对应 ParticleTypes.INSTANT_EFFECT）
        // 粒子位置：箭矢当前位置，速度为零
        m_world->addParticle(
            client::renderer::trident::particle::ParticleTypeId::InstantSpell,
            Vector3(x(), y(), z()),
            Vector3(0.0f, 0.0f, 0.0f));
    }
}

void SpectralArrowEntity::onEntityHit(const RayTraceResult& result)
{
    // 先调用父类处理伤害
    AbstractArrowEntity::onEntityHit(result);

    // MC 1.16.5 SpectralArrowEntity.arrowHit() 第43-46行
    // 命中生物时施加发光效果
    if (!result.hitEntity) {
        return;
    }

    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (livingTarget != nullptr) {
        // 施加发光效果，持续时间 m_glowDuration ticks（默认 200 ticks = 10 秒）
        livingTarget->addEffect(entity::effect::EffectInstance(
            entity::effect::EffectType::Glowing,
            m_glowDuration,
            0));
    }
}

ItemStack SpectralArrowEntity::getArrowStack() const
{
    // 参考 MC 1.16.5 SpectralArrowEntity.getArrowStack() 第39-41行
    // 光灵箭总是返回光灵箭物品
    return ItemStack(*Items::SPECTRAL_ARROW, 1);
}

} // namespace entity
} // namespace mc
