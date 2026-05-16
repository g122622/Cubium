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

#include "../../MinMaxBounds.hpp"
#include "MobEffectsPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// 前向声明
namespace mc {
class Entity;
class DamageSource;
} // namespace mc

namespace mc::advancement {

/**
 * @brief 实体谓词
 *
 * 用于匹配实体的条件谓词，检查实体类型、位置、效果等。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.EntityPredicate
 */
class EntityPredicate {
public:
    /**
     * @brief 默认构造（匹配任意实体）
     */
    EntityPredicate() = default;

    /**
     * @brief 检查实体是否匹配
     * @param entity 实体
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const Entity& entity) const;

    /**
     * @brief 检查实体是否匹配（带伤害源）
     */
    [[nodiscard]] bool test(const Entity& entity, const DamageSource& source) const;

    /**
     * @brief 检查是否匹配任意实体
     */
    [[nodiscard]] bool isAny() const noexcept { return m_isAny; }

    /**
     * @brief 从JSON解析
     */
    static Result<EntityPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const std::optional<ResourceLocation>& getType() const noexcept { return m_type; }
    [[nodiscard]] const MobEffectsPredicate& getEffects() const noexcept { return m_effects; }

private:
    std::optional<ResourceLocation> m_type; ///< 实体类型
    MobEffectsPredicate m_effects;          ///< 效果谓词
    // TODO: 更多匹配条件（距离、位置、NBT等）
    bool m_isAny = true;
};

/**
 * @brief 伤害源谓词
 *
 * 用于匹配伤害源的条件谓词。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.DamageSourcePredicate
 */
class DamageSourcePredicate {
public:
    /**
     * @brief 默认构造（匹配任意伤害源）
     */
    DamageSourcePredicate() = default;

    /**
     * @brief 检查伤害源是否匹配
     */
    [[nodiscard]] bool test(const DamageSource& source) const;

    /**
     * @brief 检查是否匹配任意伤害源
     */
    [[nodiscard]] bool isAny() const noexcept { return m_isAny; }

    /**
     * @brief 从JSON解析
     */
    static Result<DamageSourcePredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    // TODO: 伤害类型、是否魔法、是否爆炸、是否火焰等
    bool m_isAny = true;
};

} // namespace mc::advancement
