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

#include "common/core/Result.hpp"
#include <optional>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class Entity;
class LivingEntity;
} // namespace mc

namespace mc::advancement {

/**
 * @brief 实体标志谓词
 *
 * 用于匹配实体状态标志的条件谓词。
 */
class EntityFlagsPredicate {
public:
    /**
     * @brief 默认构造（匹配任意标志）
     */
    EntityFlagsPredicate() = default;

    /**
     * @brief 检查实体是否匹配所有标志条件
     * @param entity 实体
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const Entity& entity) const;

    /**
     * @brief 检查是否匹配任意标志
     */
    [[nodiscard]] bool isAny() const noexcept;

    /**
     * @brief 从JSON解析
     */
    static Result<EntityFlagsPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const std::optional<bool>& isOnFire() const noexcept { return m_isOnFire; }
    [[nodiscard]] const std::optional<bool>& isSneaking() const noexcept { return m_isSneaking; }
    [[nodiscard]] const std::optional<bool>& isSprinting() const noexcept { return m_isSprinting; }
    [[nodiscard]] const std::optional<bool>& isSwimming() const noexcept { return m_isSwimming; }
    [[nodiscard]] const std::optional<bool>& isBaby() const noexcept { return m_isBaby; }

    // ========== Setters ==========

    void setOnFire(std::optional<bool> value)
    {
        m_isOnFire = value;
        _updateIsAny();
    }
    void setSneaking(std::optional<bool> value)
    {
        m_isSneaking = value;
        _updateIsAny();
    }
    void setSprinting(std::optional<bool> value)
    {
        m_isSprinting = value;
        _updateIsAny();
    }
    void setSwimming(std::optional<bool> value)
    {
        m_isSwimming = value;
        _updateIsAny();
    }
    void setBaby(std::optional<bool> value)
    {
        m_isBaby = value;
        _updateIsAny();
    }

private:
    void _updateIsAny();

    std::optional<bool> m_isOnFire;    ///< 是否燃烧
    std::optional<bool> m_isSneaking;  ///< 是否潜行
    std::optional<bool> m_isSprinting; ///< 是否疾跑
    std::optional<bool> m_isSwimming;  ///< 是否游泳
    std::optional<bool> m_isBaby;      ///< 是否幼年
    bool m_isAny = true;
};

} // namespace mc::advancement
