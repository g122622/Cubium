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
    // 服务端代码应使用以下方式之一触发检测：
    //
    // 方法1：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<TameAnimalTrigger>();
    // trigger->AbstractCriterionTrigger<TameAnimalTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&entity](const TameAnimalTriggerInstance& instance) {
    //         return instance.test(entity);
    //     }
    // );
    //
    // 方法2：在 setTamedBy 中直接调用（推荐服务端代码处理）
    // 参考：server/advancement/AdvancementEventHandler.hpp
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
    // 服务端代码应使用以下方式触发检测：
    //
    // 方法：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<BredAnimalsTrigger>();
    // trigger->AbstractCriterionTrigger<BredAnimalsTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&child, &parent, &partner](const BredAnimalsTriggerInstance& instance) {
    //         return instance.test(child, parent, partner);
    //     }
    // );
    //
    // 参考：server/advancement/AdvancementEventHandler.hpp 中的 onBredAnimals()
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
    // 服务端代码应使用以下方式触发检测：
    //
    // 方法：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<SummonedEntityTrigger>();
    // trigger->AbstractCriterionTrigger<SummonedEntityTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&entity](const SummonedEntityTriggerInstance& instance) {
    //         return instance.test(entity);
    //     }
    // );
    //
    // 参考：server/advancement/AdvancementEventHandler.hpp（待实现 onSummonedEntity）
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
    // 服务端代码应使用以下方式触发检测：
    //
    // 方法：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<CuredZombieVillagerTrigger>();
    // trigger->AbstractCriterionTrigger<CuredZombieVillagerTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&zombie, &villager](const CuredZombieVillagerTriggerInstance& instance) {
    //         return instance.test(zombie, villager);
    //     }
    // );
    //
    // 参考：server/advancement/AdvancementEventHandler.hpp 中的 onCuredZombieVillager()
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
    // 服务端代码应使用以下方式触发检测：
    //
    // 方法：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<VillagerTradeTrigger>();
    // trigger->AbstractCriterionTrigger<VillagerTradeTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&villager, &item](const VillagerTradeTriggerInstance& instance) {
    //         return instance.test(villager, item);
    //     }
    // );
    //
    // 参考：server/advancement/AdvancementEventHandler.hpp（待实现 onVillagerTrade）
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
    // 服务端代码应使用以下方式触发检测：
    //
    // 方法：使用 TriggerInstantiation.hpp 中的 trigger 模板方法
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<PlayerInteractedWithEntityTrigger>();
    // trigger->AbstractCriterionTrigger<PlayerInteractedWithEntityTriggerInstance>::trigger(
    //     *player.getAdvancements(),
    //     [&item, &entity](const PlayerInteractedWithEntityTriggerInstance& instance) {
    //         return instance.test(item, entity);
    //     }
    // );
    //
    // 参考：server/advancement/AdvancementEventHandler.hpp
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(entity);
}

} // namespace mc::advancement
