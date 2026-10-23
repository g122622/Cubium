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

#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"
#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {
namespace {
constexpr const char* _COMPONENTS_TAG = "CubiumAddonComponents";

void _save(const mc::Entity& entity, mc::nbt::tags::compound_tag& tag)
{
    if (const auto* components = entity.tryGetComponent<AddonEntityComponents>()) {
        tag.value.insert_or_assign(_COMPONENTS_TAG, mc::nbt::jsonToNbt(components->save()));
    }
}

Result<void> _load(mc::Entity& entity, const mc::nbt::tags::compound_tag& tag)
{
    const auto found = tag.value.find(_COMPONENTS_TAG);
    if (found == tag.value.end()) {
        return Result<void>::ok();
    }
    auto* components = entity.tryGetComponent<AddonEntityComponents>();
    if (components == nullptr || found->second == nullptr || found->second->id() != mc::nbt::TagId::Compound) {
        return Error(ErrorCode::InvalidData, "Saved addon components require a matching entity definition");
    }
    return components->load(entity, mc::nbt::nbtToJson(*found->second));
}
} // namespace

AddonEntityComponents::AddonEntityComponents(std::shared_ptr<const AddonEntityDefinition> definition)
    : m_definition(std::move(definition))
    , m_deathHandled(false)
{
    MC_ASSERT_RELEASE(m_definition != nullptr);
    for (const auto& [name, value] : m_definition->baseComponents().items()) {
        m_sources[name] = "";
    }
}

void AddonEntityComponents::initialize(mc::Entity& entity)
{
    for (const char* name : {"minecraft:collision_box",
             "minecraft:physics",
             "minecraft:health",
             "minecraft:movement",
             "minecraft:timer",
             "minecraft:on_death"}) {
        _apply(entity, name);
    }
}

void AddonEntityComponents::queue(const std::vector<EntityComponentGroupChange>& changes)
{
    m_pending.insert(m_pending.end(), changes.begin(), changes.end());
}

void AddonEntityComponents::applyPending(mc::Entity& entity)
{
    auto pending = std::move(m_pending);
    m_pending.clear();
    for (const auto& change : pending) {
        const auto& group = m_definition->componentGroups()[change.group];
        if (change.add) {
            m_activeGroups.insert(change.group);
            for (const auto& [name, value] : group.items()) {
                m_sources[name] = change.group;
                // 重复添加活动组同样重新初始化，不能仅检查配置是否发生变化。
                _apply(entity, name);
            }
        } else if (m_activeGroups.erase(change.group) > 0) {
            for (const auto& [name, value] : group.items()) {
                // 移除就是删除该组件，不回退到基础定义或其他仍活动的组。
                m_sources.erase(name);
                _apply(entity, name);
            }
        }
    }
}

bool AddonEntityComponents::has(const std::string& name) const
{
    return m_sources.contains(name);
}

const nlohmann::json* AddonEntityComponents::component(const std::string& name) const
{
    const auto found = m_sources.find(name);
    if (found == m_sources.end()) {
        return nullptr;
    }
    const auto& source =
        found->second.empty() ? m_definition->baseComponents() : m_definition->componentGroups()[found->second];
    return &source[name];
}

f32 AddonEntityComponents::width() const
{
    const auto* collision = component("minecraft:collision_box");
    return collision != nullptr ? std::max(0.0f, collision->value("width", 1.0f)) : 0.0f;
}

f32 AddonEntityComponents::height() const
{
    const auto* collision = component("minecraft:collision_box");
    return collision != nullptr ? std::max(0.0f, collision->value("height", 1.0f)) : 0.0f;
}

void AddonEntityComponents::_apply(mc::Entity& entity, const std::string& name)
{
    const auto* value = component(name);
    if (name == "minecraft:collision_box") {
        entity.setPosition(entity.position());
    } else if (name == "minecraft:physics") {
        entity.setNoGravity(value == nullptr || !value->value("has_gravity", true));
        entity.setNoClip(value == nullptr || !value->value("has_collision", true));
    } else if (name == "minecraft:timer") {
        if (value == nullptr) {
            m_timer.reset();
        } else {
            auto timer = EntityTimerState::create(*value, entity.getRandom());
            MC_ASSERT_RELEASE(timer.success());
            m_timer = std::move(timer.value());
        }
    } else if (name == "minecraft:on_death") {
        if (value == nullptr) {
            m_onDeath.reset();
        } else {
            auto trigger = EntityEventTrigger::compile(*value);
            MC_ASSERT_RELEASE(trigger.success());
            m_onDeath = std::move(trigger.value());
        }
    } else if (auto* living = dynamic_cast<mc::LivingEntity*>(&entity)) {
        if (name == "minecraft:health") {
            living->attributes().setBaseValue(
                mc::entity::attribute::Attributes::MAX_HEALTH, value != nullptr ? value->value("max", 20.0) : 20.0);
            living->setHealth(value != nullptr ? value->value("value", 20.0f) : 20.0f);
        } else if (name == "minecraft:movement") {
            living->attributes().setBaseValue(
                mc::entity::attribute::Attributes::MOVEMENT_SPEED, value != nullptr ? value->value("value", 1.0) : 0.0);
        }
    }
}

nlohmann::json AddonEntityComponents::save() const
{
    MC_ASSERT_RELEASE(has("minecraft:timer") == m_timer.has_value());
    nlohmann::json pending = nlohmann::json::array();
    for (const auto& change : m_pending) {
        pending.push_back({{"operation", change.add ? "add" : "remove"}, {"group", change.group}});
    }
    // 保存来源名而非组件 JSON，避免 NBT Byte 丢失配置中的布尔类型。
    return {{"sources", m_sources},
        {"active", m_activeGroups},
        {"pending", std::move(pending)},
        {"death", m_deathHandled ? "handled" : "alive"},
        {"timer", m_timer ? m_timer->save() : nlohmann::json{{"state", "absent"}}}};
}

Result<void> AddonEntityComponents::load(mc::Entity& entity, const nlohmann::json& saved)
{
    if (!saved.is_object() || !saved.contains("sources") || !saved["sources"].is_object() ||
        !saved.contains("active") || !saved["active"].is_array() || !saved.contains("pending") ||
        !saved["pending"].is_array() || !saved.contains("timer") || !saved["timer"].is_object() ||
        !saved.contains("death") || (saved["death"] != "handled" && saved["death"] != "alive")) {
        return Error(ErrorCode::InvalidData, "Invalid saved addon component state");
    }
    std::map<std::string, std::string> sources;
    std::set<std::string> active;
    std::vector<EntityComponentGroupChange> pending;
    const auto& groups = m_definition->componentGroups();
    for (const auto& group : saved["active"]) {
        if (!group.is_string() || !groups.contains(group.get<std::string>()) ||
            !active.insert(group.get<std::string>()).second) {
            return Error(ErrorCode::InvalidData, "Invalid saved addon active group");
        }
    }
    for (const auto& [name, group] : saved["sources"].items()) {
        if (!group.is_string()) {
            return Error(ErrorCode::InvalidData, "Invalid saved addon component source");
        }
        const auto source = group.get<std::string>();
        if ((source.empty() && !m_definition->baseComponents().contains(name)) ||
            (!source.empty() && (!active.contains(source) || !groups[source].contains(name)))) {
            return Error(ErrorCode::InvalidData, "Saved addon component source is not defined: " + name);
        }
        sources[name] = source;
    }
    for (const auto& change : saved["pending"]) {
        if (!change.is_object() || !change.contains("operation") || !change["operation"].is_string() ||
            !change.contains("group") || !change["group"].is_string() ||
            !groups.contains(change["group"].get<std::string>())) {
            return Error(ErrorCode::InvalidData, "Invalid saved addon component operation");
        }
        const auto operation = change["operation"].get<std::string>();
        if (operation != "add" && operation != "remove") {
            return Error(ErrorCode::InvalidData, "Unknown saved addon component operation");
        }
        pending.push_back({operation == "add", change["group"].get<std::string>()});
    }
    if (sources.contains("minecraft:timer")) {
        MC_TRY(EntityTimerState::validateSavedState(saved["timer"]));
    } else if (saved["timer"] != nlohmann::json{{"state", "absent"}}) {
        return Error(ErrorCode::InvalidData, "Saved timer exists without an active timer component");
    }
    m_sources = std::move(sources);
    m_activeGroups = std::move(active);
    m_pending = std::move(pending);
    m_deathHandled = saved["death"] == "handled";
    initialize(entity);
    if (m_timer) {
        auto restored = m_timer->load(saved["timer"]);
        MC_ASSERT_RELEASE(restored.success());
    }
    return Result<void>::ok();
}

void registerAddonEntityComponentsSerializer(
    mc::entity::serialization::components::ComponentSerializerRegistry& registry)
{
    registry.registerSerializer<AddonEntityComponents>(_save, _load, -100);
}

void AddonEntityComponents::tickTimers(mc::Entity& entity)
{
    if (entity.isRemoved() || !m_timer) {
        return;
    }
    auto result = m_timer->tick(entity);
    if (result.failed()) {
        spdlog::warn("Failed to execute timer event for '{}': {}", entity.getTypeId(), result.error().message());
    }
}

bool AddonEntityComponents::onDeath(mc::Entity& entity, mc::DamageSource& source)
{
    if (m_deathHandled) {
        return false;
    }
    // 在进入事件程序前置位，避免事件通知引起重入后重复执行死亡和掉落。
    m_deathHandled = true;
    if (m_onDeath) {
        const auto* attacker = source.getTrueSource();
        const EntityFilterDamage damage{&entity, source.type(), true};
        const EntityFilterContext context{
            entity, attacker, dynamic_cast<const mc::Player*>(attacker), nullptr, attacker, nullptr, &damage};
        auto result = m_onDeath->fire(context);
        if (result.failed()) {
            spdlog::warn("Failed to execute death event for '{}': {}", entity.getTypeId(), result.error().message());
        }
    }
    return true;
}

bool AddonEntityComponents::despawnIfRequested(mc::Entity& entity) const
{
    if (!has("minecraft:instant_despawn")) {
        return false;
    }
    entity.discard();
    return true;
}

} // namespace mc::mod::bedrock::addon
