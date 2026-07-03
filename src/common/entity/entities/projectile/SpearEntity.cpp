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
#include "../../core/EntityTypeIdNumber.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "ProjectileHelper.hpp"
#include "common/particle/ParticleTypes.hpp"

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

SpearEntity::SpearEntity(EntityId id)
    : AbstractArrowEntity(id)
{
    // 长矛投掷命中伤害与三叉戟一致（8.0）
    m_damage = 8.0f;
    setPickupStatus(PickupStatus::Allowed);
}

std::unique_ptr<Entity> SpearEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SpearEntity>(0);
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
        bool isPlayer = shooter->typeId() == entity::EntityTypeIdNumber::PLAYER;
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Spear, shooter, this, isPlayer);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Spear, this, this, false);
    }

    // 标记已造成伤害
    m_dealtDamage = true;

    // 应用伤害
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);
    if (livingTarget != nullptr) {
        livingTarget->hurt(*damageSource, damage);
    }

    // 击退效果
    if (m_knockbackStrength > 0) {
        f32 ratio = 0.6f * static_cast<f32>(m_knockbackStrength);
        Vector3 horizontalVel(m_velocity.x, 0.0f, m_velocity.z);
        if (horizontalVel.lengthSquared() > 0.0f) {
            horizontalVel = horizontalVel.normalized();
            Vector3 knockback(horizontalVel.x * ratio, 0.1f, horizontalVel.z * ratio);
            target->addVelocity(knockback);
        }
    }

    // 速度反转为轻微反弹（与三叉戟一致）
    m_velocity = Vector3(m_velocity.x * -0.01f, m_velocity.y * -0.1f, m_velocity.z * -0.01f);

    // 播放命中音效
    playSound(SoundEvents::ITEM_SPEAR_HIT, 1.0f, 1.0f);
}

void SpearEntity::onBlockHit(const RayTraceResult& result)
{
    m_inGround = true;

    // 保存方块状态
    if (m_world && result.type == RayTraceResultType::Block) {
        const BlockState* state = m_world->getBlockState(result.blockPos.x, result.blockPos.y, result.blockPos.z);
        if (state != nullptr) {
            m_inBlockState = *state;
        }
    }

    // 清除暴击和穿透状态
    m_critical = false;
    m_pierceLevel = 0;
    clearPiercedEntities();

    // 播放命中地面音效
    playSound(SoundEvents::ITEM_SPEAR_HIT_GROUND, 1.0f, 1.0f);
}

f32 SpearEntity::getWaterDrag() const
{
    // 长矛水中阻力与三叉戟一致（极小）
    return 0.99f;
}

void SpearEntity::setBaseDamageFromMob(f32 power)
{
    // 长矛不使用弓类附魔（力量/冲击/火焰），公式与三叉戟相同：
    // damage = power * 2.0 + triangle(difficulty * 0.11, 0.57425)
    math::Random rng = createRandomFromEntity(*this);
    f32 difficultyBonus = m_world ? static_cast<f32>(static_cast<u8>(m_world->difficulty())) * 0.11f : 0.0f;
    f32 triangle = difficultyBonus + (rng.nextFloat() - rng.nextFloat()) * 0.57425f;
    m_damage = power * 2.0f + triangle;
}

bool SpearEntity::onPlayerPickup(Player& player)
{
    // 必须在服务端执行
    if (m_world->isClientSide()) {
        return false;
    }

    // 只有当长矛在地上时才能被拾取
    if (!m_inGround) {
        return false;
    }

    // 长矛不能处于抖动状态
    if (m_arrowShake > 0) {
        return false;
    }

    // 检查拾取权限
    bool canPickup = (pickupStatus() == PickupStatus::Allowed) ||
        (pickupStatus() == PickupStatus::CreativeOnly && player.isCreative());

    if (!canPickup) {
        return false;
    }

    // 添加到玩家背包
    if (!m_spearStack.isEmpty()) {
        i32 added = player.inventory().add(m_spearStack);
        (void)added;
        if (m_spearStack.getCount() > 0) {
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

} // namespace entity
} // namespace mc
