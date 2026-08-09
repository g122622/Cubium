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

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/ProjectileHelper.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

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

AbstractArrowEntity::AbstractArrowEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileEntity(id, registry)
{}

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
        clearFire();
        // 水中生成气泡粒子尾迹
        if (m_world) {
            for (int j = 0; j < 4; ++j) {
                f32 offset = 0.25f;
                Vector3 pos(x() - m_builtIn.velocity->m_velocity.x * offset,
                    y() - m_builtIn.velocity->m_velocity.y * offset,
                    z() - m_builtIn.velocity->m_velocity.z * offset);
                m_world->addParticle(particle::ParticleTypeId::Bubble, pos, m_builtIn.velocity->m_velocity);
            }
        }
    }

    // ========== 检查是否在方块内 ==========
    BlockPos currentPos = BlockPos(static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.y)),
        static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.z)));
    if (m_world) {
        const BlockState* blockState = m_world->getBlockState(currentPos.x, currentPos.y, currentPos.z);
        // 检查是否在非空气方块的碰撞箱内
        if (blockState != nullptr && !blockState->isAir()) {
            // 获取方块的碰撞形状
            const CollisionShape& collisionShape = blockState->getCollisionShape();

            // 如果碰撞形状不为空，检查箭矢位置是否在碰撞箱内
            if (!collisionShape.isEmpty()) {
                // 获取世界坐标下的碰撞箱列表
                std::vector<AxisAlignedBB> worldBoxes =
                    collisionShape.getWorldBoxes(currentPos.x, currentPos.y, currentPos.z);

                // 检查箭矢位置是否在任意碰撞箱内
                for (const AxisAlignedBB& box : worldBoxes) {
                    if (box.contains(m_builtIn.stateVector->m_pos)) {
                        m_inGround = true;
                        m_inBlockState = *blockState;
                        break;
                    }
                }
            }
        }
    }

    // 调用父类tick进行射线追踪和移动
    ProjectileEntity::tick();

    // 暴击粒子效果
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

            m_world->addParticle(particle::ParticleTypeId::Crit, pos, vel);
        }
    }
}

void AbstractArrowEntity::tickInGround()
{
    // 检查方块是否仍然存在
    if (m_world) {
        const BlockState* currentBlock =
            m_world->getBlockState(static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.x)),
                static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.y)),
                static_cast<BlockCoord>(std::floor(m_builtIn.stateVector->m_pos.z)));

        // 检查方块变更导致箭矢脱落
        if (currentBlock != nullptr && m_inBlockState.has_value() && *currentBlock != *m_inBlockState &&
            checkInBlockEmpty()) {
            detachFromBlock();
            return;
        }
    }

    ++m_ticksInGround;
    ++m_timeInGround;

    // 超时移除（1200 ticks = 60秒）
    if (m_ticksInGround >= 1200) {
        remove();
    }
}

bool AbstractArrowEntity::checkInBlockEmpty()
{
    // 检查箭矢周围是否有碰撞箱
    // 创建一个很小的检测盒（0.06）
    AxisAlignedBB testBox(m_builtIn.stateVector->m_pos.x - 0.06f,
        m_builtIn.stateVector->m_pos.y - 0.06f,
        m_builtIn.stateVector->m_pos.z - 0.06f,
        m_builtIn.stateVector->m_pos.x + 0.06f,
        m_builtIn.stateVector->m_pos.y + 0.06f,
        m_builtIn.stateVector->m_pos.z + 0.06f);

    if (m_world) {
        return m_world->hasNoCollisions(testBox);
    }
    return true;
}

void AbstractArrowEntity::detachFromBlock()
{
    m_inGround = false;

    // 随机弹射
    math::Random rng = createRandomFromEntity(*this);
    f32 randX = rng.nextFloat() * 0.2f;
    f32 randY = rng.nextFloat() * 0.2f;
    f32 randZ = rng.nextFloat() * 0.2f;
    m_builtIn.velocity->m_velocity = Vector3(randX, randY, randZ);

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

    // 计算伤害
    f32 speed = std::sqrt(m_builtIn.velocity->m_velocity.x * m_builtIn.velocity->m_velocity.x +
        m_builtIn.velocity->m_velocity.y * m_builtIn.velocity->m_velocity.y +
        m_builtIn.velocity->m_velocity.z * m_builtIn.velocity->m_velocity.z);
    i32 damage = static_cast<i32>(std::clamp(static_cast<f64>(speed * m_damage), 0.0, 2147483647.0));

    // 暴击伤害加成
    if (m_critical) {
        mc::math::Random rng = createRandomFromEntity(*this);
        i32 bonus = rng.nextInt(damage / 2 + 2);
        damage = static_cast<i32>(std::min(static_cast<i64>(damage) + bonus, static_cast<i64>(2147483647)));
    }

    // 穿透检查
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
        bool isPlayer = shooter->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Arrow, shooter, this, isPlayer);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Arrow, this, this, false);
    }

    // 应用伤害并增加箭矢计数
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);
    if (livingTarget != nullptr) {
        bool hurt = livingTarget->hurt(*damageSource, static_cast<f32>(damage));
        // 只有非穿透箭在造成伤害后才增加箭矢计数
        if (hurt && m_pierceLevel <= 0) {
            livingTarget->setArrowCountInEntity(livingTarget->getArrowCount() + 1);
        }
    }

    // 击退效果
    if (m_knockbackStrength > 0) {
        f32 ratio = 0.6f * static_cast<f32>(m_knockbackStrength);
        Vector3 horizontalVel(m_builtIn.velocity->m_velocity.x, 0.0f, m_builtIn.velocity->m_velocity.z);
        if (horizontalVel.lengthSquared() > 0.0f) {
            horizontalVel = horizontalVel.normalized();
            Vector3 knockback(horizontalVel.x * ratio, 0.1f, horizontalVel.z * ratio);
            target->addVelocity(knockback);
        }
    }

    // 火焰伤害
    if (isOnFire()) {
        target->igniteForSeconds(5.0f);
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
    Vector3 hitOffset = hitVec - m_builtIn.stateVector->m_pos;
    m_builtIn.velocity->m_velocity = hitOffset;
    Vector3 normalizedOffset = hitOffset.normalized() * 0.05f;
    m_builtIn.stateVector->m_pos = m_builtIn.stateVector->m_pos - normalizedOffset;

    m_arrowShake = 7;

    // 清除暴击和穿透状态
    m_critical = false;
    m_pierceLevel = 0;
    clearPiercedEntities();

    // 播放命中地面音效
    math::Random rng = createRandomFromEntity(*this);
    playSound(SoundEvents::ENTITY_ARROW_HIT_GROUND, 1.0f, 1.2f / (rng.nextFloat() * 0.2f + 0.9f));
}

void AbstractArrowEntity::setBaseDamageFromMob(f32 power)
{
    // 公式：power * 2.0 + triangle(difficulty * 0.11, 0.57425)
    math::Random rng = createRandomFromEntity(*this);
    f32 difficultyBonus = m_world ? static_cast<f32>(static_cast<u8>(m_world->difficulty())) * 0.11f : 0.0f;
    f32 triangle = difficultyBonus + (rng.nextFloat() - rng.nextFloat()) * 0.57425f;
    m_damage = power * 2.0f + triangle;
}

void AbstractArrowEntity::applyBowEnchantments(LivingEntity& shooter)
{
    // 力量附魔增加伤害（PowerEnchantment: 每级 +0.5 伤害 + 基础 0.5）
    i32 powerLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        shooter.getMainHandItem(), &item::enchant::AllEnchantments::POWER);
    if (powerLevel > 0) {
        m_damage += static_cast<f32>(powerLevel) * 0.5f + 0.5f;
    }

    // 冲击附魔增加击退（PunchEnchantment: 每级增加 1 点击退强度）
    i32 punchLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        shooter.getMainHandItem(), &item::enchant::AllEnchantments::PUNCH);
    if (punchLevel > 0) {
        m_knockbackStrength = punchLevel;
    }

    // 火焰附魔：设置箭矢着火 100 ticks（5 秒），命中时点燃目标
    if (item::enchant::EnchantmentHelper::getEnchantmentLevel(
            shooter.getMainHandItem(), &item::enchant::AllEnchantments::FLAME) > 0) {
        igniteForTicks(100);
    }
}

void AbstractArrowEntity::onCollideWithPlayer(Player& player)
{
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
    if (m_world) {
        math::Random rng = createRandomFromEntity(*this);
        m_world->playSound(SoundEvents::ENTITY_ITEM_PICKUP,
            sound::SoundCategory::Players,
            m_builtIn.stateVector->m_pos,
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

ArrowEntity::ArrowEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractArrowEntity(id, registry)
{
    m_damage = 2.0f;
}

std::unique_ptr<Entity> ArrowEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ArrowEntity>(0, registry);
}

std::unique_ptr<ArrowEntity> ArrowEntity::createFromShooter(LivingEntity& shooter, IWorld* world)
{
    // ECS 迁移：实体构造需要 registry 句柄，静态方法无 this，从 IWorld* 参数取；
    // ClientWorld 返回 nullptr 表客户端不接入 ECS，此时无法构造箭矢
    auto* registry = world->entityRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    auto arrow = std::make_unique<ArrowEntity>(0, *registry);
    arrow->setTypeId(EntityTypeKeys::ARROW);
    arrow->setWorld(world);
    arrow->setPosition(shooter.x(), shooter.y() + shooter.eyeHeight() - 0.1f, shooter.z());
    arrow->setShooter(&shooter);

    // 玩家射出的箭默认允许拾取，非玩家射出的箭默认不允许拾取
    Player* player = dynamic_cast<Player*>(&shooter);
    if (player != nullptr) {
        arrow->setPickupStatus(PickupStatus::Allowed);
    }

    return arrow;
}

void ArrowEntity::tick()
{
    AbstractArrowEntity::tick();

    // 药水箭的粒子效果处理
    if (m_color != 0xFFFFFFFF && !m_inGround && m_world && m_world->isClientSide()) {
        // 将 ARGB 颜色转换为 RGB 分量 (0.0-1.0 范围)
        // 使用 EntityEffect 粒子，速度参数作为颜色传递
        f32 r = static_cast<f32>((m_color >> 16) & 0xFF) / 255.0f;
        f32 g = static_cast<f32>((m_color >> 8) & 0xFF) / 255.0f;
        f32 b = static_cast<f32>(m_color & 0xFF) / 255.0f;

        // 飞行中每 tick 生成 2 个粒子
        math::Random rng = createRandomFromEntity(*this);
        for (int i = 0; i < 2; ++i) {
            // 粒子位置在箭矢周围随机偏移
            f32 ox = (rng.nextFloat() - 0.5f) * width();
            f32 oy = rng.nextFloat() * height();
            f32 oz = (rng.nextFloat() - 0.5f) * width();

            Vector3 pos(x() + ox, y() + oy, z() + oz);
            Vector3 colorVel(r, g, b); // 颜色作为速度参数传递

            m_world->addParticle(particle::ParticleTypeId::EntityEffect, pos, colorVel);
        }
    }
}

void ArrowEntity::onEntityHit(const RayTraceResult& result)
{
    // 先调用父类处理伤害
    AbstractArrowEntity::onEntityHit(result);

    // 应用药水效果到被命中的生物
    if (!result.hitEntity || m_effects.empty()) {
        return;
    }

    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (livingTarget != nullptr && livingTarget->isAlive()) {
        // 对目标施加所有药水效果
        for (const auto& effect : m_effects) {
            livingTarget->addEffect(effect);
        }
    }
}

ItemStack ArrowEntity::getArrowStack() const
{
    // 如果有药水效果，返回药水箭；否则返回普通箭矢
    if (hasEffects()) {
        // 创建药水箭物品堆
        ItemStack tippedArrow(*Items::TIPPED_ARROW, 1);

        // 设置药水效果到物品堆的 NBT 标签
        // 注意：ArrowEntity 没有存储 Potion 类型，只有效果列表
        // 所以只设置自定义效果和颜色
        potion::PotionUtils::setCustomEffects(tippedArrow, m_effects);

        // 设置自定义颜色（如果有）
        if (m_color != 0xFFFFFFFF) {
            potion::PotionUtils::setCustomPotionColor(tippedArrow, m_color);
        }

        return tippedArrow;
    } else {
        // 返回普通箭矢
        return ItemStack(*Items::ARROW, 1);
    }
}

// ============================================================================
// SpectralArrowEntity
// ============================================================================

SpectralArrowEntity::SpectralArrowEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractArrowEntity(id, registry)
{
    m_damage = 2.0f;
}

std::unique_ptr<Entity> SpectralArrowEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SpectralArrowEntity>(0, registry);
}

void SpectralArrowEntity::tick()
{
    AbstractArrowEntity::tick();

    // 光灵箭粒子效果 - 仅客户端执行
    if (!m_inGround && m_world && m_world->isClientSide()) {
        // 使用 INSTANT_EFFECT 粒子
        // 粒子位置：箭矢当前位置，速度为零
        m_world->addParticle(particle::ParticleTypeId::InstantSpell, Vector3(x(), y(), z()), Vector3(0.0f, 0.0f, 0.0f));
    }
}

void SpectralArrowEntity::onEntityHit(const RayTraceResult& result)
{
    // 先调用父类处理伤害
    AbstractArrowEntity::onEntityHit(result);

    // 命中生物时施加发光效果
    if (!result.hitEntity) {
        return;
    }

    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (livingTarget != nullptr) {
        // 施加发光效果，持续时间 m_glowDuration ticks（默认 200 ticks = 10 秒）
        livingTarget->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Glowing, m_glowDuration, 0));
    }
}

ItemStack SpectralArrowEntity::getArrowStack() const
{
    // 光灵箭总是返回光灵箭物品
    return ItemStack(*Items::SPECTRAL_ARROW, 1);
}

} // namespace entity
} // namespace mc
