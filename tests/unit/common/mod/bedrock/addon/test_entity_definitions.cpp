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

#include "common/TempDirHelper.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptGameObjects.hpp"
#include "common/mod/bedrock/addon/content/entity/events/AddonEntityEvents.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventProgram.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPlugin.hpp"

#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"
#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/mod/bedrock/addon/content/entity/filters/EntityFilter.hpp"
#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/mod/bedrock/addon/content/entity/state/timer/EntityTimerState.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {
constexpr const char* _PACK = "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa";
constexpr const char* _OTHER = "bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb";

nlohmann::json _definition(const std::string& identifier)
{
    return {{"format_version", "1.21.0"},
        {"minecraft:entity",
            {{"description",
                 {{"identifier", identifier},
                     {"is_summonable", true},
                     {"properties", {{"test:count", {{"type", "int"}, {"range", {0, 100}}, {"default", 3}}}}}}},
                {"components",
                    {{"minecraft:collision_box", {{"width", 0.8}, {"height", 1.4}}},
                        {"minecraft:physics", {{"has_gravity", false}, {"has_collision", true}}},
                        {"minecraft:fire_immune", nlohmann::json::object()}}}}}};
}

class AddonEntityDefinitionTest : public testing::Test {
protected:
    void SetUp() override
    {
        root = mc::test::makeUniqueTestDir("addon-entity-definitions");
        ASSERT_TRUE(manager.initialize().success());
    }

    void TearDown() override
    {
        manager.shutdown();
        mc::test::removeTestDir(root);
    }

    std::filesystem::path pack(const std::string& folder, const std::string& uuid, std::optional<std::string> script)
    {
        const auto directory = root / folder;
        std::filesystem::create_directories(directory / "entities/nested");
        nlohmann::json manifest = {{"format_version", 2},
            {"header", {{"name", folder}, {"uuid", uuid}, {"version", {1, 0, 0}}, {"min_engine_version", {1, 21, 90}}}},
            {"modules",
                {{{"type", "data"}, {"uuid", "cccccccc-cccc-4ccc-8ccc-cccccccccccc"}, {"version", {1, 0, 0}}}}}};
        if (script) {
            manifest["modules"].push_back({{"type", "script"},
                {"uuid", "dddddddd-dddd-4ddd-8ddd-dddddddddddd"},
                {"version", {1, 0, 0}},
                {"entry", "scripts/main.js"},
                {"language", "javascript"}});
            manifest["dependencies"] = {{{"module_name", "@minecraft/server"}, {"version", "2.0.0"}}};
            std::filesystem::create_directories(directory / "scripts");
            std::ofstream(directory / "scripts/main.js") << *script;
        }
        std::ofstream(directory / "manifest.json") << manifest.dump();
        return directory;
    }

    void write(const std::filesystem::path& path, const nlohmann::json& document)
    {
        std::ofstream(path) << document.dump();
    }

    std::unique_ptr<mc::Entity> create(const std::string& id)
    {
        const auto* type = mc::entity::EntityRegistry::instance().getType(id);
        EXPECT_NE(type, nullptr);
        return type != nullptr ? type->create(nullptr, entities) : nullptr;
    }

    std::filesystem::path root;
    mc::ecs::EntityRegistry entities{"addon-defined-entities"};
    ScriptManager manager;
};
} // namespace

TEST_F(AddonEntityDefinitionTest, DataOnlyPackInstallsPropertiesDimensionsAndPhysics)
{
    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/nested/actor.json", _definition("addon_test:actor"));
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    EXPECT_EQ(manager.pluginManager().pluginCount(), 0u);
    auto actor = create("addon_test:actor");
    ASSERT_NE(actor, nullptr);
    EXPECT_EQ(dynamic_cast<mc::LivingEntity*>(actor.get()), nullptr);
    EXPECT_FLOAT_EQ(actor->width(), 0.8f);
    EXPECT_FLOAT_EQ(actor->height(), 1.4f);
    EXPECT_TRUE(actor->hasNoGravity());
    EXPECT_FALSE(actor->noClip());
    EXPECT_TRUE(actor->isImmuneToFire());
    const auto* state = actor->tryGetComponent<EntityAddonState>();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->getProperty("test:count"), nlohmann::json(3));
    EXPECT_EQ(actor->entityType()->name(), "addon_test:actor");
}

TEST_F(AddonEntityDefinitionTest, LivingComponentsInitializeHealthAndMovement)
{
    const auto directory = pack("living", _PACK, std::nullopt);
    auto definition = _definition("addon_test:living");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"max", 64}, {"value", 40}};
    definition["minecraft:entity"]["components"]["minecraft:movement"] = {{"value", 0.35}};
    write(directory / "entities/living.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:living");
    auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->maxHealth(), 64);
    EXPECT_FLOAT_EQ(living->health(), 40);
    EXPECT_DOUBLE_EQ(living->attributes().getValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED, -1), 0.35);
}

TEST_F(AddonEntityDefinitionTest, EntityDeserializerInstallsSchemaBeforeRestoringProperties)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("saved", _PACK, std::nullopt);
    write(directory / "entities/saved.json", _definition("addon_test:saved"));
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto original = create("addon_test:saved");
    ASSERT_NE(original, nullptr);
    auto& state = original->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(state.setProperty("test:count", 42).success());
    ASSERT_TRUE(state.dynamicProperties().set(_PACK, "flag", true).success());
    const auto expected = state.save();
    mc::nbt::tags::compound_tag snapshot;
    snapshot.put("id", original->getTypeId());
    original->writeToNBT(snapshot);
    auto direct = mc::entity::serialization::EntityDeserializer::deserialize(snapshot, entities);
    ASSERT_TRUE(direct.success()) << direct.error().message();
    auto directEntity = direct.value();
    ASSERT_NE(directEntity, nullptr);
    ASSERT_EQ(directEntity->getOrAddComponent<EntityAddonState>().save(), expected);
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*original);
    ASSERT_TRUE(saved.success()) << saved.error().message();
    auto restored = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(restored.success()) << restored.error().message();
    // Result<unique_ptr<T>>::value() 会转移所有权，必须先持有实体再借用组件。
    auto restoredEntity = restored.value();
    ASSERT_NE(restoredEntity, nullptr);
    const auto* restoredState = restoredEntity->tryGetComponent<EntityAddonState>();
    ASSERT_NE(restoredState, nullptr);
    EXPECT_EQ(restoredEntity->getTypeId(), "addon_test:saved");
    EXPECT_EQ(restoredState->save(), expected);
    EXPECT_EQ(restoredState->getProperty("test:count"), nlohmann::json(42));
    EXPECT_EQ(restoredState->dynamicProperties().get(_PACK, "flag"), nlohmann::json(true));
}

TEST_F(AddonEntityDefinitionTest, InvalidDefinitionDoesNotRegisterEarlierValidFiles)
{
    const auto directory = pack("invalid", _PACK, std::nullopt);
    write(directory / "entities/a-valid.json", _definition("addon_test:uncommitted"));
    auto invalid = _definition("addon_test:broken");
    invalid["minecraft:entity"]["components"]["minecraft:physics"]["has_gravity"] = "false";
    write(directory / "entities/z-invalid.json", invalid);
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:uncommitted"));
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:broken"));
}

TEST_F(AddonEntityDefinitionTest, ScriptFailureRollsBackOnlyNewContent)
{
    const auto first = pack("existing", _PACK, std::nullopt);
    write(first / "entities/existing.json", _definition("addon_test:existing"));
    ASSERT_TRUE(manager.packList()->addPack(first.string(), true, 0).success());
    ASSERT_TRUE(manager.loadPacks("", "").success());
    const auto* existing = mc::entity::EntityRegistry::instance().getType("addon_test:existing");
    const auto broken = pack("broken", _OTHER, std::string("invalid javascript ???"));
    write(broken / "entities/new.json", _definition("addon_test:rollback"));
    ASSERT_TRUE(manager.packList()->addPack(broken.string(), true, 0).success());
    EXPECT_TRUE(manager.loadPacks("", "").failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:rollback"));
    EXPECT_EQ(mc::entity::EntityRegistry::instance().getType("addon_test:existing"), existing);
}

TEST_F(AddonEntityDefinitionTest, ReloadRetiresLookupButKeepsLiveEntityDefinition)
{
    const auto directory = pack("reload", _PACK, std::nullopt);
    auto definition = _definition("addon_test:reload");
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto oldEntity = create("addon_test:reload");
    ASSERT_NE(oldEntity, nullptr);
    const auto* oldType = oldEntity->entityType();
    const auto oldId = oldType->id();
    manager.reload();
    EXPECT_EQ(mc::entity::EntityRegistry::instance().getType("addon_test:reload"), nullptr);
    EXPECT_EQ(mc::entity::EntityRegistry::instance().getTypeById(oldId), nullptr);
    EXPECT_EQ(oldType->name(), "addon_test:reload");
    EXPECT_FLOAT_EQ(oldEntity->width(), 0.8f);
    definition["minecraft:entity"]["components"]["minecraft:collision_box"]["width"] = 2;
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto current = create("addon_test:reload");
    ASSERT_NE(current, nullptr);
    EXPECT_FLOAT_EQ(current->width(), 2);
    EXPECT_FLOAT_EQ(oldEntity->width(), 0.8f);
    EXPECT_NE(current->entityType(), oldType);
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().unregisterType("addon_test:reload", oldType));
    const auto registered = mc::entity::EntityRegistry::instance().getAllTypes();
    EXPECT_EQ(std::find(registered.begin(), registered.end(), oldType), registered.end());
}

TEST_F(AddonEntityDefinitionTest, HigherPriorityPackOverridesTheSameIdentifier)
{
    const auto low = pack("low", _PACK, std::nullopt);
    const auto high = pack("high", _OTHER, std::nullopt);
    auto lowDefinition = _definition("addon_test:priority");
    auto highDefinition = lowDefinition;
    highDefinition["minecraft:entity"]["description"]["properties"]["test:count"]["default"] = 9;
    write(low / "entities/entity.json", lowDefinition);
    write(high / "entities/entity.json", highDefinition);
    ASSERT_TRUE(manager.packList()->addPack(low.string(), true, 0).success());
    ASSERT_TRUE(manager.packList()->addPack(high.string(), true, 100).success());
    ASSERT_TRUE(manager.loadPacks("", "").success());
    auto entity = create("addon_test:priority");
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(9));
}

TEST_F(AddonEntityDefinitionTest, DuplicateDefinitionsWithinOnePackAreRejected)
{
    const auto directory = pack("duplicate", _PACK, std::nullopt);
    write(directory / "entities/a.json", _definition("addon_test:duplicate"));
    write(directory / "entities/b.json", _definition("addon_test:duplicate"));
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:duplicate"));
}

TEST_F(AddonEntityDefinitionTest, UnsupportedComponentsAndEventSectionsAreExplicitFailures)
{
    auto definition = _definition("addon_test:unsupported");
    definition["minecraft:entity"]["components"]["minecraft:inventory"] = {{"inventory_size", 9}};
    auto result = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), mc::ErrorCode::Unsupported);
    definition = _definition("addon_test:unsupported");
    definition["minecraft:entity"]["events"]["test:change"] = {
        {"set_property", {{"test:count", "variable.counter + 1"}}}};
    result = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), mc::ErrorCode::Unsupported);
    definition = _definition("addon_test:unsupported");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {
        {"value", {{"range_min", 10}, {"range_max", 20}}}};
    result = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), mc::ErrorCode::Unsupported);
}

TEST_F(AddonEntityDefinitionTest, MalformedSchemasAndComponentValuesAreRejected)
{
    auto definition = _definition("addon_test:invalid");
    std::vector<nlohmann::json> invalid;
    invalid.push_back(nlohmann::json::array());
    auto broken = definition;
    broken["minecraft:entity"]["description"]["identifier"] = "invalid identifier";
    invalid.push_back(broken);
    broken = definition;
    broken["minecraft:entity"]["description"]["properties"]["test:count"]["default"] = 101;
    invalid.push_back(broken);
    broken = definition;
    broken["minecraft:entity"]["components"]["minecraft:collision_box"]["height"] =
        std::numeric_limits<double>::infinity();
    invalid.push_back(broken);
    broken = definition;
    broken["minecraft:entity"]["components"] = nlohmann::json::array();
    invalid.push_back(broken);
    broken = definition;
    broken["minecraft:entity"]["events"] = nlohmann::json::array();
    invalid.push_back(broken);
    for (const auto& document : invalid) {
        EXPECT_TRUE(AddonEntityDefinition::parse(document).failed());
    }
}

TEST_F(AddonEntityDefinitionTest, ResourceEnumerationIsSortedAndRejectsParentTraversal)
{
    const auto directory = pack("paths", _PACK, std::nullopt);
    write(directory / "entities/z.json", _definition("addon_test:path_z"));
    write(directory / "entities/nested/a.json", _definition("addon_test:path_a"));
    ASSERT_TRUE(manager.packList()->addPack(directory.string(), true, 0).success());
    auto* behavior = manager.packList()->getPackByUuid(_PACK);
    ASSERT_NE(behavior, nullptr);
    auto files = behavior->listResources("entities", ".json");
    ASSERT_TRUE(files.success()) << files.error().message();
    EXPECT_EQ(files.value(), (std::vector<std::string>{"entities/nested/a.json", "entities/z.json"}));
    EXPECT_TRUE(behavior->listResources("../", ".json").failed());
    EXPECT_TRUE(behavior->listResources(directory.string(), ".json").failed());
    auto missing = behavior->listResources("missing", ".json");
    ASSERT_TRUE(missing.success());
    EXPECT_TRUE(missing.value().empty());
}

TEST_F(AddonEntityDefinitionTest, PropertyEventsExecuteSequenceAndSelfTriggersAtTickBoundary)
{
    const auto directory = pack("events", _PACK, std::nullopt);
    auto definition = _definition("addon_test:events");
    definition["minecraft:entity"]["events"] = {{"test:inner", {{"set_property", {{"test:count", 7}}}}},
        {"test:outer",
            {{"sequence",
                {{{"set_property", {{"test:count", 5}}}},
                    {{"trigger", {{"event", "test:inner"}, {"target", "self"}}}},
                    {{"set_property", {{"test:count", 11}}}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:events");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_NE(events, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(events->trigger(*entity, "test:outer").success());
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(11));
}

TEST_F(AddonEntityDefinitionTest, EventNumericValuesClampWhileScriptWritesRejectOutOfRange)
{
    const auto directory = pack("clamp", _PACK, std::nullopt);
    auto definition = _definition("addon_test:clamp");
    definition["minecraft:entity"]["events"] = {{"test:high", {{"set_property", {{"test:count", 1000}}}}},
        {"test:low", {{"set_property", {{"test:count", -1000}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:clamp");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_TRUE(state.setProperty("test:count", 1000).failed());
    ASSERT_TRUE(events->trigger(*entity, "test:high").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(100));
    ASSERT_TRUE(events->trigger(*entity, "test:low").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(0));
}

TEST_F(AddonEntityDefinitionTest, WeightedRandomizeExecutesOnePositiveWeightBranch)
{
    const auto directory = pack("random", _PACK, std::nullopt);
    auto definition = _definition("addon_test:random");
    definition["minecraft:entity"]["events"]["test:random"] = {{"randomize",
        {{{"weight", 0}, {"set_property", {{"test:count", 99}}}},
            {{"weight", 1}, {"set_property", {{"test:count", 4}}}},
            {{"weight", 2}, {"set_property", {{"test:count", 8}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:random");
    ASSERT_NE(entity, nullptr);
    entity->getRandom().setSeed(123);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    int first = 0;
    int second = 0;
    for (int index = 0; index < 100; ++index) {
        ASSERT_TRUE(events->trigger(*entity, "test:random").success());
        entity->tick();
        const auto value = state.getProperty("test:count");
        EXPECT_TRUE(value == nlohmann::json(4) || value == nlohmann::json(8));
        first += value == nlohmann::json(4);
        second += value == nlohmann::json(8);
    }
    EXPECT_GT(first, 0);
    EXPECT_GT(second, 0);
}

TEST_F(AddonEntityDefinitionTest, RecursiveFailureDoesNotCommitAnyPendingWritesOrNotifications)
{
    const auto directory = pack("recursive", _PACK, std::nullopt);
    auto definition = _definition("addon_test:recursive");
    definition["minecraft:entity"]["events"]["test:loop"] = {
        {"sequence", {{{"set_property", {{"test:count", 90}}}}, {{"trigger", "test:loop"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:recursive");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    const auto before = state.save();
    EXPECT_TRUE(events->trigger(*entity, "test:loop").failed());
    EXPECT_TRUE(events->trigger(*entity, "test:missing").failed());
    EXPECT_EQ(state.save(), before);
    EXPECT_FALSE(manager.eventBus().hasPendingAfterEvents());
}

TEST_F(AddonEntityDefinitionTest, MalformedEventActionsAndReferencesFailBeforeRegistration)
{
    const auto properties = _definition("addon_test:validation")["minecraft:entity"]["description"]["properties"];
    const std::vector<nlohmann::json> invalid = {{{"test:event", {{"trigger", "test:missing"}}}},
        {{"test:event", {{"sequence", nlohmann::json::object()}}}},
        {{"test:event", {{"randomize", nlohmann::json::array()}}}},
        {{"test:event", {{"randomize", {{{"weight", -1}}}}}}},
        {{"test:event", {{"randomize", {{{"weight", 0}}}}}}},
        {{"test:event", {{"set_property", {{"test:missing", true}}}}}},
        {{"test:event", {{"set_property", {{"test:count", true}}}}}},
        {{"test:event", {{"trigger", {{"event", "test:event"}, {"target", "other"}}}}}}};
    for (const auto& events : invalid) {
        EXPECT_TRUE(EntityEventProgram::compile(events, properties, nlohmann::json::object()).failed())
            << events.dump();
    }
}

TEST_F(AddonEntityDefinitionTest, SpawnEventsAreOnceOnlyAndAreNotReplayedByRestoreFactories)
{
    const auto directory = pack("spawn-events", _PACK, std::nullopt);
    auto definition = _definition("addon_test:spawn_events");
    definition["minecraft:entity"]["events"]["minecraft:entity_spawned"] = {{"set_property", {{"test:count", 8}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    // nullptr 世界是统一反序列化工厂入口，不能重新触发 entity_spawned。
    auto entity = create("addon_test:spawn_events");
    ASSERT_NE(entity, nullptr);
    entity->tick();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    auto compiled = EntityEventProgram::compile(definition["minecraft:entity"]["events"],
        definition["minecraft:entity"]["description"]["properties"],
        nlohmann::json::object());
    ASSERT_TRUE(compiled.success());
    auto program = std::make_shared<const EntityEventProgram>(std::move(compiled.value()));
    AddonEntityEvents spawned(program, {}, true);
    spawned.onTick(*entity);
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(8));
    ASSERT_TRUE(state.setProperty("test:count", 2).success());
    entity->tick();
    spawned.onTick(*entity);
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(2));
}

TEST_F(AddonEntityDefinitionTest, ScriptTriggerEventPublishesFilteredDeferredSnapshots)
{
    const auto directory = pack("script-events", _PACK, std::string(R"(
        import { world } from '@minecraft/server';
        globalThis.events = [];
        world.afterEvents.dataDrivenEntityTrigger.subscribe(event => {
            events.push({name: event.eventId, type: event.entity.typeId, modifiers: event.getModifiers().length});
        }, {eventTypes: ['test:go'], entityTypes: ['addon_test:script_events']});
    )"));
    auto definition = _definition("addon_test:script_events");
    definition["minecraft:entity"]["events"] = {
        {"test:go", {{"set_property", {{"test:count", 9}}}}}, {"test:ignored", nlohmann::json::object()}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:script_events");
    ASSERT_NE(entity, nullptr);
    auto* plugin = manager.pluginManager().getPlugin(_PACK);
    ASSERT_NE(plugin, nullptr);
    auto* context = plugin->context();
    ASSERT_NE(context, nullptr);
    auto& bindings = context->bindingContext();
    void* global = bindings.getGlobalObject();
    void* subject = wrapEntity(bindings, entity.get());
    bindings.setProperty(global, "subject", subject);
    bindings.releaseValue(subject);
    bindings.releaseValue(global);
    auto triggered = context->evaluate("subject.triggerEvent('test:go'); subject.triggerEvent('test:ignored'); if "
                                       "(events.length) throw Error('event was synchronous');",
        "trigger.js",
        EvalFlags::None);
    ASSERT_TRUE(triggered.success()) << triggered.errorMessage();
    manager.eventBus().tick();
    auto checked = context->evaluate("if (events.length !== 1 || events[0].name !== 'test:go' || events[0].modifiers "
                                     "!== 0) throw Error('wrong filtered event');",
        "check.js",
        EvalFlags::None);
    ASSERT_TRUE(checked.success()) << checked.errorMessage();
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(9));
}

TEST_F(AddonEntityDefinitionTest, EventsOutliveScriptShutdownWithoutKeepingTheBusAlive)
{
    const auto directory = pack("shutdown-events", _PACK, std::nullopt);
    auto definition = _definition("addon_test:shutdown_events");
    definition["minecraft:entity"]["events"]["test:change"] = {{"set_property", {{"test:count", 6}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:shutdown_events");
    ASSERT_NE(entity, nullptr);
    manager.shutdown();
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_NE(events, nullptr);
    ASSERT_TRUE(events->trigger(*entity, "test:change").success());
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(6));
}

TEST_F(AddonEntityDefinitionTest, ComponentGroupsChangeDimensionsAtTickBoundaryAndRemoveWithoutFallback)
{
    const auto directory = pack("groups", _PACK, std::nullopt);
    auto definition = _definition("addon_test:groups");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"] = {{"test:add", {{"add", {{"component_groups", {"large"}}}}}},
        {"test:remove", {{"remove", {{"component_groups", {"large"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:groups");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:add").success());
    EXPECT_FLOAT_EQ(entity->width(), 0.8f);
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 2);
    EXPECT_FLOAT_EQ(entity->height(), 3);
    ASSERT_TRUE(events->trigger(*entity, "test:remove").success());
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 0);
    EXPECT_FLOAT_EQ(entity->height(), 0);
}

TEST_F(AddonEntityDefinitionTest, OverlappingGroupRemovalDoesNotReactivateAnotherActiveGroup)
{
    const auto directory = pack("overlapping", _PACK, std::nullopt);
    auto definition = _definition("addon_test:overlapping");
    auto& groups = definition["minecraft:entity"]["component_groups"];
    groups["first"] = {{"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    groups["second"] = {{"minecraft:collision_box", {{"width", 4}, {"height", 5}}}};
    definition["minecraft:entity"]["events"] = {{"test:first", {{"add", {{"component_groups", {"first"}}}}}},
        {"test:second", {{"add", {{"component_groups", {"second"}}}}}},
        {"test:remove_first", {{"remove", {{"component_groups", {"first"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:overlapping");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:first").success());
    ASSERT_TRUE(events->trigger(*entity, "test:second").success());
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 4);
    ASSERT_TRUE(events->trigger(*entity, "test:remove_first").success());
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 0);
    const auto state = entity->tryGetComponent<AddonEntityComponents>()->save();
    EXPECT_EQ(state["active"], nlohmann::json::array({"second"}));
    EXPECT_FALSE(state["sources"].contains("minecraft:collision_box"));
    ASSERT_TRUE(events->trigger(*entity, "test:second").success());
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 4);
}

TEST_F(AddonEntityDefinitionTest, ReaddingAnActiveHealthGroupReinitializesItsValues)
{
    const auto directory = pack("health-group", _PACK, std::nullopt);
    auto definition = _definition("addon_test:health_group");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"max", 32}, {"value", 24}};
    definition["minecraft:entity"]["component_groups"]["strong"] = {
        {"minecraft:health", {{"max", 128}, {"value", 96}}}};
    definition["minecraft:entity"]["events"]["test:strong"] = {{"add", {{"component_groups", {"strong"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:health_group");
    auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    ASSERT_TRUE(events->trigger(*entity, "test:strong").success());
    components->applyPending(*entity);
    EXPECT_FLOAT_EQ(living->maxHealth(), 128);
    EXPECT_FLOAT_EQ(living->health(), 96);
    living->setHealth(50);
    ASSERT_TRUE(events->trigger(*entity, "test:strong").success());
    components->applyPending(*entity);
    EXPECT_FLOAT_EQ(living->health(), 96);
}

TEST_F(AddonEntityDefinitionTest, GroupPhysicsAndFireImmunityRemainPerInstance)
{
    const auto directory = pack("physics-group", _PACK, std::nullopt);
    auto definition = _definition("addon_test:physics_group");
    definition["minecraft:entity"]["components"].erase("minecraft:fire_immune");
    definition["minecraft:entity"]["component_groups"]["ghost"] = {
        {"minecraft:physics", {{"has_gravity", false}, {"has_collision", false}}},
        {"minecraft:fire_immune", nlohmann::json::object()}};
    definition["minecraft:entity"]["events"] = {{"test:ghost", {{"add", {{"component_groups", {"ghost"}}}}}},
        {"test:normal", {{"remove", {{"component_groups", {"ghost"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto first = create("addon_test:physics_group");
    auto second = create("addon_test:physics_group");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_TRUE(first->tryGetComponent<AddonEntityEvents>()->trigger(*first, "test:ghost").success());
    first->tick();
    EXPECT_TRUE(first->noClip());
    EXPECT_TRUE(first->isImmuneToFire());
    EXPECT_FALSE(second->noClip());
    EXPECT_FALSE(second->isImmuneToFire());
    ASSERT_TRUE(first->tryGetComponent<AddonEntityEvents>()->trigger(*first, "test:normal").success());
    first->tick();
    EXPECT_FALSE(first->isImmuneToFire());
}

TEST_F(AddonEntityDefinitionTest, SavedGroupsRestoreActualSourcesWithoutRecombiningDefinitions)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("save-groups", _PACK, std::nullopt);
    auto definition = _definition("addon_test:save_groups");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"] = {{"test:add", {{"add", {{"component_groups", {"large"}}}}}},
        {"test:remove", {{"remove", {{"component_groups", {"large"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:save_groups");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:add").success());
    entity->tick();
    ASSERT_TRUE(events->trigger(*entity, "test:remove").success());
    entity->tick();
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success()) << saved.error().message();
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_FLOAT_EQ(restored->width(), 0);
    EXPECT_EQ(restored->tryGetComponent<AddonEntityComponents>()->save(),
        entity->tryGetComponent<AddonEntityComponents>()->save());
}

TEST_F(AddonEntityDefinitionTest, SavedPendingGroupChangesApplyAfterRestoration)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("pending-groups", _PACK, std::nullopt);
    auto definition = _definition("addon_test:pending_groups");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:add"] = {{"add", {{"component_groups", {"large"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:pending_groups");
    ASSERT_NE(entity, nullptr);
    ASSERT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:add").success());
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_FLOAT_EQ(restored->width(), 0.8f);
    restored->tick();
    EXPECT_FLOAT_EQ(restored->width(), 2);
}

TEST_F(AddonEntityDefinitionTest, GroupMaximumHealthRestoresBeforeSavedCurrentHealth)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("saved-health-group", _PACK, std::nullopt);
    auto definition = _definition("addon_test:saved_health_group");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"max", 32}, {"value", 24}};
    definition["minecraft:entity"]["component_groups"]["strong"] = {
        {"minecraft:health", {{"max", 128}, {"value", 96}}}};
    definition["minecraft:entity"]["events"]["test:add"] = {{"add", {{"component_groups", {"strong"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:saved_health_group");
    auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    ASSERT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:add").success());
    entity->tryGetComponent<AddonEntityComponents>()->applyPending(*entity);
    living->setHealth(70);
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    auto* restoredLiving = dynamic_cast<mc::LivingEntity*>(restored.get());
    ASSERT_NE(restoredLiving, nullptr);
    EXPECT_FLOAT_EQ(restoredLiving->maxHealth(), 128);
    EXPECT_FLOAT_EQ(restoredLiving->health(), 70);
}

TEST_F(AddonEntityDefinitionTest, UnknownGroupReferencesAndCorruptSnapshotsPreserveState)
{
    const auto directory = pack("invalid-groups", _PACK, std::nullopt);
    auto definition = _definition("addon_test:invalid_groups");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    auto invalid = definition;
    invalid["minecraft:entity"]["events"]["test:invalid"] = {{"add", {{"component_groups", {"missing"}}}}};
    EXPECT_TRUE(AddonEntityDefinition::parse(invalid).failed());
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:invalid_groups");
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto before = components->save();
    auto broken = before;
    broken["sources"]["minecraft:collision_box"] = "missing";
    EXPECT_TRUE(components->load(*entity, broken).failed());
    EXPECT_EQ(components->save(), before);
    broken = before;
    broken["pending"].push_back({{"operation", "invalid"}, {"group", "large"}});
    EXPECT_TRUE(components->load(*entity, broken).failed());
    EXPECT_EQ(components->save(), before);
    EXPECT_FLOAT_EQ(entity->width(), 0.8f);
}

TEST_F(AddonEntityDefinitionTest, RecursiveGroupEventFailureQueuesNoComponentChanges)
{
    const auto directory = pack("group-cycle", _PACK, std::nullopt);
    auto definition = _definition("addon_test:group_cycle");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:loop"] = {
        {"sequence", {{{"add", {{"component_groups", {"large"}}}}}, {{"trigger", "test:loop"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:group_cycle");
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto before = components->save();
    EXPECT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:loop").failed());
    EXPECT_EQ(components->save(), before);
}

TEST_F(AddonEntityDefinitionTest, ScriptModifiersDescribeGroupChangesAndInactiveHealthIsAbsent)
{
    const auto directory = pack("script-groups", _PACK, std::string(R"(
        import { world } from '@minecraft/server';
        globalThis.modifiers = [];
        world.afterEvents.dataDrivenEntityTrigger.subscribe(event => {
            modifiers = event.getModifiers();
        }, {eventTypes: ['test:add']});
    )"));
    auto definition = _definition("addon_test:script_groups");
    definition["minecraft:entity"]["component_groups"]["health"] = {{"minecraft:health", {{"max", 64}, {"value", 40}}}};
    definition["minecraft:entity"]["events"]["test:add"] = {{"add", {{"component_groups", {"health"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:script_groups");
    ASSERT_NE(entity, nullptr);
    auto* context = manager.pluginManager().getPlugin(_PACK)->context();
    auto& bindings = context->bindingContext();
    void* global = bindings.getGlobalObject();
    void* subject = wrapEntity(bindings, entity.get());
    bindings.setProperty(global, "subject", subject);
    bindings.releaseValue(subject);
    bindings.releaseValue(global);
    auto check = context->evaluate("if (subject.getComponent('minecraft:health') !== undefined) throw Error('inactive "
                                   "health was exposed'); subject.triggerEvent('test:add');",
        "group-trigger.js",
        EvalFlags::None);
    ASSERT_TRUE(check.success()) << check.errorMessage();
    manager.eventBus().tick();
    check = context->evaluate("if (modifiers.length !== 1 || modifiers[0].addedComponentGroups[0] !== 'health' || "
                              "modifiers[0].removedComponentGroups.length) throw Error('wrong modifiers');",
        "group-modifiers.js",
        EvalFlags::None);
    ASSERT_TRUE(check.success()) << check.errorMessage();
    entity->tryGetComponent<AddonEntityComponents>()->applyPending(*entity);
    check = context->evaluate(
        "if (subject.getComponent('minecraft:health').currentValue !== 40) throw Error('active health missing');",
        "group-health.js",
        EvalFlags::None);
    ASSERT_TRUE(check.success()) << check.errorMessage();
}

TEST_F(AddonEntityDefinitionTest, MolangPropertyQueriesReadCommittedStateWithinTheSameTick)
{
    const auto directory = pack("molang-events", _PACK, std::nullopt);
    auto definition = _definition("addon_test:molang_events");
    definition["minecraft:entity"]["events"]["test:increment"] = {
        {"set_property", {{"test:count", "q.property('test:count') + 1"}}}};
    write(directory / "entities/entity.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto entity = create("addon_test:molang_events");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(events->trigger(*entity, "test:increment").success());
    ASSERT_TRUE(events->trigger(*entity, "test:increment").success());
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    ASSERT_TRUE(events->trigger(*entity, "test:increment").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(5));
}

TEST_F(AddonEntityDefinitionTest, MaidTimerAndProgressExpressionsUseMolangArithmetic)
{
    const auto directory = pack("maid-expressions", _PACK, std::nullopt);
    auto definition = _definition("addon_test:maid_expressions");
    auto& properties = definition["minecraft:entity"]["description"]["properties"];
    properties["thlm:time"] = {{"type", "int"}, {"range", {0, 1000}}, {"default", 10}};
    properties["thlm:total_time"] = {{"type", "int"}, {"range", {0, 1000}}, {"default", 20}};
    properties["thlm:progress"] = {{"type", "float"}, {"range", {0, 1}}, {"default", 0}};
    definition["minecraft:entity"]["events"]["test:update"] = {{"set_property",
        {{"thlm:time", "q.property('thlm:time')+1"},
            {"thlm:progress", "q.property('thlm:time')/q.property('thlm:total_time')"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:maid_expressions");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:update").success());
    entity->tick();
    const auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_EQ(state.getProperty("thlm:time"), nlohmann::json(11));
    EXPECT_EQ(state.getProperty("thlm:progress"), nlohmann::json(0.5));
}

TEST_F(AddonEntityDefinitionTest, MolangBooleanAndEnumResultsMatchTheirPropertyTypes)
{
    const auto directory = pack("typed-expressions", _PACK, std::nullopt);
    auto definition = _definition("addon_test:typed_expressions");
    auto& properties = definition["minecraft:entity"]["description"]["properties"];
    properties["test:enabled"] = {{"type", "bool"}, {"default", false}};
    properties["test:mode"] = {{"type", "enum"}, {"values", {"idle", "work"}}, {"default", "idle"}};
    definition["minecraft:entity"]["events"]["test:update"] = {{"set_property",
        {{"test:enabled", "q.property('test:count') > 2"},
            {"test:mode", "q.property('test:mode') == 'idle' ? 'work' : 'idle'"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:typed_expressions");
    ASSERT_NE(entity, nullptr);
    ASSERT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:update").success());
    entity->tick();
    const auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_EQ(state.getProperty("test:enabled"), nlohmann::json(true));
    EXPECT_EQ(state.getProperty("test:mode"), nlohmann::json("work"));
}

TEST_F(AddonEntityDefinitionTest, MolangRuntimeFailureCommitsNeitherPropertiesNorComponentGroups)
{
    const auto directory = pack("expression-failure", _PACK, std::nullopt);
    auto definition = _definition("addon_test:expression_failure");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:fail"] = {{"sequence",
        {{{"add", {{"component_groups", {"large"}}}}, {"set_property", {{"test:count", 90}}}},
            {{"set_property", {{"test:count", "math.sqrt(-1)"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:expression_failure");
    ASSERT_NE(entity, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto beforeState = state.save();
    const auto beforeComponents = components->save();
    EXPECT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:fail").failed());
    EXPECT_EQ(state.save(), beforeState);
    EXPECT_EQ(components->save(), beforeComponents);
    EXPECT_FALSE(manager.eventBus().hasPendingAfterEvents());
}

TEST_F(AddonEntityDefinitionTest, FactoryEvaluatesDefaultExpressionsBeforeEntityUse)
{
    const auto directory = pack("default-expressions", _PACK, std::nullopt);
    auto definition = _definition("addon_test:default_expressions");
    auto& properties = definition["minecraft:entity"]["description"]["properties"];
    properties["test:count"]["default"] = "math.random_integer(7, 7)";
    properties["test:flag"] = {{"type", "bool"}, {"default", "math.sin(90)"}};
    properties["test:mode"] = {{"type", "enum"}, {"values", {"idle", "work"}}, {"default", "'work'"}};
    definition["minecraft:entity"]["events"]["test:increment"] = {
        {"set_property", {{"test:count", "q.property('test:count') + 1"}}}};
    write(directory / "entities/entity.json", definition);
    const auto result = manager.loadPacks("", root.string());
    ASSERT_TRUE(result.success()) << result.error().message();
    auto entity = create("addon_test:default_expressions");
    ASSERT_NE(entity, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(7));
    EXPECT_EQ(state.getProperty("test:flag"), nlohmann::json(true));
    EXPECT_EQ(state.getProperty("test:mode"), nlohmann::json("work"));
    ASSERT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:increment").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(8));
    EXPECT_EQ(state.resetProperty("test:count"), nlohmann::json(7));
}

TEST_F(AddonEntityDefinitionTest, SavedPropertyValuesOverrideExpressionDefaultsDuringRestore)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("saved-defaults", _PACK, std::nullopt);
    auto definition = _definition("addon_test:saved_defaults");
    definition["minecraft:entity"]["description"]["properties"]["test:count"]["default"] = "math.random_integer(7, 7)";
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:saved_defaults");
    ASSERT_NE(entity, nullptr);
    ASSERT_TRUE(entity->getOrAddComponent<EntityAddonState>().setProperty("test:count", 42).success());
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(42));
}

TEST_F(AddonEntityDefinitionTest, InvalidDefaultExpressionEvaluationRejectsEntityCreation)
{
    const auto directory = pack("invalid-default", _PACK, std::nullopt);
    auto definition = _definition("addon_test:invalid_default");
    definition["minecraft:entity"]["description"]["properties"]["test:count"]["default"] = "math.sqrt(-1)";
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    const auto* type = mc::entity::EntityRegistry::instance().getType("addon_test:invalid_default");
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->create(nullptr, entities), nullptr);
}

TEST_F(AddonEntityDefinitionTest, EventFiltersReadCommittedPropertiesAndGateGroupChanges)
{
    const auto directory = pack("filtered-event", _PACK, std::nullopt);
    auto definition = _definition("addon_test:filtered_event");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:conditional"] = {
        {"filters", {{"test", "int_property"}, {"domain", "test:count"}, {"operator", ">"}, {"value", 5}}},
        {"add", {{"component_groups", {"large"}}}},
        {"set_property", {{"test:count", 9}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:filtered_event");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(state.setProperty("test:count", 6).success());
    ASSERT_TRUE(events->trigger(*entity, "test:conditional").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(6));
    EXPECT_FLOAT_EQ(entity->width(), 0.8f);
    ASSERT_TRUE(events->trigger(*entity, "test:conditional").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(9));
    EXPECT_FLOAT_EQ(entity->width(), 2);
}

TEST_F(AddonEntityDefinitionTest, FamilyFiltersUseTheCurrentlyActiveFamilyComponent)
{
    const auto directory = pack("family-event", _PACK, std::nullopt);
    auto definition = _definition("addon_test:family_event");
    definition["minecraft:entity"]["components"]["minecraft:type_family"] = {{"family", {"maid", "mob"}}};
    definition["minecraft:entity"]["component_groups"]["worker"] = {
        {"minecraft:type_family", {{"family", {"worker"}}}}};
    definition["minecraft:entity"]["events"] = {{"test:activate", {{"add", {{"component_groups", {"worker"}}}}}},
        {"test:deactivate", {{"remove", {{"component_groups", {"worker"}}}}}},
        {"test:work",
            {{"filters", {{"test", "is_family"}, {"value", "worker"}}}, {"set_property", {{"test:count", 66}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:family_event");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(events->trigger(*entity, "test:activate").success());
    ASSERT_TRUE(events->trigger(*entity, "test:work").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    ASSERT_TRUE(events->trigger(*entity, "test:work").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(66));
    ASSERT_TRUE(events->trigger(*entity, "test:deactivate").success());
    entity->tick();
    auto originalFamily = EntityFilter::compile({{"test", "is_family"}, {"value", "maid"}});
    ASSERT_TRUE(originalFamily.success());
    auto matches = originalFamily.value().matches(
        EntityFilterContext{*entity, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
    ASSERT_TRUE(matches.success());
    EXPECT_FALSE(matches.value());
}

TEST_F(AddonEntityDefinitionTest, EventFiltersReceiveExplicitOtherEntityContext)
{
    const auto directory = pack("interaction-filter", _PACK, std::nullopt);
    auto definition = _definition("addon_test:interaction_filter");
    definition["minecraft:entity"]["events"]["test:interact"] = {
        {"filters", {{"test", "int_property"}, {"subject", "other"}, {"domain", "test:count"}, {"value", 99}}},
        {"set_property", {{"test:count", 12}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:interaction_filter");
    auto other = create("addon_test:interaction_filter");
    ASSERT_NE(entity, nullptr);
    ASSERT_NE(other, nullptr);
    ASSERT_TRUE(other->getOrAddComponent<EntityAddonState>().setProperty("test:count", 99).success());
    other->tick();
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:interact").success());
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(3));
    ASSERT_TRUE(events
            ->trigger(
                EntityFilterContext{*entity, other.get(), nullptr, nullptr, nullptr, nullptr, nullptr}, "test:interact")
            .success());
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(12));
}

TEST_F(AddonEntityDefinitionTest, FilterEvaluationFailureAbortsTheWholeEventTransaction)
{
    const auto directory = pack("failed-filter", _PACK, std::nullopt);
    auto definition = _definition("addon_test:failed_filter");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:fail"] = {{"sequence",
        {{{"add", {{"component_groups", {"large"}}}}, {"set_property", {{"test:count", 90}}}},
            {{"filters", {{"test", "is_family"}, {"subject", "other"}, {"value", "unmapped-native-family"}}},
                {"set_property", {{"test:count", 80}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:failed_filter");
    ASSERT_NE(entity, nullptr);
    mc::Entity other(mc::EntityInstanceId(2), nullptr, entities);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto before = components->save();
    EXPECT_TRUE(entity->tryGetComponent<AddonEntityEvents>()
            ->trigger(EntityFilterContext{*entity, &other, nullptr, nullptr, nullptr, nullptr, nullptr}, "test:fail")
            .failed());
    EXPECT_EQ(components->save(), before);
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(3));
    EXPECT_FALSE(manager.eventBus().hasPendingAfterEvents());
}

TEST_F(AddonEntityDefinitionTest, FixedOneShotTimerFiresAfterItsDurationAndAppliesEventNextTick)
{
    const auto directory = pack("one-shot", _PACK, std::nullopt);
    auto definition = _definition("addon_test:one_shot");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 0.1}, {"looping", false}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = {
        {"set_property", {{"test:count", "q.property('test:count')+1"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:one_shot");
    ASSERT_NE(entity, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    for (int index = 0; index < 8; ++index) {
        entity->tick();
    }
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["timer"]["state"], "stopped");
}

TEST_F(AddonEntityDefinitionTest, LoopingTimerContinuesToAdvanceThePropertyEventChain)
{
    const auto directory = pack("loop-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:loop_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 0.05}, {"time_down_event", {{"event", "test:fire"}, {"target", "self"}}}};
    definition["minecraft:entity"]["events"]["test:fire"] = {
        {"set_property", {{"test:count", "q.property('test:count')+1"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:loop_timer");
    ASSERT_NE(entity, nullptr);
    for (int index = 0; index < 5; ++index) {
        entity->tick();
    }
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(7));
    const auto timer = entity->tryGetComponent<AddonEntityComponents>()->save()["timer"];
    EXPECT_EQ(timer["state"], "running");
    EXPECT_EQ(timer["remaining"], 1);
}

TEST_F(AddonEntityDefinitionTest, TimerGroupReadditionRestartsAndRemovalStopsTheTimer)
{
    const auto directory = pack("timer-group", _PACK, std::nullopt);
    auto definition = _definition("addon_test:timer_group");
    definition["minecraft:entity"]["component_groups"]["clock"] = {
        {"minecraft:timer", {{"time", 0.15}, {"looping", false}, {"time_down_event", "test:fire"}}}};
    definition["minecraft:entity"]["events"] = {{"test:start", {{"add", {{"component_groups", {"clock"}}}}}},
        {"test:stop", {{"remove", {{"component_groups", {"clock"}}}}}},
        {"test:fire", {{"set_property", {{"test:count", "q.property('test:count')+1"}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:timer_group");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(events->trigger(*entity, "test:start").success());
    entity->tick();
    entity->tick();
    ASSERT_TRUE(events->trigger(*entity, "test:start").success());
    entity->tick();
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    ASSERT_TRUE(events->trigger(*entity, "test:start").success());
    entity->tick();
    ASSERT_TRUE(events->trigger(*entity, "test:stop").success());
    for (int index = 0; index < 6; ++index) {
        entity->tick();
    }
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["timer"]["state"], "absent");
}

TEST_F(AddonEntityDefinitionTest, SavedTimerRemainingTicksAreRestoredWithoutRestartingTheInterval)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("saved-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:saved_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 0.25}, {"looping", false}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = {{"set_property", {{"test:count", 8}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:saved_timer");
    ASSERT_NE(entity, nullptr);
    entity->tick();
    entity->tick();
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["timer"]["remaining"], 3);
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->tryGetComponent<AddonEntityComponents>()->save()["timer"]["remaining"], 3);
    for (int index = 0; index < 3; ++index) {
        restored->tick();
    }
    EXPECT_EQ(restored->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(3));
    restored->tick();
    EXPECT_EQ(restored->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(8));
}

TEST_F(AddonEntityDefinitionTest, CompletedOneShotTimerDoesNotFireAgainAfterRestore)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("completed-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:completed_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 0}, {"looping", false}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = {
        {"set_property", {{"test:count", "q.property('test:count')+1"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:completed_timer");
    ASSERT_NE(entity, nullptr);
    entity->tick();
    entity->tick();
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success());
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    for (int index = 0; index < 6; ++index) {
        restored->tick();
    }
    EXPECT_EQ(restored->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(4));
    EXPECT_EQ(restored->tryGetComponent<AddonEntityComponents>()->save()["timer"]["state"], "stopped");
}

TEST_F(AddonEntityDefinitionTest, WeightedTimerChoicesIgnoreZeroWeights)
{
    const auto directory = pack("weighted-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:weighted_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"random_time_choices", {{{"weight", 0}, {"value", 100}}, {{"weight", 1}, {"value", 0.1}}}},
        {"looping", false},
        {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = {{"set_property", {{"test:count", 8}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:weighted_timer");
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["timer"]["interval"], 2);
    for (int index = 0; index < 3; ++index) {
        entity->tick();
    }
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(8));
}

TEST_F(AddonEntityDefinitionTest, NonRandomLoopReusesItsChosenInterval)
{
    const auto directory = pack("fixed-random-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:fixed_random_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", {0.1, 0.3}}, {"randomInterval", false}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = nlohmann::json::object();
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:fixed_random_timer");
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto interval = components->save()["timer"]["interval"];
    EXPECT_GE(interval.get<int>(), 2);
    EXPECT_LE(interval.get<int>(), 6);
    for (int index = 0; index < 40; ++index) {
        entity->tick();
        EXPECT_EQ(components->save()["timer"]["interval"], interval);
    }
}

TEST_F(AddonEntityDefinitionTest, TimerEventReferenceFiltersGateTheExpiryAction)
{
    const auto directory = pack("filtered-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:filtered_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {{"time", 0.05},
        {"time_down_event",
            {{"event", "test:fire"},
                {"filters", {{"test", "int_property"}, {"domain", "test:count"}, {"operator", ">"}, {"value", 5}}}}}};
    definition["minecraft:entity"]["events"]["test:fire"] = {{"set_property", {{"test:count", 8}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:filtered_timer");
    ASSERT_NE(entity, nullptr);
    entity->tick();
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(3));
    ASSERT_TRUE(entity->getOrAddComponent<EntityAddonState>().setProperty("test:count", 6).success());
    entity->tick();
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(8));
}

TEST_F(AddonEntityDefinitionTest, InvalidTimerDefinitionsAndUnknownEventsAreRejected)
{
    const std::vector<nlohmann::json> timers = {{{"time", -1}, {"time_down_event", "test:fire"}},
        {{"time", {2, 1}}, {"time_down_event", "test:fire"}},
        {{"time", {1}}, {"time_down_event", "test:fire"}},
        {{"time", 1}, {"looping", "true"}, {"time_down_event", "test:fire"}},
        {{"time", 1}, {"random_time_choices", {{{"value", 2}}}}, {"time_down_event", "test:fire"}},
        {{"random_time_choices", {{{"value", 2}, {"weight", 0}}}}, {"time_down_event", "test:fire"}},
        {{"time", 1}, {"time_down_event", {{"event", "test:fire"}, {"target", "other"}}}},
        {{"time", 1}}};
    for (const auto& timer : timers) {
        EXPECT_TRUE(EntityTimerState::validateDefinition(timer).failed()) << timer.dump();
    }
    auto definition = _definition("addon_test:unknown_timer_event");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 1}, {"time_down_event", "test:missing"}};
    EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed());
}

TEST_F(AddonEntityDefinitionTest, InvalidSavedTimerDoesNotMutateLiveComponentState)
{
    const auto directory = pack("corrupt-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:corrupt_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {{"time", 1}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = nlohmann::json::object();
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:corrupt_timer");
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto before = components->save();
    auto corrupt = before;
    corrupt["timer"]["remaining"] = 1000;
    EXPECT_TRUE(components->load(*entity, corrupt).failed());
    EXPECT_EQ(components->save(), before);
    corrupt = before;
    corrupt["timer"]["state"] = "stopped";
    EXPECT_TRUE(components->load(*entity, corrupt).failed());
    EXPECT_EQ(components->save(), before);
}
