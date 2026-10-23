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

#include "common/mod/bedrock/addon/content/entity/events/EntityEventTrigger.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/mod/bedrock/addon/content/entity/events/AddonEntityEvents.hpp"

namespace mc::mod::bedrock::addon {
Result<EntityEventTrigger> EntityEventTrigger::compile(const nlohmann::json& definition)
{
    EntityEventTrigger result;
    if (definition.is_string()) {
        result.m_eventId = definition.get<std::string>();
    } else if (definition.is_object()) {
        for (const auto& [name, value] : definition.items()) {
            if (name != "event" && name != "target" && name != "filters") {
                return Error(ErrorCode::Unsupported, "Component event reference field is not implemented: " + name);
            }
        }
        if (!definition.contains("event") || !definition["event"].is_string()) {
            return Error(ErrorCode::InvalidData, "Component event reference requires an event name");
        }
        result.m_eventId = definition["event"].get<std::string>();
        if (definition.contains("target") && definition["target"] != "self") {
            // TODO: 让非自身组件事件引用使用实际交互参与者作为目标。
            return Error(ErrorCode::Unsupported, "Component event targets other than self are not implemented");
        }
        if (definition.contains("filters")) {
            auto filter = EntityFilter::compile(definition["filters"]);
            if (filter.failed()) {
                return filter.error();
            }
            result.m_filter = std::move(filter.value());
        }
    } else {
        return Error(ErrorCode::InvalidData, "Component event reference must be a string or object");
    }
    if (result.m_eventId.empty()) {
        return Error(ErrorCode::InvalidData, "Component event name cannot be empty");
    }
    return result;
}

const std::string& EntityEventTrigger::eventId() const
{
    return m_eventId;
}

Result<void> EntityEventTrigger::fire(mc::Entity& entity) const
{
    const EntityFilterContext context{entity, nullptr, nullptr, nullptr, nullptr, nullptr};
    if (m_filter) {
        auto matches = m_filter->matches(context);
        if (matches.failed()) {
            return matches.error();
        }
        if (!matches.value()) {
            return Result<void>::ok();
        }
    }
    auto* events = entity.tryGetComponent<AddonEntityEvents>();
    if (events == nullptr) {
        return Error(ErrorCode::InvalidState, "Component event owner has no event program");
    }
    return events->trigger(context, m_eventId);
}
} // namespace mc::mod::bedrock::addon
