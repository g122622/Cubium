#include "Criterion.hpp"
#include "trigger/CriterionTriggers.hpp"
#include "trigger/impl/ImpossibleTrigger.hpp"

namespace mc::advancement {

Criterion::Criterion(std::string name, std::shared_ptr<ICriterionInstance> triggerInstance)
    : m_name(std::move(name))
    , m_triggerInstance(std::move(triggerInstance)) {
}

Result<Criterion> Criterion::fromJson(const std::string& name, const nlohmann::json& json) {
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError,
                     "Criterion '" + name + "' must be a JSON object");
    }

    if (!json.contains("trigger")) {
        return Error(ErrorCode::ResourceParseError,
                     "Criterion '" + name + "' missing 'trigger' field");
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

nlohmann::json Criterion::toJson() const {
    if (!m_triggerInstance) {
        return nullptr;
    }
    return m_triggerInstance->toJson();
}

} // namespace mc::advancement
