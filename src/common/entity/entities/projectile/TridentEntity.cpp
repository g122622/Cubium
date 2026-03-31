#include "TridentEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../item/enchantment/EnchantmentHelper.hpp"
#include <cmath>

namespace mc {
namespace entity {

TridentEntity::TridentEntity(LegacyEntityType type, EntityId id)
    : AbstractArrowEntity(type, id)
{
    m_damage = 8.0f;  // 三叉戟伤害更高
    setPickupStatus(PickupStatus::Allowed);
}

std::unique_ptr<Entity> TridentEntity::create(IWorld* /*world*/) {
    return std::make_unique<TridentEntity>(LegacyEntityType::Unknown, 0);
}

void TridentEntity::tick() {
    // 如果在返回中，执行返回逻辑
    if (m_returning) {
        tickReturning();
        return;
    }

    // 调用父类tick
    AbstractArrowEntity::tick();

    // 如果已击中方块且没有忠诚附魔，不需要额外处理
    if (m_inGround) {
        // 检查忠诚附魔
        if (m_loyaltyLevel > 0) {
            // 忠诚附魔的三叉戟会返回
            m_returning = true;
        }
    }
}

void TridentEntity::tickReturning() {
    mc::Entity* shooter = getShooter();
    if (!shooter || !shooter->isAlive()) {
        // 射手已死亡或不存在，移除三叉戟
        remove();
        return;
    }

    // 朝向射手飞行
    Vector3 direction(
        shooter->x() - m_position.x,
        shooter->y() + shooter->eyeHeight() - m_position.y,
        shooter->z() - m_position.z
    );

    f32 distance = direction.length();
    if (distance < 2.0f) {
        // 到达射手，添加到背包
        // 如果是玩家射出的
        Player* player = dynamic_cast<Player*>(shooter);
        if (player) {
            onPlayerPickup(*player);
        } else {
            // 非玩家射手，直接移除
            remove();
        }
        return;
    }

    // 归一化并设置速度
    direction = direction.normalized();
    f32 speed = 1.5f + static_cast<f32>(m_loyaltyLevel) * 0.5f;
    m_velocity = direction * speed;

    // 更新位置
    m_prevPosition = m_position;
    m_position = m_position + m_velocity;

    // 更新旋转
    updateRotation();

    // 检查是否在水中
    if (isInWater()) {
        for (int i = 0; i < 4; ++i) {
            // TODO: 生成气泡粒子
        }
    }
}

void TridentEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 获取射击者
    mc::Entity* shooter = getShooter();

    // 计算伤害
    f32 damage = m_damage;

    // 如果已经造成过伤害，减少后续伤害
    if (m_dealtDamage > 0.0f) {
        damage *= 0.5f;
    }

    // 创建伤害来源
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Trident, shooter, this, shooter != nullptr);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::Trident, this, this, false);
    }

    // TODO: 应用伤害
    // bool hurt = result.hitEntity->attackEntityFrom(*damageSource, damage);

    // 击退效果
    if (m_knockbackStrength > 0) {
        f32 ratio = 0.6f * m_knockbackStrength;
        Vector3 knockback(m_velocity.x * ratio, 0.1f, m_velocity.z * ratio);
        if (knockback.length() > 0.0f) {
            result.hitEntity->addVelocity(knockback);
        }
    }

    m_dealtDamage += damage;

    // 雷击附魔
    // TODO: 检查激流附魔，如果在雨中或水中召唤闪电

    // 如果没有穿透，移除
    if (m_pierceLevel <= 0) {
        // 三叉戟不移除，而是返回
        if (m_loyaltyLevel > 0) {
            m_returning = true;
        }
    }
}

void TridentEntity::onBlockHit(const RayTraceResult& result) {
    m_inGround = true;
    m_hitBlock = true;
    m_hitBlockPos = result.blockPos;

    // 清除暴击和穿透状态
    m_critical = false;
    m_pierceLevel = 0;
    m_piercedEntities.clear();

    // 播放命中音效
    // playSound(SoundEvents.ENTITY_TRIDENT_HIT_GROUND, 1.0F, 1.0F);
}

} // namespace entity
} // namespace mc
