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

#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventProgram.hpp"
#include "common/mod/bedrock/addon/content/entity/properties/EntityPropertyDefinitions.hpp"
#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/mod/bedrock/addon/content/entity/state/timer/EntityTimerState.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>
#include <unordered_set>

namespace mc::mod::bedrock::addon {
namespace {

Result<void> _members(const nlohmann::json& value, std::initializer_list<std::string_view> allowed)
{
    if (!value.is_object()) {
        return Error(ErrorCode::InvalidData, "Entity component must be an object");
    }
    for (const auto& [key, member] : value.items()) {
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
            // TODO: 随组件实现补齐其余字段，禁止忽略未知行为参数。
            return Error(ErrorCode::Unsupported, "Unsupported entity definition field: " + key);
        }
    }
    return Result<void>::ok();
}

Result<void> _number(const nlohmann::json& value, const char* field, double minimum, double maximum)
{
    const auto found = value.find(field);
    if (found == value.end()) {
        return Result<void>::ok();
    }
    if (!found->is_number()) {
        // TODO: 支持组件数值的范围对象和 Molang 表达式。
        return Error(ErrorCode::Unsupported, std::string("Entity component requires a literal number: ") + field);
    }
    const double number = found->get<double>();
    if (!std::isfinite(number) || number < minimum || number > maximum) {
        return Error(ErrorCode::InvalidData, std::string("Entity component number is out of range: ") + field);
    }
    return Result<void>::ok();
}

Result<void> _boolean(const nlohmann::json& value, const char* field)
{
    if (value.contains(field) && !value[field].is_boolean()) {
        return Error(ErrorCode::InvalidData, std::string("Entity component field must be boolean: ") + field);
    }
    return Result<void>::ok();
}

class _AddonActor final : public mc::Entity {
public:
    _AddonActor(
        std::shared_ptr<const AddonEntityDefinition> definition, mc::IWorld* world, mc::ecs::EntityRegistry& registry)
        : Entity(mc::EntityInstanceId(0), world, registry)
        , m_definition(std::move(definition))
    {}

    f32 width() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        return components != nullptr ? components->width() : m_definition->width();
    }

    f32 height() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        return components != nullptr ? components->height() : m_definition->height();
    }

    void tick() override
    {
        auto* events = tryGetComponent<AddonEntityEvents>();
        MC_ASSERT_RELEASE(events != nullptr);
        events->onTick(*this);
        auto* components = tryGetComponent<AddonEntityComponents>();
        MC_ASSERT_RELEASE(components != nullptr);
        components->applyPending(*this);
        if (components->despawnIfRequested(*this)) {
            return;
        }
        mc::Entity::tick();
        components->tickEnvironment(*this);
        components->tickTimers(*this);
    }

    bool isImmuneToFire() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        return components != nullptr ? components->has("minecraft:fire_immune") : m_definition->fireImmune();
    }

private:
    std::shared_ptr<const AddonEntityDefinition> m_definition;
};

class _AddonMob final : public mc::CreatureEntity {
public:
    _AddonMob(
        std::shared_ptr<const AddonEntityDefinition> definition, mc::IWorld* world, mc::ecs::EntityRegistry& registry)
        : CreatureEntity(mc::EntityInstanceId(0), registry)
        , m_definition(std::move(definition))
    {
        setWorld(world);
    }

    f32 width() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        return components != nullptr ? components->width() : m_definition->width();
    }

    f32 height() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        return components != nullptr ? components->height() : m_definition->height();
    }

    void tick() override
    {
        auto* events = tryGetComponent<AddonEntityEvents>();
        MC_ASSERT_RELEASE(events != nullptr);
        events->onTick(*this);
        auto* components = tryGetComponent<AddonEntityComponents>();
        MC_ASSERT_RELEASE(components != nullptr);
        components->applyPending(*this);
        if (components->despawnIfRequested(*this)) {
            return;
        }
        mc::CreatureEntity::tick();
        components->tickEnvironment(*this);
        components->tickTimers(*this);
    }

    bool isImmuneToFire() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        return components != nullptr ? components->has("minecraft:fire_immune") : m_definition->fireImmune();
    }

    bool hurt(mc::DamageSource& source, f32 amount) override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        if (components != nullptr && !components->has("minecraft:health")) {
            return false;
        }
        return mc::CreatureEntity::hurt(source, amount);
    }

    void die(mc::DamageSource& source) override
    {
        if (!isDead() || isRemoved()) {
            return;
        }
        auto* components = tryGetComponent<AddonEntityComponents>();
        MC_ASSERT_RELEASE(components != nullptr);
        if (!components->onDeath(*this, source)) {
            return;
        }
        mc::CreatureEntity::die(source);
    }

private:
    std::shared_ptr<const AddonEntityDefinition> m_definition;
};
} // namespace

Result<AddonEntityDefinition> AddonEntityDefinition::parse(const nlohmann::json& document)
{
    if (!document.is_object() || !document.contains("format_version") || !document["format_version"].is_string() ||
        !document.contains("minecraft:entity") || !document["minecraft:entity"].is_object()) {
        return Error(ErrorCode::InvalidData, "Entity definition requires format_version and minecraft:entity");
    }
    // TODO: 按实体 format_version 接入完整版本语义矩阵。
    const auto& entity = document["minecraft:entity"];
    MC_TRY(_members(entity, {"description", "components", "component_groups", "events"}));
    if (!entity.contains("description") || !entity["description"].is_object()) {
        return Error(ErrorCode::InvalidData, "Entity definition requires a description");
    }
    const auto& description = entity["description"];
    MC_TRY(_members(description, {"identifier", "is_spawnable", "is_summonable", "is_experimental", "properties"}));
    if (!description.contains("identifier") || !description["identifier"].is_string()) {
        return Error(ErrorCode::InvalidData, "Entity definition requires an identifier");
    }
    const auto id = description["identifier"].get<std::string>();
    if (id.find(':') == std::string::npos || !mc::ResourceLocation::parse(id).isValid()) {
        return Error(ErrorCode::InvalidData, "Invalid addon entity identifier: " + id);
    }
    for (const char* flag : {"is_spawnable", "is_summonable", "is_experimental"}) {
        MC_TRY(_boolean(description, flag));
    }
    for (const char* section : {"component_groups", "events"}) {
        if (entity.contains(section) && !entity[section].is_object()) {
            return Error(ErrorCode::InvalidData, std::string("Entity section must be an object: ") + section);
        }
    }

    AddonEntityDefinition result;
    result.m_document = document;
    result.m_identifier = id;
    result.m_canSummon = description.value("is_summonable", false);
    auto properties = EntityPropertyDefinitions::compile(description.value("properties", nlohmann::json::object()));
    if (properties.failed()) {
        return properties.error();
    }
    result.m_properties = std::make_shared<const EntityPropertyDefinitions>(std::move(properties.value()));
    result.m_components = entity.value("components", nlohmann::json::object());
    if (!result.m_components.is_object()) {
        return Error(ErrorCode::InvalidData, "Entity components must be an object");
    }
    MC_TRY(validateComponents(result.m_components));
    result.m_componentGroups = entity.value("component_groups", nlohmann::json::object());
    for (const auto& [name, group] : result.m_componentGroups.items()) {
        if (name.empty()) {
            return Error(ErrorCode::InvalidData, "Entity component group name cannot be empty");
        }
        MC_TRY(validateComponents(group));
    }
    bool hasMovement = result.m_components.contains("minecraft:movement");
    for (const auto& [name, group] : result.m_componentGroups.items()) {
        hasMovement = hasMovement || group.contains("minecraft:movement");
    }
    if (hasMovement && !result.isLiving()) {
        return Error(ErrorCode::Unsupported, "Moving addon actors without health are not implemented");
    }
    auto events = EntityEventProgram::compile(entity.value("events", nlohmann::json::object()),
        result.m_properties->literalSchema(),
        result.m_componentGroups);
    if (events.failed()) {
        return events.error();
    }
    result.m_events = std::make_shared<const EntityEventProgram>(std::move(events.value()));

    // 组件结构校验完成后，再验证其事件引用能在整个实体定义中解析。
    const auto validateComponentEvents = [&result](const nlohmann::json& components) -> Result<void> {
        if (const auto timer = components.find("minecraft:timer"); timer != components.end()) {
            const auto& trigger = (*timer)["time_down_event"];
            const auto name = trigger.is_string() ? trigger.get<std::string>() : trigger["event"].get<std::string>();
            if (!result.m_events->contains(name)) {
                return Error(ErrorCode::InvalidData, "Timer references an unknown entity event: " + name);
            }
        }
        if (const auto death = components.find("minecraft:on_death"); death != components.end()) {
            const auto name = (*death)["event"].get<std::string>();
            if (!result.m_events->contains(name)) {
                return Error(ErrorCode::InvalidData, "Death trigger references an unknown entity event: " + name);
            }
        }
        if (const auto sensor = components.find("minecraft:environment_sensor"); sensor != components.end()) {
            auto compiled = EntityEnvironmentSensor::compile(*sensor);
            if (compiled.failed()) {
                return compiled.error();
            }
            MC_TRY(compiled.value().validateEvents(*result.m_events));
        }
        return Result<void>::ok();
    };
    MC_TRY(validateComponentEvents(result.m_components));
    for (const auto& [name, group] : result.m_componentGroups.items()) {
        MC_TRY(validateComponentEvents(group));
    }

    const auto collision = result.m_components.find("minecraft:collision_box");
    result.m_width = collision == result.m_components.end() ? 0.0f : std::max(0.0f, collision->value("width", 1.0f));
    result.m_height = collision == result.m_components.end() ? 0.0f : std::max(0.0f, collision->value("height", 1.0f));
    return result;
}

Result<void> AddonEntityDefinition::validateComponents(const nlohmann::json& components)
{
    if (!components.is_object()) {
        return Error(ErrorCode::InvalidData, "Entity components must be an object");
    }
    for (const auto& [name, component] : components.items()) {
        if (name == "minecraft:collision_box") {
            MC_TRY(_members(component, {"width", "height"}));
            MC_TRY(_number(component, "width", -100000000.0, 100000000.0));
            MC_TRY(_number(component, "height", -100000000.0, 100000000.0));
        } else if (name == "minecraft:physics") {
            MC_TRY(_members(component, {"has_collision", "has_gravity", "push_towards_closest_space"}));
            for (const char* field : {"has_collision", "has_gravity", "push_towards_closest_space"}) {
                MC_TRY(_boolean(component, field));
            }
            if (component.value("push_towards_closest_space", false)) {
                // TODO: 复用实体脱困检测实现 push_towards_closest_space。
                return Error(ErrorCode::Unsupported, "Entity closest-space physics is not implemented");
            }
        } else if (name == "minecraft:health") {
            MC_TRY(_members(component, {"value", "max"}));
            const auto bounds = mc::entity::attribute::Attributes::maxHealth();
            MC_TRY(_number(component, "max", bounds->minValue(), bounds->maxValue()));
            MC_TRY(_number(component, "value", 0.0, bounds->maxValue()));
        } else if (name == "minecraft:movement") {
            MC_TRY(_members(component, {"value"}));
            const auto bounds = mc::entity::attribute::Attributes::movementSpeed();
            MC_TRY(_number(component, "value", bounds->minValue(), bounds->maxValue()));
        } else if (name == "minecraft:timer") {
            MC_TRY(EntityTimerState::validateDefinition(component));
        } else if (name == "minecraft:environment_sensor") {
            auto sensor = EntityEnvironmentSensor::compile(component);
            if (sensor.failed()) {
                return sensor.error();
            }
        } else if (name == "minecraft:on_death") {
            MC_TRY(_members(component, {"event", "target", "filters"}));
            auto trigger = EntityEventTrigger::compile(component);
            if (trigger.failed()) {
                return trigger.error();
            }
        } else if (name == "minecraft:instant_despawn") {
            MC_TRY(_members(component, {"remove_child_entities"}));
            MC_TRY(_boolean(component, "remove_child_entities"));
            if (component.value("remove_child_entities", false)) {
                // TODO: 接入基岩子实体关系，包括被拴住的实体，不能用骑乘列表代替。
                return Error(ErrorCode::Unsupported, "Despawning linked child entities is not implemented");
            }
        } else if (name == "minecraft:fire_immune") {
            MC_TRY(_members(component, {}));
        } else if (name == "minecraft:variant" || name == "minecraft:mark_variant") {
            MC_TRY(_members(component, {"value"}));
            MC_TRY(_number(component, "value", std::numeric_limits<i32>::min(), std::numeric_limits<i32>::max()));
            if (component.contains("value") && !component["value"].is_number_integer()) {
                return Error(ErrorCode::InvalidData, "Entity variant value must be an integer");
            }
            // 变种由活动组件来源保存，切换组不需要复制第二份实例状态。
            // TODO: 将基岩变种和标记变种同步到 Java 客户端的实体投影。
        } else if (name == "minecraft:type_family") {
            MC_TRY(_members(component, {"family"}));
            if (const auto family = component.find("family"); family != component.end()) {
                if (family->is_string()) {
                    if (family->get_ref<const std::string&>().empty()) {
                        return Error(ErrorCode::InvalidData, "Entity family name cannot be empty");
                    }
                } else if (family->is_array()) {
                    for (const auto& value : *family) {
                        if (!value.is_string() || value.get_ref<const std::string&>().empty()) {
                            return Error(ErrorCode::InvalidData, "Entity family names must be nonempty strings");
                        }
                    }
                } else {
                    return Error(ErrorCode::InvalidData, "Entity family must be a string or string array");
                }
            }
        } else {
            // TODO: 分批接入行为、背包、驯服、骑乘、投射物和其他实体组件。
            return Error(ErrorCode::Unsupported, "Entity component is not implemented: " + name);
        }
    }
    return Result<void>::ok();
}

std::unique_ptr<mc::Entity> AddonEntityDefinition::create(std::shared_ptr<const AddonEntityDefinition> definition,
    mc::IWorld* world,
    mc::ecs::EntityRegistry& registry,
    std::weak_ptr<EntityEventSink> sink)
{
    MC_ASSERT_RELEASE(definition != nullptr);
    std::unique_ptr<mc::Entity> entity;
    if (definition->isLiving()) {
        entity = std::make_unique<_AddonMob>(definition, world, registry);
    } else {
        entity = std::make_unique<_AddonActor>(definition, world, registry);
    }
    auto initialized = definition->_initialize(*entity);
    if (initialized.failed()) {
        spdlog::warn(
            "Failed to initialize addon entity '{}': {}", definition->identifier(), initialized.error().message());
        return nullptr;
    }
    auto& components = entity->getOrAddComponent<AddonEntityComponents>(definition);
    components.initialize(*entity);
    static_cast<void>(
        entity->getOrAddComponent<AddonEntityEvents>(definition->m_events, std::move(sink), world != nullptr));
    return entity;
}

Result<void> AddonEntityDefinition::_initialize(mc::Entity& entity) const
{
    auto definitions = m_properties->instantiate(entity.getRandom());
    if (definitions.failed()) {
        return definitions.error();
    }
    MC_TRY(entity.getOrAddComponent<EntityAddonState>().defineProperties(definitions.value()));
    // TODO: 为 Java 客户端实现自定义类型投影、资源包模型与 client_sync 属性同步。
    // TODO: 按 is_spawnable 生成对应的自定义刷怪蛋及生成规则关联。
    return Result<void>::ok();
}

const std::string& AddonEntityDefinition::identifier() const
{
    return m_identifier;
}

const nlohmann::json& AddonEntityDefinition::document() const
{
    return m_document;
}

f32 AddonEntityDefinition::width() const
{
    return m_width;
}

f32 AddonEntityDefinition::height() const
{
    return m_height;
}

bool AddonEntityDefinition::canSummon() const
{
    return m_canSummon;
}

bool AddonEntityDefinition::fireImmune() const
{
    return m_components.contains("minecraft:fire_immune");
}

const nlohmann::json& AddonEntityDefinition::baseComponents() const
{
    return m_components;
}

const nlohmann::json& AddonEntityDefinition::componentGroups() const
{
    return m_componentGroups;
}

bool AddonEntityDefinition::isLiving() const
{
    if (m_components.contains("minecraft:health")) {
        return true;
    }
    return std::any_of(m_componentGroups.begin(), m_componentGroups.end(), [](const nlohmann::json& group) {
        return group.contains("minecraft:health");
    });
}

} // namespace mc::mod::bedrock::addon
