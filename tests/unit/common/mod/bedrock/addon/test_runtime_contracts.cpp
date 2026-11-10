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
    // 官方 @minecraft/server 的 GameMode 是**字符串枚举**，键名与值都是全小写
    // （`creative = 'creative'`，见 index.d.ts）。此前实现与用例都写成首字母大写，
    // 导致 `GameMode.survival` 取到 undefined、spawnSimulatedPlayer 静默落回 Creative。
    evaluate(R"(
        import { Player, Entity, GameMode } from '@minecraft/server';
        globalThis.inherits = Object.getPrototypeOf(Player.prototype) === Entity.prototype ? 1 : 0;
        globalThis.creative = GameMode.creative === 'creative' ? 1 : 0;
        globalThis.survival = GameMode.survival === 'survival' ? 1 : 0;
        globalThis.adventure = GameMode.adventure === 'adventure' ? 1 : 0;
        globalThis.spectator = GameMode.spectator === 'spectator' ? 1 : 0;
    )");
    EXPECT_EQ(number("inherits"), 1);
    EXPECT_EQ(number("creative"), 1);
    EXPECT_EQ(number("survival"), 1);
    EXPECT_EQ(number("adventure"), 1);
    EXPECT_EQ(number("spectator"), 1);
}

// 顶层 await 的模块若不断自我追加微任务，Promise 永远不会 settle，而 job 队列永远非空。
// 旧实现只靠"settle 或 无 job 可执行"两个出口，遇到这种模块会卡死整个服务端。
// 现在有 job 执行预算：超限即报错返回，让有问题的行为包在加载期就明确失败。
TEST_F(AddonRuntimeContractTest, SelfSchedulingModuleFailsInsteadOfHanging)
{
    const auto result = context->evaluateModule(R"(
        await new Promise(() => {
            const spin = () => { queueMicrotask(spin); };
            queueMicrotask(spin);
        });
    )",
        "contracts/runaway.js");

    ASSERT_FALSE(result.success()) << "自调度模块必须在预算耗尽后失败，而不是无限等待";
    EXPECT_NE(result.errorMessage().find("pending-job budget"), std::string::npos) << result.errorMessage();
}

// 脚本对象/数组跨原生边界：此前 convertArgsToScriptValues 对非标量一律降级成 undefined，
// convertScriptValueToJSValue 对 Object/Array 也返回 undefined。表现是注册给脚本的原生函数
// 收到对象参数时静默变成 undefined——脚本侧看不到任何错误，只是数据凭空消失。
// 现在统一以 JSON 承载，双向都能完整往返。
TEST_F(AddonRuntimeContractTest, ObjectAndArrayArgumentsRoundTripThroughNativeBoundary)
{
    // 只验证对象形态；数组形态由下一条用例单独覆盖（两者在脚本侧的语义不同）。
    std::vector<ScriptValue> objectArg;
    ASSERT_TRUE(context->registerGlobalFunction(
        "__captureObject", [&objectArg](const std::vector<ScriptValue>& args) -> ScriptValue {
            objectArg = args;
            return ScriptValue();
        }));
    evaluate(R"(__captureObject({ name: 'zombie', health: 20, nested: { flag: true }, list: [1, 2, 3] });)");
    ASSERT_EQ(objectArg.size(), 1u);
    EXPECT_TRUE(objectArg[0].isObject());
    EXPECT_EQ(objectArg[0].asJson()["name"], "zombie");
    EXPECT_EQ(objectArg[0].asJson()["health"], 20);
    EXPECT_TRUE(objectArg[0].asJson()["nested"]["flag"]);
    ASSERT_TRUE(objectArg[0].asJson()["list"].is_array());
    EXPECT_EQ(objectArg[0].asJson()["list"].size(), 3u);
}

TEST_F(AddonRuntimeContractTest, ArrayArgumentIsDistinguishedFromObject)
{
    std::vector<ScriptValue> received;
    ASSERT_TRUE(context->registerGlobalFunction(
        "__captureArray", [&received](const std::vector<ScriptValue>& args) -> ScriptValue {
            received = args;
            return ScriptValue();
        }));

    evaluate(R"(__captureArray([10, 'twenty', false, null]);)");

    ASSERT_EQ(received.size(), 1u);
    // 数组必须被识别为 Array 而不是 Object：两者在脚本侧语义不同（length、索引、迭代器）。
    EXPECT_TRUE(received[0].isArray());
    EXPECT_FALSE(received[0].isObject());
    ASSERT_TRUE(received[0].asJson().is_array());
    EXPECT_EQ(received[0].asJson()[0], 10);
    EXPECT_EQ(received[0].asJson()[1], "twenty");
    EXPECT_EQ(received[0].asJson()[2], false);
    EXPECT_TRUE(received[0].asJson()[3].is_null());
}

// 原生侧返回对象/数组时，脚本必须拿到真实值而不是 undefined。
TEST_F(AddonRuntimeContractTest, NativeReturnValueCanBeAnObjectOrArray)
{
    ASSERT_TRUE(context->registerGlobalFunction("__makeObject", [](const std::vector<ScriptValue>&) -> ScriptValue {
        nlohmann::json object = {{"id", "minecraft:zombie"}, {"count", 3}, {"tags", {"hostile", "undead"}}};
        return ScriptValue(std::move(object), false);
    }));
    ASSERT_TRUE(context->registerGlobalFunction("__makeArray", [](const std::vector<ScriptValue>&) -> ScriptValue {
        nlohmann::json array = nlohmann::json::array({1, "two", true});
        return ScriptValue(std::move(array), true);
    }));

    evaluate(R"(
        globalThis.fromObject = __makeObject();
        globalThis.fromArray = __makeArray();
    )");

    const auto id = context->getGlobalVariable("fromObject");
    ASSERT_TRUE(id.has_value());

    // 脚本侧直接读属性，验证拿到的是真实对象而非 undefined。
    evaluate(R"(
        globalThis.objectId = __makeObject().id;
        globalThis.objectCount = __makeObject().count;
        globalThis.objectTagCount = __makeObject().tags.length;
        globalThis.arrayLength = __makeArray().length;
        globalThis.arraySecond = __makeArray()[1];
        globalThis.arrayIsArray = Array.isArray(__makeArray());
    )");

    const auto objectId = context->getGlobalVariable("objectId");
    ASSERT_TRUE(objectId.has_value());
    EXPECT_EQ(objectId->asString(), "minecraft:zombie");

    const auto objectCount = context->getGlobalVariable("objectCount");
    ASSERT_TRUE(objectCount.has_value());
    EXPECT_DOUBLE_EQ(objectCount->asNumber(), 3.0);

    const auto objectTagCount = context->getGlobalVariable("objectTagCount");
    ASSERT_TRUE(objectTagCount.has_value());
    EXPECT_DOUBLE_EQ(objectTagCount->asNumber(), 2.0);

    const auto arrayLength = context->getGlobalVariable("arrayLength");
    ASSERT_TRUE(arrayLength.has_value());
    EXPECT_DOUBLE_EQ(arrayLength->asNumber(), 3.0);

    const auto arraySecond = context->getGlobalVariable("arraySecond");
    ASSERT_TRUE(arraySecond.has_value());
    EXPECT_EQ(arraySecond->asString(), "two");

    const auto arrayIsArray = context->getGlobalVariable("arrayIsArray");
    ASSERT_TRUE(arrayIsArray.has_value());
    EXPECT_TRUE(arrayIsArray->asBoolean());
}

// callFunction / setGlobalVariable / getGlobalVariable 走的是成员函数 _jsValueToScriptValue
// 与 _scriptValueToJSValue，与全局函数回调路径是两套实现。上一轮只补了回调路径，
// 成员函数路径仍然把对象/数组降级成 undefined——同一个缺陷有两个入口。
TEST_F(AddonRuntimeContractTest, CallFunctionPassesAndReturnsObjects)
{
    evaluate(R"(
        globalThis.echoObject = (payload) => ({ received: payload, size: payload.items.length });
    )");

    nlohmann::json payload = {{"items", {1, 2, 3}}, {"label", "stack"}};
    std::vector<ScriptValue> args;
    args.emplace_back(std::move(payload), false);

    const auto result = context->callFunction("echoObject", args);
    ASSERT_TRUE(result.success()) << result.errorMessage();

    ASSERT_TRUE(result.value().isObject());
    EXPECT_EQ(result.value().asJson()["size"], 3);
    EXPECT_EQ(result.value().asJson()["received"]["label"], "stack");
    ASSERT_TRUE(result.value().asJson()["received"]["items"].is_array());
    EXPECT_EQ(result.value().asJson()["received"]["items"][2], 3);
}

TEST_F(AddonRuntimeContractTest, CallFunctionPassesArrays)
{
    evaluate(R"(
        globalThis.sumArray = (values) => values.reduce((total, next) => total + next, 0);
    )");

    nlohmann::json values = nlohmann::json::array({1, 2, 3, 4});
    std::vector<ScriptValue> args;
    args.emplace_back(std::move(values), true);

    const auto result = context->callFunction("sumArray", args);
    ASSERT_TRUE(result.success()) << result.errorMessage();
    ASSERT_TRUE(result.value().isNumber());
    // 数组若被降级成 undefined，reduce 会抛错，因此这里同时验证了参数确实到达。
    EXPECT_DOUBLE_EQ(result.value().asNumber(), 10.0);
}

TEST_F(AddonRuntimeContractTest, GlobalVariablesCarryObjectsAndArrays)
{
    nlohmann::json object = {{"id", "minecraft:zombie"}, {"count", 3}};
    ASSERT_TRUE(context->setGlobalVariable("objVar", ScriptValue(std::move(object), false)));

    nlohmann::json array = nlohmann::json::array({"a", "b"});
    ASSERT_TRUE(context->setGlobalVariable("arrVar", ScriptValue(std::move(array), true)));

    evaluate(R"(
        globalThis.objReadBack = objVar.id;
        globalThis.arrReadBack = arrVar.length;
        globalThis.arrIsArray = Array.isArray(arrVar);
    )");

    const auto objReadBack = context->getGlobalVariable("objReadBack");
    ASSERT_TRUE(objReadBack.has_value());
    EXPECT_EQ(objReadBack->asString(), "minecraft:zombie");

    const auto arrReadBack = context->getGlobalVariable("arrReadBack");
    ASSERT_TRUE(arrReadBack.has_value());
    EXPECT_DOUBLE_EQ(arrReadBack->asNumber(), 2.0);

    const auto arrIsArray = context->getGlobalVariable("arrIsArray");
    ASSERT_TRUE(arrIsArray.has_value());
    EXPECT_TRUE(arrIsArray->asBoolean());

    // 读取路径同样要还原成对象，而不是 undefined
    const auto objVar = context->getGlobalVariable("objVar");
    ASSERT_TRUE(objVar.has_value());
    ASSERT_TRUE(objVar->isObject());
    EXPECT_EQ(objVar->asJson()["id"], "minecraft:zombie");

    const auto arrVar = context->getGlobalVariable("arrVar");
    ASSERT_TRUE(arrVar.has_value());
    ASSERT_TRUE(arrVar->isArray());
    EXPECT_EQ(arrVar->asJson().size(), 2u);
}
