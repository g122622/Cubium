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

#include "EntityTriggers.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/CriterionContext.hpp"
#include "common/advancement/trigger/conditions/DamagePredicate.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include <algorithm>
#include <memory>
#include <utility>
#include <nlohmann/json_fwd.hpp>

// 注意：trigger() 方法的完整实现需要服务端模块的支持
// 服务端代码应包含 server/advancement/TriggerInstantiation.hpp
// 并使用 triggerWithPredicate() 方法或直接调用基类的 trigger() 模板方法

namespace mc::advancement {

// ========== TameAnimalTriggerInstance ==========

TameAnimalTriggerInstance::TameAnimalTriggerInstance(EntityPredicate entity)
    : m_entity(std::move(entity))
{}

bool TameAnimalTriggerInstance::test(const Entity& entity) const
{
    return m_entity.test(entity);
}

bool TameAnimalTriggerInstance::test(const CriterionContext& context) const
{
    return m_entityContext.test(context);
}

Result<void> TameAnimalTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("entity")) {
        auto contextResult = ContextAwarePredicate::fromJson(json["entity"]);
        if (contextResult.failed()) {
            return contextResult.error();
        }
        m_entityContext = std::move(contextResult.value());
        if (json["entity"].is_object()) {
            auto result = EntityPredicate::fromJson(json["entity"]);
            if (result.failed()) {
                return result.error();
            }
            m_entity = result.value();
        }
    }

    return {};
}

nlohmann::json TameAnimalTriggerInstance::conditionsToJson() const
{
    if (!m_entity.isAny()) {
        return {{"entity", m_entity.toJson()}};
    }
    return nullptr;
}

// ========== TameAnimalTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> TameAnimalTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<TameAnimalTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void TameAnimalTrigger::trigger(ServerPlayer& player, const Entity& entity)
{
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端通过 AdvancementEventHandler::_onTameAnimal() 订阅 TameAnimalEvent，
    // 然后直接调用 AbstractCriterionTrigger<TameAnimalTriggerInstance>::trigger() 触发检测。
    // 游戏逻辑通过 IWorld::onTameAnimal() -> ServerWorld::onTameAnimal() 发布事件。
    // 触发场景：鹦鹉驯服 (ParrotEntity::interactMob)、马驯服 (AbstractHorseEntity::setTamedBy) 等。
    MC_UNUSED(player);
    MC_UNUSED(entity);
}

// ========== BredAnimalsTriggerInstance ==========

BredAnimalsTriggerInstance::BredAnimalsTriggerInstance(
    EntityPredicate child, EntityPredicate parent, EntityPredicate partner)
    : m_child(std::move(child))
    , m_parent(std::move(parent))
    , m_partner(std::move(partner))
{}

bool BredAnimalsTriggerInstance::test(const Entity& child, const Entity& parent, const Entity& partner) const
{
    if (!m_child.test(child)) {
        return false;
    }
    if (!m_parent.test(parent)) {
        return false;
    }
    if (!m_partner.test(partner)) {
        return false;
    }
    return true;
}

bool BredAnimalsTriggerInstance::test(const CriterionContext& childContext,
    const CriterionContext& parentContext,
    const CriterionContext& partnerContext) const
{
    return m_childContext.test(childContext) && m_parentContext.test(parentContext) &&
        m_partnerContext.test(partnerContext);
}

Result<void> BredAnimalsTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    // 条件列表写法（ContextAwarePredicate）优先解析到对应上下文
    const auto parseContext = [](const nlohmann::json& value, ContextAwarePredicate& out) -> Result<void> {
        auto result = ContextAwarePredicate::fromJson(value);
        if (result.failed()) {
            return result.error();
        }
        out = std::move(result.value());
        return {};
    };
    if (json.contains("child")) {
        if (auto r = parseContext(json["child"], m_childContext); r.failed()) {
            return r.error();
        }
    }
    if (json.contains("parent")) {
        if (auto r = parseContext(json["parent"], m_parentContext); r.failed()) {
            return r.error();
        }
    }
    if (json.contains("partner")) {
        if (auto r = parseContext(json["partner"], m_partnerContext); r.failed()) {
            return r.error();
        }
    }

    if (json.contains("child")) {
        auto result = EntityPredicate::fromJson(json["child"]);
        if (result.failed()) {
            return result.error();
        }
        m_child = result.value();
    }

    if (json.contains("parent")) {
        auto result = EntityPredicate::fromJson(json["parent"]);
        if (result.failed()) {
            return result.error();
        }
        m_parent = result.value();
    }

    if (json.contains("partner")) {
        auto result = EntityPredicate::fromJson(json["partner"]);
        if (result.failed()) {
            return result.error();
        }
        m_partner = result.value();
    }

    return {};
}

nlohmann::json BredAnimalsTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_child.isAny()) {
        json["child"] = m_child.toJson();
    }
    if (!m_parent.isAny()) {
        json["parent"] = m_parent.toJson();
    }
    if (!m_partner.isAny()) {
        json["partner"] = m_partner.toJson();
    }

    return json;
}

// ========== BredAnimalsTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> BredAnimalsTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<BredAnimalsTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void BredAnimalsTrigger::trigger(ServerPlayer& player, const Entity& child, const Entity& parent, const Entity& partner)
{
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端通过 AdvancementEventHandler::_onBredAnimals() 订阅 BredAnimalsEvent，
    // 然后直接调用 AbstractCriterionTrigger<BredAnimalsTriggerInstance>::trigger() 触发检测。
    // 游戏逻辑通过 IWorld::onBredAnimals() -> ServerWorld::onBredAnimals() 发布事件。
    MC_UNUSED(player);
    MC_UNUSED(child);
    MC_UNUSED(parent);
    MC_UNUSED(partner);
}

// ========== SummonedEntityTriggerInstance ==========

SummonedEntityTriggerInstance::SummonedEntityTriggerInstance(EntityPredicate entity)
    : m_entity(std::move(entity))
{}

bool SummonedEntityTriggerInstance::test(const Entity& entity) const
{
    return m_entity.test(entity);
}

Result<void> SummonedEntityTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("entity")) {
        auto contextResult = ContextAwarePredicate::fromJson(json["entity"]);
        if (contextResult.failed()) {
            return contextResult.error();
        }
        m_entityContext = std::move(contextResult.value());
        if (json["entity"].is_object()) {
            auto result = EntityPredicate::fromJson(json["entity"]);
            if (result.failed()) {
                return result.error();
            }
            m_entity = result.value();
        }
    }

    return {};
}

bool SummonedEntityTriggerInstance::test(const CriterionContext& context) const
{
    return m_entityContext.test(context);
}

nlohmann::json SummonedEntityTriggerInstance::conditionsToJson() const
{
    if (!m_entity.isAny()) {
        return {{"entity", m_entity.toJson()}};
    }
    return nullptr;
}

// ========== SummonedEntityTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> SummonedEntityTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<SummonedEntityTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void SummonedEntityTrigger::trigger(ServerPlayer& player, const Entity& entity)
{
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端通过 AdvancementEventHandler::_onSummonedEntity() 订阅 SummonedEntityEvent，
    // 然后直接调用 AbstractCriterionTrigger<SummonedEntityTriggerInstance>::trigger() 触发检测。
    // 游戏逻辑通过 IWorld::onSummonedEntity() -> ServerWorld::onSummonedEntity() 发布事件。
    // 触发场景：/summon 命令、建造铁傀儡/雪傀儡、建造凋灵、重生末影龙等。
    MC_UNUSED(player);
    MC_UNUSED(entity);
}

// ========== CuredZombieVillagerTriggerInstance ==========

CuredZombieVillagerTriggerInstance::CuredZombieVillagerTriggerInstance(EntityPredicate zombie, EntityPredicate villager)
    : m_zombie(std::move(zombie))
    , m_villager(std::move(villager))
{}

bool CuredZombieVillagerTriggerInstance::test(const Entity& zombie, const Entity& villager) const
{
    if (!m_zombie.test(zombie)) {
        return false;
    }
    if (!m_villager.test(villager)) {
        return false;
    }
    return true;
}

bool CuredZombieVillagerTriggerInstance::test(
    const CriterionContext& zombieContext, const CriterionContext& villagerContext) const
{
    return m_zombieContext.test(zombieContext) && m_villagerContext.test(villagerContext);
}

Result<void> CuredZombieVillagerTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("zombie")) {
        auto contextResult = ContextAwarePredicate::fromJson(json["zombie"]);
        if (contextResult.failed()) {
            return contextResult.error();
        }
        m_zombieContext = std::move(contextResult.value());
        if (json["zombie"].is_object()) {
            auto result = EntityPredicate::fromJson(json["zombie"]);
            if (result.failed()) {
                return result.error();
            }
            m_zombie = result.value();
        }
    }

    if (json.contains("villager")) {
        auto contextResult = ContextAwarePredicate::fromJson(json["villager"]);
        if (contextResult.failed()) {
            return contextResult.error();
        }
        m_villagerContext = std::move(contextResult.value());
        if (json["villager"].is_object()) {
            auto result = EntityPredicate::fromJson(json["villager"]);
            if (result.failed()) {
                return result.error();
            }
            m_villager = result.value();
        }
    }

    return {};
}

nlohmann::json CuredZombieVillagerTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_zombie.isAny()) {
        json["zombie"] = m_zombie.toJson();
    }
    if (!m_villager.isAny()) {
        json["villager"] = m_villager.toJson();
    }

    return json;
}

// ========== CuredZombieVillagerTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> CuredZombieVillagerTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<CuredZombieVillagerTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void CuredZombieVillagerTrigger::trigger(ServerPlayer& player, const Entity& zombie, const Entity& villager)
{
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端通过 AdvancementEventHandler::_onCuredZombieVillager() 订阅 CuredZombieVillagerEvent，
    // 然后直接调用 AbstractCriterionTrigger<CuredZombieVillagerTriggerInstance>::trigger() 触发检测。
    // 游戏逻辑通过 IWorld::onZombieVillagerCured() -> ServerWorld::onZombieVillagerCured() 发布事件。
    MC_UNUSED(player);
    MC_UNUSED(zombie);
    MC_UNUSED(villager);
}

// ========== VillagerTradeTriggerInstance ==========

VillagerTradeTriggerInstance::VillagerTradeTriggerInstance(EntityPredicate villager, ItemPredicate item)
    : m_villager(std::move(villager))
    , m_item(std::move(item))
{}

bool VillagerTradeTriggerInstance::test(const Entity& villager, const ItemStack& item) const
{
    if (!m_villager.test(villager)) {
        return false;
    }
    if (!m_item.test(item)) {
        return false;
    }
    return true;
}

Result<void> VillagerTradeTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("villager")) {
        auto result = EntityPredicate::fromJson(json["villager"]);
        if (result.failed()) {
            return result.error();
        }
        m_villager = result.value();
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    return {};
}

nlohmann::json VillagerTradeTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_villager.isAny()) {
        json["villager"] = m_villager.toJson();
    }
    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }

    return json;
}

// ========== VillagerTradeTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> VillagerTradeTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<VillagerTradeTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void VillagerTradeTrigger::trigger(ServerPlayer& player, const Entity& villager, const ItemStack& item)
{
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端代码通过 AdvancementEventHandler::_onVillagerTrade() 触发，
    // 由 IWorld::onVillagerTrade() -> ServerWorld::onVillagerTrade() -> VillagerTradeEvent 链路触发。
    MC_UNUSED(player);
    MC_UNUSED(villager);
    MC_UNUSED(item);
}

// ========== PlayerInteractedWithEntityTriggerInstance ==========

PlayerInteractedWithEntityTriggerInstance::PlayerInteractedWithEntityTriggerInstance(
    ItemPredicate item, EntityPredicate entity)
    : m_item(std::move(item))
    , m_entity(std::move(entity))
{}

bool PlayerInteractedWithEntityTriggerInstance::test(const ItemStack& item, const Entity& entity) const
{
    if (!m_item.isAny() && !m_item.test(item)) {
        return false;
    }
    // 条件列表写法（ContextAwarePredicate）在无上下文时按实体判定；裸谓词写法仍走 m_entity
    CriterionContext context;
    context.entity = &entity;
    context.item = &item;
    return m_entityContext.test(context);
}

Result<void> PlayerInteractedWithEntityTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    if (json.contains("entity")) {
        // 兼容两种写法：裸 EntityPredicate 对象，或 ContextAwarePredicate 条件列表
        auto result = ContextAwarePredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entityContext = std::move(result.value());
        if (json["entity"].is_object()) {
            auto entityResult = EntityPredicate::fromJson(json["entity"]);
            if (entityResult.failed()) {
                return entityResult.error();
            }
            m_entity = entityResult.value();
        }
    }

    return {};
}

bool PlayerInteractedWithEntityTriggerInstance::test(const CriterionContext& context) const
{
    if (context.item != nullptr && !m_item.test(*context.item)) {
        return false;
    }
    return m_entityContext.test(context);
}

nlohmann::json PlayerInteractedWithEntityTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }
    if (!m_entity.isAny()) {
        json["entity"] = m_entity.toJson();
    }

    return json;
}

// ========== PlayerInteractedWithEntityTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> PlayerInteractedWithEntityTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<PlayerInteractedWithEntityTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void PlayerInteractedWithEntityTrigger::trigger(ServerPlayer& player, const ItemStack& item, const Entity& entity)
{
    // common 模块无法访问 PlayerAdvancements 完整定义，故此便捷方法为空桩。
    // 实际触发由 ServerPlayHandler::_triggerPlayerInteractedWithEntity 在 INTERACT/
    // INTERACT_AT 返回 Success 时，按 AdvancementEventHandler 既定模式直接调用
    // AbstractCriterionTrigger<PlayerInteractedWithEntityTriggerInstance>::trigger()
    // 模板（见 server/advancement/TriggerInstantiation.hpp）。保留此空方法供未来
    // common 层封装后切换调用点。
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(entity);
}

// ========== PlayerHurtEntityTrigger ==========

bool PlayerHurtEntityTriggerInstance::test(
    const DamageSource& source, f32 dealt, f32 taken, bool blocked, const Entity& entity) const
{
    // 对齐 vanilla matches：damage 不匹配直接失败；entity 缺省则匹配任意实体
    if (m_damage.has_value() && !m_damage->test(source, dealt, taken, blocked)) {
        return false;
    }
    if (m_entity.has_value() && !m_entity->test(entity)) {
        return false;
    }
    return true;
}

Result<void> PlayerHurtEntityTriggerInstance::fromJson(const nlohmann::json& json)
{
    m_damage.reset();
    m_entity.reset();
    if (json.is_null()) {
        return {};
    }
    if (json.contains("damage")) {
        auto result = DamagePredicate::fromJson(json["damage"]);
        if (result.failed()) {
            return result.error();
        }
        m_damage = std::move(result.value());
    }
    if (json.contains("entity")) {
        auto result = EntityPredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entity = std::move(result.value());
    }
    // TODO(player-predicate): vanilla 条件还含可选的 player（ContextAwarePredicate）字段，
    //  项目尚未实现基于玩家状态（等级/游戏模式/统计）的玩家谓词，暂忽略。
    return {};
}

nlohmann::json PlayerHurtEntityTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;
    if (m_damage.has_value() && !m_damage->isAny()) {
        json["damage"] = m_damage->toJson();
    }
    if (m_entity.has_value() && !m_entity->isAny()) {
        json["entity"] = m_entity->toJson();
    }
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> PlayerHurtEntityTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<PlayerHurtEntityTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void PlayerHurtEntityTrigger::trigger(
    ServerPlayer& player, const Entity& entity, const DamageSource& source, f32 dealt, f32 taken, bool blocked)
{
    // 与其它触发器一致：实际派发在 ServerPlayer::triggerPlayerHurtEntity 经
    // AbstractCriterionTrigger<PlayerHurtEntityTriggerInstance>::trigger 完成。
    MC_UNUSED(player);
    MC_UNUSED(entity);
    MC_UNUSED(source);
    MC_UNUSED(dealt);
    MC_UNUSED(taken);
    MC_UNUSED(blocked);
}

// ========== EntityHurtPlayerTrigger ==========

bool EntityHurtPlayerTriggerInstance::test(const DamageSource& source, f32 dealt, f32 taken, bool blocked) const
{
    return !m_damage.has_value() || m_damage->test(source, dealt, taken, blocked);
}

Result<void> EntityHurtPlayerTriggerInstance::fromJson(const nlohmann::json& json)
{
    m_damage.reset();
    if (json.is_null()) {
        return {};
    }
    if (json.contains("damage")) {
        auto result = DamagePredicate::fromJson(json["damage"]);
        if (result.failed()) {
            return result.error();
        }
        m_damage = std::move(result.value());
    }
    // TODO(player-predicate): vanilla 条件还含可选的 player（ContextAwarePredicate）字段，暂忽略。
    return {};
}

nlohmann::json EntityHurtPlayerTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;
    if (m_damage.has_value() && !m_damage->isAny()) {
        json["damage"] = m_damage->toJson();
    }
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> EntityHurtPlayerTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<EntityHurtPlayerTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EntityHurtPlayerTrigger::trigger(
    ServerPlayer& player, const DamageSource& source, f32 dealt, f32 taken, bool blocked)
{
    // 实际派发由 ServerPlayer::triggerEntityHurtPlayer 经模板方法完成
    MC_UNUSED(player);
    MC_UNUSED(source);
    MC_UNUSED(dealt);
    MC_UNUSED(taken);
    MC_UNUSED(blocked);
}

// ========== ChangedDimensionTrigger ==========

bool ChangedDimensionTriggerInstance::test(const ResourceLocation& from, const ResourceLocation& to) const
{
    if (m_from.has_value() && m_from.value() != from) {
        return false;
    }
    return !m_to.has_value() || m_to.value() == to;
}

Result<void> ChangedDimensionTriggerInstance::fromJson(const nlohmann::json& json)
{
    m_from.reset();
    m_to.reset();
    if (json.is_null()) {
        return {};
    }
    if (json.contains("from") && json["from"].is_string()) {
        m_from = ResourceLocation(json["from"].get<std::string>());
    }
    if (json.contains("to") && json["to"].is_string()) {
        m_to = ResourceLocation(json["to"].get<std::string>());
    }
    // TODO(player-predicate): vanilla 条件还含可选的 player（ContextAwarePredicate）字段，暂忽略。
    return {};
}

nlohmann::json ChangedDimensionTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;
    if (m_from.has_value()) {
        json["from"] = m_from->toString();
    }
    if (m_to.has_value()) {
        json["to"] = m_to->toString();
    }
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> ChangedDimensionTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<ChangedDimensionTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void ChangedDimensionTrigger::trigger(ServerPlayer& player, const ResourceLocation& from, const ResourceLocation& to)
{
    // 实际派发由 ServerPlayer::triggerChangedDimension 经模板方法完成
    MC_UNUSED(player);
    MC_UNUSED(from);
    MC_UNUSED(to);
}

// ========== FishingRodHookedTrigger ==========

bool FishingRodHookedTriggerInstance::test(
    const ItemStack& rod, const Entity* hookedEntity, const std::vector<ItemStack>& items) const
{
    // 对齐 vanilla matches(rod, lootContext, items)：
    //   rod 不匹配即失败；entity 不匹配即失败；item 需命中钩住的 ItemEntity 或本次掉落物之一。
    if (m_rod.has_value() && !m_rod->test(rod)) {
        return false;
    }
    if (m_entity.has_value()) {
        if (hookedEntity == nullptr || !m_entity->test(*hookedEntity)) {
            return false;
        }
    }
    if (m_item.has_value()) {
        bool matched = false;
        if (hookedEntity != nullptr) {
            if (const auto* itemEntity = dynamic_cast<const ItemEntity*>(hookedEntity);
                itemEntity != nullptr && m_item->test(itemEntity->getItemStack())) {
                matched = true;
            }
        }
        if (!matched) {
            for (const auto& stack : items) {
                if (m_item->test(stack)) {
                    matched = true;
                    break;
                }
            }
        }
        if (!matched) {
            return false;
        }
    }
    return true;
}

Result<void> FishingRodHookedTriggerInstance::fromJson(const nlohmann::json& json)
{
    m_rod.reset();
    m_entity.reset();
    m_item.reset();
    if (json.is_null()) {
        return {};
    }
    if (json.contains("rod")) {
        auto result = ItemPredicate::fromJson(json["rod"]);
        if (result.failed()) {
            return result.error();
        }
        m_rod = std::move(result.value());
    }
    if (json.contains("entity")) {
        auto result = EntityPredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entity = std::move(result.value());
    }
    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = std::move(result.value());
    }
    // TODO(player-predicate): vanilla 条件还含可选的 player（ContextAwarePredicate）字段，暂忽略。
    return {};
}

nlohmann::json FishingRodHookedTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;
    if (m_rod.has_value() && !m_rod->isAny()) {
        json["rod"] = m_rod->toJson();
    }
    if (m_entity.has_value() && !m_entity->isAny()) {
        json["entity"] = m_entity->toJson();
    }
    if (m_item.has_value() && !m_item->isAny()) {
        json["item"] = m_item->toJson();
    }
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> FishingRodHookedTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<FishingRodHookedTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void FishingRodHookedTrigger::trigger(
    ServerPlayer& player, const ItemStack& rod, const Entity* hookedEntity, const std::vector<ItemStack>& items)
{
    // 实际派发由 Player::triggerFishingRodHooked → ServerPlayer 实现经模板方法完成
    MC_UNUSED(player);
    MC_UNUSED(rod);
    MC_UNUSED(hookedEntity);
    MC_UNUSED(items);
}

// ========== KilledByArrowTrigger ==========

KilledByArrowTriggerInstance::KilledByArrowTriggerInstance(
    std::vector<EntityPredicate> victims, IntBounds uniqueEntityTypes, std::optional<ItemPredicate> firedFromWeapon)
    : m_victims(std::move(victims))
    , m_uniqueEntityTypes(std::move(uniqueEntityTypes))
    , m_firedFromWeapon(std::move(firedFromWeapon))
{}

bool KilledByArrowTriggerInstance::test(
    const std::vector<const Entity*>& victims, const ItemStack& firedFromWeapon) const
{
    // 对齐 vanilla matches：武器条件（武器缺失时不匹配带 fired_from_weapon 的条件）
    if (m_firedFromWeapon.has_value()) {
        if (firedFromWeapon.isEmpty() || !m_firedFromWeapon->test(firedFromWeapon)) {
            return false;
        }
    }

    // victims 多重集合匹配：每个谓词须消耗掉一个尚未匹配的受害者
    if (!m_victims.empty()) {
        std::vector<const Entity*> remaining;
        remaining.reserve(victims.size());
        for (const Entity* victim : victims) {
            if (victim != nullptr) {
                remaining.push_back(victim);
            }
        }
        for (const auto& predicate : m_victims) {
            bool matched = false;
            for (auto it = remaining.begin(); it != remaining.end(); ++it) {
                if (predicate.test(**it)) {
                    remaining.erase(it);
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                return false;
            }
        }
    }

    // 去重后的实体类型数量
    if (!m_uniqueEntityTypes.isUnbounded()) {
        std::vector<std::string> typeIds;
        for (const Entity* victim : victims) {
            if (victim == nullptr) {
                continue;
            }
            const std::string& typeId = victim->getTypeId();
            if (std::find(typeIds.begin(), typeIds.end(), typeId) == typeIds.end()) {
                typeIds.push_back(typeId);
            }
        }
        if (!m_uniqueEntityTypes.test(static_cast<i32>(typeIds.size()))) {
            return false;
        }
    }
    return true;
}

Result<void> KilledByArrowTriggerInstance::fromJson(const nlohmann::json& json)
{
    m_victims.clear();
    m_uniqueEntityTypes = IntBounds();
    m_firedFromWeapon.reset();
    if (json.is_null()) {
        return {};
    }
    if (json.contains("victims") && json["victims"].is_array()) {
        for (const auto& entry : json["victims"]) {
            auto result = EntityPredicate::fromJson(entry);
            if (result.failed()) {
                return result.error();
            }
            m_victims.push_back(std::move(result.value()));
        }
    }
    if (json.contains("unique_entity_types")) {
        m_uniqueEntityTypes = IntBounds::fromJson(json["unique_entity_types"]);
    }
    if (json.contains("fired_from_weapon")) {
        auto result = ItemPredicate::fromJson(json["fired_from_weapon"]);
        if (result.failed()) {
            return result.error();
        }
        m_firedFromWeapon = std::move(result.value());
    }
    // TODO(player-predicate): vanilla 条件还含可选的 player（ContextAwarePredicate）字段，暂忽略。
    return {};
}

nlohmann::json KilledByArrowTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;
    if (!m_victims.empty()) {
        nlohmann::json victims = nlohmann::json::array();
        for (const auto& predicate : m_victims) {
            victims.push_back(predicate.toJson());
        }
        json["victims"] = std::move(victims);
    }
    if (!m_uniqueEntityTypes.isUnbounded()) {
        json["unique_entity_types"] = m_uniqueEntityTypes.toJson();
    }
    if (m_firedFromWeapon.has_value() && !m_firedFromWeapon->isAny()) {
        json["fired_from_weapon"] = m_firedFromWeapon->toJson();
    }
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> KilledByArrowTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<KilledByArrowTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void KilledByArrowTrigger::trigger(
    ServerPlayer& player, const std::vector<const Entity*>& victims, const ItemStack& firedFromWeapon)
{
    // 实际派发由 Player::triggerKilledByArrow → ServerPlayer 实现经模板方法完成
    MC_UNUSED(player);
    MC_UNUSED(victims);
    MC_UNUSED(firedFromWeapon);
}

// ========== PlayerInteractTrigger ==========

bool PlayerInteractTriggerInstance::test(const ItemStack& item, const Entity& entity) const
{
    if (!m_item.isAny() && !m_item.test(item)) {
        return false;
    }
    CriterionContext context;
    context.entity = &entity;
    context.item = &item;
    return m_entityContext.test(context);
}

Result<void> PlayerInteractTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }
    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }
    if (json.contains("entity")) {
        auto result = ContextAwarePredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entityContext = std::move(result.value());
    }
    // TODO(player-predicate): vanilla 条件还含可选的 player（ContextAwarePredicate）字段，暂忽略。
    return {};
}

nlohmann::json PlayerInteractTriggerInstance::conditionsToJson() const
{
    nlohmann::json json = nlohmann::json::object();
    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }
    if (!m_entityContext.isAny()) {
        json["entity"] = m_entityContext.toJson();
    }
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> PlayerInteractTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<PlayerInteractTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void PlayerInteractTrigger::trigger(ServerPlayer& player, const ItemStack& item, const Entity& entity)
{
    // 实际派发由 Player::triggerPlayerShearedEquipment → ServerPlayer 实现经模板方法完成
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(entity);
}

// ========== StartRidingTrigger ==========

bool StartRidingTriggerInstance::test(const CriterionContext& context) const
{
    return m_playerContext.test(context);
}

Result<void> StartRidingTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }
    if (json.contains("player")) {
        auto result = ContextAwarePredicate::fromJson(json["player"]);
        if (result.failed()) {
            return result.error();
        }
        m_playerContext = std::move(result.value());
    }
    return {};
}

nlohmann::json StartRidingTriggerInstance::conditionsToJson() const
{
    if (m_playerContext.isAny()) {
        return nlohmann::json::object();
    }
    return nlohmann::json{{"player", m_playerContext.toJson()}};
}

Result<std::shared_ptr<ICriterionInstance>> StartRidingTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<StartRidingTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void StartRidingTrigger::trigger(ServerPlayer& player)
{
    // 实际派发由 Player::triggerStartedRiding → ServerPlayer 实现经模板方法完成
    MC_UNUSED(player);
}

// ========== LightningStrikeTrigger ==========

bool LightningStrikeTriggerInstance::test(const Entity& lightning, const Entity& bystander) const
{
    // 对齐 vanilla matches：lightning 谓词作用于闪电本身，bystander 谓词作用于旁观者
    CriterionContext lightningContext;
    lightningContext.entity = &lightning;
    if (!m_lightningContext.test(lightningContext)) {
        return false;
    }
    CriterionContext bystanderContext;
    bystanderContext.entity = &bystander;
    return m_bystanderContext.test(bystanderContext);
}

Result<void> LightningStrikeTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }
    if (json.contains("lightning")) {
        auto result = ContextAwarePredicate::fromJson(json["lightning"]);
        if (result.failed()) {
            return result.error();
        }
        m_lightningContext = std::move(result.value());
    }
    if (json.contains("bystander")) {
        auto result = ContextAwarePredicate::fromJson(json["bystander"]);
        if (result.failed()) {
            return result.error();
        }
        m_bystanderContext = std::move(result.value());
    }
    // TODO(player-predicate): vanilla 条件还含可选的 player（ContextAwarePredicate）字段，暂忽略。
    return {};
}

nlohmann::json LightningStrikeTriggerInstance::conditionsToJson() const
{
    nlohmann::json json = nlohmann::json::object();
    if (!m_lightningContext.isAny()) {
        json["lightning"] = m_lightningContext.toJson();
    }
    if (!m_bystanderContext.isAny()) {
        json["bystander"] = m_bystanderContext.toJson();
    }
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> LightningStrikeTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<LightningStrikeTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void LightningStrikeTrigger::trigger(ServerPlayer& player, const Entity& lightning, const Entity& bystander)
{
    // 实际派发由 Player::triggerLightningStrike → ServerPlayer 实现经模板方法完成
    MC_UNUSED(player);
    MC_UNUSED(lightning);
    MC_UNUSED(bystander);
}

// ========== DistanceTrigger ==========

DistanceTrigger::DistanceTrigger(const char* triggerId)
    : m_triggerId(triggerId != nullptr ? triggerId : TRIGGER_ID)
{}

bool DistanceTriggerInstance::test(const Vector3& startPos, const Vector3& endPos) const
{
    // 对齐 vanilla DistanceTrigger.TriggerInstance.matches：
    // 先校验起点位置谓词，再校验起止点距离
    if (m_startPosition.has_value() && !m_startPosition->testPosition(startPos.x, startPos.y, startPos.z)) {
        return false;
    }
    if (m_distance.has_value() && !m_distance->test(startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z)) {
        return false;
    }
    return true;
}

bool DistanceTriggerInstance::test(
    const CriterionContext& context, const Vector3& startPos, const Vector3& endPos) const
{
    // 起点位置谓词：有世界时完整判定（对齐 vanilla startPosition.matches(level, pos)）；
    // 无世界上下文（单元测试/无世界场景）无法判定维度与生物群系，按"仅坐标"处理。
    if (m_startPosition.has_value()) {
        if (!m_startPosition->testPosition(startPos.x, startPos.y, startPos.z)) {
            return false;
        }
        if (context.world != nullptr && !m_startPosition->test(*context.world, startPos.x, startPos.y, startPos.z)) {
            return false;
        }
    }
    if (!m_playerContext.test(context)) {
        return false;
    }
    if (m_distance.has_value() && !m_distance->test(startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z)) {
        return false;
    }
    return true;
}

Result<void> DistanceTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }
    if (json.contains("player")) {
        auto result = ContextAwarePredicate::fromJson(json["player"]);
        if (result.failed()) {
            return result.error();
        }
        m_playerContext = std::move(result.value());
    }
    if (json.contains("start_position")) {
        auto result = LocationPredicate::fromJson(json["start_position"]);
        if (result.failed()) {
            return result.error();
        }
        m_startPosition = result.value();
    }
    if (json.contains("distance")) {
        auto result = DistancePredicate::fromJson(json["distance"]);
        if (result.failed()) {
            return result.error();
        }
        m_distance = result.value();
    }
    return {};
}

nlohmann::json DistanceTriggerInstance::conditionsToJson() const
{
    nlohmann::json json = nlohmann::json::object();
    if (!m_playerContext.isAny()) {
        json["player"] = m_playerContext.toJson();
    }
    if (m_startPosition.has_value()) {
        json["start_position"] = m_startPosition->toJson();
    }
    if (m_distance.has_value() && !m_distance->isAny()) {
        json["distance"] = m_distance->toJson();
    }
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> DistanceTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<DistanceTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void DistanceTrigger::trigger(ServerPlayer& player, const Vector3& startPos, const Vector3& endPos)
{
    // 实际派发由 Player::triggerDistance → ServerPlayer 实现经模板方法与具体实例判定完成
    MC_UNUSED(player);
    MC_UNUSED(startPos);
    MC_UNUSED(endPos);
}

} // namespace mc::advancement
