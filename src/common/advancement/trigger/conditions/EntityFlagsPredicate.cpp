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

#include "EntityFlagsPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

bool EntityFlagsPredicate::test(const Entity& entity) const
{
    if (m_isAny) {
        return true;
    }

    // 检查是否燃烧
    if (m_isOnFire.has_value()) {
        if (entity.isOnFire() != m_isOnFire.value()) {
            return false;
        }
    }

    // 检查是否潜行
    if (m_isSneaking.has_value()) {
        if (entity.isSneaking() != m_isSneaking.value()) {
            return false;
        }
    }

    // 检查是否疾跑
    // 注意：isSprinting() 和 isSwimming() 只在 Player 类中实现
    if (m_isSprinting.has_value()) {
        // 尝试转换为 Player，只有玩家才有疾跑状态
        const Player* player = dynamic_cast<const Player*>(&entity);
        bool isSprinting = (player != nullptr && player->isSprinting());
        if (isSprinting != m_isSprinting.value()) {
            return false;
        }
    }

    // 检查是否游泳
    if (m_isSwimming.has_value()) {
        // 尝试转换为 Player，只有玩家才有游泳状态
        const Player* player = dynamic_cast<const Player*>(&entity);
        bool isSwimming = (player != nullptr && player->isSwimming());
        if (isSwimming != m_isSwimming.value()) {
            return false;
        }
    }

    // 检查是否幼年
    if (m_isBaby.has_value()) {
        bool isBaby = entity.isChild();
        if (isBaby != m_isBaby.value()) {
            return false;
        }
    }

    return true;
}

bool EntityFlagsPredicate::isAny() const noexcept
{
    return m_isAny;
}

Result<EntityFlagsPredicate> EntityFlagsPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return EntityFlagsPredicate{};
    }

    EntityFlagsPredicate predicate;

    if (json.contains("is_on_fire")) {
        predicate.m_isOnFire = json["is_on_fire"].get<bool>();
    }
    if (json.contains("is_sneaking")) {
        predicate.m_isSneaking = json["is_sneaking"].get<bool>();
    }
    if (json.contains("is_sprinting")) {
        predicate.m_isSprinting = json["is_sprinting"].get<bool>();
    }
    if (json.contains("is_swimming")) {
        predicate.m_isSwimming = json["is_swimming"].get<bool>();
    }
    if (json.contains("is_baby")) {
        predicate.m_isBaby = json["is_baby"].get<bool>();
    }

    predicate._updateIsAny();
    return predicate;
}

nlohmann::json EntityFlagsPredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;

    if (m_isOnFire.has_value()) {
        json["is_on_fire"] = m_isOnFire.value();
    }
    if (m_isSneaking.has_value()) {
        json["is_sneaking"] = m_isSneaking.value();
    }
    if (m_isSprinting.has_value()) {
        json["is_sprinting"] = m_isSprinting.value();
    }
    if (m_isSwimming.has_value()) {
        json["is_swimming"] = m_isSwimming.value();
    }
    if (m_isBaby.has_value()) {
        json["is_baby"] = m_isBaby.value();
    }

    return json;
}

void EntityFlagsPredicate::_updateIsAny()
{
    m_isAny = !m_isOnFire.has_value() && !m_isSneaking.has_value() && !m_isSprinting.has_value() &&
        !m_isSwimming.has_value() && !m_isBaby.has_value();
}

} // namespace mc::advancement
