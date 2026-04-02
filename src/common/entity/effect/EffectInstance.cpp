#include "EffectInstance.hpp"
#include "EffectAttributeModifiers.hpp"
#include "../core/LivingEntity.hpp"

namespace mc {
namespace entity {
namespace effect {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 不祥之兆持续时间（约100分钟）
constexpr i32 BAD_OMEN_DURATION = 120000;

/// 村庄英雄持续时间（约40分钟）
constexpr i32 HERO_DURATION = 48000;

} // namespace

// ============================================================================
// EffectInstance 实现
// ============================================================================

EffectInstance::EffectInstance(
    EffectType type,
    i32 duration,
    i32 amplifier,
    bool ambient,
    bool visible,
    bool showIcon
)
    : m_type(type)
    , m_duration(duration)
    , m_amplifier(amplifier)
    , m_ambient(ambient)
    , m_visible(visible)
    , m_showIcon(showIcon)
{
}

EffectInstance::EffectInstance(const EffectInstance& other)
    : m_type(other.m_type)
    , m_duration(other.m_duration)
    , m_amplifier(other.m_amplifier)
    , m_ambient(other.m_ambient)
    , m_visible(other.m_visible)
    , m_showIcon(other.m_showIcon)
    , m_applied(other.m_applied)
{
}

EffectInstance& EffectInstance::operator=(const EffectInstance& other) {
    if (this != &other) {
        m_type = other.m_type;
        m_duration = other.m_duration;
        m_amplifier = other.m_amplifier;
        m_ambient = other.m_ambient;
        m_visible = other.m_visible;
        m_showIcon = other.m_showIcon;
        m_applied = other.m_applied;
    }
    return *this;
}

bool EffectInstance::tick(LivingEntity& entity) {
    // 永久效果不减少持续时间
    if (!isPermanent()) {
        if (m_duration > 0) {
            --m_duration;
        }
        if (m_duration <= 0) {
            // 效果结束，移除属性修改
            if (m_applied) {
                remove(entity);
            }
            return false;
        }
    }

    // 每tick执行效果逻辑
    applyEffect(entity);

    return true;
}

bool EffectInstance::merge(const EffectInstance& other) {
    // 只能合并相同类型的效果
    if (m_type != other.m_type) {
        return false;
    }

    // 比较效果强度
    bool otherIsStronger = other.m_amplifier > m_amplifier;
    bool sameAmplifier = other.m_amplifier == m_amplifier;
    bool otherIsLonger = other.m_duration > m_duration;

    if (otherIsStronger) {
        // 新效果更强，完全替换
        m_amplifier = other.m_amplifier;
        m_duration = other.m_duration;
        m_ambient = other.m_ambient;
        m_visible = other.m_visible;
        m_showIcon = other.m_showIcon;
        return true;
    } else if (sameAmplifier && otherIsLonger) {
        // 同级但更长，延长时间
        m_duration = other.m_duration;
        return true;
    }

    return false;
}

void EffectInstance::apply(LivingEntity& entity) {
    if (m_applied) {
        return;
    }

    // 应用属性修改器
    const auto& modifiers = EffectAttributeModifiers::getEffectModifiers(m_type);
    for (const auto& modifierInfo : modifiers) {
        attribute::AttributeModifier modifier = EffectAttributeModifiers::createModifier(
            modifierInfo, m_amplifier
        );
        entity.attributes().addModifier(modifierInfo.attributeName, modifier);
    }

    // 特殊效果：生命提升需要设置生命值
    if (m_type == EffectType::HealthBoost && m_amplifier > 0) {
        f64 boostPerLevel = 4.0; // 每级增加4点生命
        f64 newHealth = m_amplifier * boostPerLevel + static_cast<f64>(entity.health());
        f64 maxHp = static_cast<f64>(entity.maxHealth());
        entity.setHealth(static_cast<f32>(std::min(newHealth, maxHp)));
    }

    m_applied = true;
}

void EffectInstance::remove(LivingEntity& entity) {
    if (!m_applied) {
        return;
    }

    // 移除属性修改器
    const auto& modifiers = EffectAttributeModifiers::getEffectModifiers(m_type);
    for (const auto& modifierInfo : modifiers) {
        entity.attributes().removeModifier(modifierInfo.attributeName, modifierInfo.uuid);
    }

    // 特殊效果：生命提升移除时需要调整生命值
    if (m_type == EffectType::HealthBoost) {
        f32 maxHealth = entity.maxHealth();
        if (entity.health() > maxHealth) {
            entity.setHealth(maxHealth);
        }
    }

    m_applied = false;
}

void EffectInstance::applyEffect(LivingEntity& entity) {
    // 根据效果类型执行每tick逻辑
    switch (m_type) {
        case EffectType::Regeneration: {
            // 每 50/(level+1) tick 治疗 1 HP
            i32 interval = 50 >> m_amplifier;
            if (interval <= 0) interval = 1;
            if (m_duration % interval == 0) {
                if (entity.health() < entity.maxHealth()) {
                    entity.heal(1.0f);
                }
            }
            break;
        }

        case EffectType::Poison: {
            // 每 25/(level+1) tick 造成 1 HP 伤害（不能致死）
            i32 interval = 25 >> m_amplifier;
            if (interval <= 0) interval = 1;
            if (m_duration % interval == 0) {
                if (entity.health() > 1.0f) {
                    auto source = DamageSources::magic();
                    entity.hurt(source, 1.0f);
                }
            }
            break;
        }

        case EffectType::Wither: {
            // 每 40/(level+1) tick 造成 1 HP 伤害
            i32 interval = 40 >> m_amplifier;
            if (interval <= 0) interval = 1;
            if (m_duration % interval == 0) {
                auto source = DamageSources::wither();
                entity.hurt(source, 1.0f);
            }
            break;
        }

        case EffectType::Hunger: {
            // 每tick增加饥饿消耗（如果玩家）
            // TODO: 需要Player类和食物系统
            // if (auto* player = dynamic_cast<PlayerEntity*>(&entity)) {
            //     player->addExhaustion(0.005f * (m_amplifier + 1));
            // }
            break;
        }

        case EffectType::Saturation: {
            // 瞬间效果：恢复饥饿值（如果玩家）
            // 由于是瞬间效果，这里不应被调用
            // 应在添加效果时立即处理
            break;
        }

        case EffectType::SlowFalling: {
            // 缓降效果在实体物理tick中处理
            // 减少摔落速度，取消摔落伤害
            break;
        }

        case EffectType::ConduitPower: {
            // 潮涌能量：水下呼吸+挖掘速度+视野
            // 在实体tick中处理水下检测
            break;
        }

        case EffectType::DolphinsGrace: {
            // 海豚的恩惠：增加游泳速度
            // 在实体物理tick中处理
            break;
        }

        case EffectType::WaterBreathing: {
            // 水下呼吸：在实体tick中增加氧气恢复速度
            break;
        }

        case EffectType::FireResistance: {
            // 防火：免疫火焰伤害
            // 在伤害处理中检查
            break;
        }

        case EffectType::Resistance: {
            // 抗性提升：减少伤害
            // 在伤害处理中应用
            break;
        }

        case EffectType::BadOmen:
        case EffectType::HeroOfTheVillage:
            // 这些效果不在tick时执行逻辑
            // 不祥之兆：在进入村庄时触发袭击
            // 村庄英雄：在交易时提供折扣
            break;

        default:
            break;
    }
}

// ============================================================================
// 静态工厂方法
// ============================================================================

EffectInstance EffectInstance::badOmen(i32 level) {
    // 不祥之兆等级范围 1-5
    i32 amplifier = std::max(0, std::min(level - 1, 4));
    return EffectInstance(
        EffectType::BadOmen,
        BAD_OMEN_DURATION,
        amplifier,
        false,  // 不是环境效果
        true,   // 显示粒子
        true    // 显示图标
    );
}

EffectInstance EffectInstance::heroOfTheVillage(i32 level) {
    // 村庄英雄等级范围 1-5
    i32 amplifier = std::max(0, std::min(level - 1, 4));
    return EffectInstance(
        EffectType::HeroOfTheVillage,
        HERO_DURATION,
        amplifier,
        false,  // 不是环境效果
        true,   // 显示粒子
        true    // 显示图标
    );
}

} // namespace effect
} // namespace entity
} // namespace mc
