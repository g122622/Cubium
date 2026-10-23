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

#include "common/mod/bedrock/addon/content/entity/events/EntityEventProgram.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/util/assert/AssertMacros.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <string_view>
#include <utility>

namespace mc::mod::bedrock::addon {
namespace {
constexpr std::size_t _MAX_EVENT_DEPTH = 64;
constexpr std::size_t _MAX_EVENT_STEPS = 4096;

Result<void> _compileAction(nlohmann::json& action,
    const nlohmann::json& events,
    const nlohmann::json& properties,
    const nlohmann::json& groups,
    EntityAddonState& validator,
    std::size_t depth)
{
    if (!action.is_object() || depth >= _MAX_EVENT_DEPTH) {
        return Error(ErrorCode::InvalidData, "Invalid entity event action or excessive nesting");
    }
    for (const auto& [key, value] : action.items()) {
        if (key != "set_property" && key != "sequence" && key != "randomize" && key != "trigger" && key != "weight" &&
            key != "add" && key != "remove") {
            // TODO: 接入统一过滤器、命令和其他事件动作。
            return Error(ErrorCode::Unsupported, "Entity event action is not implemented: " + key);
        }
    }
    for (const char* operation : {"remove", "add"}) {
        if (!action.contains(operation)) {
            continue;
        }
        const auto& change = action[operation];
        if (!change.is_object() || change.size() != 1 || !change.contains("component_groups") ||
            !change["component_groups"].is_array()) {
            return Error(ErrorCode::InvalidData, "Entity group action requires a component_groups array");
        }
        for (const auto& group : change["component_groups"]) {
            if (!group.is_string() || !groups.contains(group.get<std::string>())) {
                return Error(ErrorCode::InvalidData, "Entity group action references an unknown group");
            }
        }
    }
    if (action.contains("weight") &&
        (!action["weight"].is_number() || !std::isfinite(action["weight"].get<double>()) ||
            action["weight"].get<double>() < 0)) {
        return Error(ErrorCode::InvalidData, "Entity event weight must be finite and nonnegative");
    }
    if (action.contains("set_property")) {
        auto& values = action["set_property"];
        if (!values.is_object()) {
            return Error(ErrorCode::InvalidData, "Entity event set_property must be an object");
        }
        for (auto& [name, value] : values.items()) {
            const auto definition = properties.find(name);
            if (definition == properties.end()) {
                return Error(ErrorCode::InvalidData, "Entity event property is not declared: " + name);
            }
            const auto& type = (*definition)["type"];
            if (value.is_string() && type != "enum") {
                // TODO: 通过 Molang 计算数值和布尔属性表达式。
                return Error(ErrorCode::Unsupported, "Entity event Molang expressions are not implemented: " + name);
            }
            if (value.is_number() && (type == "int" || type == "float")) {
                double number = value.get<double>();
                if (!std::isfinite(number)) {
                    return Error(ErrorCode::InvalidData, "Entity event property must be finite: " + name);
                }
                // JSON 事件赋值将数值限制在声明范围，脚本 setProperty 则报告越界。
                double minimum = type == "int" ? static_cast<double>(std::numeric_limits<mc::i32>::min())
                                               : -static_cast<double>(std::numeric_limits<mc::f32>::max());
                double maximum = type == "int" ? static_cast<double>(std::numeric_limits<mc::i32>::max())
                                               : static_cast<double>(std::numeric_limits<mc::f32>::max());
                if (definition->contains("range")) {
                    const auto& range = (*definition)["range"];
                    minimum = range[0].get<double>();
                    maximum = range[1].get<double>();
                }
                number = std::clamp(number, minimum, maximum);
                if (type == "int") {
                    number = std::trunc(number);
                }
                value = number;
            }
            MC_TRY(validator.setProperty(name, value));
        }
    }
    for (const char* branch : {"sequence", "randomize"}) {
        if (!action.contains(branch)) {
            continue;
        }
        auto& children = action[branch];
        if (!children.is_array() || (std::string_view(branch) == "randomize" && children.empty())) {
            return Error(ErrorCode::InvalidData, "Entity event branch must be an array");
        }
        double totalWeight = 0;
        for (auto& child : children) {
            MC_TRY(_compileAction(child, events, properties, groups, validator, depth + 1));
            totalWeight += child.value("weight", 1.0);
        }
        if (std::string_view(branch) == "randomize" && (!std::isfinite(totalWeight) || totalWeight <= 0)) {
            return Error(ErrorCode::InvalidData, "Entity event randomize requires a finite positive total weight");
        }
    }
    if (action.contains("trigger")) {
        auto& trigger = action["trigger"];
        if (trigger.is_object()) {
            for (const auto& [key, value] : trigger.items()) {
                if (key != "event" && key != "target") {
                    return Error(ErrorCode::Unsupported, "Entity trigger field is not implemented: " + key);
                }
            }
            if (trigger.contains("target") && trigger["target"] != "self") {
                // TODO: 解析 other/player/target/baby/parent 事件目标。
                return Error(ErrorCode::Unsupported, "Entity trigger target other than self is not implemented");
            }
            if (!trigger.contains("event") || !trigger["event"].is_string()) {
                return Error(ErrorCode::InvalidData, "Entity trigger requires an event name");
            }
            trigger = trigger["event"].get<std::string>();
        }
        if (!trigger.is_string() || !events.contains(trigger.get<std::string>())) {
            return Error(ErrorCode::InvalidData, "Entity trigger references an unknown event");
        }
    }
    return Result<void>::ok();
}
} // namespace

Result<EntityEventProgram> EntityEventProgram::compile(
    const nlohmann::json& events, const nlohmann::json& properties, const nlohmann::json& groups)
{
    if (!events.is_object() || !groups.is_object()) {
        return Error(ErrorCode::InvalidData, "Entity events must be an object");
    }
    EntityAddonState validator;
    MC_TRY(validator.defineProperties(properties));
    EntityEventProgram result;
    result.m_events = events;
    for (auto& [name, action] : result.m_events.items()) {
        if (name.empty()) {
            return Error(ErrorCode::InvalidData, "Entity event name cannot be empty");
        }
        MC_TRY(_compileAction(action, events, properties, groups, validator, 0));
    }
    return result;
}

Result<EntityEventExecution> EntityEventProgram::execute(mc::Entity& entity, const std::string& eventId) const
{
    if (!contains(eventId)) {
        return Error(ErrorCode::NotFound, "Entity event is not defined: " + eventId);
    }
    std::vector<std::pair<std::string, nlohmann::json>> writes;
    EntityEventExecution execution;
    std::size_t steps = 0;
    std::function<Result<void>(const nlohmann::json&, std::size_t)> executeAction;
    std::function<Result<void>(const std::string&, std::size_t)> executeEvent;
    executeEvent = [&](const std::string& name, std::size_t depth) -> Result<void> {
        const auto notification = execution.notifications.size();
        execution.notifications.push_back({name, {}});
        const auto firstChange = execution.componentChanges.size();
        MC_TRY(executeAction(m_events[name], depth));
        execution.notifications[notification].componentChanges.assign(
            execution.componentChanges.begin() + static_cast<std::ptrdiff_t>(firstChange),
            execution.componentChanges.end());
        return Result<void>::ok();
    };
    executeAction = [&](const nlohmann::json& action, std::size_t depth) -> Result<void> {
        if (depth >= _MAX_EVENT_DEPTH || ++steps > _MAX_EVENT_STEPS) {
            return Error(ErrorCode::InvalidState, "Entity event execution budget exceeded");
        }
        for (const char* operation : {"remove", "add"}) {
            if (const auto change = action.find(operation); change != action.end()) {
                for (const auto& group : (*change)["component_groups"]) {
                    execution.componentChanges.push_back(
                        {std::string_view(operation) == "add", group.get<std::string>()});
                }
            }
        }
        if (const auto sequence = action.find("sequence"); sequence != action.end()) {
            for (const auto& child : *sequence) {
                MC_TRY(executeAction(child, depth + 1));
            }
        }
        if (const auto randomize = action.find("randomize"); randomize != action.end()) {
            double total = 0;
            for (const auto& child : *randomize) {
                total += child.value("weight", 1.0);
            }
            double choice = entity.getRandom().nextDouble() * total;
            const nlohmann::json* selected = nullptr;
            for (const auto& child : *randomize) {
                const double weight = child.value("weight", 1.0);
                if (weight == 0) {
                    continue;
                }
                selected = &child;
                if (choice < weight) {
                    break;
                }
                choice -= weight;
            }
            // 浮点累计舍入落在区间末端时仍由最后一个正权重分支承接。
            MC_ASSERT_RELEASE(selected != nullptr);
            MC_TRY(executeAction(*selected, depth + 1));
        }
        if (const auto trigger = action.find("trigger"); trigger != action.end()) {
            MC_TRY(executeEvent(trigger->get<std::string>(), depth + 1));
        }
        if (const auto properties = action.find("set_property"); properties != action.end()) {
            for (const auto& [name, value] : properties->items()) {
                writes.emplace_back(name, value);
            }
        }
        return Result<void>::ok();
    };
    MC_TRY(executeEvent(eventId, 0));
    auto* state = entity.tryGetComponent<EntityAddonState>();
    MC_ASSERT_RELEASE(state != nullptr);
    // 先完成整个动作计算，避免递归失败留下部分属性写入。
    for (const auto& [name, value] : writes) {
        auto written = state->setProperty(name, value);
        MC_ASSERT_RELEASE(written.success());
    }
    return execution;
}

bool EntityEventProgram::contains(const std::string& eventId) const
{
    return m_events.contains(eventId);
}

} // namespace mc::mod::bedrock::addon
