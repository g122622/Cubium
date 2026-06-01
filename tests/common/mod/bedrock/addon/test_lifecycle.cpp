#include <gtest/gtest.h>

#include <chrono>

#include "common/mod/bedrock/addon/diagnostics/ScriptDiagnostics.hpp"
#include "common/mod/bedrock/addon/diagnostics/ScriptSentryLogger.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptLogger.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptTickListener.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptWatchdog.hpp"
#include "common/mod/bedrock/addon/pack/AddonManifest.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"
#include "common/mod/bedrock/addon/plugin/PluginExecutionGroup.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPackConfiguration.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPackPermissions.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPlugin.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPluginManager.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPluginSource.hpp"

using namespace mc::mod::bedrock::addon;

// ===== PluginExecutionGroup =====

TEST(PluginExecutionGroupTest, NameReturnsCorrectStrings)
{
    EXPECT_STREQ(pluginExecutionGroupName(PluginExecutionGroup::PrePackLoad), "PrePackLoad");
    EXPECT_STREQ(pluginExecutionGroupName(PluginExecutionGroup::ServerStart), "ServerStart");
    EXPECT_STREQ(pluginExecutionGroupName(PluginExecutionGroup::ClientLevel), "ClientLevel");
}

// ===== ScriptPackPermissions =====

TEST(ScriptPackPermissionsTest, DefaultHasNoPermissions)
{
    ScriptPackPermissions perms;
    EXPECT_FALSE(perms.hasPermission(ScriptPermission::AllowEval));
}

TEST(ScriptPackPermissionsTest, SetPermissionWorks)
{
    ScriptPackPermissions perms;
    perms.setPermission(ScriptPermission::AllowEval);
    EXPECT_TRUE(perms.hasPermission(ScriptPermission::AllowEval));
}

TEST(ScriptPackPermissionsTest, ParseFromCapabilities)
{
    std::vector<std::string> caps = {"script_eval"};
    ScriptPackPermissions perms(caps);
    EXPECT_TRUE(perms.hasPermission(ScriptPermission::AllowEval));
}

TEST(ScriptPackPermissionsTest, UnknownCapabilityIgnored)
{
    std::vector<std::string> caps = {"unknown_capability"};
    ScriptPackPermissions perms(caps);
    EXPECT_FALSE(perms.hasPermission(ScriptPermission::AllowEval));
}

TEST(ScriptPackPermissionsTest, UnsetPermission)
{
    ScriptPackPermissions perms;
    perms.setPermission(ScriptPermission::AllowEval);
    perms.setPermission(ScriptPermission::AllowEval, false);
    EXPECT_FALSE(perms.hasPermission(ScriptPermission::AllowEval));
}

// ===== ScriptPackConfiguration =====

TEST(ScriptPackConfigurationTest, EmptyAllowsAllModules)
{
    ScriptPackConfiguration config;
    EXPECT_TRUE(config.isModuleAllowed("@minecraft/server"));
    EXPECT_TRUE(config.isModuleAllowed("any_module"));
}

TEST(ScriptPackConfigurationTest, AllowedModulesListRestricts)
{
    ScriptPackConfiguration config;
    config.setAllowedModules({"@minecraft/server"});
    EXPECT_TRUE(config.isModuleAllowed("@minecraft/server"));
    EXPECT_FALSE(config.isModuleAllowed("@minecraft/server-ui"));
}

TEST(ScriptPackConfigurationTest, ExcludedModulesBlocksAccess)
{
    ScriptPackConfiguration config;
    config.setExcludedModules({"@minecraft/server-net"});
    EXPECT_TRUE(config.isModuleAllowed("@minecraft/server"));
    EXPECT_FALSE(config.isModuleAllowed("@minecraft/server-net"));
}

// ===== ScriptPlugin =====

TEST(ScriptPluginTest, ConstructionSetsProperties)
{
    ScriptPlugin plugin("uuid-123", "TestPlugin", "1.0.0", PluginExecutionGroup::ServerStart);
    EXPECT_EQ(plugin.uuid(), "uuid-123");
    EXPECT_EQ(plugin.name(), "TestPlugin");
    EXPECT_EQ(plugin.version(), "1.0.0");
    EXPECT_EQ(plugin.executionGroup(), PluginExecutionGroup::ServerStart);
    EXPECT_EQ(plugin.state(), ScriptPlugin::State::Unloaded);
    EXPECT_TRUE(plugin.errorMessage().empty());
}

TEST(ScriptPluginTest, StateNameReturnsCorrectStrings)
{
    EXPECT_STREQ(ScriptPlugin::stateName(ScriptPlugin::State::Unloaded), "Unloaded");
    EXPECT_STREQ(ScriptPlugin::stateName(ScriptPlugin::State::Loading), "Loading");
    EXPECT_STREQ(ScriptPlugin::stateName(ScriptPlugin::State::Loaded), "Loaded");
    EXPECT_STREQ(ScriptPlugin::stateName(ScriptPlugin::State::Running), "Running");
    EXPECT_STREQ(ScriptPlugin::stateName(ScriptPlugin::State::Error), "Error");
    EXPECT_STREQ(ScriptPlugin::stateName(ScriptPlugin::State::Unloading), "Unloading");
}

TEST(ScriptPluginTest, StartWithoutLoadFails)
{
    ScriptPlugin plugin("uuid-123", "TestPlugin", "1.0.0");
    auto result = plugin.start();
    EXPECT_TRUE(result.failed());
}

TEST(ScriptPluginTest, UnloadFromUnloadedIsNoop)
{
    ScriptPlugin plugin("uuid-123", "TestPlugin", "1.0.0");
    plugin.unload(); // Should not crash
    EXPECT_EQ(plugin.state(), ScriptPlugin::State::Unloaded);
}

TEST(ScriptPluginTest, MoveSemantics)
{
    ScriptPlugin plugin1("uuid-1", "Plugin1", "1.0.0");
    ScriptPlugin plugin2(std::move(plugin1));
    EXPECT_EQ(plugin2.uuid(), "uuid-1");
    EXPECT_EQ(plugin1.state(), ScriptPlugin::State::Unloaded);
}

// ===== ScriptLogger =====

TEST(ScriptLoggerTest, DefaultLogLevel)
{
    ScriptLogger logger;
    EXPECT_EQ(logger.logLevel(), 2); // spdlog::level::info
}

TEST(ScriptLoggerTest, SetLogLevel)
{
    ScriptLogger logger;
    logger.setLogLevel(4); // spdlog::level::err
    EXPECT_EQ(logger.logLevel(), 4);
}

// ===== ScriptWatchdog =====

TEST(ScriptWatchdogTest, DefaultConfig)
{
    ScriptWatchdog::Config config;
    EXPECT_EQ(config.tickTimeLimitMs, 50u);
    EXPECT_EQ(config.memoryLimitBytes, 64u * 1024 * 1024);
    EXPECT_TRUE(config.enabled);
}

TEST(ScriptWatchdogTest, BeginEndTickMeasuresTime)
{
    ScriptWatchdog::Config config;
    config.tickTimeLimitMs = 1000;
    ScriptWatchdog watchdog(config);

    watchdog.beginTick();
    // Simulate work — busy-wait to ensure measurable duration
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(2)) {
        // spin
    }
    watchdog.endTick();

    EXPECT_GT(watchdog.lastTickDurationMs(), 0u);
}

TEST(ScriptWatchdogTest, CheckExecutionTime)
{
    ScriptWatchdog::Config config;
    config.tickTimeLimitMs = 0; // Very low limit
    ScriptWatchdog watchdog(config);

    watchdog.beginTick();
    // Simulate work so duration > 0ms
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1)) {
        // spin
    }
    watchdog.endTick();

    // Should detect timeout since limit is 0ms and duration > 0ms
    EXPECT_TRUE(watchdog.checkExecutionTime());
}

// ===== ScriptDiagnostics =====

TEST(ScriptDiagnosticsTest, AddAndCount)
{
    ScriptDiagnostics diag;
    diag.add(DiagnosticLevel::Info, "test", "info message");
    diag.add(DiagnosticLevel::Warning, "test", "warn message");
    diag.add(DiagnosticLevel::Error, "test", "error message");

    EXPECT_EQ(diag.entries().size(), 3u);
    EXPECT_EQ(diag.count(DiagnosticLevel::Info), 1u);
    EXPECT_EQ(diag.count(DiagnosticLevel::Warning), 1u);
    EXPECT_EQ(diag.count(DiagnosticLevel::Error), 1u);
}

TEST(ScriptDiagnosticsTest, ClearRemovesAll)
{
    ScriptDiagnostics diag;
    diag.add(DiagnosticLevel::Error, "test", "error");
    diag.clear();
    EXPECT_TRUE(diag.entries().empty());
}

TEST(ScriptDiagnosticsTest, GenerateReport)
{
    ScriptDiagnostics diag;
    diag.add(DiagnosticLevel::Error, "plugin1", "something failed", "main.js", 42);
    std::string report = diag.generateReport();
    EXPECT_FALSE(report.empty());
    EXPECT_NE(report.find("1 entries"), std::string::npos);
    EXPECT_NE(report.find("plugin1"), std::string::npos);
    EXPECT_NE(report.find("main.js"), std::string::npos);
}

// ===== ScriptSentryLogger =====

TEST(ScriptSentryLoggerTest, LogMethodsDontCrash)
{
    ScriptSentryLogger logger;
    // These should just log to spdlog without crashing
    logger.logScriptLoaded("TestPlugin", "scripts/main.js");
    logger.logScriptError("TestPlugin", "TypeError: foo is not a function", "main.js", 10);
    logger.logScriptWarning("TestPlugin", "Deprecated API used");
    logger.logModuleRegistered("@minecraft/server", "2.0.0");
    logger.logWatchdogEvent("timeout", "Script execution exceeded 50ms");
}

// ===== ScriptManager =====

TEST(ScriptManagerTest, InitializationAndShutdown)
{
    ScriptManager manager;
    EXPECT_FALSE(manager.isInitialized());

    auto result = manager.initialize();
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(manager.isInitialized());

    manager.shutdown();
    EXPECT_FALSE(manager.isInitialized());
}

TEST(ScriptManagerTest, DoubleInitializeIsNoop)
{
    ScriptManager manager;
    auto result1 = manager.initialize();
    auto result2 = manager.initialize();
    EXPECT_TRUE(result1.success());
    EXPECT_TRUE(result2.success());
    manager.shutdown();
}

TEST(ScriptManagerTest, ShutdownWithoutInitIsNoop)
{
    ScriptManager manager;
    manager.shutdown(); // Should not crash
}

TEST(ScriptManagerTest, AccessorsWorkAfterInit)
{
    ScriptManager manager;
    manager.initialize();

    EXPECT_NO_THROW(manager.engine());
    EXPECT_NO_THROW(manager.pluginManager());
    EXPECT_NO_THROW(manager.eventBus());
    EXPECT_NO_THROW(manager.watchdog());
    EXPECT_NO_THROW(manager.logger());
    EXPECT_NO_THROW(manager.packList());

    manager.shutdown();
}

// ===== BehaviorPackList =====

TEST(BehaviorPackListTest, EmptyList)
{
    BehaviorPackList list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0u);
    EXPECT_TRUE(list.getEnabledPacks().empty());
    EXPECT_TRUE(list.getScriptModules().empty());
}

TEST(BehaviorPackListTest, GetPackByUuidReturnsNullptrForMissing)
{
    BehaviorPackList list;
    EXPECT_EQ(list.getPackByUuid("nonexistent"), nullptr);
}

// ===== AddonManifest =====

TEST(AddonManifestTest, HasScriptModuleReturnsFalseForEmpty)
{
    AddonManifest manifest;
    EXPECT_FALSE(manifest.hasScriptModule());
}

TEST(AddonManifestTest, ParseInvalidJsonReturnsError)
{
    auto result = AddonManifest::parse("not valid json");
    EXPECT_TRUE(result.failed());
}
