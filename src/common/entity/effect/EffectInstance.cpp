#include "EffectInstance.hpp"
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

    // 应用属性修改
    // TODO: 根据效果类型修改实体属性
    // 例如：速度效果修改 MOVEMENT_SPEED 属性

    m_applied = true;
}

void EffectInstance::remove(LivingEntity& entity) {
    if (!m_applied) {
        return;
    }

    // 移除属性修改
    // TODO: 根据效果类型还原实体属性

    m_applied = false;
}

void EffectInstance::applyEffect(LivingEntity& entity) {
    // 根据效果类型执行每tick逻辑
    switch (m_type) {
        case EffectType::Regeneration:
            // 每 50/(level+1) tick 治疗 1 HP
            // TODO: 实现治疗逻辑
            break;

        case EffectType::Poison:
            // 每 25/(level+1) tick 造成 1 HP 伤害
            // TODO: 实现中毒伤害
            break;

        case EffectType::Wither:
            // 每 40/(level+1) tick 造成 1 HP 伤害
            // TODO: 实现凋零伤害
            break;

        case EffectType::Hunger:
            // 增加饥饿消耗
            // TODO: 实现饥饿效果
            break;

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
