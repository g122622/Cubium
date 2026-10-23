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
#include "common/mod/bedrock/addon/content/base/properties/AddonDynamicProperties.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPlugin.hpp"
#include "server/world/storage/SingleLevelStorageManager.hpp"

#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {
constexpr const char* _FIRST = "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa";
constexpr const char* _SECOND = "bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb";

class AddonWorldPropertiesTest : public testing::Test {
protected:
    void SetUp() override
    {
        path = mc::test::makeUniqueTestDir("addon-world-properties");
        std::filesystem::create_directories(path / "packs");
        // 存储门面只打开已有存档目录，目录创建由存档管理流程负责。
        std::filesystem::create_directories(path / "world");
        ASSERT_TRUE(manager.initialize().success());
    }

    void TearDown() override
    {
        manager.shutdown();
        mc::test::removeTestDir(path);
    }

    void pack(const std::string& uuid, const std::string& source)
    {
        const auto directory = path / "packs" / uuid;
        std::filesystem::create_directories(directory / "scripts");
        const nlohmann::json manifest = {{"format_version", 2},
            {"header", {{"name", uuid}, {"uuid", uuid}, {"version", {1, 0, 0}}, {"min_engine_version", {1, 21, 90}}}},
            {"modules",
                {{{"type", "script"},
                    {"uuid", "cccccccc-cccc-4ccc-8ccc-cccccccccccc"},
                    {"version", {1, 0, 0}},
                    {"language", "javascript"},
                    {"entry", "scripts/main.js"}}}},
            {"dependencies", {{{"module_name", "@minecraft/server"}, {"version", "2.0.0"}}}}};
        std::ofstream(directory / "manifest.json") << manifest.dump();
        std::ofstream(directory / "scripts/main.js")
            << "import { world } from '@minecraft/server'; globalThis.world = world;\n"
            << source;
    }

    void load(ScriptManager& target)
    {
        const auto result = target.loadPacks("", (path / "packs").string());
        ASSERT_TRUE(result.success()) << result.error().message();
    }

    void check(ScriptManager& target, const std::string& uuid, const std::string& source)
    {
        auto* plugin = target.pluginManager().getPlugin(uuid);
        ASSERT_NE(plugin, nullptr);
        ASSERT_NE(plugin->context(), nullptr);
        const auto result = plugin->context()->evaluate(source, "world-properties.js", EvalFlags::None);
        ASSERT_TRUE(result.success()) << result.errorMessage();
    }

    std::filesystem::path path;
    ScriptManager manager;
};
} // namespace

TEST(AddonDynamicPropertiesTest, DirtyStateTracksChangesAndSuccessfulSaves)
{
    AddonDynamicProperties values;
    EXPECT_FALSE(values.isDirty());
    ASSERT_TRUE(values.set("pack", "missing", nullptr).success());
    EXPECT_FALSE(values.isDirty());
    ASSERT_TRUE(values.set("pack", "key", true).success());
    EXPECT_TRUE(values.isDirty());
    values.markSaved();
    ASSERT_TRUE(values.set("pack", "key", true).success());
    EXPECT_FALSE(values.isDirty());
    EXPECT_TRUE(values.set("pack", "key", std::numeric_limits<double>::infinity()).failed());
    EXPECT_FALSE(values.isDirty());
    values.clear("other-pack");
    EXPECT_FALSE(values.isDirty());
    values.clear("pack");
    EXPECT_TRUE(values.isDirty());
    EXPECT_TRUE(values.save().empty());
}

TEST(AddonDynamicPropertiesTest, LoadIsAtomicAndRestoredValuesAreClean)
{
    AddonDynamicProperties values;
    ASSERT_TRUE(values
            .setMany("pack",
                {{"flag", false}, {"number", 1.5}, {"text", "女仆"}, {"position", {{"x", 1}, {"y", 2}, {"z", 3}}}})
            .success());
    const auto saved = values.save();
    AddonDynamicProperties restored;
    ASSERT_TRUE(restored.load(saved).success());
    EXPECT_FALSE(restored.isDirty());
    EXPECT_EQ(restored.save(), saved);
    auto malformed = saved;
    malformed["pack"]["bad"] = {{"kind", "boolean"}, {"value", 2}};
    EXPECT_TRUE(values.load(malformed).failed());
    EXPECT_TRUE(values.isDirty());
    EXPECT_EQ(values.save(), saved);
    malformed = saved;
    malformed["pack"]["bad"] = {{"kind", "vector"}, {"value", {{"x", 1}, {"y", 2}}}};
    EXPECT_TRUE(values.load(malformed).failed());
    EXPECT_EQ(values.save(), saved);
    EXPECT_TRUE(values.load({{"", nlohmann::json::object()}}).failed());
    EXPECT_TRUE(values.load({{"pack", nlohmann::json::array()}}).failed());
}

TEST_F(AddonWorldPropertiesTest, ScriptsUseHeaderUuidAndCannotAccessAnotherPack)
{
    pack(_FIRST,
        "world.setDynamicProperties({same: 'first', flag: false, n: 2.5, text: '女仆', position: {x: 1, y: 2, z: "
        "3}});");
    pack(_SECOND,
        "if (world.getDynamicProperty('same') !== undefined) throw Error('cross-pack read'); "
        "world.setDynamicProperty('same', 'second');");
    load(manager);
    const auto snapshot = manager.saveWorldProperties();
    EXPECT_TRUE(snapshot.contains(_FIRST));
    EXPECT_TRUE(snapshot.contains(_SECOND));
    EXPECT_FALSE(snapshot.contains("cccccccc-cccc-4ccc-8ccc-cccccccccccc"));
    check(manager, _FIRST, R"(
        if (world.getDynamicProperty('same') !== 'first') throw Error('wrong scope');
        if (world.getDynamicProperty('flag') !== false) throw Error('boolean conversion');
        if (world.getDynamicProperty('n') !== 2.5) throw Error('number conversion');
        if (world.getDynamicProperty('text') !== '女仆') throw Error('string conversion');
        if (world.getDynamicProperty('position').z !== 3) throw Error('vector conversion');
        if (world.getDynamicPropertyIds().length !== 5) throw Error('key enumeration');
    )");
    check(manager,
        _SECOND,
        "world.clearDynamicProperties(); if (world.getDynamicPropertyIds().length) throw Error('clear failed');");
    check(manager, _FIRST, "if (world.getDynamicProperty('same') !== 'first') throw Error('cross-pack clear');");
    EXPECT_TRUE(manager.worldPropertiesDirty());
}

TEST_F(AddonWorldPropertiesTest, ScriptReloadPreservesWorldProperties)
{
    pack(_FIRST, "world.setDynamicProperty('loads', (world.getDynamicProperty('loads') ?? 0) + 1);");
    load(manager);
    manager.markWorldPropertiesSaved();
    EXPECT_FALSE(manager.worldPropertiesDirty());
    manager.reload();
    load(manager);
    check(manager, _FIRST, "if (world.getDynamicProperty('loads') !== 2) throw Error('reload lost state');");
    EXPECT_TRUE(manager.worldPropertiesDirty());
}

TEST_F(AddonWorldPropertiesTest, ManagersHaveIndependentWorldState)
{
    pack(_FIRST, "world.setDynamicProperty('loads', (world.getDynamicProperty('loads') ?? 0) + 1);");
    load(manager);
    ScriptManager other;
    ASSERT_TRUE(other.initialize().success());
    load(other);
    check(other,
        _FIRST,
        "if (world.getDynamicProperty('loads') !== 1) throw Error('world state leaked'); "
        "world.setDynamicProperty('loads', 50);");
    check(manager, _FIRST, "if (world.getDynamicProperty('loads') !== 1) throw Error('world state was overwritten');");
    other.shutdown();
    check(manager,
        _FIRST,
        "world.setDynamicProperty('active', true); if (world.getDynamicProperty('active') !== true) throw Error('other "
        "shutdown broke state');");
}

TEST_F(AddonWorldPropertiesTest, InvalidScriptBatchesDoNotWriteAndDeletionIsImmediate)
{
    pack(_FIRST, "world.setDynamicProperty('saved', true);");
    load(manager);
    manager.markWorldPropertiesSaved();
    check(manager, _FIRST, R"(
        const operations = [() => world.setDynamicProperties({good: true, bad: 'x'.repeat(32768)}),
            () => world.setDynamicProperty('bad', NaN), () => world.setDynamicProperty('bad', Infinity),
            () => world.setDynamicProperty('bad', [1,2,3]), () => world.setDynamicProperty('bad', {x:1,y:2}),
            () => world.setDynamicProperties([]), () => world.setDynamicProperty('', true),
            () => world.setDynamicProperty.call({}, 'bad', true)];
        for (const operation of operations) {
            let rejected = false;
            try { operation(); } catch { rejected = true; }
            if (!rejected) throw Error('invalid write accepted');
        }
        if (world.getDynamicProperty('good') !== undefined) throw Error('partial write');
    )");
    EXPECT_FALSE(manager.worldPropertiesDirty());
    check(manager,
        _FIRST,
        "world.setDynamicProperties({saved: undefined}); if (world.getDynamicProperty('saved') !== undefined) throw "
        "Error('delete failed');");
    EXPECT_TRUE(manager.worldPropertiesDirty());
    EXPECT_TRUE(manager.saveWorldProperties().empty());
}

TEST_F(AddonWorldPropertiesTest, RestartRestoresDataBeforePluginEntryRuns)
{
    using namespace mc::world::storage;
    pack(_FIRST, R"(
        world.setDynamicProperty('loads', (world.getDynamicProperty('loads') ?? 0) + 1);
        world.setDynamicProperties({flag: true, text: '女仆', n: 2.5, vector: {x: 1, y: 2, z: 3}});
    )");
    load(manager);
    SingleLevelStorageConfig config;
    config.enableBackup = false;
    SingleLevelStorageManager storage;
    ASSERT_TRUE(storage.open(path / "world", config).success());
    auto missing = storage.loadAddonWorldProperties();
    ASSERT_TRUE(missing.success());
    EXPECT_FALSE(missing.value().has_value());
    const auto snapshot = manager.saveWorldProperties();
    ASSERT_TRUE(storage.saveAddonWorldProperties(snapshot, true).success());
    manager.markWorldPropertiesSaved();
    manager.shutdown();
    storage.close();

    SingleLevelStorageManager reopened;
    ASSERT_TRUE(reopened.open(path / "world", config).success());
    auto saved = reopened.loadAddonWorldProperties();
    ASSERT_TRUE(saved.success());
    ASSERT_TRUE(saved.value().has_value());
    EXPECT_EQ(*saved.value(), snapshot);
    ScriptManager restored;
    ASSERT_TRUE(restored.loadWorldProperties(*saved.value()).success());
    EXPECT_FALSE(restored.worldPropertiesDirty());
    ASSERT_TRUE(restored.initialize().success());
    load(restored);
    check(restored, _FIRST, R"(
        if (world.getDynamicProperty('loads') !== 2) throw Error('state unavailable at entry');
        if (world.getDynamicProperty('flag') !== true || world.getDynamicProperty('text') !== '女仆') throw Error('types lost');
    )");
    restored.shutdown();
    reopened.close();
}

TEST_F(AddonWorldPropertiesTest, InvalidRestorePreservesDirtyStateAndSnapshot)
{
    pack(_FIRST, "world.setDynamicProperty('flag', true);");
    load(manager);
    const auto before = manager.saveWorldProperties();
    auto invalid = before;
    invalid[_FIRST]["broken"] = {{"kind", "number"}, {"value", "bad"}};
    EXPECT_TRUE(manager.loadWorldProperties(invalid).failed());
    EXPECT_EQ(manager.saveWorldProperties(), before);
    EXPECT_TRUE(manager.worldPropertiesDirty());
}

TEST_F(AddonWorldPropertiesTest, GetterAndProxyExceptionsAreRethrownWithoutCommittingWrites)
{
    pack(_FIRST, "world.setDynamicProperty('saved', true);");
    load(manager);
    manager.markWorldPropertiesSaved();
    check(manager, _FIRST, R"(
        const sentinel = {reason: 'getter failed'};
        const cases = [
            () => world.setDynamicProperties({good: 1, get saved() { throw sentinel; }}),
            () => world.setDynamicProperties(new Proxy({}, {ownKeys() { throw sentinel; }})),
            () => world.setDynamicProperty('saved', {get x() { throw sentinel; }, y: 2, z: 3})
        ];
        for (const operation of cases) {
            let caught = false;
            try { operation(); } catch (error) {
                if (error !== sentinel) throw Error('exception identity was lost');
                caught = true;
            }
            if (!caught) throw Error('exception was swallowed');
        }
        if (world.getDynamicProperty('saved') !== true) throw Error('getter failure changed data');
        if (world.getDynamicProperty('good') !== undefined) throw Error('getter failure partially committed');
    )");
    EXPECT_FALSE(manager.worldPropertiesDirty());
}

TEST_F(AddonWorldPropertiesTest, StorageRejectsClosedReadonlyAndMalformedWrites)
{
    using namespace mc::world::storage;
    SingleLevelStorageManager storage;
    EXPECT_TRUE(storage.loadAddonWorldProperties().failed());
    EXPECT_TRUE(storage.saveAddonWorldProperties(nlohmann::json::object(), true).failed());
    SingleLevelStorageConfig config;
    config.enableBackup = false;
    ASSERT_TRUE(storage.open(path / "world", config).success());
    const nlohmann::json snapshot = {{"pack", {{"flag", {{"kind", "boolean"}, {"value", true}}}}}};
    ASSERT_TRUE(storage.saveAddonWorldProperties(snapshot, true).success());
    EXPECT_TRUE(storage.saveAddonWorldProperties(nlohmann::json::array(), true).failed());
    storage.close();
    config.readonly = true;
    ASSERT_TRUE(storage.open(path / "world", config).success());
    EXPECT_TRUE(storage.saveAddonWorldProperties(nlohmann::json::object(), true).failed());
    auto saved = storage.loadAddonWorldProperties();
    ASSERT_TRUE(saved.success());
    ASSERT_TRUE(saved.value().has_value());
    EXPECT_EQ(*saved.value(), snapshot);
    storage.close();
}
