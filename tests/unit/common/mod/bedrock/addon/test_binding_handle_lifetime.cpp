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

// 绑定层句柄生命周期测试：类原型句柄的所有权归绑定上下文（借用语义），
// 上下文析构时必须全部归还，否则句柄自身与其持有的 JS 类原型引用图一并泄漏。
// 回归背景：nightly 集成测试 20/24 分片因 LeakSanitizer 报出 5.7 MB / 55013 次分配泄漏
// 而以 SIGABRT 退出（句柄未释放 → 原型引用计数非 0 → JS_FreeRuntime 回收不掉原型对象）。

#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"

#include <cstddef>
#include <optional>

#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {

ContextConfig _makeConfig()
{
    // ContextConfig 无默认值，必须显式传入内存/栈上限（见 core/README.md 坑 2）。
    return ContextConfig{
        Capabilities{},                   // 无特殊能力
        std::nullopt,                     // 无特殊权限
        DEFAULT_CONTEXT_MEMORY_BYTES,     // 64MB 内存限制
        DEFAULT_CONTEXT_STACK_SIZE_BYTES, // 4MB 栈限制
    };
}

std::size_t _probeHandleCount(IScriptEngine& engine)
{
    auto probe = engine.runtime().createContext(_makeConfig());
    if (probe == nullptr) {
        return static_cast<std::size_t>(-1);
    }
    return probe->bindingContext().liveClassProtoHandleCount();
}

} // namespace

TEST(ScriptBindingHandleLifetimeTest, ContextDestructionReleasesClassProtoHandles)
{
    auto engine = createScriptEngine();
    ASSERT_NE(engine, nullptr);
    ASSERT_TRUE(engine->initialize());

    const std::size_t baseline = _probeHandleCount(*engine);
    ASSERT_NE(baseline, static_cast<std::size_t>(-1));

    constexpr int kProtoCount = 8;
    {
        auto context = engine->runtime().createContext(_makeConfig());
        ASSERT_NE(context, nullptr);
        auto& bindings = context->bindingContext();

        for (int i = 0; i < kProtoCount; ++i) {
            const mc::u64 classId = bindings.allocateClassId();
            ASSERT_TRUE(bindings.registerClass(classId, "HandleProbeClass", true));
            void* proto = bindings.createClassProto(classId);
            ASSERT_NE(proto, nullptr);
            bindings.registerMethod(proto,
                "probe",
                [](IScriptBindingContext& ctx, void* /*thisVal*/, mc::i32 /*argc*/, void** /*args*/) -> void* {
                    return ctx.createInt32(1);
                });
        }

        // 上下文存活期间句柄必须在位（调用方借用，不得释放）。
        EXPECT_EQ(bindings.liveClassProtoHandleCount(), baseline + kProtoCount);
    }

    // 上下文析构后：句柄所有权归上下文，必须已全部释放（否则句柄与原型引用图双双泄漏）。
    EXPECT_EQ(_probeHandleCount(*engine), baseline)
        << "绑定上下文析构后仍有类原型句柄未释放（createClassProto 的句柄所有权未归还）";

    engine->shutdown();
}

TEST(ScriptBindingHandleLifetimeTest, ClassRegistriesAreIsolatedBetweenContexts)
{
    auto engine = createScriptEngine();
    ASSERT_TRUE(engine->initialize());
    auto first = engine->runtime().createContext(_makeConfig());
    auto second = engine->runtime().createContext(_makeConfig());
    auto& firstBindings = first->bindingContext();
    auto& secondBindings = second->bindingContext();
    const auto firstId = firstBindings.allocateClassId();
    const auto secondId = secondBindings.allocateClassId();
    ASSERT_TRUE(firstBindings.registerClass(firstId, "Entity", true));
    ASSERT_TRUE(secondBindings.registerClass(secondId, "Entity", true));
    void* firstProto = firstBindings.createClassProto(firstId);
    void* secondProto = secondBindings.createClassProto(secondId);
    firstBindings.classRegistry().registerClass(firstId, firstProto, "Entity");
    secondBindings.classRegistry().registerClass(secondId, secondProto, "Entity");
    EXPECT_EQ(firstBindings.classRegistry().classIdByName("Entity"), firstId);
    EXPECT_EQ(secondBindings.classRegistry().classIdByName("Entity"), secondId);
    EXPECT_EQ(firstBindings.classRegistry().protoByName("Entity"), firstProto);
    first.reset();
    EXPECT_EQ(secondBindings.classRegistry().protoByName("Entity"), secondProto);
    EXPECT_EQ(secondBindings.classRegistry().classIdByName("Entity"), secondId);
    second.reset();
    engine->shutdown();
}

TEST(ScriptBindingHandleLifetimeTest, RepeatedContextRecreationDoesNotAccumulateHandles)
{
    // 引擎在同一进程内反复重建上下文（GameTestServer 多次运行、脚本热重载）时，
    // 句柄计数不得随次数单调增长。
    auto engine = createScriptEngine();
    ASSERT_NE(engine, nullptr);
    ASSERT_TRUE(engine->initialize());

    const std::size_t baseline = _probeHandleCount(*engine);
    ASSERT_NE(baseline, static_cast<std::size_t>(-1));

    for (int round = 0; round < 4; ++round) {
        auto context = engine->runtime().createContext(_makeConfig());
        ASSERT_NE(context, nullptr);
        auto& bindings = context->bindingContext();
        const mc::u64 classId = bindings.allocateClassId();
        ASSERT_TRUE(bindings.registerClass(classId, "RoundProbeClass", true));
        ASSERT_NE(bindings.createClassProto(classId), nullptr);
        EXPECT_EQ(bindings.liveClassProtoHandleCount(), baseline + 1);
    }

    EXPECT_EQ(_probeHandleCount(*engine), baseline);

    engine->shutdown();
}
