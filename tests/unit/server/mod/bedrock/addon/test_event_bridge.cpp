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

#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"
#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"
#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/mod/bedrock/addon/bridge/EventBridge.hpp"
#include "server/mod/bedrock/addon/bridge/ServerEventSignals.hpp"

#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {
class AddonEventBridgeContractTest : public testing::Test {
protected:
    void SetUp() override
    {
        ScriptWorldAccessor::instance().setGetDimensionsCallback([]() {
            return std::vector<mc::IWorld*>{};
        });
        engine = createScriptEngine();
        ASSERT_TRUE(engine->initialize());
        events.initialize();
        bridge.initialize(mc::server::event::ServerEventBus::instance(), events);
        context = engine->runtime().createContext(ContextConfig{
            Capabilities{}, std::nullopt, DEFAULT_CONTEXT_MEMORY_BYTES, DEFAULT_CONTEXT_STACK_SIZE_BYTES});
        auto signals = mc::server::getBeforeEventSignals();
        const auto after = mc::server::getAfterEventSignals();
        signals.insert(signals.end(), after.begin(), after.end());
        MinecraftModuleFactory factory;
        factory.setEventBus(&events);
        factory.setEventSignals(signals);
        ASSERT_TRUE(factory.registerBindings(*context));
    }

    void TearDown() override
    {
        bridge.shutdown();
        context.reset();
        events.shutdown();
        engine->shutdown();
        ScriptWorldAccessor::instance().setGetDimensionsCallback({});
    }

    std::unique_ptr<IScriptEngine> engine;
    ScriptEventBus events;
    mc::server::EventBridge bridge;
    std::unique_ptr<IScriptContext> context;
};
} // namespace

TEST_F(AddonEventBridgeContractTest, BeforeCancellationUpdatesNativeEvent)
{
    const auto script = context->evaluateModule(R"(
        import { world } from '@minecraft/server';
        world.beforeEvents.chatSend.subscribe(event => {
            if (event.message === 'cancel me') event.cancel = true;
        });
    )",
        "before-event.js");
    ASSERT_TRUE(script.success()) << script.errorMessage();
    mc::server::event::ChatEvent cancelled(1, 7, "cancel me", "tester");
    mc::server::event::ChatEvent allowed(1, 7, "allow me", "tester");
    mc::server::event::ServerEventBus::instance().publish(cancelled);
    mc::server::event::ServerEventBus::instance().publish(allowed);
    EXPECT_TRUE(cancelled.isCancelled());
    EXPECT_FALSE(allowed.isCancelled());
}

TEST_F(AddonEventBridgeContractTest, DeferredEventsOwnTheirStringPayload)
{
    const auto script = context->evaluateModule(R"(
        import { world } from '@minecraft/server';
        globalThis.snapshot = '';
        world.afterEvents.chatSend.subscribe(event => { globalThis.snapshot = event.playerName + ':' + event.message; });
    )",
        "after-event.js");
    ASSERT_TRUE(script.success()) << script.errorMessage();
    {
        mc::server::event::ChatEvent native(1, 7, "original", "tester");
        mc::server::event::ServerEventBus::instance().publish(native);
        native.message = "modified after publish";
    }

    events.tick();
    const auto result = context->getGlobalVariable("snapshot");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->asString(), "tester:original");
}
