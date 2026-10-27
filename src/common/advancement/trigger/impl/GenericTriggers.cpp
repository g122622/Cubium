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

#include "GenericTriggers.hpp"

#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/CriterionContext.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <nlohmann/json.hpp>

namespace mc::advancement {

namespace {

/// 条件对象不存在（或为 null）时视为「无条件」
[[nodiscard]] bool _isAbsent(const nlohmann::json& json)
{
    return json.is_null() || !json.is_object();
}

/// 解析可选的 ContextAwarePredicate 字段
Result<ContextAwarePredicate> _parsePlayer(const nlohmann::json& json)
{
    if (_isAbsent(json) || !json.contains("player")) {
        return ContextAwarePredicate{};
    }
    return ContextAwarePredicate::fromJson(json["player"]);
}

/// 回写可选的 ContextAwarePredicate 字段
void _writePlayer(nlohmann::json& out, const ContextAwarePredicate& predicate)
{
    if (!predicate.isAny()) {
        out["player"] = predicate.toJson();
    }
}

/// 解析可选物品谓词
Result<std::optional<ItemPredicate>> _parseItem(const nlohmann::json& json, const char* key = "item")
{
    if (_isAbsent(json) || !json.contains(key)) {
        return std::optional<ItemPredicate>{};
    }
    auto parsed = ItemPredicate::fromJson(json[key]);
    if (parsed.failed()) {
        return parsed.error();
    }
    return std::optional<ItemPredicate>{std::move(parsed.value())};
}

/// 回写可选物品谓词
void _writeItem(nlohmann::json& out, const std::optional<ItemPredicate>& predicate, const char* key = "item")
{
    if (predicate.has_value()) {
        out[key] = predicate->toJson();
    }
}

/// 解析可选整数范围
[[nodiscard]] IntBounds _parseIntBounds(const nlohmann::json& json, const char* key)
{
    if (_isAbsent(json) || !json.contains(key)) {
        return IntBounds{};
    }
    return IntBounds::fromJson(json[key]);
}

/// 回写整数范围（无边界时不写）
void _writeIntBounds(nlohmann::json& out, const char* key, const IntBounds& bounds)
{
    if (!bounds.isUnbounded()) {
        out[key] = bounds.toJson();
    }
}

} // namespace

// ========== PlayerContextTriggerInstance ==========

Result<void> PlayerContextTriggerInstance::fromJson(const nlohmann::json& json)
{
    auto player = _parsePlayer(json);
    if (player.failed()) {
        return player.error();
    }
    m_player = std::move(player.value());
    return {};
}

nlohmann::json PlayerContextTriggerInstance::conditionsToJson() const
{
    nlohmann::json out = nlohmann::json::object();
    _writePlayer(out, m_player);
    return out.empty() ? nlohmann::json(nullptr) : out;
}

bool PlayerContextTriggerInstance::test(const CriterionContext& context) const
{
    return m_player.test(context);
}

// ========== ConstructBeaconTriggerInstance ==========

Result<void> ConstructBeaconTriggerInstance::fromJson(const nlohmann::json& json)
{
    auto player = _parsePlayer(json);
    if (player.failed()) {
        return player.error();
    }
    m_player = std::move(player.value());
    m_level = _parseIntBounds(json, "level");
    return {};
}

nlohmann::json ConstructBeaconTriggerInstance::conditionsToJson() const
{
    nlohmann::json out = nlohmann::json::object();
    _writePlayer(out, m_player);
    _writeIntBounds(out, "level", m_level);
    return out.empty() ? nlohmann::json(nullptr) : out;
}

bool ConstructBeaconTriggerInstance::test(i32 level, const CriterionContext& context) const
{
    return m_level.test(level) && m_player.test(context);
}

// ========== ShotCrossbowTriggerInstance ==========

Result<void> ShotCrossbowTriggerInstance::fromJson(const nlohmann::json& json)
{
    auto player = _parsePlayer(json);
    if (player.failed()) {
        return player.error();
    }
    m_player = std::move(player.value());
    auto item = _parseItem(json);
    if (item.failed()) {
        return item.error();
    }
    m_item = std::move(item.value());
    return {};
}

nlohmann::json ShotCrossbowTriggerInstance::conditionsToJson() const
{
    nlohmann::json out = nlohmann::json::object();
    _writePlayer(out, m_player);
    _writeItem(out, m_item);
    return out.empty() ? nlohmann::json(nullptr) : out;
}

bool ShotCrossbowTriggerInstance::test(const mc::ItemStack& crossbow, const CriterionContext& context) const
{
    if (!m_player.test(context)) {
        return false;
    }
    return !m_item.has_value() || m_item->test(crossbow);
}

// ========== SpearMobsTriggerInstance ==========

Result<void> SpearMobsTriggerInstance::fromJson(const nlohmann::json& json)
{
    auto player = _parsePlayer(json);
    if (player.failed()) {
        return player.error();
    }
    m_player = std::move(player.value());
    m_count = _parseIntBounds(json, "count");
    return {};
}

nlohmann::json SpearMobsTriggerInstance::conditionsToJson() const
{
    nlohmann::json out = nlohmann::json::object();
    _writePlayer(out, m_player);
    _writeIntBounds(out, "count", m_count);
    return out.empty() ? nlohmann::json(nullptr) : out;
}

bool SpearMobsTriggerInstance::test(i32 count, const CriterionContext& context) const
{
    return m_count.test(count) && m_player.test(context);
}

// ========== TargetHitTriggerInstance ==========

Result<void> TargetHitTriggerInstance::fromJson(const nlohmann::json& json)
{
    auto player = _parsePlayer(json);
    if (player.failed()) {
        return player.error();
    }
    m_player = std::move(player.value());

    if (!_isAbsent(json) && json.contains("projectile")) {
        auto projectile = ContextAwarePredicate::fromJson(json["projectile"]);
        if (projectile.failed()) {
            return projectile.error();
        }
        m_projectile = std::move(projectile.value());
    }
    m_signalStrength = _parseIntBounds(json, "signal_strength");
    return {};
}

nlohmann::json TargetHitTriggerInstance::conditionsToJson() const
{
    nlohmann::json out = nlohmann::json::object();
    _writePlayer(out, m_player);
    if (!m_projectile.isAny()) {
        out["projectile"] = m_projectile.toJson();
    }
    _writeIntBounds(out, "signal_strength", m_signalStrength);
    return out.empty() ? nlohmann::json(nullptr) : out;
}

bool TargetHitTriggerInstance::test(
    i32 signalStrength, const CriterionContext& player, const CriterionContext& projectile) const
{
    return m_signalStrength.test(signalStrength) && m_player.test(player) && m_projectile.test(projectile);
}

// ========== AllayDropItemOnBlockTriggerInstance ==========

Result<void> AllayDropItemOnBlockTriggerInstance::fromJson(const nlohmann::json& json)
{
    auto player = _parsePlayer(json);
    if (player.failed()) {
        return player.error();
    }
    m_player = std::move(player.value());

    if (!_isAbsent(json) && json.contains("location")) {
        auto location = ContextAwarePredicate::fromJson(json["location"]);
        if (location.failed()) {
            return location.error();
        }
        m_location = std::move(location.value());
    }

    auto item = _parseItem(json);
    if (item.failed()) {
        return item.error();
    }
    m_item = std::move(item.value());
    return {};
}

nlohmann::json AllayDropItemOnBlockTriggerInstance::conditionsToJson() const
{
    nlohmann::json out = nlohmann::json::object();
    _writePlayer(out, m_player);
    if (!m_location.isAny()) {
        out["location"] = m_location.toJson();
    }
    _writeItem(out, m_item);
    return out.empty() ? nlohmann::json(nullptr) : out;
}

bool AllayDropItemOnBlockTriggerInstance::test(
    const mc::ItemStack& dropped, const CriterionContext& location, const CriterionContext& player) const
{
    if (!m_player.test(player) || !m_location.test(location)) {
        return false;
    }
    return !m_item.has_value() || m_item->test(dropped);
}

// ========== CrafterRecipeCraftedTriggerInstance ==========

Result<void> CrafterRecipeCraftedTriggerInstance::fromJson(const nlohmann::json& json)
{
    auto player = _parsePlayer(json);
    if (player.failed()) {
        return player.error();
    }
    m_player = std::move(player.value());

    if (!_isAbsent(json) && json.contains("recipe_id")) {
        m_recipeId = ResourceLocation::parse(json["recipe_id"].get<std::string>());
    }
    return {};
}

nlohmann::json CrafterRecipeCraftedTriggerInstance::conditionsToJson() const
{
    nlohmann::json out = nlohmann::json::object();
    _writePlayer(out, m_player);
    if (m_recipeId.has_value()) {
        out["recipe_id"] = m_recipeId->toString();
    }
    return out.empty() ? nlohmann::json(nullptr) : out;
}

bool CrafterRecipeCraftedTriggerInstance::test(const ResourceLocation& recipeId, const CriterionContext& context) const
{
    if (!m_player.test(context)) {
        return false;
    }
    return !m_recipeId.has_value() || *m_recipeId == recipeId;
}

// ========== KillMobNearSculkCatalystTriggerInstance ==========

Result<void> KillMobNearSculkCatalystTriggerInstance::fromJson(const nlohmann::json& json)
{
    auto player = _parsePlayer(json);
    if (player.failed()) {
        return player.error();
    }
    m_player = std::move(player.value());
    return {};
}

nlohmann::json KillMobNearSculkCatalystTriggerInstance::conditionsToJson() const
{
    nlohmann::json out = nlohmann::json::object();
    _writePlayer(out, m_player);
    return out.empty() ? nlohmann::json(nullptr) : out;
}

bool KillMobNearSculkCatalystTriggerInstance::test(const CriterionContext& context) const
{
    return m_player.test(context);
}

} // namespace mc::advancement
