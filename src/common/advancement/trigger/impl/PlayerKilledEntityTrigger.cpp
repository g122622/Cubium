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

#include "PlayerKilledEntityTrigger.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <utility>
#include <nlohmann/json_fwd.hpp>

// 注意：trigger() 方法的完整实现需要服务端模块的支持
// 服务端代码应包含 server/advancement/TriggerInstantiation.hpp
// 并使用 triggerWithPredicate() 方法或直接调用基类的 trigger() 模板方法

namespace mc::advancement {

// ========== PlayerKilledEntityTriggerInstance ==========

PlayerKilledEntityTriggerInstance::PlayerKilledEntityTriggerInstance(
    EntityPredicate entity, DamageSourcePredicate killingBlow)
    : m_entity(std::move(entity))
    , m_killingBlow(std::move(killingBlow))
{}

bool PlayerKilledEntityTriggerInstance::test(const Entity& entity, const DamageSource& source) const
{
    // 检查实体谓词
    if (!m_entity.test(entity, source)) {
        return false;
    }

    // 检查伤害源谓词
    if (!m_killingBlow.test(source)) {
        return false;
    }

    return true;
}

Result<void> PlayerKilledEntityTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("entity")) {
        auto result = EntityPredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entity = result.value();
    }

    if (json.contains("killing_blow")) {
        auto result = DamageSourcePredicate::fromJson(json["killing_blow"]);
        if (result.failed()) {
            return result.error();
        }
        m_killingBlow = result.value();
    }

    return {};
}

nlohmann::json PlayerKilledEntityTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_entity.isAny()) {
        json["entity"] = m_entity.toJson();
    }
    if (!m_killingBlow.isAny()) {
        json["killing_blow"] = m_killingBlow.toJson();
    }

    return json;
}

// ========== PlayerKilledEntityTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> PlayerKilledEntityTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<PlayerKilledEntityTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void PlayerKilledEntityTrigger::trigger(ServerPlayer& player, const Entity& entity, const DamageSource& source)
{
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端代码应使用以下方式之一触发检测：
    //
    // 方法1：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<PlayerKilledEntityTrigger>();
    // trigger->AbstractCriterionTrigger<PlayerKilledEntityTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&entity, &source](const PlayerKilledEntityTriggerInstance& instance) {
    //         return instance.test(entity, source);
    //     }
    // );
    //
    // 方法2：在 AdvancementEventHandler 中直接调用（推荐）
    // 参考：server/advancement/AdvancementEventHandler.hpp
    MC_UNUSED(player);
    MC_UNUSED(entity);
    MC_UNUSED(source);
}

std::shared_ptr<PlayerKilledEntityTriggerInstance> PlayerKilledEntityTrigger::entityKilled()
{
    return std::make_shared<PlayerKilledEntityTriggerInstance>();
}

std::shared_ptr<PlayerKilledEntityTriggerInstance> PlayerKilledEntityTrigger::entityKilled(
    const EntityPredicate& entity)
{
    return std::make_shared<PlayerKilledEntityTriggerInstance>(entity, DamageSourcePredicate{});
}

std::shared_ptr<PlayerKilledEntityTriggerInstance> PlayerKilledEntityTrigger::killedByEntity(
    const EntityPredicate& killer)
{
    return std::make_shared<PlayerKilledEntityTriggerInstance>(killer, DamageSourcePredicate{});
}

// ========== EntityKilledPlayerTriggerInstance ==========

EntityKilledPlayerTriggerInstance::EntityKilledPlayerTriggerInstance(
    EntityPredicate entity, DamageSourcePredicate killingBlow)
    : m_entity(std::move(entity))
    , m_killingBlow(std::move(killingBlow))
{}

bool EntityKilledPlayerTriggerInstance::test(const Entity& entity, const DamageSource& source) const
{
    if (!m_entity.test(entity, source)) {
        return false;
    }
    if (!m_killingBlow.test(source)) {
        return false;
    }
    return true;
}

Result<void> EntityKilledPlayerTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("entity")) {
        auto result = EntityPredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entity = result.value();
    }

    if (json.contains("killing_blow")) {
        auto result = DamageSourcePredicate::fromJson(json["killing_blow"]);
        if (result.failed()) {
            return result.error();
        }
        m_killingBlow = result.value();
    }

    return {};
}

nlohmann::json EntityKilledPlayerTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_entity.isAny()) {
        json["entity"] = m_entity.toJson();
    }
    if (!m_killingBlow.isAny()) {
        json["killing_blow"] = m_killingBlow.toJson();
    }

    return json;
}

// ========== EntityKilledPlayerTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> EntityKilledPlayerTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<EntityKilledPlayerTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EntityKilledPlayerTrigger::trigger(ServerPlayer& player, const Entity& entity, const DamageSource& source)
{
    // 此方法在 common 模块中无法完整实现
    // 服务端代码应使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    MC_UNUSED(player);
    MC_UNUSED(entity);
    MC_UNUSED(source);
}

} // namespace mc::advancement
