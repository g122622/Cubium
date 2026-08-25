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

#include "SpearEntity.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../registry/VanillaEntityTypeKeys.hpp"
#include "../../serialization/EntityNbtKeys.hpp"
#include "../../serialization/NbtHelper.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "ProjectileHelper.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/ProjectileArrowStateComponent.hpp"
#include "common/entity/ecs/components/ProjectileItemComponent.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <utility>

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

SpearEntity::SpearEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractArrowEntity(id, registry)
{
    // 长矛投掷命中伤害与三叉戟一致（8.0）
    setDamage(8.0f);
    setPickupStatus(PickupStatus::Allowed);
    // 批次6 子目标2 Step4：attach ProjectileItemComponent 承载 m_spearStack。
    m_entityContext->enttRegistry().emplace<ecs::ProjectileItemComponent>(m_entityContext->entity());
}

std::unique_ptr<Entity> SpearEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SpearEntity>(0, registry);
}

void SpearEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    Entity* target = result.hitEntity;

    // 长矛投掷伤害固定 8.0（不随层级变化）
    f32 damage = 8.0f;

    // 获取射击者
    Entity* shooter = getShooter();

    // 创建伤害来源：投掷长矛伤害（使用 DamageType::Spear 类型）
    std::unique_ptr<DamageSource> damageSource;
    if (shooter != nullptr) {
        bool isPlayer = shooter->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Spear, shooter, this, isPlayer);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Spear, this, this, false);
    }

    // 标记已造成伤害
    setDealtDamage(true);

    // 应用伤害
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);

    // 攻击者记录"我打了谁"（对齐 vanilla AbstractArrow.onHitEntity:444 在 hurt 前对 shooter
    // (LivingEntity) 调 setLastHurtMob(entity) 的语义；Spear 是 Cubium 特有投掷武器，按投射物
    // 命中语义对齐 AbstractArrow）。字段由 OwnerHurtTargetGoal 消费（驯服动物帮主人攻击主人正在打的怪）。
    // 本 override 自管不调基类 onEntityHit，须显式补。
    if (shooter != nullptr) {
        LivingEntity* shooterLiving = dynamic_cast<LivingEntity*>(shooter);
        if (shooterLiving != nullptr && livingTarget != nullptr) {
            shooterLiving->setLastHurtTarget(livingTarget);
        }
    }

    if (livingTarget != nullptr) {
        livingTarget->hurt(*damageSource, damage);
    }

    // 击退效果
    if (knockbackStrength() > 0) {
        f32 ratio = 0.6f * static_cast<f32>(knockbackStrength());
        Vector3 horizontalVel(m_builtIn.velocity->m_velocity.x, 0.0f, m_builtIn.velocity->m_velocity.z);
        if (horizontalVel.lengthSquared() > 0.0f) {
            horizontalVel = horizontalVel.normalized();
            Vector3 knockback(horizontalVel.x * ratio, 0.1f, horizontalVel.z * ratio);
            target->addVelocity(knockback);
        }
    }

    // 速度反转为轻微反弹（与三叉戟一致）
    m_builtIn.velocity->m_velocity = Vector3(m_builtIn.velocity->m_velocity.x * -0.01f,
        m_builtIn.velocity->m_velocity.y * -0.1f,
        m_builtIn.velocity->m_velocity.z * -0.01f);

    // 播放命中音效
    playSound(SoundEvents::ITEM_SPEAR_HIT, 1.0f, 1.0f);
}

void SpearEntity::onBlockHit(const RayTraceResult& result)
{
    setInGround(true);

    auto* arrowState = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    // 保存方块状态
    if (arrowState != nullptr && m_world && result.type == RayTraceResultType::Block) {
        const BlockState* state = m_world->getBlockState(result.blockPos.x, result.blockPos.y, result.blockPos.z);
        if (state != nullptr) {
            *arrowState->m_inBlockState = *state;
        }
    }

    // 清除暴击和穿透状态
    setCritical(false);
    setPierceLevel(0);
    clearPiercedEntities();

    // 播放命中地面音效
    playSound(SoundEvents::ITEM_SPEAR_HIT_GROUND, 1.0f, 1.0f);
}

f32 SpearEntity::getWaterDrag() const
{
    // 长矛水中阻力与三叉戟一致（极小）
    return 0.99f;
}

// 批次6 子目标2 Step4：m_spearStack 迁入 ecs::ProjectileItemComponent。
ItemStack SpearEntity::getItemStack() const
{
    const auto* c = tryGetComponent<ecs::ProjectileItemComponent>();
    return (c != nullptr && c->m_itemStack != nullptr) ? *c->m_itemStack : ItemStack();
}

ItemStack SpearEntity::getArrowStack() const
{
    const auto* c = tryGetComponent<ecs::ProjectileItemComponent>();
    return (c != nullptr && c->m_itemStack != nullptr) ? c->m_itemStack->copy() : ItemStack();
}

void SpearEntity::setItemStack(const ItemStack& stack)
{
    auto* c = tryGetComponent<ecs::ProjectileItemComponent>();
    if (c != nullptr && c->m_itemStack != nullptr) {
        *c->m_itemStack = stack;
    }
}

void SpearEntity::setBaseDamageFromMob(f32 power)
{
    // 长矛不使用弓类附魔（力量/冲击/火焰），公式与三叉戟相同：
    // damage = power * 2.0 + triangle(difficulty * 0.11, 0.57425)
    math::Random rng = createRandomFromEntity(*this);
    f32 difficultyBonus = m_world ? static_cast<f32>(static_cast<u8>(m_world->difficulty())) * 0.11f : 0.0f;
    f32 triangle = difficultyBonus + (rng.nextFloat() - rng.nextFloat()) * 0.57425f;
    setDamage(power * 2.0f + triangle);
}

bool SpearEntity::onPlayerPickup(Player& player)
{
    // 必须在服务端执行
    if (m_world->isClientSide()) {
        return false;
    }

    // 只有当长矛在地上时才能被拾取
    if (!isInGround()) {
        return false;
    }

    // 长矛不能处于抖动状态
    if (arrowShake() > 0) {
        return false;
    }

    // 检查拾取权限
    bool canPickup = (pickupStatus() == PickupStatus::Allowed) ||
        (pickupStatus() == PickupStatus::CreativeOnly && player.isCreative());

    if (!canPickup) {
        return false;
    }

    // 添加到玩家背包
    auto* itemComp = tryGetComponent<ecs::ProjectileItemComponent>();
    const bool hasStack =
        (itemComp != nullptr && itemComp->m_itemStack != nullptr && !itemComp->m_itemStack->isEmpty());
    if (hasStack) {
        i32 added = player.inventory().add(*itemComp->m_itemStack);
        (void)added;
        if (itemComp->m_itemStack->getCount() > 0) {
            return false; // 背包满了
        }
    }

    // 播放拾取音效
    if (m_world) {
        playSound(SoundEvents::ENTITY_ITEM_PICKUP, 0.2f, 1.0f);
    }

    remove();
    return true;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void SpearEntity::addAdditionalSaveData(nbt::tags::compound_tag& /*tag*/) const
{
    // 批次6 子目标2 Step6：Spear 持久化（item + arrow 8 字段 + dealtDamage）已搬至按组件注册的
    // 序列化器（ProjectileComponentSerialization.cpp 的 saveProjectileItem/loadProjectileItem +
    // saveArrowState/loadArrowState），经 ComponentSerializerRegistry::saveAll/loadAll 调用。
    // 此 override 保留空壳。
}

Result<void> SpearEntity::readAdditionalSaveData(const nbt::tags::compound_tag& /*tag*/)
{
    // 持久化已搬注册表，此 override 空实现。
    return {};
}

} // namespace entity
} // namespace mc
