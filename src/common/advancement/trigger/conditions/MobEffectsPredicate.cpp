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

#include "MobEffectsPredicate.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <unordered_map>
#include <utility>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::advancement {

// ========== EffectInstancePredicate ==========

EffectInstancePredicate::EffectInstancePredicate(
    IntBounds amplifier, IntBounds duration, std::optional<bool> ambient, std::optional<bool> visible)
    : m_amplifier(std::move(amplifier))
    , m_duration(std::move(duration))
    , m_ambient(std::move(ambient))
    , m_visible(std::move(visible))
{}

bool EffectInstancePredicate::test(const entity::effect::EffectInstance* effect) const
{
    // 如果没有效果实例，则不匹配
    if (effect == nullptr) {
        return false;
    }

    // 检查效果等级
    if (!m_amplifier.test(effect->amplifier())) {
        return false;
    }

    // 检查持续时间
    if (!m_duration.test(effect->duration())) {
        return false;
    }

    // 检查是否为环境效果
    if (m_ambient.has_value() && m_ambient.value() != effect->isAmbient()) {
        return false;
    }

    // 检查是否显示粒子
    if (m_visible.has_value() && m_visible.value() != effect->isVisible()) {
        return false;
    }

    return true;
}

Result<EffectInstancePredicate> EffectInstancePredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        // 空对象表示任意效果实例都匹配
        return EffectInstancePredicate{};
    }

    IntBounds amplifier;
    IntBounds duration;
    std::optional<bool> ambient;
    std::optional<bool> visible;

    if (json.contains("amplifier")) {
        amplifier = IntBounds::fromJson(json["amplifier"]);
    }

    if (json.contains("duration")) {
        duration = IntBounds::fromJson(json["duration"]);
    }

    if (json.contains("ambient")) {
        ambient = json["ambient"].get<bool>();
    }

    if (json.contains("visible")) {
        visible = json["visible"].get<bool>();
    }

    return EffectInstancePredicate(std::move(amplifier), std::move(duration), std::move(ambient), std::move(visible));
}

nlohmann::json EffectInstancePredicate::toJson() const
{
    if (isAny()) {
        return nlohmann::json::object();
    }

    nlohmann::json json;

    if (!m_amplifier.isUnbounded()) {
        json["amplifier"] = m_amplifier.toJson();
    }

    if (!m_duration.isUnbounded()) {
        json["duration"] = m_duration.toJson();
    }

    if (m_ambient.has_value()) {
        json["ambient"] = m_ambient.value();
    }

    if (m_visible.has_value()) {
        json["visible"] = m_visible.value();
    }

    return json;
}

bool EffectInstancePredicate::isAny() const noexcept
{
    return m_amplifier.isUnbounded() && m_duration.isUnbounded() && !m_ambient.has_value() && !m_visible.has_value();
}

// ========== MobEffectsPredicate ==========

MobEffectsPredicate::MobEffectsPredicate(
    std::unordered_map<entity::effect::EffectType, EffectInstancePredicate> effects)
    : m_effects(std::move(effects))
{}

bool MobEffectsPredicate::test(const Entity& entity) const
{
    // 如果没有约束，匹配任意实体
    if (isAny()) {
        return true;
    }

    // 尝试转换为 LivingEntity（只有 LivingEntity 有效果）
    const LivingEntity* living = dynamic_cast<const LivingEntity*>(&entity);
    if (living == nullptr) {
        return false;
    }

    return test(*living);
}

bool MobEffectsPredicate::test(const LivingEntity& entity) const
{
    // 如果没有约束，匹配任意实体
    if (isAny()) {
        return true;
    }

    // 检查每个要求的效果
    for (const auto& [effectType, predicate] : m_effects) {
        // 获取实体身上的效果实例
        const entity::effect::EffectInstance* effect = entity.getEffect(effectType);

        // 检查效果是否满足条件
        if (!predicate.test(effect)) {
            return false;
        }
    }

    return true;
}

Result<MobEffectsPredicate> MobEffectsPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return MobEffectsPredicate{};
    }

    std::unordered_map<entity::effect::EffectType, EffectInstancePredicate> effects;

    // JSON 格式: { "minecraft:speed": { "amplifier": {...}, ... }, ... }
    if (json.is_object()) {
        for (auto& [key, value] : json.items()) {
            // 解析效果类型
            ResourceLocation effectId(key);
            auto effectType = entity::effect::getEffectByResourceLocation(effectId);

            if (!effectType.has_value()) {
                spdlog::warn("Unknown effect type in MobEffectsPredicate: {}", key);
                continue;
            }

            // 解析实例谓词
            auto predicateResult = EffectInstancePredicate::fromJson(value);
            if (predicateResult.failed()) {
                return predicateResult.error();
            }

            effects[effectType.value()] = predicateResult.value();
        }
    }

    return MobEffectsPredicate(std::move(effects));
}

nlohmann::json MobEffectsPredicate::toJson() const
{
    if (isAny()) {
        return nullptr;
    }

    nlohmann::json json = nlohmann::json::object();

    for (const auto& [effectType, predicate] : m_effects) {
        ResourceLocation effectId = entity::effect::getEffectResourceLocation(effectType);
        json[effectId.toString()] = predicate.toJson();
    }

    return json;
}

} // namespace mc::advancement
