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
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <optional>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class DamageSource;
class Entity;

namespace advancement {

/**
 * @brief 伤害谓词（对齐 vanilla DamagePredicate）
 *
 * 用于 player_hurt_entity 等"谁打了我/我打了谁"类进度条件，字段：
 *  - dealt：本次造成的伤害数值范围；
 *  - taken：受击方实际承受的伤害数值范围；
 *  - source_entity：造成者实体谓词（对应 DamageSource::getTrueSource）；
 *  - blocked：是否被格挡；
 *  - type：伤害源谓词（DamageSourcePredicate）。
 *
 * Cubium 原缺失该谓词，player_hurt_entity 条件整段被忽略，数据包
 * adventure/overoverkill（重锤一击 100 点）等进度恒不可达。
 */
class DamagePredicate {
public:
    DamagePredicate() = default;

    /**
     * @brief 检查伤害是否匹配
     * @param source 伤害源
     * @param dealt 本次造成的伤害
     * @param taken 受击方实际承受的伤害
     * @param blocked 是否被格挡
     */
    [[nodiscard]] bool test(const DamageSource& source, f32 dealt, f32 taken, bool blocked) const;

    [[nodiscard]] bool isAny() const noexcept
    {
        return m_isAny;
    }

    static Result<DamagePredicate> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const MinMaxBounds<f64>& getDealt() const noexcept
    {
        return m_dealt;
    }

    [[nodiscard]] const MinMaxBounds<f64>& getTaken() const noexcept
    {
        return m_taken;
    }

    [[nodiscard]] const std::optional<EntityPredicate>& getSourceEntity() const noexcept
    {
        return m_sourceEntity;
    }

    [[nodiscard]] const std::optional<bool>& getBlocked() const noexcept
    {
        return m_blocked;
    }

    [[nodiscard]] const std::optional<DamageSourcePredicate>& getType() const noexcept
    {
        return m_type;
    }

    DamagePredicate(const DamagePredicate& other);
    DamagePredicate& operator=(const DamagePredicate& other);
    DamagePredicate(DamagePredicate&&) noexcept = default;
    DamagePredicate& operator=(DamagePredicate&&) noexcept = default;
    ~DamagePredicate() = default;

private:
    void _updateIsAny();

    MinMaxBounds<f64> m_dealt;                     ///< 造成伤害范围
    MinMaxBounds<f64> m_taken;                     ///< 承受伤害范围
    std::optional<EntityPredicate> m_sourceEntity; ///< 造成者实体谓词
    std::optional<bool> m_blocked;                 ///< 是否格挡
    std::optional<DamageSourcePredicate> m_type;   ///< 伤害源谓词
    bool m_isAny = true;
};

} // namespace advancement
} // namespace mc
