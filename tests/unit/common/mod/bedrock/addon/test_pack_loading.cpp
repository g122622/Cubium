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

#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"
#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"
#include "common/mod/bedrock/addon/pack/AddonManifest.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace mc::mod::bedrock::addon;

namespace {
constexpr const char* _A = "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa";
constexpr const char* _B = "bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb";

class AddonPackLoadingTest : public testing::Test {
protected:
    void SetUp() override
    {
        static std::atomic<unsigned> sequence{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        root = std::filesystem::temp_directory_path() /
            ("cubium-addon-loading-" + std::to_string(stamp) + "-" + std::to_string(sequence++));
        ASSERT_TRUE(std::filesystem::create_directory(root));
        ASSERT_TRUE(manager.initialize().success());
        ScriptWorldAccessor::instance().setMessageCallback(
            [this](const std::string& value) { messages.push_back(value); });
    }
    void TearDown() override
    {
        manager.shutdown();
        ScriptWorldAccessor::instance().setMessageCallback({});
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }
    std::filesystem::path pack(const std::string& folder,
        const std::string& uuid,
        nlohmann::json dependencies,
        std::optional<std::string> source,
        const std::string& type)
    {
        const auto path = root / folder;
        std::filesystem::create_directories(path / "scripts");
        nlohmann::json module = {{"type", type}, {"uuid", uuid}, {"version", {1, 0, 0}}};
        if (type == "script") {
            module["entry"] = "scripts/main.js";
            module["language"] = "javascript";
        }
        const nlohmann::json manifest = {{"format_version", 2},
            {"header", {{"name", folder}, {"uuid", uuid}, {"version", {1, 0, 0}}, {"min_engine_version", {1, 21, 90}}}},
            {"modules", nlohmann::json::array({module})},
            {"dependencies", dependencies}};
        std::ofstream(path / "manifest.json") << manifest.dump();
        if (source) std::ofstream(path / "scripts/main.js") << *source;
        return path;
    }
    nlohmann::json native(const std::string& version)
    {
        return nlohmann::json::array({{{"module_name", "@minecraft/server"}, {"version", version}}});
    }
    ScriptManager manager;
    std::filesystem::path root;
    std::vector<std::string> messages;
};
} // namespace

TEST_F(AddonPackLoadingTest, MissingScriptEntryFailsLoading)
{
    pack("missing", _A, native("2.0.0"), std::nullopt, "script");
    const auto result = manager.loadPacks("", root.string());
    EXPECT_TRUE(result.failed());
    EXPECT_EQ(manager.pluginManager().pluginCount(), 0u);
}

TEST_F(AddonPackLoadingTest, MissingNativeModuleFailsBeforeScriptExecution)
{
    pack("missing-module",
        _A,
        nlohmann::json::array({{{"module_name", "@minecraft/unavailable"}, {"version", "2.0.0"}}}),
        std::string("globalThis.executed = true;"),
        "script");
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_EQ(manager.pluginManager().pluginCount(), 0u);
}

TEST_F(AddonPackLoadingTest, UnsupportedModuleVersionFails)
{
    pack("wrong-version", _A, native("99.0.0"), std::string("export {};"), "script");
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
}

TEST_F(AddonPackLoadingTest, MalformedModuleVersionIsRejected)
{
    pack("bad-version", _A, native("2.0.invalid"), std::string("export {};"), "script");
    const auto result = manager.loadPacks("", root.string());
    ASSERT_TRUE(result.failed());
    EXPECT_NE(result.error().message().find("Invalid module version"), std::string::npos);
}

TEST_F(AddonPackLoadingTest, BetaVersionResolvesAvailableFactory)
{
    pack("beta",
        _A,
        native("beta"),
        std::string("import { world } from '@minecraft/server'; world.sendMessage('beta-loaded');"),
        "script");
    const auto result = manager.loadPacks("", root.string());
    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_EQ(messages, std::vector<std::string>{"beta-loaded"});
}

TEST_F(AddonPackLoadingTest, MissingPackDependencyIsFatal)
{
    auto dependencies = native("2.0.0");
    dependencies.push_back({{"uuid", _B}, {"version", {1, 0, 0}}});
    pack("dependent", _A, dependencies, std::string("export {};"), "script");
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_EQ(manager.pluginManager().pluginCount(), 0u);
}

TEST_F(AddonPackLoadingTest, DependenciesLoadBeforeHigherPriorityPack)
{
    auto dependencies = native("2.0.0");
    dependencies.push_back({{"uuid", _B}, {"version", {1, 0, 0}}});
    const auto dependent = pack("dependent",
        _A,
        dependencies,
        std::string("import { world } from '@minecraft/server'; world.sendMessage('dependent');"),
        "script");
    const auto dependency = pack("dependency",
        _B,
        native("2.0.0"),
        std::string("import { world } from '@minecraft/server'; world.sendMessage('dependency');"),
        "script");
    ASSERT_TRUE(manager.packList()->addPack(dependent.string(), true, 100).success());
    ASSERT_TRUE(manager.packList()->addPack(dependency.string(), true, 0).success());
    const auto result = manager.pluginManager().loadPlugins(manager.engine(), *manager.packList());
    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_EQ(messages, (std::vector<std::string>{"dependency", "dependent"}));
}

TEST_F(AddonPackLoadingTest, DisabledPackCannotSatisfyDependency)
{
    auto dependencies = native("2.0.0");
    dependencies.push_back({{"uuid", _B}, {"version", {1, 0, 0}}});
    const auto dependent = pack("dependent", _A, dependencies, std::string("export {};"), "script");
    const auto dependency = pack("disabled", _B, native("2.0.0"), std::string("export {};"), "script");
    ASSERT_TRUE(manager.packList()->addPack(dependent.string(), true, 0).success());
    ASSERT_TRUE(manager.packList()->addPack(dependency.string(), false, 0).success());
    EXPECT_TRUE(manager.pluginManager().loadPlugins(manager.engine(), *manager.packList()).failed());
}

TEST_F(AddonPackLoadingTest, ScriptDependencyCycleFailsBeforeExecution)
{
    auto first = native("2.0.0");
    auto second = native("2.0.0");
    first.push_back({{"uuid", _B}, {"version", {1, 0, 0}}});
    second.push_back({{"uuid", _A}, {"version", {1, 0, 0}}});
    pack("first", _A, first, std::string("export {};"), "script");
    pack("second", _B, second, std::string("export {};"), "script");
    const auto result = manager.loadPacks("", root.string());
    ASSERT_TRUE(result.failed());
    EXPECT_NE(result.error().message().find("Cyclic"), std::string::npos);
    EXPECT_EQ(manager.pluginManager().pluginCount(), 0u);
}

TEST_F(AddonPackLoadingTest, ReciprocalBehaviorResourceDependenciesAreAccepted)
{
    pack("behavior", _A, nlohmann::json::array({{{"uuid", _B}, {"version", {1, 0, 0}}}}), std::nullopt, "data");
    pack("resource", _B, nlohmann::json::array({{{"uuid", _A}, {"version", {1, 0, 0}}}}), std::nullopt, "resources");
    EXPECT_TRUE(manager.loadPacks("", root.string()).success());
}

TEST_F(AddonPackLoadingTest, FailedBatchUnloadsItsEarlierPluginsAndTimers)
{
    pack("first",
        _A,
        native("2.0.0"),
        std::string("import { system } from '@minecraft/server'; system.runInterval(() => {}, 1);"),
        "script");
    pack("broken", _B, native("2.0.0"), std::string("invalid javascript ???"), "script");
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_EQ(manager.pluginManager().pluginCount(), 0u);
    EXPECT_EQ(manager.scheduler().pendingCount(), 0u);
}

TEST_F(AddonPackLoadingTest, InvalidManifestIsPropagated)
{
    std::filesystem::create_directory(root / "invalid");
    std::ofstream(root / "invalid/manifest.json") << "invalid JSON";
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
}

TEST_F(AddonPackLoadingTest, PackResourcesCannotEscapeTheirRoot)
{
    const auto folder = pack("bounded", _A, native("2.0.0"), std::string("export {};"), "script");
    std::ofstream(root / "outside.js") << "outside";
    const auto manifest = AddonManifest::loadFromFile((folder / "manifest.json").string());
    ASSERT_TRUE(manifest.success());
    BehaviorPack behavior(folder.string(), manifest.value());
    EXPECT_TRUE(behavior.readScriptFile("scripts/../scripts/main.js").success());
    EXPECT_TRUE(behavior.readScriptFile("../outside.js").failed());
    EXPECT_TRUE(behavior.readResource("../outside.js").failed());
    EXPECT_TRUE(behavior.readScriptFile((root / "outside.js").string()).failed());
}
