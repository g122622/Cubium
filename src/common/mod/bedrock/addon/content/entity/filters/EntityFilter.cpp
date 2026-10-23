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

#include "common/mod/bedrock/addon/content/entity/filters/EntityFilter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/mod/bedrock/addon/content/base/damage/BedrockDamageCause.hpp"
#include "common/mod/bedrock/addon/content/entity/filters/base/VanillaEntityFamilies.hpp"
#include "common/mod/bedrock/addon/content/entity/query/EntityStateQuery.hpp"
#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <functional>
#include <limits>
#include <set>
#include <string_view>

namespace mc::mod::bedrock::addon {
namespace {
constexpr std::size_t _MAX_DEPTH = 64;
constexpr std::size_t _MAX_NODES = 4096;

Result<void> _numeric(nlohmann::json& value, bool integer)
{
    if (value.is_string()) {
        const auto& source = value.get_ref<const std::string&>();
        double number = 0;
        const auto parsed = std::from_chars(source.data(), source.data() + source.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != source.data() + source.size()) {
            return Error(ErrorCode::InvalidData, "Entity filter numeric string is invalid");
        }
        value = number;
    }
    if (!value.is_number() || !std::isfinite(value.get<double>()) ||
        (integer && std::trunc(value.get<double>()) != value.get<double>())) {
        return Error(ErrorCode::InvalidData, "Entity filter requires a finite compatible number");
    }
    if (!integer) {
        const double number = value.get<double>();
        if (std::abs(number) > std::numeric_limits<f32>::max()) {
            return Error(ErrorCode::InvalidData, "Entity filter value exceeds float range");
        }
        value = static_cast<f32>(number);
    }
    return Result<void>::ok();
}

Result<void> _validate(nlohmann::json& filter, std::size_t depth, std::size_t& count)
{
    if (depth >= _MAX_DEPTH || ++count > _MAX_NODES) {
        return Error(ErrorCode::InvalidData, "Invalid entity filter or excessive nesting");
    }
    if (filter.is_array()) {
        if (filter.size() != 1) {
            // TODO: 核实原版对隐式多元素数组的组合语义，不能擅自替换为 all_of 或 any_of。
            return Error(ErrorCode::Unsupported, "Implicit entity filter arrays require exactly one condition");
        }
        // 单元素包装不改变逻辑；参考行为包使用这种形式包装环境条件。
        nlohmann::json condition = filter.front();
        filter = std::move(condition);
        return _validate(filter, depth + 1, count);
    }
    if (!filter.is_object()) {
        return Error(ErrorCode::InvalidData, "Entity filter must be an object");
    }
    for (const auto& [key, value] : filter.items()) {
        if (key != "all_of" && key != "any_of" && key != "none_of" && key != "test" && key != "subject" &&
            key != "operator" && key != "domain" && key != "value") {
            return Error(ErrorCode::Unsupported, "Entity filter field is not implemented: " + key);
        }
    }
    bool collection = false;
    for (const char* operation : {"all_of", "any_of", "none_of"}) {
        if (auto children = filter.find(operation); children != filter.end()) {
            collection = true;
            if (!children->is_array()) {
                return Error(ErrorCode::InvalidData, "Entity filter collection must be an array");
            }
            for (auto& child : *children) {
                MC_TRY(_validate(child, depth + 1, count));
            }
        }
    }
    if (!filter.contains("test")) {
        if (!filter.empty() && !collection) {
            return Error(ErrorCode::InvalidData, "Entity filter requires a test or collection");
        }
        for (const char* field : {"subject", "operator", "domain", "value"}) {
            if (filter.contains(field)) {
                return Error(ErrorCode::InvalidData, "Entity filter leaf fields require a test");
            }
        }
        return Result<void>::ok();
    }
    if (!filter["test"].is_string()) {
        return Error(ErrorCode::InvalidData, "Entity filter test must be a string");
    }
    const auto test = filter["test"].get<std::string>();
    static const std::set<std::string> supported = {"int_property",
        "float_property",
        "bool_property",
        "enum_property",
        "has_property",
        "is_family",
        "is_type",
        "has_tag",
        "has_component",
        "is_alive",
        "is_sneaking",
        "on_ground",
        "in_water",
        "in_lava",
        "in_water_or_rain",
        "has_target",
        "has_equipment",
        "is_owner",
        "has_container_open",
        "is_variant",
        "is_mark_variant",
        "has_damage",
        "is_daytime",
        "weather",
        "distance_to_nearest_player"};
    if (!supported.contains(test)) {
        // TODO: 接入生物群系温度等过滤测试。
        return Error(ErrorCode::Unsupported, "Entity filter test is not implemented: " + test);
    }
    if (!filter.contains("subject")) {
        filter["subject"] = "self";
    }
    if (!filter["subject"].is_string()) {
        return Error(ErrorCode::InvalidData, "Entity filter subject must be a string");
    }
    const auto subject = filter["subject"].get<std::string>();
    if (subject != "self" && subject != "other" && subject != "player" && subject != "parent" && subject != "damager" &&
        subject != "target") {
        // TODO: 为方块交互过滤提供 block 主体。
        return Error(ErrorCode::Unsupported, "Entity filter subject is not implemented: " + subject);
    }
    if (!filter.contains("operator")) {
        filter["operator"] = "==";
    }
    if (!filter["operator"].is_string()) {
        return Error(ErrorCode::InvalidData, "Entity filter operator must be a string");
    }
    auto operation = filter["operator"].get<std::string>();
    if (operation == "equals" || operation == "=") {
        operation = "==";
    }
    if (operation == "not" || operation == "<>") {
        operation = "!=";
    }
    if (operation != "==" && operation != "!=" && operation != "<" && operation != "<=" && operation != ">" &&
        operation != ">=") {
        return Error(ErrorCode::InvalidData, "Unknown entity filter comparison operator: " + operation);
    }
    filter["operator"] = operation;
    const bool boolean = test == "bool_property" || test == "is_alive" || test == "is_sneaking" ||
        test == "has_target" || test == "is_daytime" || test == "is_owner" || test == "has_container_open" ||
        test == "on_ground" || test == "in_water" || test == "in_lava" || test == "in_water_or_rain";
    if (!filter.contains("value") && boolean) {
        filter["value"] = true;
    }
    if (!filter.contains("value")) {
        return Error(ErrorCode::InvalidData, "Entity filter value is required");
    }
    auto& value = filter["value"];
    if (boolean) {
        if (value == "true") {
            value = true;
        }
        if (value == "false") {
            value = false;
        }
        if (!value.is_boolean()) {
            return Error(ErrorCode::InvalidData, "Entity filter value must be boolean");
        }
    } else if (test == "int_property" || test == "float_property" || test == "distance_to_nearest_player" ||
        test == "is_variant" || test == "is_mark_variant") {
        const bool integer = test == "int_property" || test == "is_variant" || test == "is_mark_variant";
        MC_TRY(_numeric(value, integer));
        if (integer &&
            (value.get<double>() < std::numeric_limits<i32>::min() ||
                value.get<double>() > std::numeric_limits<i32>::max())) {
            return Error(ErrorCode::InvalidData, "Entity integer filter value exceeds int32 range");
        }
        if (test == "distance_to_nearest_player" && value.get<double>() < 0) {
            return Error(ErrorCode::InvalidData, "Entity filter distance cannot be negative");
        }
    } else if (!value.is_string() || value.get_ref<const std::string&>().empty()) {
        return Error(ErrorCode::InvalidData, "Entity filter value must be a nonempty string");
    }
    if (test.ends_with("_property") && test != "has_property") {
        if (!filter.contains("domain") || !filter["domain"].is_string() ||
            filter["domain"].get_ref<const std::string&>().empty()) {
            return Error(ErrorCode::InvalidData, "Entity property filter requires a domain");
        }
    } else if (test == "has_equipment") {
        if (!filter.contains("domain")) {
            filter["domain"] = "any";
        }
        static const std::set<std::string> domains = {
            "any", "armor", "body", "feet", "hand", "head", "inventory", "leg", "main_hand", "torso"};
        if (!filter["domain"].is_string() || !domains.contains(filter["domain"].get<std::string>())) {
            return Error(ErrorCode::InvalidData, "Unknown entity equipment filter domain");
        }
        const auto item = mc::ResourceLocation::parse(value.get<std::string>());
        if (!item.isValid()) {
            return Error(ErrorCode::InvalidData, "Invalid entity equipment filter item identifier");
        }
        value = item.toString();
    } else if (filter.contains("domain")) {
        return Error(ErrorCode::Unsupported, "Entity filter domain is not implemented for: " + test);
    }
    if (test == "is_type") {
        if (operation != "==" && operation != "!=") {
            return Error(ErrorCode::Unsupported, "Ordered comparison of entity type names is not implemented");
        }
        const auto identifier = mc::ResourceLocation::parse(value.get<std::string>());
        if (!identifier.isValid()) {
            return Error(ErrorCode::InvalidData, "Invalid entity filter type identifier");
        }
        value = identifier.toString();
    }
    if (test == "has_component") {
        auto name = value.get<std::string>();
        if (name.find(':') == std::string::npos) {
            name = "minecraft:" + name;
        }
        value = std::move(name);
    }
    if (test == "weather" && value != "clear" && value != "precipitation" && value != "thunderstorm") {
        return Error(ErrorCode::Unsupported, "Entity weather filter value is not implemented");
    }
    if (test == "has_damage") {
        static const std::set<std::string> causes = {"anvil",
            "attack",
            "block_explosion",
            "contact",
            "drowning",
            "entity_explosion",
            "fall",
            "falling_block",
            "fatal",
            "fire",
            "fire_tick",
            "fly_into_wall",
            "lava",
            "magic",
            "none",
            "override",
            "piston",
            "projectile",
            "self_destruct",
            "sonic_boom",
            "stalactite",
            "stalagmite",
            "starve",
            "suffocation",
            "thorns",
            "void",
            "wither"};
        if (!causes.contains(value.get<std::string>())) {
            return Error(ErrorCode::InvalidData, "Unknown entity damage filter cause");
        }
    }
    return Result<void>::ok();
}

Result<bool> _compare(const nlohmann::json& actual, const nlohmann::json& expected, const std::string& operation)
{
    if (operation == "==") {
        return actual == expected;
    }
    if (operation == "!=") {
        return actual != expected;
    }
    const bool numbers = actual.is_number() && expected.is_number();
    const bool booleans = actual.is_boolean() && expected.is_boolean();
    if (!numbers && !booleans) {
        return Error(ErrorCode::Unsupported, "Ordered entity filter comparison requires numeric values");
    }
    const double a = booleans ? (actual.get<bool>() ? 1 : 0) : actual.get<double>();
    const double b = booleans ? (expected.get<bool>() ? 1 : 0) : expected.get<double>();
    if (operation == "<") {
        return a < b;
    }
    if (operation == "<=") {
        return a <= b;
    }
    if (operation == ">") {
        return a > b;
    }
    return a >= b;
}

const mc::Entity* _subject(const EntityFilterContext& context, const std::string& subject)
{
    if (subject == "self") {
        return &context.self;
    }
    if (subject == "other") {
        return context.other;
    }
    if (subject == "player") {
        return context.player;
    }
    if (subject == "parent") {
        return context.parent;
    }
    if (subject == "damager") {
        return context.damager;
    }
    if (context.target != nullptr) {
        return context.target;
    }
    const auto* mob = dynamic_cast<const mc::MobEntity*>(&context.self);
    return mob != nullptr ? mob->attackTarget() : nullptr;
}

Result<bool> _family(const mc::Entity& entity, const std::string& family)
{
    if (const auto* components = entity.tryGetComponent<AddonEntityComponents>()) {
        const auto* value = components->component("minecraft:type_family");
        if (value == nullptr || !value->contains("family")) {
            return false;
        }
        const auto& names = (*value)["family"];
        return names.is_string() ? names == family : std::find(names.begin(), names.end(), family) != names.end();
    }
    if (const auto matched = vanillaEntityFamilyMatch(entity.getTypeId(), family)) {
        return *matched;
    }
    if (family == "player") {
        return dynamic_cast<const mc::Player*>(&entity) != nullptr;
    }
    if (family == "mob") {
        return dynamic_cast<const mc::MobEntity*>(&entity) != nullptr;
    }
    // TODO: 接入剩余类型及动态组件组家族，并提供基岩与 Java 类型标识转换。
    return Error(ErrorCode::Unsupported, "Native Bedrock family mapping is not implemented: " + family);
}

Result<bool> _test(const nlohmann::json& filter, const EntityFilterContext& context)
{
    const auto* entity = _subject(context, filter["subject"].get<std::string>());
    if (entity == nullptr || entity->isRemoved()) {
        return false;
    }
    const auto test = filter["test"].get<std::string>();
    const auto operation = filter["operator"].get<std::string>();
    const auto& expected = filter["value"];
    const auto* state = entity->tryGetComponent<EntityAddonState>();
    if (test == "has_property") {
        return _compare(
            state != nullptr && state->getProperty(expected.get<std::string>()).has_value(), true, operation);
    }
    if (test.ends_with("_property")) {
        const auto name = filter["domain"].get<std::string>();
        const auto* definition = state != nullptr ? state->getPropertyDefinition(name) : nullptr;
        if (definition == nullptr || (*definition)["type"] != test.substr(0, test.size() - 9)) {
            return false;
        }
        const auto actual = state->getProperty(name);
        if (!actual) {
            return false;
        }
        if (test == "enum_property" && operation != "==" && operation != "!=") {
            const auto& values = (*definition)["values"];
            const auto first = std::find(values.begin(), values.end(), *actual);
            const auto second = std::find(values.begin(), values.end(), expected);
            if (first == values.end() || second == values.end()) {
                return false;
            }
            return _compare(std::distance(values.begin(), first), std::distance(values.begin(), second), operation);
        }
        return _compare(*actual, expected, operation);
    }
    if (test == "is_family") {
        auto actual = _family(*entity, expected.get<std::string>());
        if (actual.failed()) {
            return actual.error();
        }
        return _compare(actual.value(), true, operation);
    }
    if (test == "has_component") {
        auto actual = EntityStateQuery::hasComponent(*entity, expected.get<std::string>());
        if (actual.failed()) {
            return actual.error();
        }
        return _compare(actual.value(), true, operation);
    }
    if (test == "is_type") {
        return _compare(entity->getTypeId(), expected, operation);
    }
    if (test == "has_tag") {
        return _compare(entity->hasTag(expected.get<std::string>()), true, operation);
    }
    if (test == "is_alive") {
        return _compare(entity->isAlive(), expected, operation);
    }
    if (test == "has_damage") {
        if (context.damage == nullptr || context.damage->victim != entity) {
            return false;
        }
        const bool actual = expected == "fatal"
            ? context.damage->fatal
            : damageFilterCauseName(context.damage->type) == expected.get<std::string>();
        return _compare(actual, true, operation);
    }
    if (test == "is_sneaking") {
        return _compare(entity->isSneaking(), expected, operation);
    }
    if (test == "on_ground") {
        return _compare(entity->onGround(), expected, operation);
    }
    if (test == "in_water") {
        return _compare(entity->isInWater(), expected, operation);
    }
    if (test == "in_lava") {
        return _compare(entity->isInLava(), expected, operation);
    }
    if (test == "in_water_or_rain") {
        return _compare(entity->isWet(), expected, operation);
    }
    if (test == "has_target") {
        const auto* mob = dynamic_cast<const mc::MobEntity*>(entity);
        return _compare(mob != nullptr && mob->attackTarget() != nullptr, expected, operation);
    }
    if (test == "has_equipment") {
        auto actual =
            EntityStateQuery::hasEquipment(*entity, filter["domain"].get<std::string>(), expected.get<std::string>());
        if (actual.failed()) {
            return actual.error();
        }
        return _compare(actual.value(), true, operation);
    }
    if (test == "is_owner") {
        return _compare(EntityStateQuery::isOwner(context.self, *entity), expected, operation);
    }
    if (test == "has_container_open") {
        const auto* player = dynamic_cast<const mc::Player*>(entity);
        return _compare(player != nullptr && player->openContainerMenu() != nullptr, expected, operation);
    }
    if (test == "is_variant" || test == "is_mark_variant") {
        auto actual =
            EntityStateQuery::variant(*entity, test == "is_variant" ? "minecraft:variant" : "minecraft:mark_variant");
        if (actual.failed()) {
            return actual.error();
        }
        return _compare(actual.value(), expected, operation);
    }
    const auto* world = entity->world();
    if (world == nullptr) {
        return false;
    }
    if (test == "is_daytime") {
        return _compare(world->isDaytime(), expected, operation);
    }
    if (test == "weather") {
        const bool matches = expected == "clear" ? !world->isRaining() && !world->isThundering()
            : expected == "precipitation"        ? world->isRaining()
                                                 : world->isThundering();
        return _compare(matches, true, operation);
    }
    double nearest = std::numeric_limits<f32>::max();
    for (const auto* player : world->getPlayers()) {
        if (player->isRemoved()) {
            continue;
        }
        const auto delta = player->position() - entity->position();
        nearest = std::min(nearest,
            std::sqrt(static_cast<double>(delta.x) * delta.x + static_cast<double>(delta.y) * delta.y +
                static_cast<double>(delta.z) * delta.z));
    }
    return _compare(static_cast<f32>(nearest), expected, operation);
}
} // namespace

Result<EntityFilter> EntityFilter::compile(const nlohmann::json& filter)
{
    EntityFilter result;
    result.m_filter = filter;
    std::size_t count = 0;
    MC_TRY(_validate(result.m_filter, 0, count));
    return result;
}

Result<bool> EntityFilter::matches(const EntityFilterContext& context) const
{
    std::function<Result<bool>(const nlohmann::json&)> evaluate;
    evaluate = [&](const nlohmann::json& filter) -> Result<bool> {
        for (const char* operation : {"all_of", "any_of", "none_of"}) {
            const auto children = filter.find(operation);
            if (children == filter.end()) {
                continue;
            }
            bool any = false;
            for (const auto& child : *children) {
                auto value = evaluate(child);
                if (value.failed()) {
                    return value.error();
                }
                if (std::string_view(operation) == "all_of" && !value.value()) {
                    return false;
                }
                if (std::string_view(operation) == "none_of" && value.value()) {
                    return false;
                }
                if (std::string_view(operation) == "any_of" && value.value()) {
                    any = true;
                    break;
                }
            }
            if (std::string_view(operation) == "any_of" && !any) {
                return false;
            }
        }
        return filter.contains("test") ? _test(filter, context) : Result<bool>(true);
    };
    return evaluate(m_filter);
}
} // namespace mc::mod::bedrock::addon
