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

#include "ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include <memory>
#include <optional>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class Entity;
class LivingEntity;
} // namespace mc

namespace mc::advancement {

/**
 * @brief 实体装备谓词
 *
 * 用于匹配实体装备的条件谓词。
 */
class EntityEquipmentPredicate {
public:
    /**
     * @brief 默认构造（匹配任意装备）
     */
    EntityEquipmentPredicate() = default;

    /**
     * @brief 检查实体装备是否匹配
     * @param entity 实体
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const Entity& entity) const;

    /**
     * @brief 检查 LivingEntity 装备是否匹配
     * @param entity LivingEntity
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const LivingEntity& entity) const;

    /**
     * @brief 检查是否匹配任意装备
     */
    [[nodiscard]] bool isAny() const noexcept;

    /**
     * @brief 从JSON解析
     */
    static Result<EntityEquipmentPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const ItemPredicate& getHead() const noexcept { return m_head; }
    [[nodiscard]] const ItemPredicate& getChest() const noexcept { return m_chest; }
    [[nodiscard]] const ItemPredicate& getLegs() const noexcept { return m_legs; }
    [[nodiscard]] const ItemPredicate& getFeet() const noexcept { return m_feet; }
    [[nodiscard]] const ItemPredicate& getMainHand() const noexcept { return m_mainHand; }
    [[nodiscard]] const ItemPredicate& getOffHand() const noexcept { return m_offHand; }

    // ========== Setters ==========

    void setHead(ItemPredicate head) { m_head = std::move(head); }
    void setChest(ItemPredicate chest) { m_chest = std::move(chest); }
    void setLegs(ItemPredicate legs) { m_legs = std::move(legs); }
    void setFeet(ItemPredicate feet) { m_feet = std::move(feet); }
    void setMainHand(ItemPredicate mainHand) { m_mainHand = std::move(mainHand); }
    void setOffHand(ItemPredicate offHand) { m_offHand = std::move(offHand); }

private:
    ItemPredicate m_head;     ///< 头盔
    ItemPredicate m_chest;    ///< 胸甲
    ItemPredicate m_legs;     ///< 护腿
    ItemPredicate m_feet;     ///< 靴子
    ItemPredicate m_mainHand; ///< 主手物品
    ItemPredicate m_offHand;  ///< 副手物品
};

} // namespace mc::advancement
