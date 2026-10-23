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
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"
#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
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
    definition["minecraft:entity"]["events"]["test:change"] = {{"set_property", {{"test:count", 5}}}};
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
