#include "LocationTrigger.hpp"

namespace mc::advancement {

// ========== Instance ==========

LocationTrigger::Instance::Instance(LocationPredicate location)
    : m_location(std::move(location)) {
}

bool LocationTrigger::Instance::test(const World& world, f64 x, f64 y, f64 z) const {
    return m_location.test(world, x, y, z);
}

Result<void> LocationTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    auto result = LocationPredicate::fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    m_location = result.value();
    return {};
}

nlohmann::json LocationTrigger::Instance::conditionsToJson() const {
    return m_location.toJson();
}

// ========== Trigger ==========

Result<std::shared_ptr<LocationTrigger::Instance>> LocationTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void LocationTrigger::trigger(ServerPlayer& player) {
    // [TODO 阶段2+3：事件系统集成] 获取玩家位置并检测条件
    // 需要 ServerPlayer 提供位置和世界信息，由 LocationEvent 触发
    MC_UNUSED(player);
}

std::shared_ptr<LocationTrigger::Instance> LocationTrigger::atLocation(const LocationPredicate& location) {
    return std::make_shared<Instance>(location);
}

std::shared_ptr<LocationTrigger::Instance> LocationTrigger::inBiome(const ResourceLocation& biome) {
    LocationPredicate pred;
    pred = LocationPredicate::fromJson(nlohmann::json{{"biome", biome.toString()}}).value();
    return std::make_shared<Instance>(pred);
}

std::shared_ptr<LocationTrigger::Instance> LocationTrigger::inDimension(const ResourceLocation& dimension) {
    LocationPredicate pred;
    pred = LocationPredicate::fromJson(nlohmann::json{{"dimension", dimension.toString()}}).value();
    return std::make_shared<Instance>(pred);
}

} // namespace mc::advancement
