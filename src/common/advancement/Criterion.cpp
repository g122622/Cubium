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
#include "trigger/impl/GenericTriggers.hpp"
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

Criterion::Criterion(std::string name, ResourceLocation trigger, std::shared_ptr<ICriterionInstance> triggerInstance)
    : m_name(std::move(name))
    , m_trigger(std::move(trigger))
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
        // 构建期未注册的触发器按「只有 player 谓词」的实例承载：ID 取自数据包字面量，
        // 条件按 vanilla 的 player 字段解码。原版在构建期注册表里找不到触发器时会直接
        // 加载失败，这里放宽只是为了后续新增触发器时不必连带改动探针。
        auto instance = std::make_shared<PlayerContextTriggerInstance>();
        const nlohmann::json conditions = json.contains("conditions") ? json["conditions"] : nlohmann::json();
        auto parsed = instance->fromJson(conditions);
        if (parsed.failed()) {
            return parsed.error();
        }
        return Criterion(name, ResourceLocation(triggerId), std::move(instance));
    }

    // 条件实例只关心 conditions 内的字段（对齐 vanilla：条件实例的 codec 描述的是 conditions 对象），
    // 触发器 ID 已单独取出，故将 conditions 解包后传入。缺省 conditions 时传原对象。
    const nlohmann::json& conditions = json.contains("conditions") ? json["conditions"] : json;
    auto instanceResult = trigger->fromJson(conditions);
    if (instanceResult.failed()) {
        return instanceResult.error();
    }

    return Criterion(name, ResourceLocation(triggerId), std::move(instanceResult.value()));
}

nlohmann::json Criterion::toJson() const
{
    if (!m_triggerInstance) {
        return nullptr;
    }
    return m_triggerInstance->toJson();
}

} // namespace mc::advancement
