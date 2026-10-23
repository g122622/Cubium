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

#include "common/mod/bedrock/addon/content/entity/state/timer/EntityTimerState.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mc::mod::bedrock::addon {
namespace {
Result<void> _seconds(const nlohmann::json& value)
{
    if (!value.is_number() || !std::isfinite(value.get<double>()) || value.get<double>() < 0 ||
        value.get<double>() > static_cast<double>(std::numeric_limits<i32>::max()) / mc::game::TICKS_PER_SECOND) {
        return Error(ErrorCode::InvalidData, "Timer duration must be finite, nonnegative and within tick range");
    }
    const f32 ticks = value.get<f32>() * static_cast<f32>(mc::game::TICKS_PER_SECOND);
    if (!std::isfinite(ticks) || static_cast<double>(ticks) > std::numeric_limits<i32>::max()) {
        return Error(ErrorCode::InvalidData, "Timer duration exceeds its tick storage range");
    }
    return Result<void>::ok();
}

i64 _ticks(f32 seconds)
{
    // 定义按单精度秒数换算，零时长同样只在实体 tick 中触发一次。
    return std::max<i64>(1, static_cast<i64>(seconds * static_cast<f32>(mc::game::TICKS_PER_SECOND)));
}
} // namespace

Result<void> EntityTimerState::validateDefinition(const nlohmann::json& definition)
{
    if (!definition.is_object()) {
        return Error(ErrorCode::InvalidData, "Entity timer must be an object");
    }
    for (const auto& [name, value] : definition.items()) {
        if (name != "looping" && name != "randomInterval" && name != "time" && name != "random_time_choices" &&
            name != "time_down_event") {
            return Error(ErrorCode::Unsupported, "Entity timer field is not implemented: " + name);
        }
    }
    for (const char* flag : {"looping", "randomInterval"}) {
        if (definition.contains(flag) && !definition[flag].is_boolean()) {
            return Error(ErrorCode::InvalidData, "Entity timer flags must be boolean");
        }
    }
    if (definition.contains("time") == definition.contains("random_time_choices")) {
        return Error(ErrorCode::InvalidData, "Entity timer requires exactly one time specification");
    }
    if (definition.contains("time")) {
        const auto& time = definition["time"];
        if (time.is_array()) {
            if (time.size() != 2) {
                return Error(ErrorCode::InvalidData, "Timer time range requires two bounds");
            }
            MC_TRY(_seconds(time[0]));
            MC_TRY(_seconds(time[1]));
            if (time[0].get<double>() > time[1].get<double>()) {
                return Error(ErrorCode::InvalidData, "Timer time bounds must be ordered");
            }
        } else {
            MC_TRY(_seconds(time));
        }
    } else {
        const auto& choices = definition["random_time_choices"];
        if (!choices.is_array() || choices.empty()) {
            return Error(ErrorCode::InvalidData, "Timer random choices must be nonempty");
        }
        double total = 0;
        for (const auto& choice : choices) {
            if (!choice.is_object() || !choice.contains("value")) {
                return Error(ErrorCode::InvalidData, "Timer choice requires a value");
            }
            for (const auto& [name, value] : choice.items()) {
                if (name != "value" && name != "weight") {
                    return Error(ErrorCode::InvalidData, "Unknown timer choice field: " + name);
                }
            }
            MC_TRY(_seconds(choice["value"]));
            if (choice.contains("weight") &&
                (!choice["weight"].is_number() || !std::isfinite(choice["weight"].get<double>()) ||
                    choice["weight"].get<double>() < 0)) {
                return Error(ErrorCode::InvalidData, "Timer choice weight must be finite and nonnegative");
            }
            total += choice.value("weight", 1.0);
        }
        if (!std::isfinite(total) || total <= 0) {
            return Error(ErrorCode::InvalidData, "Timer choices require positive total weight");
        }
    }
    if (!definition.contains("time_down_event")) {
        return Error(ErrorCode::InvalidData, "Timer requires a time_down_event");
    }
    auto trigger = EntityEventTrigger::compile(definition["time_down_event"]);
    if (trigger.failed()) {
        return trigger.error();
    }
    return Result<void>::ok();
}

Result<EntityTimerState> EntityTimerState::create(const nlohmann::json& definition, mc::math::IRandom& random)
{
    MC_TRY(validateDefinition(definition));
    EntityTimerState state;
    state.m_definition = definition;
    auto trigger = EntityEventTrigger::compile(definition["time_down_event"]);
    if (trigger.failed()) {
        return trigger.error();
    }
    state.m_trigger = std::move(trigger.value());
    state.m_interval = state._chooseInterval(random);
    state.m_remaining = state.m_interval;
    state.m_running = true;
    return state;
}

i64 EntityTimerState::_chooseInterval(mc::math::IRandom& random) const
{
    if (const auto time = m_definition.find("time"); time != m_definition.end()) {
        return _ticks(
            time->is_array() ? random.nextFloat((*time)[0].get<f32>(), (*time)[1].get<f32>()) : time->get<f32>());
    }
    const auto& choices = m_definition["random_time_choices"];
    double total = 0;
    for (const auto& choice : choices) {
        total += choice.value("weight", 1.0);
    }
    double selected = random.nextDouble() * total;
    const nlohmann::json* last = nullptr;
    for (const auto& choice : choices) {
        const double weight = choice.value("weight", 1.0);
        if (weight == 0) {
            continue;
        }
        last = &choice;
        if (selected < weight) {
            break;
        }
        selected -= weight;
    }
    MC_ASSERT_RELEASE(last != nullptr);
    return _ticks((*last)["value"].get<f32>());
}

Result<void> EntityTimerState::tick(mc::Entity& entity)
{
    if (!m_running) {
        return Result<void>::ok();
    }
    --m_remaining;
    if (m_remaining > 0) {
        return Result<void>::ok();
    }
    if (m_definition.value("looping", true)) {
        // 固定间隔重复使用本次组件激活时选出的时长，随机间隔则在每轮重新选择。
        if (m_definition.value("randomInterval", true)) {
            m_interval = _chooseInterval(entity.getRandom());
        }
        m_remaining = m_interval;
    } else {
        m_running = false;
    }
    return m_trigger.fire(entity);
}

nlohmann::json EntityTimerState::save() const
{
    return {{"state", m_running ? "running" : "stopped"}, {"remaining", m_remaining}, {"interval", m_interval}};
}

Result<void> EntityTimerState::validateSavedState(const nlohmann::json& saved)
{
    if (!saved.is_object() || !saved.contains("state") || !saved["state"].is_string() || !saved.contains("remaining") ||
        !saved["remaining"].is_number_integer() || !saved.contains("interval") ||
        !saved["interval"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "Invalid saved entity timer state");
    }
    const auto state = saved["state"].get<std::string>();
    const auto remaining = saved["remaining"].get<double>();
    const auto interval = saved["interval"].get<double>();
    if ((state != "running" && state != "stopped") || interval < 1 || interval > std::numeric_limits<i32>::max() ||
        remaining < 0 || remaining > interval || (state == "running" && remaining < 1) ||
        (state == "stopped" && remaining != 0)) {
        return Error(ErrorCode::InvalidData, "Saved entity timer counters are inconsistent");
    }
    return Result<void>::ok();
}

Result<void> EntityTimerState::load(const nlohmann::json& saved)
{
    MC_TRY(validateSavedState(saved));
    m_running = saved["state"] == "running";
    m_remaining = saved["remaining"].get<i64>();
    m_interval = saved["interval"].get<i64>();
    return Result<void>::ok();
}
} // namespace mc::mod::bedrock::addon
