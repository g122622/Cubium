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

#include "Criterion.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "trigger/CriterionTriggers.hpp"
#include "trigger/impl/ImpossibleTrigger.hpp"
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

Criterion::Criterion(std::string name, std::shared_ptr<ICriterionInstance> triggerInstance)
    : m_name(std::move(name))
    , m_triggerInstance(std::move(triggerInstance))
{}

Result<Criterion> Criterion::fromJson(const std::string& name, const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Criterion '" + name + "' must be a JSON object");
    }

    if (!json.contains("trigger")) {
        return Error(ErrorCode::ResourceParseError, "Criterion '" + name + "' missing 'trigger' field");
    }

    std::string triggerId = json["trigger"].get<std::string>();

    // 获取触发器并反序列化
    auto& triggers = CriterionTriggers::instance();
    auto trigger = triggers.getTrigger(ResourceLocation(triggerId));
    if (!trigger) {
        // 如果触发器未注册，返回impossible实例
        return Criterion(name, std::make_shared<ImpossibleTriggerInstance>());
    }

    auto instanceResult = trigger->fromJson(json);
    if (instanceResult.failed()) {
        return instanceResult.error();
    }

    return Criterion(name, std::move(instanceResult.value()));
}

nlohmann::json Criterion::toJson() const
{
    if (!m_triggerInstance) {
        return nullptr;
    }
    return m_triggerInstance->toJson();
}

} // namespace mc::advancement
