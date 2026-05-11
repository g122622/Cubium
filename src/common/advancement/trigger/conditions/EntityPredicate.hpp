#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "../../MinMaxBounds.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

// 前向声明
namespace mc {
    class Entity;
    struct DamageSource;
}

namespace mc::advancement {

/**
 * @brief 实体效果谓词
 *
 * 检查实体身上的效果状态。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.MobEffectsPredicate
 */
class MobEffectsPredicate {
public:
    /**
     * @brief 检查实体是否匹配
     */
    [[nodiscard]] bool test(const Entity& entity) const;

    /**
     * @brief 从JSON解析
     */
    static Result<MobEffectsPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    // TODO: 效果类型和等级匹配
    bool m_isAny = true;
};

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

private:
    std::optional<ResourceLocation> m_type;      ///< 实体类型
    // TODO: 更多匹配条件（距离、位置、效果、NBT等）
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
