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

#include "EntityEquipmentPredicate.hpp"
#include "EntityFlagsPredicate.hpp"
#include "LocationPredicate.hpp"
#include "MobEffectsPredicate.hpp"
#include "NBTPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class Entity;
class LivingEntity;
class DamageSource;
class IWorld;
} // namespace mc

namespace mc::advancement {

/**
 * @brief 实体谓词
 *
 * 用于匹配实体的条件谓词，检查实体类型、位置、效果、装备、NBT等。
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
     * @brief 检查实体是否匹配（带世界和参考位置）
     *
     * 用于距离检查等需要参考位置的条件。
     *
     * @param world 世界
     * @param x 参考位置 X
     * @param y 参考位置 Y
     * @param z 参考位置 Z
     * @param entity 实体
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const IWorld& world, f64 x, f64 y, f64 z, const Entity& entity) const;

    /**
     * @brief 检查实体是否匹配（带伤害源）
     * @param entity 实体
     * @param source 伤害源
     * @return 是否匹配
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
    [[nodiscard]] const DistancePredicate& getDistance() const noexcept { return m_distance; }
    [[nodiscard]] const LocationPredicate& getLocation() const noexcept { return m_location; }
    [[nodiscard]] const MobEffectsPredicate& getEffects() const noexcept { return m_effects; }
    [[nodiscard]] const NBTPredicate& getNbt() const noexcept { return m_nbt; }
    [[nodiscard]] const EntityFlagsPredicate& getFlags() const noexcept { return m_flags; }
    [[nodiscard]] const EntityEquipmentPredicate& getEquipment() const noexcept { return m_equipment; }

    // ========== Setters ==========

    void setType(std::optional<ResourceLocation> type)
    {
        m_type = std::move(type);
        _updateIsAny();
    }
    void setDistance(DistancePredicate distance)
    {
        m_distance = std::move(distance);
        _updateIsAny();
    }
    void setLocation(LocationPredicate location)
    {
        m_location = std::move(location);
        _updateIsAny();
    }
    void setEffects(MobEffectsPredicate effects)
    {
        m_effects = std::move(effects);
        _updateIsAny();
    }
    void setNbt(NBTPredicate nbt)
    {
        m_nbt = std::move(nbt);
        _updateIsAny();
    }
    void setFlags(EntityFlagsPredicate flags)
    {
        m_flags = std::move(flags);
        _updateIsAny();
    }
    void setEquipment(EntityEquipmentPredicate equipment)
    {
        m_equipment = std::move(equipment);
        _updateIsAny();
    }

private:
    void _updateIsAny();

    std::optional<ResourceLocation> m_type; ///< 实体类型（如 "minecraft:zombie"）
    DistancePredicate m_distance;           ///< 距离谓词（与参考点的距离）
    LocationPredicate m_location;           ///< 位置谓词（生物群系、维度等）
    MobEffectsPredicate m_effects;          ///< 效果谓词
    NBTPredicate m_nbt;                     ///< NBT谓词
    EntityFlagsPredicate m_flags;           ///< 标志谓词（燃烧、潜行等）
    EntityEquipmentPredicate m_equipment;   ///< 装备谓词
    bool m_isAny = true;
};

/**
 * @brief 伤害源谓词
 *
 * 用于匹配伤害源的条件谓词，检查伤害类型标志。
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

    // ========== Getters ==========

    [[nodiscard]] const std::optional<bool>& isProjectile() const noexcept { return m_isProjectile; }
    [[nodiscard]] const std::optional<bool>& isExplosion() const noexcept { return m_isExplosion; }
    [[nodiscard]] const std::optional<bool>& isFire() const noexcept { return m_isFire; }
    [[nodiscard]] const std::optional<bool>& isMagic() const noexcept { return m_isMagic; }
    [[nodiscard]] const std::optional<bool>& isLightning() const noexcept { return m_isLightning; }
    [[nodiscard]] const std::optional<bool>& bypassesArmor() const noexcept { return m_bypassesArmor; }
    [[nodiscard]] const std::optional<bool>& bypassesInvulnerability() const noexcept
    {
        return m_bypassesInvulnerability;
    }
    [[nodiscard]] const std::optional<bool>& bypassesMagic() const noexcept { return m_bypassesMagic; }

private:
    std::optional<bool> m_isProjectile;            ///< 是否为投射物伤害
    std::optional<bool> m_isExplosion;             ///< 是否为爆炸伤害
    std::optional<bool> m_isFire;                  ///< 是否为火焰伤害
    std::optional<bool> m_isMagic;                 ///< 是否为魔法伤害
    std::optional<bool> m_isLightning;             ///< 是否为闪电伤害
    std::optional<bool> m_bypassesArmor;           ///< 是否绕过护甲
    std::optional<bool> m_bypassesInvulnerability; ///< 是否绕过无敌模式
    std::optional<bool> m_bypassesMagic;           ///< 是否绕过魔法保护
    bool m_isAny = true;
};

} // namespace mc::advancement
