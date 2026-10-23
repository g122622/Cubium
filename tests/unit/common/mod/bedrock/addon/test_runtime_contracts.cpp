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

#include "common/mod/bedrock/addon/binding/ScriptCallbackHolder.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/event/AfterEventSignal.hpp"
#include "common/mod/bedrock/addon/event/BeforeEventSignal.hpp"
#include "common/mod/bedrock/addon/event/ScriptEventData.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptLogger.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"
#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {

class _MemorySource final : public IDependencyLoader {
public:
    std::unordered_map<std::string, std::string> files;
    std::vector<std::string> requested;

    std::optional<ScriptData> loadScript(const std::string& name) override
    {
        requested.push_back(name);
        const auto it = files.find(name);
        if (it == files.end()) {
            return std::nullopt;
        }

        return ScriptData(name, it->second, name, true);
    }
};

class AddonRuntimeContractTest : public testing::Test {
protected:
    void SetUp() override
    {
        engine = createScriptEngine();
        ASSERT_TRUE(engine->initialize());
        events.initialize();
        context = engine->runtime().createContext(ContextConfig{
            Capabilities{}, std::nullopt, DEFAULT_CONTEXT_MEMORY_BYTES, DEFAULT_CONTEXT_STACK_SIZE_BYTES});
        ASSERT_NE(context, nullptr);
        MinecraftModuleFactory factory;
        factory.setScheduler(&scheduler);
        factory.setEventBus(&events);
        factory.setEventSignals({{"entityHurt", typeid(int), true}, {"itemUse", typeid(float), false}});
        ASSERT_TRUE(factory.registerBindings(*context));
    }

    void TearDown() override
    {
        context.reset();
        scheduler.clearAll();
        events.shutdown();
        engine->shutdown();
    }

    void evaluate(const std::string& source)
    {
        const auto result = context->evaluateModule(source, "contracts/main.js");
        ASSERT_TRUE(result.success()) << result.errorMessage();
    }

    double number(const std::string& name)
    {
        const auto value = context->getGlobalVariable(name);
        EXPECT_TRUE(value.has_value());

        return value && value->isNumber() ? value->asNumber() : -1;
    }

    std::unique_ptr<IScriptEngine> engine;
    ScriptScheduler scheduler;
    ScriptEventBus events;
    std::unique_ptr<IScriptContext> context;
};

} // namespace

TEST(ScriptSchedulerContractTest, CancelRunAndCancelAnotherReadyCallback)
{
    ScriptScheduler scheduler;
    int calls = 0;
    const auto cancelled = scheduler.run([&]() {
        ++calls;
    });
    EXPECT_TRUE(scheduler.clearRun(cancelled));
    ScriptScheduler::RunId second = 0;
    scheduler.run([&]() {
        EXPECT_TRUE(scheduler.clearRun(second));
    });
    second = scheduler.run([&]() {
        ++calls;
    });
    scheduler.tick(1);
    EXPECT_EQ(calls, 0);
    EXPECT_EQ(scheduler.pendingCount(), 0u);
}

TEST(ScriptSchedulerContractTest, TimeoutUsesTickAtRegistration)
{
    ScriptScheduler scheduler;
    scheduler.tick(100);
    int calls = 0;
    scheduler.runTimeout(
        [&]() {
            ++calls;
        },
        3);
    scheduler.tick(101);
    scheduler.tick(102);
    EXPECT_EQ(calls, 0);
    scheduler.tick(103);
    EXPECT_EQ(calls, 1);
}

TEST(ScriptSchedulerContractTest, NestedRunWaitsUntilNextTick)
{
    ScriptScheduler scheduler;
    int calls = 0;
    scheduler.run([&]() {
        ++calls;
        scheduler.run([&]() {
            ++calls;
        });
    });
    scheduler.tick(1);
    EXPECT_EQ(calls, 1);
    scheduler.tick(2);
    EXPECT_EQ(calls, 2);
}

TEST(ScriptEventContractTest, BeforeCallbacksCanUnsubscribeAndCancellationIsReturned)
{
    BeforeEventSignal signal;
    int calls = 0;
    ScriptEventHandler subscription;
    subscription = signal.subscribe(typeid(int), [&](std::any& data) {
        ++calls;
        EXPECT_EQ(std::any_cast<int>(data), 7);
        EXPECT_TRUE(signal.unsubscribe(subscription));

        return true;
    });
    signal.subscribe(typeid(int), [&](std::any&) {
        ++calls;

        return false;
    });
    std::any data = 7;
    EXPECT_TRUE(signal.fire(typeid(int), data));
    EXPECT_EQ(calls, 2);
    EXPECT_FALSE(signal.fire(typeid(int), data));
    EXPECT_EQ(calls, 3);
}

TEST(ScriptEventContractTest, AfterCallbacksCanEnqueueAndUnsubscribe)
{
    AfterEventSignal signal;
    int total = 0;
    ScriptEventHandler subscription;
    subscription = signal.subscribe(typeid(int), [&](const std::any& data) {
        total += std::any_cast<int>(data);
        signal.unsubscribe(subscription);
        signal.enqueue(typeid(float), 3.0f);
    });
    signal.subscribe(typeid(float), [&](const std::any& data) {
        total += static_cast<int>(std::any_cast<float>(data));
    });
    signal.enqueue(typeid(int), 2);
    EXPECT_TRUE(signal.flush());
    EXPECT_EQ(total, 2);
    EXPECT_TRUE(signal.flush());
    EXPECT_EQ(total, 5);
}

TEST_F(AddonRuntimeContractTest, TimersRetainArgumentsAfterNativeCallReturns)
{
    evaluate(R"(
        import { system } from '@minecraft/server';
        globalThis.calls = 0;
        const interval = system.runInterval(() => { ++globalThis.calls; }, 2);
        system.run(() => { globalThis.calls += 10; });
        system.runTimeout(() => { system.clearRun(interval); globalThis.calls += 100; }, 5);
        for (let i = 0; i < 5000; ++i) { const garbage = { i, value: 'reuse temporary handles' }; }
    )");
    for (mc::u64 tick = 1; tick <= 6; ++tick) {
        scheduler.tick(tick);
    }

    EXPECT_EQ(number("calls"), 112);
    EXPECT_EQ(scheduler.pendingCount(), 0u);
}

TEST_F(AddonRuntimeContractTest, ClearRunCancelsRunFromJavascript)
{
    evaluate(R"(
        import { system } from '@minecraft/server';
        globalThis.calls = 0;
        system.clearRun(system.run(() => { ++globalThis.calls; }));
    )");
    scheduler.tick(1);
    EXPECT_EQ(number("calls"), 0);
}

TEST_F(AddonRuntimeContractTest, GeneratorsCompleteAndCanBeCancelled)
{
    evaluate(R"(
        import { system } from '@minecraft/server';
        globalThis.calls = 0;
        system.runJob((function* () {
            for (let i = 0; i < 4; ++i) { ++globalThis.calls; yield; }
        })());
        const cancelled = system.runJob((function* () { globalThis.calls += 100; yield; })());
        system.clearJob(cancelled);
    )");
    scheduler.tick(1);
    EXPECT_EQ(number("calls"), 4);
    EXPECT_EQ(scheduler.pendingCount(), 0u);
}

TEST_F(AddonRuntimeContractTest, ContextDisposalCancelsTimersAndGeneratorJobs)
{
    evaluate(R"(
        import { system } from '@minecraft/server';
        system.run(() => {});
        system.runInterval(() => {}, 2);
        system.runJob((function* () { while (true) yield; })());
    )");
    EXPECT_EQ(scheduler.pendingCount(), 3u);
    context.reset();
    EXPECT_EQ(scheduler.pendingCount(), 0u);
    scheduler.tick(1);
}

TEST_F(AddonRuntimeContractTest, PersistentValuesAreInvalidatedBeforeContextDestruction)
{
    evaluate("globalThis.callback = () => 42;");
    auto& bindings = context->bindingContext();
    void* global = bindings.getGlobalObject();
    void* function = bindings.getProperty(global, "callback");
    ScriptCallbackHolder callback(bindings, function);
    bindings.releaseValue(function);
    bindings.releaseValue(global);
    EXPECT_TRUE(callback.isValid());
    void* result = callback.call0();
    EXPECT_EQ(bindings.toInt32(result), 42);
    bindings.releaseValue(result);
    context.reset();
    EXPECT_FALSE(callback.isValid());
    EXPECT_EQ(callback.context(), nullptr);
}

TEST_F(AddonRuntimeContractTest, ScriptCancellationReachesNativeEventBus)
{
    evaluate(R"(
        import { world } from '@minecraft/server';
        world.beforeEvents.entityHurt.subscribe(event => { event.cancel = true; });
    )");
    std::any data = ScriptEventData{};
    EXPECT_TRUE(events.dispatchBeforeEvent(typeid(int), data));
    context.reset();
    EXPECT_EQ(events.beforeEvents().handlerCount(typeid(int)), 0u);
}

TEST(AddonModuleContractTest, CanonicalImportsShareOneModuleInstance)
{
    auto engine = createScriptEngine();
    ASSERT_TRUE(engine->initialize());
    _MemorySource source;
    source.files["pack/scripts/value.js"] = "globalThis.loads = (globalThis.loads || 0) + 1; export const value = 7;";
    ScriptLogger logger;
    ModuleDescriptor descriptor;
    descriptor.name = "canonical";
    auto context = engine->createContext(descriptor, {}, source, logger);
    ASSERT_NE(context, nullptr);
    const auto result = context->evaluateModule(R"(
        import { value as a } from './value';
        import { value as b } from './nested/../value.js';
        globalThis.sum = a + b;
    )",
        "pack/scripts/main.js");
    ASSERT_TRUE(result.success()) << result.errorMessage();
    EXPECT_EQ(source.requested, std::vector<std::string>{"pack/scripts/value.js"});
    EXPECT_EQ(context->getGlobalVariable("loads")->asNumber(), 1);
    EXPECT_EQ(context->getGlobalVariable("sum")->asNumber(), 14);
    context.reset();
    engine->shutdown();
}

TEST(AddonModuleContractTest, EachContextUsesItsOwnSourceProvider)
{
    auto engine = createScriptEngine();
    ASSERT_TRUE(engine->initialize());
    _MemorySource firstSource;
    _MemorySource secondSource;
    firstSource.files["pack/value.js"] = "export const value = 11;";
    secondSource.files["pack/value.js"] = "export const value = 29;";
    ScriptLogger logger;
    ModuleDescriptor descriptor;
    descriptor.name = "first";
    auto first = engine->createContext(descriptor, {}, firstSource, logger);
    descriptor.name = "second";
    auto second = engine->createContext(descriptor, {}, secondSource, logger);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    const std::string script = "import { value } from './value.js'; globalThis.result = value;";
    const auto firstResult = first->evaluateModule(script, "pack/main.js");
    ASSERT_TRUE(firstResult.success()) << firstResult.errorMessage();
    const auto secondResult = second->evaluateModule(script, "pack/main.js");
    ASSERT_TRUE(secondResult.success()) << secondResult.errorMessage();
    EXPECT_EQ(first->getGlobalVariable("result")->asNumber(), 11);
    EXPECT_EQ(second->getGlobalVariable("result")->asNumber(), 29);
    second.reset();
    first.reset();
    engine->shutdown();
}

TEST_F(AddonRuntimeContractTest, SubscribeReturnsCallbackAndUnsubscribeAcceptsIt)
{
    evaluate(R"(
        import { world } from '@minecraft/server';
        globalThis.calls = 0;
        const callback = event => {
            globalThis.calls += event.amount;
            world.afterEvents.itemUse.unsubscribe(callback);
        };
        globalThis.identity = world.afterEvents.itemUse.subscribe(callback) === callback ? 1 : 0;
    )");
    ScriptEventData data;
    data.setNumber("amount", 7);
    events.enqueueAfterEvent(typeid(float), data);
    events.enqueueAfterEvent(typeid(float), data);
    events.tick();
    EXPECT_EQ(number("identity"), 1);
    EXPECT_EQ(number("calls"), 7);
    EXPECT_EQ(events.afterEvents().handlerCount(typeid(float)), 0u);
}

TEST_F(AddonRuntimeContractTest, ScriptMessagesApplyNamespaceFilter)
{
    evaluate(R"(
        import { system } from '@minecraft/server';
        globalThis.messages = 0;
        system.afterEvents.scriptEventReceive.subscribe(event => {
            if (event.message === 'payload' && event.sourceType === 'Server') ++globalThis.messages;
        }, { namespaces: ['thlm'] });
        system.sendScriptEvent('thlm:skin', 'payload');
        system.sendScriptEvent('another:skin', 'payload');
    )");
    events.tick();
    EXPECT_EQ(number("messages"), 1);
}

TEST_F(AddonRuntimeContractTest, StartupExposesRegistriesOwnedByCurrentContext)
{
    evaluate(R"(
        import { system } from '@minecraft/server';
        globalThis.registries = 0;
        system.beforeEvents.startup.subscribe(event => {
            if (typeof event.blockComponentRegistry.registerCustomComponent === 'function') ++globalThis.registries;
            if (typeof event.itemComponentRegistry.registerCustomComponent === 'function') ++globalThis.registries;
        });
    )");
    std::any data = ScriptEventData{};
    events.dispatchBeforeEvent(typeid(ScriptStartupEvent), data);
    EXPECT_EQ(number("registries"), 2);
}

TEST_F(AddonRuntimeContractTest, PlayerPrototypeInheritsEntityPrototype)
{
    evaluate(R"(
        import { Player, Entity, GameMode } from '@minecraft/server';
        globalThis.inherits = Object.getPrototypeOf(Player.prototype) === Entity.prototype ? 1 : 0;
        globalThis.creative = GameMode.Creative === 'Creative' ? 1 : 0;
    )");
    EXPECT_EQ(number("inherits"), 1);
    EXPECT_EQ(number("creative"), 1);
}
