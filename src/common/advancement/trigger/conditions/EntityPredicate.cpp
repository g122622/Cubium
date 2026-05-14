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

#include "EntityPredicate.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== MobEffectsPredicate ==========

bool MobEffectsPredicate::test(const Entity& entity) const
{
    if (m_isAny) {
        return true;
    }
    // [TODO 阶段3+4：触发器完善] 检查效果需要实体效果系统支持
    return true;
}

Result<MobEffectsPredicate> MobEffectsPredicate::fromJson(const nlohmann::json& json)
{
    MC_UNUSED(json);
    return MobEffectsPredicate{};
}

nlohmann::json MobEffectsPredicate::toJson() const
{
    return nullptr;
}

// ========== EntityPredicate ==========

bool EntityPredicate::test(const Entity& entity) const
{
    if (m_isAny) {
        return true;
    }

    // 检查实体类型
    if (m_type.has_value()) {
        // [TODO 阶段3+4：触发器完善] 需要实体类型注册表支持获取实体类型ID比较
        // if (entity.getType().getId() != m_type.value()) return false;
    }

    // [TODO 阶段3+4：触发器完善] 检查其他条件（距离、位置、效果、NBT等）
    return true;
}

bool EntityPredicate::test(const Entity& entity, const DamageSource& source) const
{
    MC_UNUSED(source);
    return test(entity);
}

Result<EntityPredicate> EntityPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return EntityPredicate{};
    }

    std::optional<ResourceLocation> type;

    if (json.contains("type")) {
        type = ResourceLocation(json["type"].get<std::string>());
    }

    // [TODO 阶段3+4：触发器完善] 解析其他条件（距离、位置、效果、NBT等）

    EntityPredicate predicate;
    predicate.m_type = std::move(type);
    predicate.m_isAny = !predicate.m_type.has_value();
    return predicate;
}

nlohmann::json EntityPredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_type.has_value()) {
        json["type"] = m_type.value().toString();
    }
    return json;
}

// ========== DamageSourcePredicate ==========

bool DamageSourcePredicate::test(const DamageSource& source) const
{
    if (m_isAny) {
        return true;
    }
    // [TODO 阶段3+4：触发器完善] 检查伤害源需要伤害系统支持
    MC_UNUSED(source);
    return true;
}

Result<DamageSourcePredicate> DamageSourcePredicate::fromJson(const nlohmann::json& json)
{
    MC_UNUSED(json);
    return DamageSourcePredicate{};
}

nlohmann::json DamageSourcePredicate::toJson() const
{
    return nullptr;
}

} // namespace mc::advancement
