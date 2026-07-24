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
#include "common/util/assert/AssertAll.hpp"

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

Result<void> TameAnimalTriggerInstance::fromJson(const nlohmann::json& json)
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

Result<void> BredAnimalsTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
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
        auto result = EntityPredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entity = result.value();
    }

    return {};
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

Result<void> CuredZombieVillagerTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("zombie")) {
        auto result = EntityPredicate::fromJson(json["zombie"]);
        if (result.failed()) {
            return result.error();
        }
        m_zombie = result.value();
    }

    if (json.contains("villager")) {
        auto result = EntityPredicate::fromJson(json["villager"]);
        if (result.failed()) {
            return result.error();
        }
        m_villager = result.value();
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
    if (!m_item.test(item)) {
        return false;
    }
    if (!m_entity.test(entity)) {
        return false;
    }
    return true;
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
        auto result = EntityPredicate::fromJson(json["entity"]);
        if (result.failed()) {
            return result.error();
        }
        m_entity = result.value();
    }

    return {};
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
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端在 ServerPlayRouter 的 UseEntity/Interact 分支中直接调用
    // AbstractCriterionTrigger<PlayerInteractedWithEntityTriggerInstance>::trigger() 触发检测，
    // 当玩家成功与实体交互（ActionResultType::Success 或 Consume）时触发。
    // TODO(Phase6): UseEntity 触发接线待补。
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(entity);
}

} // namespace mc::advancement
