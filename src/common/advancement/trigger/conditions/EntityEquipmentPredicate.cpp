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

#include "EntityEquipmentPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

bool EntityEquipmentPredicate::test(const Entity& entity) const
{
    if (isAny()) {
        return true;
    }

    // 只有 LivingEntity 才有装备
    const LivingEntity* living = dynamic_cast<const LivingEntity*>(&entity);
    if (living == nullptr) {
        return false;
    }

    return test(*living);
}

bool EntityEquipmentPredicate::test(const LivingEntity& entity) const
{
    if (isAny()) {
        return true;
    }

    // 检查每个装备槽位

    // 检查头盔
    if (!m_head.test(entity.getEquipment(EquipmentSlot::Head))) {
        return false;
    }

    // 检查胸甲
    if (!m_chest.test(entity.getEquipment(EquipmentSlot::Chest))) {
        return false;
    }

    // 检查护腿
    if (!m_legs.test(entity.getEquipment(EquipmentSlot::Legs))) {
        return false;
    }

    // 检查靴子
    if (!m_feet.test(entity.getEquipment(EquipmentSlot::Feet))) {
        return false;
    }

    // 检查主手物品
    if (!m_mainHand.test(entity.getEquipment(EquipmentSlot::MainHand))) {
        return false;
    }

    // 检查副手物品
    if (!m_offHand.test(entity.getEquipment(EquipmentSlot::OffHand))) {
        return false;
    }

    return true;
}

bool EntityEquipmentPredicate::isAny() const noexcept
{
    return m_head.isAny() && m_chest.isAny() && m_legs.isAny() && m_feet.isAny() && m_mainHand.isAny() &&
        m_offHand.isAny();
}

Result<EntityEquipmentPredicate> EntityEquipmentPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return EntityEquipmentPredicate{};
    }

    EntityEquipmentPredicate predicate;

    if (json.contains("head")) {
        auto result = ItemPredicate::fromJson(json["head"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_head = result.value();
    }

    if (json.contains("chest")) {
        auto result = ItemPredicate::fromJson(json["chest"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_chest = result.value();
    }

    if (json.contains("legs")) {
        auto result = ItemPredicate::fromJson(json["legs"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_legs = result.value();
    }

    if (json.contains("feet")) {
        auto result = ItemPredicate::fromJson(json["feet"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_feet = result.value();
    }

    if (json.contains("mainhand")) {
        auto result = ItemPredicate::fromJson(json["mainhand"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_mainHand = result.value();
    }

    if (json.contains("offhand")) {
        auto result = ItemPredicate::fromJson(json["offhand"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_offHand = result.value();
    }

    return predicate;
}

nlohmann::json EntityEquipmentPredicate::toJson() const
{
    if (isAny()) {
        return nullptr;
    }

    nlohmann::json json;

    if (!m_head.isAny()) {
        json["head"] = m_head.toJson();
    }
    if (!m_chest.isAny()) {
        json["chest"] = m_chest.toJson();
    }
    if (!m_legs.isAny()) {
        json["legs"] = m_legs.toJson();
    }
    if (!m_feet.isAny()) {
        json["feet"] = m_feet.toJson();
    }
    if (!m_mainHand.isAny()) {
        json["mainhand"] = m_mainHand.toJson();
    }
    if (!m_offHand.isAny()) {
        json["offhand"] = m_offHand.toJson();
    }

    return json;
}

} // namespace mc::advancement
