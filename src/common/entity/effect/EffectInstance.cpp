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

#include "EffectInstance.hpp"
#include "EffectAttributeModifiers.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <algorithm>
#include <memory>

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

/// 试炼之兆基础持续时间（15000 ticks × 等级）
constexpr i32 TRIAL_OMEN_DURATION_PER_LEVEL = 15000;

/// 风充能持续时间（与风弹效果同步）
constexpr i32 WIND_CHARGED_DURATION = 200;

/// 袭击之兆持续时间（30000 ticks）
constexpr i32 RAID_OMEN_DURATION = 30000;

// NBT key constants
namespace nbt_keys {
constexpr const char* ID = "Id";
constexpr const char* AMPLIFIER = "Amplifier";
constexpr const char* DURATION = "Duration";
constexpr const char* AMBIENT = "Ambient";
constexpr const char* SHOW_PARTICLES = "ShowParticles";
constexpr const char* SHOW_ICON = "ShowIcon";
} // namespace nbt_keys

} // namespace

// ============================================================================
// EffectInstance 实现
// ============================================================================

EffectInstance::EffectInstance(EffectType type, i32 duration, i32 amplifier, bool ambient, bool visible, bool showIcon)
    : m_type(type)
    , m_duration(duration)
    , m_amplifier(amplifier)
    , m_ambient(ambient)
    , m_visible(visible)
    , m_showIcon(showIcon)
{}

EffectInstance::EffectInstance(const EffectInstance& other)
    : m_type(other.m_type)
    , m_duration(other.m_duration)
    , m_amplifier(other.m_amplifier)
    , m_ambient(other.m_ambient)
    , m_visible(other.m_visible)
    , m_showIcon(other.m_showIcon)
    , m_applied(other.m_applied)
    , m_hiddenEffect(other.m_hiddenEffect ? std::make_shared<EffectInstance>(*other.m_hiddenEffect) : nullptr)
{}

EffectInstance& EffectInstance::operator=(const EffectInstance& other)
{
    if (this != &other) {
        m_type = other.m_type;
        m_duration = other.m_duration;
        m_amplifier = other.m_amplifier;
        m_ambient = other.m_ambient;
        m_visible = other.m_visible;
        m_showIcon = other.m_showIcon;
        m_applied = other.m_applied;
        m_hiddenEffect = other.m_hiddenEffect ? std::make_shared<EffectInstance>(*other.m_hiddenEffect) : nullptr;
    }
    return *this;
}

bool EffectInstance::tick(LivingEntity& entity)
{
    // 永久效果不减少持续时间
    if (!isPermanent()) {
        if (m_duration <= 0) {
            // 效果已结束，移除属性修改
            if (m_applied) {
                remove(entity);
            }
            return false;
        }
    }

    // 先用递减前的 m_duration 执行效果逻辑，再递减持续时间：
    //   int i = this.duration;  // 递减前
    //   if (effect.shouldApplyEffectTickThisTick(i, amplifier) && !effect.applyEffectTick(...)) return false;
    //   this.tickDownDuration();  // 递减在作用之后
    // 间隔型效果（凋零/中毒/再生）的 shouldApplyEffectTickThisTick 判定用 duration % interval == 0，
    // 必须用递减前的 duration，否则"duration 恰为 interval 整数倍"的那次作用会因递减后判定错位而漏掉
    // （例如凋零 duration=40/interval=40：递减前 40%40==0 命中首次伤害；若先递减成 39 再判定则永远不命中，
    // 且归零 tick 的 return false 会再吞掉一次，整个生命周期 0 次伤害）。
    _applyEffect(entity);

    // 作用之后递减持续时间
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

    return true;
}

bool EffectInstance::merge(const EffectInstance& other)
{
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
        // 注意：属性修改器的更新需要由调用方（EffectManager）处理
        // 因为这里没有实体的引用，无法直接操作属性系统
        m_amplifier = other.m_amplifier;
        m_duration = other.m_duration;
        m_ambient = other.m_ambient;
        m_visible = other.m_visible;
        m_showIcon = other.m_showIcon;
        // 标记需要重新应用属性修改器
        // 调用方会先 remove() 旧的再 apply() 新的
        m_applied = false;
        return true;
    } else if (sameAmplifier && otherIsLonger) {
        // 同级但更长，延长时间
        m_duration = other.m_duration;
        return true;
    }

    return false;
}

void EffectInstance::apply(LivingEntity& entity)
{
    if (m_applied) {
        return;
    }

    // 应用属性修改器
    const auto& modifiers = EffectAttributeModifiers::getEffectModifiers(m_type);
    for (const auto& modifierInfo : modifiers) {
        attribute::AttributeModifier modifier =
            EffectAttributeModifiers::createModifier(modifierInfo, m_type, m_amplifier);
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

void EffectInstance::remove(LivingEntity& entity)
{
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

void EffectInstance::applyInstantly(LivingEntity& entity)
{
    // 直接执行效果的 tick 逻辑，不递减持续时间
    _applyEffect(entity);
}

void EffectInstance::_applyEffect(LivingEntity& entity)
{
    // 根据效果类型执行每tick逻辑
    switch (m_type) {
        case EffectType::InstantHealth: {
            // 瞬间治疗：亡灵生物受到伤害，普通生物治疗
            // MC 原版: 基础值 4.0，每级增加 2.0
            // 注意：距离因子（multiplier）在此处默认为 1.0
            // 药水云中使用的半强度由 applyInstantEffect 单独处理
            f32 amount = 4.0f + static_cast<f32>(m_amplifier) * 2.0f;
            if (entity.getCreatureAttribute() == CreatureAttribute::Undead) {
                auto source = DamageSources::magic();
                entity.hurt(source, amount);
            } else {
                entity.heal(amount);
            }
            break;
        }

        case EffectType::InstantDamage: {
            // 瞬间伤害：亡灵生物治疗，普通生物受到伤害
            f32 amount = 4.0f + static_cast<f32>(m_amplifier) * 2.0f;
            if (entity.getCreatureAttribute() == CreatureAttribute::Undead) {
                entity.heal(amount);
            } else {
                auto source = DamageSources::magic();
                entity.hurt(source, amount);
            }
            break;
        }

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
            // 每tick增加饥饿消耗
            // exhaustion += 0.005F * (amplifier + 1)
            if (auto* player = dynamic_cast<Player*>(&entity)) {
                player->addExhaustion(0.005f * static_cast<f32>(m_amplifier + 1));
            }
            break;
        }

        case EffectType::Saturation: {
            // 瞬间效果：恢复饥饿值和饱和度（仅对玩家有效）
            // MC 原版: player.getFoodData().eat(amplifier + 1, 1.0F)
            // foodLevel += (amplifier + 1), saturationLevel += (amplifier + 1) * 1.0 * 2.0
            if (auto* player = dynamic_cast<Player*>(&entity)) {
                i32 nutrition = m_amplifier + 1;
                player->foodStats().addStats(nutrition, 1.0f);
            }
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

EffectInstance EffectInstance::badOmen(i32 level)
{
    // 不祥之兆等级范围 1-5
    i32 amplifier = std::max(0, std::min(level - 1, 4));
    return EffectInstance(EffectType::BadOmen,
        BAD_OMEN_DURATION,
        amplifier,
        false, // 不是环境效果
        true,  // 显示粒子
        true   // 显示图标
    );
}

EffectInstance EffectInstance::heroOfTheVillage(i32 level)
{
    // 村庄英雄等级范围 1-5
    i32 amplifier = std::max(0, std::min(level - 1, 4));
    return EffectInstance(EffectType::HeroOfTheVillage,
        HERO_DURATION,
        amplifier,
        false, // 不是环境效果
        true,  // 显示粒子
        true   // 显示图标
    );
}

EffectInstance EffectInstance::trialOmen(i32 level)
{
    // 试炼之兆等级范围 1-5
    i32 amplifier = std::max(0, std::min(level - 1, 4));
    // 持续时间 = 等级 × 15000 ticks
    i32 duration = level * TRIAL_OMEN_DURATION_PER_LEVEL;
    return EffectInstance(EffectType::TrialOmen,
        duration,
        amplifier,
        false, // 不是环境效果
        true,  // 显示粒子
        true   // 显示图标
    );
}

EffectInstance EffectInstance::windCharged(i32 level)
{
    // 风充能等级范围 1-1（目前只有I级）
    i32 amplifier = std::max(0, std::min(level - 1, 0));
    return EffectInstance(EffectType::WindCharged,
        WIND_CHARGED_DURATION,
        amplifier,
        false, // 不是环境效果
        true,  // 显示粒子
        true   // 显示图标
    );
}

EffectInstance EffectInstance::raidOmen(i32 level)
{
    // 袭击之兆等级范围 1-5
    i32 amplifier = std::max(0, std::min(level - 1, 4));
    return EffectInstance(EffectType::RaidOmen,
        RAID_OMEN_DURATION,
        amplifier,
        false, // 不是环境效果
        true,  // 显示粒子
        true   // 显示图标
    );
}

// ============================================================================
// 序列化
// ============================================================================

void EffectInstance::toNbt(nbt::tags::compound_tag& tag) const
{
    tag.put(nbt_keys::ID, static_cast<i8>(static_cast<i32>(m_type)));
    tag.put(nbt_keys::AMPLIFIER, static_cast<i8>(m_amplifier));
    tag.put(nbt_keys::DURATION, m_duration);
    tag.put(nbt_keys::AMBIENT, static_cast<i8>(m_ambient ? 1 : 0));
    tag.put(nbt_keys::SHOW_PARTICLES, static_cast<i8>(m_visible ? 1 : 0));
    tag.put(nbt_keys::SHOW_ICON, static_cast<i8>(m_showIcon ? 1 : 0));
}

EffectInstance EffectInstance::fromNbt(const nbt::tags::compound_tag& tag)
{
    // 读取效果类型
    EffectType type = EffectType::Speed; // 默认值
    auto it = tag.value.find(nbt_keys::ID);
    if (it != tag.value.end()) {
        if (it->second->id() == nbt::TagId::Byte) {
            type = static_cast<EffectType>(dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value);
        } else if (it->second->id() == nbt::TagId::Int) {
            type = static_cast<EffectType>(dynamic_cast<const nbt::tags::int_tag&>(*it->second).value);
        }
    }

    // 读取等级
    i32 amplifier = 0;
    it = tag.value.find(nbt_keys::AMPLIFIER);
    if (it != tag.value.end()) {
        if (it->second->id() == nbt::TagId::Byte) {
            amplifier = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value;
        } else if (it->second->id() == nbt::TagId::Int) {
            amplifier = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
        }
    }

    // 读取持续时间
    i32 duration = 600; // 默认30秒
    it = tag.value.find(nbt_keys::DURATION);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Int) {
        duration = dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
    }

    // 读取标志
    bool ambient = false;
    it = tag.value.find(nbt_keys::AMBIENT);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Byte) {
        ambient = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value != 0;
    }

    bool visible = true;
    it = tag.value.find(nbt_keys::SHOW_PARTICLES);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Byte) {
        visible = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value != 0;
    }

    bool showIcon = true;
    it = tag.value.find(nbt_keys::SHOW_ICON);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Byte) {
        showIcon = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value != 0;
    }

    return EffectInstance(type, duration, amplifier, ambient, visible, showIcon);
}

} // namespace effect
} // namespace entity
} // namespace mc
