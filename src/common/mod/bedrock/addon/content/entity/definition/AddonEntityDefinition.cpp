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
#include "common/entity/core/AgeableEntity.hpp" // minecraft:is_baby → AgeableEntity::setChild
#include "common/entity/core/CreatureEntity.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventProgram.hpp"
#include "common/mod/bedrock/addon/content/entity/properties/EntityPropertyDefinitions.hpp"
#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/mod/bedrock/addon/content/entity/state/timer/EntityTimerState.hpp"
#include "common/mod/bedrock/addon/content/molang/MolangExpression.hpp" // _number：组件数值的 Molang 形式
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
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

    // 基岩允许三种写法，加载期都要接受：
    //   1) 字面数字：          "value": 20
    //   2) 范围对象：          "value": { "range_min": 10, "range_max": 20 }（实体实例化时在区间取值）
    //   3) Molang 表达式字符串："value": "math.random(10, 20)"
    // 后两种此前一律被以 Unsupported 拒绝，等于把合法行为包挡在门外。
    double resolvedMin = 0.0;
    double resolvedMax = 0.0;

    if (found->is_number()) {
        resolvedMin = resolvedMax = found->get<double>();
    } else if (found->is_object()) {
        // 范围对象只认 range_min / range_max；其他键说明作者写错了字段名，直接报错而不是忽略。
        for (const auto& [key, member] : found->items()) {
            if (key != "range_min" && key != "range_max") {
                return Error(ErrorCode::InvalidData, std::string("Unsupported range object field: ") + key);
            }

            if (!member.is_number()) {
                return Error(ErrorCode::InvalidData,
                    std::string("Entity component range bound must be a literal number: ") + field);
            }
        }

        const auto minIt = found->find("range_min");
        const auto maxIt = found->find("range_max");
        if (minIt == found->end() && maxIt == found->end()) {
            return Error(ErrorCode::InvalidData,
                std::string("Entity component range object requires range_min or range_max: ") + field);
        }

        resolvedMin = minIt != found->end() ? minIt->get<double>() : maxIt->get<double>();
        resolvedMax = maxIt != found->end() ? maxIt->get<double>() : resolvedMin;
        if (resolvedMin > resolvedMax) {
            return Error(ErrorCode::InvalidData, std::string("Entity component range_min exceeds range_max: ") + field);
        }
    } else if (found->is_string()) {
        // Molang：加载期只校验能编译，取值推迟到实体实例化（此时没有查询上下文可代入）。
        const std::string expression = found->get<std::string>();
        if (auto compiled = MolangExpression::compile(expression); !compiled.success()) {
            return Error(ErrorCode::InvalidData,
                std::string("Invalid Molang expression for ") + field + ": " + compiled.error().message());
        }

        // Molang 结果无法在加载期静态确定，跳过范围校验——范围在求值后才可知。
        return Result<void>::ok();
    } else {
        return Error(ErrorCode::InvalidData,
            std::string("Entity component field must be a number, range object or Molang string: ") + field);
    }

    if (!std::isfinite(resolvedMin) || !std::isfinite(resolvedMax) || resolvedMin < minimum || resolvedMax > maximum) {
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

/**
 * @brief 解析 `minecraft:attack` 的 damage 值并校验其形态与范围
 *
 * 官方文档给出三种承载形态：单个数字、`[min, max]` 数组、`{range_min, range_max}` 对象。
 * 三种都必须接受——基岩原版实体定义里三种写法都出现过，只认其中一种会让合法 addon 加载失败。
 *
 * 下限允许负数：文档明确"负值改为治疗目标"。
 *
 * @param damage damage 字段的 JSON 值
 * @return 校验结果
 */
Result<void> _attackDamageRange(const nlohmann::json& damage)
{
    const auto checkBound = [](const nlohmann::json& bound, const char* field) -> Result<void> {
        if (!bound.is_number() || !std::isfinite(bound.get<double>())) {
            return Error(
                ErrorCode::InvalidData, std::string("minecraft:attack damage must be a finite number: ") + field);
        }
        return Result<void>::ok();
    };

    if (damage.is_number()) {
        return checkBound(damage, "damage");
    }

    if (damage.is_array()) {
        if (damage.size() != 2) {
            return Error(ErrorCode::InvalidData, "minecraft:attack damage array must have exactly two entries");
        }
        MC_TRY(checkBound(damage[0], "damage[0]"));
        return checkBound(damage[1], "damage[1]");
    }

    if (damage.is_object()) {
        if (!damage.contains("range_min") || !damage.contains("range_max")) {
            return Error(ErrorCode::InvalidData, "minecraft:attack damage object requires range_min and range_max");
        }
        MC_TRY(checkBound(damage["range_min"], "range_min"));
        return checkBound(damage["range_max"], "range_max");
    }

    return Error(ErrorCode::InvalidData, "minecraft:attack damage must be a number, [min, max] array, or range object");
}

/**
 * @brief 取 `minecraft:attack` damage 的代表值（区间取中值）
 *
 * 属性只能承载单个基值，而基岩的 damage 可以是区间。取中值使"平均伤害"与原版一致，
 * 是区间信息在单值属性模型下损失最小的投影。
 *
 * @param damage 已通过 _attackDamageRange 校验的 damage 字段
 * @return 代表值；形态不认识时返回 nullopt
 */
std::optional<f64> _attackDamageRepresentative(const nlohmann::json& damage)
{
    if (damage.is_number()) {
        return damage.get<double>();
    }
    if (damage.is_array() && damage.size() == 2) {
        return (damage[0].get<double>() + damage[1].get<double>()) / 2.0;
    }
    if (damage.is_object() && damage.contains("range_min") && damage.contains("range_max")) {
        return (damage["range_min"].get<double>() + damage["range_max"].get<double>()) / 2.0;
    }
    return std::nullopt;
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

    /**
     * @brief 是否可被其他实体推动
     *
     * 对应 `minecraft:pushable` 的 `is_pushable`（默认 true）。非生物实体同样受该字段约束：
     * `isPushable` 为真的实体才计入实体碰撞，进而影响"能否在该位置放置方块"。
     * 未声明该组件时沿用基类默认值。
     */
    bool isPushable() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        const nlohmann::json* pushable = components != nullptr ? components->component("minecraft:pushable") : nullptr;
        if (pushable == nullptr) {
            return mc::Entity::isPushable();
        }
        return pushable->value("is_pushable", true);
    }

    /**
     * @brief 被活塞推动时的反应
     *
     * 对应 `minecraft:pushable` 的 `is_pushable_by_piston`（默认 true）。为 false 时返回
     * `Ignore`，活塞扫过时跳过该实体；其余情况沿用基类反应。
     */
    PushReaction getPushReaction() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        const nlohmann::json* pushable = components != nullptr ? components->component("minecraft:pushable") : nullptr;
        if (pushable == nullptr || pushable->value("is_pushable_by_piston", true)) {
            return mc::Entity::getPushReaction();
        }
        return PushReaction::Ignore;
    }

private:
    std::shared_ptr<const AddonEntityDefinition> m_definition;
};

// 继承 AgeableEntity 而非 CreatureEntity：基岩 addon 的 mob 普遍带 minecraft:is_baby 与年龄
// 语义（幼体体型缩放、成长计时），AgeableEntity 才是能承载这些的基类（它是 CreatureEntity 的子类，
// 不会丢任何现有能力）。此前继承 CreatureEntity 使 is_baby 无处落地。
class _AddonMob final : public mc::AgeableEntity {
public:
    _AddonMob(
        std::shared_ptr<const AddonEntityDefinition> definition, mc::IWorld* world, mc::ecs::EntityRegistry& registry)
        : AgeableEntity(mc::EntityInstanceId(0), registry)
        , m_definition(std::move(definition))
    {
        // 构造期虚函数不派发到派生类，LivingEntity 构造里调到的 registerAttributes() 解析到
        // LivingEntity 版本而非 MobEntity 版本，导致 MOB 层属性（如 FOLLOW_RANGE）缺失——
        // 表现是 getValue(FOLLOW_RANGE) 返回 0 而不是默认 16。这里显式再调一次补上。
        registerAttributes();
        setWorld(world);
    }

    /**
     * @brief 注册 addon 生物需要的属性
     *
     * 除基类已注册的属性外，还要补 `attack_damage`：基岩实体的 `minecraft:attack.damage`
     * 映射到该属性，而 `MobEntity::registerAttributes` 只注册了 `follow_range`，
     * 具体生物类型各自注册攻击伤害。不注册的话 setBaseValue 无处落地，
     * `getAttributeValue(ATTACK_DAMAGE)` 恒为 0（表现为"这个怪打人不掉血"）。
     */
    void registerAttributes() override
    {
        mc::AgeableEntity::registerAttributes();
        attributes().registerAttribute(*mc::entity::attribute::Attributes::attackDamage());
    }

    /**
     * @brief 是否允许用命名牌改名
     *
     * 对应 `minecraft:nameable.allow_name_tag_renaming`（默认 true）。为 false 时命名牌对该
     * 实体无效。未声明该组件时沿用基类默认（允许）。
     */
    /**
     * @brief 是否可以被拴绳拴住
     *
     * 声明 `minecraft:leashable` 即可被拴（该组件本身不是开关，而是"具备拴绳能力"的标记，
     * 具体开关在 presets/can_be_stolen 等子字段里）。未声明时沿用基类判断：敌对生物不可拴。
     */
    [[nodiscard]] bool canBeLeashed() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        if (components != nullptr && components->has("minecraft:leashable")) {
            return true;
        }
        return mc::AgeableEntity::canBeLeashed();
    }

    bool isNameableByTag() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        const nlohmann::json* nameable = components != nullptr ? components->component("minecraft:nameable") : nullptr;
        if (nameable == nullptr) {
            return mc::AgeableEntity::isNameableByTag();
        }
        return nameable->value("allow_name_tag_renaming", true);
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
        _pushTowardsClosestSpace();
        components->tickEnvironment(*this);
        components->tickTimers(*this);
    }

    /**
     * @brief 卡进方块时把实体推向最近的可容纳空间
     *
     * 对应 `minecraft:physics` 的 `push_towards_closest_space`（默认 false）。该字段为 true 时，
     * 实体若整格陷在方块里（自身碰撞箱与方块碰撞相交），应被推到最近的空位，而不是永久卡住。
     *
     * 搜索范围有意收紧到 ±2 格水平、±1 格垂直：脱困只需要挪出相邻一格，搜索过大的半径
     * 会让实体在一 tick 内"瞬移"很远，反而偏离原版手感，且平白增加每 tick 的碰撞查询次数。
     * 候选按到当前位置的距离平方升序检查，第一个无碰撞的位置即采纳。
     */
    void _pushTowardsClosestSpace()
    {
        if (noClip()) {
            // 无碰撞实体不可能被方块卡住，直接跳过。
            return;
        }
        mc::IWorld* world = this->world();
        if (world == nullptr) {
            return;
        }

        const auto* components = tryGetComponent<AddonEntityComponents>();
        const nlohmann::json* physics = components != nullptr ? components->component("minecraft:physics") : nullptr;
        if (physics == nullptr || !physics->value("push_towards_closest_space", false)) {
            return;
        }

        mc::AxisAlignedBB box = boundingBox();
        if (!world->hasBlockCollision(box)) {
            return;
        }

        const mc::Vector3 origin = position();
        mc::Vector3 best = origin;
        f32 bestDistanceSq = std::numeric_limits<f32>::max();
        bool found = false;
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                for (i32 dx = -2; dx <= 2; ++dx) {
                    if (dx == 0 && dy == 0 && dz == 0) {
                        continue;
                    }
                    const mc::Vector3 candidate(origin.x + static_cast<f32>(dx),
                        origin.y + static_cast<f32>(dy),
                        origin.z + static_cast<f32>(dz));
                    const f32 distanceSq = (candidate.x - origin.x) * (candidate.x - origin.x) +
                        (candidate.y - origin.y) * (candidate.y - origin.y) +
                        (candidate.z - origin.z) * (candidate.z - origin.z);
                    if (distanceSq >= bestDistanceSq) {
                        continue;
                    }
                    mc::AxisAlignedBB candidateBox = box;
                    candidateBox.offset(candidate.x - origin.x, candidate.y - origin.y, candidate.z - origin.z);
                    if (world->hasBlockCollision(candidateBox)) {
                        continue;
                    }
                    best = candidate;
                    bestDistanceSq = distanceSq;
                    found = true;
                }
            }
        }

        if (found) {
            setPosition(best);
        }
    }

    bool isImmuneToFire() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        return components != nullptr ? components->has("minecraft:fire_immune") : m_definition->fireImmune();
    }

    /**
     * @brief 是否可被其他实体推动
     *
     * 对应 `minecraft:pushable` 的 `is_pushable`（默认 true）。该判定不只影响生物互相推挤：
     * `isPushable` 为真的实体才计入实体碰撞，进而影响"能否在该位置放置方块"。
     * 未声明该组件时沿用基类默认值。
     */
    bool isPushable() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        const nlohmann::json* pushable = components != nullptr ? components->component("minecraft:pushable") : nullptr;
        if (pushable == nullptr) {
            return mc::CreatureEntity::isPushable();
        }
        return pushable->value("is_pushable", true);
    }

    /**
     * @brief 被活塞推动时的反应
     *
     * 对应 `minecraft:pushable` 的 `is_pushable_by_piston`（默认 true）。为 false 时返回
     * `Ignore`，活塞扫过时跳过该实体；其余情况沿用基类反应。
     */
    PushReaction getPushReaction() const override
    {
        const auto* components = tryGetComponent<AddonEntityComponents>();
        const nlohmann::json* pushable = components != nullptr ? components->component("minecraft:pushable") : nullptr;
        if (pushable == nullptr || pushable->value("is_pushable_by_piston", true)) {
            return mc::CreatureEntity::getPushReaction();
        }
        return PushReaction::Ignore;
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
    // is_spawnable 此前只校验了类型就丢弃，导致该字段对运行时毫无影响。
    // 保存下来供自然生成/刷怪蛋逻辑判断；真正生成刷怪蛋仍是 TODO（见 _initialize）。
    result.m_canSpawn = description.value("is_spawnable", false);
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
            // 三个字段都在 AddonEntityComponents::_apply 里落地：has_gravity → setNoGravity、
            // has_collision → setNoClip、push_towards_closest_space → 卡进方块时的脱困推挤
            // （见 _AddonMob::_pushTowardsClosestSpace）。
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
            // remove_child_entities 在 AddonEntityComponents::despawnIfRequested 里消费：
            // 为 true 时连同拴在本实体身上的子实体一起 despawn。
        } else if (name == "minecraft:fire_immune") {
            MC_TRY(_members(component, {}));
        } else if (name == "minecraft:is_baby") {
            // 纯标记组件：存在即表示按幼体生成。实际生效在 _initialize 里调 AgeableEntity::setChild。
            MC_TRY(_members(component, {}));
        } else if (name == "minecraft:cannot_be_attacked") {
            // 纯标记组件：存在即表示不可被攻击。生效方式是 Entity::setInvulnerable(true)，
            // 这会同时挡住近战/弹射物/爆炸等常规伤害来源（与基岩语义一致）。
            MC_TRY(_members(component, {}));
        } else if (name == "minecraft:can_climb") {
            // 纯标记组件：存在即表示可攀爬梯子/藤蔓。攀爬判定读的是实体的攀爬状态，
            // 因此这里只做接受，运行期由物理/移动逻辑沿用既有 canClimb 路径。
            MC_TRY(_members(component, {}));
        } else if (name == "minecraft:nameable") {
            // allow_name_tag_renaming 由 _AddonMob::isNameableByTag 消费（NameTagItem 查询）。
            // always_show 是纯客户端展示（名字常显），客户端已停止维护，因此只校验不落地。
            // default_trigger / name_actions 需要"命名时触发事件"的挂接点，见下方 TODO。
            MC_TRY(_members(component, {"allow_name_tag_renaming", "always_show", "default_trigger", "name_actions"}));
            MC_TRY(_boolean(component, "allow_name_tag_renaming"));
            MC_TRY(_boolean(component, "always_show"));
            if (const auto trigger = component.find("default_trigger"); trigger != component.end() &&
                (!trigger->is_string() || trigger->get_ref<const std::string&>().empty())) {
                return Error(ErrorCode::InvalidData, "minecraft:nameable default_trigger must be a nonempty string");
            }
            // TODO: name_actions（按名字匹配触发事件）与 default_trigger 需要"实体获得名字"这一
            //       事件源。目前 NameTagItem 直接写名字，没有统一的命名事件派发点，待补。
        } else if (name == "minecraft:leashable") {
            // 官方 schema 的字段分三类：
            //   - can_be_cut / can_be_stolen / unleash_on_removal / on_unleash_interact_only：布尔，校验；
            //   - soft_distance / hard_distance / max_distance：整数距离，校验非负；
            //   - presets（按 filter 选弹簧参数）与 on_leash / on_unleash 事件：需要弹簧物理与
            //     命名事件派发，尚未落地，见下方 TODO。
            // 实际生效的只有"可否被拴"这一条（由 _AddonMob::canBeLeashed 消费）。
            MC_TRY(_members(component,
                {"can_be_cut",
                    "can_be_stolen",
                    "hard_distance",
                    "max_distance",
                    "on_leash",
                    "on_unleash",
                    "on_unleash_interact_only",
                    "presets",
                    "soft_distance",
                    "unleash_on_removal"}));
            for (const char* field :
                {"can_be_cut", "can_be_stolen", "unleash_on_removal", "on_unleash_interact_only"}) {
                MC_TRY(_boolean(component, field));
            }
            for (const char* field : {"soft_distance", "hard_distance", "max_distance"}) {
                if (const auto it = component.find(field); it != component.end()) {
                    if (!it->is_number_integer()) {
                        return Error(
                            ErrorCode::InvalidData, std::string("minecraft:leashable requires an integer: ") + field);
                    }
                    if (it->get<i64>() < 0) {
                        return Error(ErrorCode::InvalidData,
                            std::string("minecraft:leashable distance cannot be negative: ") + field);
                    }
                }
            }
            if (const auto presets = component.find("presets"); presets != component.end() && !presets->is_array()) {
                return Error(ErrorCode::InvalidData, "minecraft:leashable presets must be an array");
            }
            // TODO: presets 的弹簧参数（soft/hard/max_distance、spring_type、rotation_adjustment）
            //   需要拴绳弹簧物理，目前拴绳只做位置跟随，没有弹簧模型；on_leash / on_unleash 需要
            //   "实体被拴/解拴"这一事件派发点，当前 LeadItem 直接改状态、无统一事件源。
            //   在此之前这些字段只做类型校验，不生效。
        } else if (name == "minecraft:inventory") {
            // 6 个字段与官方 schema 对齐。inventory_size 是核心（默认 5）。
            // TODO: 目前只做解析与校验，实体尚未真正持有库存容器——addon 实体的可交互背包
            //   需要 IInventory 实例 + NBT 序列化 + 死亡掉落 + 容器菜单接入，属独立特性。
            //   在此之前脚本 query.has_component("minecraft:inventory") 会返回 false（见
            //   EntityStateQuery），与"组件已声明但查不到"的自洽性由该处一并维持。
            MC_TRY(_members(component,
                {"additional_slots_per_strength",
                    "can_be_siphoned_from",
                    "container_type",
                    "inventory_size",
                    "private",
                    "restrict_to_owner"}));
            for (const char* field : {"additional_slots_per_strength", "inventory_size"}) {
                if (component.contains(field) && !component[field].is_number_integer()) {
                    return Error(
                        ErrorCode::InvalidData, std::string("minecraft:inventory requires an integer: ") + field);
                }
            }
            // 槽位数不能为负：负值会让后续按该值分配的容器直接失效。
            if (const auto size = component.find("inventory_size"); size != component.end() && size->get<i64>() < 0) {
                return Error(ErrorCode::InvalidData, "minecraft:inventory inventory_size cannot be negative");
            }
            if (const auto extra = component.find("additional_slots_per_strength");
                extra != component.end() && extra->get<i64>() < 0) {
                return Error(
                    ErrorCode::InvalidData, "minecraft:inventory additional_slots_per_strength cannot be negative");
            }
            for (const char* field : {"can_be_siphoned_from", "private", "restrict_to_owner"}) {
                MC_TRY(_boolean(component, field));
            }
            if (const auto type = component.find("container_type"); type != component.end()) {
                if (!type->is_string() || type->get_ref<const std::string&>().empty()) {
                    return Error(
                        ErrorCode::InvalidData, "minecraft:inventory container_type must be a nonempty string");
                }
            }
        } else if (name == "minecraft:pushable") {
            // 两个字段由 _AddonMob / _AddonActor 的 isPushable / getPushReaction 覆写消费：
            // is_pushable 决定能否被其他实体推动（并影响实体碰撞与方块放置判定），
            // is_pushable_by_piston 为 false 时活塞扫过会跳过该实体。
            MC_TRY(_members(component, {"is_pushable", "is_pushable_by_piston"}));
            MC_TRY(_boolean(component, "is_pushable"));
            MC_TRY(_boolean(component, "is_pushable_by_piston"));
        } else if (name == "minecraft:attack") {
            // damage 支持三种形态：数字、[min, max] 数组、{range_min, range_max} 对象。
            // 属性只能承载单个基值，因此区间形态取中值写基值；区间随机尚无运行时通路，
            // 见 _initialize 里对应的 TODO。
            MC_TRY(_members(component, {"damage", "effect_name", "effect_duration", "effect_amplifier"}));
            if (const auto damage = component.find("damage"); damage != component.end()) {
                MC_TRY(_attackDamageRange(*damage));
            }
            if (const auto effect = component.find("effect_name"); effect != component.end()) {
                if (!effect->is_string() || effect->get_ref<const std::string&>().empty()) {
                    return Error(ErrorCode::InvalidData, "minecraft:attack effect_name must be a nonempty string");
                }
            }
            if (const auto duration = component.find("effect_duration"); duration != component.end()) {
                if (!duration->is_number() || !std::isfinite(duration->get<double>()) ||
                    duration->get<double>() < 0.0) {
                    return Error(
                        ErrorCode::InvalidData, "minecraft:attack effect_duration must be a nonnegative number");
                }
            }
            if (const auto amplifier = component.find("effect_amplifier"); amplifier != component.end()) {
                if (!amplifier->is_number_integer() || amplifier->get<i64>() < 0) {
                    return Error(
                        ErrorCode::InvalidData, "minecraft:attack effect_amplifier must be a nonnegative integer");
                }
            }
        } else if (name == "minecraft:follow_range") {
            // value 映射到 Attributes::FOLLOW_RANGE（单位：方块）。基岩侧 max 是"最大追击距离"
            // 的上限设定，运行时用于 cap 目标搜索；项目属性模型只有一个上限为 2048 的 follow_range，
            // 因此 value 写基值，max 仅校验范围不参与计算。
            // 与 knockback_resistance 同理，只接受字面量数字：属性基值是实例化时的常量，
            // 没有 Molang/range 的求值通路，收下不生效比报错更难排查。
            MC_TRY(_members(component, {"value", "max"}));
            const auto bounds = mc::entity::attribute::Attributes::followRange();
            for (const char* field : {"value", "max"}) {
                const auto it = component.find(field);
                if (it == component.end()) {
                    continue;
                }

                if (!it->is_number()) {
                    return Error(ErrorCode::Unsupported,
                        std::string("minecraft:follow_range requires a literal number: ") + field);
                }

                const double number = it->get<double>();
                if (!std::isfinite(number) || number < bounds->minValue() || number > bounds->maxValue()) {
                    return Error(
                        ErrorCode::InvalidData, std::string("minecraft:follow_range is out of range: ") + field);
                }
            }
        } else if (name == "minecraft:knockback_resistance") {
            // value 映射到 Attributes::KNOCKBACK_RESISTANCE（0.0 无抗性，1.0 完全免疫）。
            // 基岩侧 max 是"潜在上限"（用于显示/成长预留），运行时生效的是 value，
            // 因此只落 value；max 仅校验类型，不参与计算。
            //
            // 这里显式要求字面量数字：_number 虽已支持 range 对象与 Molang，但属性基值是
            // 实例化时一次性写入的常量，没有"每 tick 重新求值"的机制。若放任 range/Molang 通过，
            // 组件会被收下却永远不生效——那比直接报错更难排查。真正支持动态值需要先有
            // 属性绑定 Molang 的求值通路，属于另一件事。
            MC_TRY(_members(component, {"value", "max"}));
            const auto bounds = mc::entity::attribute::Attributes::knockbackResistance();
            for (const char* field : {"value", "max"}) {
                const auto it = component.find(field);
                if (it == component.end()) {
                    continue;
                }

                if (!it->is_number()) {
                    return Error(ErrorCode::Unsupported,
                        std::string("minecraft:knockback_resistance requires a literal number: ") + field);
                }

                const double number = it->get<double>();
                if (!std::isfinite(number) || number < bounds->minValue() || number > bounds->maxValue()) {
                    return Error(ErrorCode::InvalidData,
                        std::string("minecraft:knockback_resistance is out of range: ") + field);
                }
            }
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

    // minecraft:is_baby 是纯标记组件，存在即按幼体生成。living 型 addon 实体（_AddonMob）
    // 继承 AgeableEntity，因此能真正落地（体型经 babyScale 缩放、可成长）。
    // 非 living 型（_AddonActor，无年龄概念）上挂该组件无意义，记 warn 而不报错——基岩侧同样
    // 不会因为把组件放在无年龄实体上就拒绝整个行为包。
    if (m_components.contains("minecraft:is_baby")) {
        if (auto* ageable = dynamic_cast<mc::AgeableEntity*>(&entity); ageable != nullptr) {
            ageable->setChild(true);
        } else {
            spdlog::warn("[BedrockAddon] minecraft:is_baby on non-ageable entity '{}' has no effect", m_identifier);
        }
    }

    // minecraft:cannot_be_attacked 是纯标记组件，存在即不可被攻击。
    // 走 Entity::setInvulnerable(true)，与"无敌"共用同一套伤害拦截，不额外造一条并行判定。
    if (m_components.contains("minecraft:cannot_be_attacked")) {
        entity.setInvulnerable(true);
    }

    // minecraft:follow_range.value → 追击范围属性。只有 LivingEntity 有属性表。
    if (const auto it = m_components.find("minecraft:follow_range"); it != m_components.end()) {
        if (auto* living = dynamic_cast<mc::LivingEntity*>(&entity); living != nullptr) {
            if (const auto value = it->find("value"); value != it->end() && value->is_number()) {
                living->attributes().setBaseValue(
                    mc::entity::attribute::Attributes::FOLLOW_RANGE, value->get<double>());
            }
        }
    }

    // minecraft:knockback_resistance.value → 击退抗性属性。只有 LivingEntity 有属性表。
    if (const auto it = m_components.find("minecraft:knockback_resistance"); it != m_components.end()) {
        if (auto* living = dynamic_cast<mc::LivingEntity*>(&entity); living != nullptr) {
            // 允许 range 对象/Molang 之外的纯字面量；非数字形式已在 validateComponents 阶段拒绝。
            if (const auto value = it->find("value"); value != it->end() && value->is_number()) {
                living->attributes().setBaseValue(
                    mc::entity::attribute::Attributes::KNOCKBACK_RESISTANCE, value->get<double>());
            }
        }
    }

    // minecraft:attack.damage → 攻击伤害属性。只有 LivingEntity 有属性表。
    if (const auto it = m_components.find("minecraft:attack"); it != m_components.end()) {
        if (auto* living = dynamic_cast<mc::LivingEntity*>(&entity); living != nullptr) {
            if (const auto damage = it->find("damage"); damage != it->end()) {
                if (const auto representative = _attackDamageRepresentative(*damage); representative.has_value()) {
                    const double clamped = std::clamp(*representative, 0.0, 2048.0);
                    if (clamped != *representative) {
                        // 负 damage（基岩语义：治疗目标）与超过属性上限的值无法写进属性基值。
                        // 这里记 warn 并夹到属性可达范围，而不是静默丢弃整条攻击配置。
                        spdlog::warn("[BedrockAddon] minecraft:attack damage {} on '{}' is outside the "
                                     "attack_damage attribute range, clamping to {}",
                            *representative,
                            m_identifier,
                            clamped);
                    }
                    living->attributes().setBaseValue(mc::entity::attribute::Attributes::ATTACK_DAMAGE, clamped);
                }
            }
            // TODO: effect_name / effect_duration / effect_amplifier 需要"近战命中后对目标施加状态
            //       效果"的挂接点；addon 实体的近战路径尚未接入完整 doHurtTarget 管线，因此这三项
            //       目前只做校验。务必 warn 而不是静默——否则服主会以为效果已生效。
            if (it->contains("effect_name")) {
                spdlog::warn("[BedrockAddon] minecraft:attack effect_name on '{}' is validated but not applied: "
                             "melee effect application is not wired for addon mobs yet",
                    m_identifier);
            }
        } else {
            spdlog::warn("[BedrockAddon] minecraft:attack on non-living entity '{}' has no effect", m_identifier);
        }
    }
    // TODO: 为 Java 客户端实现自定义类型投影、资源包模型与 client_sync 属性同步。
    // TODO: 按 is_spawnable 生成对应的自定义刷怪蛋及生成规则关联。
    //   is_spawnable 本身已保存（见 canSpawn()），缺的是刷怪蛋物品注册与生成规则挂接。
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

bool AddonEntityDefinition::canSpawn() const
{
    return m_canSpawn;
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
