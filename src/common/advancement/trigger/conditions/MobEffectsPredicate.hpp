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

#pragma once

#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/entity/effect/EffectType.hpp"
#include <optional>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;

namespace entity::effect {
class EffectInstance;
}

namespace advancement {

/**
 * @brief 效果实例谓词
 *
 * 用于匹配单个效果实例的条件，检查效果的等级、持续时间等属性。
 *
 * JSON 格式示例:
 * @code
 * {
 *   "amplifier": { "min": 0, "max": 2 },
 *   "duration": { "min": 100 },
 *   "ambient": false,
 *   "visible": true
 * }
 * @endcode
 */
class EffectInstancePredicate {
public:
    /**
     * @brief 默认构造（匹配任意效果实例）
     */
    EffectInstancePredicate() = default;

    /**
     * @brief 构造效果实例谓词
     * @param amplifier 效果等级范围（0 = I级，1 = II级，以此类推）
     * @param duration 持续时间范围（tick）
     * @param ambient 是否为环境效果（如信标）
     * @param visible 是否显示粒子
     */
    EffectInstancePredicate(
        IntBounds amplifier, IntBounds duration, std::optional<bool> ambient, std::optional<bool> visible);

    /**
     * @brief 检查效果实例是否匹配
     * @param effect 效果实例（可能为 nullptr）
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const entity::effect::EffectInstance* effect) const;

    /**
     * @brief 从 JSON 解析
     */
    static Result<EffectInstancePredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为 JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 是否匹配任意效果实例
     */
    [[nodiscard]] bool isAny() const noexcept;

    // ========== Getters ==========

    [[nodiscard]] const IntBounds& getAmplifier() const noexcept { return m_amplifier; }
    [[nodiscard]] const IntBounds& getDuration() const noexcept { return m_duration; }
    [[nodiscard]] const std::optional<bool>& getAmbient() const noexcept { return m_ambient; }
    [[nodiscard]] const std::optional<bool>& getVisible() const noexcept { return m_visible; }

private:
    IntBounds m_amplifier;         ///< 效果等级范围
    IntBounds m_duration;          ///< 持续时间范围（tick）
    std::optional<bool> m_ambient; ///< 是否为环境效果
    std::optional<bool> m_visible; ///< 是否显示粒子
};

/**
 * @brief 实体效果谓词
 *
 * 检查实体身上的效果状态，验证特定效果是否存在以及其属性是否符合条件。
 *
 * JSON 格式示例:
 * @code
 * {
 *   "minecraft:speed": {
 *     "amplifier": { "min": 0, "max": 2 },
 *     "duration": { "min": 100 }
 *   },
 *   "minecraft:regeneration": {}
 * }
 * @endcode
 */
class MobEffectsPredicate {
public:
    /// 默认构造（匹配任意效果状态）
    MobEffectsPredicate() = default;

    /**
     * @brief 构造效果谓词
     * @param effects 效果类型到实例谓词的映射
     */
    explicit MobEffectsPredicate(std::unordered_map<entity::effect::EffectType, EffectInstancePredicate> effects);

    /**
     * @brief 检查实体是否匹配
     * @param entity 实体
     * @return 是否匹配
     *
     * 只有 LivingEntity 有效果，非 LivingEntity 始终返回 false（除非谓词为空）。
     */
    [[nodiscard]] bool test(const Entity& entity) const;

    /**
     * @brief 检查 LivingEntity 是否匹配
     * @param entity LivingEntity
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const LivingEntity& entity) const;

    /**
     * @brief 从 JSON 解析
     */
    static Result<MobEffectsPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为 JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 是否匹配任意效果状态（无约束）
     */
    [[nodiscard]] bool isAny() const noexcept { return m_effects.empty(); }

    /**
     * @brief 获取效果映射
     */
    [[nodiscard]] const std::unordered_map<entity::effect::EffectType, EffectInstancePredicate>&
    getEffects() const noexcept
    {
        return m_effects;
    }

private:
    std::unordered_map<entity::effect::EffectType, EffectInstancePredicate> m_effects;
};

} // namespace advancement

} // namespace mc
