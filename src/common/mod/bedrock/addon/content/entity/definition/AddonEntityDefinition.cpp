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
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"

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
        return m_definition->width();
    }

    f32 height() const override
    {
        return m_definition->height();
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
        return m_definition->width();
    }

    f32 height() const override
    {
        return m_definition->height();
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
        if (entity.contains(section) && !entity[section].empty()) {
            // TODO: 接入组件组变更、实体事件动作、过滤器与 Molang 求值。
            return Error(ErrorCode::Unsupported, std::string("Entity section is not implemented: ") + section);
        }
    }

    AddonEntityDefinition result;
    result.m_document = document;
    result.m_identifier = id;
    result.m_canSummon = description.value("is_summonable", false);
    result.m_properties = description.value("properties", nlohmann::json::object());
    EntityAddonState validated;
    MC_TRY(validated.defineProperties(result.m_properties));
    result.m_components = entity.value("components", nlohmann::json::object());
    if (!result.m_components.is_object()) {
        return Error(ErrorCode::InvalidData, "Entity components must be an object");
    }
    for (const auto& [name, component] : result.m_components.items()) {
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
        } else if (name == "minecraft:fire_immune") {
            MC_TRY(_members(component, {}));
        } else {
            // TODO: 分批接入行为、背包、驯服、骑乘、投射物和其他实体组件。
            return Error(ErrorCode::Unsupported, "Entity component is not implemented: " + name);
        }
    }
    if (result.m_components.contains("minecraft:movement") && !result.m_components.contains("minecraft:health")) {
        return Error(ErrorCode::Unsupported, "Moving addon actors without health are not implemented");
    }

    const auto collision = result.m_components.find("minecraft:collision_box");
    result.m_width = collision == result.m_components.end() ? 0.0f : std::max(0.0f, collision->value("width", 1.0f));
    result.m_height = collision == result.m_components.end() ? 0.0f : std::max(0.0f, collision->value("height", 1.0f));
    return result;
}

std::unique_ptr<mc::Entity> AddonEntityDefinition::create(
    std::shared_ptr<const AddonEntityDefinition> definition, mc::IWorld* world, mc::ecs::EntityRegistry& registry)
{
    MC_ASSERT_RELEASE(definition != nullptr);
    std::unique_ptr<mc::Entity> entity;
    if (definition->m_components.contains("minecraft:health")) {
        entity = std::make_unique<_AddonMob>(definition, world, registry);
    } else {
        entity = std::make_unique<_AddonActor>(definition, world, registry);
    }
    definition->_initialize(*entity);
    return entity;
}

void AddonEntityDefinition::_initialize(mc::Entity& entity) const
{
    auto defined = entity.getOrAddComponent<EntityAddonState>().defineProperties(m_properties);
    MC_ASSERT_RELEASE(defined.success());
    const auto physics = m_components.find("minecraft:physics");
    entity.setNoGravity(physics == m_components.end() || !physics->value("has_gravity", true));
    entity.setNoClip(physics == m_components.end() || !physics->value("has_collision", true));
    // 构造期虚调用只见到基类尺寸，安装定义后重新计算碰撞箱。
    entity.setPosition(entity.position());
    if (auto* living = dynamic_cast<mc::LivingEntity*>(&entity)) {
        const auto& health = m_components["minecraft:health"];
        living->attributes().setBaseValue(mc::entity::attribute::Attributes::MAX_HEALTH, health.value("max", 20.0));
        living->setHealth(health.value("value", 20.0f));
        const auto movement = m_components.find("minecraft:movement");
        living->attributes().setBaseValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED,
            movement == m_components.end() ? 0.0 : movement->value("value", 1.0));
    }
    // TODO: 为 Java 客户端实现自定义类型投影、资源包模型与 client_sync 属性同步。
    // TODO: 按 is_spawnable 生成对应的自定义刷怪蛋及生成规则关联。
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

} // namespace mc::mod::bedrock::addon
