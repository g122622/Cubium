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

#include "common/mod/bedrock/addon/content/entity/state/sensor/EntityEnvironmentSensor.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventProgram.hpp"

namespace mc::mod::bedrock::addon {
Result<EntityEnvironmentSensor> EntityEnvironmentSensor::compile(const nlohmann::json& definition)
{
    if (!definition.is_object()) {
        return Error(ErrorCode::InvalidData, "Environment sensor component must be an object");
    }
    for (const auto& [name, value] : definition.items()) {
        if (name != "triggers") {
            return Error(ErrorCode::Unsupported, "Environment sensor field is not implemented: " + name);
        }
    }
    EntityEnvironmentSensor result;
    const auto triggers = definition.find("triggers");
    if (triggers == definition.end()) {
        return result;
    }
    if (!triggers->is_object() && !triggers->is_array()) {
        return Error(ErrorCode::InvalidData, "Environment sensor triggers must be an object or array");
    }
    constexpr std::size_t MAX_TRIGGERS = 4096;
    if (triggers->is_array() && triggers->size() > MAX_TRIGGERS) {
        return Error(ErrorCode::InvalidData, "Environment sensor has too many triggers");
    }
    const auto append = [&result](const nlohmann::json& definition) -> Result<void> {
        if (!definition.is_object()) {
            return Error(ErrorCode::InvalidData, "Environment sensor trigger must be an object");
        }
        auto trigger = EntityEventTrigger::compile(definition);
        if (trigger.failed()) {
            return trigger.error();
        }
        result.m_triggers.push_back(std::move(trigger.value()));
        return Result<void>::ok();
    };
    if (triggers->is_object()) {
        MC_TRY(append(*triggers));
    } else {
        for (const auto& trigger : *triggers) {
            MC_TRY(append(trigger));
        }
    }
    return result;
}

Result<void> EntityEnvironmentSensor::validateEvents(const EntityEventProgram& events) const
{
    for (const auto& trigger : m_triggers) {
        if (!events.contains(trigger.eventId())) {
            return Error(
                ErrorCode::InvalidData, "Environment sensor references an unknown entity event: " + trigger.eventId());
        }
    }
    return Result<void>::ok();
}

Result<void> EntityEnvironmentSensor::tick(mc::Entity& entity) const
{
    for (const auto& trigger : m_triggers) {
        if (entity.isRemoved()) {
            break;
        }
        // 同一 tick 内各触发器都读取已生效状态，不提前应用前一事件的待处理写入。
        MC_TRY(trigger.fire(entity));
    }
    return Result<void>::ok();
}
} // namespace mc::mod::bedrock::addon
