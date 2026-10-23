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
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/CriterionContext.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <utility>
#include <nlohmann/json_fwd.hpp>

// 注意：trigger() 方法需要服务端模块支持
// 服务端代码应通过事件系统触发

namespace mc::advancement {

// ========== LocationTriggerInstance ==========

LocationTriggerInstance::LocationTriggerInstance(LocationPredicate location)
    : m_location(std::move(location))
{}

bool LocationTriggerInstance::test(const IWorld& world, f64 x, f64 y, f64 z) const
{
    return m_location.test(world, x, y, z);
}

Result<void> LocationTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    // 条件列表写法（ContextAwarePredicate）：[{"condition": "minecraft:entity_properties", ...}]
    if (json.is_array()) {
        auto contextResult = ContextAwarePredicate::fromJson(json);
        if (contextResult.failed()) {
            return contextResult.error();
        }
        m_playerContext = std::move(contextResult.value());
        return {};
    }

    auto result = LocationPredicate::fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    m_location = result.value();

    // 数据包写法：{"player": [{"condition": "minecraft:entity_properties", ...}]}
    if (json.contains("player")) {
        auto contextResult = ContextAwarePredicate::fromJson(json["player"]);
        if (contextResult.failed()) {
            return contextResult.error();
        }
        m_playerContext = std::move(contextResult.value());
    }
    return {};
}

bool LocationTriggerInstance::test(const CriterionContext& context) const
{
    return m_playerContext.test(context);
}

nlohmann::json LocationTriggerInstance::conditionsToJson() const
{
    if (m_playerContext.isAny()) {
        return m_location.toJson();
    }
    nlohmann::json json = nlohmann::json::object();
    json["player"] = m_playerContext.toJson();
    return json;
}

// ========== LocationTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> LocationTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<LocationTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// 注意：此方法在 common 模块中无法完整实现，因为需要 ServerPlayer 完整定义
// 服务端应通过事件系统（AdvancementEventHandler）触发这些触发器
void LocationTrigger::trigger(ServerPlayer& player)
{
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
