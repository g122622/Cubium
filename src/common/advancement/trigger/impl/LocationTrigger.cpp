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

#include "LocationTrigger.hpp"

namespace mc::advancement {

// ========== LocationTriggerInstance ==========

LocationTriggerInstance::LocationTriggerInstance(LocationPredicate location)
    : m_location(std::move(location))
{}

bool LocationTriggerInstance::test(const World& world, f64 x, f64 y, f64 z) const
{
    return m_location.test(world, x, y, z);
}

Result<void> LocationTriggerInstance::fromJson(const nlohmann::json& json)
{
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

nlohmann::json LocationTriggerInstance::conditionsToJson() const
{
    return m_location.toJson();
}

// ========== LocationTrigger ==========

Result<std::shared_ptr<LocationTriggerInstance>> LocationTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<LocationTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void LocationTrigger::trigger(ServerPlayer& player)
{
    // [TODO 阶段2+3：事件系统集成] 获取玩家位置并检测条件
    // 需要 ServerPlayer 提供位置和世界信息，由 LocationEvent 触发
    MC_UNUSED(player);
}

std::shared_ptr<LocationTriggerInstance> LocationTrigger::atLocation(const LocationPredicate& location)
{
    return std::make_shared<LocationTriggerInstance>(location);
}

std::shared_ptr<LocationTriggerInstance> LocationTrigger::inBiome(const ResourceLocation& biome)
{
    LocationPredicate pred;
    pred = LocationPredicate::fromJson(nlohmann::json{{"biome", biome.toString()}}).value();
    return std::make_shared<LocationTriggerInstance>(pred);
}

std::shared_ptr<LocationTriggerInstance> LocationTrigger::inDimension(const ResourceLocation& dimension)
{
    LocationPredicate pred;
    pred = LocationPredicate::fromJson(nlohmann::json{{"dimension", dimension.toString()}}).value();
    return std::make_shared<LocationTriggerInstance>(pred);
}

} // namespace mc::advancement
