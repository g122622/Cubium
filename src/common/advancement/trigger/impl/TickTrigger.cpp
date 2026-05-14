#include "TickTrigger.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== TickTriggerInstance ==========

Result<void> TickTriggerInstance::fromJson(const nlohmann::json& json)
{
    // Tick触发器没有条件
    MC_UNUSED(json);
    return {};
}

nlohmann::json TickTriggerInstance::conditionsToJson() const
{
    return nullptr;
}

// ========== TickTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> TickTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<TickTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void TickTrigger::trigger(ServerPlayer& player)
{
    // Tick触发器对所有监听器都触发
    // 注意：实际触发逻辑需要在服务端模块中实现，
    // 这里需要访问 PlayerAdvancements 来触发监听器
    // 服务端会在每tick调用此方法，触发器将检查所有已注册的监听器
    MC_UNUSED(player);
}

} // namespace mc::advancement
