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

#include "DamagePredicate.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"

#include <utility>
#include <nlohmann/json.hpp>

namespace mc::advancement {

DamagePredicate::DamagePredicate(const DamagePredicate& other)
    : m_dealt(other.m_dealt)
    , m_taken(other.m_taken)
    , m_sourceEntity(other.m_sourceEntity.has_value() ? std::make_optional(*other.m_sourceEntity) : std::nullopt)
    , m_blocked(other.m_blocked)
    , m_type(other.m_type)
    , m_isAny(other.m_isAny)
{}

DamagePredicate& DamagePredicate::operator=(const DamagePredicate& other)
{
    if (this == &other) {
        return *this;
    }
    m_dealt = other.m_dealt;
    m_taken = other.m_taken;
    m_sourceEntity = other.m_sourceEntity.has_value() ? std::make_optional(*other.m_sourceEntity) : std::nullopt;
    m_blocked = other.m_blocked;
    m_type = other.m_type;
    m_isAny = other.m_isAny;
    return *this;
}

bool DamagePredicate::test(const DamageSource& source, f32 dealt, f32 taken, bool blocked) const
{
    if (m_isAny) {
        return true;
    }

    // 对齐 vanilla DamagePredicate.matches：dealt → taken → source_entity → blocked → type
    if (!m_dealt.test(static_cast<f64>(dealt))) {
        return false;
    }
    if (!m_taken.test(static_cast<f64>(taken))) {
        return false;
    }
    if (m_sourceEntity.has_value()) {
        const Entity* causingEntity = source.getTrueSource();
        if (causingEntity == nullptr || !m_sourceEntity->test(*causingEntity)) {
            return false;
        }
    }
    if (m_blocked.has_value() && m_blocked.value() != blocked) {
        return false;
    }
    if (m_type.has_value() && !m_type->test(source)) {
        return false;
    }
    return true;
}

Result<DamagePredicate> DamagePredicate::fromJson(const nlohmann::json& json)
{
    DamagePredicate predicate;
    if (json.is_null()) {
        return predicate;
    }
    if (!json.is_object()) {
        return Error::invalidArgument("DamagePredicate must be a JSON object");
    }

    if (json.contains("dealt")) {
        predicate.m_dealt = MinMaxBounds<f64>::fromJson(json["dealt"]);
    }
    if (json.contains("taken")) {
        predicate.m_taken = MinMaxBounds<f64>::fromJson(json["taken"]);
    }
    if (json.contains("source_entity")) {
        auto result = EntityPredicate::fromJson(json["source_entity"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_sourceEntity = std::move(result.value());
    }
    if (json.contains("blocked") && json["blocked"].is_boolean()) {
        predicate.m_blocked = json["blocked"].get<bool>();
    }
    if (json.contains("type")) {
        auto result = DamageSourcePredicate::fromJson(json["type"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_type = std::move(result.value());
    }

    predicate._updateIsAny();
    return predicate;
}

nlohmann::json DamagePredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json = nlohmann::json::object();
    if (!m_dealt.isUnbounded()) {
        json["dealt"] = m_dealt.toJson();
    }
    if (!m_taken.isUnbounded()) {
        json["taken"] = m_taken.toJson();
    }
    if (m_sourceEntity.has_value()) {
        json["source_entity"] = m_sourceEntity->toJson();
    }
    if (m_blocked.has_value()) {
        json["blocked"] = m_blocked.value();
    }
    if (m_type.has_value()) {
        json["type"] = m_type->toJson();
    }
    return json;
}

void DamagePredicate::_updateIsAny()
{
    m_isAny = m_dealt.isUnbounded() && m_taken.isUnbounded() && !m_sourceEntity.has_value() && !m_blocked.has_value() &&
        !m_type.has_value();
}

} // namespace mc::advancement