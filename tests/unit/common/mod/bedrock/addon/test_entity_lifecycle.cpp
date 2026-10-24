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

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/content/base/damage/BedrockDamageCause.hpp"
#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/mod/bedrock/addon/content/entity/events/AddonEntityEvents.hpp"
#include "common/mod/bedrock/addon/content/entity/filters/EntityFilter.hpp"
#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::mod::bedrock::addon;

namespace {
nlohmann::json _definition(bool living)
{
    nlohmann::json document = {{"format_version", "1.21.0"},
        {"minecraft:entity",
            {{"description",
                 {{"identifier", "test:lifecycle"},
                     {"properties", {{"test:deaths", {{"type", "int"}, {"range", {0, 100}}, {"default", 0}}}}}}},
                {"components", nlohmann::json::object()},
                {"events", {{"test:death", {{"set_property", {{"test:deaths", "q.property('test:deaths') + 1"}}}}}}}}}};
    if (living) {
        document["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 5}, {"max", 5}};
        document["minecraft:entity"]["components"]["minecraft:on_death"] = {{"event", "test:death"}};
    }
    return document;
}

class AddonEntityLifecycleTest : public testing::Test {
protected:
    void SetUp() override
    {
        sink = std::make_shared<EntityEventSink>([this](Entity&, const EntityEventNotification& notification) {
            notifications.push_back(notification.id);
        });
    }

    std::unique_ptr<Entity> create(const nlohmann::json& document)
    {
        auto parsed = AddonEntityDefinition::parse(document);
        EXPECT_TRUE(parsed.success()) << parsed.error().message();
        if (parsed.failed()) {
            return nullptr;
        }
        return AddonEntityDefinition::create(
            std::make_shared<const AddonEntityDefinition>(std::move(parsed.value())), nullptr, registry, sink);
    }

    ecs::EntityRegistry registry{"addon-entity-lifecycle"};
    std::shared_ptr<EntityEventSink> sink;
    std::vector<std::string> notifications;
};
} // namespace

TEST_F(AddonEntityLifecycleTest, ActualLethalDamageTriggersOnlyOnceAndPropertyRemainsDeferred)
{
    auto entity = create(_definition(true));
    ASSERT_NE(entity, nullptr);
    auto* living = dynamic_cast<LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    auto source = DamageSources::generic();
    ASSERT_TRUE(living->hurt(source, 1));
    EXPECT_TRUE(notifications.empty());
    ASSERT_TRUE(living->hurt(source, 100));
    ASSERT_TRUE(living->isDead());
    EXPECT_EQ(notifications, std::vector<std::string>{"test:death"});
    auto* state = entity->tryGetComponent<EntityAddonState>();
    EXPECT_EQ(state->getProperty("test:deaths"), nlohmann::json(0));
    living->die(source);
    EXPECT_EQ(notifications.size(), 1u);
    entity->tick();
    EXPECT_EQ(state->getProperty("test:deaths"), nlohmann::json(1));
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["death"], "handled");
}

TEST_F(AddonEntityLifecycleTest, DeathFiltersReceiveFatalCauseAndActualAttacker)
{
    auto document = _definition(true);
    document["minecraft:entity"]["components"]["minecraft:on_death"]["filters"] = {{"all_of",
        {{{"test", "has_damage"}, {"value", "fatal"}},
            {{"test", "has_damage"}, {"value", "attack"}},
            {{"test", "is_type"}, {"subject", "damager"}, {"value", "test:killer"}},
            {{"test", "is_type"}, {"subject", "other"}, {"value", "test:killer"}}}}};
    document["minecraft:entity"]["events"]["test:death"]["filters"] = {{"test", "has_damage"}, {"value", "fatal"}};
    auto entity = create(document);
    ASSERT_NE(entity, nullptr);
    Entity killer(EntityInstanceId(2), nullptr, registry);
    killer.setTypeId("test:killer");
    auto source = DamageSources::mobAttack(&killer);
    ASSERT_TRUE(dynamic_cast<LivingEntity*>(entity.get())->hurt(source, 10));
    EXPECT_EQ(notifications, std::vector<std::string>{"test:death"});
    entity->tick();
    EXPECT_EQ(entity->tryGetComponent<EntityAddonState>()->getProperty("test:deaths"), nlohmann::json(1));
}

TEST_F(AddonEntityLifecycleTest, TotemRescueDoesNotTriggerDeath)
{
    Items::ensureInitialized();
    const auto* totem = ItemRegistry::instance().getItem(ResourceLocation("minecraft:totem_of_undying"));
    ASSERT_NE(totem, nullptr);
    auto entity = create(_definition(true));
    ASSERT_NE(entity, nullptr);
    auto* living = dynamic_cast<LivingEntity*>(entity.get());
    living->setEquipment(EquipmentSlot::OffHand, ItemStack(*totem, 1));
    auto damage = DamageSources::generic();
    ASSERT_TRUE(living->hurt(damage, 100));
    EXPECT_FALSE(living->isDead());
    EXPECT_TRUE(notifications.empty());
    EXPECT_TRUE(living->getEquipment(EquipmentSlot::OffHand).isEmpty());
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["death"], "alive");
    auto kill = DamageSources::genericKill();
    ASSERT_TRUE(living->hurt(kill, 1000));
    EXPECT_EQ(notifications, std::vector<std::string>{"test:death"});
}

TEST_F(AddonEntityLifecycleTest, RemovedDeathTriggerDoesNotRunAndAnAliveDieCallDoesNothing)
{
    auto document = _definition(true);
    auto& actor = document["minecraft:entity"];
    actor["component_groups"]["death_trigger"] = {{"minecraft:on_death", actor["components"]["minecraft:on_death"]}};
    actor["components"].erase("minecraft:on_death");
    auto entity = create(document);
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    components->queue({{true, "death_trigger"}, {false, "death_trigger"}});
    components->applyPending(*entity);
    auto source = DamageSources::generic();
    auto* living = dynamic_cast<LivingEntity*>(entity.get());
    living->die(source);
    EXPECT_EQ(components->save()["death"], "alive");
    ASSERT_TRUE(living->hurt(source, 10));
    EXPECT_TRUE(notifications.empty());
}

TEST_F(AddonEntityLifecycleTest, SavedDeathGuardSurvivesNbtAndPreventsRepeatedTrigger)
{
    auto entity = create(_definition(true));
    ASSERT_NE(entity, nullptr);
    auto source = DamageSources::generic();
    auto* living = dynamic_cast<LivingEntity*>(entity.get());
    living->setHealth(0);
    living->die(source);
    ASSERT_EQ(notifications.size(), 1u);
    auto saved = nbt::jsonToNbt(entity->tryGetComponent<AddonEntityComponents>()->save());
    auto restored = create(_definition(true));
    auto* components = restored->tryGetComponent<AddonEntityComponents>();
    ASSERT_TRUE(components->load(*restored, nbt::nbtToJson(*saved)).success());
    auto* restoredLiving = dynamic_cast<LivingEntity*>(restored.get());
    restoredLiving->setHealth(0);
    restoredLiving->die(source);
    EXPECT_EQ(notifications.size(), 1u);
    auto corrupt = components->save();
    corrupt["death"] = true;
    EXPECT_TRUE(components->load(*restored, corrupt).failed());
    EXPECT_EQ(components->save()["death"], "handled");
}

TEST_F(AddonEntityLifecycleTest, InstantDespawnDiscardsActorsAndMobsWithoutDeathEvents)
{
    for (const bool living : {false, true}) {
        auto document = _definition(living);
        document["minecraft:entity"]["components"]["minecraft:instant_despawn"] = {{"remove_child_entities", false}};
        auto entity = create(document);
        ASSERT_NE(entity, nullptr);
        EXPECT_FALSE(entity->isRemoved());
        entity->tick();
        EXPECT_TRUE(entity->isRemoved());
        EXPECT_EQ(entity->removalReason(), RemovalReason::Discarded);
        EXPECT_EQ(entity->ticksExisted(), 0);
    }
    EXPECT_TRUE(notifications.empty());
}

TEST_F(AddonEntityLifecycleTest, AddedDespawnGroupStopsSameTickTimerAndRemovalCancelsRequest)
{
    auto document = _definition(false);
    auto& actor = document["minecraft:entity"];
    actor["component_groups"]["despawn"] = {{"minecraft:instant_despawn", nlohmann::json::object()}};
    actor["components"]["minecraft:timer"] = {{"time", 0.05}, {"looping", false}, {"time_down_event", "test:death"}};
    auto entity = create(document);
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    components->queue({{true, "despawn"}});
    entity->tick();
    EXPECT_TRUE(entity->isRemoved());
    EXPECT_TRUE(notifications.empty());
    auto cancelled = create(document);
    cancelled->tryGetComponent<AddonEntityComponents>()->queue({{true, "despawn"}, {false, "despawn"}});
    cancelled->tick();
    EXPECT_FALSE(cancelled->isRemoved());
    EXPECT_EQ(notifications, std::vector<std::string>{"test:death"});
}

TEST_F(AddonEntityLifecycleTest, InvalidDeathReferencesAndUnsupportedChildRemovalFailAtLoad)
{
    auto document = _definition(true);
    document["minecraft:entity"]["components"]["minecraft:on_death"] = {{"event", "test:missing"}};
    EXPECT_TRUE(AddonEntityDefinition::parse(document).failed());
    document["minecraft:entity"]["components"]["minecraft:on_death"] = {{"event", "test:death"}, {"target", "other"}};
    EXPECT_TRUE(AddonEntityDefinition::parse(document).failed());
    document = _definition(false);
    document["minecraft:entity"]["components"]["minecraft:instant_despawn"] = {{"remove_child_entities", true}};
    auto unsupported = AddonEntityDefinition::parse(document);
    ASSERT_TRUE(unsupported.failed());
    EXPECT_EQ(unsupported.error().code(), ErrorCode::Unsupported);
}

TEST_F(AddonEntityLifecycleTest, DamageFilterHasNoStaleContextAndNeverAppliesToTheAttacker)
{
    Entity victim(EntityInstanceId(1), nullptr, registry);
    Entity attacker(EntityInstanceId(2), nullptr, registry);
    const EntityFilterDamage damage{&victim, DamageType::OnFire, false};
    EntityFilterContext context{victim, &attacker, nullptr, nullptr, &attacker, nullptr, &damage};
    auto filter = EntityFilter::compile({{"test", "has_damage"}, {"value", "fire_tick"}});
    ASSERT_TRUE(filter.success());
    EXPECT_TRUE(filter.value().matches(context).value());
    filter = EntityFilter::compile({{"test", "has_damage"}, {"value", "fatal"}});
    ASSERT_TRUE(filter.success());
    EXPECT_FALSE(filter.value().matches(context).value());
    filter =
        EntityFilter::compile({{"test", "has_damage"}, {"subject", "other"}, {"value", "fatal"}, {"operator", "not"}});
    ASSERT_TRUE(filter.success());
    EXPECT_FALSE(filter.value().matches(context).value());
    context.damage = nullptr;
    filter = EntityFilter::compile({{"test", "has_damage"}, {"value", "fire_tick"}, {"operator", "not"}});
    ASSERT_TRUE(filter.success());
    EXPECT_FALSE(filter.value().matches(context).value());
    EXPECT_TRUE(EntityFilter::compile({{"test", "has_damage"}, {"value", "fireTick"}}).failed());
    EXPECT_EQ(damageFilterCauseName(DamageType::ExplosionPlayer), "entity_explosion");
    EXPECT_EQ(damageCauseName(DamageType::OnFire), "fireTick");
}

TEST_F(AddonEntityLifecycleTest, EnvironmentSensorUsesCurrentTickValuesAndDefersEarlierTriggerWrites)
{
    auto document = _definition(false);
    auto& actor = document["minecraft:entity"];
    actor["events"] = {{"test:first", {{"set_property", {{"test:deaths", 1}}}}},
        {"test:second", {{"set_property", {{"test:deaths", 2}}}}}};
    actor["components"]["minecraft:environment_sensor"] = {{"triggers",
        {{{"event", "test:first"}, {"filters", {{{"test", "int_property"}, {"domain", "test:deaths"}, {"value", 0}}}}},
            {{"event", "test:second"},
                {"filters", {{"test", "int_property"}, {"domain", "test:deaths"}, {"value", 1}}}}}}};
    auto entity = create(document);
    ASSERT_NE(entity, nullptr);
    auto* state = entity->tryGetComponent<EntityAddonState>();
    entity->tick();
    EXPECT_EQ(notifications, std::vector<std::string>{"test:first"});
    EXPECT_EQ(state->getProperty("test:deaths"), nlohmann::json(0));
    entity->tick();
    EXPECT_EQ(notifications, (std::vector<std::string>{"test:first", "test:second"}));
    EXPECT_EQ(state->getProperty("test:deaths"), nlohmann::json(1));
    entity->tick();
    EXPECT_EQ(state->getProperty("test:deaths"), nlohmann::json(2));
    EXPECT_EQ(notifications.size(), 2u);
}

TEST_F(AddonEntityLifecycleTest, EnvironmentSensorContinuesWhileMatchingAndSupportsSingleObject)
{
    auto document = _definition(false);
    document["minecraft:entity"]["components"]["minecraft:environment_sensor"] = {
        {"triggers", {{"event", "test:death"}, {"filters", {{"test", "is_alive"}}}}}};
    auto entity = create(document);
    ASSERT_NE(entity, nullptr);
    entity->tick();
    entity->tick();
    entity->tick();
    EXPECT_EQ(notifications.size(), 3u);
    EXPECT_EQ(entity->tryGetComponent<EntityAddonState>()->getProperty("test:deaths"), nlohmann::json(2));
}

TEST_F(AddonEntityLifecycleTest, EnvironmentSensorRunsAllMatchingTriggersInDefinitionOrder)
{
    auto document = _definition(false);
    auto& actor = document["minecraft:entity"];
    actor["events"]["test:second"] = {{"set_property", {{"test:deaths", 10}}}};
    actor["components"]["minecraft:environment_sensor"] = {
        {"triggers", {{{"event", "test:death"}}, {{"event", "test:second"}}}}};
    auto entity = create(document);
    ASSERT_NE(entity, nullptr);
    entity->tick();
    EXPECT_EQ(notifications, (std::vector<std::string>{"test:death", "test:second"}));
    auto* state = entity->tryGetComponent<EntityAddonState>();
    EXPECT_EQ(state->getProperty("test:deaths"), nlohmann::json(0));
    entity->tick();
    EXPECT_EQ(state->getProperty("test:deaths"), nlohmann::json(10));
}

TEST_F(AddonEntityLifecycleTest, EnvironmentSensorGroupRemovalAndRestoreDoNotReinstateBaseSensor)
{
    auto document = _definition(false);
    auto& actor = document["minecraft:entity"];
    const nlohmann::json sensor = {{"triggers", {{"event", "test:death"}}}};
    actor["components"]["minecraft:environment_sensor"] = sensor;
    actor["component_groups"]["sensor"] = {{"minecraft:environment_sensor", sensor}};
    auto entity = create(document);
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    components->queue({{true, "sensor"}});
    entity->tick();
    ASSERT_EQ(notifications.size(), 1u);
    components->queue({{false, "sensor"}});
    auto saved = nbt::jsonToNbt(components->save());
    auto restored = create(document);
    ASSERT_NE(restored, nullptr);
    auto* restoredComponents = restored->tryGetComponent<AddonEntityComponents>();
    ASSERT_TRUE(restoredComponents->load(*restored, nbt::nbtToJson(*saved)).success());
    restored->tick();
    EXPECT_FALSE(restoredComponents->has("minecraft:environment_sensor"));
    EXPECT_EQ(notifications.size(), 1u);
    entity->tick();
    EXPECT_FALSE(components->has("minecraft:environment_sensor"));
    EXPECT_EQ(notifications.size(), 1u);
}

TEST_F(AddonEntityLifecycleTest, InstantDespawnSkipsEnvironmentEvents)
{
    auto document = _definition(false);
    auto& components = document["minecraft:entity"]["components"];
    components["minecraft:instant_despawn"] = nlohmann::json::object();
    components["minecraft:environment_sensor"] = {{"triggers", {{"event", "test:death"}}}};
    auto entity = create(document);
    ASSERT_NE(entity, nullptr);
    entity->tick();
    EXPECT_TRUE(entity->isRemoved());
    EXPECT_TRUE(notifications.empty());
}

TEST_F(AddonEntityLifecycleTest, EnvironmentSensorRejectsInvalidTriggersAndUnresolvedEvents)
{
    for (const auto& sensor : std::vector<nlohmann::json>{nlohmann::json::array(),
             {{"triggers", 3}},
             {{"triggers", {"test:death"}}},
             {{"triggers", {{"event", "test:missing"}}}},
             {{"triggers", {{"event", "test:death"}, {"unknown", true}}}},
             {{"triggers", {{"event", "test:death"}, {"filters", {{"test", "unknown"}}}}}}}) {
        auto document = _definition(false);
        document["minecraft:entity"]["components"]["minecraft:environment_sensor"] = sensor;
        EXPECT_TRUE(AddonEntityDefinition::parse(document).failed()) << sensor.dump();
    }
    auto document = _definition(false);
    document["minecraft:entity"]["components"]["minecraft:environment_sensor"] = {
        {"triggers", nlohmann::json::array()}};
    EXPECT_TRUE(AddonEntityDefinition::parse(document).success());
}
