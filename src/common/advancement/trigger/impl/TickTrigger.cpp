#include "TickTrigger.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== TickTriggerInstance ==========

Result<void> TickTriggerInstance::fromJson(const nlohmann::json& json) {
    // Tick触发器没有条件
    MC_UNUSED(json);
    return {};
}

nlohmann::json TickTriggerInstance::conditionsToJson() const {
    return nullptr;
}

// ========== TickTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> TickTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<TickTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void TickTrigger::trigger(ServerPlayer& player) {
    // TODO: Implement trigger in server module where ServerPlayer is available
    // Tick触发器对所有监听器都触发
    // for (const auto& listener : getListeners(*player.getAdvancements())) {
    //     listener.grantCriterion(*player.getAdvancements());
    // }
    MC_UNUSED(player);
}

} // namespace mc::advancement
